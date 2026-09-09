#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <atomic>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/version.hpp>
#if BOOST_VERSION >= 108600
#include <boost/process/v1.hpp>
#else
#include <boost/process.hpp>
#endif
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <lowwi.hpp>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <windows.h>

#include <onnxruntime_c_api.h>
#else
#include <csignal>
#include <fcntl.h>
#include <unistd.h>
#endif

#ifdef __linux__
#include <iterator>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#if BOOST_VERSION >= 108600
namespace bp = boost::process::v1;
#else
namespace bp = boost::process;
#endif

static constexpr std::int64_t kStaleMs = 5000;
static constexpr int kPollSeconds = 2;

static std::atomic_bool g_running = false;
static std::atomic<std::int64_t> g_lastcallback = 0;

#ifdef _WIN32
static HANDLE g_reloadevent = nullptr;
#else
static volatile sig_atomic_t g_reload = 0;
#endif

static std::int64_t now_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

static std::filesystem::path log_path() {
    std::error_code ec;
    return std::filesystem::temp_directory_path(ec) / "ultijarvis.log";
}

static void jlog(const std::string &msg) {
    std::ofstream f(log_path(), std::ios::app);
    if (!f.is_open())
        return;
    std::time_t t = std::time(nullptr);
    char stamp[32] = {0};
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", &tm);
    f << stamp << "  " << msg << "\n";
}

static std::filesystem::path exe_dir() {
    return std::filesystem::path(
        boost::dll::program_location().parent_path().string());
}

#ifdef __linux__
static void jarvis_confine_plugin_paths() {
    std::string dir = boost::dll::program_location().parent_path().string();
    setenv("OPENSSL_MODULES", (dir + "/ossl-modules").c_str(), 0);
    setenv("OPENSSL_ENGINES", (dir + "/engines-3").c_str(), 0);
    setenv("GIO_MODULE_DIR", (dir + "/gio-modules").c_str(), 0);
    setenv("SASL_PATH", (dir + "/sasl2").c_str(), 0);
}
#endif

static void on_terminate() {
    try {
        std::exception_ptr e = std::current_exception();
        if (e)
            std::rethrow_exception(e);
        jlog("FATAL: terminate called with no active exception");
    } catch (const std::exception &ex) {
        jlog(std::string("FATAL: unhandled exception: ") + ex.what());
    } catch (...) {
        jlog("FATAL: unhandled exception of unknown type");
    }
    std::_Exit(1);
}

#ifdef _WIN32
static LONG WINAPI crash_filter(EXCEPTION_POINTERS *info) {
    char buf[160];
    std::snprintf(
        buf, sizeof(buf), "FATAL: exception 0x%08lX at %p",
        static_cast<unsigned long>(info->ExceptionRecord->ExceptionCode),
        info->ExceptionRecord->ExceptionAddress);
    jlog(buf);
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

static void install_crash_handlers() {
    std::set_terminate(on_terminate);
#ifdef _WIN32
    SetUnhandledExceptionFilter(crash_filter);
#endif
}

static std::filesystem::path app_path() {
#ifdef _WIN32
    return exe_dir() / "Ulti Jarvis.exe";
#else
    return exe_dir() / "Ulti Jarvis";
#endif
}

static std::filesystem::path pid_path() {
    std::error_code ec;
    return std::filesystem::temp_directory_path(ec) / "ultijarvis.pid";
}

static std::filesystem::path lock_path() {
    std::error_code ec;
    return std::filesystem::temp_directory_path(ec) / "ultijarvis.lock";
}

#ifdef __linux__

static void adopt_session_environment() {
    static const char *wanted[] = {"DISPLAY",
                                   "WAYLAND_DISPLAY",
                                   "XAUTHORITY",
                                   "XDG_RUNTIME_DIR",
                                   "XDG_SESSION_TYPE",
                                   "XDG_CURRENT_DESKTOP",
                                   "DBUS_SESSION_BUS_ADDRESS"};
    if (!getenv("DISPLAY") && !getenv("WAYLAND_DISPLAY")) {
        uid_t self = getuid();
        std::error_code ec;
        for (const std::filesystem::directory_entry &entry :
             std::filesystem::directory_iterator("/proc", ec)) {
            std::string pid = entry.path().filename().string();
            if (pid.find_first_not_of("0123456789") != std::string::npos)
                continue;
            struct stat st;
            if (stat(entry.path().c_str(), &st) != 0 || st.st_uid != self)
                continue;
            std::ifstream f(entry.path() / "environ", std::ios::binary);
            if (!f.is_open())
                continue;
            std::string blob((std::istreambuf_iterator<char>(f)),
                             std::istreambuf_iterator<char>());
            if (blob.find("DISPLAY=") == std::string::npos)
                continue;
            bool adopted = false;
            for (std::size_t i = 0; i < blob.size();) {
                std::size_t end = blob.find('\0', i);
                if (end == std::string::npos)
                    end = blob.size();
                std::string pair = blob.substr(i, end - i);
                i = end + 1;
                std::size_t eq = pair.find('=');
                if (eq == std::string::npos)
                    continue;
                std::string key = pair.substr(0, eq);
                for (const char *name : wanted)
                    if (key == name) {
                        setenv(name, pair.c_str() + eq + 1, 1);
                        adopted = true;
                    }
            }
            if (adopted) {
                jlog("took the desktop session environment from pid " + pid);
                break;
            }
        }
    }
    if (!getenv("DISPLAY") && !getenv("WAYLAND_DISPLAY"))
        jlog("no desktop session was found, the window will not be able to "
             "open");
    else if (!getenv("DISPLAY"))
        setenv("QT_QPA_PLATFORM", "wayland", 1);
}

#endif

void wakeword_callback(CLFML::LOWWI::Lowwi_ctx_t, std::shared_ptr<void>) {
    jlog("wake word detected");
    if (g_running.exchange(true)) {
        jlog("app already running, ignoring");
        return;
    }
    std::thread([]() {
        try {
#ifdef __linux__
            adopt_session_environment();
#endif
            jlog("launching " + app_path().string());
            bp::child c(boost::filesystem::path(app_path().string()));
            c.wait();
            jlog("app exited with " + std::to_string(c.exit_code()));
        } catch (const std::exception &e) {
            jlog(std::string("app launch failed: ") + e.what());
        } catch (...) {
            jlog("app launch failed");
        }
        g_running.store(false);
    }).detach();
}

static void call_back(ma_device *pDevice, void *, const void *pInput,
                      ma_uint32 frameCount) {
    g_lastcallback.store(now_ms());
    auto *runtime = static_cast<CLFML::LOWWI::Lowwi *>((*pDevice).pUserData);
    const float *samples = static_cast<const float *>(pInput);
    (*runtime).run(std::vector<float>(samples, samples + frameCount));
}

static std::string config_dir() {
#ifdef _WIN32
    const char *appdata = getenv("APPDATA");
    std::string base =
        appdata && *appdata ? std::string(appdata) : std::string();
#elif defined(__APPLE__)
    const char *home = getenv("HOME");
    std::string base =
        std::string(home ? home : "") + "/Library/Application Support";
#else
    const char *xdg = getenv("XDG_CONFIG_HOME");
    const char *home = getenv("HOME");
    std::string base = xdg && *xdg ? std::string(xdg)
                                   : std::string(home ? home : "") + "/.config";
#endif
    return base + "/ultijarvis";
}

static std::string config_value(const char *key) {
    std::ifstream f(config_dir() + "/config");
    std::string line;
    std::string want = std::string(key) + "=";
    while (std::getline(f, line))
        if (line.rfind(want, 0) == 0)
            return line.substr(want.size());
    return "";
}

static bool take_reload() {
#ifdef _WIN32
    if (!g_reloadevent)
        return false;
    return WaitForSingleObject(g_reloadevent, 0) == WAIT_OBJECT_0;
#else
    if (!g_reload)
        return false;
    g_reload = 0;
    return true;
#endif
}

#ifndef _WIN32
static void on_hup(int) { g_reload = 1; }
#endif

static void detach_from_terminal() {
#ifndef _WIN32
#if defined(NDEBUG) && !defined(__APPLE__)
    // IMPORTANT: DAEMON WONT WORK CONTINUOUSLY IN DEBUG MODE,
    // SET CMAKE TO RELEASE BEFORE BUILDING
    if (fork() > 0)
        _exit(0);
    setsid();
#endif
    int devnull = open("/dev/null", O_RDWR);
    dup2(devnull, STDIN_FILENO);
    dup2(devnull, STDOUT_FILENO);
    dup2(devnull, STDERR_FILENO);
    close(devnull);
#endif
}

static bool open_capture(ma_context *context, ma_device *device,
                         ma_device_config *config, ma_device_id *matched_id,
                         CLFML::LOWWI::Lowwi *runtime) {
    ma_device_info *captureInfos;
    ma_uint32 captureCount = 0;
    ma_result gd = ma_context_get_devices(context, nullptr, nullptr,
                                          &captureInfos, &captureCount);
    std::string report = "capture devices: " + std::to_string(captureCount) +
                         " (get_devices=" + std::to_string(int(gd)) + ")";
    for (ma_uint32 i = 0; i < captureCount; i++)
        report +=
            "\n    device " + std::to_string(i) + ": " + captureInfos[i].name;
    *config = ma_device_config_init(ma_device_type_capture);
    (*config).capture.format = ma_format_f32;
    (*config).capture.channels = 1;
    (*config).sampleRate = 16000;
    (*config).dataCallback = call_back;
    (*config).pUserData = runtime;
    (*config).capture.pDeviceID = nullptr;
    std::string confdev = config_value("device");
    for (ma_uint32 i = 0; i < captureCount && !confdev.empty(); i++) {
        if (confdev == captureInfos[i].name) {
            *matched_id = captureInfos[i].id;
            (*config).capture.pDeviceID = matched_id;
            break;
        }
    }
    report += "\n    configured: " +
              std::string(confdev.empty() ? "<default>" : confdev) +
              ((*config).capture.pDeviceID ? " (matched)" : " (using default)");
    ma_result ir = ma_device_init(context, config, device);
    report += "\n    ma_device_init = " + std::to_string(int(ir));
    ma_result sr = MA_ERROR;
    if (ir == MA_SUCCESS) {
        sr = ma_device_start(device);
        report += "\n    ma_device_start = " + std::to_string(int(sr));
        if (sr != MA_SUCCESS)
            ma_device_uninit(device);
    }
    static std::string previous;
    if (report != previous) {
        previous = report;
        jlog(report);
    }
    if (ir != MA_SUCCESS || sr != MA_SUCCESS)
        return false;
    g_lastcallback.store(now_ms());
    return true;
}

int main() {
#ifdef __linux__
    jarvis_confine_plugin_paths();
#endif
    detach_from_terminal();
    install_crash_handlers();
    jlog("=== daemon start, exe=" + exe_dir().string() + " ===");
    std::filesystem::path lockfile = lock_path();
    {
        std::ofstream create(lockfile, std::ios::app);
        if (!create.is_open()) {
            jlog("cannot create lock file " + lockfile.string());
            return 0;
        }
    }
    boost::interprocess::file_lock lock;
    try {
        lock = boost::interprocess::file_lock(lockfile.string().c_str());
    } catch (const std::exception &e) {
        jlog(std::string("file_lock failed: ") + e.what());
        return 0;
    } catch (...) {
        jlog("file_lock failed");
        return 0;
    }
    if (!lock.try_lock()) {
        jlog("another daemon already holds the lock, exiting");
        return 0;
    }
    {
        std::ofstream f(pid_path(), std::ios::trunc);
        f << boost::this_process::get_id();
    }
    jlog("lock acquired, pid " +
         std::to_string(int(boost::this_process::get_id())));
#ifdef _WIN32
    g_reloadevent =
        CreateEventW(nullptr, FALSE, FALSE, L"Local\\ultijarvis-reload");
#else
    signal(SIGHUP, on_hup);
#endif
    std::error_code ec;
    std::filesystem::current_path(exe_dir(), ec);
    jlog("cwd = " + std::filesystem::current_path(ec).string());
#ifdef _WIN32
    const OrtApiBase *ort = OrtGetApiBase();
    if (!ort || !ort->GetApi(ORT_API_VERSION)) {
        jlog(
            std::string("onnxruntime unusable: this build needs API ") +
            std::to_string(ORT_API_VERSION) +
            (ort ? std::string(", loaded library is ") + ort->GetVersionString()
                 : std::string(", no library resolved")));
        return 1;
    }
    jlog(std::string("onnxruntime ready, version ") + ort->GetVersionString());
#endif
    std::unique_ptr<CLFML::LOWWI::Lowwi> ww_owner;
    try {
        ww_owner = std::make_unique<CLFML::LOWWI::Lowwi>();
        jlog("lowwi runtime created");
    } catch (const std::exception &e) {
        jlog(std::string("lowwi init failed: ") + e.what());
        return 1;
    } catch (...) {
        jlog("lowwi init failed");
        return 1;
    }
    CLFML::LOWWI::Lowwi &ww_runtime = *ww_owner;
    CLFML::LOWWI::Lowwi_word_t ww;
    ww.cbfunc = wakeword_callback;
    ww.threshold = 0.3f;
    ww.min_activations = 1;
    ww.refractory = 20;
    ww.model_path = "models/example_wakewords/hey_jarvis.onnx";
    ww.phrase = "Hey Jarvis";
    jlog(std::string("wake word model present: ") +
         (std::filesystem::exists(ww.model_path, ec) ? "yes" : "NO") + " (" +
         std::filesystem::path(ww.model_path).string() + ")");
    try {
        ww_runtime.add_wakeword(ww);
        jlog("wake word loaded");
    } catch (const std::exception &e) {
        jlog(std::string("add_wakeword threw: ") + e.what());
        return 1;
    } catch (...) {
        jlog("add_wakeword threw");
        return 1;
    }
    ma_context context;
    ma_result cr = ma_context_init(nullptr, 0, nullptr, &context);
    jlog("ma_context_init = " + std::to_string(int(cr)));
    ma_device_id matched_id;
    ma_device device;
    ma_device_config config;
    bool open =
        open_capture(&context, &device, &config, &matched_id, &ww_runtime);
    if (!open)
        jlog("capture unavailable, will keep retrying");

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(kPollSeconds));
        bool stale = open && now_ms() - g_lastcallback.load() > kStaleMs;
        if (!take_reload() && !stale && open)
            continue;
        if (open) {
            ma_device_stop(&device);
            ma_device_uninit(&device);
            open = false;
        }
        open =
            open_capture(&context, &device, &config, &matched_id, &ww_runtime);
        if (open)
            jlog("capture running");
    }
}

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
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <lowwi.hpp>
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

static std::filesystem::path exe_dir() {
    return std::filesystem::path(
        boost::dll::program_location().parent_path().string());
}

#ifdef _WIN32
extern "C" const OrtApiBase *ORT_API_CALL OrtGetApiBase(void) NO_EXCEPTION {
    static const OrtApiBase *api = []() -> const OrtApiBase * {
        HMODULE lib = nullptr;
        std::error_code ec;
        std::filesystem::path local = exe_dir() / "onnxruntime.dll";
        if (std::filesystem::exists(local, ec))
            lib = LoadLibraryExW(local.wstring().c_str(), nullptr,
                                 LOAD_WITH_ALTERED_SEARCH_PATH);
#ifdef JARVIS_ONNXRUNTIME_DLL
        if (!lib) {
            std::filesystem::path configured(JARVIS_ONNXRUNTIME_DLL);
            if (std::filesystem::exists(configured, ec))
                lib = LoadLibraryExW(configured.wstring().c_str(), nullptr,
                                     LOAD_WITH_ALTERED_SEARCH_PATH);
        }
#endif
        if (!lib)
            return nullptr;
        auto entry = reinterpret_cast<const OrtApiBase *(ORT_API_CALL *)(void)>(
            reinterpret_cast<void *>(GetProcAddress(lib, "OrtGetApiBase")));
        return entry ? entry() : nullptr;
    }();
    return api;
}
#endif

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

void wakeword_callback(CLFML::LOWWI::Lowwi_ctx_t, std::shared_ptr<void>) {
    if (g_running.exchange(true))
        return;
    std::thread([]() {
        try {
            bp::child c(boost::filesystem::path(app_path().string()));
            c.wait();
        } catch (...) {
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
    // SET CMAKE TO RELEASE BEFORE BUILDING.
    // Not on macOS: launchd KeepAlive requires the process to stay in the
    // foreground, and forking makes it respawn forever.
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

static void open_capture(ma_context *context, ma_device *device,
                         ma_device_config *config, ma_device_id *matched_id,
                         CLFML::LOWWI::Lowwi *runtime) {
    ma_device_info *captureInfos;
    ma_uint32 captureCount;
    ma_context_get_devices(context, nullptr, nullptr, &captureInfos,
                           &captureCount);
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
    ma_device_init(context, config, device);
    ma_device_start(device);
    g_lastcallback.store(now_ms());
}

int main() {
    detach_from_terminal();
    std::filesystem::path lockfile = lock_path();
    {
        std::ofstream create(lockfile, std::ios::app);
        if (!create.is_open())
            return 0;
    }
    boost::interprocess::file_lock lock;
    try {
        lock = boost::interprocess::file_lock(lockfile.string().c_str());
    } catch (...) {
        return 0;
    }
    if (!lock.try_lock())
        return 0;
    {
        std::ofstream f(pid_path(), std::ios::trunc);
        f << boost::this_process::get_id();
    }
#ifdef _WIN32
    g_reloadevent =
        CreateEventW(nullptr, FALSE, FALSE, L"Local\\ultijarvis-reload");
#else
    signal(SIGHUP, on_hup);
#endif
    std::error_code ec;
    std::filesystem::current_path(exe_dir(), ec);
    CLFML::LOWWI::Lowwi ww_runtime;
    CLFML::LOWWI::Lowwi_word_t ww;
    ww.cbfunc = wakeword_callback;
    ww.threshold = 0.3f;
    ww.min_activations = 1;
    ww.refractory = 20;
    ww.model_path = "models/example_wakewords/hey_jarvis.onnx";
    ww.phrase = "Hey Jarvis";
    ww_runtime.add_wakeword(ww);
    ma_context context;
    ma_context_init(nullptr, 0, nullptr, &context);
    ma_device_id matched_id;
    ma_device device;
    ma_device_config config;
    open_capture(&context, &device, &config, &matched_id, &ww_runtime);

    // Suspend/resume can silently kill the underlying capture without
    // firing an error, so watch the callback clock and reopen if it goes quiet
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(kPollSeconds));
        bool stale = now_ms() - g_lastcallback.load() > kStaleMs;
        if (!take_reload() && !stale)
            continue;
        ma_device_stop(&device);
        ma_device_uninit(&device);
        open_capture(&context, &device, &config, &matched_id, &ww_runtime);
    }
}

#define MINIAUDIO_IMPLEMENTATION
#include "backend.h"
#include "apikey.h"
#include <QDebug>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/version.hpp>
#if BOOST_VERSION >= 108600
#include <boost/process/v1.hpp>
#ifdef _WIN32
#include <boost/process/v1/windows.hpp>
#endif
#else
#include <boost/process.hpp>
#ifdef _WIN32
#include <boost/process/windows.hpp>
#endif
#endif
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <windows.h>

#include <shellapi.h>
#else
#include <csignal>
#include <sys/types.h>
#endif

#ifdef __APPLE__
#include <unistd.h>
#endif

#if BOOST_VERSION >= 108600
namespace bp = boost::process::v1;
#else
namespace bp = boost::process;
#endif

static std::filesystem::path pid_path() {
    std::error_code ec;
    return std::filesystem::temp_directory_path(ec) / "ultijarvis.pid";
}

static std::filesystem::path lock_path() {
    std::error_code ec;
    return std::filesystem::temp_directory_path(ec) / "ultijarvis.lock";
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
    std::tm tm {};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", &tm);
    f << stamp << "  settings: " << msg << "\n";
}

static int daemon_pid() {
    std::ifstream f(pid_path());
    int pid = 0;
    if (!(f >> pid))
        return 0;
    return pid;
}

static void run_tool(const std::string &tool,
                     const std::vector<std::string> &args) {
    try {
        boost::filesystem::path exe = bp::search_path(tool);
        if (exe.empty()) {
            jlog(tool + " not found on PATH");
            return;
        }
#ifdef _WIN32
        bp::child c(exe, args, bp::std_out > bp::null, bp::std_err > bp::null,
                    bp::windows::hide);
#else
        bp::child c(exe, args, bp::std_out > bp::null, bp::std_err > bp::null);
#endif
        c.wait();
        if (c.exit_code() != 0)
            jlog(tool + " exited with " + std::to_string(c.exit_code()));
    } catch (const std::exception &e) {
        jlog(tool + " failed: " + e.what());
    } catch (...) {
        jlog(tool + " failed");
    }
}

static std::filesystem::path daemon_path() {
    std::filesystem::path dir(
        boost::dll::program_location().parent_path().string());
#ifdef _WIN32
    return dir / "Ulti-Jarvis-Daemon.exe";
#else
    return dir / "Ulti-Jarvis-Daemon";
#endif
}

#ifdef __APPLE__
static std::filesystem::path autostart_path() {
    const char *home = getenv("HOME");
    return std::filesystem::path(home ? home : "") / "Library" /
           "LaunchAgents" / "ultijarvis.daemon.plist";
}
#elif !defined(_WIN32)
static std::filesystem::path config_home() {
    const char *xdg = getenv("XDG_CONFIG_HOME");
    const char *home = getenv("HOME");
    return xdg && *xdg ? std::filesystem::path(xdg)
                       : std::filesystem::path(home ? home : "") / ".config";
}

static std::filesystem::path autostart_path() {
    return config_home() / "systemd" / "user" / "ultijarvis-daemon.service";
}

static std::filesystem::path legacy_autostart_path() {
    return config_home() / "autostart" / "ultijarvis-daemon.desktop";
}

#endif

static void register_autostart() {
#ifdef _WIN32
    HKEY key;
    if (RegCreateKeyExW(HKEY_CURRENT_USER,
                        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                        0, nullptr, 0, KEY_SET_VALUE, nullptr, &key,
                        nullptr) != ERROR_SUCCESS)
        return;
    std::wstring value = L"\"" + daemon_path().wstring() + L"\"";
    RegSetValueExW(key, L"UltiJarvis", 0, REG_SZ,
                   reinterpret_cast<const BYTE *>(value.c_str()),
                   DWORD((value.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(key);
    run_tool("schtasks", {"/Create", "/TN", "UltiJarvisDaemon", "/TR",
                          daemon_path().string(), "/SC", "MINUTE", "/MO", "10",
                          "/F"});
#else
    std::error_code ec;
    std::filesystem::path entry = autostart_path();
    std::filesystem::create_directories(entry.parent_path(), ec);
    std::ofstream f(entry, std::ios::trunc);
    if (!f.is_open())
        return;
#ifdef __APPLE__
    f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      << "<plist version=\"1.0\">\n<dict>\n"
      << "<key>Label</key><string>ultijarvis.daemon</string>\n"
      << "<key>ProgramArguments</key><array><string>"
      << daemon_path().string() << "</string></array>\n"
      << "<key>RunAtLoad</key><true/>\n"
      << "<key>KeepAlive</key><true/>\n</dict>\n</plist>\n";
    f.close();
    run_tool("launchctl", {"load", "-w", autostart_path().string()});
#else
    f << "[Unit]\n"
      << "Description=Ulti Jarvis Daemon\n"
      << "\n[Service]\n"
      << "Type=forking\n"
      << "PIDFile=" << pid_path().string() << "\n"
      << "Environment=QT_QPA_PLATFORM=xcb\n"
      << "ExecStart=\"" << daemon_path().string() << "\"\n"
      << "Restart=always\n"
      << "RestartSec=10\n"
      << "\n[Install]\n"
      << "WantedBy=default.target\n";
    f.close();
    std::filesystem::remove(legacy_autostart_path(), ec);
    run_tool("systemctl", {"--user", "daemon-reload"});
    run_tool("systemctl", {"--user", "enable", "--now",
                           "ultijarvis-daemon.service"});
    const char *user = getenv("USER");
    if (user && *user)
        run_tool("loginctl", {"enable-linger", user});
#endif
#endif
}

static void unregister_autostart() {
#ifdef _WIN32
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                      0, KEY_SET_VALUE, &key) != ERROR_SUCCESS)
        return;
    RegDeleteValueW(key, L"UltiJarvis");
    RegCloseKey(key);
    run_tool("schtasks", {"/Delete", "/TN", "UltiJarvisDaemon", "/F"});
#else
    std::error_code ec;
#ifdef __APPLE__
    run_tool("launchctl", {"unload", "-w", autostart_path().string()});
#else
    run_tool("systemctl", {"--user", "disable", "--now",
                           "ultijarvis-daemon.service"});
    std::filesystem::remove(legacy_autostart_path(), ec);
#endif
    std::filesystem::remove(autostart_path(), ec);
#endif
}

#ifdef _WIN32
static void repair_elevated() {
    std::wstring self = boost::dll::program_location().wstring();
    SHELLEXECUTEINFOW info {};
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_NOCLOSEPROCESS;
    info.lpVerb = L"runas";
    info.lpFile = self.c_str();
    info.lpParameters = L"--repair";
    info.nShow = SW_HIDE;
    if (!ShellExecuteExW(&info) || !info.hProcess) {
        jlog("elevated repair was declined or failed to start");
        return;
    }
    WaitForSingleObject(info.hProcess, 120000);
    CloseHandle(info.hProcess);
    jlog("elevated repair finished");
}
#else
static void repair_elevated() {}
#endif

static void restore_missing_files(bool elevated = false) {
    bool denied = false;
    std::filesystem::path dir(
        boost::dll::program_location().parent_path().string());
    std::filesystem::path backup = dir / "restore";
    std::error_code ec;
    if (!std::filesystem::is_directory(backup, ec))
        return;
    for (const std::filesystem::directory_entry &entry :
         std::filesystem::recursive_directory_iterator(backup, ec)) {
        if (!entry.is_regular_file(ec))
            continue;
        std::filesystem::path rel =
            std::filesystem::relative(entry.path(), backup, ec);
        if (ec || rel.empty())
            continue;
        std::filesystem::path live = dir / rel;
        std::uintmax_t good = entry.file_size(ec);
        if (ec)
            continue;
        std::error_code live_ec;
        std::uintmax_t have = std::filesystem::file_size(live, live_ec);
        if (!live_ec && have == good)
            continue;
        std::filesystem::create_directories(live.parent_path(), ec);
        std::error_code copy_ec;
        std::filesystem::copy_file(
            entry.path(), live,
            std::filesystem::copy_options::overwrite_existing, copy_ec);
        if (!copy_ec) {
            jlog("restored " + rel.string());
            continue;
        }
        jlog("could not restore " + rel.string() + ": " + copy_ec.message());
        denied = true;
    }
    if (denied && !elevated)
        repair_elevated();
}

#ifdef _WIN32
static void probe_runtime() {
    std::filesystem::path dir(
        boost::dll::program_location().parent_path().string());
    const wchar_t *names[] = {L"onnxruntime.dll", L"libonnx.dll",
                              L"libprotobuf-lite.dll", L"libre2-11.dll",
                              L"libstdc++-6.dll", L"libgcc_s_seh-1.dll",
                              L"libwinpthread-1.dll"};
    for (const wchar_t *name : names) {
        std::filesystem::path dll = dir / name;
        std::error_code ec;
        if (!std::filesystem::exists(dll, ec)) {
            jlog("probe: missing " + dll.filename().string());
            continue;
        }
        HMODULE h = LoadLibraryExW(dll.wstring().c_str(), nullptr,
                                   LOAD_WITH_ALTERED_SEARCH_PATH);
        if (h) {
            FreeLibrary(h);
            continue;
        }
        jlog("probe: " + dll.filename().string() + " failed to load, error " +
             std::to_string(GetLastError()));
    }
}
#else
static void probe_runtime() {}
#endif

void jarvis_repair_install() { restore_missing_files(true); }

static void start_daemon() {
#ifdef __linux__

    if (!bp::search_path("systemctl").empty()) {
        run_tool("systemctl", {"--user", "start", "ultijarvis-daemon.service"});
        return;
    }
#endif
    std::filesystem::path exe = daemon_path();
    std::error_code ec;
    if (!std::filesystem::exists(exe, ec)) {
        jlog("daemon binary missing at " + exe.string());
        return;
    }
    std::thread([exe]() {
        try {
            bp::child c(boost::filesystem::path(exe.string()));
            jlog("started daemon, pid " + std::to_string(c.id()));
            c.wait();
            jlog("daemon exited with " + std::to_string(c.exit_code()));
        } catch (const std::exception &e) {
            jlog(std::string("daemon launch failed: ") + e.what());
        } catch (...) {
            jlog("daemon launch failed");
        }
    }).detach();
}

Settings::Settings(QObject *parent)
    : QObject(parent), m_language("en"), m_deviceindex(0), m_customkey(false), m_haskey(false),
      m_captureinfos(nullptr), m_capturecount(0) {
    if (ma_context_init(nullptr, 0, nullptr, &(*this).m_context) !=
        MA_SUCCESS) {
        qDebug() << "Error for context_init() function";
    }
    if (ma_context_get_devices(&(*this).m_context, nullptr, nullptr,
                               &(*this).m_captureinfos,
                               &(*this).m_capturecount) != MA_SUCCESS) {
        qDebug() << "Error during getting devices.";
    }
    for (ma_uint32 device = 0; device < (*this).m_capturecount; device++) {
        (*this).m_devicenames.append((*this).m_captureinfos[device].name);
    }
    load();
    for (int i = 0; i < m_devicenames.size(); i++)
        if (m_devicenames[i] == m_device)
            m_deviceindex = i;
    restore_missing_files();
    probe_runtime();
    register_autostart();
    start_daemon();
    emit changed();
}

Settings::~Settings() { ma_context_uninit(&m_context); }

std::string Settings::config_dir() const {
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

void Settings::load() {
    std::ifstream f(config_dir() + "/config");
    std::string sline;
    while (std::getline(f, sline)) {
        QString line = QString::fromStdString(sline);
        int eq = line.indexOf('=');
        if (eq <= 0)
            continue;
        QString key = line.left(eq);
        QString value = line.mid(eq + 1);
        if (key == "device")
            m_device = value;
        else if (key == "language" && !value.isEmpty())
            m_language = value;
        else if (key == "apikey")
            m_apikey = value;
    }
    m_customkey = !m_apikey.isEmpty();
    m_haskey = m_customkey || kDefaultApiKey[0] != '\0';
}

void Settings::save() {
    std::error_code ec;
    std::filesystem::create_directories(config_dir(), ec);
    std::string path = config_dir() + "/config";
    std::ofstream f(path, std::ios::trunc);
    if (!f.is_open())
        return;
    f << "device=" << m_device.trimmed().toStdString() << "\n";
    f << "language=" << m_language.toStdString() << "\n";
    if (!m_apikey.isEmpty())
        f << "apikey=" << m_apikey.toStdString() << "\n";
    f.close();

    std::filesystem::permissions(path,
                                 std::filesystem::perms::owner_read |
                                     std::filesystem::perms::owner_write,
                                 ec);
    emit changed();
}

void Settings::signal_daemon() {
#ifdef _WIN32
    HANDLE ev =
        CreateEventW(nullptr, FALSE, FALSE, L"Local\\ultijarvis-reload");
    if (ev) {
        SetEvent(ev);
        CloseHandle(ev);
    }
#else
    int pid = daemon_pid();
    if (pid > 0)
        kill(pid_t(pid), SIGHUP);
#endif
}

#ifdef __APPLE__

static void remove_matching(const std::filesystem::path &dir,
                            const std::string &prefix) {
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec))
        return;
    for (const std::filesystem::directory_entry &entry :
         std::filesystem::directory_iterator(dir, ec)) {
        std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0)
            std::filesystem::remove_all(entry.path(), ec);
    }
}

static std::filesystem::path darwin_cache_dir() {
    char buf[1024] = {0};
    std::size_t n = confstr(_CS_DARWIN_USER_CACHE_DIR, buf, sizeof(buf));
    if (n == 0 || n > sizeof(buf))
        return {};
    return std::filesystem::path(buf);
}
#endif

#ifdef __linux__

static void linux_remove_matching(const std::filesystem::path &dir,
                                  const std::string &prefix) {
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec))
        return;
    for (const std::filesystem::directory_entry &entry :
         std::filesystem::directory_iterator(dir, ec)) {
        std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0)
            std::filesystem::remove_all(entry.path(), ec);
    }
}

static std::filesystem::path linux_xdg_dir(const char *var,
                                           const char *fallback) {
    const char *xdg = getenv(var);
    if (xdg && *xdg)
        return std::filesystem::path(xdg);
    const char *home = getenv("HOME");
    return std::filesystem::path(home ? home : "") / fallback;
}

static void linux_purge_user_data() {
    for (const std::filesystem::path &base :
         {linux_xdg_dir("XDG_CACHE_HOME", ".cache"),
          linux_xdg_dir("XDG_CONFIG_HOME", ".config"),
          linux_xdg_dir("XDG_DATA_HOME", ".local/share"),
          linux_xdg_dir("XDG_STATE_HOME", ".local/state")}) {
        linux_remove_matching(base, "Ulti Jarvis");
        linux_remove_matching(base, "Ulti-Jarvis");
        linux_remove_matching(base, "ultijarvis");
    }
}

#endif

static void stop_daemon() {
    int pid = daemon_pid();
    if (pid <= 0)
        return;
#ifdef _WIN32
    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, DWORD(pid));
    if (h) {
        TerminateProcess(h, 0);
        CloseHandle(h);
    }
#else
    kill(pid_t(pid), SIGTERM);
#endif
}

void Settings::select_device(int index) {
    if (index < 0 || index >= m_devicenames.size())
        return;
    m_deviceindex = index;
    m_device = m_devicenames[index];
    save();
    signal_daemon();
}

void Settings::end_daemon() { stop_daemon(); }

void Settings::restart_daemon() {
#if !defined(_WIN32) && !defined(__APPLE__)
    run_tool("systemctl",
             {"--user", "restart", "ultijarvis-daemon.service"});
#else
    stop_daemon();
    start_daemon();
#endif
}

void Settings::set_language(const QString &lang) {
    m_language = lang;
    save();
}

void Settings::set_apikey(const QString &key) {
    QString trimmed = key.trimmed();
    if (trimmed.isEmpty()) {
        reset_apikey();
        return;
    }
    m_apikey = trimmed;
    m_customkey = true;
    m_haskey = true;
    save();
}

void Settings::reset_apikey() {
    m_apikey.clear();
    m_customkey = false;
    m_haskey = kDefaultApiKey[0] != '\0';
    save();
}

void Settings::uninstall() {
    unregister_autostart();
    stop_daemon();
    std::error_code ec;
    std::filesystem::remove_all(config_dir(), ec);
    std::filesystem::remove(pid_path(), ec);
    std::filesystem::remove(lock_path(), ec);
    std::filesystem::remove(log_path(), ec);
    std::filesystem::path dir(
        boost::dll::program_location().parent_path().string());
#ifdef _WIN32
    std::filesystem::path un = dir / "Uninstall.exe";
    if (std::filesystem::exists(un, ec)) {
        ShellExecuteW(nullptr, L"open", un.wstring().c_str(), nullptr,
                      dir.wstring().c_str(), SW_SHOWNORMAL);
    } else {
        std::wstring args = L"/c timeout /t 3 /nobreak >nul & rmdir /s /q \"" +
                            dir.wstring() + L"\"";
        ShellExecuteW(nullptr, L"open", L"cmd.exe", args.c_str(), nullptr,
                      SW_HIDE);
    }
#elif defined(__APPLE__)
    std::filesystem::path bundle = dir.parent_path().parent_path();
    if (bundle.extension() != ".app")
        bundle = dir;

    run_tool("pkill", {"-f", (dir / "Ulti Jarvis").string()});

    std::filesystem::path home(getenv("HOME") ? getenv("HOME") : "");
    remove_matching(home / "Library" / "Caches", "Ulti Jarvis");
    remove_matching(home / "Library" / "Saved Application State",
                    "com.ultijarvis.");
    remove_matching(home / "Library" / "Preferences", "com.ultijarvis.");
    remove_matching(home / "Library" / "Application Support" / "CrashReporter",
                    "Ulti Jarvis");
    remove_matching(home / "Library" / "Logs" / "DiagnosticReports",
                    "Ulti Jarvis");
    std::filesystem::path cache = darwin_cache_dir();
    if (!cache.empty())
        std::filesystem::remove_all(cache / "com.ultijarvis.UltiJarvis", ec);

    run_tool("/System/Library/Frameworks/CoreServices.framework/Frameworks/"
             "LaunchServices.framework/Support/lsregister",
             {"-u", bundle.string()});

    std::filesystem::remove_all(bundle, ec);

    if (std::filesystem::exists(bundle, ec)) {
        std::string command =
            "rm -rf '" + bundle.string() +
            "'; for p in $(/usr/sbin/pkgutil --pkgs | /usr/bin/grep -i "
            "ultijarvis); do /usr/sbin/pkgutil --forget $p; done";
        run_tool("osascript",
                 {"-e", "do shell script \"" + command +
                            "\" with administrator privileges"});
        if (std::filesystem::exists(bundle, ec))
            jlog("the application could not be removed from " +
                 bundle.string());
    }
#else
#ifdef __linux__

    run_tool("pkill", {"-f", (dir / "Ulti Jarvis").string()});
    linux_purge_user_data();
    const char *linger_user = getenv("USER");
    if (linger_user && *linger_user)
        run_tool("loginctl", {"disable-linger", linger_user});
#endif
    bool packaged = false;
    try {
        boost::filesystem::path pk = bp::search_path("pkexec");
        std::string d = dir.string();
        if (!pk.empty() && d.rfind("/usr/", 0) == 0) {
            std::vector<std::string> args = {"apt-get", "purge", "-y",
                                             "ultijarvis"};
#ifdef __linux__
            if (bp::search_path("apt-get").empty() &&
                !bp::search_path("dnf").empty())
                args = {"dnf", "remove", "-y", "ultijarvis"};
#endif
            bp::child c(pk, args);
#ifdef __linux__

            c.wait();
            int rc = c.exit_code();
            packaged = rc == 0;

            if (!packaged && rc != 126 && rc != 127) {
                jlog("package removal exited with " + std::to_string(rc) +
                     ", removing the files directly");
                run_tool("pkexec", {"rm", "-rf", d});
                std::error_code left;
                packaged = !std::filesystem::exists(dir, left);
            }
            if (!packaged)
                jlog("the application could not be removed from " + d);
#else
            c.detach();
            packaged = true;
#endif
        }
    } catch (...) {
    }
    if (!packaged)
        std::filesystem::remove_all(dir, ec);
#endif
    std::exit(0);
}

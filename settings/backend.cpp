#define MINIAUDIO_IMPLEMENTATION
#include "backend.h"
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

static void start_daemon() {
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
    : QObject(parent), m_language("en"), m_deviceindex(0), m_customkey(false),
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

    // config file holds the api key in plaintext, so lock it down to owner-only
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
    save();
}

void Settings::reset_apikey() {
    m_apikey.clear();
    m_customkey = false;
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
    if (bundle.extension() == ".app")
        std::filesystem::remove_all(bundle, ec);
    else
        std::filesystem::remove_all(dir, ec);
#else
    bool packaged = false;
    try {
        boost::filesystem::path pk = bp::search_path("pkexec");
        std::string d = dir.string();
        if (!pk.empty() && d.rfind("/usr/", 0) == 0) {
            std::vector<std::string> args = {"apt-get", "purge", "-y",
                                             "ultijarvis"};
            bp::child c(pk, args);
            c.detach();
            packaged = true;
        }
    } catch (...) {
    }
    if (!packaged)
        std::filesystem::remove_all(dir, ec);
#endif
    std::exit(0);
}

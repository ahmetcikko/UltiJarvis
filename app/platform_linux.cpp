#include "platform.h"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace platform {

std::string home_dir() {
    const char *home = getenv("HOME");
    return home ? std::string(home) : std::string();
}

std::string config_dir() {
    const char *xdg = getenv("XDG_CONFIG_HOME");
    const char *home = getenv("HOME");
    std::string base = xdg && *xdg ? std::string(xdg)
                                   : std::string(home ? home : "") + "/.config";
    return base + "/ultijarvis";
}

QStringList app_dirs() {
    QStringList dirs;
    const char *home = getenv("HOME");
    const char *dataHome = getenv("XDG_DATA_HOME");
    std::string base = dataHome && *dataHome
                           ? std::string(dataHome)
                           : std::string(home ? home : "") + "/.local/share";
    dirs.append(QString::fromStdString(base + "/applications"));
    const char *dataDirs = getenv("XDG_DATA_DIRS");
    std::string list =
        dataDirs && *dataDirs ? dataDirs : "/usr/local/share:/usr/share";
    for (const QString &d :
         QString::fromStdString(list).split(':', Qt::SkipEmptyParts))
        dirs.append(d + "/applications");
    return dirs;
}

bool open_path(const QString &path) { return spawn({"xdg-open", path}); }

bool launch_parsed(const QStringList &args, const QString &) {
    if (args.isEmpty())
        return false;
    return spawn(args);
}

int parent_pid(int pid) {
    std::ifstream f("/proc/" + std::to_string(pid) + "/stat");
    std::string content;
    std::getline(f, content);
    size_t p = content.rfind(')');
    if (p == std::string::npos)
        return 0;
    int ppid = 0;

    if (std::sscanf(content.c_str() + p + 1, " %*c %d", &ppid) != 1)
        return 0;
    return ppid;
}

bool is_system_path(const std::string &path) {
    static const std::vector<std::string> roots = {
        "/usr/lib/systemd/",  "/lib/systemd/",  "/usr/libexec/",
        "/usr/sbin/",         "/sbin/",         "/usr/lib/gdm",
        "/usr/lib/polkit-1/", "/usr/lib/xorg/", "/usr/lib/dbus-1.0/"};
    for (const std::string &r : roots)
        if (path.rfind(r, 0) == 0)
            return true;
    return false;
}

static std::string cgroup_path(const std::string &line) {

    size_t first = line.find(':');
    if (first == std::string::npos)
        return line;
    size_t second = line.find(':', first + 1);
    if (second == std::string::npos)
        return line;
    return line.substr(second + 1);
}

static bool in_app_slice(int pid) {
    std::ifstream f("/proc/" + std::to_string(pid) + "/cgroup");
    std::string line;
    while (std::getline(f, line))
        if (cgroup_path(line).find("/app.slice/") != std::string::npos)
            return true;
    return false;
}

bool is_shielded(int pid, bool own_lineage) {

    if (own_lineage && !in_app_slice(pid))
        return true;
    std::ifstream f("/proc/" + std::to_string(pid) + "/cgroup");
    std::string line;
    while (std::getline(f, line)) {
        std::string path = cgroup_path(line);
        if (path.find("/system.slice/") != std::string::npos ||
            path.find("/init.scope") != std::string::npos ||
            path.find("/session.slice/") != std::string::npos ||
            path.find("/background.slice/") != std::string::npos)
            return true;
        size_t slash = path.rfind('/');
        if (slash != std::string::npos) {
            std::string leaf = path.substr(slash + 1);
            if (leaf.size() > 8 &&
                leaf.compare(leaf.size() - 8, 8, ".service") == 0)
                return true;
        }
    }
    return false;
}

std::vector<ProcEntry> enumerate_procs() {
    std::vector<ProcEntry> out;
    long page = sysconf(_SC_PAGESIZE);
    uid_t self = getuid();
    std::error_code ec;
    for (const std::filesystem::directory_entry &entry :
         std::filesystem::directory_iterator("/proc", ec)) {
        std::string name = entry.path().filename().string();
        if (name.find_first_not_of("0123456789") != std::string::npos)
            continue;
        struct stat st;
        if (stat(("/proc/" + name).c_str(), &st) != 0 || st.st_uid != self)
            continue;
        std::ifstream cmd("/proc/" + name + "/cmdline");
        std::string arg0;
        std::getline(cmd, arg0, '\0');
        if (arg0.empty())
            continue;
        std::ifstream cf("/proc/" + name + "/comm");
        std::string comm;
        std::getline(cf, comm);
        if (comm.empty())
            continue;
        ProcEntry e;
        e.pid = std::stoi(name);
        e.comm = QString::fromStdString(comm);
        std::error_code lec;
        std::filesystem::path target =
            std::filesystem::read_symlink("/proc/" + name + "/exe", lec);
        if (!lec)
            e.exe = target.string();
        std::ifstream sm("/proc/" + name + "/statm");
        std::uint64_t sz = 0, rss = 0;
        sm >> sz >> rss;
        e.rss = rss * std::uint64_t(page);
        out.push_back(e);
    }
    return out;
}

bool terminate_pid(int pid) { return kill(pid_t(pid), SIGTERM) == 0; }

bool set_mute(bool mute) {
    return spawn(
        {"pactl", "set-sink-mute", "@DEFAULT_SINK@", mute ? "1" : "0"});
}

bool step_volume(int delta) {
    return spawn(
        {"pactl", "set-sink-volume", "@DEFAULT_SINK@",
         QString(delta >= 0 ? "+" : "") + QString::number(delta) + "%"});
}

bool set_volume(int level) {
    return spawn({"pactl", "set-sink-mute", "@DEFAULT_SINK@", "0"}) &&
           spawn({"pactl", "set-sink-volume", "@DEFAULT_SINK@",
                  QString::number(level) + "%"});
}

bool system_power(const QString &action) {
    if (action == "shutdown")
        return spawn({"systemctl", "poweroff"});
    if (action == "reboot")
        return spawn({"systemctl", "reboot"});
    return spawn({"systemctl", "suspend"});
}

}

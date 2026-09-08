#include "platform.h"

#include <csignal>
#include <cstdlib>
#include <string>
#include <vector>

#include <libproc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace platform {

std::string home_dir() {
    const char *home = getenv("HOME");
    return home ? std::string(home) : std::string();
}

std::string config_dir() {
    const char *home = getenv("HOME");
    std::string base =
        std::string(home ? home : "") + "/Library/Application Support";
    return base + "/ultijarvis";
}

QStringList app_dirs() {
    QStringList dirs;
    const char *home = getenv("HOME");
    dirs.append("/Applications");
    dirs.append("/System/Applications");
    if (home && *home)
        dirs.append(QString::fromUtf8(home) + "/Applications");
    return dirs;
}

bool open_path(const QString &path) { return spawn({"open", path}); }

bool launch_parsed(const QStringList &, const QString &exec) {
    return open_path(exec);
}

int parent_pid(int pid) {
    struct proc_bsdinfo bsd;
    if (proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &bsd, sizeof(bsd)) ==
        int(sizeof(bsd)))
        return int(bsd.pbi_ppid);
    return 0;
}

bool is_system_path(const std::string &path) {
    static const std::vector<std::string> roots = {
        "/System/", "/usr/libexec/", "/usr/sbin/", "/sbin/", "/usr/bin/"};
    for (const std::string &r : roots)
        if (path.rfind(r, 0) == 0)
            return true;
    return false;
}

bool is_shielded(int, bool own_lineage) { return own_lineage; }

std::vector<ProcEntry> enumerate_procs() {
    std::vector<ProcEntry> out;
    int bytes = proc_listpids(PROC_ALL_PIDS, 0, nullptr, 0);
    if (bytes <= 0)
        return out;
    std::vector<pid_t> pids(std::size_t(bytes) / sizeof(pid_t) + 64, 0);
    bytes = proc_listpids(PROC_ALL_PIDS, 0, pids.data(),
                          int(pids.size() * sizeof(pid_t)));
    if (bytes <= 0)
        return out;
    int n = bytes / int(sizeof(pid_t));
    uid_t self = getuid();
    for (int i = 0; i < n; i++) {
        if (pids[i] <= 0)
            continue;
        struct proc_bsdinfo bsd;
        if (proc_pidinfo(pids[i], PROC_PIDTBSDINFO, 0, &bsd, sizeof(bsd)) !=
            int(sizeof(bsd)))
            continue;
        if (bsd.pbi_uid != self)
            continue;
        ProcEntry e;
        e.pid = int(pids[i]);
        e.comm =
            QString::fromUtf8(bsd.pbi_name[0] ? bsd.pbi_name : bsd.pbi_comm);
        char pathbuf[PROC_PIDPATHINFO_MAXSIZE];
        if (proc_pidpath(pids[i], pathbuf, sizeof(pathbuf)) > 0)
            e.exe = pathbuf;
        e.rss = 0;
        struct proc_taskinfo ti;
        if (proc_pidinfo(pids[i], PROC_PIDTASKINFO, 0, &ti, sizeof(ti)) ==
            int(sizeof(ti)))
            e.rss = std::uint64_t(ti.pti_resident_size);
        if (!e.comm.isEmpty())
            out.push_back(e);
    }
    return out;
}

bool terminate_pid(int pid) { return kill(pid_t(pid), SIGTERM) == 0; }

bool set_mute(bool mute) {
    return spawn({"osascript", "-e",
                  QString("set volume output muted ") +
                      (mute ? "true" : "false")});
}

bool step_volume(int delta) {
    return spawn({"osascript", "-e",
                  QString("set volume output volume "
                          "(output volume of (get volume settings) ") +
                      (delta >= 0 ? "+ " : "- ") +
                      QString::number(delta < 0 ? -delta : delta) + ")"});
}

bool set_volume(int level) {
    return spawn({"osascript", "-e", "set volume output muted false"}) &&
           spawn({"osascript", "-e",
                  "set volume output volume " + QString::number(level)});
}

bool system_power(const QString &action) {
    if (action == "shutdown")
        return spawn({"osascript", "-e",
                      "tell application \"System Events\" to shut down"});
    if (action == "reboot")
        return spawn({"osascript", "-e",
                      "tell application \"System Events\" to restart"});
    return spawn(
        {"osascript", "-e", "tell application \"System Events\" to sleep"});
}

}

#pragma once

#include <QString>
#include <QStringList>
#include <cstdint>
#include <string>
#include <vector>

struct ProcEntry {
    int pid;
    QString comm;
    std::string exe;
    std::uint64_t rss;
};

namespace platform {

bool spawn(const QStringList &args);
std::string home_dir();
std::string config_dir();
QStringList app_dirs();
bool open_path(const QString &path);
bool launch_parsed(const QStringList &args, const QString &exec);
int parent_pid(int pid);
bool is_system_path(const std::string &path);
bool is_shielded(int pid, bool own_lineage);
std::vector<ProcEntry> enumerate_procs();
bool terminate_pid(int pid);
bool set_mute(bool mute);
bool step_volume(int delta);
bool set_volume(int level);
bool system_power(const QString &action);

}

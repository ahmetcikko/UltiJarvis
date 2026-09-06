#include "platform.h"

#include <cstdlib>
#include <string>
#include <vector>

#include <windows.h>

#include <initguid.h>

#include <endpointvolume.h>
#include <mmdeviceapi.h>
#include <psapi.h>
#include <shellapi.h>
#include <tlhelp32.h>

namespace platform {

std::string home_dir() {
    const char *profile = getenv("USERPROFILE");
    return profile ? std::string(profile) : std::string();
}

std::string config_dir() {
    const char *appdata = getenv("APPDATA");
    std::string base =
        appdata && *appdata ? std::string(appdata) : std::string();
    return base + "/ultijarvis";
}

QStringList app_dirs() {
    QStringList dirs;
    const char *appdata = getenv("APPDATA");
    const char *programdata = getenv("ProgramData");
    if (appdata && *appdata)
        dirs.append(QString::fromUtf8(appdata) +
                    "/Microsoft/Windows/Start Menu/Programs");
    if (programdata && *programdata)
        dirs.append(QString::fromUtf8(programdata) +
                    "/Microsoft/Windows/Start Menu/Programs");
    return dirs;
}

bool open_path(const QString &path) {
    std::wstring w = path.toStdWString();
    HINSTANCE r = ShellExecuteW(nullptr, L"open", w.c_str(), nullptr, nullptr,
                                SW_SHOWNORMAL);
    return reinterpret_cast<INT_PTR>(r) > 32;
}

bool launch_parsed(const QStringList &, const QString &exec) {
    return open_path(exec);
}

int parent_pid(int pid) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return 0;
    int ppid = 0;
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do {
            if (int(pe.th32ProcessID) == pid) {
                ppid = int(pe.th32ParentProcessID);
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return ppid;
}

bool is_system_path(const std::string &path) {
    wchar_t win[MAX_PATH];
    UINT n = GetWindowsDirectoryW(win, MAX_PATH);
    if (!n)
        return false;
    return QString::fromStdString(path).startsWith(
        QString::fromWCharArray(win, int(n)), Qt::CaseInsensitive);
}

bool is_shielded(int, bool own_lineage) { return own_lineage; }

std::vector<ProcEntry> enumerate_procs() {
    std::vector<ProcEntry> out;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return out;
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do {
            HANDLE h =
                OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ,
                            FALSE, pe.th32ProcessID);
            if (!h)
                continue;
            ProcEntry e;
            e.pid = int(pe.th32ProcessID);
            e.comm = QString::fromWCharArray(pe.szExeFile);
            if (e.comm.endsWith(".exe", Qt::CaseInsensitive))
                e.comm.chop(4);
            e.rss = 0;
            wchar_t buf[MAX_PATH];
            DWORD len = MAX_PATH;
            if (QueryFullProcessImageNameW(h, 0, buf, &len))
                e.exe = QString::fromWCharArray(buf, int(len)).toStdString();
            PROCESS_MEMORY_COUNTERS pmc;
            if (GetProcessMemoryInfo(h, &pmc, sizeof(pmc)))
                e.rss = std::uint64_t(pmc.WorkingSetSize);
            CloseHandle(h);
            if (!e.comm.isEmpty())
                out.push_back(e);
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return out;
}

bool terminate_pid(int pid) {
    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, DWORD(pid));
    if (!h)
        return false;
    BOOL ok = TerminateProcess(h, 0);
    CloseHandle(h);
    return ok != FALSE;
}

template <typename F> static bool with_endpoint_volume(F fn) {
    HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool ok = false;
    IMMDeviceEnumerator *devices = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr,
                                   CLSCTX_ALL, IID_IMMDeviceEnumerator,
                                   reinterpret_cast<void **>(&devices)))) {
        IMMDevice *endpoint = nullptr;
        if (SUCCEEDED((*devices).GetDefaultAudioEndpoint(eRender, eMultimedia,
                                                         &endpoint))) {
            IAudioEndpointVolume *volume = nullptr;
            if (SUCCEEDED((*endpoint).Activate(
                    IID_IAudioEndpointVolume, CLSCTX_ALL, nullptr,
                    reinterpret_cast<void **>(&volume)))) {
                fn(volume);
                (*volume).Release();
                ok = true;
            }
            (*endpoint).Release();
        }
        (*devices).Release();
    }
    if (SUCCEEDED(init))
        CoUninitialize();
    return ok;
}

bool set_mute(bool mute) {
    return with_endpoint_volume([mute](IAudioEndpointVolume *v) {
        (*v).SetMute(mute ? TRUE : FALSE, nullptr);
    });
}

bool step_volume(int delta) {
    return with_endpoint_volume([delta](IAudioEndpointVolume *v) {
        float level = 0.0f;
        if (FAILED((*v).GetMasterVolumeLevelScalar(&level)))
            return;
        level += float(delta) / 100.0f;
        level = level < 0.0f ? 0.0f : (level > 1.0f ? 1.0f : level);
        (*v).SetMasterVolumeLevelScalar(level, nullptr);
    });
}

bool set_volume(int level) {
    return with_endpoint_volume([level](IAudioEndpointVolume *v) {
        (*v).SetMute(FALSE, nullptr);
        (*v).SetMasterVolumeLevelScalar(float(level) / 100.0f, nullptr);
    });
}

bool system_power(const QString &action) {
    if (action == "shutdown")
        return spawn({"shutdown", "/s", "/t", "0"});
    if (action == "reboot")
        return spawn({"shutdown", "/r", "/t", "0"});
    return spawn({"rundll32.exe", "powrprof.dll,SetSuspendState", "0,1,0"});
}

} // namespace platform

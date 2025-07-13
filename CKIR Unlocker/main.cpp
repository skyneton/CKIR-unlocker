#include "main.h"
#include "privilege.h"
#include "inj.h"
#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
using namespace std;
#define INITIALIZE_IOCTL_CODE 0x9876C004
#define TERMINSTE_PROCESS_IOCTL_CODE 0x9876C094
#define LOOP_COUNT 20

typedef long (NTAPI* pNtProcess)(HANDLE proccessHandle);
pNtProcess NtSuspendProcess;
pNtProcess NtResumeProcess;

class TerminateData {
public:
    DWORD pid;
    bool terminated;
    TerminateData(DWORD pid) :pid(pid), terminated(false) {}
};

static bool LoadNT() {
    auto ntdll = GetModuleHandleA("ntdll.dll");
    if (!ntdll) return false;
    NtSuspendProcess = (pNtProcess)GetProcAddress(ntdll, "NtSuspendProcess");
    NtResumeProcess = (pNtProcess)GetProcAddress(ntdll, "NtResumeProcess");
    return !!NtSuspendProcess;
}

BOOL CALLBACK TerminateByHWND(HWND hwnd, LPARAM lParam) {
    DWORD pid;
    TerminateData* data = reinterpret_cast<TerminateData*>(lParam);
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid != data->pid) return true;
    if (PostMessageA(hwnd, WM_CLOSE, NULL, NULL)) {
        data->terminated = true;
    }
    return true;
}

static int TerminateProc(PROCESSENTRY32& procEntry, int buildType) {
    DWORD output[2] = { 0 };
    DWORD size = sizeof(output);
    DWORD byteReturned = 0;

	DWORD dwExitCode = 0;
	auto handle = OpenProcess(PROCESS_ALL_ACCESS, true, procEntry.th32ProcessID);
    if (GetExitCodeProcess(handle, &dwExitCode)) {
        if (TerminateProcess(handle, dwExitCode)) {
            CloseHandle(handle);
            return 1;
        }
    }
    if (buildType == BUILD_TYPE_FORCE) {
        if (InjectCode(handle)) {
            //cout << "\n* Injection Success\n";
            return 1;
        }
    }

    TerminateData result(procEntry.th32ProcessID);
    EnumWindows(TerminateByHWND, reinterpret_cast<long long>(&result));
    if (result.terminated) {
        CloseHandle(handle);
        return 1;
    }

    if (NtSuspendProcess) {
        auto result = NtSuspendProcess(handle);
        cout << result;
        if (result >= 0) {
            CloseHandle(handle);
            return 2;
        }
    }

	CloseHandle(handle);
	return 0;
}

static bool ResumeProc(PROCESSENTRY32& procEntry) {
    auto handle = OpenProcess(PROCESS_ALL_ACCESS, true, procEntry.th32ProcessID);

    if (NtSuspendProcess) {
        auto result = NtResumeProcess(handle);
        cout << result;
        if (result >= 0) {
            CloseHandle(handle);
            return true;
        }
    }

    CloseHandle(handle);
    return false;
}

static PROCESSENTRY32 Find(const wchar_t* name) {
    auto handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    PROCESSENTRY32 result = { 0 };
    PROCESSENTRY32 procEntry = { 0 };
    result.th32ProcessID = 0;
    procEntry.dwSize = sizeof(PROCESSENTRY32);
    Process32First(handle, &procEntry);
    do {
        if (wcscmp(procEntry.szExeFile, name)) continue;
        result = procEntry;
        break;
    } while (Process32Next(handle, &procEntry));
    CloseHandle(handle);
    return result;
}

static void LoopKiller(const char* type, int count, const wchar_t* list[], int length, int buildType) {
    cout << type << " Process Detecting...\n";
    for (int i = 0; i < length; i++) {
        auto p = Find(list[i]);
        if (p.th32ProcessID == 0) continue;
        cout << "Detected ["; wcout << p.szExeFile; cout << "](PID:" << p.th32ProcessID << ")\n";
        auto handle = OpenProcess(PROCESS_ALL_ACCESS, false, p.th32ProcessID);
        wchar_t filePath[MAX_PATH] = { 0 };
        DWORD len = MAX_PATH;
        QueryFullProcessImageNameW(handle, NULL, filePath, &len);
        CloseHandle(handle);
        cout << "PATH: "; wcout << filePath; cout << '\n';
        if (buildType == BUILD_TYPE_RESUME) {
            cout << "Try To Resume...\n";
            if (ResumeProc(p)) cout << "\033[1;32mSuccess!\033[0m\n";
            else cout << "\033[1;31mFailed.\033[0m\n";
            continue;
        }
        bool killed = false;
        bool deleted = false;
        cout << "Try To Kill...\n";

        for (int j = 0; j < count; j++) {
            p = Find(list[i]);
            if (!p.th32ProcessID) {
                if (DeleteFileW(filePath)) {
                    deleted = true;
                }
                //killed = 1;
                //break;
                continue;
            }

            int result = TerminateProc(p, buildType);
            if (!result) continue;
            if (result == 2) {
                cout << "\033[1;35mSuspended..\033[0m\n";
                break;
            }
            if (length != MAX_PATH);
            if (DeleteFileW(filePath)) {
                deleted = true;
                //killed = true;
                //break;
            }
        }

        if (!killed) {
            p = Find(list[i]);
            if (!p.th32ProcessID) killed = true;
        }
        cout << "Kill " << (killed ? "\033[1;32mSuccess!" : "\033[1;31mFailed...") << "\033[0m\n";
        cout << "Try To Delete Execution File...\n";
        if (deleted) cout << "\033[1;32mSuccess!\033[0m\n";
        else cout << "\033[1;31mFailed...\033[0m\n";
    }
}

void run(int buildType) {
    cin.tie(0), cout.tie(0)->sync_with_stdio(0);
    wcin.tie(0), wcout.tie(0)->sync_with_stdio(0);
    if (Privilege()) cout << "Success Privileges.\n";
    const wchar_t* latest[] = { L"lqndauccd.exe", L"MaestroWebSvr.exe", L"qukapttp.exe", L"MaestroWebAgent.exe", L"nfowjxyfd.exe" };
    const wchar_t* remote[] = { L"TDepend64.exe", L"TDepend.exe" };
    const wchar_t* unknown[] = { L"rwtyijsa.exe", L"nhfneczzm.exe" };
    const wchar_t* legacy[] = { L"SoluSpSvr.exe", L"MaestroNAgent.exe", L"MaestroNSvr.exe" };

    if (LoadNT())
        cout << "NT Loaded\n";
    else
        cout << "NT Load Failed..\n";

    LoopKiller("Latest CKIR", LOOP_COUNT, latest, sizeof(latest) / sizeof(const wchar_t*), buildType);
    LoopKiller("Remote Control", LOOP_COUNT, remote, sizeof(remote) / sizeof(const wchar_t*), buildType);
    LoopKiller("Unknown", LOOP_COUNT, unknown, sizeof(unknown) / sizeof(const wchar_t*), buildType);
    LoopKiller("Legacy", LOOP_COUNT, legacy, sizeof(legacy) / sizeof(const wchar_t*), buildType);

	cout << "\n\nPress Enter to exit...";
    system("pause");
	getchar();
}
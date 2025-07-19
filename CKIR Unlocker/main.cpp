#include "main.h"
#include "privilege.h"
#include "inj.h"
#include "color.h"
#include <Windows.h>
#include <TlHelp32.h>
using namespace std;
#define INITIALIZE_IOCTL_CODE 0x9876C004
#define TERMINSTE_PROCESS_IOCTL_CODE 0x9876C094
#define LOOP_COUNT 70

typedef long (NTAPI* pNtProcess)(HANDLE proccessHandle);
pNtProcess NtSuspendProcess;
pNtProcess NtResumeProcess;

const wchar_t* latest[] = { L"lqndauccd.exe", L"MaestroWebSvr.exe", L"qukapttp.exe", L"MaestroWebAgent.exe", L"nfowjxyfd.exe" };
const wchar_t* remote[] = { L"TDepend64.exe", L"TDepend.exe" };
const wchar_t* unknown[] = { L"rwtyijsa.exe", L"nhfneczzm.exe" };
const wchar_t* legacy[] = { L"SoluSpSvr.exe", L"MaestroNAgent.exe", L"MaestroNSvr.exe" };

static bool LoadNT() {
    auto ntdll = GetModuleHandleA("ntdll.dll");
    if (!ntdll) return false;
    NtSuspendProcess = (pNtProcess)GetProcAddress(ntdll, "NtSuspendProcess");
    NtResumeProcess = (pNtProcess)GetProcAddress(ntdll, "NtResumeProcess");
    return !!NtSuspendProcess;
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

static bool IsMainProcess() {
    wchar_t src[MAX_PATH];
    DWORD len = GetModuleFileNameW(NULL, src, MAX_PATH);
    if(!len) return false;
    wchar_t* filename = wcsrchr(src, L'\\') + 1;
    for (int i = sizeof(latest) / sizeof(const wchar_t*) - 1; i >= 0; i--) {
        if (!wcscmp(filename, latest[i])) return false;
    }
    for (int i = sizeof(remote) / sizeof(const wchar_t*) - 1; i >= 0; i--) {
        if (!wcscmp(filename, remote[i])) return false;
    }
    for (int i = sizeof(unknown) / sizeof(const wchar_t*) - 1; i >= 0; i--) {
        if (!wcscmp(filename, unknown[i])) return false;
    }
    for (int i = sizeof(legacy) / sizeof(const wchar_t*) - 1; i >= 0; i--) {
        if (!wcscmp(filename, legacy[i])) return false;
    }
    return true;
}

static void CopyTo(const wchar_t* dest) {
    wchar_t src[MAX_PATH];
    if (GetModuleFileNameW(NULL, src, MAX_PATH))
        CopyFileW(src, dest, false);
}

static int LoopKiller(const char* type, int count, const wchar_t* list[], int length, int buildType) {
    //bool isMainProcess = IsMainProcess();
    int remain = length;
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
            if (ResumeProc(p)) cout << green << "Success!";
            else cout << red << "Failed.";
            cout << white << '\n';
            continue;
        }
        bool killed = false;
        bool deleted = false;

        for (int j = 0; j < count; j++) {
            p = Find(list[i]);
            if (!p.th32ProcessID) {
                if (/*wcscmp(wcsrchr(filePath, L'\\') + 1, L"lqndauccd.exe") && */DeleteFileW(filePath)) {
                    deleted = true;
                    // if (buildType == BUILD_TYPE_WORM && isMainProcess) CopyTo(filePath);
                }
                //killed = 1;
                //break;
                continue;
            }

            int result = TerminateProc(p, buildType);
            if (!result) continue;
            if (result == 2) {
                cout << magenta << "Suspended.." << white << '\n';
                break;
            }
            if (DeleteFileW(filePath)) {
                deleted = true;
                // if(buildType == BUILD_TYPE_WORM && isMainProcess) CopyTo(filePath);
                //killed = true;
                //break;
            }
        }

        if (!killed) {
            p = Find(list[i]);
            if (!p.th32ProcessID) killed = true;
        }
        cout << "Try To Kill...\n";
        if (killed) cout << green << "Success!";
        else cout << red << "Failed...";
        cout << white << '\n';

        cout << "Try To Delete Execution File...\n";
        if (deleted) cout << green << "Success!";
        else cout << red << "Failed...";
        cout << white << '\n';
        remain -= deleted;
    }
    return remain;
}

void run(int buildType) {
    cin.tie(0), cout.tie(0)->sync_with_stdio(0);
    wcin.tie(0), wcout.tie(0)->sync_with_stdio(0);
    if (Privilege()) cout << "Success Privileges.\n";

    if (LoadNT())
        cout << "NT Loaded\n";
    else
        cout << "NT Load Failed..\n";

    restart:
    int remain = 0;

    remain += LoopKiller("Latest CKIR", LOOP_COUNT, latest, sizeof(latest) / sizeof(const wchar_t*), buildType);
    remain += LoopKiller("Remote Control", LOOP_COUNT, remote, sizeof(remote) / sizeof(const wchar_t*), buildType);
    remain += LoopKiller("Unknown", LOOP_COUNT, unknown, sizeof(unknown) / sizeof(const wchar_t*), buildType);
    remain += LoopKiller("Legacy", LOOP_COUNT, legacy, sizeof(legacy) / sizeof(const wchar_t*), buildType);

    cout << "\nTotal Remain Processors: " << remain << '\n';

    if (buildType == BUILD_TYPE_WORM && remain > 1) {
        cout << yellow << "Restart!" << white << '\n';
        goto restart;
    }

	cout << "\n\nPress Enter to exit...";
    system("pause");
	getchar();
}
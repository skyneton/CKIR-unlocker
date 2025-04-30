#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
using namespace std;
#define INITIALIZE_IOCTL_CODE 0x9876C004
#define TERMINSTE_PROCESS_IOCTL_CODE 0x9876C094

bool TerminateProc(PROCESSENTRY32& procEntry) {
	DWORD dwExitCode = 0;
	auto handle = OpenProcess(PROCESS_ALL_ACCESS, 0, procEntry.th32ProcessID);
	if (GetExitCodeProcess(handle, &dwExitCode)) {
		if (TerminateProcess(handle, dwExitCode)) {
			CloseHandle(handle);
			return true;
		}
	}
	CloseHandle(handle);

	DWORD output[2] = { 0 };
	DWORD size = sizeof(output);
	DWORD byteReturned = 0;
	handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (DeviceIoControl(handle, TERMINSTE_PROCESS_IOCTL_CODE, &procEntry.th32ProcessID, sizeof(procEntry.th32ProcessID), output, size, &byteReturned, NULL)) {
		CloseHandle(handle);
		return true;
	}
	CloseHandle(handle);
	return false;
}

void Unlock() {
	auto handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	PROCESSENTRY32 procEntry = { 0 };
	procEntry.dwSize = sizeof(PROCESSENTRY32);
	Process32First(handle, &procEntry);
	do {
		if (!wcscmp(procEntry.szExeFile, L"lqndauccd.exe") || !wcscmp(procEntry.szExeFile, L"MaestroWebSvr.exe")
			|| !wcscmp(procEntry.szExeFile, L"qukapttp.exe") || !wcscmp(procEntry.szExeFile, L"MaestroWebAgent.exe")
			|| !wcscmp(procEntry.szExeFile, L"nfowjxyfd.exe") || !wcscmp(procEntry.szExeFile, L"notepad.exe")) {
			cout << "Latest CKIR process ["; wcout << procEntry.szExeFile; cout << "] ";
			if (TerminateProc(procEntry)) cout << "KILLED\n";
			else cout << "KILL failed\n";
			continue;
		}
		if (!wcscmp(procEntry.szExeFile, L"TDepend64.exe") || !wcscmp(procEntry.szExeFile, L"TDepend.exe")) {
			cout << "Remote control process ["; wcout << procEntry.szExeFile; cout << "] ";
			if (TerminateProc(procEntry)) cout << "KILLED\n";
			else cout << "KILL failed\n";
			continue;
		}
		if (!wcscmp(procEntry.szExeFile, L"rwtyijsa.exe") || !wcscmp(procEntry.szExeFile, L"nhfneczzm.exe")) {
			cout << "Unknown process ["; wcout << procEntry.szExeFile; cout << "] ";
			if (TerminateProc(procEntry)) cout << "KILLED\n";
			else cout << "KILL failed\n";
			continue;
		}
		if (!wcscmp(procEntry.szExeFile, L"SoluSpSvr.exe") || !wcscmp(procEntry.szExeFile, L"MaestroNAgent.exe")
			|| !wcscmp(procEntry.szExeFile, L"MaestroNSvr.exe")) {
			cout << "Legacy CKIR process ["; wcout << procEntry.szExeFile; cout << "] ";
			if (TerminateProc(procEntry)) cout << "KILLED\n";
			else cout << "KILL failed\n";
			continue;
		}
	} while (Process32Next(handle, &procEntry));
	CloseHandle(handle);
}
int main() {
	cin.tie(0), cout.tie(0)->sync_with_stdio(0);
	Unlock();
	cout << "\n\nPress Enter to exit...";
	getchar();
	return 0;
}
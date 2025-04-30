#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
using namespace std;

bool terminate_proc(PROCESSENTRY32& procEntry) {
	DWORD dwExitCode = 0;
	auto handle = OpenProcess(PROCESS_ALL_ACCESS, 0, procEntry.th32ProcessID);
	if (GetExitCodeProcess(handle, &dwExitCode)) {
		if (TerminateProcess(handle, dwExitCode)) {
			return true;
		}
	}
	return false;
}

void unlock() {
	auto handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	PROCESSENTRY32 procEntry = { 0 };
	procEntry.dwSize = sizeof(PROCESSENTRY32);
	Process32First(handle, &procEntry);
	do {
		if (!wcscmp(procEntry.szExeFile, L"lqndauccd.exe") || !wcscmp(procEntry.szExeFile, L"MaestroWebSvr.exe")
			|| !wcscmp(procEntry.szExeFile, L"qukapttp.exe") || !wcscmp(procEntry.szExeFile, L"MaestroWebAgent.exe")
			|| !wcscmp(procEntry.szExeFile, L"nfowjxyfd.exe")) {
			cout << "Latest CKIR process [" << procEntry.szExeFile << "] ";
			if (terminate_proc(procEntry)) cout << "KILLED\n";
			else cout << "KILL failed\n";
			continue;
		}
		if (!wcscmp(procEntry.szExeFile, L"TDepend64.exe") || !wcscmp(procEntry.szExeFile, L"TDepend.exe")) {
			cout << "Remote control process [" << procEntry.szExeFile << "] ";
			if (terminate_proc(procEntry)) cout << "KILLED\n";
			else cout << "KILL failed\n";
			continue;
		}
		if (!wcscmp(procEntry.szExeFile, L"rwtyijsa.exe") || !wcscmp(procEntry.szExeFile, L"nhfneczzm.exe")) {
			cout << "Unknown process [" << procEntry.szExeFile << "] ";
			if (terminate_proc(procEntry)) cout << "KILLED\n";
			else cout << "KILL failed\n";
			continue;
		}
		if (!wcscmp(procEntry.szExeFile, L"SoluSpSvr.exe") || !wcscmp(procEntry.szExeFile, L"MaestroNAgent.exe")
			|| !wcscmp(procEntry.szExeFile, L"MaestroNSvr.exe")) {
			cout << "Legacy CKIR process [" << procEntry.szExeFile << "] ";
			if (terminate_proc(procEntry)) cout << "KILLED\n";
			else cout << "KILL failed\n";
			continue;
		}
	} while (Process32Next(handle, &procEntry));
}
int main() {
	cin.tie(0), cout.tie(0)->sync_with_stdio(0);
	unlock();
	cout << "\n\n";
	system("PAUSE");
	return 0;
}
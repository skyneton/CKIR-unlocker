#include "inj.h"
#include <iostream>
using namespace std;
typedef struct _THREAD_PARAM {
	FARPROC pFunc[2];
	char szBuf[2][128];
} THREAD_PARAM, * PTHREAD_PARAM;

//LoadLibraryA()
typedef HMODULE(WINAPI* PFLOADLIBRARYA) (LPCSTR lpLibFileName);
//GetProcAddress()
typedef FARPROC(WINAPI* PFGETPROCADDRESS) (HMODULE hModule, LPCSTR lpProcName);
//MessageBoxA()
typedef void (WINAPI* PF_EXIT) (INT exitCode);

DWORD WINAPI ThreadProc(LPVOID lParam) {
	PTHREAD_PARAM pParam = (PTHREAD_PARAM)lParam;
	HMODULE hMod = ((PFLOADLIBRARYA)pParam->pFunc[0])(pParam->szBuf[0]);
	FARPROC pFunc = (FARPROC)((PFGETPROCADDRESS)pParam->pFunc[1])(hMod, pParam->szBuf[1]);
	((PF_EXIT)pFunc)(0);
	return 0;
}

bool InjectCode(HANDLE &hProcess) {
	THREAD_PARAM param = { 0, };
	LPVOID pRemoteBuf[2] = { 0, };
	DWORD dwSize = sizeof(THREAD_PARAM);
	HANDLE hThread = NULL;
	HMODULE hMod = GetModuleHandleA("kernel32.dll");
	param.pFunc[0] = GetProcAddress(hMod, "LoadLibraryA");
	param.pFunc[1] = GetProcAddress(hMod, "GetProcAddress");
	strcpy_s(param.szBuf[0], "msvcrt.dll");
	strcpy_s(param.szBuf[1], "_exit");
	if (!(pRemoteBuf[0] = VirtualAllocEx(hProcess, NULL, dwSize, MEM_COMMIT, PAGE_READWRITE))) {
		cout << "\033[1;33m* VirtualAllocEx() A fail : err_code=" << (long long)GetLastError() << "\033[0m\n";
		return false;
	}

	if (!WriteProcessMemory(hProcess, pRemoteBuf[0], (LPVOID)&param, dwSize, NULL)) {
		cout << "\033[1;33m* WriteProcessMemory() A fail : err_code=" << (long long)GetLastError() << "\033[0m\n";
		return false;
	}
	dwSize = (DWORD)InjectCode - (DWORD)ThreadProc;
	if (!(pRemoteBuf[1] = VirtualAllocEx(hProcess, NULL, dwSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE))) {
		cout << "\033[1;33m* VirtualAllocEx() B fail : err_code=" << (long long)GetLastError() << "\033[0m\n";
		return false;
	}

	if (!WriteProcessMemory(hProcess, pRemoteBuf[1], (LPVOID)ThreadProc, dwSize, NULL)) {
		cout << "\033[1;33m* WriteProcessMemory() B fail : err_code=" << (long long)GetLastError() << "\033[0m\n";
		return false;
	}


	if (!(hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pRemoteBuf[1], pRemoteBuf[0], 0, NULL))) {
		cout << "\033[1;33m* CreateRemoteThread() fail : err_code=" << (long long)GetLastError() << "\033[0m\n";
		return false;
	}

	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	CloseHandle(hProcess);
	return true;
}
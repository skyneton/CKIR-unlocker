#include "Processor.h"
using namespace std;

BOOL KillProcessorByUserMode(DWORD dwProcessId) {
	HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (hThreadSnap == INVALID_HANDLE_VALUE) {
		cout << red << "CreateToolhelp32Snapshot failed.\n" << white;
		return FALSE;
	}

	THREADENTRY32 te32;
	te32.dwSize = sizeof(THREADENTRY32);

	if (!Thread32First(hThreadSnap, &te32)) {
		cout << red << "Thread32First failed.\n" << white;
		CloseHandle(hThreadSnap);
		return FALSE;
	}

	do {
		if (te32.th32OwnerProcessID == dwProcessId) {
			HANDLE hThread = OpenThread(THREAD_TERMINATE, FALSE, te32.th32ThreadID);
			if (hThread != NULL) {
				TerminateThread(hThread, 0);
				CloseHandle(hThread);
			}
		}
	} while (Thread32Next(hThreadSnap, &te32));

	CloseHandle(hThreadSnap);

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, true, dwProcessId);
	if (hProcess != NULL && !InjectCode(hProcess)) {
		TerminateProcess(hProcess, 0);
		CloseHandle(hProcess);
	}

	return TRUE;
}

static PVOID SearchMemory(PVOID pStartAddress, PVOID pEndAddress, PUCHAR pMemoryData, ULONG ulMemoryDataSize) {
	PVOID pAddress = NULL;
	for (PUCHAR i = (PUCHAR)pStartAddress; i < (PUCHAR)pEndAddress; i++) {
		ULONG m = 0;
		for (; m < ulMemoryDataSize; m++) {
			if (*(PUCHAR)(i + m) != pMemoryData[m]) break;
		}
		if (m >= ulMemoryDataSize) {
			pAddress = (PVOID)(i + ulMemoryDataSize);
			break;
		}
	}
	return pAddress;
}

/*
static PVOID SearchPspTerminateThreadByPointer(PUCHAR pSpecialData, ULONG ulSpecialDataSize) {
	UNICODE_STRING ustrFuncName;
	RtlInitUnicodeString(&ustrFuncName, L"PsTerminateSystemThread");
	PVOID pPspTerminateSystemThread = MmGetSystemRoutineAddress(&ustrFuncName);
	PVOID pPspTerminateThreadByPointer = NULL;
	if (!pPspTerminateSystemThread) {
		cout << red << "MmGetSystemRoutineAddress\n" << white;
		return pPspTerminateThreadByPointer;
	}

	PVOID pAddress = SearchMemory(pPspTerminateSystemThread, (PVOID)((PUCHAR)pPspTerminateSystemThread + 0xFF), pSpecialData, ulSpecialDataSize);
	if (!pAddress) {
		cout << red << "SearchMemory\n" << white;
		return pPspTerminateThreadByPointer;
	}
	LONG lOffset = *(PLONG)pAddress;
	pPspTerminateThreadByPointer = (PVOID)((PUCHAR)pAddress + sizeof(LONG) + lOffset);
	return pPspTerminateThreadByPointer;
}

static PVOID GetPspLoadImageNotifyRoutine() {
	RTL_OSVERSIONINFOW osInfo = { 0 };
	UCHAR pSpecialData[50] = { 0 };
	ULONG ulSpecialDataSize = 0;

	RtlGetVersion(&osInfo);
	if (6 == osInfo.dwMajorVersion) {
		if (1 == osInfo.dwMinorVersion) { // Win7
			pSpecialData[0] = 0xE8;
			ulSpecialDataSize = 1;
		}
		else if (2 == osInfo.dwMinorVersion); // Win8
		else if (3 == osInfo.dwMinorVersion) {
#ifdef _WIN64
			pSpecialData[0] = 0xE9;
#else
			pSpecialData[0] = 0xE8;
#endif
			ulSpecialDataSize = 1;
		}
	}
	else if (10 == osInfo.dwMajorVersion) {
#ifdef _WIN64
		pSpecialData[0] = 0xE9;
#else
		pSpecialData[0] = 0xE8;
#endif
		ulSpecialDataSize = 1;
	}
	return SearchPspTerminateThreadByPointer(pSpecialData, ulSpecialDataSize);
}
NTSTATUS KillProcessor(HANDLE hProcessId) {
#ifdef _WIN64
	typedef NTSTATUS(__fastcall* PSPTERMINATETHREADBYPOINTER)(PETHREAD pEthread, NTSTATUS ntExitCode, BOOLEAN bDirectTerminate);
#else
	typedef NTSTATUS(*PSPTERMINATETHREADBYPOINTER)(PETHREAD pEthread, NTSTATUS ntExitCode, BOOLEAN bDirectTerminate);
#endif
	PVOID pPspTerminateThreadByPointerAddress = GetPspLoadImageNotifyRoutine();
	if (!pPspTerminateThreadByPointerAddress) {
		cout << red << "GetPspLoadImageNotifyRoutine\n" << white;
		return -1;
	}

	PEPROCESS pEProcess = NULL;
	NTSTATUS status = PsLookupProcessByProcessId(hProcessId, &pEProcess);
	if (!NT_SUCCESS(status)) {
		cout << red << "PsLookupProcessByProcessId - " << status << '\n' << white;
		return status;
	}

	PETHREAD pEThread = NULL;
	PEPROCESS pThreadEProcess = NULL;
	for (ULONG i = 4; i < 0x80000; i += 4) {
		status = PsLookupThreadByThreadId((HANDLE)i, &pEThread);
		if (NT_SUCCESS(status)) {
			pThreadEProcess = PsGetThreadProcess(pEThread);
			if (pEProcess == pThreadEProcess) {
				((PSPTERMINATETHREADBYPOINTER)pPspTerminateThreadByPointerAddress)(pEThread, 0, 1);
			}
			ObDereferenceObject(pEThread);
		}
	}

	ObDereferenceObject(pEProcess);
	return STATUS_SUCCESS;
}
*/
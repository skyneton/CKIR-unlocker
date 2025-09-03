#include "Processor.h"
using namespace std;

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
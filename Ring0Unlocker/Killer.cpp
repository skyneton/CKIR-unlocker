#include "Killer.h"

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
		LogError("MmGetSystemRoutineAddress", 0);
		return pPspTerminateThreadByPointer;
	}

	PVOID pAddress = SearchMemory(pPspTerminateSystemThread, (PVOID)((PUCHAR)pPspTerminateSystemThread + 0xFF), pSpecialData, ulSpecialDataSize);
	if (!pAddress) {
		LogError("SearchMemory", 0);
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
		LogError("GetPspLoadImageNotifyRoutine", 0);
		return -1;
	}

	PEPROCESS pEProcess = NULL;
	NTSTATUS status = PsLookupProcessByProcessId(hProcessId, &pEProcess);
	if (!NT_SUCCESS(status)) {
		LogError("PsLookupProcessByProcessId", status);
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

PEPROCESS FindProcessor(const wchar_t* name) {
	PEPROCESS nextProcess = PsInitialSystemProcess;
	if (!nextProcess) return NULL;
	NTSTATUS status = STATUS_SUCCESS;
	while (NT_SUCCESS(status)) {
		wchar_t path[400];
		PsGetFileName(nextProcess, path);
		if (!wcscmp(path, name)) {
			status = ObReferenceObjectByPointer(nextProcess, PROCESS_ALL_ACCESS, *PsProcessType, KernelMode);
			return nextProcess;
		}
		status = PsGetNextProcess(&nextProcess);
	}
	return NULL;
}

int LoopKiller(const char* type, int count, const wchar_t* list[], int length) {
	int remain = 0;
	Log(type); Log(" Process Detecting...\n");
	for (int i = 0; i < length; i++) {
		auto p = FindProcessor(list[i]);
		if (!p) continue;
		HANDLE pid = PsGetProcessId(p);
		remain++;
		Log("Detected ["); Log(list[i]); Log("](PID:"); Log((long long)pid); Log(")\n");

		wchar_t filePath[400] = { 0 };
		int len = PsGetFullName(p, filePath);
		Log("PATH: "); Log(filePath); Log("\n");
		bool killed = false;
		bool deleted = false;

		for (int j = 0; j < count; j++) {
			p = FindProcessor(list[i]);
			if (!p) {
				if (len && NT_SUCCESS(DeleteFile(filePath))) {
					deleted = true;
				}
				//killed = 1;
				//break;
				continue;
			}

			int result = KillProcessor(pid);
			if (!NT_SUCCESS(result)) continue;
			if (len && NT_SUCCESS(DeleteFile(filePath))) {
				deleted = true;
				//killed = true;
				//break;
			}
		}

		if (!killed) {
			p = FindProcessor(list[i]);
			if (!p) killed = true;
		}
		Log("Try To Kill...\n");
		if (killed) Log("Success!");
		else Log("Failed...");
		Log('\n');

		Log("Try To Delete Execution File...\n");
		if (deleted) Log("Success!");
		else Log("Failed...");
		Log('\n');
		remain -= deleted;
	}
	return remain;
}
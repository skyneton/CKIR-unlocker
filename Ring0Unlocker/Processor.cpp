#include "Processor.h"

NTSTATUS NTAPI PsGetFirstProcess(OUT PEPROCESS* firstProcess) {
	if (!firstProcess) return STATUS_INVALID_PARAMETER;
	*firstProcess = PsInitialSystemProcess;
	if (!*firstProcess) return STATUS_NOT_FOUND;
	return STATUS_SUCCESS;
}

int PsGetFullName(PEPROCESS& process, wchar_t* path) {
	PUNICODE_STRING processImageName = NULL;
	if (NT_SUCCESS(SeLocateProcessImageName(process, &processImageName)) && !processImageName) {
		wcscpy(path, processImageName->Buffer);
		int size = processImageName->Length;
		ExFreePool(processImageName);
		return size;
	}
	return 0;
}

int PsGetFileName(PEPROCESS& process, wchar_t* path) {
	wchar_t fullPath[400];
	int len = PsGetFullName(process, fullPath);
	if (len) {
		wchar_t* start = wcsrchr(fullPath, L'\\') + 1;
		wcscpy(path, start);
		return len - (int)(start - fullPath);
	}
	return 0;
}

NTSTATUS NTAPI DeleteFile(wchar_t* path) {
	OBJECT_ATTRIBUTES oa = {};
	wchar_t fullPath[400] = L"\\?\\";
	wcscat(fullPath, path);
	UNICODE_STRING file;
	RtlInitUnicodeString(&file, fullPath);
	InitializeObjectAttributes(&oa, &file, OBJ_CASE_INSENSITIVE, NULL, NULL);
	NTSTATUS status = ZwDeleteFile(&oa);
	RtlFreeUnicodeString(&file);
	return status;
}

/*
NTSTATUS NTAPI PsGetNextProcess(IN OUT PEPROCESS* pProcess) {
	// 1. Validate the IN OUT parameter
	if (!pProcess || !*pProcess) {
		return STATUS_INVALID_PARAMETER;
	}

	// WARNING: This offset is the crucial part. The method in your original
	// code for finding it was flawed. You must determine this offset reliably for your
	// target Windows version. This value is just an EXAMPLE for Windows 10 x64 (1903).
	ULONG activeProcessLinksOffset = 0x448;

	// 2. Get the current process and its list entry
	PEPROCESS currentEprocess = *pProcess;
	PLIST_ENTRY pCurrentListEntry = (PLIST_ENTRY)((PCHAR)currentEprocess + activeProcessLinksOffset);

	// 3. Find the list entry of the next process in the circular list
	PLIST_ENTRY pNextListEntry = pCurrentListEntry->Flink;

	// 4. Calculate the base address of the next EPROCESS structure.
	// This is the correct way to get the parent structure from a member's address
	// and fixes the "incomplete type" error.
	PEPROCESS nextEprocess = (PEPROCESS)((PCHAR)pNextListEntry - activeProcessLinksOffset);

	// Using a handle to the System process for comparison. Its PID is typically 4.
	HANDLE systemProcessId = PsGetProcessId(PsInitialSystemProcess);

	// 5. Check if the next process is valid and not the System process (as in your original logic)
	if (nextEprocess && PsGetProcessId(nextEprocess) != systemProcessId) {
		// 6. Success! Increment the reference count of the new process object we are returning.
		ObReferenceObject(nextEprocess);

		// 7. Dereference the old process object that was passed in.
		ObDereferenceObject(currentEprocess);

		// 8. Update the caller's pointer to point to the new process.
		*pProcess = nextEprocess;
		return STATUS_SUCCESS;
	}

	// 9. The next process was the system process or invalid.
	return STATUS_NOT_FOUND;
}

*/
NTSTATUS NTAPI PsGetNextProcess(IN OUT PEPROCESS* nextProcess) {
	ULONG activeProcessLinksOffset = 0, processIdOffset = 0;
	BOOLEAN bFound = FALSE;
	PLIST_ENTRY activeProcessLinks;
	PEPROCESS process;
	PEPROCESS currentProcess = PsGetCurrentProcess();
	HANDLE currentProcessId = PsGetCurrentProcessId();
	HANDLE systemProcessId = PsGetProcessId(PsInitialSystemProcess);
	if (!nextProcess) return STATUS_INVALID_PARAMETER;
	NTSTATUS status = ObReferenceObjectByPointer(*nextProcess, PROCESS_ALL_ACCESS, *PsProcessType, KernelMode);
	if (!NT_SUCCESS(status)) return status;
	for (processIdOffset = 0; processIdOffset < PAGE_SIZE * 4; processIdOffset++) {
		if (*(PHANDLE)((PCHAR)currentProcess + processIdOffset) == currentProcessId) {
			bFound = true;
			break;
		}
	}
	if (!bFound) return STATUS_UNSUCCESSFUL;
	activeProcessLinksOffset = processIdOffset + 4;
	activeProcessLinks = (PLIST_ENTRY)((char*)*nextProcess + activeProcessLinksOffset);
	process = (PEPROCESS)activeProcessLinks->Flink;
	process = (PEPROCESS)((char*)process - activeProcessLinksOffset);
	if (process) {
		if (systemProcessId != PsGetProcessId(process)) {
			ObDereferenceObject(*nextProcess);
			*nextProcess = process;
			return STATUS_SUCCESS;
		}
	}
	return STATUS_NOT_FOUND;
	//https://skensita.tistory.com/entry/PsGetFirstProcessPsGetNextProcess-%EA%B5%AC%ED%98%84
}
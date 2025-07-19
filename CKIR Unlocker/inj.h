#pragma once
#include <Windows.h>
typedef struct _CLIENT_ID {
	PVOID UniqueProcess;
	PVOID UniqueThread;
} CLIENT_ID, *PCLIENT_ID;
typedef NTSTATUS(WINAPI* PROC_RtlCreateUserThread)(
	HANDLE handle,
	PSECURITY_DESCRIPTOR sd,
	BOOLEAN suspended,
	ULONG zeroBits,
	SIZE_T stackReverse,
	SIZE_T stackCommit,
	PTHREAD_START_ROUTINE startAddress,
	PVOID parameter,
	PHANDLE threadHandle,
	PCLIENT_ID client
	);
bool InjectCode(HANDLE& hProcess);
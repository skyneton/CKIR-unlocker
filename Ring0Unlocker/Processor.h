#pragma once
#include <ntifs.h>

NTSTATUS NTAPI PsGetNextProcess(IN OUT PEPROCESS* nextProcess);
int PsGetFullName(PEPROCESS& process, wchar_t* path);
int PsGetFileName(PEPROCESS& process, wchar_t* path);
NTSTATUS NTAPI DeleteFile(wchar_t* path);

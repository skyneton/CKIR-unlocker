#pragma once
#include <ntifs.h>
#include "Logger.h"
#include "Processor.h"

PEPROCESS FindProcessor(const wchar_t* name);
NTSTATUS KillProcessor(HANDLE hProcessId);
int LoopKiller(const char* type, int count, const wchar_t* list[], int length);
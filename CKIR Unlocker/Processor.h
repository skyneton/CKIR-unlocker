#pragma once

//#include "ntdll\ntos.h"
#include<Windows.h>
#include <tlhelp32.h>
#include "color.h"
#include "inj.h"

BOOL KillProcessorByUserMode(DWORD dwProcessId);
//typedef struct _KPROCESS* PEPROCESS;
//typedef struct _KTHREAD* PETHREAD;
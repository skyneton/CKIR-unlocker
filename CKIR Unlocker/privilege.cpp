#include "privilege.h"
#include <Windows.h>
#include <TlHelp32.h>
#include<stdio.h>

bool Privilege() {
    HANDLE token;
    OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token);
    TOKEN_PRIVILEGES tp;
    LUID luid;
    LookupPrivilegeValueA(NULL, "SeDebugPrivilege", &luid);
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    //privs.Luid.LowPart = privs.Luid.HighPart = 0;
	return AdjustTokenPrivileges(token, FALSE, &tp, sizeof(tp), NULL, NULL);
}
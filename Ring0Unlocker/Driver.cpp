#include "Driver.h"
#pragma warning (disable: 4100)
#define LOOP_COUNT 70
const wchar_t* latest[] = { L"lqndauccd.exe", L"MaestroWebSvr.exe", L"qukapttp.exe", L"MaestroWebAgent.exe", L"nfowjxyfd.exe", L"SoluLock.exe" };
const wchar_t* remote[] = { L"TDepend64.exe", L"TDepend.exe" };
const wchar_t* unknown[] = { L"rwtyijsa.exe", L"nhfneczzm.exe", L"rzzykzbis.exe" };
const wchar_t* legacy[] = { L"SoluSpSvr.exe", L"MaestroNAgent.exe", L"MaestroNSvr.exe" };


extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath) {
	pDriverObject->DriverUnload = DriverUnload;
    int remain = 0;
    remain += LoopKiller("Latest CKIR", LOOP_COUNT, latest, sizeof(latest) / sizeof(const wchar_t*));
    remain += LoopKiller("Remote Control", LOOP_COUNT, remote, sizeof(remote) / sizeof(const wchar_t*));
    remain += LoopKiller("Unknown", LOOP_COUNT, unknown, sizeof(unknown) / sizeof(const wchar_t*));
    remain += LoopKiller("Legacy", LOOP_COUNT, legacy, sizeof(legacy) / sizeof(const wchar_t*));
    Log("\nTotal Remain Processors: "); Log(remain);
	return STATUS_SUCCESS;
}

VOID DriverUnload(PDRIVER_OBJECT pDriverObject) {

}
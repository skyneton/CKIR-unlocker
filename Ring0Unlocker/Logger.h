#pragma once
#include<wchar.h>

void Log(const char* msg);
void Log(int msg);
void Log(long long msg);
void Log(const wchar_t* msg);
void LogError(const char* msg, int code);

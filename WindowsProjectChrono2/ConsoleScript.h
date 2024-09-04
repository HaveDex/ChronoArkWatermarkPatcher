#pragma once
#include <windows.h>

DWORD GetProcessIdByName(const wchar_t* processName);
bool PatchGameMemory(DWORD processId);
bool PatchModString(DWORD processId);
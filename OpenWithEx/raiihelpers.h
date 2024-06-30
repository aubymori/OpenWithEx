#pragma once

#include <windows.h>

// RAII helper for WinAPI HANDLE.
struct CloseHandleDeleter
{
	void operator()(HANDLE handle) const { CloseHandle(handle); }
};

// RAII helper for WinAPI LocalFree.
struct LocalFreeDeleter
{
	void operator()(void *handle) const { LocalFree(handle); }
};

// RAII helper for WinAPI RegCloseKey.
struct RegCloseKeyDeleter
{
	void operator()(HKEY hKey) const { RegCloseKey(hKey); }
};
#include "util.h"
#include "openwithex.h"
#include <stdio.h>

int LocalizedMessageBox(
	HWND hWndParent,
	UINT uMsgId,
	UINT uType
)
{
	WCHAR szMsg[1024] = { 0 };
	LoadStringW(g_hMuiInstance, uMsgId, szMsg, 1024);
	return MessageBoxW(
		hWndParent,
		szMsg,
		L"OpenWithEx",
		uType
	);
}

/**
  * Get the registry key associated with a file type.
  * 
  * For example, passing L".dll" in will return a handle to
  * `HKEY_CLASSES_ROOT\dllfile`.
  */
HKEY GetExtensionRegKey(
	LPCWSTR lpszExtension
)
{
	if (!lpszExtension || !*lpszExtension)
	{
		return NULL;
	}

	HKEY hk = NULL;
	RegOpenKeyExW(
		HKEY_CLASSES_ROOT,
		lpszExtension,
		NULL,
		KEY_READ,
		&hk
	);

	WCHAR szKeyName[MAX_PATH] = { 0 };
	DWORD dwKeyNameSize = sizeof(szKeyName);
	RegQueryValueExW(
		hk,
		L"",
		NULL,
		NULL,
		(LPBYTE)szKeyName,
		&dwKeyNameSize
	);

	RegCloseKey(hk);

	if (!*szKeyName)
	{
		return NULL;
	}

	HKEY result = NULL;
	RegOpenKeyExW(
		HKEY_CLASSES_ROOT,
		szKeyName,
		NULL,
		KEY_READ,
		&result
	);
	return result;
}
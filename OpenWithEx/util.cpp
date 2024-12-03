#include "util.h"
#include "openwithex.h"
#include <stdio.h>

#include "assocuserchoice.h"
#include "wil/com.h"
#include "wil/resource.h"

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
bool GetExtensionRegKey(
	LPCWSTR  lpszExtension,
	HKEY    *pHkOut
)
{
	if (!lpszExtension || !*lpszExtension || !pHkOut)
	{
		return false;
	}
	*pHkOut = NULL;

	wil::unique_hkey hk = NULL;
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
		hk.get(),
		L"",
		NULL,
		NULL,
		(LPBYTE)szKeyName,
		&dwKeyNameSize
	);

	hk.reset();

	if (!*szKeyName)
	{
		return false;
	}

	HKEY hkResult = NULL;
	RegOpenKeyExW(
		HKEY_CLASSES_ROOT,
		szKeyName,
		NULL,
		KEY_READ,
		&hkResult
	);
	*pHkOut = hkResult;
	return (hkResult != NULL);
}

/**
  * Checks if an association exists for a file extension or
  * URL protocol.
  */
bool AssociationExists(LPCWSTR lpszExtension, bool fIsUri)
{
	if (!lpszExtension)
		return false;

	wil::unique_hkey hk;
	if (GetExtensionRegKey(lpszExtension, &hk) && hk.get())
		return true;

	std::unique_ptr<WCHAR[]> lpszKeyPath = GetAssociationKeyPath(lpszExtension, fIsUri);
	if (STATUS_SUCCESS == RegOpenKeyExW(
		HKEY_CURRENT_USER,
		lpszKeyPath.get(),
		NULL,
		KEY_READ,
		&hk
	) && hk.get())
		return true;

	return false;
}
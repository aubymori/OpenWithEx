#pragma once

#include <windows.h>

class CShellProtectedRegLock
{
protected:
	PTOKEN_USER _pToken;
	HKEY _hkeySecurity;
	PACL _pDacl;
	PSECURITY_DESCRIPTOR _psd;

	// Custom method:
	HRESULT QueryUserToken(HKEY hKey, LPCWSTR lpwszValue);

	// Custom method:
	static HRESULT OpenEffectiveToken(OUT PSID *ppSid);

public:
	~CShellProtectedRegLock();

	LSTATUS Init(HKEY hKey, LPCWSTR lpwszValue);

	void Lock();

	void Unlock();
};

LSTATUS SHSetProtectedValue(HKEY hKey, LPCWSTR pszSubKey, LPCWSTR pszValue, bool unused, LPCVOID pvData, DWORD cbData);

LSTATUS SHDeleteProtectedValue(HKEY hKey, LPCWSTR pszSubKey, LPCWSTR pszValue, bool bDeleteSubKeys);
#pragma once

#include <windows.h>

class CShellProtectedRegLock
{
protected:
	PTOKEN_USER m_pTokenUser;
	HKEY m_hKey;
	PACL m_pAcl;
	PSECURITY_DESCRIPTOR m_pSecurityDescriptor;

	HRESULT QueryUserToken(HKEY hKey, LPCWSTR lpwszValue);
	static HRESULT OpenEffectiveToken(OUT PSID *ppSid);

public:
	~CShellProtectedRegLock();

	LSTATUS Init(HKEY hKey, LPCWSTR lpwszValue);

	void Lock();

	void Unlock();
};

LSTATUS SHSetProtectedValue(HKEY hKey, LPCWSTR pszSubKey, LPCWSTR pszValue, bool unused, LPCVOID pvData, DWORD cbData);

LSTATUS SHDeleteProtectedValue(HKEY hKey, LPCWSTR pszSubKey, LPCWSTR pszValue, bool bDeleteSubKeys);
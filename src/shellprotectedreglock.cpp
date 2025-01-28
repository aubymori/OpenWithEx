/**
 * Reimplementation of CShellProtectedRegLock and SH****ProtectedValue APIs.
 * by Isabella Lulamoon (kawapure)
 * 
 * These are associated with the User Choice protection system. I believe
 * that they are exclusively used for it.
 * 
 * The original implementations are all in TWinUI.dll.
 * 
 * This composes a surprisingly simple system. No magic constants or
 * verification algorithms or anything are used for protection. This simply
 * deletes the permissions in the registry denying modification by the user,
 * changes the value, and then creates a new set of permissions.
 * 
 * In fact, it doesn't even care to check what's what. The unlocking mechanism
 * simply deletes all "access denied" permission entries, regardless of
 * whom it applies to, and then the locking mechanism just creates a whole new
 * set of permissions for the current user.
 * 
 * I PROMISE that this is really Microsoft's code. This isn't my way of
 * oversimplifying the code for this specific implementation; this is almost-
 * exactly reverse engineered from my local copy of TWinUI.dll.
 * 
 * The only things I took liberties on were:
 *    - I moved "SHQueryToken<TOKEN_USER>" and its dependency
 *      "SHOpenEffectiveToken" to private methods of CShellProtectedRegLock.
 *    - I did not reimplement the helper class "CTLocalAllocPolicy". This
 *      can be found in the Windows Driver Kit, but it's a rather useless
 *      wrapper for LocalAlloc. It doesn't even manage RAII for C++, so I don't
 *      really know what it's meant for.
 *      https://github.com/wmliang/wdk-10/blob/master/Include/10.0.14393.0/um/memsafe.h#L359
 * 
 * Also Microsoft has a patent on this mechanism LOL:
 * https://patents.google.com/patent/US20130198646A1/en
 */

#include <windows.h>
#include <aclapi.h>
#include <processthreadsapi.h>
#include <securitybaseapi.h>
#include <shlwapi.h>
#include <wchar.h>

#include "shellprotectedreglock.h"

SID c_sidLocalSystem = { 0x1, 0x1, { 0, 0, 0, 0, 0, 0x5 }, 0x12 };

CShellProtectedRegLock::~CShellProtectedRegLock()
{
	if (_hkeySecurity)
		RegCloseKey(_hkeySecurity);
	LocalFree(_pToken);
	LocalFree(_psd);
}

HRESULT CShellProtectedRegLock::QueryUserToken(HKEY hKey, LPCWSTR lpwszValue)
{
	constexpr int kTokenInfoBufferSize = 2048;
	PSID pSid = nullptr;
	HRESULT hr = s_OpenEffectiveToken(&pSid);
	DWORD dwLastError = 0;

	_pToken = (PTOKEN_USER)LocalAlloc(LPTR, kTokenInfoBufferSize);

	if (_pToken)
	{
		DWORD dwTokenLength = kTokenInfoBufferSize;
		if (!GetTokenInformation(pSid, TokenUser, _pToken, dwTokenLength, &dwTokenLength))
		{
			dwLastError = GetLastError();

			if (dwLastError == ERROR_INSUFFICIENT_BUFFER)
			{
				LocalFree(_pToken);

				_pToken = (PTOKEN_USER)LocalAlloc(LPTR, dwTokenLength);

				if (_pToken)
				{
					if (GetTokenInformation(pSid, TokenUser, _pToken, dwTokenLength, &dwTokenLength))
					{
						hr = S_OK;
					}
				}
				else
				{
					hr = E_FAIL;
				}
			}
			else
			{
				hr = HRESULT_FROM_WIN32(dwLastError);
			}
		}
	}
	else // !_pToken
	{
		hr = E_FAIL;
	}

	if (FAILED(hr))
	{
		LocalFree(_pToken);
		_pToken = nullptr;
	}

	if (pSid)
	{
		CloseHandle(pSid);
	}

	return hr;
}

// static
HRESULT CShellProtectedRegLock::s_OpenEffectiveToken(OUT PSID *ppSid)
{
	*ppSid = nullptr;

	HANDLE hCurrentThread = GetCurrentThread();
	if (OpenThreadToken(hCurrentThread, TOKEN_QUERY, FALSE, ppSid))
	{
		return S_OK;
	}

	DWORD dwLastError = GetLastError();
	if (dwLastError == ERROR_NO_TOKEN)
	{
		HANDLE hCurrentProcess = GetCurrentProcess();
		if (OpenProcessToken(hCurrentProcess, TOKEN_QUERY, ppSid))
		{
			return S_OK;
		}

		dwLastError = GetLastError();
	}

	WCHAR aaaa[1024];
	DWORD error = GetLastError();
	swprintf_s(aaaa, L"dude it's so FUCKING over: %d", dwLastError);
	OutputDebugStringW(aaaa);

	return HRESULT_FROM_WIN32(dwLastError);
}

LSTATUS CShellProtectedRegLock::Init(HKEY hKey, LPCWSTR lpwszValue)
{
	if (SUCCEEDED(QueryUserToken(hKey, lpwszValue)))
	{
		return RegCreateKeyExW(
			hKey, 
			L"UserChoice", 
			0, 
			nullptr, 
			0,
			READ_CONTROL | WRITE_DAC,
			nullptr,
			&_hkeySecurity,
			nullptr
		);
	}
	else
	{
		return ERROR_NOT_ENOUGH_MEMORY;
	}
}

void CShellProtectedRegLock::Lock()
{
	ACL_SIZE_INFORMATION sizeInfo;

	if (
		_hkeySecurity &&
		!EqualSid(&c_sidLocalSystem, _pToken->User.Sid) &&
		GetAclInformation(_pDacl, &sizeInfo, sizeof(sizeInfo), AclSizeInformation)
	)
	{
		DWORD dwBytesInUse = sizeInfo.AclBytesInUse;
		DWORD dwRevision = 2;

		ACL_REVISION_INFORMATION revisionInfo;

		if (GetAclInformation(_pDacl, &revisionInfo, sizeof(revisionInfo), AclRevisionInformation))
		{
			dwRevision = revisionInfo.AclRevision;
		}

		DWORD dwSidLen = GetLengthSid(_pToken->User.Sid);
		DWORD dwAceListLength = dwSidLen + 8;
		DWORD dwBufferSize = dwAceListLength + dwBytesInUse;
		PACL pNewAcl = nullptr;

		if (
			dwAceListLength <= MAXWORD &&
			dwBufferSize <= MAXWORD &&
			(pNewAcl = (PACL)LocalAlloc(LPTR, dwBufferSize))
		)
		{
			InitializeAcl(pNewAcl, dwBufferSize, dwRevision);

			PACCESS_DENIED_ACE pNewAce = (PACCESS_DENIED_ACE)((PBYTE)pNewAcl + dwAceListLength);
			pNewAce->Header.AceType = ACCESS_DENIED_ACE_TYPE;
			pNewAce->Header.AceSize = dwAceListLength;
			pNewAce->Mask = KEY_SET_VALUE;
			memcpy(&pNewAce->SidStart, _pToken->User.Sid, dwSidLen);

			if (AddAce(pNewAcl, dwRevision, MAXDWORD, pNewAce, dwAceListLength))
			{
				bool fGiveUp = false;

				if (sizeInfo.AceCount)
				{
					for (int i = 0; i < sizeInfo.AceCount; i++)
					{
						PACL pCurrentAcl;
						if (GetAce(_pDacl, i, (LPVOID *)&pCurrentAcl))
						{
							if (!AddAce(pNewAcl, dwRevision, MAXDWORD, pCurrentAcl, pCurrentAcl->AclSize))
							{
								fGiveUp = true;
							}
						}
					}
				}

				if (!fGiveUp)
				{
					SetSecurityInfo(
						_hkeySecurity,
						SE_REGISTRY_KEY,
						DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION,
						nullptr,
						nullptr,
						pNewAcl,
						nullptr
					);
				}
			}
		}

		if (pNewAcl)
			LocalFree(pNewAcl);
	}
}

void CShellProtectedRegLock::Unlock()
{
	if (
		GetSecurityInfo(
			_hkeySecurity,
			SE_REGISTRY_KEY,
			DACL_SECURITY_INFORMATION,
			nullptr,
			nullptr,
			&_pDacl,
			nullptr,
			&_psd
		) == ERROR_SUCCESS
	)
	{
		ACL_SIZE_INFORMATION sizeInfo;
		PACCESS_DENIED_ACE pAce;

		if (_pDacl)
		{
			if (GetAclInformation(_pDacl, &sizeInfo, sizeof(sizeInfo), AclSizeInformation))
			{
				for (int i = sizeInfo.AceCount - 1; i >= 0; --i)
				{
					if (
						GetAce(_pDacl, i, (LPVOID *)&pAce) &&
						pAce->Header.AceType == ACCESS_DENIED_ACE_TYPE &&
						pAce->Mask == KEY_SET_VALUE
					)
					{
						if (DeleteAce(_pDacl, i))
						{
							SetSecurityInfo(
								_hkeySecurity,
								SE_REGISTRY_KEY,
								DACL_SECURITY_INFORMATION,
								nullptr,
								nullptr,
								_pDacl,
								nullptr
							);
						}
					}
				}
			}
		}
	}
}

LSTATUS SHSetProtectedValue(HKEY hKey, LPCWSTR pszSubKey, LPCWSTR pszValue, bool unused, LPCVOID pvData, DWORD cbData)
{
	LSTATUS ls = ERROR_SUCCESS;
	CShellProtectedRegLock lock;

	ls = lock.Init(hKey, pszValue);
	if (!ls)
	{
		lock.Unlock();
		ls = SHSetValueW(hKey, L"UserChoice", pszValue, REG_SZ, pvData, cbData);
		lock.Lock();
	}
	
	return ls;
}

LSTATUS SHDeleteProtectedValue(HKEY hKey, LPCWSTR pszSubKey, LPCWSTR pszValue, bool bDeleteSubKeys)
{
	LSTATUS ls = ERROR_SUCCESS;
	CShellProtectedRegLock lock;

	ls = lock.Init(hKey, pszValue);
	if (!ls)
	{
		lock.Unlock();
		
		ls = SHDeleteValueW(hKey, L"UserChoice", pszValue);

		HKEY hSubKey;
		bool fShouldLock = true;
		if (bDeleteSubKeys && RegOpenKeyExW(hKey, L"UserChoice", 0, KEY_QUERY_VALUE, &hSubKey))
		{
			DWORD cSubKeys;
			DWORD cValues;

			if (
				!RegQueryInfoKeyW(hSubKey, nullptr, nullptr, nullptr, &cSubKeys, nullptr, nullptr, &cValues, nullptr, nullptr, nullptr, nullptr) &&
				!cSubKeys &&
				!cValues &&
				!RegDeleteKeyExW(hKey, L"UserChoice", 0, 0)
			)
			{
				fShouldLock = false;
			}

			RegCloseKey(hKey);
		}

		if (fShouldLock)
			lock.Lock();
	}

	return ls;
}
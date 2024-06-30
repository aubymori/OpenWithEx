/**
 * Reimplementation of CShellProtectedRegLock and SH****ProtectedValue APIs.
 * by Isabella (kawapure)
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
 *    - I did not reimplement the helper function "CTLocalAllocPolicy". This
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

/**
 * "Restricted Code" SID
 * 
 * An identity that's used by a process that's running in a restricted security
 * context. When code runs at the restricted security level, the Restricted SID
 * is added to the user's access token.
 * 
 * The union hack is used to hardcode the SID, avoiding some calls into functions
 * to parse one from a string. This will not work if Windows ever decides to work
 * on big-endian CPUs, I suppose.
 * 
 * https://github.com/MicrosoftDocs/windowsserverdocs/blob/main/WindowsServerDocs/identity/ad-ds/manage/understand-security-identifiers.md
 * 
 * S-1-5-12
 */
union
{
	BYTE bytes[];
	SID sid;
} SID_RESTRICTED_CODE = { 0x1, 0x1, 0, 0, 0, 0, 0, 0x5, 0x12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

CShellProtectedRegLock::~CShellProtectedRegLock()
{
	if (m_hKey)
		RegCloseKey(m_hKey);
	LocalFree(m_pTokenUser);
	LocalFree(m_pSecurityDescriptor);
}

HRESULT CShellProtectedRegLock::QueryUserToken(HKEY hKey, LPCWSTR lpwszValue)
{
	constexpr int TOKEN_INFO_BUFFER_SIZE = 2048;
	PSID pSid = nullptr;
	HRESULT hr = OpenEffectiveToken(&pSid);
	DWORD dwLastError = 0;

	m_pTokenUser = (PTOKEN_USER)LocalAlloc(LPTR, TOKEN_INFO_BUFFER_SIZE);

	if (m_pTokenUser)
	{
		//LPVOID pvTokenInfo = nullptr;
		DWORD dwTokenLength = TOKEN_INFO_BUFFER_SIZE;
		if (!GetTokenInformation(pSid, TokenUser, m_pTokenUser, dwTokenLength, &dwTokenLength))
		{
			dwLastError = GetLastError();

			if (dwLastError == ERROR_INSUFFICIENT_BUFFER)
			{
				LocalFree(m_pTokenUser);

				m_pTokenUser = (PTOKEN_USER)LocalAlloc(LPTR, dwTokenLength);

				if (m_pTokenUser)
				{
					if (GetTokenInformation(pSid, TokenUser, m_pTokenUser, dwTokenLength, &dwTokenLength))
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
	else // !m_pTokenUser
	{
		hr = E_FAIL;
	}

	if (FAILED(hr))
	{
		LocalFree(m_pTokenUser);
		m_pTokenUser = nullptr;
	}

	if (pSid)
	{
		CloseHandle(pSid);
	}

	return hr;
}

// static
HRESULT CShellProtectedRegLock::OpenEffectiveToken(OUT PSID *ppSid)
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
			&m_hKey,
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
		m_hKey &&
		!EqualSid(&SID_RESTRICTED_CODE, m_pTokenUser->User.Sid) &&
		GetAclInformation(m_pAcl, &sizeInfo, sizeof(sizeInfo), AclSizeInformation)
	)
	{
		DWORD dwBytesInUse = sizeInfo.AclBytesInUse;
		DWORD dwRevision = 2;

		ACL_REVISION_INFORMATION revisionInfo;

		if (GetAclInformation(m_pAcl, &revisionInfo, sizeof(revisionInfo), AclRevisionInformation))
		{
			dwRevision = revisionInfo.AclRevision;
		}

		DWORD dwSidLen = GetLengthSid(m_pTokenUser->User.Sid);
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
			memcpy(&pNewAce->SidStart, m_pTokenUser->User.Sid, dwSidLen);

			if (AddAce(pNewAcl, dwRevision, MAXDWORD, pNewAce, dwAceListLength))
			{
				bool bGiveUp = false;

				if (sizeInfo.AceCount)
				{
					for (int i = 0; i < sizeInfo.AceCount; i++)
					{
						PACL pCurrentAcl;
						if (GetAce(m_pAcl, i, (LPVOID *)&pCurrentAcl))
						{
							if (!AddAce(pNewAcl, dwRevision, MAXDWORD, pCurrentAcl, pCurrentAcl->AclSize))
							{
								bGiveUp = true;
							}
						}
					}
				}

				if (!bGiveUp)
				{
					SetSecurityInfo(
						m_hKey,
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
			m_hKey,
			SE_REGISTRY_KEY,
			DACL_SECURITY_INFORMATION,
			nullptr,
			nullptr,
			&m_pAcl,
			nullptr,
			&m_pSecurityDescriptor
		) == ERROR_SUCCESS
	)
	{
		ACL_SIZE_INFORMATION sizeInfo;
		PACCESS_DENIED_ACE pAce;

		if (m_pAcl)
		{
			if (GetAclInformation(m_pAcl, &sizeInfo, sizeof(sizeInfo), AclSizeInformation))
			{
				for (int i = sizeInfo.AceCount - 1; i >= 0; --i)
				{
					if (
						GetAce(m_pAcl, i, (LPVOID *)&pAce) &&
						pAce->Header.AceType == ACCESS_DENIED_ACE_TYPE &&
						pAce->Mask == KEY_SET_VALUE
					)
					{
						if (DeleteAce(m_pAcl, i))
						{
							SetSecurityInfo(
								m_hKey,
								SE_REGISTRY_KEY,
								DACL_SECURITY_INFORMATION,
								nullptr,
								nullptr,
								m_pAcl,
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
		bool bShouldLock = true;
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
				bShouldLock = false;
			}

			RegCloseKey(hKey);
		}

		if (bShouldLock)
			lock.Lock();
	}

	return ls;
}
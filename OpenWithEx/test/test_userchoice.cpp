#include "test_userchoice.h"

#include "../assocuserchoice.h"
#include "../raiihelpers.h"

/**
 * Generate a UserChoice hash; compare it with the one that is stored.
 *
 * NOTE: The passed-in current user SID is used here, instead of getting the SID
 * for the owner of the key. We are assuming that this key is HKCU is owned by
 * the current user, since we want to replace that key ourselves. If the key is
 * owned by someone else, then this check will fail; this is okay because we
 * would likely not want to replace that other user's key anyway.
 *
 * @param lpszExtension  File extension or protocol association to check
 * @param lpszUserSid    String SID of the current user
 *
 * @return Result of the check, see CheckUserChoiceHashResult
 */
CheckUserChoiceHashResult CheckUserChoiceHash(
	LPCWSTR lpszExtension,
	LPCWSTR lpszUserSid
)
{
	std::unique_ptr<WCHAR[]> keyPath = GetAssociationKeyPath(lpszExtension);

	if (!keyPath)
	{
		return CheckUserChoiceHashResult::ERR_OTHER;
	}

	HKEY hKeyAssoc;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, keyPath.get(), 0, KEY_READ, &hKeyAssoc) != ERROR_SUCCESS)
	{
		return CheckUserChoiceHashResult::ERR_OTHER;
	}

	// Auto-close key after return:
	std::unique_ptr<HKEY__, RegCloseKeyDeleter> assocKeyDeletionManager(hKeyAssoc);

	FILETIME lastWriteFileTime;
	{
		// Get the last-write file time from the UserChoice root.

		HKEY hKeyUserChoice;
		if (RegOpenKeyExW(hKeyAssoc, L"UserChoice", 0, KEY_READ, &hKeyUserChoice) != ERROR_SUCCESS)
		{
			return CheckUserChoiceHashResult::ERR_OTHER;
		}

		// Auto-close key after return:
		std::unique_ptr<HKEY__, RegCloseKeyDeleter> userChoiceKeyDeletionManager(hKeyUserChoice);

		if (
			RegQueryInfoKeyW(
				hKeyUserChoice,
				nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
				&lastWriteFileTime
			) != ERROR_SUCCESS)
		{
			return CheckUserChoiceHashResult::ERR_OTHER;
		}
	}

	SYSTEMTIME lastWriteSystemTime;
	if (!FileTimeToSystemTime(&lastWriteFileTime, &lastWriteSystemTime))
	{
		return CheckUserChoiceHashResult::ERR_OTHER;
	}

	// ************
	// Read ProgId:
	// ************
	DWORD dwDataSizeBytes = 0;
	if (
		RegGetValueW(
			hKeyAssoc,
			L"UserChoice",
			L"ProgId",
			RRF_RT_REG_SZ,
			nullptr,
			nullptr,
			&dwDataSizeBytes
		) != ERROR_SUCCESS
		)
	{
		return CheckUserChoiceHashResult::ERR_OTHER;
	}

	// +1 in case dataSizeBytes was odd, +1 to ensure termination:
	DWORD dwDataSizeChars = (dwDataSizeBytes / sizeof(WCHAR)) + 2;

	std::unique_ptr<WCHAR[]> pszProgId = std::make_unique<WCHAR[]>(dwDataSizeChars);
	if (
		RegGetValueW(
			hKeyAssoc,
			L"UserChoice",
			L"ProgId",
			RRF_RT_REG_SZ,
			nullptr,
			pszProgId.get(),
			&dwDataSizeBytes
		) != ERROR_SUCCESS
		)
	{
		return CheckUserChoiceHashResult::ERR_OTHER;
	}

	// **********
	// Read Hash:
	// **********
	dwDataSizeBytes = 0;
	if (
		RegGetValueW(
			hKeyAssoc,
			L"UserChoice",
			L"Hash",
			RRF_RT_REG_SZ,
			nullptr,
			nullptr,
			&dwDataSizeBytes
		) != ERROR_SUCCESS
		)
	{
		return CheckUserChoiceHashResult::ERR_OTHER;
	}

	dwDataSizeChars = (dwDataSizeBytes / sizeof(WCHAR)) + 2;

	std::unique_ptr<WCHAR[]> pszStoredHash = std::make_unique<WCHAR[]>(dwDataSizeChars);
	if (
		RegGetValueW(
			hKeyAssoc,
			L"UserChoice",
			L"Hash",
			RRF_RT_REG_SZ,
			nullptr,
			pszStoredHash.get(),
			&dwDataSizeBytes
		) != ERROR_SUCCESS
		)
	{
		return CheckUserChoiceHashResult::ERR_OTHER;
	}

	std::unique_ptr<WCHAR[]> pszComputedHash = GenerateUserChoiceHash(
		lpszExtension,
		lpszUserSid,
		pszProgId.get(),
		&lastWriteSystemTime
	);

	if (!pszComputedHash)
	{
		return CheckUserChoiceHashResult::ERR_OTHER;
	}

	OutputDebugStringW(L"Theirs: ");
	OutputDebugStringW(pszStoredHash.get());
	OutputDebugStringW(L"\n");
	OutputDebugStringW(L"  Ours: ");
	OutputDebugStringW(pszComputedHash.get());
	OutputDebugStringW(L"\n\n");

	if (CompareStringOrdinal(pszComputedHash.get(), -1, pszStoredHash.get(), -1, FALSE) != CSTR_EQUAL)
	{
		return CheckUserChoiceHashResult::ERR_MISMATCH;
	}

	return CheckUserChoiceHashResult::OK_V1;
}
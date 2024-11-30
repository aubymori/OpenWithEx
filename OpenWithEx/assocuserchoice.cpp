/**
 * Manages setting file type associations on Windows 10.
 * 
 * On Windows 10, file type associations are protected by a "UserChoice" hash,
 * a value which is protected by the system. However, this algorithm has since
 * been reverse engineered, and even implemented in popular software such as
 * the Firefox web browser.
 * 
 * This code is copied from Firefox's implementation here:
 * https://github.com/mozilla/gecko-dev/blob/master/browser/components/shell/WindowsUserChoice.cpp
 * https://github.com/mozilla/gecko-dev/blob/93692d0756f01f99e2b028e40b45776fa0a397e9/toolkit/mozapps/defaultagent/SetDefaultBrowser.cpp#L345
 * 
 * Additionally-referenced code:
 * https://github.com/DanysysTeam/PS-SFTA/blob/master/SFTA.ps1
 * 
 * Microsoft Patent regarding the User Choice system:
 * https://patents.google.com/patent/US20130198646A1/en
 */

#include <windows.h>
#include <sddl.h> // for ConvertSidToStringSidW
#include <wincrypt.h> // for CryptoAPI base64
#include <bcrypt.h> // CNG MD5
#include <winternl.h> // for NT_SUCCESS()
#include <shlobj.h> // for SHChangeNotify
#include <rpc.h> // for UuidCreate
#include "versionhelper.h" // for CVersionHelper
#include "shellprotectedreglock.h" // for SH***ProtectedValue APIs

#include <memory>

#include "assocuserchoice.h"

#include "wil/com.h"
#include "wil/registry.h"
#include "wil/resource.h"

/**
 * A constant copy of the User Experience string.
 * 
 * This string is stored internally in shell32.dll and is used in generating
 * UserChoice hashes.
 * 
 * In the future, this should be restructured to be pulled from shell32.dll
 * during runtime instead of being hardcoded.
 */
LPCWSTR g_szConstantUserExperience =
	L"User Choice set via Windows User Experience "
	L"{D18B6DD5-6124-4341-9318-804003BAFA0B}";

#pragma region Private
#pragma region Private: Timing functions

/**
 * Takes two times: the start time and the current time, and returns the number
 * of seconds left before the current time hits the next minute from the start
 * time. Used to check if we are within the same minute as the start time and
 * how much time we have left to perform an operation in the same minute.
 * 
 * User Choice hashes must be written to the registry in the same minute in
 * which they are generated.
 * 
 * @param pOperationStartTime  The start time of the operation.
 * @param pCurrentTime         The current time.
 * 
 * @return The number of milliseconds left from the current time to the next
 *         minute from pOperationStartTime, or zero if pCurrentTime is
 *         already at the next minute or greater.
 */
static WORD GetMillisecondsToNextMinute(
	SYSTEMTIME *pOperationStartTime,
	SYSTEMTIME *pCurrentTime
)
{
	SYSTEMTIME operationStartTimeMinute = *pOperationStartTime;
	SYSTEMTIME currentTimeMinute = *pCurrentTime;

	// Zero out the seconds and milliseconds so that we can confirm that they are
	// the same minutes:
	operationStartTimeMinute.wSecond = 0;
	operationStartTimeMinute.wMilliseconds = 0;

	currentTimeMinute.wSecond = 0;
	currentTimeMinute.wMilliseconds = 0;

	// Convert to a 64-bit value so that we can compare them directly:
	FILETIME fileTimeOperationStart;
	FILETIME fileTimeCurrentTime;

	if (
		!SystemTimeToFileTime(&operationStartTimeMinute, &fileTimeOperationStart) ||
		!SystemTimeToFileTime(&currentTimeMinute, &fileTimeCurrentTime)
	)
	{
		// Error: Report that there are 0 milliseconds until the next minute.
		return 0;
	}

	// The minutes for both times have to be the same, so we confirm that they are,
	// and if they aren't, return 0 milliseconds to indicate that we're already not
	// on the same minute that the operation start time was on:
	if (
		(fileTimeOperationStart.dwLowDateTime != fileTimeCurrentTime.dwLowDateTime) ||
		(fileTimeOperationStart.dwHighDateTime != fileTimeCurrentTime.dwHighDateTime)
	)
	{
		return 0;
	}

	// The minutes are the same; determine the number of milliseconds left until
	// the next minute:
	constexpr WORD SECS_TO_MS = 1000;
	constexpr WORD MINS_TO_SECS = 60;

	// 1 minute converted to milliseconds - (the current second converted to
	// milliseconds + the current milliseconds):
	return (1 * MINS_TO_SECS * SECS_TO_MS) -
		(
			(pCurrentTime->wSecond * SECS_TO_MS) +
			pCurrentTime->wMilliseconds
		);
}

/**
 * Compare two SYSTEMTIMEs as FILETIME after clearing everything
 * below minutes.
 */
static bool CheckEqualMinutes(SYSTEMTIME *pSysTime1, SYSTEMTIME *pSysTime2)
{
	SYSTEMTIME sysTime1 = *pSysTime1;
	SYSTEMTIME sysTime2 = *pSysTime2;

	sysTime1.wSecond = 0;
	sysTime1.wMilliseconds = 0;

	sysTime2.wSecond = 0;
	sysTime2.wMilliseconds = 0;

	FILETIME fileTime1;
	FILETIME fileTime2;

	if (
		!SystemTimeToFileTime(&sysTime1, &fileTime1) ||
		!SystemTimeToFileTime(&sysTime2, &fileTime2)
	)
	{
		return false;
	}

	return (fileTime1.dwLowDateTime == fileTime2.dwLowDateTime) &&
		   (fileTime1.dwHighDateTime == fileTime2.dwHighDateTime);
}

static bool AddMillisecondsToSystemTime(SYSTEMTIME *pSystemTime, ULONGLONG ulIncrementMs)
{
	FILETIME fileTime;
	ULARGE_INTEGER fileTimeInt;

	if (!SystemTimeToFileTime(pSystemTime, &fileTime))
	{
		return false;
	}

	fileTimeInt.LowPart = fileTime.dwLowDateTime;
	fileTimeInt.HighPart = fileTime.dwHighDateTime;

	// FILETIME is in units of 100ns.
	fileTimeInt.QuadPart += ulIncrementMs * 1000 * 10;

	fileTime.dwLowDateTime = fileTimeInt.LowPart;
	fileTime.dwHighDateTime = fileTimeInt.HighPart;

	SYSTEMTIME tmpSystemTime;
	if (!FileTimeToSystemTime(&fileTime, &tmpSystemTime))
	{
		return false;
	}

	*pSystemTime = tmpSystemTime;
	return true;
}

#pragma endregion

#pragma region Private: Hash functions
/**
 * Create the string which becomes the input to the UserChoice hash.
 *
 * NOTE: This uses the format as of Windows 10 20H2 (latest as of Mozilla
 * writing the original code), used since at least 1803.
 *
 * There was at least one older version, not currently supported: on
 * Windows 10 RTM (build 10240, aka 1507), the hash function is the same, but
 * the timestamp and User Experience strings aren't included; instead (for
 * protocols), the string ends with the exe path. The changelog of SetUserFTA
 * suggests the algorithm changed in 1703, so there may be two versions:
 * before 1703, and 1703 to now.
 *
 * @see GenerateUserChoiceHash() for parameters.
 *
 * @return A string pointer to the formatted string, or nullptr on failure.
 */
static std::unique_ptr<WCHAR[]> FormatUserChoiceString(
	LPCWSTR lpszExtension,
	LPCWSTR lpszUserSid,
	LPCWSTR lpszProgId,
	SYSTEMTIME *pTimestamp
)
{
	SYSTEMTIME timestamp = *pTimestamp;

	timestamp.wSecond = 0;
	timestamp.wMilliseconds = 0;

	FILETIME fileTime = { 0 };
	if (!SystemTimeToFileTime(&timestamp, &fileTime))
	{
		return nullptr;
	}

	// This string is built into Windows as part of the UserChoice hash algorithm.
	// It might vary across Windows SKUs (e.g. Windows 10 vs Windows Server), or
	// across builds of the same SKU, but this is the only currently known
	// version. There isn't any known way of deriving it, so we assume this
	// constant value. If we are wrong, we will not be able to generate correct
	// UserChoice hashes.
	LPCWSTR szUserExperience = g_szConstantUserExperience;

	LPCWSTR userChoiceFormat =
		L"%s%s%s"
		L"%08lx"
		L"%08lx"
		L"%s";

	int cchUserChoice = _scwprintf(
		userChoiceFormat,
		lpszExtension,
		lpszUserSid,
		lpszProgId,
		fileTime.dwHighDateTime,
		fileTime.dwLowDateTime,
		szUserExperience
	);
	cchUserChoice += 1; // _scwprintf does not include the terminator

	std::unique_ptr<WCHAR[]> pszUserChoice = std::make_unique<WCHAR[]>(cchUserChoice);

	_snwprintf_s(
		pszUserChoice.get(),
		cchUserChoice,
		_TRUNCATE,
		userChoiceFormat,
		lpszExtension,
		lpszUserSid,
		lpszProgId,
		fileTime.dwHighDateTime,
		fileTime.dwLowDateTime,
		szUserExperience
	);

	CharLowerW(pszUserChoice.get());

	return pszUserChoice;
}

/**
 * Hashes the data with the MD5 hashing algorithm.
 *
 * @return A string pointer to the MD5 hash of the input, nullptr on failure.
 */
static std::unique_ptr<DWORD[]> CNG_MD5(const BYTE *pbData, ULONG bytesLen)
{
	constexpr ULONG MD5_BYTES = 16;
	constexpr ULONG MD5_DWORDS = MD5_BYTES / sizeof(DWORD);

	std::unique_ptr<DWORD[]> hash;

	BCRYPT_ALG_HANDLE hAlg = nullptr;
	if (NT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_MD5_ALGORITHM, nullptr, 0)))
	{
		BCRYPT_HASH_HANDLE hHash = nullptr;

		// As of Windows 7, the hash handle will manage its own object buffer when
		// pbHashObject is nullptr and cbHashObject is 0.
		if (NT_SUCCESS(BCryptCreateHash(hAlg, &hHash, nullptr, 0, nullptr, 0, 0)))
		{
			// BCryptHashData promises not to modify pbInput.
			if (NT_SUCCESS(BCryptHashData(hHash, (LPBYTE)pbData, bytesLen, 0)))
			{
				hash = std::make_unique<DWORD[]>(MD5_DWORDS);

				if (!NT_SUCCESS(BCryptFinishHash(hHash, (LPBYTE)hash.get(), MD5_DWORDS * sizeof(DWORD), 0)))
				{
					hash.reset();
				}
			}

			BCryptDestroyHash(hHash);
		}

		BCryptCloseAlgorithmProvider(hAlg, 0);
	}

	return hash;
}

/**
 * Base64 encodes a string.
 *
 * @param pBytes      A pointer to an array of pbData to encode in base64
 * @param dwBytesLen  The length of the pbData array
 *
 * @return A pointer to the input byte array encoded as a base64 string,
 *         nullptr on failure.
 */
static std::unique_ptr<WCHAR[]> CryptoAPI_Base64Encode(const BYTE *pBytes, DWORD dwBytesLen)
{
	DWORD base64Len = 0;

	if (!CryptBinaryToStringW(
		pBytes,
		dwBytesLen,
		CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
		nullptr,
		&base64Len)
		)
	{
		return nullptr;
	}

	std::unique_ptr<WCHAR[]> base64 = std::make_unique<WCHAR[]>(base64Len);

	if (!CryptBinaryToStringW(
		pBytes,
		dwBytesLen,
		CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
		base64.get(),
		&base64Len)
		)
	{
		return nullptr;
	}

	return base64;
}

static inline DWORD WordSwap(DWORD v) { return (v >> 16) | (v << 16); }

/**
 * Generate the UserChoice hash.
 *
 * This implementation is based on the references listed in Mozilla's
 * implementation linked above.
 *
 * @param lpszInputString  A null-terminated string to hash.
 *
 * @return A string pointer to the base64-encoded hash, or nullptr on failure.
 */
static std::unique_ptr<WCHAR[]> HashString(LPCWSTR lpszInputString)
{
	LPCBYTE inputBytes = (LPCBYTE)lpszInputString;
	int inputByteCount = (lstrlenW(lpszInputString) + 1) * sizeof(WCHAR);

	constexpr size_t DWORDS_PER_BLOCK = 2;
	constexpr size_t BLOCK_SIZE = sizeof(DWORD) * DWORDS_PER_BLOCK;

	// Incomplete blocks are ignored.
	int blockCount = inputByteCount / BLOCK_SIZE;

	if (blockCount == 0)
	{
		return nullptr;
	}

	// Compute an MD5 hash. md5[0] and md5[1] will be used as constant
	// multipliers in the scramble below.
	std::unique_ptr<DWORD[]> md5 = CNG_MD5(inputBytes, inputByteCount);
	if (!md5)
	{
		return nullptr;
	}

	// The following loop effectively computes two checksums, scrambled like
	// a hash after every DWORD is added.

	// Constant multipliers for the scramble, one set for each DWORD in a block:
	const DWORD C0s[DWORDS_PER_BLOCK][5] = {
		{md5[0] | 1, 0xCF98B111uL, 0x87085B9FuL, 0x12CEB96DuL, 0x257E1D83uL},
		{md5[1] | 1, 0xA27416F5uL, 0xD38396FFuL, 0x7C932B89uL, 0xBFA49F69uL}
	};

	const DWORD C1s[DWORDS_PER_BLOCK][5] = {
		{md5[0] | 1, 0xEF0569FBuL, 0x689B6B9FuL, 0x79F8A395uL, 0xC3EFEA97uL},
		{md5[1] | 1, 0xC31713DBuL, 0xDDCD1F0FuL, 0x59C3AF2DuL, 0x35BD1EC9uL}
	};

	// The checksums:
	DWORD h0 = 0;
	DWORD h1 = 0;

	// Accumulated total of the checksum after each DWORD:
	DWORD h0Acc = 0;
	DWORD h1Acc = 0;

	for (int i = 0; i < blockCount; ++i)
	{
		for (size_t j = 0; j < DWORDS_PER_BLOCK; ++j)
		{
			const DWORD *C0 = C0s[j];
			const DWORD *C1 = C1s[j];

			DWORD input;
			memcpy(&input, &inputBytes[(i * DWORDS_PER_BLOCK + j) * sizeof(DWORD)], sizeof(DWORD));

			h0 += input;
			// Scramble 0:
			h0 *= C0[0];
			h0 = WordSwap(h0) * C0[1];
			h0 = WordSwap(h0) * C0[2];
			h0 = WordSwap(h0) * C0[3];
			h0 = WordSwap(h0) * C0[4];
			h0Acc += h0;

			h1 += input;
			// Scramble 1:
			h1 = WordSwap(h1) * C1[1] + h1 * C1[0];
			h1 = (h1 >> 16) * C1[2] + h1 * C1[3];
			h1 = WordSwap(h1) * C1[4] + h1;
			h1Acc += h1;
		}
	}

	DWORD hash[2] = { h0 ^ h1, h0Acc ^ h1Acc };

	return CryptoAPI_Base64Encode((LPCBYTE)hash, sizeof(hash));
}
#pragma endregion

#pragma region Private: Writing to registry
static std::unique_ptr<WCHAR[]> GetRandomUUID()
{
	LPCWSTR UUID_FORMAT = L"{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}";

	UUID uuid;
	UuidCreate(&uuid);

	int cchUuid = _scwprintf(
		UUID_FORMAT,
		uuid.Data1,
		uuid.Data2,
		uuid.Data3,
		uuid.Data4[0],
		uuid.Data4[1],
		uuid.Data4[2],
		uuid.Data4[3],
		uuid.Data4[4],
		uuid.Data4[5],
		uuid.Data4[6],
		uuid.Data4[7]
	);
	cchUuid += 1; // _scwprintf does not include the terminator

	std::unique_ptr<WCHAR[]> pszUuid = std::make_unique<WCHAR[]>(cchUuid);
	_snwprintf_s(
		pszUuid.get(),
		cchUuid,
		_TRUNCATE,
		UUID_FORMAT,
		uuid.Data1,
		uuid.Data2,
		uuid.Data3,
		uuid.Data4[0],
		uuid.Data4[1],
		uuid.Data4[2],
		uuid.Data4[3],
		uuid.Data4[4],
		uuid.Data4[5],
		uuid.Data4[6],
		uuid.Data4[7]
	);

	return pszUuid;
}

/**
 * Update the User Choice association in the registry with new data.
 *
 * @param lpszExtension  File extension or protocol being registered.
 * @param lpszProgId     ProgID to associate with the extension.
 * @param pszHash        The generated UserChoice hash.
 */
static HRESULT WriteToRegistry(
	LPCWSTR lpszExtension,
	LPCWSTR lpszProgId,
	std::unique_ptr<WCHAR[]> &pszHash
)
{
	std::unique_ptr<WCHAR[]> pszAssocKeyPath = GetAssociationKeyPath(lpszExtension);

	wil::unique_hkey hKeyAssoc = nullptr;

	LSTATUS ls;
	ls = RegCreateKeyExW(
		HKEY_CURRENT_USER,
		pszAssocKeyPath.get(),
		0,
		nullptr,
		0,
		KEY_READ | KEY_WRITE,
		0,
		&hKeyAssoc,
		nullptr
	);

	if (ls != ERROR_SUCCESS)
	{
		return HRESULT_FROM_WIN32(ls);
	}

	// Windows file association keys are read-only (Deny Set Value) for the
	// user, meaning that they can not be modified, but can be deleted and
	// recreated. We don't set any similar special permissions.
	// NOTE: This only applies to file extensions, not URL protocols.
	if (lpszExtension && lpszExtension[0] == '.')
	{
		ls = SHDeleteProtectedValue(hKeyAssoc.get(), NULL, L"UserChoice", true);
	}

	// According to Mozilla, some keys may be protected from modification by
	// certain kernel drivers; renaming the keys to a random UUID is sufficient
	// to bypass this.
	// https://github.com/mozilla/gecko-dev/blob/master/toolkit/mozapps/defaultagent/SetDefaultBrowser.cpp#L186-L191
	std::unique_ptr<WCHAR[]> pszTempName = GetRandomUUID();
	ls = RegRenameKey(hKeyAssoc.get(), nullptr, pszTempName.get());

	if (ls != ERROR_SUCCESS)
	{
		return HRESULT_FROM_WIN32(ls);
	}

	DWORD progIdByteCount = (lstrlenW(lpszProgId) + 1) * sizeof(WCHAR);
	ls = SHSetProtectedValue(
		hKeyAssoc.get(),
		L"UserChoice",
		L"ProgId",
		false,
		lpszProgId,
		progIdByteCount
	);

	DWORD hashByteCount = (lstrlenW(pszHash.get()) + 1) * sizeof(WCHAR);
	ls = SHSetProtectedValue(
		hKeyAssoc.get(),
		L"UserChoice",
		L"Hash",
		false,
		pszHash.get(),
		hashByteCount
	);

	if (ls != ERROR_SUCCESS)
	{
		return HRESULT_FROM_WIN32(ls);
	}

	ls = RegRenameKey(hKeyAssoc.get(), nullptr, lpszExtension);

	if (ls != ERROR_SUCCESS)
	{
		return HRESULT_FROM_WIN32(ls);
	}

	return S_OK;
}
#pragma endregion
#pragma endregion

/**
 * Get the current user's SID.
 * 
 * @return Pointer to string SID for the user of the current process,
 *         nullptr on failure.
 */
std::unique_ptr<WCHAR[]> GetCurrentUserStringSid()
{
	wil::unique_handle processToken;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &processToken))
	{
		return nullptr;
	}

	// Get the buffer size into dwUserSize so we know how much to allocate:
	DWORD dwUserSize = 0;
	if (
		GetTokenInformation(processToken.get(), TokenUser, nullptr, 0, &dwUserSize) &&
		GetLastError() != ERROR_INSUFFICIENT_BUFFER
	)
	{
		// Return a null pointer if the call to GetTokenInformation fails for any
		// reason other than the buffer being insufficiently sized.
		return nullptr;
	}

	std::unique_ptr<BYTE[]> userBytes = std::make_unique<BYTE[]>(dwUserSize);
	if (!GetTokenInformation(processToken.get(), TokenUser, userBytes.get(), dwUserSize, &dwUserSize))
	{
		return nullptr;
	}

	wil::unique_hlocal_string rawSid = nullptr;
	if (!ConvertSidToStringSidW( ((PTOKEN_USER)userBytes.get())->User.Sid, &rawSid ))
	{
		return nullptr;
	}

	int cchSid = lstrlenW(rawSid.get()) + 1;
	std::unique_ptr<WCHAR[]> sid = std::make_unique<WCHAR[]>(cchSid);
	memcpy(sid.get(), rawSid.get(), cchSid * sizeof(WCHAR));

	return sid;
}

/**
 * Get the registry path for the given association, file extension or protocol.
 * 
 * @param lpszExtension   File extension or protocol being retrieved.
 * @param fIsUri          Whether lpszExtension should act as a file extension
 *                        or URI protocol.
 * 
 * @return A string pointer to the path, or nullptr on failure.
 */
std::unique_ptr<WCHAR[]> GetAssociationKeyPath(LPCWSTR lpszExtension, bool fIsUri)
{
	LPCWSTR keyPathFmt;
	if (fIsUri)
	{
		keyPathFmt = L"SOFTWARE\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\%s";
	}
	else
	{
		keyPathFmt = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\%s";
	}

	int cchKeyPath = _scwprintf(keyPathFmt, lpszExtension);
	cchKeyPath += 1; // _scwprintf does not include the terminator

	std::unique_ptr<WCHAR[]> lpszKeyPath = std::make_unique<WCHAR[]>(cchKeyPath);
	_snwprintf_s(
		lpszKeyPath.get(),
		cchKeyPath,
		_TRUNCATE,
		keyPathFmt,
		lpszExtension
	);

	return lpszKeyPath;
}

/**
 * Generate the UserChoice hash.
 * 
 * @param lpszExtension  File extension or protocol being registered
 * @param lpszUserSid    String SID of the current user
 * @param lpszProgId     ProgID to associate with the extension.
 * @param pTimestamp     Approximate write time of the UserChoice key
 *                       (within the same minute)
 * 
 * @return Pointer to UserChoice hash string, nullptr on failure
 */
std::unique_ptr<WCHAR[]> GenerateUserChoiceHash(
	LPCWSTR lpszExtension,
	LPCWSTR lpszUserSid,
	LPCWSTR lpszProgId,
	SYSTEMTIME *pTimestamp
)
{
	std::unique_ptr<WCHAR[]> userChoice = FormatUserChoiceString(
		lpszExtension,
		lpszUserSid,
		lpszProgId,
		pTimestamp
	);

	if (!userChoice)
	{
		return nullptr;
	}

	return HashString(userChoice.get());
}

/**
 * Check that the given ProgID exists in HKCR.
 * 
 * @return true if it could be opened for reading, false otherwise
 */
bool CheckProgIdExists(LPCWSTR lpszProgId)
{
	HKEY hKey;

	if (RegOpenKeyExW(HKEY_CLASSES_ROOT, lpszProgId, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
	{
		return false;
	}

	RegCloseKey(hKey);
	return true;
}

/**
 * Sets the UserChoice association to a ProgID for a given extension or protocol.
 * 
 * @param lpszExtension  The extension or protocol for which to make the
 *                       association.
 * @param lpszProgId     The ProgID with which to be associated.
 */
SetUserChoiceAndHashResult SetUserChoiceAndHash(LPCWSTR lpszExtension, LPCWSTR lpszProgId)
{
	if (!CVersionHelper::IsWindows10_1703OrGreater())
	{
		return SetUserChoiceAndHashResult::UNSUPPORTED_OS;
	}

	std::unique_ptr<WCHAR[]> pszUserSid = GetCurrentUserStringSid();

	// The hash changes at the end of each minute, so check that the hash should
	// be the same by the time we're done writing.
	constexpr ULONGLONG WRITING_TIMING_THRESHOLD_MS = 1000;

	if (!pszUserSid)
	{
		return SetUserChoiceAndHashResult::FAIL;
	}

	// Because the hash can change before we write it, we run this code in a loop
	// to ensure that the hash has the chance to be written to disk before it
	// expires; if it fails, the loop is simply restarted:
	SYSTEMTIME hashTimestamp;
	SYSTEMTIME writeEndTimestamp;
	DWORD dwRecheckTimes = 0;
	std::unique_ptr<WCHAR[]> pszHash;
	do
	{
		GetSystemTime(&hashTimestamp);

		// Generate the User Choice hash:
		pszHash = GenerateUserChoiceHash(
			lpszExtension,
			pszUserSid.get(),
			lpszProgId,
			&hashTimestamp
		);

		if (!pszHash)
		{
			return SetUserChoiceAndHashResult::FAIL;
		}

		GetSystemTime(&writeEndTimestamp);

		if (!AddMillisecondsToSystemTime(&writeEndTimestamp, WRITING_TIMING_THRESHOLD_MS))
		{
			// If, for whatever reason, this function fails, then we simply have to fail.
			return SetUserChoiceAndHashResult::FAIL;
		}

		if (dwRecheckTimes > 2)
		{
			// Only three checks are allowed (first go + 2 rechecks).
			return SetUserChoiceAndHashResult::FAIL;
		}

		dwRecheckTimes++;
	}
	while (!CheckEqualMinutes(&hashTimestamp, &writeEndTimestamp));

	if (
		FAILED(WriteToRegistry(
			lpszExtension,
			lpszProgId,
			pszHash
		))
	)
	{
		return SetUserChoiceAndHashResult::FAIL;
	}

	// Notify shell to refresh icons:
	SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);

	return SetUserChoiceAndHashResult::OK;
}
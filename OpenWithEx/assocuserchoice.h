#pragma once

#include <memory>
#include <windows.h>

/**
 * Result from SetUserChoiceAndHash. 
 */
enum class SetUserChoiceAndHashResult
{
	// The operation succeeded.
	OK,

	// Generic failure.
	FAIL,
	
	// The operating system is unsupported.
	UNSUPPORTED_OS,
};

/**
 * Sets the UserChoice association to a ProgID for a given extension or protocol.
 *
 * @param lpszExtension  The extension or protocol for which to make the
 *                       association.
 * @param lpszProgId     The ProgID with which to be associated.
 */
SetUserChoiceAndHashResult SetUserChoiceAndHash(LPCWSTR lpszExtension, LPCWSTR lpszProgId);

/**
 * Get the current user's SID.
 *
 * @return Pointer to string SID for the user of the current process,
 *         nullptr on failure.
 */
std::unique_ptr<WCHAR[]> GetCurrentUserStringSid();

/**
 * Get the registry path for the given association, file extension or protocol.
 *
 * @param lpszExtension   File extension or protocol being retrieved.
 * @param fIsUri          Whether lpszExtension should act as a file extension
 *                        or URI protocol.
 *
 * @return A string pointer to the path, or nullptr on failure.
 */
std::unique_ptr<WCHAR[]> GetAssociationKeyPath(LPCWSTR lpszExtension, bool fIsUri = false);

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
	PSYSTEMTIME pTimestamp
);

/**
 * Check that the given ProgID exists in HKCR.
 *
 * @return true if it could be opened for reading, false otherwise
 */
bool CheckProgIdExists(LPCWSTR lpszProgId);

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
);
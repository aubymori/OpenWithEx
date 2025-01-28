#pragma once

#include <windows.h>
#include <memory>

/**
 * Result from CheckUserChoiceHash.
 *
 * NOTE: Currently the only positive result is OK_V1, but the enum
 * could be extended to indicate different versions of the hash.
 */
enum class CheckUserChoiceHashResult
{
	// Matched the current version of the hash (as of Win10 20H2).
	OK_V1,

	// The hash did not match.
	ERR_MISMATCH,

	// Error reading or generating the hash.
	ERR_OTHER,
};
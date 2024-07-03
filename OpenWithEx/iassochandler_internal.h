#pragma once

/*
 * Private undocumented interfaces from shell32/Windows.Storage.dll:
 * 
 * IAssocHandlerInfo
 * {D5C0CDAC-7A15-4F0A-87BA-2E7AAF19E0EC}
 */

#include <windows.h>

/**
 * Format for IAssocHandlerInfo::GetInternalProgID.
 * 
 * These are names I came up with myself, since we don't have the original
 * enum definition at all. We do know the enum name itself from the symbols.
 */
enum class ASSOC_PROGID_FORMAT : int
{
	USE_GENERATED_PROGID_IF_NECESSARY = 0,
	ALWAYS_USE_GENERATED_PROGID = 1,
};

/**
 * These are internal flags returned by IAssocHandlerInfo::GetFlags.
 * 
 * Names are taken from:
 * https://github.com/marlersoft/cwin32/blob/c0dbf4dbe4867bd197ea499f4770d4c10893448e/include/ui/shell.h#L3506-L3515
 */
enum class AHTYPE : int
{
	UNDEFINED = 0x0,
	USER_APPLICATION = 0x8,
	ANY_APPLICATION = 0x10,
	MACHINE_DEFAULT = 0x20,
	PROGID = 0x40,
	APPLICATION = 0x80,
	CLASS_APPLICATION = 0x100,
	ANY_PROGID = 0x200,
};

MIDL_INTERFACE("D5C0CDAC-7A15-4F0A-87BA-2E7AAF19E0EC")
IAssocHandlerInfo : public IUnknown
{
	/**
	 * Determine if the handler was recently installed.
	 * 
	 * This is used to highlight newly-installed applications in the GUI.
	 */
	virtual bool IsRecentlyInstalled() = 0;

	/**
	 * Get the internal ProgID of the handler.
	 * 
	 * This just wraps the IObjectWithProgID::GetProgID method, but will return
	 * a generated ProgID from the application path if no ProgID exists for the
	 * handler.
	 */
	virtual HRESULT GetInternalProgID(ASSOC_PROGID_FORMAT fmt, LPWSTR *ppszProgId) = 0;

	/**
	 * Gets the type of the association handler.
	 * 
	 * @see {@link AHTYPE}
	 */
	virtual HRESULT GetType(LPDWORD pdwOut) = 0;
};
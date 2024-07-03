#pragma once

/*
 * Private undocumented interface from shell32/Windows.Storage.dll:
 * 
 * IAssocHandlerInfo
 * {D5C0CDAC-7A15-4F0A-87BA-2E7AAF19E0EC}
 */

#include <windows.h>

/**
 * Format for IAssocHandler::GetInternalProgID.
 * 
 * These are names I came up with myself, since we don't have the original
 * enum definition at all. We do know the enum name itself from the symbols.
 */
enum class ASSOC_PROGID_FORMAT : int
{
	USE_GENERATED_PROGID_IF_NECESSARY = 0,
	ALWAYS_USE_GENERATED_PROGID = 1,
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
	virtual LRESULT GetInternalProgID(ASSOC_PROGID_FORMAT fmt, LPWSTR *ppszProgId) = 0;

	// This method was removed at some point, so the compiler will cluster it with
	// other no-op functions of the same prototype. As such, even its name is
	// unknown.
	virtual LRESULT placeholder0(void *) = 0;
};
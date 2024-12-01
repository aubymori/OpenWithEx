#pragma once

/*
 * Private undocumented interfaces from shell32/Windows.Storage.dll:
 * 
 * IAssocHandlerInfo
 * {D5C0CDAC-7A15-4F0A-87BA-2E7AAF19E0EC}
 * 
 * IAssocHandlerWithCompanyName
 * {E1B15A0F-2139-44F2-8C6C-3D2CA890F9D9}
 * 
 * IAssocHandlerPromptCount
 * {2AD6D87E-5A40-400C-A06F-E25378B17013}
 * 
 * IAssocHandlerMakeDefault
 * {571A5DB3-3B08-441F-B796-68E8164259BB}
 */

#include <windows.h>

/**
 * Format for IAssocHandlerInfo::GetInternalProgID.
 */
enum class ASSOC_PROGID_FORMAT : int
{
	APF_DEFAULT = 0,
	APF_APPLICATION = 1,
};

///**
// * These are internal flags returned by IAssocHandlerInfo::GetFlags.
// * 
// * Names are taken from:
// * https://github.com/marlersoft/cwin32/blob/c0dbf4dbe4867bd197ea499f4770d4c10893448e/include/ui/shell.h#L3506-L3515
// */
//enum class AHTYPE : int
//{
//	UNDEFINED = 0x0,
//	USER_APPLICATION = 0x8,
//	ANY_APPLICATION = 0x10,
//	MACHINE_DEFAULT = 0x20,
//	PROGID = 0x40,
//	APPLICATION = 0x80,
//	CLASS_APPLICATION = 0x100,
//	ANY_PROGID = 0x200,
//};

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
	virtual HRESULT GetHandlerType(AHTYPE *pAssociationTypeOut) = 0;
};

MIDL_INTERFACE("E1B15A0F-2139-44F2-8C6C-3D2CA890F9D9")
IAssocHandlerWithCompanyName : public IUnknown
{
	/**
	 * Gets the company name from the association handler.
	 * 
	 * NOTE: This is not used exclusively. A fallback whereby the property key
	 * PKEY_COMPANY is read from the shell item *is* used by shell32.dll where
	 * applicable. Notably, this access *seems* to come from an unrelated class
	 * function `CAppInfo::Init(CAppInfo *this)`. This isn't actually the case,
	 * since this function is referenced by OpenWith-related code. I don't want
	 * to spend too much time looking around the disassembly, but it at least
	 * gets some data from an IEnumAssocHandlers object (if not implements that
	 * class, but IDA is probably just confusing types).
	 * 
	 * The above usage is the only usage of this function by the entire OS, so
	 * assumedly TWinUI gets the company name in a different way.
	 */
	virtual HRESULT GetCompany(LPWSTR *ppszCompanyName) = 0;
};

MIDL_INTERFACE("2AD6D87E-5A40-400C-A06F-E25378B17013")
IAssocHandlerPromptCount : public IUnknown
{
	/*
	 * I think this is a notification prompt counter to make sure the user is
	 * not flooded with a billion notifications for association change spam,
	 * but I'm not entirely sure and I don't care to look into it right now.
	 */
	virtual HRESULT UpdatePromptCount(DWORD/*ASSOCHANDLER_PROMPTUPDATE_BEHAVIOR*/) = 0;
};

MIDL_INTERFACE("571A5DB3-3B08-441F-B796-68E8164259BB")
IAssocHandlerMakeDefault : public IUnknown
{
	/**
	 * This method was removed at some point.
	 */
	virtual HRESULT placeholder0(void *) = 0;
	
	/*
	 * Tries to register the application association.
	 * 
	 * It seems that this is responsible for registering a program as being
	 * able to be associated with a file type in the future, rather than
	 * being used to set the association. The naming that Microsoft uses is
	 * quite weird.
	 * 
	 * For future reference, this method is only used if the type is one of
	 * {ANY_PROGID, PROGID, MACHINE_DEFAULT} by the shell32 unexposed
	 * function `SetDefaultAssociationForAssocHandler`.
	 * 
	 * To reiterate: this is called specifically for AHTYPE values of:
	 * - UNDEFINED
	 * - USER_APPLICATION
	 * - ANY_APPLICATION
	 * - APPLICATION
	 * - CLASS_APPLICATION
	 * 
	 * Notably, it is not called when a ProgID exists, seemingly because it
	 * is responsible for making the ProgID; it calls AssocMakeProgid.
	 * 
	 * NOTE: I do not think that this is responsible for the automatic
	 * generation of Software\Classes\Applications\[basename.exe] keys in the
	 * registry. This process seems to be invoked in the implementation (copied
	 * between shell32 and Windows.Storage.dll) in the function
	 * `_CreateHKCUApplicationKey` or `OpenHKCRApplicationKey` (both of which are
	 * called by CAssocHandler::Init), depending on the AHTYPE. Specifically, a
	 * user key is created if AHTYPE is USER_APPLICATION, and the global root
	 * is used otherwise (AHTYPE is either ANY_APPLICATION or APPLICATION).
	 * Obviously, it is not used when some sort of registration already exists,
	 * such as a ProgID.
	 */
	virtual HRESULT TryRegisterApplicationAssoc() = 0;
};
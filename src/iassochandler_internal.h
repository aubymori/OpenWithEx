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
enum ASSOC_PROGID_FORMAT
{
	APF_DEFAULT = 0,
	APF_APPLICATION = 1,
};

MIDL_INTERFACE("D5C0CDAC-7A15-4F0A-87BA-2E7AAF19E0EC")
IAssocHandlerInfo : public IUnknown
{
	/**
	 * Determine if the handler was recently installed.
	 * 
	 * This is used to highlight newly-installed applications in the GUI.
	 */
	STDMETHOD_(bool, IsRecentlyInstalled)() PURE;

	/**
	 * Get the internal ProgID of the handler.
	 * 
	 * This just wraps the IObjectWithProgID::GetProgID method, but will return
	 * a generated ProgID from the application path if no ProgID exists for the
	 * handler.
	 */
	STDMETHOD(GetInternalProgID)(ASSOC_PROGID_FORMAT fmt, LPWSTR *ppszProgId) PURE;

	/**
	 * Gets the type of the association handler.
	 * 
	 * @see {@link AHTYPE}
	 */
	STDMETHOD(GetHandlerType)(AHTYPE *pAssociationTypeOut) PURE;
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
	STDMETHOD(GetCompany)(LPWSTR *ppszCompanyName) PURE;
};

enum ASSOCHANDLER_PROMPTUPDATE_BEHAVIOR
{
     ASSOCHANDLER_PROMPTUPDATE_BEHAVIOR_CLEAR = 0x0,
     ASSOCHANDLER_PROMPTUPDATE_BEHAVIOR_DECREMENT = 0x1,
};

MIDL_INTERFACE("2AD6D87E-5A40-400C-A06F-E25378B17013")
IAssocHandlerPromptCount : public IUnknown
{
	/**
	 * Updates the prompt count, which controls the OS automatically opening the
	 * open with UI, i.e. when a new application is installed.
	 * 
	 * This is new behaviour as of Windows 10.
	 */
	STDMETHOD(UpdatePromptCount)(ASSOCHANDLER_PROMPTUPDATE_BEHAVIOR) PURE;
};

MIDL_INTERFACE("F04004DD-2583-40BA-ADED-4ACF5A04000C")
IAssocHandlerMakeDefault_Win8 : public IUnknown
{
	/**
	 * Private interface to make an association handler the default one
	 * in Windows 8 and early versions of Windows 10.
	 *
	 * This was later removed and replaced with a stub.
	 *
	 * @param szUseless  This was probably equivalent to the szDescription
	 *                   argument in the public IAssocHandler::MakeDefault
	 *                   definition, but it's not read at all by the
	 *                   implementation of this function in Shell32.
	 */
	STDMETHOD(MakeDefaultPriv)(LPCWSTR szUseless) PURE;
};

MIDL_INTERFACE("571A5DB3-3B08-441F-B796-68E8164259BB")
IAssocHandlerMakeDefault : public IUnknown
{
	/*
	 * This is a stub that will always return E_NOTIMPL after 1703.
	 */
	STDMETHOD(MakeDefaultPriv)(LPCWSTR szUseless) PURE;
	
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
	STDMETHOD(TryRegisterApplicationAssoc)() PURE;
};
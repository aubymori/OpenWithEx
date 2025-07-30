#include "openwithex.h"

#include "versionhelper.h"

#include "assocuserchoice.h"

#include "iassochandler_internal.h"

#include "wil/com.h"
#include "wil/resource.h"

/**
 * Sets the default association for Windows versions starting from Windows 10, version 1703.
 * 
 * This is currently the latest method, affecting most Windows 10 versions and all
 * current Windows 11 versions.
 * 
 * These builds use an encrypted User Choice key. The implementation of the User Choice
 * encryption and default association management is put directly into the Open With
 * agent application (in our case, OpenWithEx, but this is also true for the canonical
 * implementations in the immersive Open With UI and other places such as immersive
 * control panel).
 */
HRESULT SetDefaultAssociationForAfter1703(LPCWSTR szExtOrProtocol, IAssocHandler *pAssocHandler)
{
	wil::com_ptr<IObjectWithProgID> pProgIdObj = nullptr;
	if (FAILED(pAssocHandler->QueryInterface(IID_PPV_ARGS(&pProgIdObj))))
	{
		// This should be the case, even if the object doesn't actually
		// have a ProgID. While the implementation of this interface is
		// not documented, it seems to even be expected by Windows 10's
		// own implementation.
		return E_FAIL;
	}

	wil::com_ptr<IAssocHandlerInfo> pAssocInfo = nullptr;
	if (FAILED(pAssocHandler->QueryInterface(IID_PPV_ARGS(&pAssocInfo))))
	{
		// This is an undocumented interface implemented by IAssocHandler which
		// provides another method for getting the ProgID.
		// This is the preferred method, but if we fail, we'll just notify this
		// via the debug console for now.
		OutputDebugStringW(L"Failed to query interface IAssocHandlerInfo.\n");
	}

	AHTYPE ahType = AHTYPE_APPLICATION; // FIX(isabella): Avoid uninitialised memory.
	if (pAssocInfo)
		pAssocInfo->GetHandlerType(&ahType);

	/*
	 * KNOWING THE PROGID IS IMPORTANT!
	 *
	 * Windows makes file associations via the ProgID, not the file path.
	 * It seems that the system generates a ProgID from the App Paths
	 * when there is no ProgID (IObjectWithProgID::GetProgID returns nullptr).
	 *
	 * If there is an existing registration of the application name in the
	 * App Paths, then we get that (from the undocumented IAssocHandlerInfo
	 * interface). If we fail to get that, then we will fail to make the
	 * association.
	 *
	 * This means that we currently can't make new registrations from
	 * new handlers added via the file browser. They lack a ProgID at all, and
	 * so we just don't generate one.
	 */

	 // Get the ProgID of the element:
	wil::unique_cotaskmem_string spszProgId = nullptr;

	if (pAssocInfo)
	{
		// If we could get the IAssocHandlerInfo interface, then we'll call
		// GetInternalProgID. This automatically handles generated ProgIDs
		// like "Applications\notepad++.exe".
		if (ahType & (AHTYPE_ANY_PROGID | AHTYPE_PROGID | AHTYPE_MACHINEDEFAULT))
		{
			// If our association handler already supports ProgIDs, then we'll
			// just retrieve that ProgID.
			pAssocInfo->GetInternalProgID(
				APF_DEFAULT,
				&spszProgId
			);
		}
		else
		{
			// If we don't already have a ProgID on our object, then we'll have
			// to tell the system to make one. The following is what TWinUI does.
			// APF_APPLICATION is simply used by Shell32 to tell it to add an
			// "Applications\" prefix; if it is not specified, then
			// GetInternalProgID() just calls into the public GetProgID() method.
			pAssocInfo->GetInternalProgID(
				APF_APPLICATION,
				&spszProgId
			);

			wil::com_ptr<IAssocHandlerMakeDefault> pMakeDefault = nullptr;
			if (SUCCEEDED(pAssocHandler->QueryInterface(IID_PPV_ARGS(&pMakeDefault))))
			{
				pMakeDefault->TryRegisterApplicationAssoc();
			}
		}
	}
	else
	{
		// If we couldn't, then we'll fall back to the IObjectWithProgID
		// method. This does not handle the above case, so it's inherently
		// limited.
		pProgIdObj->GetProgID(&spszProgId);
	}

	if (!spszProgId)
	{
		// If we couldn't get any ProgID at all, then we cancel the registration.
		OutputDebugStringW(L"Application doesn't have a ProgID; exiting.");
		return E_FAIL;
	}

	OutputDebugStringW(L"\nProgID: ");
	OutputDebugStringW(spszProgId.get());
	OutputDebugStringW(L"\n");

	SetUserChoiceAndHashResult userChoiceResult =
		SetUserChoiceAndHash(
			szExtOrProtocol,
			spszProgId.get()
		);
		
	return S_OK;
}

/**
 * Sets the default association for Windows 8.0, 8.1, and early releases of Windows 10
 * before the later User Choice system was introduced.
 * 
 * These versions of Windows just used a private interface.
 */
HRESULT SetDefaultAssociationForPre1703(LPCWSTR szExtOrProtocol, IAssocHandler *pAssocHandler)
{
	wil::com_ptr<IAssocHandlerMakeDefault_Win8> pMakeDefault = nullptr;
	if (SUCCEEDED(pAssocHandler->QueryInterface(IID_PPV_ARGS(&pMakeDefault))))
	{
		// Immersive UI sends nullptr in Windows 8.1 TWinUI too. Furthermore, that
		// argument does not seem to be used for anything in the Windows 8.0
		// implementation of this function in Shell32 (I double checked).
		return pMakeDefault->MakeDefaultPriv(nullptr);
	}

	return E_FAIL;
}

/**
 * Set the default association for the extension or protocol to the specified
 * handler.
 *
 * This function will diffuse to the correct OS-specific behaviour depending
 * on the user's operating system.
 */
HRESULT SetDefaultAssociation(LPCWSTR szExtOrProtocol, IAssocHandler *pAssocHandler)
{
	if (CVersionHelper::IsWindows10_1703OrGreater())
	{
		return SetDefaultAssociationForAfter1703(szExtOrProtocol, pAssocHandler);
	}
	else if (CVersionHelper::IsWindows8OrGreater())
	{
		return SetDefaultAssociationForPre1703(szExtOrProtocol, pAssocHandler);
	}
	else // < Windows 8
	{
		// If someone got OpenWithEx working on Windows 7 or an even earlier
		// version, then they'd be really pleased to know that it's WAY too
		// easy. We pass nullptr because we set the description of the handler
		// in baseopenasdlg.cpp.
		return pAssocHandler->MakeDefault(nullptr);
	}

	return E_NOTIMPL;
}
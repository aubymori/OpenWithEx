#include "openasdlg.h"
#include <shlwapi.h>
#include <shlobj.h>
#include <uxtheme.h>
#include <propvarutil.h>
#include <propkey.h>
#include <memory>

#include "assocuserchoice.h"
#include "versionhelper.h"
#include "iassochandler_internal.h"

LPCWSTR s_szSysTypes[] = {
	L".bin",
	L".dat",
	L".dos"
};

/*
 * Get the app icon.
 * 
 * @param lpszIconPath  The path of the binary containing the icon, or an
 *                      indirect string in the case of UWP applications.
 * @param iIndex        The index of the icon in the binary.
 * 
 */
inline static int GetAppIconIndex(LPCWSTR lpszIconPath, int iIndex)
{
	/*
	 * This seems to be an edge case handled in TWinUI.
	 * 
	 * I did not bother looking into it further.
	 */
	if (iIndex == -1)
	{
		return Shell_GetCachedImageIndexW(L"shell32.dll", 2, 0);
	}

	/*
	 * Get the app icon for UWP applications.
	 *
	 * In the documentation for Shell_GetCachedImageIndex, Microsoft straight up
	 * lies about the purpose of the `uIconFlags` argument, stating that it is
	 * not used. It is, in fact, used.
	 *
	 * TWinUI happily supplies an argument for it in
	 * `CImmersiveOpenWithUI::_GetIcon()`.
	 *
	 * https://learn.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-shell_getcachedimageindexa
	 *
	 * I copied this logic from TWinUI, but I spent some time figuring out why it
	 * works as well.
	 *
	 * The gist of it is that this uIconFlags argument, like arguments of the same
	 * name on other functions, is one of the GIL_ variables. Of course, Microsoft
	 * didn't bother documenting this, but it can be observed by a simple GitHub
	 * search of the variable name, which will show you leaked Windows source code
	 * in the search results with proper documentation available.
	 *
	 * https://files.catbox.moe/z4xgza.png
	 *
	 * When you call IAssocHandler::GetIconLocation() on a UWP handler, instead of
	 * getting an absolute file system path, you get a uniform resource locator
	 * wrapped in a custom format which begins with "@". I have no idea what this
	 * is exactly, but apparently it's called an "indirect string" and will always
	 * start with an "@" character.
	 *
	 * https://learn.microsoft.com/en-us/windows/win32/api/shlwapi/nf-shlwapi-shloadindirectstring
	 *
	 * Anyway, TWinUI doesn't bother with this indirect string, and neither will
	 * we. It just sets the flags to GIL_ASYNC | GIL_NOTFILENAME and calls into
	 * `Shell_GetCachedImageIndexW()` like normal. This is enough to get the
	 * icon of a UWP application, and has worked perfectly in my testing.
	 * 
	 * According to Microsoft's C headers, GIL_NOTFILENAME causes the system to
	 * call into ExtractIcon() to get the icon. This flag results in no icon
	 * being displayed at all without GIL_ASYNC, so it seems that this only works
	 * asynchronously.
	 * 
	 * https://github.com/tpn/winsdk-10/blob/master/Include/10.0.10240.0/um/ShlObj.h#L258
	 * 
	 * The aforementioned function `CImmersiveOpenWithUI::_GetIcon()` in TWinUI
	 * does perform some additional work in case `Shell_GetCachedImageIndex()`
	 * doesn't work, but we don't handle any such case as of right now.
	 */
	UINT uIconFlags = NULL;
	if (lpszIconPath && lpszIconPath[0] == L'@')
		uIconFlags = GIL_ASYNC | GIL_NOTFILENAME;

	return Shell_GetCachedImageIndexW(
		lpszIconPath, iIndex, uIconFlags
	);
}

INT_PTR CALLBACK COpenAsDlg::v_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			/* Set icon */
			HMODULE hShell32 = LoadLibraryW(L"shell32.dll");
			HICON hiOpenWith = LoadIconW(hShell32, MAKEINTRESOURCEW(134));
			SendDlgItemMessageW(
				hWnd,
				IDD_OPENWITH_ICON,
				STM_SETICON,
				(WPARAM)hiOpenWith,
				NULL
			);

			/* Set path text */
			SetDlgItemTextW(
				hWnd,
				IDD_OPENWITH_FILE,
				m_pszFileName
			);

			/* Set up checkbox */
			if (!m_szExtOrProtocol || !*m_szExtOrProtocol)
			{
				EnableWindow(
					GetDlgItem(hWnd, IDD_OPENWITH_ASSOC),
					FALSE
				);
			}
			else
			{
				SendDlgItemMessageW(
					hWnd,
					IDD_OPENWITH_ASSOC,
					BM_SETCHECK,
					BST_CHECKED,
					NULL
				);
			}

			/* Theme list view */
#ifndef XP
			SetWindowTheme(
				GetDlgItem(hWnd, IDD_OPENWITH_LISTVIEW),
				L"Explorer",
				NULL
			);
			SendDlgItemMessageW(
				hWnd,
				IDD_OPENWITH_LISTVIEW,
				LVM_SETVIEW,
				LV_VIEW_TILE,
				NULL
			);
#endif

			HIMAGELIST himl = NULL;
			HIMAGELIST himlSmall = NULL;
			Shell_GetImageLists(&himl, &himlSmall);
#ifndef XP
			SendDlgItemMessageW(
				hWnd,
				IDD_OPENWITH_LISTVIEW,
				LVM_SETIMAGELIST,
				LVSIL_NORMAL,
				(LPARAM)himl
			);
#endif
			SendDlgItemMessageW(
				hWnd,
				IDD_OPENWITH_LISTVIEW,
#ifdef XP
				TVM_SETIMAGELIST,
				TVSIL_NORMAL,
#else
				LVM_SETIMAGELIST,
				LVSIL_SMALL,
#endif
				(LPARAM)himlSmall
			);

#ifdef XP
			SendDlgItemMessageW(
				hWnd,
				IDD_OPENWITH_LISTVIEW,
				TVM_SETIMAGELIST,
				TVSIL_STATE,
				(LPARAM)himlSmall
			);
#endif

#ifndef XP
			LVCOLUMNW col = { 0 };
			col.mask = LVCF_SUBITEM;
			col.iSubItem = 0;
			SendDlgItemMessageW(
				hWnd, IDD_OPENWITH_LISTVIEW,
				LVM_INSERTCOLUMNW, NULL,
				(LPARAM)&col
			);
			col.iSubItem = 1;
			SendDlgItemMessageW(
				hWnd, IDD_OPENWITH_LISTVIEW,
				LVM_INSERTCOLUMNW, NULL,
				(LPARAM)&col
			);
#endif

			_GetHandlers();
			
			if (m_bRecommended)
				_SetupCategories();

			for (size_t i = 0; i < m_handlers.size(); i++)
			{
				_AddItem(m_handlers.at(i), i, false);
			}

			return TRUE;
		}
		case WM_CLOSE:
			EndDialog(hWnd, IDCANCEL);
			return TRUE;
		case WM_COMMAND:
			switch (wParam)
			{
				case IDOK:
					_OnOk();
					break;
				case IDCANCEL:
					EndDialog(hWnd, IDCANCEL);
					break;
				case IDD_OPENWITH_BROWSE:
					_BrowseForProgram();
					break;
			}
			break;
		case WM_NOTIFY:
			switch (((LPNMHDR)lParam)->code)
			{
#ifdef XP
				case TVN_SELCHANGED:
#else
				case LVN_ITEMCHANGED:
#endif
					EnableWindow(
						GetDlgItem(hWnd, IDOK),
						_GetSelectedItem() != nullptr
					);

			}
			break;
	}

	return FALSE;
}

IAssocHandler *COpenAsDlg::_GetSelectedItem()
{
#ifdef XP
	HTREEITEM hSelected = (HTREEITEM)SendDlgItemMessageW(
		m_hWnd, IDD_OPENWITH_LISTVIEW,
		TVM_GETNEXTITEM, TVGN_CARET,
		NULL
	);
	if (!hSelected)
		return nullptr;

	TVITEMW tvi = { 0 };
	tvi.mask = TVIF_PARAM | TVIF_HANDLE;
	tvi.hItem = hSelected;

	SendDlgItemMessageW(
		m_hWnd, IDD_OPENWITH_LISTVIEW,
		TVM_GETITEM, NULL,
		(LPARAM)&tvi
	);

	return (IAssocHandler *)tvi.lParam;
#else
	int index = SendDlgItemMessageW(
		m_hWnd, IDD_OPENWITH_LISTVIEW,
		LVM_GETNEXTITEM, -1, LVNI_SELECTED
	);

	if (index == -1)
		return nullptr;

	LVITEMW lvi = { 0 };
	lvi.iItem = index;
	lvi.mask = LVIF_PARAM;

	SendDlgItemMessageW(
		m_hWnd, IDD_OPENWITH_LISTVIEW,
		LVM_GETITEMW, NULL,
		(LPARAM)&lvi
	);

	return (IAssocHandler *)lvi.lParam;
#endif
}

int COpenAsDlg::_FindItemIndex(LPCWSTR lpszPath)
{
	IAssocHandler *pResult = nullptr;
	for (size_t i = 0; i < m_handlers.size(); i++)
	{
		IAssocHandler *pHandler = m_handlers.at(i);
		LPWSTR lpszCurPath = nullptr;
		pHandler->GetName(&lpszCurPath);
		if (lpszCurPath)
		{
			if (0 == _wcsicmp(lpszPath, lpszCurPath))
				pResult = pHandler;

			CoTaskMemFree(lpszCurPath);
		}

		if (pResult)
			return i;
	}
	return -1;
}

void COpenAsDlg::_SelectItemByIndex(int index)
{
#ifdef XP
	HTREEITEM hItem = m_treeItems.at(index);
	if (hItem)
	{
		SetFocus(GetDlgItem(m_hWnd, IDD_OPENWITH_LISTVIEW));
		SendDlgItemMessageW(
			m_hWnd, IDD_OPENWITH_LISTVIEW,
			TVM_SELECTITEM, TVGN_CARET,
			(LPARAM)hItem
		);
	}
#else
	LVITEMW lvi = { 0 };
	lvi.iItem = index;
	lvi.mask = LVIF_STATE;
	lvi.stateMask = LVIS_SELECTED;
	lvi.state = LVIS_SELECTED;

	SetFocus(GetDlgItem(m_hWnd, IDD_OPENWITH_LISTVIEW));
	SendDlgItemMessageW(
		m_hWnd, IDD_OPENWITH_LISTVIEW,
		LVM_SETITEMW, NULL,
		(LPARAM)&lvi
	);

	IAssocHandler *pHandler = m_handlers.at(index);
	if (pHandler)
	{
		LVGROUP lvg = { sizeof(LVGROUP) };
		lvg.mask = LVGF_STATE;
		lvg.stateMask = LVGS_COLLAPSED;

		SendDlgItemMessageW(
			m_hWnd, IDD_OPENWITH_LISTVIEW,
			LVM_SETGROUPINFO,
			(S_OK == pHandler->IsRecommended()) ? I_RECOMMENDED : I_OTHER,
			(LPARAM)&lvg
		);
	}

	SendDlgItemMessageW(
		m_hWnd, IDD_OPENWITH_LISTVIEW,
		LVM_ENSUREVISIBLE, index,
		TRUE
	);
#endif
}

void COpenAsDlg::_SetupCategories()
{
#ifdef XP
	TVITEMW recommended = { 0 };
	recommended.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_STATE;
	recommended.iImage = Shell_GetCachedImageIndexW(L"shell32.dll", 19, NULL);
	recommended.iSelectedImage = recommended.iImage;
	recommended.state = TVIS_EXPANDED;
	recommended.stateMask = TVIS_EXPANDED;
	WCHAR szRecommended[MAX_PATH] = { 0 };
	LoadStringW(g_hMuiInstance, IDS_RECOMMENDED, szRecommended, MAX_PATH);
	recommended.pszText = szRecommended;
	recommended.cchTextMax = MAX_PATH;

	TVINSERTSTRUCTW insert = { 0 };
	insert.item = recommended;

	m_hRecommended = (HTREEITEM)SendDlgItemMessageW(
		m_hWnd,
		IDD_OPENWITH_LISTVIEW,
		TVM_INSERTITEMW,
		NULL,
		(LPARAM)&insert
	);

	TVITEMW other = recommended;
	WCHAR szOther[MAX_PATH] = { 0 };
	LoadStringW(g_hMuiInstance, IDS_OTHER, szOther, MAX_PATH);
	other.pszText = szOther;
	insert.item = other;
	m_hOther = (HTREEITEM)SendDlgItemMessageW(
		m_hWnd,
		IDD_OPENWITH_LISTVIEW,
		TVM_INSERTITEMW,
		NULL,
		(LPARAM)&insert
	);
#else
	SendDlgItemMessageW(
		m_hWnd,
		IDD_OPENWITH_LISTVIEW,
		LVM_ENABLEGROUPVIEW,
		TRUE,
		NULL
	);

	LVGROUP recommended = { sizeof(LVGROUP) };
	recommended.mask = LVGF_HEADER | LVGF_GROUPID | LVGF_STATE;
	recommended.iGroupId = I_RECOMMENDED;
	recommended.state = LVGS_COLLAPSIBLE;
	recommended.stateMask = LVGS_COLLAPSIBLE;
	WCHAR szRecommended[MAX_PATH] = { 0 };
	LoadStringW(g_hMuiInstance, IDS_RECOMMENDED, szRecommended, MAX_PATH);
	recommended.pszHeader = szRecommended;
	recommended.cchHeader = MAX_PATH;
	SendDlgItemMessageW(
		m_hWnd,
		IDD_OPENWITH_LISTVIEW,
		LVM_INSERTGROUP,
		-1,
		(LPARAM)&recommended
	);

	LVGROUP other = recommended;
	other.iGroupId = I_OTHER;
	WCHAR szOther[MAX_PATH] = { 0 };
	LoadStringW(g_hMuiInstance, IDS_OTHER, szOther, MAX_PATH);
	other.pszHeader = szOther;
	other.state = LVGS_COLLAPSIBLE | LVGS_COLLAPSED;
	other.stateMask = LVGS_COLLAPSIBLE | LVGS_COLLAPSED;

	SendDlgItemMessageW(
		m_hWnd,
		IDD_OPENWITH_LISTVIEW,
		LVM_INSERTGROUP,
		-1,
		(LPARAM)&other
	);
#endif
}

void COpenAsDlg::_AddItem(IAssocHandler *pItem, int index, bool bForceSelect)
{
#ifdef XP
	TVITEMW tvi = { 0 };
	tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	pItem->GetUIName(&tvi.pszText);
	tvi.cchTextMax = MAX_PATH;
	tvi.lParam = (LPARAM)pItem;

	LPWSTR pszPath = nullptr;
	int    iIndex = 0;
	pItem->GetIconLocation(
		&pszPath,
		&iIndex
	);

	tvi.iImage = GetAppIconIndex(
		pszPath, iIndex
	);
	tvi.iSelectedImage = tvi.iImage;

	TVINSERTSTRUCTW insert = { 0 };
	insert.item = tvi;
	insert.hInsertAfter = TVI_LAST;
	if (m_bRecommended)
	{
		insert.hParent = (S_OK == pItem->IsRecommended()) ? m_hRecommended : m_hOther;
	}

	m_treeItems.push_back((HTREEITEM)SendDlgItemMessageW(
		m_hWnd,
		IDD_OPENWITH_LISTVIEW,
		TVM_INSERTITEMW,
		NULL,
		(LPARAM)&insert
	));
#else
	LVITEMW lvi = { 0 };
	lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
	lvi.iItem = index;
	if (m_bRecommended)
	{
		lvi.mask |= LVIF_GROUPID;
		lvi.iGroupId = (S_OK == pItem->IsRecommended()) ? I_RECOMMENDED : I_OTHER;
	}
	pItem->GetUIName(&lvi.pszText);
	lvi.cchTextMax = MAX_PATH;
	lvi.lParam = (LPARAM)pItem;

	if (index == 0 || bForceSelect)
	{
		lvi.mask |= LVIF_STATE;
		lvi.stateMask = LVIS_SELECTED;
		lvi.state = LVIS_SELECTED;
	}
	
	LPWSTR pszIconPath = nullptr;
	int    iIndex   = 0;
	pItem->GetIconLocation(
		&pszIconPath,
		&iIndex
	);

	lvi.iImage = GetAppIconIndex(
		pszIconPath, iIndex
	);
	CoTaskMemFree(pszIconPath);

	SendDlgItemMessageW(
		m_hWnd,
		IDD_OPENWITH_LISTVIEW,
		LVM_INSERTITEMW,
		NULL,
		(LPARAM)&lvi
	);

	IAssocHandlerWithCompanyName *pCompanyNameInfo = nullptr;
	HRESULT hr = pItem->QueryInterface(IID_PPV_ARGS(&pCompanyNameInfo));
	std::unique_ptr<WCHAR[]> pszCompanyName = nullptr;

	if (SUCCEEDED(hr))
	{
		// If we could get the IAssocHandlerWithCompanyName object, then we just
		// query that for the company name.
		LPWSTR buffer = nullptr;
		hr = pCompanyNameInfo->GetCompany(&buffer);

		if (buffer)
		{
			int cchBuffer = lstrlenW(buffer) + 1;
			pszCompanyName = std::make_unique<WCHAR[]>(cchBuffer);
			wcscpy_s(pszCompanyName.get(), cchBuffer, buffer);
		}

		CoTaskMemFree(buffer);
	}

	if (FAILED(hr))
	{
		// If we failed to get the IAssocHandlerWithCompanyName object, or failed to
		// query the company name from that, then we will attempt to query it from
		// the shell item properties instead.
		LPWSTR pszPath = nullptr;

		// BUGBUG: UWP applications report their display names (i.e. "Photos") instead
		// of their location when calling this function. Thus, they will fail this
		// procedure.
		pItem->GetName(&pszPath);

		IShellItem2 *psi = nullptr;
		hr = SHCreateItemFromParsingName(pszPath, nullptr, IID_PPV_ARGS(&psi));
		CoTaskMemFree(pszPath);

		if (SUCCEEDED(hr))
		{
			PROPVARIANT pvar;
			PropVariantInit(&pvar);
			psi->GetProperty(PKEY_Company, &pvar);

			if (pvar.vt == VT_LPWSTR && pvar.pwszVal)
			{
				int cchCompanyName = lstrlenW(pvar.pwszVal) + 1;
				pszCompanyName = std::make_unique<WCHAR[]>(cchCompanyName);
				memcpy(pszCompanyName.get(), pvar.pwszVal, cchCompanyName * sizeof(WCHAR));
			}

			PropVariantClear(&pvar);
		}
	}

	if (SUCCEEDED(hr))
	{
		if (pszCompanyName)
		{
			LVTILEINFO lvti = { sizeof(LVTILEINFO) };
			lvti.iItem = index;
			lvti.cColumns = 1;
			UINT cols[] = { 1 };
			lvti.puColumns = cols;

			SendDlgItemMessageW(
				m_hWnd, IDD_OPENWITH_LISTVIEW,
				LVM_SETTILEINFO, 0,
				(LPARAM)&lvti
			);

			LVITEMW lvi = { 0 };
			lvi.pszText = pszCompanyName.get();
			lvi.iSubItem = 1;
			SendDlgItemMessageW(
				m_hWnd, IDD_OPENWITH_LISTVIEW,
				LVM_SETITEMTEXTW, index,
				(LPARAM)&lvi
			);
		}
	}

	if (pCompanyNameInfo)
	{
		pCompanyNameInfo->Release();
	}
#endif
}

void COpenAsDlg::_GetHandlers()
{
	IEnumAssocHandlers *pEnumHandler = nullptr;
	SHAssocEnumHandlers(
		m_szExtOrProtocol,
		ASSOC_FILTER_NONE,
		&pEnumHandler
	);

	IAssocHandler *pAssoc = nullptr;
	ULONG pceltFetched = 0;
	while (SUCCEEDED(pEnumHandler->Next(1, &pAssoc, &pceltFetched)) && pAssoc)
	{
		if (!m_bRecommended && S_OK == pAssoc->IsRecommended())
		{
			m_bRecommended = true;
		}
		m_handlers.push_back(pAssoc);
	}
}

void COpenAsDlg::_BrowseForProgram()
{
	IShellItem *psi = nullptr;
	WCHAR szPath[MAX_PATH] = { 0 };
	ExpandEnvironmentStringsW(L"%ProgramFiles%", szPath, MAX_PATH);
	SHCreateItemFromParsingName(szPath, nullptr, IID_PPV_ARGS(&psi));

	IFileOpenDialog *pdlg = nullptr;
	CoCreateInstance(
		CLSID_FileOpenDialog,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pdlg)
	);
	
	if (!pdlg)
		return;

	pdlg->SetFolder(psi);

	WCHAR szPrograms[MAX_PATH] = { 0 };
	WCHAR szAllFiles[MAX_PATH] = { 0 };
	LoadStringW(g_hMuiInstance, IDS_PROGRAMS, szPrograms, MAX_PATH);
	LoadStringW(g_hMuiInstance, IDS_ALLFILES, szAllFiles, MAX_PATH);

	COMDLG_FILTERSPEC filters[] = {
		{ szPrograms, L"*.exe;*.pif;*.com;*.bat;*.cmd" },
		{ szAllFiles, L"*.*" }
	};

	pdlg->SetFileTypes(ARRAYSIZE(filters), filters);

	WCHAR szTitle[MAX_PATH] = { 0 };
	LoadStringW(g_hMuiInstance, IDS_BROWSETITLE, szTitle, MAX_PATH);
	pdlg->SetTitle(szTitle);

	pdlg->Show(m_hWnd);

	IShellItem *pResult = nullptr;
	pdlg->GetResult(&pResult);

	if (pResult)
	{
		LPWSTR lpszPath = nullptr;
		pResult->GetDisplayName(SIGDN_FILESYSPATH, &lpszPath);
		if (lpszPath)
		{
			int index = _FindItemIndex(lpszPath);
			if (index != -1)
			{
				_SelectItemByIndex(index);
			}
			else
			{
				IAssocHandler *pHandler = nullptr;
				SHCreateAssocHandler(AHTYPE_USER_APPLICATION, m_szExtOrProtocol, lpszPath, &pHandler);
				if (pHandler)
				{
					m_handlers.push_back(pHandler);
					_AddItem(pHandler, m_handlers.size() - 1, true);
					_SelectItemByIndex(m_handlers.size() - 1);
				}
				CoTaskMemFree(lpszPath);
			}
		}
	}
}

void COpenAsDlg::_OnOk()
{
	bool bAssoc = IsDlgButtonChecked(m_hWnd, IDD_OPENWITH_ASSOC);
	WCHAR szDesc[MAX_PATH] = { 0 };
	GetDlgItemTextW(
		m_hWnd,
		IDD_OPENWITH_DESC,
		szDesc,
		MAX_PATH
	);

	IAssocHandler *pSelected = _GetSelectedItem();
	if (pSelected)
	{
		EndDialog(m_hWnd, IDOK);

		// Start the program with the file:
		IShellItem2 *pShellItem = nullptr;
		HRESULT hr = SHCreateItemFromParsingName(m_szPath, nullptr, IID_PPV_ARGS(&pShellItem));

		if (SUCCEEDED(hr) && pShellItem)
		{
			IDataObject *pInvocationObj = nullptr;
			hr = pShellItem->BindToHandler(nullptr, BHID_DataObject, IID_PPV_ARGS(&pInvocationObj));

			if (SUCCEEDED(hr))
			{
				pSelected->Invoke(pInvocationObj);

				if (pInvocationObj)
					pInvocationObj->Release();

				if (pShellItem)
					pShellItem->Release();
			}
		}

		if (bAssoc)
		{
			// We're setting the file association now, so let's have some
			// fun!
			if (!CVersionHelper::IsWindows10OrGreater())
			{
				// We don't currently handle pre-Windows 10 cases.
				return;
			}

			IObjectWithProgID *pProgIdObj = nullptr;
			if (FAILED(pSelected->QueryInterface(IID_PPV_ARGS(&pProgIdObj))))
			{
				// This should be the case, even if the object doesn't actually
				// have a ProgID. While the implementation of this interface is
				// not documented, it seems to even be expected by Windows 10's
				// own implementation.
				return;
			}

			IAssocHandlerInfo *pAssocInfo = nullptr;
			if (FAILED(pSelected->QueryInterface(IID_PPV_ARGS(&pAssocInfo))))
			{
				// This is an undocumented interface implemented by IAssocHandler which
				// provides another method for getting the ProgID.
				// This is the preferred method, but if we fail, we'll just notify this
				// via the debug console for now.
				OutputDebugStringW(L"Failed to query interface IAssocHandlerInfo.\n");
			}

			bool bCancelAssoc = false;

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
			LPWSTR lpszProgId = nullptr;

			if (pAssocInfo)
			{
				// If we could get the IAssocHandlerInfo interface, then we'll call
				// GetInternalProgID. This automatically handles generated ProgIDs
				// like "Applications\notepad++.exe".
				pAssocInfo->GetInternalProgID(
					ASSOC_PROGID_FORMAT::USE_GENERATED_PROGID_IF_NECESSARY,
					&lpszProgId
				);
			}
			else
			{
				// If we couldn't, then we'll fall back to the IObjectWithProgID
				// method. This does not handle the above case, so it's inherently
				// limited.
				pProgIdObj->GetProgID(&lpszProgId);
			}

			if (!lpszProgId)
			{
				// If we couldn't get any ProgID at all, then we cancel the registration.
				OutputDebugStringW(L"Application doesn't have a ProgID; exiting.");
				bCancelAssoc = true;
			}

			if (!bCancelAssoc)
			{
				OutputDebugStringW(L"\nProgID: ");
				OutputDebugStringW(lpszProgId);
				OutputDebugStringW(L"\n");

				SetUserChoiceAndHashResult userChoiceResult =
					SetUserChoiceAndHash(
						(LPCWSTR)&m_szExtOrProtocol,
						lpszProgId
					);
			}

			if (pAssocInfo)
			{
				pAssocInfo->Release();
			}

			if (pProgIdObj)
			{
				pProgIdObj->Release();
			}

			CoTaskMemFree(lpszProgId);
		}
	}
}

COpenAsDlg::COpenAsDlg(LPCWSTR lpszPath, bool bOverride, bool bUri)
	: CImpDialog(g_hMuiInstance, IDD_OPENWITH_WITHDESC)
	, m_szExtOrProtocol{ 0 }
	, m_bOverride(bOverride)
	, m_bUri(bUri)
	, m_bRecommended(false)
{
	wcscpy_s(m_szPath, lpszPath);
	m_pszFileName = PathFindFileNameW(m_szPath);
	if (m_bUri)
	{
		WCHAR *pColon = wcschr(m_szPath, L':');
		/* Protocol length */
		int length = ((long long)pColon - (long long)m_szPath) / sizeof(WCHAR);
		wcsncpy_s(m_szExtOrProtocol, m_szPath, length);
	}
	else
	{
		LPWSTR pszExtension = PathFindExtensionW(m_szPath);
		if (pszExtension)
		{
			wcscpy_s(m_szExtOrProtocol, pszExtension);
		}
	}

	if (m_bOverride || m_bUri || !m_szExtOrProtocol || !*m_szExtOrProtocol)
		m_uDlgId = IDD_OPENWITH;
}

COpenAsDlg::~COpenAsDlg()
{
	for (size_t i = 0; i < m_handlers.size(); i++)
	{
		IAssocHandler *pItem = m_handlers.at(i);
		if (pItem)
		{
			pItem->Release();
		}
	}
	m_handlers.clear();
}
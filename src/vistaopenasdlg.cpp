#include "vistaopenasdlg.h"

void CVistaOpenAsDlg::_BrowseForProgram()
{
	wil::com_ptr<IShellItem> psi = nullptr;
	WCHAR szPath[MAX_PATH] = { 0 };
	ExpandEnvironmentStringsW(L"%ProgramFiles%", szPath, MAX_PATH);
	SHCreateItemFromParsingName(szPath, nullptr, IID_PPV_ARGS(&psi));

	wil::com_ptr_nothrow<IFileOpenDialog> pdlg = nullptr;
	CoCreateInstance(
		CLSID_FileOpenDialog,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pdlg)
	);

	if (!pdlg)
		return;

	pdlg->SetFolder(psi.get());

	WCHAR szPrograms[MAX_PATH] = { 0 };
	WCHAR szAllFiles[MAX_PATH] = { 0 };
	LoadStringW(g_hInst, IDS_PROGRAMS, szPrograms, MAX_PATH);
	LoadStringW(g_hInst, IDS_ALLFILES, szAllFiles, MAX_PATH);

	COMDLG_FILTERSPEC filters[] = {
		{ szPrograms, L"*.exe;*.pif;*.com;*.bat;*.cmd" },
		{ szAllFiles, L"*.*" }
	};

	pdlg->SetFileTypes(ARRAYSIZE(filters), filters);

	WCHAR szTitle[MAX_PATH] = { 0 };
	LoadStringW(g_hInst, IDS_BROWSETITLE, szTitle, MAX_PATH);
	pdlg->SetTitle(szTitle);
	pdlg->Show(m_hWnd);

	wil::com_ptr_nothrow<IShellItem> pResult = nullptr;
	pdlg->GetResult(&pResult);

	if (pResult)
	{
		wil::unique_cotaskmem_string pszPath;
		pResult->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
		if (pszPath.get())
		{
			_SelectOrAddItem(pszPath.get());
		}
	}
}

void CVistaOpenAsDlg::_InitProgList()
{
	/* Theme list view */
	SetWindowTheme(
		GetDlgItem(m_hWnd, IDD_OPENWITH_PROGLIST),
		L"Explorer",
		NULL
	);
	SendDlgItemMessageW(
		m_hWnd,
		IDD_OPENWITH_PROGLIST,
		LVM_SETVIEW,
		LV_VIEW_TILE,
		NULL
	);

	HIMAGELIST himl = NULL;
	HIMAGELIST himlSmall = NULL;
	Shell_GetImageLists(&himl, &himlSmall);
	SendDlgItemMessageW(
		m_hWnd,
		IDD_OPENWITH_PROGLIST,
		LVM_SETIMAGELIST,
		LVSIL_NORMAL,
		(LPARAM)himl
	);
	SendDlgItemMessageW(
		m_hWnd,
		IDD_OPENWITH_PROGLIST,
		LVM_SETIMAGELIST,
		LVSIL_SMALL,
		(LPARAM)himlSmall
	);

	LVCOLUMNW col = { 0 };
	col.mask = LVCF_SUBITEM;
	col.iSubItem = 0;
	SendDlgItemMessageW(
		m_hWnd, IDD_OPENWITH_PROGLIST,
		LVM_INSERTCOLUMNW, NULL,
		(LPARAM)&col
	);
	col.iSubItem = 1;
	SendDlgItemMessageW(
		m_hWnd, IDD_OPENWITH_PROGLIST,
		LVM_INSERTCOLUMNW, NULL,
		(LPARAM)&col
	);
}

wil::com_ptr<IAssocHandler> CVistaOpenAsDlg::_GetSelectedItem()
{
	int index = SendDlgItemMessageW(
		m_hWnd, IDD_OPENWITH_PROGLIST,
		LVM_GETNEXTITEM, -1, LVNI_SELECTED
	);

	if (index == -1)
		return nullptr;

	LVITEMW lvi = { 0 };
	lvi.iItem = index;
	lvi.mask = LVIF_PARAM;

	SendDlgItemMessageW(
		m_hWnd, IDD_OPENWITH_PROGLIST,
		LVM_GETITEMW, NULL,
		(LPARAM)&lvi
	);

	return (IAssocHandler *)lvi.lParam;
}

void CVistaOpenAsDlg::_SelectItemByIndex(int index)
{
	LVITEMW lvi = { 0 };
	lvi.iItem = index;
	lvi.mask = LVIF_STATE;
	lvi.stateMask = LVIS_SELECTED;
	lvi.state = LVIS_SELECTED;

	SetFocus(GetDlgItem(m_hWnd, IDD_OPENWITH_PROGLIST));
	SendDlgItemMessageW(
		m_hWnd, IDD_OPENWITH_PROGLIST,
		LVM_SETITEMW, NULL,
		(LPARAM)&lvi
	);

	wil::com_ptr<IAssocHandler> pHandler = m_handlers.at(index);
	if (pHandler)
	{
		LVGROUP lvg = { sizeof(LVGROUP) };
		lvg.mask = LVGF_STATE;
		lvg.stateMask = LVGS_COLLAPSED;

		SendDlgItemMessageW(
			m_hWnd, IDD_OPENWITH_PROGLIST,
			LVM_SETGROUPINFO,
			(S_OK == pHandler->IsRecommended()) ? I_RECOMMENDED : I_OTHER,
			(LPARAM)&lvg
		);
	}

	SendDlgItemMessageW(
		m_hWnd, IDD_OPENWITH_PROGLIST,
		LVM_ENSUREVISIBLE, index,
		TRUE
	);
}

void CVistaOpenAsDlg::_SetupCategories()
{
	SendDlgItemMessageW(
		m_hWnd,
		IDD_OPENWITH_PROGLIST,
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
	LoadStringW(g_hInst, IDS_RECOMMENDED, szRecommended, MAX_PATH);
	recommended.pszHeader = szRecommended;
	recommended.cchHeader = MAX_PATH;
	SendDlgItemMessageW(
		m_hWnd,
		IDD_OPENWITH_PROGLIST,
		LVM_INSERTGROUP,
		-1,
		(LPARAM)&recommended
	);

	LVGROUP other = recommended;
	other.iGroupId = I_OTHER;
	WCHAR szOther[MAX_PATH] = { 0 };
	LoadStringW(g_hInst, IDS_OTHER, szOther, MAX_PATH);
	other.pszHeader = szOther;
	other.state = LVGS_COLLAPSIBLE | LVGS_COLLAPSED;
	other.stateMask = LVGS_COLLAPSIBLE | LVGS_COLLAPSED;

	SendDlgItemMessageW(
		m_hWnd,
		IDD_OPENWITH_PROGLIST,
		LVM_INSERTGROUP,
		-1,
		(LPARAM)&other
	);
}

void CVistaOpenAsDlg::_AddItem(wil::com_ptr<IAssocHandler> pItem, int index, bool fForceSelect)
{
	LVITEMW lvi = { 0 };
	wil::unique_cotaskmem_string pszDisplayName;
	lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
	lvi.iItem = index;
	if (m_fRecommended)
	{
		lvi.mask |= LVIF_GROUPID;
		lvi.iGroupId = (S_OK == pItem->IsRecommended()) ? I_RECOMMENDED : I_OTHER;
	}
	pItem->GetUIName(&pszDisplayName);
	lvi.pszText = pszDisplayName.get();
	lvi.cchTextMax = wcslen(lvi.pszText) + 1;

	// This is somewhat unsafe, but we're expecting that the item doesn't get
	// unallocated before the list view item is destroyed.
	lvi.lParam = (LPARAM)pItem.get();

	if (index == 0 || fForceSelect)
	{
		lvi.mask |= LVIF_STATE;
		lvi.stateMask = LVIS_SELECTED;
		lvi.state = LVIS_SELECTED;
	}

	wil::unique_cotaskmem_string pszIconPath = nullptr;
	int iIndex = 0;
	pItem->GetIconLocation(
		&pszIconPath,
		&iIndex
	);

	lvi.iImage = GetAppIconIndex(
		pszIconPath.get(), iIndex
	);

	SendDlgItemMessageW(
		m_hWnd,
		IDD_OPENWITH_PROGLIST,
		LVM_INSERTITEMW,
		NULL,
		(LPARAM)&lvi
	);

	wil::com_ptr_nothrow<IAssocHandlerWithCompanyName> pCompanyNameInfo = nullptr;
	HRESULT hr = pItem->QueryInterface(IID_PPV_ARGS(&pCompanyNameInfo));
	std::unique_ptr<WCHAR[]> pszCompanyName = nullptr;

	if (SUCCEEDED(hr))
	{
		// If we could get the IAssocHandlerWithCompanyName object, then we just
		// query that for the company name.
		wil::unique_cotaskmem_string pszTempBuffer = nullptr;
		hr = pCompanyNameInfo->GetCompany(&pszTempBuffer);

		if (pszTempBuffer)
		{
			int cchBuffer = lstrlenW(pszTempBuffer.get()) + 1;
			pszCompanyName = std::make_unique<WCHAR[]>(cchBuffer);
			wcscpy_s(pszCompanyName.get(), cchBuffer, pszTempBuffer.get());
		}
	}

	if (FAILED(hr))
	{
		// If we failed to get the IAssocHandlerWithCompanyName object, or failed to
		// query the company name from that, then we will attempt to query it from
		// the shell item properties instead.
		wil::unique_cotaskmem_string pszPath = nullptr;

		// BUGBUG: UWP applications report their display names (i.e. "Photos") instead
		// of their location when calling this function. Thus, they will fail this
		// procedure.
		pItem->GetName(&pszPath);

		wil::com_ptr<IShellItem2> psi = nullptr;
		hr = SHCreateItemFromParsingName(pszPath.get(), nullptr, IID_PPV_ARGS(&psi));

		if (SUCCEEDED(hr))
		{
			wil::unique_prop_variant pvar;
			psi->GetProperty(PKEY_Company, &pvar);

			if (pvar.vt == VT_LPWSTR && pvar.pwszVal)
			{
				int cchCompanyName = lstrlenW(pvar.pwszVal) + 1;
				pszCompanyName = std::make_unique<WCHAR[]>(cchCompanyName);
				memcpy(pszCompanyName.get(), pvar.pwszVal, cchCompanyName * sizeof(WCHAR));
			}
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
				m_hWnd, IDD_OPENWITH_PROGLIST,
				LVM_SETTILEINFO, 0,
				(LPARAM)&lvti
			);

			LVITEMW lvi = { 0 };
			lvi.pszText = pszCompanyName.get();
			lvi.iSubItem = 1;
			SendDlgItemMessageW(
				m_hWnd, IDD_OPENWITH_PROGLIST,
				LVM_SETITEMTEXTW, index,
				(LPARAM)&lvi
			);
		}
	}
}

CVistaOpenAsDlg::CVistaOpenAsDlg(LPCWSTR lpszPath, IMMERSIVE_OPENWITH_FLAGS flags, bool fUri, bool fPreregistered)
	: CBaseOpenAsDlg(lpszPath, flags, fUri, fPreregistered, IDD_OPENWITH, IDD_OPENWITH_WITHDESC, IDD_OPENWITH_PROTOCOL)
{

}
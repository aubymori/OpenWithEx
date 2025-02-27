#include "classicopenasdlg.h"

void CClassicOpenAsDlg::_InitProgList()
{
	LVCOLUMNW col = { 0 };
	col.mask = LVCF_SUBITEM | LVCF_WIDTH;
	col.iSubItem = 0;

	HWND hwndProgList = GetDlgItem(m_hWnd, IDD_OPENWITH_PROGLIST);
	RECT rc;
	GetClientRect(hwndProgList, &rc);
	col.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL) - 4 * GetSystemMetrics(SM_CXEDGE);

	SendDlgItemMessageW(
		m_hWnd, IDD_OPENWITH_PROGLIST,
		LVM_INSERTCOLUMNW, NULL,
		(LPARAM)&col
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

	WCHAR szFormat[200];
	WCHAR szTemp[MAX_PATH + 200];
	GetDlgItemTextW(m_hWnd, IDD_OPENWITH_TEXT, szFormat, 200);
	swprintf_s(szTemp, szFormat, m_fUri ? m_szPath : m_pszFileName);
	SetDlgItemTextW(m_hWnd, IDD_OPENWITH_TEXT, szTemp);

	GetDlgItemTextW(m_hWnd, IDD_OPENWITH_EXT, szFormat, 200);
	swprintf_s(szTemp, szFormat, m_szExtOrProtocol);
	SetDlgItemTextW(m_hWnd, IDD_OPENWITH_EXT, szTemp);
}

wil::com_ptr<IAssocHandler> CClassicOpenAsDlg::_GetSelectedItem()
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

void CClassicOpenAsDlg::_SelectItemByIndex(int index)
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

void CClassicOpenAsDlg::_SetupCategories()
{

}

void CClassicOpenAsDlg::_AddItem(wil::com_ptr<IAssocHandler> pItem, int index, bool fForceSelect)
{
	LVITEMW lvi = { 0 };
	wil::unique_cotaskmem_string pszDisplayName;
	lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
	lvi.iItem = index;
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
}

CClassicOpenAsDlg::CClassicOpenAsDlg(LPCWSTR lpszPath, IMMERSIVE_OPENWITH_FLAGS flags, bool fUri, bool fPreregistered)
	: CBaseOpenAsDlg(
		lpszPath, flags, fUri, fPreregistered,
		(g_style == OWXS_NT4) ? IDD_OPENWITH_NT4 : IDD_OPENWITH_2K,
		(g_style == OWXS_NT4) ? IDD_OPENWITH_WITHDESC_NT4 : IDD_OPENWITH_WITHDESC_2K,
		(g_style == OWXS_NT4) ? IDD_OPENWITH_PROTOCOL_NT4 : IDD_OPENWITH_PROTOCOL_2K)
{

}
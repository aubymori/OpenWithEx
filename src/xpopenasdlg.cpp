#include "xpopenasdlg.h"

void CXPOpenAsDlg::_InitProgList()
{
	HIMAGELIST himl = NULL;
	Shell_GetImageLists(nullptr, &himl);
	SendDlgItemMessageW(
		m_hWnd,
		IDD_OPENWITH_PROGLIST,
		TVM_SETIMAGELIST,
		TVSIL_NORMAL,
		(LPARAM)himl
	);
	SendDlgItemMessageW(
		m_hWnd,
		IDD_OPENWITH_PROGLIST,
		TVM_SETIMAGELIST,
		TVSIL_STATE,
		(LPARAM)himl
	);
}

wil::com_ptr<IAssocHandler> CXPOpenAsDlg::_GetSelectedItem()
{
	HTREEITEM hSelected = (HTREEITEM)SendDlgItemMessageW(
		m_hWnd, IDD_OPENWITH_PROGLIST,
		TVM_GETNEXTITEM, TVGN_CARET,
		NULL
	);
	if (!hSelected)
		return nullptr;

	TVITEMW tvi = { 0 };
	tvi.mask = TVIF_PARAM | TVIF_HANDLE;
	tvi.hItem = hSelected;

	SendDlgItemMessageW(
		m_hWnd, IDD_OPENWITH_PROGLIST,
		TVM_GETITEM, NULL,
		(LPARAM)&tvi
	);

	return (IAssocHandler *)tvi.lParam;
}

void CXPOpenAsDlg::_SelectItemByIndex(int index)
{
	HTREEITEM hItem = m_treeItems.at(index);
	if (hItem)
	{
		SetFocus(GetDlgItem(m_hWnd, IDD_OPENWITH_PROGLIST));
		SendDlgItemMessageW(
			m_hWnd, IDD_OPENWITH_PROGLIST,
			TVM_SELECTITEM, TVGN_CARET,
			(LPARAM)hItem
		);
	}
}

void CXPOpenAsDlg::_SetupCategories()
{
	TVITEMW recommended = { 0 };
	recommended.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_STATE;
	recommended.iImage = Shell_GetCachedImageIndexW(L"shell32.dll", 19, NULL);
	recommended.iSelectedImage = recommended.iImage;
	recommended.state = TVIS_EXPANDED;
	recommended.stateMask = TVIS_EXPANDED;
	WCHAR szRecommended[MAX_PATH] = { 0 };
	LoadStringW(g_hInst, IDS_RECOMMENDED_XP, szRecommended, MAX_PATH);
	recommended.pszText = szRecommended;
	recommended.cchTextMax = MAX_PATH;

	TVINSERTSTRUCTW insert = { 0 };
	insert.item = recommended;

	m_hRecommended = (HTREEITEM)SendDlgItemMessageW(
		m_hWnd,
		IDD_OPENWITH_PROGLIST,
		TVM_INSERTITEMW,
		NULL,
		(LPARAM)&insert
	);

	TVITEMW other = recommended;
	WCHAR szOther[MAX_PATH] = { 0 };
	LoadStringW(g_hInst, IDS_OTHER_XP, szOther, MAX_PATH);
	other.pszText = szOther;
	insert.item = other;
	m_hOther = (HTREEITEM)SendDlgItemMessageW(
		m_hWnd,
		IDD_OPENWITH_PROGLIST,
		TVM_INSERTITEMW,
		NULL,
		(LPARAM)&insert
	);
}

void CXPOpenAsDlg::_AddItem(wil::com_ptr<IAssocHandler> pItem, int index, bool fForceSelect)
{
	TVITEMW tvi = { 0 };
	wil::unique_cotaskmem_string pszDisplayName;
	tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	pItem->GetUIName(&pszDisplayName);
	tvi.pszText = pszDisplayName.get();
	tvi.cchTextMax = MAX_PATH;

	tvi.lParam = (LPARAM)pItem.get();

	LPWSTR pszPath = nullptr;
	int iIndex = 0;
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
	if (m_fRecommended)
	{
		insert.hParent = (S_OK == pItem->IsRecommended()) ? m_hRecommended : m_hOther;
	}

	m_treeItems.push_back((HTREEITEM)SendDlgItemMessageW(
		m_hWnd,
		IDD_OPENWITH_PROGLIST,
		TVM_INSERTITEMW,
		NULL,
		(LPARAM)&insert
	));
}

CXPOpenAsDlg::CXPOpenAsDlg(LPCWSTR lpszPath, IMMERSIVE_OPENWITH_FLAGS flags, bool fUri, bool fPreregistered)
	: CBaseOpenAsDlg(lpszPath, flags, fUri, fPreregistered, IDD_OPENWITH_XP, IDD_OPENWITH_WITHDESC_XP, IDD_OPENWITH_PROTOCOL_XP)
{

}
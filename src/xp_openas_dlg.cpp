#include "xp_openas_dlg.h"

CXPOpenAsDlg::CXPOpenAsDlg(COpenWithExUI *pOpenWithUI,
                           OPENAS_DLG_TYPE type,
                           IMMERSIVE_OPENWITH_FLAGS flags)
    : COpenAsDlg(
        DLG_OPENAS_XP,
        pOpenWithUI,
        type,
        flags)
    , _cItems(0)
    , _fHasRecommended(false)
    , _hitemRecommended(NULL)
    , _hitemOther(NULL)
{

}

void CXPOpenAsDlg::OnInitDialog()
{
    _hwndAppList = GetDlgItem(_hwnd, IDD_APPLIST);

    HIMAGELIST himl;
    Shell_GetImageLists(nullptr, &himl);
    TreeView_SetImageList(_hwndAppList, himl, TVSIL_NORMAL);
    TreeView_SetImageList(_hwndAppList, himl, TVSIL_STATE);

    COpenAsDlg::OnInitDialog();
}

void CXPOpenAsDlg::AddItem(IAssocHandler *pah)
{
    wil::unique_cotaskmem_string spsz;
    if (FAILED(pah->GetUIName(&spsz)))
        return;

    TVINSERTSTRUCTW tvi = { 0 };
    tvi.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.item.pszText = spsz.get();
    tvi.item.cchTextMax = wcslen(spsz.get()) + 1;
    tvi.item.lParam = (LPARAM)pah;

    LPWSTR pszPath;
    int iIndex;
    if (FAILED(pah->GetIconLocation(&pszPath, &iIndex)))
        return;

    tvi.item.iImage = GetAppIconIndex(pszPath, iIndex);
    tvi.item.iSelectedImage = tvi.item.iImage;

    if (_fHasRecommended)
    {
        tvi.hParent = (S_OK == pah->IsRecommended()) ? _hitemRecommended : _hitemOther;
    }

    _items.push_back(TreeView_InsertItem(_hwndAppList, &tvi));
}

void CXPOpenAsDlg::SetupCategories()
{
    _fHasRecommended = true;

    WCHAR szBuffer[MAX_PATH];
    LoadStringW(g_hinst, IDS_OPENWITH_RECOMMENDED_XP, szBuffer, ARRAYSIZE(szBuffer));

    TVINSERTSTRUCTW tvi = { 0 };
    tvi.item.mask           = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_STATE;
    tvi.item.iImage         = Shell_GetCachedImageIndexW(L"shell32.dll", 19, NULL);
    tvi.item.iSelectedImage = tvi.item.iImage;
    tvi.item.state          = TVIS_EXPANDED;
    tvi.item.stateMask      = TVIS_EXPANDED;
    tvi.item.pszText        = szBuffer;
    tvi.item.cchTextMax     = ARRAYSIZE(szBuffer);
    _hitemRecommended = TreeView_InsertItem(_hwndAppList, &tvi);

    LoadStringW(g_hinst, IDS_OPENWITH_OTHERS_XP, szBuffer, ARRAYSIZE(szBuffer));
    _hitemOther = TreeView_InsertItem(_hwndAppList, &tvi);
}

IAssocHandler *CXPOpenAsDlg::GetSelectedItem()
{
    HTREEITEM hitemSelected = TreeView_GetNextItem(_hwndAppList, NULL, TVGN_CARET);
    if (!hitemSelected)
        return nullptr;

    TVITEMW tvi = { 0 };
    tvi.mask = TVIF_PARAM | TVIF_HANDLE;
    tvi.hItem = hitemSelected;

    TreeView_GetItem(_hwndAppList, &tvi);
    return (IAssocHandler *)tvi.lParam;
}

void CXPOpenAsDlg::SelectItemByIndex(int i)
{
    HTREEITEM hitem = _items.at(i);
    if (hitem)
    {
        SetFocus(_hwndAppList);
        TreeView_SelectItem(_hwndAppList, hitem);
    }
}
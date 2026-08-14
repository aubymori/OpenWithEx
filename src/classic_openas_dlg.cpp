#include "classic_openas_dlg.h"
#include "interfaces.h"

CClassicOpenAsDlg::CClassicOpenAsDlg(COpenWithExUI *pOpenWithUI,
                                     OPENAS_DLG_TYPE type,
                                     IMMERSIVE_OPENWITH_FLAGS flags)
    : COpenAsDlg(
        (g_style == OPENWITHEX_STYLE_NT4) ? DLG_OPENAS_NT4 : DLG_OPENAS_2K,
        pOpenWithUI,
        type,
        flags)
    , _cItems(0)
{

}

void CClassicOpenAsDlg::OnInitDialog()
{
    _hwndAppList = GetDlgItem(_hwnd, IDD_APPLIST);

    LVCOLUMNW lvcol = { 0 };
    lvcol.mask = LVCF_SUBITEM | LVCF_WIDTH;
    lvcol.iSubItem = 0;
    RECT rc;
    GetClientRect(_hwndAppList, &rc);
    lvcol.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL) - 4 * GetSystemMetrics(SM_CXEDGE);
    ListView_InsertColumn(_hwndAppList, 0, &lvcol);

    HIMAGELIST himl;
    Shell_GetImageLists(nullptr, &himl);
    ListView_SetImageList(_hwndAppList, himl, LVSIL_SMALL);

    wil::unique_cotaskmem_string spszFileName;
    HRESULT hr = _pOpenWithUI->GetItemName(
        SIGDN_NORMALDISPLAY,
        &spszFileName);
    if (FAILED(hr) && (_flags & IMMERSIVE_OPENWITH_PROTOCOL))
    {
        hr = _pOpenWithUI->GetItemName(SIGDN_URL, &spszFileName);
    }
    if (FAILED(hr))
    {
        EndDialog(_hwnd, IDCANCEL);
        return;
    }

    wil::unique_cotaskmem_string spszTypeID;
    if (FAILED(_pOpenWithUI->GetTypeID(&spszTypeID)))
    {
        EndDialog(_hwnd, IDCANCEL);
        return;
    }

    WCHAR szFormat[200];
    WCHAR szTemp[MAX_PATH + 200];
    GetDlgItemTextW(_hwnd, IDD_TEXT, szFormat, 200);
    swprintf_s(szTemp, szFormat, spszFileName.get());
    SetDlgItemTextW(_hwnd, IDD_TEXT, szTemp);

    GetDlgItemTextW(_hwnd, IDD_DESCRIPTIONTEXT, szFormat, 200);
    swprintf_s(szTemp, szFormat, spszTypeID.get());
    SetDlgItemTextW(_hwnd, IDD_DESCRIPTIONTEXT, szTemp);

    COpenAsDlg::OnInitDialog();
}

void CClassicOpenAsDlg::AddItem(IAssocHandler *pah)
{
    LVITEMW lvi;
    lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    lvi.iItem = _cItems++;
    lvi.iSubItem = 0;

    wil::unique_cotaskmem_string spszItemName;
    if (FAILED(pah->GetUIName(&spszItemName)))
        return;
    lvi.pszText = spszItemName.get();
    lvi.cchTextMax = (int)wcslen(spszItemName.get()) + 1;

    lvi.lParam = (LPARAM)pah;

    if (_cItems == 1)
    {
        lvi.mask |= LVIF_STATE;
        lvi.stateMask = LVIS_SELECTED;
        lvi.state = LVIS_SELECTED;
    }

    wil::unique_cotaskmem_string spszIconPath;
    int iIconIndex;
    pah->GetIconLocation(&spszIconPath, &iIconIndex);

    lvi.iImage = GetAppIconIndex(spszIconPath.get(), iIconIndex);

    ListView_InsertItem(_hwndAppList, &lvi);
}

void CClassicOpenAsDlg::SetupCategories()
{
    
}

IAssocHandler *CClassicOpenAsDlg::GetSelectedItem()
{
    int iSelected = ListView_GetNextItem(_hwndAppList, -1, LVNI_SELECTED);
    if (iSelected == -1)
        return nullptr;

    LVITEMW lvi;
    lvi.iItem = iSelected;
    lvi.mask = LVIF_PARAM;
    ListView_GetItem(_hwndAppList, &lvi);
    return (IAssocHandler *)lvi.lParam;
}

void CClassicOpenAsDlg::SelectItemByIndex(int i)
{
    // Focus list view and select item
    LVITEMW lvi;
    lvi.iItem = i;
    lvi.mask = LVIF_STATE;
    lvi.stateMask = LVIS_SELECTED;
    lvi.state = LVIS_SELECTED;
    SetFocus(_hwndAppList);
    ListView_SetItem(_hwndAppList, &lvi);

    // Ensure the newly selected item is visible
    ListView_EnsureVisible(_hwndAppList, i, FALSE);
}
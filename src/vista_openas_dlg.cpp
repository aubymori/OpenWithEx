#include "vista_openas_dlg.h"
#include "interfaces.h"

#define I_RECOMMENDED   0
#define I_OTHERS        1

CVistaOpenAsDlg::CVistaOpenAsDlg(COpenWithExUI *pOpenWithUI,
                                 OPENAS_DLG_TYPE type,
                                 IMMERSIVE_OPENWITH_FLAGS flags)
    : COpenAsDlg(
        // TODO: Get Vista variants for other locales.
        (g_style == OPENWITHEX_STYLE_VISTA) ? DLG_OPENAS_VISTA : DLG_OPENAS,
        pOpenWithUI,
        type,
        flags)
    , _cItems(0)
    , _fHasRecommended(false)
{

}

void CVistaOpenAsDlg::OnInitDialog()
{
    _hwndAppList = GetDlgItem(_hwnd, IDD_APPLIST);

    SetWindowTheme(_hwndAppList, L"Explorer", nullptr);

    LVCOLUMNW lvcol = { 0 };
    lvcol.mask = LVCF_SUBITEM;
    lvcol.iSubItem = 0;
    ListView_InsertColumn(_hwndAppList, 0, &lvcol);

    lvcol.iSubItem = 1;
    ListView_InsertColumn(_hwndAppList, 0, &lvcol);

    ListView_SetView(_hwndAppList, LV_VIEW_TILE);

    HIMAGELIST himl;
    Shell_GetImageLists(&himl, nullptr);
    ListView_SetImageList(_hwndAppList, himl, LVSIL_NORMAL);

    COpenAsDlg::OnInitDialog();
}

void CVistaOpenAsDlg::AddItem(IAssocHandler *pah)
{
    LVITEMW lvi;
    lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    lvi.iItem = _cItems++;
    lvi.iSubItem = 0;
    if (_fHasRecommended)
    {
        lvi.mask |= LVIF_GROUPID;
        lvi.iGroupId = (S_OK == pah->IsRecommended())
            ? I_RECOMMENDED
            : I_OTHERS;
    }

    wil::unique_cotaskmem_string spszItemName;
    pah->GetUIName(&spszItemName);
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

    wil::unique_cotaskmem_string spszCompanyName;

    ComPtr<IAssocHandlerWithCompanyName> spAssocHandler;
    HRESULT hr = pah->QueryInterface(IID_PPV_ARGS(&spAssocHandler));
    if (SUCCEEDED(hr))
    {
        hr = spAssocHandler->GetCompany(&spszCompanyName);
    }

    if (FAILED(hr))
    {
        ComPtr<IShellItem2> spItem;
        hr = _pOpenWithUI->GetItem(&spItem);
        if (SUCCEEDED(hr))
        {
            hr = spItem->GetString(PKEY_Company, &spszCompanyName);
        }
    }

    if (SUCCEEDED(hr) && spszCompanyName)
    {
        LVTILEINFO lvti = { sizeof(lvti) };
        lvti.iItem = lvi.iItem;
        lvti.cColumns = 1;
        UINT uCols[] = { 1 };
        lvti.puColumns = uCols;
        ListView_SetTileInfo(_hwndAppList, &lvti);
        ListView_SetItemText(_hwndAppList, lvi.iItem, 1, spszCompanyName.get());
    }
}

void CVistaOpenAsDlg::SetupCategories()
{
    _fHasRecommended = true;
    ListView_EnableGroupView(_hwndAppList, TRUE);

    WCHAR szHeader[MAX_PATH];
    LoadStringW(g_hinst, IDS_OPENWITH_RECOMMENDED, szHeader, ARRAYSIZE(szHeader));

    LVGROUP lvgrp;
    lvgrp.cbSize = sizeof(lvgrp);
    lvgrp.mask = LVGF_HEADER | LVGF_GROUPID;
    lvgrp.pszHeader = szHeader;
    lvgrp.iGroupId = I_RECOMMENDED;
    ListView_InsertGroup(_hwndAppList, -1, &lvgrp);

    LoadStringW(g_hinst, IDS_OPENWITH_OTHERS, szHeader, ARRAYSIZE(szHeader));
    lvgrp.mask = LVGF_HEADER | LVGF_GROUPID | LVGF_STATE;
    lvgrp.stateMask = LVGS_COLLAPSIBLE | LVGS_COLLAPSED;
    lvgrp.state = LVGS_COLLAPSIBLE | LVGS_COLLAPSED;
    lvgrp.iGroupId = I_OTHERS;
    ListView_InsertGroup(_hwndAppList, -1, &lvgrp);
}

IAssocHandler *CVistaOpenAsDlg::GetSelectedItem()
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

void CVistaOpenAsDlg::SelectItemByIndex(int i)
{
    // Focus list view and select item
    LVITEMW lvi;
    lvi.iItem = i;
    lvi.mask = LVIF_STATE;
    lvi.stateMask = LVIS_SELECTED;
    lvi.state = LVIS_SELECTED;
    SetFocus(_hwndAppList);
    ListView_SetItem(_hwndAppList, &lvi);

    // Expand Other Programs group if this item is in there
    lvi.mask = LVIF_GROUPID;
    ListView_GetItem(_hwndAppList, &lvi);
    if (lvi.iGroupId == I_OTHERS)
    {
        LVGROUP lvgrp;
        lvgrp.cbSize = sizeof(lvgrp);
        lvgrp.mask = LVGF_STATE;
        lvgrp.stateMask = LVGS_COLLAPSED;
        lvgrp.state = 0;
        ListView_SetGroupInfo(_hwndAppList, I_OTHERS, &lvgrp);
    }

    // Ensure the newly selected item is visible
    ListView_EnsureVisible(_hwndAppList, i, FALSE);
}
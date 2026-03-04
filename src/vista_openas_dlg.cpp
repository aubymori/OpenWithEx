#include "vista_openas_dlg.h"

CVistaOpenAsDlg::CVistaOpenAsDlg(COpenWithExUI *pOpenWithUI,
                                 OPENAS_DLG_TYPE type,
                                 IMMERSIVE_OPENWITH_FLAGS flags)
    : COpenAsDlg(DLG_OPENAS, pOpenWithUI, type, flags)
{

}

void CVistaOpenAsDlg::OnInitDialog()
{
    COpenAsDlg::OnInitDialog();

    SetWindowTheme(_hwndAppList, L"Explorer", nullptr);
    ListView_SetView(_hwndAppList, LV_VIEW_TILE);

    HIMAGELIST himl;
    Shell_GetImageLists(&himl, nullptr);
    ListView_SetImageList(_hwndAppList, himl, LVSIL_NORMAL);

    LVCOLUMNW lvcol = { 0 };
    lvcol.mask = LVCF_SUBITEM;
    lvcol.iSubItem = 0;
    ListView_InsertColumn(_hwndAppList, 0, &lvcol);

    lvcol.iSubItem = 1;
    ListView_InsertColumn(_hwndAppList, 0, &lvcol);
}

void CVistaOpenAsDlg::AddItem(IAssocHandler *pah)
{

}

void CVistaOpenAsDlg::SetupCategories()
{

}

IAssocHandler *CVistaOpenAsDlg::GetSelectedItem()
{
    return nullptr;
}
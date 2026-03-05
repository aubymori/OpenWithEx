#include "noopen_dlg.h"

INT_PTR CNoOpenDlg::v_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            WCHAR szFormat[MAX_PATH];
            WCHAR szTemp[MAX_PATH];

            WCHAR szDescription[MAX_PATH];
            wil::unique_cotaskmem_string spszTypeID, spszFileName;
            if (FAILED(_pOpenWithUI->GetDescription(szDescription, ARRAYSIZE(szDescription)))
            || FAILED(_pOpenWithUI->GetTypeID(&spszTypeID))
            || FAILED(_pOpenWithUI->GetItemName(SIGDN_DESKTOPABSOLUTEPARSING, &spszFileName)))
            {
                EndDialog(hwnd, IDCANCEL);
                return TRUE;
            }

            GetDlgItemTextW(hwnd, IDD_TEXT1, szFormat, ARRAYSIZE(szTemp));
            swprintf_s(szTemp, szFormat, szDescription, spszTypeID.get());
            SetDlgItemTextW(hwnd, IDD_TEXT1, szTemp);

            if (_pszNoOpenMsg[0])
                SetDlgItemTextW(hwnd, IDD_TEXT2, _pszNoOpenMsg);

            SHFILEINFOW sfi;
            HICON hIcon;
            if (SHGetFileInfoW(spszFileName.get(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON)
                && sfi.hIcon)
            {
                hIcon = sfi.hIcon;
            }
            else
            {
                HIMAGELIST himl;
                Shell_GetImageLists(&himl, nullptr);
                hIcon = ImageList_ExtractIcon(NULL, himl, 0); // II_DOCNOASSOC
            }

            hIcon = (HICON)SendDlgItemMessageW(hwnd, IDD_ICON, STM_SETICON, (WPARAM)hIcon, 0);
            if (hIcon)
            {
                DestroyIcon(hIcon);
            }
            return TRUE;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDD_OPENWITH:
                case IDCANCEL:
                    EndDialog(hwnd, LOWORD(wParam));
                    break;
            }
            return TRUE;
        default:
            return FALSE;
    }
}

CNoOpenDlg::CNoOpenDlg(COpenWithExUI *pOpenWithUI, LPWSTR pszNoOpenMsg)
    : CDialog((g_style >= OPENWITHEX_STYLE_VISTA) ? DLG_NOOPEN_VISTA : DLG_NOOPEN)
    , _pOpenWithUI(pOpenWithUI)
    , _pszNoOpenMsg(pszNoOpenMsg)
{

}
#pragma once
#include "dialog.h"
#include "openwithex_ui.h"

class CNoOpenDlg : public CDialog
{
private:
    COpenWithExUI *_pOpenWithUI;
    LPWSTR _pszNoOpenMsg;

    INT_PTR v_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

public:
    CNoOpenDlg(COpenWithExUI *pOpenWithUI, LPWSTR pszNoOpenMsg);
};
#pragma once
#include "dialog.h"
#include "openwithex_ui.h"

class CInternetOpenAsDlg : public CDialog
{
private:
	COpenWithExUI *_pOpenWithUI;

	INT_PTR v_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	CInternetOpenAsDlg(COpenWithExUI *pOpenWithUI);
};
#pragma once

#include "openwithex.h"
#include "impdialog.h"

class CCantOpenDlg : public CImpDialog
{
private:
	WCHAR  m_szPath[MAX_PATH];
	LPWSTR m_pszFileName;

	INT_PTR CALLBACK v_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	CCantOpenDlg(LPCWSTR lpszPath);
};
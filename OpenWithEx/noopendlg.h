#include "openwithex.h"
#include "impdialog.h"

#ifndef _noopendlg_h_
#define _noopendlg_h_

class CNoOpenDlg : public CImpDialog
{
private:
	WCHAR  m_szPath[MAX_PATH];
	LPWSTR m_pszExtension;

	INT_PTR CALLBACK v_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	CNoOpenDlg(LPCWSTR lpszPath);
};

#endif
#include "dialog.h"

INT_PTR CALLBACK CDialog::s_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			CDialog *pThis = (CDialog *)lParam;
			if (pThis)
			{
				pThis->_hwnd = hwnd;
				SetWindowLongPtrW(
					hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
			}
			break;
		}
		// For some reason, initializing through the server doesn't activate the window.
		// Let's activate it manually when it's shown.
		case WM_WINDOWPOSCHANGED:
		{
			CDialog *pThis = (CDialog *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
			if (pThis && ((LPWINDOWPOS)lParam)->flags & SWP_SHOWWINDOW && !pThis->_fShown)
			{
				pThis->_fShown = true;
				SetForegroundWindow(hwnd);
				if (GetForegroundWindow() != hwnd)
				{
					SwitchToThisWindow(hwnd, TRUE);
					Sleep(2);
					SetForegroundWindow(hwnd);
				}
				SetActiveWindow(hwnd);
			}
			break;
		}
	}

	CDialog *pThis = (CDialog *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
	if (pThis)
	{
		return pThis->v_DlgProc(hwnd, uMsg, wParam, lParam);
	}
	return FALSE;
}

CDialog::CDialog(UINT uDlgID)
	: _uDlgID(uDlgID)
{

}

INT_PTR CDialog::ShowDialog(HWND hwndParent)
{
	return DialogBoxParamW(
		g_hinst,
		MAKEINTRESOURCEW(_uDlgID),
		hwndParent,
		s_DlgProc,
		(LPARAM)this);
}
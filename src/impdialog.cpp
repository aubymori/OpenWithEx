#include "impdialog.h"

INT_PTR CALLBACK CImpDialog::s_DlgProc(
	HWND   hWnd,
	UINT   uMsg,
	WPARAM wParam,
	LPARAM lParam
)
{
	if (uMsg == WM_INITDIALOG)
	{
		CImpDialog *pThis = (CImpDialog *)lParam;
		if (pThis)
		{
			pThis->m_hWnd = hWnd;
			SetWindowLongPtrW(
				hWnd,
				GWLP_USERDATA,
				(LONG_PTR)pThis
			);
		}
	}
	// For some reason, initializing through the server doesn't activate the window.
	// Let's activate it manually when it's shown.
	else if (uMsg == WM_WINDOWPOSCHANGED)
	{	
		CImpDialog *pThis = (CImpDialog *)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
		if (pThis && ((LPWINDOWPOS)lParam)->flags & SWP_SHOWWINDOW && !pThis->m_fShown)
		{
			pThis->m_fShown = true;
			SetForegroundWindow(hWnd);
			if (GetForegroundWindow() != hWnd)
			{
				SwitchToThisWindow(hWnd, TRUE);
				Sleep(2);
				SetForegroundWindow(hWnd);
			}
			SetActiveWindow(hWnd);
		}
	}

	CImpDialog *pThis = (CImpDialog *)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
	if (pThis)
	{
		return pThis->v_DlgProc(hWnd, uMsg, wParam, lParam);
	}
	return FALSE;
}

CImpDialog::CImpDialog(HINSTANCE hInst, UINT uDlgId)
	: m_hWnd(NULL)
	, m_hInst(hInst)
	, m_uDlgId(uDlgId)
{

}

INT_PTR CImpDialog::ShowDialog(HWND hWndParent)
{
	return DialogBoxParamW(
		m_hInst,
		MAKEINTRESOURCEW(m_uDlgId),
		hWndParent,
		s_DlgProc,
		(LPARAM)this
	);
}
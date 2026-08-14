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

/* Loads a shell32 icon and gives IDD_ICON that icon. */
void CDialog::SetShellIcon(int iIconID)
{
	static HMODULE hmodShell = GetModuleHandleW(L"shell32.dll");

	int cxIcon = _GetSystemMetrics(SM_CXICON);
	int cyIcon = _GetSystemMetrics(SM_CYICON);

	HICON hIcon = (HICON)LoadImageW(
		hmodShell, MAKEINTRESOURCEW(iIconID), IMAGE_ICON,
		cxIcon, cyIcon, LR_DEFAULTCOLOR);
	if (hIcon)
	{
		SendDlgItemMessageW(
			_hwnd,
			IDD_ICON,
			STM_SETICON,
			(WPARAM)hIcon,
			0);
	}
}

int CDialog::_GetSystemMetrics(int nIndex)
{
	static HMODULE hmodUser = GetModuleHandleW(L"user32.dll");
	using GetSystemMetricsForDpi_t = decltype(&GetSystemMetricsForDpi);
	static GetSystemMetricsForDpi_t pfnGetSystemMetricsForDpi
		= (GetSystemMetricsForDpi_t)GetProcAddress(hmodUser, "GetSystemMetricsForDpi");
	if (pfnGetSystemMetricsForDpi)
	{
		HDC hdc = GetDC(_hwnd);
		int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
		ReleaseDC(_hwnd, hdc);

		return pfnGetSystemMetricsForDpi(nIndex, dpi);
	}
	else
	{
		return GetSystemMetrics(nIndex);
	}
}

CDialog::CDialog(UINT uDlgID)
	: _fShown(false)
	, _hwnd(NULL)
	, _uDlgID(uDlgID)
{
	
}

INT_PTR CDialog::ShowDialog(HWND hwndParent)
{
	if (hwndParent && !IsWindow(hwndParent))
	{
		hwndParent = NULL;
	}

	return DialogBoxParamW(
		g_hinst,
		MAKEINTRESOURCEW(_uDlgID),
		hwndParent,
		s_DlgProc,
		(LPARAM)this);
}
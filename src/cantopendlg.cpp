#include "cantopendlg.h"
#include <shlwapi.h>

INT_PTR CALLBACK CCantOpenDlg::v_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			/* Set icon */
			HMODULE hShell32 = LoadLibraryW(L"shell32.dll");
			HICON hiOpenWith = LoadIconW(hShell32, MAKEINTRESOURCEW(134));
			SendDlgItemMessageW(
				hWnd,
				IDD_CANTOPEN_ICON,
				STM_SETICON,
				(WPARAM)hiOpenWith,
				NULL
			);

			/* Set path text */
			SetDlgItemTextW(
				hWnd,
				IDD_CANTOPEN_FILE,
				m_pszFileName
			);

			/* Check web radio button by default */
			CheckRadioButton(
				hWnd,
				IDD_CANTOPEN_USEWEB,
				IDD_CANTOPEN_OPENWITH,
				IDD_CANTOPEN_USEWEB
			);

			return TRUE;
		}
		case WM_CLOSE:
			EndDialog(hWnd, IDCANCEL);
			return TRUE;
		case WM_COMMAND:
			switch (wParam)
			{
				case IDOK:
					if (BST_CHECKED == SendDlgItemMessageW(
						hWnd,
						IDD_CANTOPEN_USEWEB,
						BM_GETCHECK,
						0, 0
					))
					{
						EndDialog(hWnd, IDD_CANTOPEN_USEWEB);
					}
					else if (BST_CHECKED == SendDlgItemMessageW(
						hWnd,
						IDD_CANTOPEN_OPENWITH,
						BM_GETCHECK,
						0, 0
					))
					{
						EndDialog(hWnd, IDD_CANTOPEN_OPENWITH);
					}
					else
					{
						EndDialog(hWnd, IDCANCEL);
					}
					break;
				case IDCANCEL:
					EndDialog(hWnd, IDCANCEL);
					break;
			}
			break;
	}

	return FALSE;
}

CCantOpenDlg::CCantOpenDlg(LPCWSTR lpszPath)
	: CImpDialog(g_hInst, IDD_CANTOPEN)
{
	wcscpy_s(m_szPath, lpszPath);
	m_pszFileName = PathFindFileNameW(m_szPath);
}
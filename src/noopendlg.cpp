#include "noopendlg.h"
#include <shlwapi.h>
#include <stdio.h>
#include <commctrl.h>
#include <shlobj.h>

INT_PTR CALLBACK CNoOpenDlg::v_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
		   /* Load friendly type name for display */
			HKEY hk = NULL;
			GetExtensionRegKey(m_pszExtension, &hk);
			WCHAR szIndirectString[MAX_PATH + 200] = { 0 };
			DWORD dwIndirectStringSize = sizeof(szIndirectString);
			RegQueryValueExW(
				hk,
				L"FriendlyTypeName",
				NULL,
				NULL,
				(LPBYTE)szIndirectString,
				&dwIndirectStringSize
			);
			RegCloseKey(hk);

			WCHAR szFriendlyTypeName[MAX_PATH] = { 0 };
			if (*szIndirectString)
			{
				if (*szIndirectString == L'@')
				{
					SHLoadIndirectString(
						szIndirectString,
						szFriendlyTypeName,
						MAX_PATH,
						nullptr
					);
				}
				else
				{
					wcscpy_s(szFriendlyTypeName, szIndirectString);
				}
			}

			/* Set up text format */
			WCHAR szFormat[MAX_PATH + 200] = { 0 };
			GetDlgItemTextW(hWnd, IDD_NOOPEN_LABEL, szFormat, ARRAYSIZE(szFormat));

			WCHAR szMessage[MAX_PATH + 200] = { 0 };
			swprintf_s(szMessage, szFormat, szFriendlyTypeName, m_pszExtension);

			SetDlgItemTextW(hWnd, IDD_NOOPEN_LABEL, szMessage);

			/* Set the icon */
			SHFILEINFOW sfi = { 0 };
			SHGetFileInfoW(
				m_szPath,
				NULL,
				&sfi,
				sizeof(SHFILEINFOW),
				SHGFI_ICON
			);

			HIMAGELIST himl = NULL;
			Shell_GetImageLists(&himl, nullptr);

			HICON hIcon = ImageList_GetIcon(himl, sfi.iIcon, NULL);
			if (hIcon)
			{
				SendDlgItemMessageW(
					hWnd,
					IDD_NOOPEN_ICON,
					STM_SETICON,
					(WPARAM)hIcon,
					NULL
				);
			}
			return TRUE;
		}
		case WM_CLOSE:
			EndDialog(hWnd, IDCANCEL);
			break;
		case WM_COMMAND:
			switch (wParam)
			{
				case IDD_NOOPEN_OPENWITH:
				case IDCANCEL:
					EndDialog(hWnd, wParam);
					break;
			}
			break;
	}

	return FALSE;
}

CNoOpenDlg::CNoOpenDlg(LPCWSTR lpszPath)
	: CImpDialog(g_hInst, IDD_NOOPEN)
{
	wcscpy_s(m_szPath, lpszPath);
	m_pszExtension = PathFindExtensionW(m_szPath);
}
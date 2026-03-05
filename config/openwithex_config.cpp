#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include "openwithex.h"
#include "resource.h"

HKEY g_hKey = NULL;

#define WIDE(str) (L"" str)

INT_PTR CALLBACK ConfigDlgProc(
	HWND   hWnd,
	UINT   uMsg,
	WPARAM wParam,
	LPARAM lParam
)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			EnableWindow(GetDlgItem(hWnd, IDD_APPLY), FALSE);

			HWND hwndCombo = GetDlgItem(hWnd, IDD_STYLEBOX);
			ComboBox_AddString(hwndCombo, L"Windows 7");
			ComboBox_AddString(hwndCombo, L"Windows Vista");
			ComboBox_AddString(hwndCombo, L"Windows XP");
			ComboBox_AddString(hwndCombo, L"Windows 2000");
			ComboBox_AddString(hwndCombo, L"Windows 95/98/NT 4.0");

			WCHAR szFormat[256], szBuffer[256];
			GetDlgItemTextW(hWnd, IDD_VERSION, szFormat, ARRAYSIZE(szFormat));
			swprintf_s(szBuffer, szFormat, WIDE(VER_STRING));
			SetDlgItemTextW(hWnd, IDD_VERSION, szBuffer);
			EnableWindow(GetDlgItem(hWnd, IDD_VERSION), FALSE);

			OPENWITHEX_STYLE style;
			GetUserStyle(&style);
			ComboBox_SetCurSel(hwndCombo, style);

			return TRUE;
		}
		case WM_CLOSE:
			EndDialog(hWnd, IDCANCEL);
			return TRUE;
		case WM_COMMAND:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				EnableWindow(GetDlgItem(hWnd, IDD_APPLY), TRUE);
				break;
			}
			switch (LOWORD(wParam))
			{
				case IDOK:
				case IDD_APPLY:
				{
					DWORD dw = ComboBox_GetCurSel(GetDlgItem(hWnd, IDD_STYLEBOX));
					RegSetValueExW(
						g_hKey,
						REGSTR_VAL_STYLE,
						NULL,
						REG_DWORD,
						(LPBYTE)&dw,
						sizeof(DWORD));
					dw = VER_DWORD;
					RegSetValueExW(
						g_hKey,
						REGSTR_VAL_SETTINGSVER,
						NULL,
						REG_DWORD,
						(LPBYTE)&dw,
						sizeof(DWORD));
					if (LOWORD(wParam) == IDOK)
						EndDialog(hWnd, IDOK);
					EnableWindow(GetDlgItem(hWnd, IDD_APPLY), FALSE);
					break;
				}
				case IDCANCEL:
					EndDialog(hWnd, IDCANCEL);
					break;
			}
			return TRUE;
	}

	return FALSE;
}

int WINAPI wWinMain(
	_In_     HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_     LPWSTR    lpCmdLine,
	_In_     int       nCmdShow
)
{
	if (ERROR_SUCCESS != RegCreateKeyExW(
		HKEY_CURRENT_USER,
		REGSTR_PATH_OPENWITHEX,
		NULL,
		nullptr,
		NULL,
		KEY_READ | KEY_WRITE,
		nullptr,
		&g_hKey,
		nullptr
	))
	{
		MessageBoxW(NULL, L"Failed to open the OpenWithEx registry key", L"OpenWithEx Configuration", MB_ICONERROR);
		return 1;
	}

	DialogBoxParamW(
		hInstance, MAKEINTRESOURCEW(IDD_OPENWITHEXCONFIG),
		NULL, ConfigDlgProc, NULL
	);

	RegCloseKey(g_hKey);
	return 0;
}
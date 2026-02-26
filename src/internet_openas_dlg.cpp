#include "internet_openas_dlg.h"

#define IDH_CANNOTOPEN_USEWEB           3002
#define IDH_CANNOTOPEN_SELECTLIST       3003

static const DWORD aOpenAsDownloadHelpIDs[] = {
	IDD_ICON,             (DWORD)-1,
	IDD_FILE_TEXT,        (DWORD)-1,
	IDD_WEBAUTOLOOKUP,    IDH_CANNOTOPEN_USEWEB,
	IDD_OPENWITHLIST,     IDH_CANNOTOPEN_SELECTLIST,
	0, 0
};

INT_PTR CInternetOpenAsDlg::v_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			SetShellIcon(134);
			CheckDlgButton(hwnd, IDD_WEBAUTOLOOKUP, BST_CHECKED);

			wil::unique_cotaskmem_string spszFileName;
			if (FAILED(_pOpenWithUI->GetItemName(SIGDN_NORMALDISPLAY, &spszFileName)))
			{
				EndDialog(hwnd, IDCANCEL);
				return TRUE;
			}

			SetDlgItemTextW(hwnd, IDD_FILE_TEXT, spszFileName.get());
			return TRUE;
		}
		case WM_COMMAND:
		{
			int idEnd = IDCANCEL;
			switch (LOWORD(wParam))
			{
				case IDOK:
					idEnd = (BST_CHECKED == IsDlgButtonChecked(hwnd, IDD_WEBAUTOLOOKUP))
						? IDD_WEBAUTOLOOKUP
						: IDOK;
					if (idEnd == IDD_WEBAUTOLOOKUP)
					{
						_pOpenWithUI->OpenDownloadURL(hwnd);
					}
					// fallthrough
				case IDCANCEL:
					EndDialog(hwnd, idEnd);
					break;
			}
			return TRUE;
		}
		case WM_HELP:
			if (g_style == OPENWITHEX_STYLE_XP)
				WinHelpW(
					(HWND)((LPHELPINFO)lParam)->hItemHandle,
					nullptr, HELP_WM_HELP,
					(ULONG_PTR)aOpenAsDownloadHelpIDs);
			return TRUE;
		case WM_CONTEXTMENU:
			if (g_style == OPENWITHEX_STYLE_XP)
				WinHelpW(
					(HWND)wParam, nullptr,
					HELP_CONTEXTMENU,
					(ULONG_PTR)aOpenAsDownloadHelpIDs);
			return TRUE;
		default:
			return FALSE;
	}
}

CInternetOpenAsDlg::CInternetOpenAsDlg(COpenWithExUI *pOpenWithUI)
	: CDialog((g_style == OPENWITHEX_STYLE_VISTA) ? DLG_OPENAS_DOWNALOAD : DLG_OPENAS_DOWNALOAD_XP)
	, _pOpenWithUI(pOpenWithUI)
{

}
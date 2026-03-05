#include "openas_dlg.h"

// static
int COpenAsDlg::GetAppIconIndex(LPWSTR pszPath, int iIndex)
{
	if (iIndex == -1)
	{
		return Shell_GetCachedImageIndexW(L"shell32.dll", 2, 0);
	}

	UINT uIconFlags = 0;
	if (pszPath[0] == L'@')
		uIconFlags = GIL_ASYNC | GIL_NOTFILENAME;

	return Shell_GetCachedImageIndexW(pszPath, iIndex, uIconFlags);
}

INT_PTR COpenAsDlg::v_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
			OnInitDialog();
			return TRUE;
		case WM_CLOSE:
			EndDialog(hwnd, IDCANCEL);
			return TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDCANCEL:
					EndDialog(hwnd, IDCANCEL);
					break;
			}
			return TRUE;
		default:
			return FALSE;
	}
}

COpenAsDlg::COpenAsDlg(UINT idBaseDlg,
					   COpenWithExUI *pOpenWithUI,
					   OPENAS_DLG_TYPE type,
					   IMMERSIVE_OPENWITH_FLAGS flags)
	: CDialog(idBaseDlg + type)
	, _pOpenWithUI(pOpenWithUI)
	, _type(type)
	, _flags(flags)
	, _hwndAppList(NULL)
{

}

void COpenAsDlg::OnInitDialog()
{
	_hwndAppList = GetDlgItem(_hwnd, IDD_APPLIST);

	SetShellIcon(134);

	wil::unique_cotaskmem_string spszFileName;
	if (FAILED(_pOpenWithUI->GetItemName(SIGDN_NORMALDISPLAY, &spszFileName)))
	{
		EndDialog(_hwnd, IDCANCEL);
		return;
	}

	RECT rc;
	GetClientRect(GetDlgItem(_hwnd, IDD_FILE_TEXT), &rc);
	PathCompactPathW(NULL, spszFileName.get(), rc.right - 4 * GetSystemMetrics(SM_CXBORDER));
	SetDlgItemTextW(_hwnd, IDD_FILE_TEXT, spszFileName.get());

	bool fAssocRestricted = SHRestricted(REST_NOFILEASSOCIATE);

	if (!fAssocRestricted
		&& ((_flags & IMMERSIVE_OPENWITH_DONOT_EXEC)
		|| _type != OPENAS_DLG_NORMAL))
	{
		CheckDlgButton(_hwnd, IDD_MAKEASSOC, BST_CHECKED);
	}

	if (fAssocRestricted
		|| !_pOpenWithUI->AllowRegistration()
		|| (_flags & IMMERSIVE_OPENWITH_DONOT_EXEC))
	{
		EnableWindow(GetDlgItem(_hwnd, IDD_MAKEASSOC), FALSE);
	}

	_pOpenWithUI->FillListByEnumHandlers();
}
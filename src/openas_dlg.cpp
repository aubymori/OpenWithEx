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
				case IDOK:
					_OnOk();
					break;
				case IDCANCEL:
					EndDialog(hwnd, IDCANCEL);
					break;
				case IDD_OPENWITH_BROWSE:
					_pOpenWithUI->OpenAsOther();
					break;
			}
			return TRUE;
		case WM_NOTIFY:
		{
			switch (((LPNMHDR)lParam)->code)
			{
				case TVN_SELCHANGED:
				case LVN_ITEMCHANGED:
					EnableWindow(GetDlgItem(hwnd, IDOK), GetSelectedItem() != nullptr);
					break;
				case NM_CLICK:
					if (((LPNMHDR)lParam)->idFrom == IDD_OPENWITH_WEBSITE)
					{
						if (!wcscmp(((PNMLINK)lParam)->item.szID, L"Browse"))
						{
							_pOpenWithUI->OpenDownloadURL(_hwnd);
							EndDialog(hwnd, IDCANCEL);
						}
					}
					break;
				case NM_DBLCLK:
					if (((LPNMHDR)lParam)->idFrom == IDD_APPLIST)
					{
						_OnOk();
					}
					break;
			}
			return TRUE;
		}
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
	SetShellIcon(134);

	wil::unique_cotaskmem_string spszFileName;
	HRESULT hr = _pOpenWithUI->GetItemName(
		SIGDN_NORMALDISPLAY,
		&spszFileName);
	if (FAILED(hr) && (_flags & IMMERSIVE_OPENWITH_PROTOCOL))
	{
		hr = _pOpenWithUI->GetItemName(SIGDN_URL, &spszFileName);
	}
	if (FAILED(hr))
	{
		EndDialog(_hwnd, IDCANCEL);
		return;
	}

	RECT rc;
	GetClientRect(GetDlgItem(_hwnd, IDD_FILE_TEXT), &rc);
	PathCompactPathW(NULL, spszFileName.get(), rc.right - 4 * _GetSystemMetrics(SM_CXBORDER));
	SetDlgItemTextW(_hwnd, IDD_FILE_TEXT, spszFileName.get());

	bool fMakeAssocRestricted = SHRestricted(REST_NOFILEASSOCIATE);

	if (!fMakeAssocRestricted
		&& !(_flags & IMMERSIVE_OPENWITH_DONOT_SETDEFAULT)
		&& ((_flags & IMMERSIVE_OPENWITH_DONOT_EXEC)
		|| _type != OPENAS_DLG_NORMAL))
	{
		CheckDlgButton(_hwnd, IDD_MAKEASSOC, BST_CHECKED);
	}

	if (fMakeAssocRestricted
		|| !_pOpenWithUI->AllowRegistration()
		|| (_flags & IMMERSIVE_OPENWITH_DONOT_EXEC))
	{
		EnableWindow(GetDlgItem(_hwnd, IDD_MAKEASSOC), FALSE);
	}

	_pOpenWithUI->FillListByEnumHandlers();
}

void COpenAsDlg::_OnOk()
{
	bool fMakeAssoc = IsDlgButtonChecked(_hwnd, IDD_MAKEASSOC);
	WCHAR szDescription[64];
	LPWSTR pszDescription = nullptr;

	if (_type == OPENAS_DLG_NOTYPE && fMakeAssoc)
	{
		GetDlgItemTextW(_hwnd, IDD_DESCRIPTION, szDescription, ARRAYSIZE(szDescription));
		pszDescription = szDescription;
	}

	_pOpenWithUI->OnOk(fMakeAssoc, pszDescription);
	EndDialog(_hwnd, IDOK);
}
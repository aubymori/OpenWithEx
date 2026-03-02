#include "openas_dlg.h"

INT_PTR COpenAsDlg::v_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
			OnInitDialog();
			return TRUE;
		default:
			return FALSE;
	}
}

COpenAsDlg::COpenAsDlg(UINT idBaseDlg, COpenWithExUI *pOpenWithUI, OPENAS_DLG_TYPE type)
	: CDialog(idBaseDlg + type)
	, _pOpenWithUI(pOpenWithUI)
	, _type(type)
{

}

void COpenAsDlg::OnInitDialog()
{
	SetShellIcon(134);
}
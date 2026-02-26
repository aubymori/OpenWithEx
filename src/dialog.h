#pragma once
#include "openwithex_priv.h"

class CDialog
{
private:
	bool _fShown;

	static INT_PTR CALLBACK s_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	HWND _hwnd;
	UINT _uDlgID;

	virtual INT_PTR v_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

	void SetShellIcon(int iIconID);

	CDialog(UINT uDlgID);

public:
	INT_PTR ShowDialog(HWND hwndParent);
};
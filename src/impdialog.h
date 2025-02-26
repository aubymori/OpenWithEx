#pragma once

#include <windows.h>

class CImpDialog
{
private:
	static INT_PTR CALLBACK s_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	HWND m_hWnd;
	HINSTANCE m_hInst;
	UINT m_uDlgId;
	bool m_fShown;

	virtual INT_PTR CALLBACK v_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

	CImpDialog(HINSTANCE hInst, UINT uDlgId);

public:
	INT_PTR ShowDialog(HWND hWndParent);
};
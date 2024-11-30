#pragma once

#include "openwithex.h"
#include "impdialog.h"
#include <shobjidl.h>
#include <commctrl.h>
#include <vector>

#include "wil/com.h"
#include "wil/resource.h"

#define I_RECOMMENDED 1
#define I_OTHER       2

class COpenAsDlg : public CImpDialog
{
private:
	WCHAR  m_szPath[MAX_PATH];
	LPWSTR m_pszFileName;
	WCHAR  m_szExtOrProtocol[MAX_PATH];
	IMMERSIVE_OPENWITH_FLAGS m_flags;
	bool   m_fUri;
	bool   m_fPreregistered;
	bool   m_fRecommended;
	std::vector<wil::com_ptr<IAssocHandler>> m_handlers;
#ifdef XP
	std::vector<HTREEITEM> m_treeItems;
	HTREEITEM m_hRecommended;
	HTREEITEM m_hOther;
#endif

	INT_PTR CALLBACK v_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	wil::com_ptr<IAssocHandler> _GetSelectedItem();
	int _FindItemIndex(LPCWSTR lpszPath);
	void _SelectItemByIndex(int index);
	void _SetupCategories();
	void _AddItem(wil::com_ptr<IAssocHandler> pItem, int index, bool fForceSelect);
	void _GetHandlers();

	void _BrowseForProgram();
	void _OnOk();

public:
	COpenAsDlg(LPCWSTR lpszPath, IMMERSIVE_OPENWITH_FLAGS flags, bool fUri, bool fPreregistered);
	~COpenAsDlg();
};
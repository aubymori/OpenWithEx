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

int GetAppIconIndex(LPCWSTR lpszIconPath, int iIndex);

class CBaseOpenAsDlg : public CImpDialog
{
private:
	UINT   m_uBrowseTitleId;
	IMMERSIVE_OPENWITH_FLAGS m_flags;
	bool   m_fPreregistered;

	INT_PTR CALLBACK v_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	int _FindItemIndex(LPCWSTR lpszPath);
	void _GetHandlers();
	HRESULT _ClearRecentlyInstalled();

	void _BrowseForProgram();
	void _OnOk();

protected:
	WCHAR  m_szExtOrProtocol[MAX_PATH];
	WCHAR  m_szPath[MAX_PATH];
	LPWSTR m_pszFileName;
	bool   m_fUri;
	std::vector<wil::com_ptr<IAssocHandler>> m_handlers;
	bool   m_fRecommended;

	virtual void _InitProgList() = 0;
	virtual wil::com_ptr<IAssocHandler> _GetSelectedItem() = 0;
	virtual void _SelectItemByIndex(int index) = 0;
	virtual void _SetupCategories() = 0;
	virtual void _AddItem(wil::com_ptr<IAssocHandler> pItem, int index, bool fForceSelect) = 0;

	CBaseOpenAsDlg(LPCWSTR lpszPath, IMMERSIVE_OPENWITH_FLAGS flags, bool fUri, bool fPreregistered, UINT uDlgId, UINT uDlgWithDescId, UINT uDlgProtocolId, UINT uBrowseTitleId);

public:
	~CBaseOpenAsDlg();
};
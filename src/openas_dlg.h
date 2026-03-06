#pragma once
#include "dialog.h"
#include "openwithex_priv.h"
#include "openwithex_ui.h"

enum OPENAS_DLG_TYPE
{
	OPENAS_DLG_NORMAL = 0,
	OPENAS_DLG_NOTYPE,
	OPENAS_DLG_PROTOCOL,
};

class COpenAsDlg : public CDialog
{
protected:
	COpenWithExUI *_pOpenWithUI;
	OPENAS_DLG_TYPE _type;
	IMMERSIVE_OPENWITH_FLAGS _flags;
	HWND _hwndAppList;

	static int GetAppIconIndex(LPWSTR pszPath, int iIndex);

	virtual INT_PTR v_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	COpenAsDlg(UINT idBaseDlg,
			   COpenWithExUI *pOpenWithUI,
			   OPENAS_DLG_TYPE type,
			   IMMERSIVE_OPENWITH_FLAGS flags);
	virtual void OnInitDialog();

private:
	void _OnOk();

public:
	virtual ~COpenAsDlg() {}

	virtual void AddItem(IAssocHandler *pah) = 0;
	virtual void SetupCategories() = 0;
	virtual IAssocHandler *GetSelectedItem() = 0;
	virtual void SelectItemByIndex(int i) = 0;

	friend class COpenWithExUI;
};
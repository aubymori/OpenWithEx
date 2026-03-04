#pragma once
#include "dialog.h"
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

	virtual INT_PTR v_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	COpenAsDlg(UINT idBaseDlg, COpenWithExUI *pOpenWithUI, OPENAS_DLG_TYPE type);
	virtual void OnInitDialog();

public:
	virtual ~COpenAsDlg()
	{

	}

	friend class COpenWithExUI;
};
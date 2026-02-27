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

	COpenAsDlg(UINT idBaseDlg, COpenWithExUI *pOpenWithUI, OPENAS_DLG_TYPE type);

	friend class COpenWithExUI;
};
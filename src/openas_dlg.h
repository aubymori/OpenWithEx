#pragma once
#include "dialog.h"
#include "openwithex_ui.h"

enum OPENAS_DLG_TYPE
{
	OPENAS_DLG_NORMAL = 0,
	OPENAS_DLG_NOTYPE,
	OPENAS_DLG_PROTOCOL,
};

class COpenAsDlg
{
protected:
	COpenWithExUI *_pOpenWithUI;

	template <UINT idBaseDlg>
	COpenAsDlg(COpenWithExUI *pOpenWithUI, OPENAS_DLG_TYPE type)
		: CDialog(idBaseDlg + type)
		, _pOpenWithUI(pOpenWithUI)
	{

	}

	friend class COpenWithExUI;
};
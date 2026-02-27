#include "openas_dlg.h"

COpenAsDlg::COpenAsDlg(UINT idBaseDlg, COpenWithExUI *pOpenWithUI, OPENAS_DLG_TYPE type)
	: CDialog(idBaseDlg + type)
	, _pOpenWithUI(pOpenWithUI)
	, _type(type)
{

}
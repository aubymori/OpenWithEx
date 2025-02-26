#pragma once
#include "baseopenasdlg.h"
#include <uxtheme.h>
#include <shlobj.h>
#include <commctrl.h>
#include <propvarutil.h>
#include <propkey.h>
#include <memory>
#include "iassochandler_internal.h"

class CVistaOpenAsDlg : public CBaseOpenAsDlg
{
private:
	void _InitProgList();
	wil::com_ptr<IAssocHandler> _GetSelectedItem();
	void _SelectItemByIndex(int index);
	void _SetupCategories();
	void _AddItem(wil::com_ptr<IAssocHandler> pItem, int index, bool fForceSelect);

public:
	CVistaOpenAsDlg(LPCWSTR lpszPath, IMMERSIVE_OPENWITH_FLAGS flags, bool fUri, bool fPreregistered);
};
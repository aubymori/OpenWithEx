#pragma once
#include "baseopenasdlg.h"
#include <uxtheme.h>
#include <shlobj.h>
#include <commctrl.h>

class CClassicOpenAsDlg : public CBaseOpenAsDlg
{
private:
	std::vector<HTREEITEM> m_treeItems;
	HTREEITEM m_hRecommended;
	HTREEITEM m_hOther;

	void _InitProgList();
	wil::com_ptr<IAssocHandler> _GetSelectedItem();
	void _SelectItemByIndex(int index);
	void _SetupCategories();
	void _AddItem(wil::com_ptr<IAssocHandler> pItem, int index, bool fForceSelect);

public:
	CClassicOpenAsDlg(LPCWSTR lpszPath, IMMERSIVE_OPENWITH_FLAGS flags, bool fUri, bool fPreregistered);
};
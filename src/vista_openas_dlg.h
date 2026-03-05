#pragma once
#include "openas_dlg.h"

class CVistaOpenAsDlg : public COpenAsDlg
{
private:
    int _cItems;
    bool _fHasRecommended;

public:
    CVistaOpenAsDlg(COpenWithExUI *pOpenWithUI,
                    OPENAS_DLG_TYPE type,
                    IMMERSIVE_OPENWITH_FLAGS flags);

    void OnInitDialog() override;

    void AddItem(IAssocHandler *pah) override;
    void SetupCategories() override;
    IAssocHandler *GetSelectedItem() override;
    void SelectItemByIndex(int i) override;
};
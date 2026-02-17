#pragma once
#include "openwithex_priv.h"

class CExecuteItem
{
private:
    CMINVOKECOMMANDINFOEX _invokeInfo;
    char _szVerb[64];
    IShellItemArray *_pItems;
    IUnknown *_punkSite;
    IUnknown *_pqaOverride;
    LPWSTR _pszDir;
    LPWSTR _pszParam;
    bool _fEnableContainerVerbs;

    void _InitMembers();
    HRESULT _GetContextMenu(IContextMenu **ppContextMenu);

public:
    CExecuteItem(IShellItemArray *pItems);
    ~CExecuteItem();

    void SetSite(IUnknown *punkSite);
    void SetWindow(HWND hwnd);
    HRESULT Execute();
};
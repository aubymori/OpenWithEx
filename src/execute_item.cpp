#include "execute_item.h"
#include "interfaces.h"
#include "wil_priv.h"
#include "util.h"

void CExecuteItem::_InitMembers()
{
    ZeroMemory(_szVerb, sizeof(_szVerb));
    _pszDir = nullptr;
    _pszParam = nullptr;
    _punkSite = nullptr;
    _pqaOverride = nullptr;
    _fEnableContainerVerbs = false;
    _invokeInfo.cbSize = sizeof(_invokeInfo);
    _invokeInfo.nShow = SW_SHOWNORMAL;
    _invokeInfo.fMask = CMIC_MASK_FLAG_LOG_USAGE;
}

HRESULT CExecuteItem::_GetContextMenu(IContextMenu **ppContextMenu)
{
    *ppContextMenu = nullptr;

    HRESULT hr = (_pItems != nullptr) ? S_OK : E_OUTOFMEMORY;
    if (SUCCEEDED(hr))
    {
        wil::ShellBindContextHelper bindCtx;
        CreateBindCtx(0, &bindCtx);
        hr = bindCtx.SetNamedBoolean(L"HeterogeneousContextMenu");
        if (SUCCEEDED(hr))
        {
            hr = _pItems->BindToHandler(
                bindCtx.Get(),
                BHID_SFUIObject,
                IID_PPV_ARGS(ppContextMenu));
            if (SUCCEEDED(hr) && _pqaOverride)
            {
                ComPtr<IObjectWithAssociationList> spowal;
                hr = IUnknown_QueryService(
                    *ppContextMenu,
                    SID_CtxQueryAssociations,
                    IID_PPV_ARGS(&spowal));
                if (SUCCEEDED(hr))
                {
                    ComPtr<IObjectWithAssociationList> spowalSource;
                    hr = _pqaOverride->QueryInterface(IID_PPV_ARGS(&spowalSource));
                    if (SUCCEEDED(hr))
                    {
                        ComPtr<IAssociationList> spal;
                        hr = spowalSource->GetList(&spal);
                        if (SUCCEEDED(hr))
                        {
                            hr = spowal->SetList(spal.Get());
                        }
                    }
                }
            }
        }
    }

    if (FAILED(hr))
    {
        SafeRelease(ppContextMenu);
    }

    return hr;
}

CExecuteItem::CExecuteItem(IShellItemArray *pItems)
    : _pItems(pItems)
{
    _pItems->AddRef();
    _InitMembers();
}

CExecuteItem::~CExecuteItem()
{
    CoTaskMemFree(_pszDir);
    CoTaskMemFree(_pszParam);
    SafeRelease(&_pItems);
    SafeRelease(&_punkSite);
}

void CExecuteItem::SetSite(IUnknown *punkSite)
{
    SetInterface(&_punkSite, punkSite);
}

void CExecuteItem::SetWindow(HWND hwnd)
{
    _invokeInfo.hwnd = hwnd;
}

HRESULT CExecuteItem::Execute()
{

}
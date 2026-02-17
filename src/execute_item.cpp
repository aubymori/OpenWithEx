#include "execute_item.h"
#include "interfaces.h"
#include "wil_priv.h"
#include "util.h"
#include <initguid.h>

DEFINE_GUID(SID_DefFolderMenu, 0x76876E96, 0xD981, 0x4546, 0x94,0xF8, 0x15,0xD0,0x0A,0xB4,0xCE,0x3C);

namespace ServiceProviderImpl
{
    namespace Details
    {
        template <typename T>
        class CServiceProviderImpl final
            : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>
            , IServiceProvider
            >
        {
        public:
            CServiceProviderImpl(IUnknown *punkSite, const T &queryService)
                : _queryService(queryService)
                , _spunkSite(punkSite)
            {
            }

            CServiceProviderImpl(const CServiceProviderImpl &) = delete;
            CServiceProviderImpl(CServiceProviderImpl &&) noexcept = delete;

            //~ Begin IServiceProvider Interface
            STDMETHODIMP QueryService(REFGUID serviceId, REFIID riid, void **ppv) override
            {
                *ppv = nullptr;
                HRESULT hr = _queryService(serviceId, riid, ppv);
                if (hr == S_FALSE) // Assumed if condition
                {
                    hr = IUnknown_QueryService(_spunkSite.Get(), serviceId, riid, ppv);
                }
                return hr;
            }
            //~ End IServiceProvider Interface

        private:
            T _queryService;
            Microsoft::WRL::ComPtr<IUnknown> _spunkSite;
        };
    }

    template <typename T>
    HRESULT Make(IUnknown *punkSite, REFIID riid, void **ppv, const T &queryService)
    {
        auto spServiceProvider = Microsoft::WRL::Make<Details::CServiceProviderImpl<T>>(punkSite, queryService);
        HRESULT hr = spServiceProvider.Get() ? S_OK : E_OUTOFMEMORY;
        if (SUCCEEDED(hr))
        {
            hr = spServiceProvider.CopyTo(riid, ppv);
        }
        return hr;
    }
}

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
    IContextMenu *pContextMenu;
    HRESULT hr = _GetContextMenu(&pContextMenu);
    if (SUCCEEDED(hr))
    {
        HMENU hMenu = CreatePopupMenu();
        hr = hMenu ? S_OK : E_OUTOFMEMORY;
        if (SUCCEEDED(hr))
        {
            wil::unique_cotaskmem_string spszVerbW;
            if (_invokeInfo.lpVerb && SUCCEEDED(SHStrDupA(_invokeInfo.lpVerb, &spszVerbW)))
            {
                Microsoft::WRL::ComPtr<IContextMenuForProgInvoke> spcmpi;
                if (SUCCEEDED(pContextMenu->QueryInterface(IID_PPV_ARGS(&spcmpi))))
                {
                    const WCHAR *rgVerbs[] = { spszVerbW.get() };
                    spcmpi->SetInvokeVerbs(rgVerbs, ARRAYSIZE(rgVerbs));
                }
            }

            Microsoft::WRL::ComPtr<IUnknown> spSite;
            hr = ServiceProviderImpl::Make(_punkSite, IID_PPV_ARGS(&spSite), [](REFGUID serviceId, REFIID riid, void **ppv) -> HRESULT
            {
                return IsEqualGUID(serviceId, SID_DefFolderMenu) ? E_NOINTERFACE : S_FALSE;
            });
            if (SUCCEEDED(hr))
            {
                IUnknown_SetSite(pContextMenu, spSite.Get());

                UINT cmfOptions = 0;
                if (!_fEnableContainerVerbs)
                {
                    cmfOptions |= CMF_VERBSONLY;
                }
                if (_invokeInfo.lpVerb)
                {
                    cmfOptions |= CMF_OPTIMIZEFORINVOKE;
                }

                hr = pContextMenu->QueryContextMenu(hMenu, 0, 1, 0xFF, cmfOptions);
                if (SUCCEEDED(hr))
                {
                    UINT idCmd = -1;
                    if (!_invokeInfo.lpVerb)
                    {
                        idCmd = GetMenuDefaultItem(hMenu, FALSE, 0);
                        if (idCmd != -1)
                        {
                            _invokeInfo.lpVerb = (LPCSTR)(WORD)(idCmd - 1);
                        }
                    }
                    if (_invokeInfo.lpVerb)
                    {
                        if (((UINT)(UINT_PTR)_invokeInfo.lpVerb & 0xFFFF0000) != 0)
                        {
                            _invokeInfo.fMask |= CMIC_MASK_UNICODE;
                            _invokeInfo.lpVerbW = spszVerbW.get();
                        }
                    }
                    else if (idCmd == -1)
                    {
                        hr = E_FAIL;
                    }

                    if (SUCCEEDED(hr))
                    {
                        hr = pContextMenu->InvokeCommand((CMINVOKECOMMANDINFO *)&_invokeInfo);
                    }
                }

                IUnknown_SetSite(pContextMenu, nullptr);
            }

            DestroyMenu(hMenu);
        }

        pContextMenu->Release();
    }

    return hr;
}
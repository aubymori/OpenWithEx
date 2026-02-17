#include "openwithex_launcher.h"
#include "undoc.h"
#include "util.h"
#include "caller_identity.h"
#include "rpc_options_helper.h"
#include "execute_item.h"
#include <initguid.h>
#include <appmgmt.h>

UINT_PTR g_idleTimerId = (UINT_PTR)-1;

HRESULT COpenWithExLauncher::_GetSelectedItem(REFIID riid, LPVOID *ppv)
{
    HRESULT hr = DISP_E_BADINDEX;
    if (_psiaSelection)
    {
        IShellItem *psi = nullptr;
        hr = _psiaSelection->GetItemAt(0, &psi);
        if (SUCCEEDED(hr))
        {
            hr = psi->QueryInterface(riid, ppv);
            psi->Release();
        }
    }
    return hr;
}

HRESULT COpenWithExLauncher::_InstallApplication(IShellItem2 *psi, REFIID riid, void **ppva)
{
    wil::unique_cotaskmem_string spszExt;
    HRESULT hr = psi->GetString(PKEY_FileExtension, &spszExt);
    if (SUCCEEDED(hr))
    {
        if (spszExt.get() && spszExt.get()[0])
        {
            INSTALLDATA id = {};
            id.Type = FILEEXT;
            id.Spec.FileExt = spszExt.get();
            hr = HRESULT_FROM_WIN32(InstallApplication(&id));
            if (SUCCEEDED(hr))
            {
                ComPtr<IAssociationArray> spaa;
                hr = psi->BindToHandler(
                    nullptr, BHID_AssociationArray, IID_PPV_ARGS(&spaa));
                if (SUCCEEDED(hr))
                {
                    hr = spaa->QueryObject(
                        AQVO_SHELLVERB_EXECUTE, nullptr,
                        riid, ppva);
                }
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION);
        }
    }
    return hr;
}

HRESULT COpenWithExLauncher::_InitDelegate(IExecuteCommand *pxc)
{
    HRESULT hr = S_OK;

    if (_state & ECBF_SHOWWINDOW)
    {
        hr = pxc->SetShowWindow(_nShow);
    }

    if (SUCCEEDED(hr) && (_state & ECBF_POSITION))
    {
        hr = pxc->SetPosition(_ptPosition);
    }

    if (SUCCEEDED(hr) && (_state & ECBF_KEYSTATE))
    {
        hr = pxc->SetKeyState(_grfKeyState);
    }

    if (SUCCEEDED(hr) && (_state & ECBF_DIRECTORY))
    {
        hr = pxc->SetDirectory(_pszDirectory);
    }

    if (SUCCEEDED(hr) && (_state & ECBF_PARAMETERS))
    {
        hr = pxc->SetParameters(_pszParameters);
    }

    if (SUCCEEDED(hr) && (_state & ECBF_NOSHOWUI))
    {
        hr = pxc->SetNoShowUI(_fNoShowUI);
    }

    if (SUCCEEDED(hr))
    {
        IObjectWithSelection *pows = nullptr;
        if (SUCCEEDED(pxc->QueryInterface(&pows)))
        {
            pows->SetSelection(_psiaSelection);
            pows->Release();
        }
    }

    return hr;
}

HRESULT COpenWithExLauncher::_InstallHandlerIfNeededAndInvoke()
{
    ComPtr<IShellItem2> spsi;
    HRESULT hr = _GetSelectedItem(IID_PPV_ARGS(&spsi));
    if (SUCCEEDED(hr))
    {
        ComPtr<IExecuteCommand> spxc;
        hr = _InstallApplication(spsi.Get(), IID_PPV_ARGS(&spxc));
        if (SUCCEEDED(hr))
        {
            hr = _InitDelegate(spxc.Get());
        }

        if (SUCCEEDED(hr))
        {
            CoAllowSetForegroundWindow(spxc.Get(), nullptr);
            hr = spxc->Execute();
        }
    }
    return hr;
}

HRESULT COpenWithExLauncher::_GetSelectedItem(REFIID riid, LPVOID *ppv)
{
    HRESULT hr = DISP_E_BADINDEX;
    *ppv = nullptr;

    if (_psiaSelection)
    {
        IShellItem *psi = nullptr;
        hr = _psiaSelection->GetItemAt(0, &psi);
        if (SUCCEEDED(hr))
        {
            hr = psi->QueryInterface(riid, ppv);
            psi->Release();
        }
    }
    return hr;
}

bool COpenWithExLauncher::_AllowSetDefault()
{
    ComPtr<IOpenWithTypeOverride> spTypeOverride;
    wil::unique_cotaskmem_string spsz;

    if (SUCCEEDED(QueryService(IID_IOpenWithTypeOverride, IID_PPV_ARGS(&spTypeOverride)))
        && SUCCEEDED(spTypeOverride->GetOpenWithTypeOverride(&spsz)))
    {
        ComPtr<IAssociationElement> spAssocElem;
        if (SUCCEEDED(AssocCreateElement(CLSID_AssocProgidElement, IID_PPV_ARGS(&spAssocElem))))
        {
            ComPtr<IPersistString2> spPersistString;
            if (SUCCEEDED(spAssocElem.As(&spPersistString))
                && SUCCEEDED(spPersistString->SetString(spsz.get())))
            {
                return FAILED(spAssocElem->QueryExists(AQN_NAMED_VALUE, L"NoOpenWith"));
            }
        }
    }
    else
    {
        ComPtr<IShellItemArray> spItems;
        if (SUCCEEDED(IUnknown_GetSelection(
                static_cast<IServiceProvider *>(this), IID_PPV_ARGS(&spItems))))
        {
            ComPtr<IShellItem2> spItem;
            if (SUCCEEDED(IShellItemArray_GetItemAt(spItems.Get(), 0, IID_PPV_ARGS(&spItem))))
            {
                ComPtr<IAssociationArray> spAssocArray;
                if (SUCCEEDED(spItem->BindToHandler(
                        nullptr, BHID_AssociationArray, IID_PPV_ARGS(&spAssocArray))))
                {
                    return FAILED(spAssocArray->QueryExists(AQN_NAMED_VALUE, L"NoOpenWith"));
                }
            }
        }
    }
    return true;
}

void COpenWithExLauncher::_DoExecute()
{
    HRESULT hr;

    if (CSTR_EQUAL == CompareStringOrdinal(L"openas", -1, _spszCommandName.get(), -1, TRUE))
        hr = _InstallHandlerIfNeededAndInvoke();
    else
        hr = E_FAIL;

    if (FAILED(hr))
    {
        IMMERSIVE_OPENWITH_FLAGS flags;
        if (CSTR_EQUAL == CompareStringOrdinal(L"OpenWithSetDefaultOn", -1, _spszCommandName.get(), -1, TRUE))
        {
            flags = IMMERSIVE_OPENWITH_OVERRIDE;
        }
        else
        {
            flags = IMMERSIVE_OPENWITH_NONE;
            ComPtr<IObjectWithOpenWithFlags> flagsProvider;
            if (SUCCEEDED(IUnknown_QueryService(
                _punkSite,
                IID_IObjectWithOpenWithFlags,
                IID_PPV_ARGS(&flagsProvider)
            )))
            {
                flagsProvider->get_Flags(&flags);
            }
        }

        flags |= IMMERSIVE_OPENWITH_OVERRIDE;

        if (_state & ECBF_POSITION)
        {
            _spOpenWithUI->SetPosition(_ptPosition);
            flags |= IMMERSIVE_OPENWITH_USEPOSITION;
        }

        if (!_AllowSetDefault())
            flags &= ~IMMERSIVE_OPENWITH_OVERRIDE;

        _spOpenWithUI->CreateAndShowFromDelegateExecute(
            static_cast<IExecuteCommand *>(this), flags);
        SafeRelease(&_psiaSelection);
        IUnknown_SetSite(_spOpenWithUI.Get(), nullptr);
    }
}

STDMETHODIMP COpenWithExLauncher::Initialize(LPCWSTR pszCommandName, IPropertyBag *)
{
    return SHStrDupW(pszCommandName, &_spszCommandName);
}

STDMETHODIMP COpenWithExLauncher::GetValue(AHE_TYPE *pahe)
{
    *pahe = AHE_IMMERSIVE;
    return S_OK;
}

STDMETHODIMP COpenWithExLauncher::QueryService(REFGUID serviceId, REFIID riid, void **ppv)
{
    if (_spSiteProxy)
    {
        return _spSiteProxy->QueryService(serviceId, riid, ppv);
    }
    return IUnknown_QueryService(_punkSite, serviceId, riid, ppv);
}

STDMETHODIMP COpenWithExLauncher::Launch(HWND hwndOwner, LPCWSTR pszFile, IMMERSIVE_OPENWITH_FLAGS flags)
{
    ComPtr<COpenWithExUI> spOpenWithUI = Make<COpenWithExUI>();
    if (!spOpenWithUI)
        return E_OUTOFMEMORY;
    return spOpenWithUI->CreateAndShow(hwndOwner, pszFile, flags);
}

STDMETHODIMP COpenWithExLauncher::CreateInstance(IUnknown *, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

STDMETHODIMP COpenWithExLauncher::LockServer(BOOL)
{
    return S_OK;
}

STDMETHODIMP COpenWithExLauncher::SetSite(IUnknown *punkSite)
{
    IUnknown_Set(&_punkSite, punkSite);
    return S_OK;
}

STDMETHODIMP COpenWithExLauncher::GetSite(REFIID riid, void **ppvSite)
{
    if (_punkSite)
    {
        return _punkSite->QueryInterface(riid, ppvSite);
    }
    return E_FAIL;
}

STDMETHODIMP COpenWithExLauncher::SetKeyState(DWORD grfKeyState)
{
    _grfKeyState = grfKeyState;
    _state |= ECBF_KEYSTATE;
    return S_OK;
}

STDMETHODIMP COpenWithExLauncher::SetParameters(LPCWSTR pszParameters)
{
    if (Str_SetPtrW(&_pszParameters, pszParameters))
    {
        _state |= ECBF_PARAMETERS;
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

STDMETHODIMP COpenWithExLauncher::SetPosition(POINT pt)
{
    _ptPosition = pt;
    _state |= ECBF_POSITION;
    return S_OK;
}

STDMETHODIMP COpenWithExLauncher::SetShowWindow(int nShow)
{
    _nShow = nShow;
    _state |= ECBF_SHOWWINDOW;
    return S_OK;
}

STDMETHODIMP COpenWithExLauncher::SetNoShowUI(BOOL fNoShowUI)
{
    _fNoShowUI = fNoShowUI;
    _state |= ECBF_NOSHOWUI;
    return S_OK;
}

STDMETHODIMP COpenWithExLauncher::SetDirectory(LPCWSTR pszDirectory)
{
    if (Str_SetPtrW(&_pszDirectory, pszDirectory))
    {
        _state |= ECBF_DIRECTORY;
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

STDMETHODIMP COpenWithExLauncher::Execute()
{
    HRESULT hr;

    KillTimer(NULL, g_idleTimerId);
    if (CSTR_EQUAL == CompareStringOrdinal(
            L"InvokeDefaultVerbInOtherProcess", -1,
            _spszCommandName.get(), -1,
            TRUE))
    {
        hr = _psiaSelection ? S_OK : E_INVALIDARG;
        if (SUCCEEDED(hr))
        {
            PROCESS_UICONTEXT processUIContext;
            hr = CallerIdentity::GetCallingProcessType(&processUIContext);
            if (SUCCEEDED(hr))
            {
                hr = (processUIContext == PROCESS_UICONTEXT_IMMERSIVE_BROKER) ? S_OK : E_FAIL;
                if (SUCCEEDED(hr))
                {
                    ComPtr<IWakeOnRPCCalls> spWakeOnCalls;
                    if (SUCCEEDED(IUnknown_QueryService(
                            _punkSite, __uuidof(IWakeOnRPCCalls), IID_PPV_ARGS(&spWakeOnCalls)))
                        && S_OK == spWakeOnCalls->ShouldWakeOnRPCCalls())
                    {
                        // This is misspelled as "spSeriveProvider" in the original OpenWith.exe
                        ComPtr<IServiceProvider> spServiceProvider;
                        if (SUCCEEDED(_punkSite->QueryInterface(IID_PPV_ARGS(&spServiceProvider)))
                            && SUCCEEDED(RpcOptionsHelper::CopyProxy(
                                    spServiceProvider.Get(), _spSiteProxy.ReleaseAndGetAddressOf())))
                        {
                            ComPtr<IRpcOptions> spRpcOptions;
                            if (SUCCEEDED(RpcOptionsHelper::GetRpcOptions(_spSiteProxy.Get(), &spRpcOptions)))
                            {
                                spRpcOptions->Set(_spSiteProxy.Get(), COMBND_RESERVED1, 0);
                            }
                        }
                    }
                }

                HWND hwndOwner;
                IUnknown_GetParentWindow(_punkSite, &hwndOwner);

                CExecuteItem executeItem(_psiaSelection);
                executeItem.SetWindow(hwndOwner);
                executeItem.SetSite(static_cast<IServiceProvider *>(this));
                executeItem.Execute();
            }
        }

        PostQuitMessage(0);
    }
    else
    {
        _spOpenWithUI = Make<COpenWithExUI>();
        if (!_spOpenWithUI)
            return E_OUTOFMEMORY;

        IUnknown_SetSite(_spOpenWithUI.Get(), static_cast<IServiceProvider *>(this));

        if (PostThreadMessageW(GetCurrentThreadId(), 0x8001, 0, 0))
            hr = S_OK;
        else
            hr = ResultFromKnownLastError();
    }

    return hr;
}

STDMETHODIMP COpenWithExLauncher::SetAssocElement(IAssociationElement *pae)
{
    IUnknown_Set((IUnknown **)&_paeAssoc, pae);
    return S_OK;
}

STDMETHODIMP COpenWithExLauncher::GetAssocElement(REFIID riid, void **ppv)
{
    if (_paeAssoc)
    {
        return _paeAssoc->QueryInterface(riid, ppv);
    }
    return E_NOINTERFACE;
}

STDMETHODIMP COpenWithExLauncher::SetSelection(IShellItemArray *psia)
{
    IUnknown_Set((IUnknown **)&_psiaSelection, psia);
    return S_OK;
}

STDMETHODIMP COpenWithExLauncher::GetSelection(REFIID riid, void **ppv)
{
    if (_psiaSelection)
    {
        return _psiaSelection->QueryInterface(riid, ppv);
    }
    return E_NOT_SET;
}

DEFINE_GUID(CLSID_ExecuteUnknown, 0xE44E9428, 0xBDBC, 0x4987, 0xA0,0x99, 0x40,0xDC,0x8F,0xD2,0x55,0xE7);

HRESULT COpenWithExLauncher::RunMessageLoop()
{
    DWORD dwReg;
    RETURN_IF_FAILED(CoRegisterClassObject(
        CLSID_ExecuteUnknown,
        static_cast<IExecuteCommand *>(this),
        CLSCTX_LOCAL_SERVER,
        REGCLS_SINGLEUSE,
        &dwReg));

    g_idleTimerId = SetTimer(NULL, 0, 20000, nullptr);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0)
    {
        if (msg.message == 0x8001)
        {
            _DoExecute();
        }
        else if (msg.message == WM_TIMER && !msg.hwnd && msg.wParam == g_idleTimerId)
        {
            PostQuitMessage(0);
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    CoRevokeClassObject(dwReg);
    return S_OK;
}
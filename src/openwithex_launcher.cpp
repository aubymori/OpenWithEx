#include "openwithex_launcher.h"
#include <initguid.h>

UINT_PTR g_idleTimerId = (UINT_PTR)-1;

void COpenWithExLauncher::_DoExecute()
{

}

STDMETHODIMP COpenWithExLauncher::GetValue(AHE_TYPE *pahe)
{
    *pahe = AHE_IMMERSIVE;
    return S_OK;
}

STDMETHODIMP COpenWithExLauncher::Initialize(LPCWSTR pszCommandName, IPropertyBag *)
{
    return SHStrDupW(pszCommandName, &_spszCommandName);
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
    return E_NOTIMPL;
}

STDMETHODIMP COpenWithExLauncher::CreateInstance(IUnknown *, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

STDMETHODIMP COpenWithExLauncher::LockServer(BOOL)
{
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

STDMETHODIMP COpenWithExLauncher::SetSite(IUnknown *punkSite)
{
    IUnknown_Set(&_punkSite, punkSite);
    return S_OK;
}

STDMETHODIMP COpenWithExLauncher::SetAssocElement(IAssociationElement *pae)
{
    IUnknown_Set((IUnknown **)&_paeAssoc, pae);
    return E_NOTIMPL;
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
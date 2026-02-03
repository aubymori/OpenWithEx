#include "openwithex_launcher.h"

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
    return E_NOTIMPL;
}

STDMETHODIMP COpenWithExLauncher::LockServer(BOOL)
{
    return E_NOTIMPL;
}

STDMETHODIMP COpenWithExLauncher::GetSite(REFIID riid, void **ppvSite)
{
    return E_NOTIMPL;
}

STDMETHODIMP COpenWithExLauncher::SetSite(IUnknown *punkSite)
{
    return E_NOTIMPL;
}

STDMETHODIMP COpenWithExLauncher::SetAssocElement(IAssociationElement *pae)
{
    return E_NOTIMPL;
}

STDMETHODIMP COpenWithExLauncher::GetAssocElement(REFIID riid, void **ppv)
{
    return E_NOTIMPL;
}

STDMETHODIMP COpenWithExLauncher::SetSelection(IShellItemArray *psia)
{
    return E_NOTIMPL;
}

STDMETHODIMP COpenWithExLauncher::GetSelection(REFIID riid, void **ppv)
{
    return E_NOTIMPL;
}
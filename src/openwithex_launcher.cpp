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
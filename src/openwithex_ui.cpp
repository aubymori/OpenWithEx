#include "openwithex_ui.h"

HRESULT COpenWithExUI::CreateAndShow(HWND hwndOwner, LPCWSTR pszFileName, IMMERSIVE_OPENWITH_FLAGS flags)
{
	return E_NOTIMPL;
}

STDMETHODIMP COpenWithExUI::GetSite(REFIID riid, LPVOID *ppvSite)
{
	*ppvSite = nullptr;

	if (_spunkSite)
	{
		return _spunkSite->QueryInterface(riid, ppvSite);
	}
	return E_FAIL;
}

STDMETHODIMP COpenWithExUI::SetSite(IUnknown *punkSite)
{
	IUnknown_Set(&_spunkSite, punkSite);
	return S_OK;
}
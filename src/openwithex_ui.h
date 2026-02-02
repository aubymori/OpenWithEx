#pragma once
#include "openwithex_priv.h"

class COpenWithExUI : public RuntimeClass<
	RuntimeClassFlags<ClassicCom>,
	IObjectWithSite>
{
private:
	wil::unique_cotaskmem_string _spszTypeID;
	ComPtr<IUnknown> _spunkSite;

public:
	HRESULT CreateAndShow(HWND hwndOwner, LPCWSTR pszFileName, IMMERSIVE_OPENWITH_FLAGS flags);

	// IObjectWithSite impl
	STDMETHODIMP SetSite(IUnknown *punkSite) override;
	STDMETHODIMP GetSite(REFIID riid, LPVOID *ppvSite) override;
};
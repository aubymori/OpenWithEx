#include "openwithex_ui.h"
#include "file_sys_bind_data.h"
#include "interfaces.h"
#include "util.h"

HRESULT COpenWithExUI::_CreateAndShow()
{
	wil::unique_cotaskmem_string spsz;
	_spItem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &spsz);

	HRESULT hr = S_OK;

	if (_spszTypeID.get())
	{
		if (_spszTypeID.get()[0] == L'.')
		{
			_openwithflags &= ~IMMERSIVE_OPENWITH_PROTOCOL;
		}
		else
		{
			_openwithflags |= IMMERSIVE_OPENWITH_PROTOCOL;
		}
	}
	else if (!(_openwithflags & IMMERSIVE_OPENWITH_PROTOCOL)
			 && !(_openwithflags & IMMERSIVE_OPENWITH_URL))
	{
		hr = _spItem->GetString(PKEY_FileExtension, &_spszTypeID);
		if (hr == E_NOT_SET)
		{
			_spszTypeID = wil::make_cotaskmem_string(L"");
			if (!_spszTypeID.get())
				hr = E_OUTOFMEMORY;
		}
	}
	else if (!(_openwithflags & IMMERSIVE_OPENWITH_URL))
	{
		hr = GetUrlPartFromShellItemName(_spItem.Get(), SIGDN_URL, URL_PART_SCHEME, &_spszTypeID);
	}
	else
	{
		wil::unique_cotaskmem_string spszUrl;
		hr = _spItem->GetDisplayName(SIGDN_URL, &spszUrl);
		if (SUCCEEDED(hr))
		{
			hr = GetUrlPartFromString(spszUrl.get(), URL_PART_HOSTNAME, &_spszTypeID);
		}
	}

	WCHAR szMessage[MAX_PATH * 2];
	swprintf_s(
		szMessage, 
		L"Item: %s\nType: %s\nFlags: 0x%X",
		spsz.get(), _spszTypeID.get(), _openwithflags);

	MessageBoxW(NULL, szMessage, L"OpenWithEx", MB_ICONINFORMATION);
	return E_NOTIMPL;
}

STDMETHODIMP COpenWithExUI::SetSite(IUnknown *punkSite)
{
	if (punkSite)
	{
		ComPtr<IOpenWithTypeOverride> spTypeOverride;
		if (SUCCEEDED(punkSite->QueryInterface(IID_PPV_ARGS(&spTypeOverride))))
		{
			spTypeOverride->GetOpenWithTypeOverride(&_spszTypeID);
		}
	}

	IUnknown_Set(&_spunkSite, punkSite);
	return S_OK;
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

HRESULT COpenWithExUI::CreateAndShow(HWND hwndOwner, LPCWSTR pszFileName, IMMERSIVE_OPENWITH_FLAGS flags)
{
	HRESULT hr;
	if (flags & IMMERSIVE_OPENWITH_PROTOCOL)
		hr = SHCreateItemFromParsingName(pszFileName, nullptr, IID_PPV_ARGS(&_spItem));
	else
		hr = SHSimpleItemFromAttributes(pszFileName, FILE_ATTRIBUTE_NORMAL, IID_PPV_ARGS(&_spItem));

	if (SUCCEEDED(hr))
	{
		hr = SHCreateShellItemArrayFromShellItem(_spItem.Get(), IID_PPV_ARGS(&_spItems));
		if (SUCCEEDED(hr))
		{
			_hwndOwner = hwndOwner;
			_openwithflags = flags;
			return _CreateAndShow();
		}
	}

	return hr;
}

HRESULT COpenWithExUI::CreateAndShowFromDelegateExecute(IExecuteCommand *pxc, IMMERSIVE_OPENWITH_FLAGS flags)
{
	_openwithflags = flags;
	
	RETURN_IF_FAILED(IUnknown_GetSelection(pxc, IID_PPV_ARGS(&_spItems)));
	RETURN_IF_FAILED(IShellItemArray_GetItemAt(_spItems.Get(), 0, IID_PPV_ARGS(&_spItem)));

	wil::unique_cotaskmem_string spsz;
	if ((_openwithflags & IMMERSIVE_OPENWITH_URL)
		&& SUCCEEDED_LOG(_spItem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &spsz)))
	{
		if (PathIsURLW(spsz.get()))
			_openwithflags |= IMMERSIVE_OPENWITH_PROTOCOL;
	}

	RETURN_HR(_CreateAndShow());
}

HRESULT COpenWithExUI::SetPosition(POINT pt)
{
	_ptPosition = pt;
	return S_OK;
}
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

	if (SUCCEEDED(hr) && !(_openwithflags & IMMERSIVE_OPENWITH_URL))
	{
		_fEmptyExt = (_spszTypeID.get()[0] == L'\0') 
			|| (CSTR_EQUAL == CompareStringOrdinal(_spszTypeID.get(), -1, L".", -1, TRUE));

		ComPtr<IApplicationAssociationRegistrationInternal> spIAAR;
		hr = SHCreateAssociationRegistration(IID_PPV_ARGS(&spIAAR));
		if (SUCCEEDED(hr) && !_fEmptyExt)
		{
			if (_openwithflags & IMMERSIVE_OPENWITH_PROTOCOL)
			{
				hr = spIAAR->QueryCurrentDefault(_spszTypeID.get(), AT_URLPROTOCOL, AL_EFFECTIVE, &_spszDefaultProgID);
			}
			else
			{
				hr = spIAAR->QueryCurrentDefault(_spszTypeID.get(), AT_FILEEXTENSION, AL_EFFECTIVE, &_spszDefaultProgID);
				if (SUCCEEDED(hr))
				{
					ComPtr<IQueryAssociations> spQueryAssoc;
					if (SUCCEEDED(_spItem->BindToHandler(nullptr, BHID_AssociationArray, IID_PPV_ARGS(&spQueryAssoc))))
					{
						WCHAR szNoOpenMsg[MAX_PATH];
						DWORD cch = ARRAYSIZE(szNoOpenMsg);
						if (SUCCEEDED(spQueryAssoc->GetString(ASSOCF_IGNOREBASECLASS, ASSOCSTR_NOOPEN, nullptr, szNoOpenMsg, &cch)))
						{
							MessageBoxW(NULL,
										L"Is NoOpen",
										L"OpenWithEx",
										MB_ICONINFORMATION);
						}
					}
				}
			}
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

COpenWithExUI::COpenWithExUI()
	: _hwndOwner(NULL)
	, _openwithflags(IMMERSIVE_OPENWITH_NONE)
	, _ptPosition{ 0, 0 }
	, _fEmptyExt(false)
{

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
#include "openwithex_ui.h"
#include "file_sys_bind_data.h"
#include "interfaces.h"
#include "util.h"
#include "noopen_dlg.h"
#include "internet_openas_dlg.h"
#include "vista_openas_dlg.h"

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

	RETURN_IF_FAILED(_spItem->BindToHandler(nullptr, BHID_AssociationArray, IID_PPV_ARGS(&_spQueryAssoc)));

	OPENAS_DLG_TYPE dlgType = OPENAS_DLG_NOTYPE;

	if (SUCCEEDED(hr) && !(_openwithflags & IMMERSIVE_OPENWITH_URL))
	{
		_fEmptyExt = (_spszTypeID.get()[0] == L'\0') 
			|| (CSTR_EQUAL == CompareStringOrdinal(_spszTypeID.get(), -1, L".", -1, TRUE));

		bool fHasCommand = false;
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
			}

			if (SUCCEEDED(hr))
			{
				MessageBoxW(
					NULL,
					_spszDefaultProgID.get(),
					L"Default ProgID:",
					MB_ICONINFORMATION);

				WCHAR szCmd[MAX_PATH];
				DWORD cch = ARRAYSIZE(szCmd);
				if (SUCCEEDED(_spQueryAssoc->GetString(ASSOCF_IGNOREBASECLASS, ASSOCSTR_COMMAND, nullptr, szCmd, &cch)))
				{
					MessageBoxW(
						NULL,
						szCmd,
						L"Command:",
						MB_ICONINFORMATION);
					fHasCommand = true;
				}
			}

			if (hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION))
				hr = S_OK;
		}

		if (!(_openwithflags & IMMERSIVE_OPENWITH_PROTOCOL) && !_fEmptyExt)
		{
			WCHAR szNoOpenMsg[MAX_PATH];
			DWORD cchNoOpenMsg = ARRAYSIZE(szNoOpenMsg);
			WCHAR szTypeName[MAX_PATH];
			DWORD cchTypeName = ARRAYSIZE(szTypeName);
			wil::unique_cotaskmem_string spszFileName;

			if (!fHasCommand)
			{
				if (g_style != OPENWITHEX_STYLE_NT4)
				{
					HRESULT hrNoOpen = _spQueryAssoc->GetString(ASSOCF_IGNOREBASECLASS, ASSOCSTR_NOOPEN, nullptr, szNoOpenMsg, &cchNoOpenMsg);

					if (SUCCEEDED(hrNoOpen))
					{
						hrNoOpen = _spItem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &spszFileName);
					}

					if (SUCCEEDED(hrNoOpen))
					{
						hrNoOpen = _spQueryAssoc->GetString(0, ASSOCSTR_FRIENDLYDOCNAME, nullptr, szTypeName, &cchTypeName);
					}

					if (SUCCEEDED(hrNoOpen))
					{
						CNoOpenDlg dlg(this, szNoOpenMsg);
						INT_PTR result = dlg.ShowDialog(_hwndOwner);
						if (result == IDCANCEL)
						{
							return hr;
						}
					}
				}

				if (g_style <= OPENWITHEX_STYLE_XP
					&& !SHRestricted(REST_NOINTERNETOPENWITH))
				{
					CInternetOpenAsDlg dlg(this);
					INT_PTR result = dlg.ShowDialog(_hwndOwner);
					if (result != IDOK)
					{
						return hr;
					}
				}
			}
		}
	}

	// Empty extensions can't be associated.
	if (_fEmptyExt)
	{
		dlgType = OPENAS_DLG_NORMAL;
	}
	else if (_openwithflags & IMMERSIVE_OPENWITH_PROTOCOL)
	{
		dlgType = OPENAS_DLG_PROTOCOL;
	}
	// The original XP code uses COM here to check if the class key
	// exists. However, that method will now return the key from
	// FileExts as a valid "class key". This is problematic because
	// *every* file extension that ever reaches the Open with UI gets
	// a key there. Only preregistered types and user types with
	// handlers get a *true* class key.
	else
	{
		HKEY hkey;
		if (ERROR_SUCCESS == RegOpenKeyExW(
			HKEY_CLASSES_ROOT,
			_spszTypeID.get(),
			0, KEY_READ, &hkey))
		{
			dlgType = OPENAS_DLG_NORMAL;
			RegCloseKey(hkey);
		}
	}

	COpenAsDlg *pDlg = nullptr;
	switch (g_style)
	{
		case OPENWITHEX_STYLE_VISTA:
			pDlg = new CVistaOpenAsDlg(this, dlgType, _openwithflags);
			break;
		default:
			goto SkipDialog;
	}

	pDlg->ShowDialog(_hwndOwner);
	
SkipDialog:
	WCHAR szMessage[MAX_PATH * 2];
	swprintf_s(
		szMessage, 
		L"Item: %s\nType: %s\nFlags: 0x%X",
		spsz.get(), _spszTypeID.get(), _openwithflags);
	MessageBoxW(NULL, szMessage, L"OpenWithEx", MB_ICONINFORMATION);

	return hr;
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

HRESULT COpenWithExUI::GetItemName(SIGDN sigdnName, LPWSTR *ppszOut)
{
	return _spItem->GetDisplayName(sigdnName, ppszOut);
}

HRESULT COpenWithExUI::GetTypeID(LPWSTR *ppszOut)
{
	return SHStrDupW(_spszTypeID.get(), ppszOut);
}

HRESULT COpenWithExUI::GetDescription(LPWSTR pszOut, DWORD cchOut)
{
	return !_spQueryAssoc ? E_FAIL : _spQueryAssoc->GetString(0, ASSOCSTR_FRIENDLYDOCNAME, nullptr, pszOut, &cchOut);
}

void COpenWithExUI::OpenDownloadURL(HWND hwnd)
{
	WCHAR szUrl[1024];
	swprintf_s(
		szUrl,
		L"http://go.microsoft.com/fwlink/?LinkId=57426&Ext=%s",
		_spszTypeID.get());
	ShellExecuteW(
		hwnd,
		nullptr,
		szUrl,
		nullptr, nullptr,
		SW_SHOWNORMAL);
}

bool COpenWithExUI::AllowRegistration()
{
	if (_fEmptyExt)
		return false;

	if (!(_openwithflags & IMMERSIVE_OPENWITH_OVERRIDE)
	|| (_openwithflags & IMMERSIVE_OPENWITH_DONOT_SETDEFAULT))
		return false;

	return true;
}
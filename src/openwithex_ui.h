#pragma once
#include "openwithex_priv.h"

class COpenWithExUI : public RuntimeClass<
	RuntimeClassFlags<ClassicCom>,
	IObjectWithSite>
{
private:
	ComPtr<IUnknown> _spunkSite;
	HWND _hwndOwner;
	ComPtr<IShellItem2> _spItem;
	ComPtr<IShellItemArray> _spItems;
	ComPtr<IQueryAssociations> _spQueryAssoc;
	wil::unique_cotaskmem_string _spszTypeID;
	wil::unique_cotaskmem_string _spszDefaultProgID;
	IMMERSIVE_OPENWITH_FLAGS _openwithflags;
	POINT _ptPosition;
	bool _fEmptyExt;

	HRESULT _CreateAndShow();

public:
	// IObjectWithSite
	STDMETHODIMP SetSite(IUnknown *punkSite) override;
	STDMETHODIMP GetSite(REFIID riid, LPVOID *ppvSite) override;

	// COpenWithExUI
	COpenWithExUI();
	HRESULT CreateAndShow(HWND hwndOwner, LPCWSTR pszFileName, IMMERSIVE_OPENWITH_FLAGS flags);
	HRESULT CreateAndShowFromDelegateExecute(IExecuteCommand *pxc, IMMERSIVE_OPENWITH_FLAGS flags);
	HRESULT SetPosition(POINT pt);

	HRESULT GetItemName(SIGDN sigdnName, LPWSTR *ppszOut);
	HRESULT GetTypeID(LPWSTR *ppszOut);
	HRESULT GetDescription(LPWSTR pszOut, DWORD cchOut);
	void OpenDownloadURL(HWND hwnd);
};
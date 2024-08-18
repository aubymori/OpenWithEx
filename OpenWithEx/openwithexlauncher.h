#pragma once
#include "iopenwithlauncher.h"

class COpenWithExLauncher
	: public IExecuteCommandApplicationHostEnvironment
	, public IServiceProvider
	, public IObjectWithSite
	, public IObjectWithSelection
	, public IInitializeCommand
	, public IOpenWithLauncher
	, public IClassFactory
{
private:
	ULONG m_cRef;
	IUnknown *m_pSite;
	IShellItemArray *m_pSelection;

public:
	// IUnknown
	HRESULT QueryInterface(REFIID riid, LPVOID *ppvObj);
	ULONG AddRef();
	ULONG Release();

	// IExecuteCommandApplicationHostEnvironment
	HRESULT GetValue(AHE_TYPE *pahe);

	// IServiceProvider
	HRESULT QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

	// IObjectWithSite
	HRESULT GetSite(REFIID riid, void **ppvSite);
	HRESULT SetSite(IUnknown *pUnkSite);
	
	// IObjectWithSelection
	HRESULT GetSelection(REFIID riid, void **ppv);
	HRESULT SetSelection(IShellItemArray *psia);

	// IInitializeCommand
	HRESULT Initialize(LPCWSTR pszCommandName, IPropertyBag *ppb);

	// IOpenWithLauncher
	HRESULT Launch(HWND hWndParent, LPCWSTR lpszPath, IMMERSIVE_OPENWITH_FLAGS flags);

	// IClassFactory
	HRESULT CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject);
	HRESULT LockServer(BOOL fLock);

	// COpenWithExLauncher
	COpenWithExLauncher();
	~COpenWithExLauncher();
};
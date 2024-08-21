#pragma once
#include "iopenwithlauncher.h"

class COpenWithExLauncher
	: public IUnknown
	, public IExecuteCommandApplicationHostEnvironment
	, public IServiceProvider
	, public IObjectWithSite
	, public IObjectWithSelection
	, public IInitializeCommand
	, public IExecuteCommand
	, public IOpenWithLauncher
	, public IClassFactory
{
private:
	ULONG m_cRef;
	IUnknown *m_pSite;
	IShellItemArray *m_pSelection;

public:
	// IUnknown
	HRESULT QueryInterface(REFIID riid, LPVOID *ppvObj) override;
	ULONG AddRef() override;
	ULONG Release() override;

	// IExecuteCommandApplicationHostEnvironment
	HRESULT GetValue(AHE_TYPE *pahe) override;

	// IServiceProvider
	HRESULT QueryService(REFGUID guidService, REFIID riid, void **ppvObject) override;

	// IObjectWithSite
	HRESULT GetSite(REFIID riid, void **ppvSite) override;
	HRESULT SetSite(IUnknown *pUnkSite) override;
	
	// IObjectWithSelection
	HRESULT GetSelection(REFIID riid, void **ppv) override;
	HRESULT SetSelection(IShellItemArray *psia) override;

	// IInitializeCommand
	HRESULT Initialize(LPCWSTR pszCommandName, IPropertyBag *ppb) override;

	// IExecuteCommand
	HRESULT STDMETHODCALLTYPE SetKeyState(DWORD grfKeyState) override;

	HRESULT STDMETHODCALLTYPE SetParameters(__RPC__in_string LPCWSTR pszParameters) override;

	HRESULT STDMETHODCALLTYPE SetPosition(POINT pt) override;

	HRESULT STDMETHODCALLTYPE SetShowWindow(int nShow) override;

	HRESULT STDMETHODCALLTYPE SetNoShowUI(BOOL fNoShowUI) override;

	HRESULT STDMETHODCALLTYPE SetDirectory(__RPC__in_string LPCWSTR pszDirectory) override;

	HRESULT STDMETHODCALLTYPE Execute(void) override;

	// IOpenWithLauncher
	HRESULT Launch(HWND hWndParent, LPCWSTR lpszPath, IMMERSIVE_OPENWITH_FLAGS flags) override;

	// IClassFactory
	HRESULT CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject) override;
	HRESULT LockServer(BOOL fLock) override;

	// COpenWithExLauncher
	COpenWithExLauncher();
	~COpenWithExLauncher();

	HRESULT RunMessageLoop();

#ifndef NDEBUG
	static DWORD s_instCounter;
	DWORD m_instId;
	inline void Log(const wchar_t *szMethodName, const wchar_t *format, ...);
#endif
};
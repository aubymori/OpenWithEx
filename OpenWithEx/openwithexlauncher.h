#pragma once
#include "iopenwithlauncher.h"
#include "iobjectwithassociationelement.h"

class COpenWithExLauncher
	: public IUnknown
	, public IExecuteCommandApplicationHostEnvironment
	, public IServiceProvider
	, public IObjectWithSite
	, public IObjectWithAssociationElement
	, public IObjectWithSelection
	, public IInitializeCommand
	, public IExecuteCommand
	, public IOpenWithLauncher
	, public IClassFactory
{
private:
	// IUnknown
	ULONG m_cRef;

	// IObjectWithSite
	IUnknown *m_pSite;

	// IObjectWithAssociationElement
	IAssociationElement *m_pAssocElm;

	// IObjectWithSelection
	IShellItemArray *m_pSelection;

	// IExecuteCommand
	DWORD  m_dwKeyState;
	LPWSTR m_pszParameters;
	POINT  m_position;
	int    m_nShow;
	LPWSTR m_pszDirectory;
	BOOL   m_fNoShowUI;
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

	// IObjectWithAssociationElement
	HRESULT SetAssocElement(IAssociationElement *pae);
	HRESULT GetAssocElement(REFIID riid, void **ppv);
	
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
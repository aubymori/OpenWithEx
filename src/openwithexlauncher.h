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
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj) override;
STDMETHODIMP_(	ULONG) AddRef() override;
STDMETHODIMP_(	ULONG) Release() override;

	// IExecuteCommandApplicationHostEnvironment
	STDMETHODIMP GetValue(AHE_TYPE *pahe) override;

	// IServiceProvider
	STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppvObject) override;

	// IObjectWithSite
	STDMETHODIMP GetSite(REFIID riid, void **ppvSite) override;
	STDMETHODIMP SetSite(IUnknown *pUnkSite) override;

	// IObjectWithAssociationElement
	STDMETHODIMP SetAssocElement(IAssociationElement *pae);
	STDMETHODIMP GetAssocElement(REFIID riid, void **ppv);
	
	// IObjectWithSelection
	STDMETHODIMP GetSelection(REFIID riid, void **ppv) override;
	STDMETHODIMP SetSelection(IShellItemArray *psia) override;

	// IInitializeCommand
	STDMETHODIMP Initialize(LPCWSTR pszCommandName, IPropertyBag *ppb) override;

	// IExecuteCommand
	STDMETHODIMP SetKeyState(DWORD grfKeyState) override;
	STDMETHODIMP SetParameters(__RPC__in_string LPCWSTR pszParameters) override;
	STDMETHODIMP SetPosition(POINT pt) override;
	STDMETHODIMP SetShowWindow(int nShow) override;
	STDMETHODIMP SetNoShowUI(BOOL fNoShowUI) override;
	STDMETHODIMP SetDirectory(__RPC__in_string LPCWSTR pszDirectory) override;
	STDMETHODIMP Execute(void) override;

	// IOpenWithLauncher
	STDMETHODIMP Launch(HWND hWndParent, LPCWSTR lpszPath, IMMERSIVE_OPENWITH_FLAGS flags) override;

	// IClassFactory
	STDMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject) override;
	STDMETHODIMP LockServer(BOOL fLock) override;

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
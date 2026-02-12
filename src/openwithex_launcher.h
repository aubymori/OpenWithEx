#pragma once
#include "openwithex_priv.h"
#include "openwithex_ui.h"
#include "interfaces.h"

enum EXEC_CMD_BASE_STATE_FLAGS
{
	ECBF_DEFAULT    = 0x0,
	ECBF_KEYSTATE   = 0x2,
	ECBF_PARAMETERS = 0x4,
	ECBF_POSITION   = 0x8,
	ECBF_SHOWWINDOW = 0x10,
	ECBF_DIRECTORY  = 0x80,
	ECBF_NOSHOWUI   = 0x100,
};

DEFINE_ENUM_FLAG_OPERATORS(EXEC_CMD_BASE_STATE_FLAGS);

class COpenWithExLauncher : public RuntimeClass<
	RuntimeClassFlags<ClassicCom>,
	IInitializeCommand,
	IExecuteCommandApplicationHostEnvironment,
	IServiceProvider,
	IOpenWithLauncher,
	IClassFactory,
	IObjectWithSite,
	IExecuteCommand,
	IObjectWithAssociationElement,
	IObjectWithSelection>
{
private:
	EXEC_CMD_BASE_STATE_FLAGS _state;
	DWORD _grfKeyState;
	POINT _ptPosition;
	int _nShow;
	BOOL _fNoShowUI;
	BOOL _fAllowAsync;
	LPWSTR _pszParameters;
	LPWSTR _pszDirectory;
	IAssociationElement *_paeAssoc;
	/*
	 * The original COpenWithLauncher has this and never uses it
	 * except for releasing it in SetAssocElement...
	 */
	// IQuerySource *_pqs;
	IShellItemArray *_psiaSelection;
	IUnknown *_punkSite;
	wil::unique_cotaskmem_string _spszCommandName;
	ComPtr<COpenWithExUI> _spOpenWithUI;
	ComPtr<IServiceProvider> _spSiteProxy;

	HRESULT _GetSelectedItem(REFIID riid, LPVOID *ppv);
	HRESULT _InstallApplication(IShellItem2 *psi, REFIID riid, void **ppva);
	HRESULT _InitDelegate(IExecuteCommand *pxc);
	HRESULT _InstallHandlerIfNeededAndInvoke();
	bool _AllowSetDefault();
	bool _IsOpenWithUndecidedAppUrl();
	void _DoExecute();

public:
	// IInitializeCommand
	STDMETHODIMP Initialize(LPCWSTR pszCommandName, IPropertyBag *) override;

	// IExecuteCommandApplicationHostEnvironment
	STDMETHODIMP GetValue(AHE_TYPE *pahe) override;

	// IServiceProvider
	STDMETHODIMP QueryService(REFGUID serviceId, REFIID riid, void **ppv) override;

	// IOpenWithLauncher
	STDMETHODIMP Launch(HWND hwndOwner, LPCWSTR pszFile, IMMERSIVE_OPENWITH_FLAGS flags) override;

	// IClassFactory
	STDMETHODIMP CreateInstance(IUnknown *, REFIID riid, void **ppv) override;
	STDMETHODIMP LockServer(BOOL) override;

	// IObjectWithSite
	STDMETHODIMP SetSite(IUnknown *punkSite) override;
	STDMETHODIMP GetSite(REFIID riid, void **ppvSite) override;

	// IExecuteCommand
	STDMETHODIMP SetKeyState(DWORD grfKeyState) override;
	STDMETHODIMP SetParameters(LPCWSTR pszParameters) override;
	STDMETHODIMP SetPosition(POINT pt) override;
	STDMETHODIMP SetShowWindow(int nShow) override;
	STDMETHODIMP SetNoShowUI(BOOL fNoShowUI) override;
	STDMETHODIMP SetDirectory(LPCWSTR pszDirectory) override;
	STDMETHODIMP Execute() override;

	// IObjectWithAssociationElement
	STDMETHODIMP SetAssocElement(IAssociationElement *pae) override;
	STDMETHODIMP GetAssocElement(REFIID riid, void **ppv) override;

	// IObjectWithSelection
	STDMETHODIMP SetSelection(IShellItemArray *psia) override;
	STDMETHODIMP GetSelection(REFIID riid, void **ppv) override;

	// COpenWithExLauncher
	HRESULT RunMessageLoop();
};
#pragma once
#include "openwithex_priv.h"
#include "interfaces.h"

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
	// IExecuteCommandApplicationHostEnvironment
	STDMETHODIMP GetValue(AHE_TYPE *pahe) override;

	// IInitializeCommand
	STDMETHODIMP Initialize(LPCWSTR pszCommandLine, IPropertyBag *) override;

	// IServiceProvider
	STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv) override;

	// IOpenWithLauncher
	STDMETHODIMP Launch(HWND hwndOwner, LPCWSTR pszFile, IMMERSIVE_OPENWITH_FLAGS flags) override;

	// IClassFactory
	STDMETHODIMP CreateInstance(IUnknown *, REFIID riid, void **ppv) override;
	STDMETHODIMP LockServer(BOOL) override;

	// IObjectWithSite
	STDMETHODIMP GetSite(REFIID riid, void **ppvSite) override;
	STDMETHODIMP SetSite(IUnknown *punkSite) override;

	// IObjectWithAssociationElement
	STDMETHODIMP SetAssocElement(IAssociationElement *pae) override;
	STDMETHODIMP GetAssocElement(REFIID riid, void **ppv) override;

	// IObjectWithSelection
	STDMETHODIMP SetSelection(IShellItemArray *psia) override;
	STDMETHODIMP GetSelection(REFIID riid, void **ppv) override;
};
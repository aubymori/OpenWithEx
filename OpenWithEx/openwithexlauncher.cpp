#include "openwithexlauncher.h"
#include <stdio.h>
#include <shlwapi.h>

const IID s_supportedInterfaces[] = {
	IID_IUnknown,
	IID_IExecuteCommandApplicationHostEnvironment,
	IID_IServiceProvider,
	IID_IObjectWithSite,
	IID_IObjectWithSelection,
	IID_IInitializeCommand,
	IID_IOpenWithLauncher,
	IID_IClassFactory
};

#pragma region "IUnknown"
HRESULT COpenWithExLauncher::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
	LPWSTR lpString = nullptr;
	(void)StringFromCLSID(riid, &lpString);
	if (lpString && lpString[0])
	{
		debuglog(L"[COpenWithExLauncher::QueryInterface] Querying %s\n", lpString);
		CoTaskMemFree(lpString);
	}

	if (!ppvObj)
	{
		debuglog(L"[COpenWithExLauncher::QueryInterface] ppvObj is null\n");
		return E_INVALIDARG;
	}

	*ppvObj = nullptr;
	for (int i = 0; i < ARRAYSIZE(s_supportedInterfaces); i++)
	{
		if (riid == s_supportedInterfaces[i])
		{
			debuglog(L"[COpenWithExLauncher::QueryInterface] Interface found\n");
			*ppvObj = (LPVOID)this;
			AddRef();
			return S_OK;
		}
	}

	debuglog(L"[COpenWithExLauncher::QueryInterface] No such interface supported\n");
	return E_NOINTERFACE;
}


ULONG COpenWithExLauncher::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

ULONG COpenWithExLauncher::Release()
{
	ULONG ulRefCount = InterlockedDecrement(&m_cRef);
	if (0 == ulRefCount)
	{
		delete this;
	}
	return ulRefCount;
}
#pragma endregion // "IUnknown"

#pragma region "IExecuteCommandApplicationHostEnvironment"
HRESULT COpenWithExLauncher::GetValue(AHE_TYPE *pahe)
{
	// Windows will pass in null for this and complain if it fails.
	if (pahe)
	{
		// Matching modern Open With, despite being a desktop app. I don't think it
		// matters much, due to the above comment.
		*pahe = AHE_IMMERSIVE;
	}
	return S_OK;
}
#pragma endregion // "IExecuteCommandApplicationHostEnvironment"

#pragma region "IServiceProvider"
HRESULT COpenWithExLauncher::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
	LPWSTR lpString = nullptr;
	LPWSTR lpString2 = nullptr;
	(void)StringFromCLSID(guidService, &lpString);
	(void)StringFromCLSID(riid, &lpString2);
	if (lpString && lpString[0] && lpString2 && lpString2[0])
	{
		debuglog(L"[COpenWithExLauncher::QueryService] Querying service %s with IID %s\n", lpString, lpString2);
		CoTaskMemFree(lpString);
		CoTaskMemFree(lpString2);
	}
	return E_NOTIMPL;// IUnknown_QueryService(this, guidService, riid, ppvObject);
}
#pragma endregion // "IServiceProvider"

#pragma region "IObjectWithSite"
HRESULT COpenWithExLauncher::GetSite(REFIID riid, void **ppvObject)
{
	if (m_pSite)
		return m_pSite->QueryInterface(riid, ppvObject);
	else
		return E_FAIL;
}

HRESULT COpenWithExLauncher::SetSite(IUnknown *pUnkSite)
{
	IUnknown_Set(&m_pSite, pUnkSite);
	return S_OK;
}
#pragma endregion // "IObjectWithSite"

#pragma region "IObjectWithSelection"
HRESULT COpenWithExLauncher::GetSelection(REFIID riid, void **ppv)
{
	if (m_pSelection)
		return m_pSelection->QueryInterface(riid, ppv);
	else
		return E_FAIL;
}

HRESULT COpenWithExLauncher::SetSelection(IShellItemArray *psia)
{
	IUnknown_Set((IUnknown **)&m_pSelection, psia);
	return S_OK;
}
#pragma endregion // "IObjectWithSelection"

#pragma region "IInitializeCommand"
HRESULT COpenWithExLauncher::Initialize(LPCWSTR pszCommandName, IPropertyBag *ppb)
{
	return S_OK;
}
#pragma endregion // "IInitializeCommand"

#pragma region "IOpenWithLauncher"
HRESULT COpenWithExLauncher::Launch(HWND hWndParent, LPCWSTR lpszPath, IMMERSIVE_OPENWITH_FLAGS flags)
{
	debuglog(
		L"[COpenWithExLauncher] IOpenWithLauncher::Launch info:"
		L"\nhWndParent: 0x % X\nlpszPath: % s\nflags : 0x % X\n",
		hWndParent, lpszPath, flags
	);
	return S_OK;
}
#pragma endregion // "IOpenWithLauncher"

#pragma region "IClassFactory"
HRESULT COpenWithExLauncher::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject)
{
	if (!ppvObject)
	{
		debuglog(L"[COpenWithExLauncher::CreateInstance] ppvObject is null\n");
		return E_INVALIDARG;
	}
	*ppvObject = this;
	return S_OK;
	/*LPWSTR lpString = nullptr;
	(void)StringFromCLSID(riid, &lpString);
	if (lpString && lpString[0])
	{
		MessageBoxW(NULL, lpString, NULL, NULL);
		CoTaskMemFree(lpString);
	}
	return E_FAIL;*/
}

HRESULT COpenWithExLauncher::LockServer(BOOL fLock)
{
	return S_OK;
}
#pragma endregion // "IClassFactory"

#pragma region "COpenWithExLauncher"
COpenWithExLauncher::COpenWithExLauncher()
	: m_pSite(nullptr)
	, m_pSelection(nullptr)
{

}

COpenWithExLauncher::~COpenWithExLauncher()
{
	if (m_pSite)
		m_pSite->Release();

	if (m_pSelection)
		m_pSelection->Release();
}
#pragma endregion // "COpenWithExLauncher"
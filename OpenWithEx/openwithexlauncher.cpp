#include "openwithexlauncher.h"
#include <stdio.h>
#include <shlwapi.h>
#include <stdarg.h>
#include "wil/com.h"
#include "wil/resource.h"

// Disable debug calls if we are in release mode:
#ifdef NDEBUG
	#define Log(...)
	#define DebugSetMethodName(METHOD_NAME)
	#define LogReturn(RETURNVALUE) { return RETURNVALUE; }
#endif

#ifndef NDEBUG
	#define DebugSetMethodName(METHOD_NAME) LPCWSTR method = METHOD_NAME;

	#define LogReturn(RETURNVALUE, METHODNAME, FORMAT, ...)       \
	{                                                             \
		Log(METHODNAME, FORMAT, __VA_ARGS__);                     \
        Log(METHODNAME, L"<return value = %s>", L#RETURNVALUE);   \
        return RETURNVALUE;                                       \
	}
#endif

const IID s_supportedInterfaces[] = {
	IID_IUnknown,
	IID_IExecuteCommandApplicationHostEnvironment,
	IID_IServiceProvider,
	IID_IObjectWithSite,
	IID_IObjectWithSelection,
	IID_IInitializeCommand,
	IID_IExecuteCommand,
	IID_IOpenWithLauncher,
	IID_IClassFactory
};

#pragma region "Debugging"
#ifndef NDEBUG
DWORD COpenWithExLauncher::s_instCounter = 0;

void COpenWithExLauncher::Log(const wchar_t *szMethodName, const wchar_t *format, ...)
{
	va_list args;
	va_start(args, format);

	WCHAR buffer[2048];

	vswprintf_s(buffer, format, args);
	wprintf_s(L"[%s:%d] %s", szMethodName, m_instId, buffer);

	va_end(args);
}
#endif
#pragma endregion

#pragma region "IUnknown"
#define QI_PUT_OUT(INTERFACE)                              \
	if (riid == IID_ ## INTERFACE)                         \
	{                                                      \
		*ppvObj = static_cast<INTERFACE *>(this);          \
		Log(method, L"Found interface %s\n", L#INTERFACE); \
        bFailedToFind = false;                             \
	}

HRESULT COpenWithExLauncher::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	DebugSetMethodName(L"COpenWithExLauncher::QueryInterface");
	HRESULT hr = E_NOINTERFACE;

	Log(method, L"Entered method\n");

	LPWSTR lpString = nullptr;
	(void)StringFromCLSID(riid, &lpString);
	if (lpString && lpString[0])
	{
		Log(method, L"Querying %s\n", lpString);
		CoTaskMemFree(lpString);
	}

	if (!ppvObj)
	{
		Log(method, L"ppvObj is null\n");
		Log(method, L"Exiting method\n");
		return E_INVALIDARG;
	}

	*ppvObj = nullptr;

	bool bFailedToFind = true;

	QI_PUT_OUT(IUnknown)
	QI_PUT_OUT(IExecuteCommandApplicationHostEnvironment)
	QI_PUT_OUT(IServiceProvider)
	QI_PUT_OUT(IObjectWithSite)
	QI_PUT_OUT(IObjectWithSelection)
	QI_PUT_OUT(IInitializeCommand)
	QI_PUT_OUT(IExecuteCommand)
	QI_PUT_OUT(IOpenWithLauncher)
	QI_PUT_OUT(IClassFactory);

	if (!bFailedToFind)
	{
		hr = S_OK;
		AddRef();
	}
	else
	{
		Log(method, L"No such interface supported\n");
		hr = E_NOINTERFACE;
	}

	Log(method, L"Exiting method\n");
	return hr;
}


ULONG COpenWithExLauncher::AddRef()
{
	DebugSetMethodName(L"COpenWithExLauncher::AddRef");

	Log(method, L"Entered method\n");
	auto rv = InterlockedIncrement(&m_cRef);
	Log(method, L"Exiting method\n");

	return rv;
}

ULONG COpenWithExLauncher::Release()
{
	DebugSetMethodName(L"COpenWithExLauncher::Release");

	Log(method, L"Entered method\n");

	ULONG ulRefCount = InterlockedDecrement(&m_cRef);
	if (0 == ulRefCount)
	{
		delete this;
		debuglog(L"[COpenWithExLauncher::Release] Exiting method for now-deleted object\n");
	}
	else
	{
		Log(method, L"Exiting method\n");
	}

	return ulRefCount;
}
#pragma endregion // "IUnknown"

#pragma region "IExecuteCommandApplicationHostEnvironment"
HRESULT COpenWithExLauncher::GetValue(AHE_TYPE *pahe)
{
	DebugSetMethodName(L"COpenWithExLauncher::GetValue");

	Log(method, L"Entered method\n");

	// Windows will pass in null for this and complain if it fails.
	if (pahe)
	{
		// Matching modern Open With, despite being a desktop app. I don't think it
		// matters much, due to the above comment.
		*pahe = AHE_IMMERSIVE;
	}
	else
	{
		Log(method, L"pahe is a null pointer, so the operating system wants us to fail\n");
	}

	Log(method, L"Exiting method\n");
	return S_OK;
}
#pragma endregion // "IExecuteCommandApplicationHostEnvironment"

#pragma region "IServiceProvider"
HRESULT COpenWithExLauncher::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
	DebugSetMethodName(L"COpenWithExLauncher::QueryService");

	Log(method, L"Entered method\n");

	LPWSTR lpString = nullptr;
	LPWSTR lpString2 = nullptr;
	(void)StringFromCLSID(guidService, &lpString);
	(void)StringFromCLSID(riid, &lpString2);
	if (lpString && lpString[0] && lpString2 && lpString2[0])
	{
		Log(method, L"Querying service %s with SID %s\n", lpString, lpString2);
		CoTaskMemFree(lpString);
		CoTaskMemFree(lpString2);
	}

	Log(method, L"Exiting method\n");
	return E_NOTIMPL;// IUnknown_QueryService(this, guidService, riid, ppvObject);
}
#pragma endregion // "IServiceProvider"

#pragma region "IObjectWithSite"
HRESULT COpenWithExLauncher::GetSite(REFIID riid, void **ppvObject)
{
	DebugSetMethodName(L"COpenWithExLauncher::GetSite");

	Log(method, L"Entered method (exit will not be logged)\n");

	if (m_pSite)
	{
		LogReturn(m_pSite->QueryInterface(riid, ppvObject), method, L"Exited method\n");
	}
	else
	{
		LogReturn(E_FAIL, method, L"Exited method\n");
	}
}

HRESULT COpenWithExLauncher::SetSite(IUnknown *pUnkSite)
{
	DebugSetMethodName(L"COpenWithExLauncher::SetSite");

	Log(method, L"Entered method\n");
	IUnknown_Set(&m_pSite, pUnkSite);
	Log(method, L"Exiting method\n");
	return S_OK;
}
#pragma endregion // "IObjectWithSite"

#pragma region "IObjectWithSelection"
HRESULT COpenWithExLauncher::GetSelection(REFIID riid, void **ppv)
{
	DebugSetMethodName(L"COpenWithExLauncher::GetSelection");

	Log(method, L"Entered method\n");
	if (m_pSelection)
	{
		LogReturn(m_pSelection->QueryInterface(riid, ppv), method, L"Exited method\n");
	}
	else
	{
		LogReturn(E_FAIL, method, L"Exited method\n");
	}
}

HRESULT COpenWithExLauncher::SetSelection(IShellItemArray *psia)
{
	DebugSetMethodName(L"COpenWithExLauncher::SetSelection");

	Log(method, L"Entered method\n");
	IUnknown_Set((IUnknown **)&m_pSelection, psia);
	Log(method, L"Exiting method\n");
	return S_OK;
}
#pragma endregion // "IObjectWithSelection"

#pragma region "IInitializeCommand"
HRESULT COpenWithExLauncher::Initialize(LPCWSTR pszCommandName, IPropertyBag *ppb)
{
	DebugSetMethodName(L"COpenWithExLauncher::Initialize");

	Log(method, L"Entered method\n");
	Log(method, L"Exiting method\n");
	return S_OK;
}
#pragma endregion // "IInitializeCommand"

#pragma region "IExecuteCommand"
HRESULT COpenWithExLauncher::SetKeyState(DWORD grfKeyState)
{
	DebugSetMethodName(L"COpenWithExLauncher::SetKeyState");

	Log(method, L"Entering method\n");
	Log(method, L"Exiting method\n");
	return S_OK;
}

HRESULT COpenWithExLauncher::SetParameters(__RPC__in_string LPCWSTR pszParameters)
{
	DebugSetMethodName(L"COpenWithExLauncher::SetParameters");

	Log(method, L"Entering method\n");
	Log(method, L"Exiting method\n");
	return S_OK;
}

HRESULT COpenWithExLauncher::SetPosition(POINT pt)
{
	DebugSetMethodName(L"COpenWithExLauncher::SetPosition");

	Log(method, L"Entering method\n");
	Log(method, L"Exiting method\n");
	return S_OK;
}

HRESULT COpenWithExLauncher::SetShowWindow(int nShow)
{
	DebugSetMethodName(L"COpenWithExLauncher::SetShowWindow");

	Log(method, L"Entering method\n");
	Log(method, L"Exiting method\n");
	return S_OK;
}

HRESULT COpenWithExLauncher::SetNoShowUI(BOOL fNoShowUI)
{
	DebugSetMethodName(L"COpenWithExLauncher::SetNoShowUI");

	Log(method, L"Entering method\n");
	Log(method, L"Exiting method\n");
	return S_OK;
}

HRESULT COpenWithExLauncher::SetDirectory(__RPC__in_string LPCWSTR pszDirectory)
{
	DebugSetMethodName(L"COpenWithExLauncher::SetDirectory");

	Log(method, L"Entering method\n");
	Log(method, L"Exiting method\n");
	return S_OK;
}

HRESULT COpenWithExLauncher::Execute()
{
	DebugSetMethodName(L"COpenWithExLauncher::Execute");

	Log(method, L"Entering method\n");
	Log(method, L"Exiting method\n");
	return S_OK;
}
#pragma endregion

#pragma region "IOpenWithLauncher"
HRESULT COpenWithExLauncher::Launch(HWND hWndParent, LPCWSTR lpszPath, IMMERSIVE_OPENWITH_FLAGS flags)
{
	debuglog(
		L"[COpenWithExLauncher] IOpenWithLauncher::Launch info:\n"
		L"\nhWndParent: 0x % X\nlpszPath: % s\nflags : 0x % X\n",
		hWndParent, lpszPath, flags
	);
	return S_OK;
}
#pragma endregion // "IOpenWithLauncher"

#pragma region "IClassFactory"
HRESULT COpenWithExLauncher::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject)
{
	DebugSetMethodName(L"COpenWithExLauncher::CreateInstance");

	Log(method, L"Entered method\n");

	if (!ppvObject)
	{
		Log(method, L"ppvObject is null\n");
		return E_INVALIDARG;
	}

	LPWSTR lpString = nullptr;
	(void)StringFromCLSID(riid, &lpString);
	Log(method, L"riid is %s\n", lpString);

	//void *pIntermediate = nullptr;
	//HRESULT hr = QueryInterface(riid, &pIntermediate);

	//if (FAILED(hr))
	//{
	//	Log(method, L"Exiting method (failure)\n");
	//	return E_FAIL;
	//}

	if (riid == IID_IUnknown)
	{
		Log(method, L"riid is IUnknown, putting that out...\n");
		*ppvObject = static_cast<IUnknown *>(this);
	}

	Log(method, L"Exiting method\n");
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
	DebugSetMethodName(L"COpenWithExLauncher::LockServer");

	Log(method, L"Entered method\n");
	Log(method, L"Exiting method\n");
	return S_OK;
}
#pragma endregion // "IClassFactory"

#pragma region "COpenWithExLauncher"
COpenWithExLauncher::COpenWithExLauncher()
	: m_pSite(nullptr)
	, m_pSelection(nullptr)
{
	DebugSetMethodName(L"COpenWithExLauncher::COpenWithExLauncher");

	// Intentionally not using Log because instance ID isn't set up
	// yet.
	debuglog(L"[COpenWithExLauncher::COpenWithExLauncher] Entered method\n");

#ifndef NDEBUG
	m_instId = s_instCounter++;
	Log(method, L"Set instance ID to %d\n", m_instId);
#endif

	Log(method, L"Exiting method\n");
}

COpenWithExLauncher::~COpenWithExLauncher()
{
	DebugSetMethodName(L"COpenWithExLauncher::~COpenWithExLauncher");

	Log(method, L"Entered method\n");

	if (m_pSite)
		m_pSite->Release();

	if (m_pSelection)
		m_pSelection->Release();

	Log(method, L"Exiting method\n");
}

HRESULT COpenWithExLauncher::RunMessageLoop()
{
	DebugSetMethodName(L"COpenWithExLauncher::RunMessageLoop");

	Log(method, L"Entered method\n");

	DWORD dwRegister = 0;

	Log(method, L"Entering CoRegisterClassObject\n");

	wil::com_ptr<IExecuteCommand> aaaa;
	HRESULT hr = QueryInterface(IID_IExecuteCommand, (void **)&aaaa);

	if (FAILED(hr))
	{
		Log(method, L"Failed to get IExecuteCommand\n");
		return hr;
	}

	hr = CoRegisterClassObject(CLSID_ExecuteUnknown, aaaa.get(), CLSCTX_LOCAL_SERVER, NULL, &dwRegister);
	Log(method, L"Exiting CoRegisterClassObject\n");
	if (FAILED(hr))
	{
		return hr;
	}

	MSG msg;
	while (true)
	{
		if (GetMessageW(&msg, NULL, 0, 0) <= 0)
		{
			Log(method, L"Exiting because message loop is over\n");
			CoRevokeClassObject(dwRegister);
			return hr;
		}

		if (msg.message)
		{
			Log(method, L"Received message %x %x %x %x\n", msg.message, msg.hwnd, msg.wParam, msg.lParam);
		}

		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	Log(method, L"Exiting method\n");
	return S_OK;
}
#pragma endregion // "COpenWithExLauncher"
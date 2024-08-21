#include "openwithex.h"
#include "mui.h"
#include "cantopendlg.h"
#include "openasdlg.h"
#include "noopendlg.h"
#include "openwithexlauncher.h"
#include <shlobj.h>
#include <shlwapi.h>
#include <stdio.h>
#include "wil/com.h"
#include "wil/resource.h"

HMODULE g_hAppInstance = nullptr;
HMODULE g_hMuiInstance = nullptr;

SHCreateAssocHandler_t SHCreateAssocHandler = nullptr;

WCHAR  szPath[MAX_PATH] = { 0 };
LPWSTR pszExtension = nullptr;
bool   bOverride = false;
bool   bUri = false;

int WINAPI wWinMain(
	_In_     HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_     LPWSTR    lpCmdLine,
	_In_     int       nCmdShow
)
{
#ifndef NDEBUG
	FILE *dummy;
	AllocConsole();
	SetConsoleTitleW(L"OpenWithEx");
	_wfreopen_s(&dummy, L"CONOUT$", L"w", stdout);
#endif

	debuglog(L"OpenWithEx Debug Console\n\n");

	g_hAppInstance = hInstance;
	g_hMuiInstance = GetMUIModule(g_hAppInstance);
	if (!g_hMuiInstance)
	{
		MessageBoxW(
			NULL,
			L"Failed to load language resources",
			L"OpenWithEx",
			MB_ICONERROR
		);
		return 1;
	}

	(void)CoInitialize(nullptr);

	/* Load undocumented shell32 function */
	HMODULE hShell32 = GetModuleHandleW(L"shell32.dll");
	SHCreateAssocHandler = (SHCreateAssocHandler_t)GetProcAddress(
		hShell32,
		(LPCSTR)765 // Ordinal number
	);
	if (!SHCreateAssocHandler)
	{
		LocalizedMessageBox(
			NULL,
			IDS_ERR_SHELL32,
			MB_ICONERROR
		);
		return -1;
	}

	/**
	  * HACKHACK: Windows loves to pass the full executable path as the first
	  * "argument" when there's no user arguments passed. Get the path and
	  * later compare it to prevent taking the executable name as the open
	  * with path.
	  */
	WCHAR szModulePath[MAX_PATH] = { 0 };
	GetModuleFileNameW(hInstance, szModulePath, MAX_PATH);

	int argc = 0;
	LPWSTR *argv = CommandLineToArgvW(lpCmdLine, &argc);
	for (int i = 0; i < argc; i++)
	{
		/* COM bullshit */
		if (0 == _wcsicmp(argv[i], L"-embedding"))
		{
#if 0
			LocalizedMessageBox(
				NULL,
				IDS_ERR_EMBEDDING,
				MB_ICONERROR
			);
			return -1;
#endif
			debuglog(L"Started as COM server.\n");

#if !NDEBUG && 0
			debuglog(L"Waiting for debugger...\n");
			while (!IsDebuggerPresent())
				Sleep(100);
			debuglog(L"Debugger attached!\n");
#endif
			
			wil::com_ptr<COpenWithExLauncher> powl = new COpenWithExLauncher();
			
			if (powl)
			{
				HRESULT hr = powl->RunMessageLoop();
				
				if (FAILED(hr))
				{
					LocalizedMessageBox(
						NULL,
						IDS_ERR_EMBEDDING,
						MB_ICONERROR
					);
					return -1;
				}
			}
		}
		/* Implies the type is already registered and user explicitly wants to
	       open with */
		else if (0 == _wcsicmp(argv[i], L"-override"))
		{
			bOverride = true;
		}
		/* Assume path is last arg that doesn't start with - */
		else if (i == argc - 1 && *argv[i] != L'-' && 0 != _wcsicmp(argv[i], szModulePath))
		{
			wcscpy_s(szPath, argv[i]);
		}
	}
	LocalFree(argv);

	if (!*szPath)
	{
		LocalizedMessageBox(
			NULL,
			IDS_ERR_NOPATH,
			MB_ICONERROR
		);
		return -1;
	}

	bUri = UrlIsW(szPath, URLIS_URL);
	if (!bUri)
	{
		LPWSTR pszExtension = PathFindExtensionW(szPath);

		/* Check if the file is a system file and open the no-open dialog if it is. */
		if (pszExtension && *pszExtension)
		{
			HKEY hk = GetExtensionRegKey(pszExtension);
			LSTATUS ls = RegQueryValueExW(
				hk,
				L"NoOpen",
				NULL,
				NULL,
				NULL,
				NULL
			);
			RegCloseKey(hk);
			if (ls == ERROR_SUCCESS)
			{
				CNoOpenDlg noDlg(szPath);
				INT_PTR result = noDlg.ShowDialog(NULL);
				if (result == IDCANCEL)
				{
					return 0;
				}
			}
		}

		if (!bOverride && !SHRestricted(REST_NOINTERNETOPENWITH)
			&& pszExtension && *pszExtension)
		{
			CCantOpenDlg coDlg(szPath);
			INT_PTR result = coDlg.ShowDialog(NULL);
			if (result == IDCANCEL)
			{
				return 0;
			}
			else if (result == IDD_CANTOPEN_USEWEB)
			{
				WCHAR szFormat[MAX_PATH] = { 0 };
				WCHAR szUrl[MAX_PATH] = { 0 };
				LoadStringW(g_hMuiInstance, IDS_SEARCH_FORMAT, szFormat, MAX_PATH);
				swprintf_s(szUrl, szFormat, pszExtension + 1);
				ShellExecuteW(
					NULL,
					L"open",
					szUrl,
					NULL,
					NULL,
					SW_SHOWNORMAL
				);
				return 0;
			}
		}
	}

	COpenAsDlg oaDialog(szPath, bOverride, bUri);
	oaDialog.ShowDialog(NULL);

	CoUninitialize();
	return 0;
}
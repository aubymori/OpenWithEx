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
#include "wil/registry.h"

HMODULE g_hAppInstance = nullptr;
HMODULE g_hMuiInstance = nullptr;

SHCreateAssocHandler_t SHCreateAssocHandler = nullptr;

WCHAR szPath[MAX_PATH] = { 0 };

void OpenDownloadURL(LPCWSTR pszExtension)
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
}

void ShowOpenWithDialog(HWND hWndParent, LPCWSTR lpszPath, IMMERSIVE_OPENWITH_FLAGS flags)
{
	bool fUri = false;
	bool fPreregistered = false;

	fUri = UrlIsW(lpszPath, URLIS_URL) || (flags & IMMERSIVE_OPENWITH_PROTOCOL);
	if (!fUri)
	{
		LPWSTR pszExtension = PathFindExtensionW(lpszPath);
		wil::unique_hkey hk;
		GetExtensionRegKey(pszExtension, &hk);
		fPreregistered = (hk.get() != NULL);

		/* Check if the file is a system file and open the no-open dialog if it is. */
		if (pszExtension && *pszExtension)
		{	
			LSTATUS ls = RegQueryValueExW(
				hk.get(),
				L"NoOpen",
				NULL,
				NULL,
				NULL,
				NULL
			);
			if (ls == ERROR_SUCCESS)
			{
				CNoOpenDlg noDlg(lpszPath);
				INT_PTR result = noDlg.ShowDialog(hWndParent);
				if (result == IDCANCEL)
				{
					return;
				}
			}
		}

		/* The Can't open dialog is only shown on files with an extension
		   that is not already registered. */
		if (!fPreregistered && !SHRestricted(REST_NOINTERNETOPENWITH)
			&& pszExtension && *pszExtension)
		{
			CCantOpenDlg coDlg(lpszPath);
			INT_PTR result = coDlg.ShowDialog(hWndParent);
			if (result == IDCANCEL)
			{
				return;
			}
			else if (result == IDD_CANTOPEN_USEWEB)
			{
				OpenDownloadURL(pszExtension);
				return;
			}
		}
	}

	COpenAsDlg oaDialog(lpszPath, flags, fUri, fPreregistered);
	oaDialog.ShowDialog(hWndParent);
}

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
			debuglog(L"Started as COM server.\n");			
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
				return 0;
			}
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

	ShowOpenWithDialog(NULL, szPath, IMMERSIVE_OPENWITH_OVERRIDE);

	CoUninitialize();
	return 0;
}
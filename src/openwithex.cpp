#include "openwithex.h"
#include "cantopendlg.h"
#include "vistaopenasdlg.h"
#include "xpopenasdlg.h"
#include "classicopenasdlg.h"
#include "noopendlg.h"
#include "openwithexlauncher.h"
#include "assocuserchoice.h"
#include <shlobj.h>
#include <shlwapi.h>
#include <stdio.h>
#include "wil/com.h"
#include "wil/resource.h"
#include "wil/registry.h"

HMODULE         g_hInst     = nullptr;
HMODULE         g_hShell32  = nullptr;
OPENWITHEXSTYLE g_style     = OWXS_VISTA;

SHCreateAssocHandler_t SHCreateAssocHandler = nullptr;
IsBlockedFromOpenWithBrowse_t IsBlockedFromOpenWithBrowse = nullptr;

WCHAR szPath[MAX_PATH] = { 0 };

void OpenDownloadURL(LPCWSTR pszExtension)
{
	WCHAR szUrl[MAX_PATH] = { 0 };
	swprintf_s(szUrl, L"http://go.microsoft.com/fwlink/?LinkId=57426&Ext=%s", pszExtension + 1);
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
		fPreregistered = AssociationExists(pszExtension, fUri);

		/* Check if the file is a system file and open the no-open dialog if it is. */
		if (g_style != OWXS_NT4 && pszExtension && *pszExtension)
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
		if ((g_style == OWXS_VISTA || g_style == OWXS_XP)
		&& !fPreregistered && !SHRestricted(REST_NOINTERNETOPENWITH)
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

	CBaseOpenAsDlg *pDialog = nullptr;
	switch (g_style)
	{
		case OWXS_VISTA:
			pDialog = new CVistaOpenAsDlg(lpszPath, flags, fUri, fPreregistered);
			break;
		case OWXS_XP:
			pDialog = new CXPOpenAsDlg(lpszPath, flags, fUri, fPreregistered);
			break;
		case OWXS_2K:
		case OWXS_NT4:
			pDialog = new CClassicOpenAsDlg(lpszPath, flags, fUri, fPreregistered);
			break;
	}
	pDialog->ShowDialog(hWndParent);
	delete pDialog;
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

	g_hInst = hInstance;

	(void)CoInitialize(nullptr);

	/* Read user style option */
	wil::unique_hkey hk;
	RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\OpenWithEx", NULL, KEY_READ, &hk);
	if (hk.get())
	{
		DWORD dwValue = 0;
		DWORD dwSize = sizeof(DWORD);
		RegQueryValueExW(hk.get(), L"Style", nullptr, nullptr, (LPBYTE)&dwValue, &dwSize);
		if (dwValue < OWXS_LAST)
			g_style = (OPENWITHEXSTYLE)dwValue;
	}

	/* Load undocumented functions */
	g_hShell32 = GetModuleHandleW(L"shell32.dll");
	SHCreateAssocHandler = (SHCreateAssocHandler_t)GetProcAddress(
		g_hShell32,
		(LPCSTR)765 // Ordinal number
	);
	IsBlockedFromOpenWithBrowse = (IsBlockedFromOpenWithBrowse_t)GetProcAddress(
		g_hShell32,
		(LPCSTR)779
	);
	if (!SHCreateAssocHandler || !IsBlockedFromOpenWithBrowse)
	{
		LocalizedMessageBox(
			NULL,
			IDS_ERR_UNDOC,
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
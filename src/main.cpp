#include "openwithex_priv.h"
#include "process_and_thread_refhost.h"
#include "openwithex_launcher.h"
#include "openwithex_ui.h"
#include <stdio.h>

HINSTANCE g_hinst = NULL;
OPENWITHEX_STYLE g_style = OPENWITHEX_STYLE_VISTA;

int WINAPI wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR    lpCmdLine,
	int       nShowCmd
)
{
	g_hinst = hInstance;

	CoInitialize(nullptr);

	// Read user's style
	HKEY hkey;
	if (ERROR_SUCCESS == RegOpenKeyExW(
		HKEY_CURRENT_USER,
		REGSTR_PATH_OPENWITHEX,
		0, KEY_READ,
		&hkey))
	{
		DWORD dwStyle = 0;
		DWORD cbStyle = sizeof(dwStyle);
		if (ERROR_SUCCESS == RegQueryValueExW(
			hkey,
			REGSTR_VAL_STYLE,
			nullptr, nullptr,
			(LPBYTE)&dwStyle,
			&cbStyle))
		{
			if (dwStyle < OPENWITHEX_STYLE_COUNT)
				g_style = (OPENWITHEX_STYLE)dwStyle;
		}
	}

	// CommandLineToArgvW will set the first argument to the program name if the
	// string is totally empty. This behavior is stinky so if our cmd line is
	// empty we don't even bother.
	int nArgs = 0;
	LPWSTR *ppszArgs = nullptr;
	if (lpCmdLine[0] != L'\0')
	{
		ppszArgs = CommandLineToArgvW(lpCmdLine, &nArgs);
		RETURN_LAST_ERROR_IF_NULL(ppszArgs);
	}

	// We are running as a local server.
	if (nArgs == 1 && ppszArgs[0][0] && !_wcsicmp(&ppszArgs[0][1], L"embedding"))
	{
#ifndef NDEBUG
		while (!IsDebuggerPresent())
			Sleep(100);
#endif

		ComPtr<COpenWithExLauncher> spLauncher = Make<COpenWithExLauncher>();
		if (!spLauncher)
			return E_OUTOFMEMORY;
		spLauncher->RunMessageLoop();
	}
	// User ran with some other arguments... 
	else
	{
		LPWSTR pszFile = nullptr;
		for (int i = 0; i < nArgs; i++)
		{
			if (ppszArgs[i][0] != L'-' && ppszArgs[i][0] != L'/')
			{
				pszFile = ppszArgs[i];
				break;
			}
		}

		if (!pszFile)
			return E_INVALIDARG;

		IMMERSIVE_OPENWITH_FLAGS flags = IMMERSIVE_OPENWITH_DONOT_SETDEFAULT;
		if (PathIsURLW(pszFile))
			flags |= IMMERSIVE_OPENWITH_PROTOCOL;

		ComPtr<COpenWithExUI> spOpenWithUI = Make<COpenWithExUI>();
		if (!spOpenWithUI)
			return E_OUTOFMEMORY;
		return spOpenWithUI->CreateAndShow(NULL, pszFile, flags);
	}

	return 0;
}
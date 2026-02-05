#include "openwithex_priv.h"
#include "process_and_thread_refhost.h"
#include "openwithex_launcher.h"
#include "openwithex_ui.h"
#include <stdio.h>

HINSTANCE g_hinst = NULL;

int WINAPI wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR    lpCmdLine,
	int       nShowCmd
)
{
	g_hinst = hInstance;

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

		// TODO(aubymori): Open the dialog.
		ComPtr<COpenWithExUI> spOpenWithUI = Make<COpenWithExUI>();
		if (!spOpenWithUI)
			return E_OUTOFMEMORY;
		return spOpenWithUI->CreateAndShow(NULL, pszFile, flags);
	}

	return 0;
}
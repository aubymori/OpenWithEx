#include "openwithex_priv.h"
#include <stdio.h>

int WINAPI wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR    lpCmdLine,
	int       nShowCmd
)
{
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

	for (int i = 0; i < nArgs; i++)
	{
		MessageBoxW(NULL, ppszArgs[i], L"hi", MB_ICONINFORMATION);
	}

	return 0;
}
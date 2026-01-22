#include "openwithex_priv.h"

int WINAPI wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR    lpCmdLine,
	int       nShowCmd
)
{
	int nArgs;
	LPWSTR *ppszArgs = CommandLineToArgvW(lpCmdLine, &nArgs);

	return 0;
}
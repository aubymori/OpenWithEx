#include "mui.h"

HMODULE GetMUIModule(HMODULE hMod)
{
	HMODULE  hMui;
	WCHAR    szPath[MAX_PATH];
	WCHAR    szMuiPath[MAX_PATH];
	
	if (!GetModuleFileNameW(
		hMod,
		szPath,
		MAX_PATH
	))
	{
		return NULL;
	}

	WCHAR  szLocale[LOCALE_NAME_MAX_LENGTH];
	if (!GetUserDefaultLocaleName(
		szLocale,
		LOCALE_NAME_MAX_LENGTH
	))
	{
		return NULL;
	}

	WCHAR *pBackslash = wcsrchr(szPath, L'\\');
	if (pBackslash)
	{
		*pBackslash = L'\0';
		wsprintfW(
			szMuiPath,
			L"%s\\%s\\%s.mui",
			szPath,
			szLocale,
			pBackslash + 1
		);

		hMui = LoadLibraryW(szMuiPath);
		if (!hMui)
		{
			wsprintfW(
				szMuiPath,
				L"%s\\%s\\%s.mui",
				szPath,
				FALLBACK_LOCALE,
				pBackslash + 1
			);

			hMui = LoadLibraryW(szMuiPath);
			if (hMui)
			{
				return hMui;
			}
		}
		return hMui;
	}

	return NULL;
}

HMODULE LoadMUIModule(LPWSTR lpLibFileName)
{
	HMODULE hTemp = LoadLibraryW(lpLibFileName);

	if (hTemp)
	{
		HMODULE hRet = GetMUIModule(hTemp);
		FreeLibrary(hTemp);
		return hRet;
	}

	return NULL;
}
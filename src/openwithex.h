#pragma once
#include <windows.h>

#define VER_MAJOR              2
#define VER_MINOR              0
#define VER_REVISION           0

#define STRINGIZE_(s)          #s
#define STRINGIZE(s)           STRINGIZE_(s)

#define VER_STRING \
    STRINGIZE(VER_MAJOR) "." STRINGIZE(VER_MINOR) "." STRINGIZE(VER_REVISION)

#define MAKE_VER_DWORD(major, minor, rev) \
	(DWORD)(((major) << 16) | ((minor) << 8) | (rev))

#define VER_DWORD        MAKE_VER_DWORD(VER_MAJOR, VER_MINOR, VER_REVISION)

#define REGSTR_PATH_OPENWITHEX         L"SOFTWARE\\OpenWithEx"
#define REGSTR_VAL_STYLE               L"Style"
#define REGSTR_VAL_SETTINGSVER         L"SettingsVersion"

typedef enum _OPENWITHEX_STYLE
{
	OPENWITHEX_STYLE_7 = 0,
	OPENWITHEX_STYLE_VISTA,
	OPENWITHEX_STYLE_XP,
	OPENWITHEX_STYLE_2000,
	OPENWITHEX_STYLE_NT4,
	OPENWITHEX_STYLE_COUNT,
} OPENWITHEX_STYLE;

inline HRESULT GetUserStyle(OPENWITHEX_STYLE *pStyle)
{
	if (!pStyle)
		return E_INVALIDARG;

	*pStyle = OPENWITHEX_STYLE_7;

	HKEY hkey;
	DWORD dwErr = RegOpenKeyExW(
		HKEY_CURRENT_USER,
		REGSTR_PATH_OPENWITHEX,
		0, KEY_READ,
		&hkey);
	if (dwErr == ERROR_SUCCESS)
	{
		DWORD dwStyle = 0;
		DWORD cbStyle = sizeof(dwStyle);
		dwErr = RegQueryValueExW(
			hkey,
			REGSTR_VAL_STYLE,
			nullptr, nullptr,
			(LPBYTE)&dwStyle,
			&cbStyle);
		if (dwErr == ERROR_SUCCESS)
		{
			DWORD dwSettingsVer = 0;
			DWORD cbSettingsVer = sizeof(dwSettingsVer);
			RegQueryValueExW(
				hkey,
				REGSTR_VAL_SETTINGSVER,
				nullptr, nullptr,
				(LPBYTE)&dwSettingsVer,
				&cbSettingsVer);

			// 2.0.0 separates Vista and 7 into their own styles, which
			// shifts everything up by one.
			if (dwSettingsVer < MAKE_VER_DWORD(2,0,0))
			{
				dwStyle++;
			}

			if (dwStyle >= OPENWITHEX_STYLE_COUNT)
				dwStyle = OPENWITHEX_STYLE_7;

			*pStyle = (OPENWITHEX_STYLE)dwStyle;
			return S_OK;
		}
	}

	return HRESULT_FROM_WIN32(dwErr);
}
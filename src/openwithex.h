#pragma once

#include <windows.h>
#include <stdio.h>
#include <shobjidl.h>
#include "util.h"

#define VER_MAJOR              1
#define VER_MINOR              1
#define VER_REVISION           0
#define VER_STRING       "1.1.0"

extern HMODULE g_hInst;
extern HMODULE g_hShell32;
extern enum OPENWITHEXSTYLE
{
	OWXS_VISTA,
	OWXS_XP,
	OWXS_2K,
	OWXS_NT4,
	OWXS_LAST,
} g_style;

enum IMMERSIVE_OPENWITH_FLAGS
{
	IMMERSIVE_OPENWITH_NONE = 0x0,
	IMMERSIVE_OPENWITH_OVERRIDE = 0x1,
	IMMERSIVE_OPENWITH_DONOT_EXEC = 0x4,
	IMMERSIVE_OPENWITH_PROTOCOL = 0x8,
	IMMERSIVE_OPENWITH_URL = 0x10,
	IMMERSIVE_OPENWITH_USEPOSITION = 0x20,
	IMMERSIVE_OPENWITH_DONOT_SETDEFAULT = 0x40,
	IMMERSIVE_OPENWITH_ACTION = 0x80,
	IMMERSIVE_OPENWITH_ALLOW_EXECDEFAULT = 0x100,
	IMMERSIVE_OPENWITH_NONEDP_TO_EDP = 0x200,
	IMMERSIVE_OPENWITH_EDP_TO_NONEDP = 0x400,
	IMMERSIVE_OPENWITH_CALLING_IN_APP = 0x800,
};

void OpenDownloadURL(LPCWSTR pszExtension);
void ShowOpenWithDialog(HWND hWndParent, LPCWSTR lpszPath, IMMERSIVE_OPENWITH_FLAGS flags);

typedef HRESULT (WINAPI *SHCreateAssocHandler_t)(UINT uFlags, LPCWSTR pszExt, LPCWSTR pszApp, IAssocHandler **ppah);
extern SHCreateAssocHandler_t SHCreateAssocHandler;

typedef bool (WINAPI *IsBlockedFromOpenWithBrowse_t)(LPCWSTR lpszPath);
extern IsBlockedFromOpenWithBrowse_t IsBlockedFromOpenWithBrowse;

DEFINE_GUID(CLSID_ExecuteUnknown, 0xE44E9428, 0xBDBC, 0x4987, 0xA0,0x99, 0x40,0xDC,0x8F,0xD2,0x55,0xE7);

inline void debuglog(const wchar_t *format, ...)
{
#ifndef NDEBUG
	va_list args;
	va_start(args, format);
	vwprintf_s(format, args);
#endif
}

/* Icons */
#define IDI_OPENWITH 100

/* Dialogs */
#define IDD_OPENWITH               200
#define IDD_OPENWITH_WITHDESC      201
#define IDD_OPENWITH_PROTOCOL      202
#define IDD_CANTOPEN               203
#define IDD_NOOPEN                 204
#define IDD_OPENWITH_XP            205
#define IDD_OPENWITH_WITHDESC_XP   206
#define IDD_OPENWITH_PROTOCOL_XP   207
#define IDD_CANTOPEN_XP            208
#define IDD_NOOPEN_XP              209
#define IDD_OPENWITH_2K            210
#define IDD_OPENWITH_WITHDESC_2K   211
#define IDD_OPENWITH_PROTOCOL_2K   212
#define IDD_OPENWITH_NT4           213
#define IDD_OPENWITH_WITHDESC_NT4  214
#define IDD_OPENWITH_PROTOCOL_NT4  215

/* Controls */
#define IDD_OPENWITH_ICON         300
#define IDD_OPENWITH_FILE         301
#define IDD_OPENWITH_PROGLIST     302
#define IDD_OPENWITH_ASSOC        303
#define IDD_OPENWITH_BROWSE       304
#define IDD_OPENWITH_DESC         305
#define IDD_OPENWITH_LINK         306
#define IDD_OPENWITH_TEXT         307
#define IDD_OPENWITH_EXT          308

#define IDD_CANTOPEN_ICON         400
#define IDD_CANTOPEN_FILE         401
#define IDD_CANTOPEN_USEWEB       402
#define IDD_CANTOPEN_OPENWITH     403

#define IDD_NOOPEN_ICON           500
#define IDD_NOOPEN_LABEL          501
#define IDD_NOOPEN_OPENWITH       502

/* Strings */
#define IDS_ERR_EMBEDDING         1000
#define IDS_ERR_NOPATH            1001
#define IDS_ERR_UNDOC             1002
#define IDS_RECOMMENDED           1003
#define IDS_OTHER                 1004
#define IDS_PROGRAMS              1005
#define IDS_ALLFILES              1006
#define IDS_BROWSETITLE           1007
#define IDS_RECOMMENDED_XP        1008
#define IDS_OTHER_XP              1009
#define IDS_BROWSETITLE_XP        1010

// Extra newline required because we are included in an .rc file:

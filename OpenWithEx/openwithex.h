#pragma once

#include <windows.h>
#include <stdio.h>
#include <shobjidl.h>
#include "util.h"

/* Uncomment to build with XP-style dialogs */
//#define XP

extern HMODULE g_hAppInstance;
extern HMODULE g_hMuiInstance;

enum IMMERSIVE_OPENWITH_FLAGS
{
	IOWF_DEFAULT            = 0x00000000,
	IOWF_ALLOW_REGISTRATION = 0x00000001,
	IOWF_FORCE_REGISTRATION = 0x00000004,
	IOWF_FILE_IS_URI        = 0x00000008
};

void ShowOpenWithDialog(HWND hWndParent, LPCWSTR lpszPath, IMMERSIVE_OPENWITH_FLAGS flags);

typedef HRESULT(WINAPI *SHCreateAssocHandler_t)(UINT uFlags, LPCWSTR pszExt, LPCWSTR pszApp, IAssocHandler **ppah);
extern SHCreateAssocHandler_t SHCreateAssocHandler;

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
#define IDD_OPENWITH          200
#define IDD_OPENWITH_WITHDESC 201
#define IDD_CANTOPEN          202
#define IDD_NOOPEN            203

/* Controls */
#define IDD_OPENWITH_ICON     300
#define IDD_OPENWITH_FILE     301
#define IDD_OPENWITH_LISTVIEW 302
#define IDD_OPENWITH_ASSOC    303
#define IDD_OPENWITH_BROWSE   304
#define IDD_OPENWITH_DESC     305

#define IDD_CANTOPEN_ICON     306
#define IDD_CANTOPEN_FILE     307
#define IDD_CANTOPEN_USEWEB   308
#define IDD_CANTOPEN_OPENWITH 309

#define IDD_NOOPEN_ICON       310
#define IDD_NOOPEN_LABEL      311
#define IDD_NOOPEN_OPENWITH   312

/* Strings */
#define IDS_ERR_EMBEDDING     400
#define IDS_ERR_NOPATH        401
#define IDS_ERR_SHELL32       402
#define IDS_SEARCH_FORMAT     403
#define IDS_RECOMMENDED       404
#define IDS_OTHER             405
#define IDS_PROGRAMS          406
#define IDS_ALLFILES          407
#define IDS_BROWSETITLE       408

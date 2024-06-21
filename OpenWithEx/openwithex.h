#include <windows.h>
#include <shobjidl.h>
#include "util.h"

#ifndef _openwithex_h_
#define _openwithex_h_

/* Uncomment to build with XP-style dialogs */
//#define XP

extern HMODULE g_hAppInstance;
extern HMODULE g_hMuiInstance;

typedef HRESULT(WINAPI *SHCreateAssocHandler_t)(UINT uFlags, LPCWSTR pszExt, LPCWSTR pszApp, IAssocHandler **ppah);
extern SHCreateAssocHandler_t SHCreateAssocHandler;

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

#endif

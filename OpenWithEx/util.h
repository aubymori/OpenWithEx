#include <windows.h>

#ifndef _util_h_
#define _util_h_

int LocalizedMessageBox(HWND hWndParent, UINT uMsgId, UINT uType);
HKEY GetExtensionRegKey(LPCWSTR lpszExtension);

#endif

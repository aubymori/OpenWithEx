#pragma once

#include <windows.h>

int LocalizedMessageBox(HWND hWndParent, UINT uMsgId, UINT uType);
bool GetExtensionRegKey(LPCWSTR lpszExtension, HKEY *pHkOut);
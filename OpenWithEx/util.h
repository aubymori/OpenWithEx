#pragma once

#include <windows.h>

int LocalizedMessageBox(HWND hWndParent, UINT uMsgId, UINT uType);
HKEY GetExtensionRegKey(LPCWSTR lpszExtension);
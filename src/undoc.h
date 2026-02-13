#pragma once
#include "openwithex_priv.h"
#include <initguid.h>

DEFINE_GUID(CLSID_AssocProgidElement, 0x9016D0DD, 0x7C41, 0x46CC, 0xA6,0x64, 0xBF,0x22,0xF7,0xCB,0x18,0x6A);

EXTERN_C HRESULT WINAPI AssocCreateElement(REFCLSID clsid, REFIID riid, LPVOID *ppv);
#pragma once
#include "openwithex.h"

DEFINE_GUID(IID_IOpenWithLauncher, 0x6A283FE2, 0xECFA, 0x4599, 0x91,0xC4, 0xE8,0x09,0x57,0x13,0x7B,0x26);

MIDL_INTERFACE("6A283FE2-ECFA-4599-91C4-E80957137B26")
IOpenWithLauncher : public IUnknown
{
public:
	STDMETHOD(Launch)(HWND hWndParent, LPCWSTR lpszPath, IMMERSIVE_OPENWITH_FLAGS flags) PURE;
};
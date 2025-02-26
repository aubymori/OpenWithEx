#pragma once
#include "openwithex.h"

DEFINE_GUID(IID_IObjectWithOpenWithFlags, 0x9D923EDC, 0xB7A9, 0x4F77, 0x99,0x33, 0x28,0x4E,0x7E,0x2B,0x25,0x36);

MIDL_INTERFACE("9D923EDC-B7A9-4F77-9933-284E7E2B2536")
IObjectWithOpenWithFlags : IUnknown
{
public:
	STDMETHOD(get_Flags)(IMMERSIVE_OPENWITH_FLAGS *out) PURE;
};
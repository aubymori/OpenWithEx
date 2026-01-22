#pragma once
#include "openwithex_priv.h"

MIDL_INTERFACE("94724f59-eb2c-4efb-ad2b-8538f6496f7d")
IOpenWithTypeOverride : IUnknown
{
    STDMETHOD(GetOpenWithTypeOverride)(LPWSTR *) PURE;
};
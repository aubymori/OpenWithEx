#pragma once
#include "openwithex_priv.h"

template <typename T>
void SafeRelease(T **ppT)
{
    T *pT = *ppT;
    *ppT = nullptr;
    if (pT)
        pT->Release();
}

STDAPI BindCtx_SetMode(IBindCtx *pbcIn, DWORD grfMode, IBindCtx **ppbcOut);
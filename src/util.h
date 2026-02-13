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

STDAPI IUnknown_GetSelection(IUnknown *punk, REFIID riid, LPVOID *ppv);

STDAPI IShellItemArray_GetItemAt(IShellItemArray *psia, DWORD dwIndex, REFIID riid, LPVOID *ppv);
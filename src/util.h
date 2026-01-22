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
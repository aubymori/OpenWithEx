#pragma once
#include "openwithex_priv.h"

inline HRESULT ResultFromWin32(__in DWORD dwErr)
{
    return HRESULT_FROM_WIN32(dwErr);
}

inline HRESULT ResultFromLastError()
{
    return ResultFromWin32(GetLastError());
}

inline HRESULT ResultFromKnownLastError()
{
    HRESULT hr = ResultFromLastError();
    return (SUCCEEDED(hr) ? E_FAIL : hr);
}

inline HRESULT ResultFromWin32Bool(BOOL b)
{
    return b ? S_OK : ResultFromKnownLastError();
}

inline HRESULT ResultFromWin32Count(UINT cchResult, UINT cchBuffer)
{
    return cchResult && cchResult <= cchBuffer ? S_OK : ResultFromWin32(ERROR_INSUFFICIENT_BUFFER);
}

template <typename T>
void SafeRelease(T **ppT)
{
    T *pT = *ppT;
    *ppT = nullptr;
    if (pT)
        pT->Release();
}

template <typename T>
HRESULT SetInterface(T **ppT, T *punk)
{
    SafeRelease(ppT);
    return punk ? punk->QueryInterface(IID_PPV_ARGS(ppT)) : E_NOINTERFACE;
}

STDAPI BindCtx_SetMode(IBindCtx *pbcIn, DWORD grfMode, IBindCtx **ppbcOut);

STDAPI IUnknown_GetSelection(IUnknown *punk, REFIID riid, LPVOID *ppv);

STDAPI IShellItemArray_GetItemAt(IShellItemArray *psia, DWORD dwIndex, REFIID riid, LPVOID *ppv);

STDAPI IUnknown_GetParentWindow(IUnknown *punkSite, HWND *phwnd);
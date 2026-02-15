#include "util.h"

STDAPI BindCtx_SetMode(IBindCtx *pbcIn, DWORD grfMode, IBindCtx **ppbcOut)
{
    HRESULT hr = S_OK;

    *ppbcOut = pbcIn;
    if (pbcIn)
    {
        pbcIn->AddRef();
    }
    else
    {
        hr = CreateBindCtx(0, ppbcOut);
    }

    if (SUCCEEDED(hr))
    {
        BIND_OPTS bo;
        bo.cbStruct = sizeof(bo);
        bo.grfFlags = 0;
        bo.grfMode = grfMode;
        bo.dwTickCountDeadline = 0;
        if (pbcIn)
        {
            hr = pbcIn->GetBindOptions(&bo);
            bo.grfMode = grfMode;
        }

        if (SUCCEEDED(hr)) // @Note: This check is new in Windows 10
            hr = (*ppbcOut)->SetBindOptions(&bo);

        if (FAILED(hr))
            SafeRelease(ppbcOut);
    }

    return hr;
}

STDAPI IUnknown_GetSelection(IUnknown *punk, REFIID riid, LPVOID *ppv)
{
    HRESULT hr = E_FAIL;
    *ppv = nullptr;

    if (punk)
    {
        IObjectWithSelection *pows = nullptr;
        hr = punk->QueryInterface(&pows);
        if (SUCCEEDED(hr))
        {
            hr = pows->GetSelection(riid, ppv);
            pows->Release();
        }
    }

    return hr;
}

STDAPI IShellItemArray_GetItemAt(IShellItemArray *psia, DWORD dwIndex, REFIID riid, LPVOID *ppv)
{
    *ppv = nullptr;

    IShellItem *psi = nullptr;
    HRESULT hr = psia->GetItemAt(0, &psi);
    if (SUCCEEDED(hr))
    {
        hr = psi->QueryInterface(riid, ppv);
        psi->Release();
    }
    return hr;
}

STDAPI IUnknown_GetParentWindow(IUnknown *punkSite, HWND *phwnd)
{
    IUnknown *punkRelease = nullptr;
    IUnknown *punk = punkSite;
    HRESULT hr;

    do
    {
        hr = IUnknown_GetWindow(punkSite, phwnd);
        if (SUCCEEDED(hr) || FAILED(IUnknown_GetSite(punk, IID_PPV_ARGS(&punk))))
        {
            punkSite = punk;
        }
        else
        {
            SafeRelease(&punkRelease);
            punkSite = punk;
            punkRelease = punk;
        }
    }
    while (punkSite && FAILED(hr));

    SafeRelease(&punkRelease);
    return hr;
}
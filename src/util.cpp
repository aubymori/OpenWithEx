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
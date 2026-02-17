#pragma once
#include "openwithex_priv.h"

/* WIL classes that are not available publicly. */

namespace wil
{
    namespace details
    {
        class DummyUnknown final : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IPersist, IOleWindow>
        {
        public:
            DummyUnknown() : m_clsid(GUID_NULL), m_hwnd(nullptr) {} // @MOD Not initialized in OG, but I've added zero initializers here to silence out warnings

            HRESULT RuntimeClassInitialize(REFGUID clsid, HWND hwnd) { m_clsid = clsid; m_hwnd = hwnd; return S_OK; }

            //~ Begin IPersist Interface
            STDMETHODIMP GetClassID(CLSID *pClassID) override { *pClassID = m_clsid; return S_OK; }
            //~ End IPersist Interface

            //~ Begin IOleWindow Interface
            STDMETHODIMP GetWindow(HWND *phwnd) override { *phwnd = m_hwnd; return S_OK; }
            STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) override { return E_NOTIMPL; }
            //~ End IOleWindow Interface

        private:
            GUID m_clsid;
            HWND m_hwnd;
        };
    }

    class ShellBindContextHelper
    {
    public:
        IBindCtx *Get() const { return m_bindCtx.Get(); }
        IBindCtx **operator&() { return ReleaseAndGetAddressOf(); }
        IBindCtx **GetAddressOf() { return m_bindCtx.GetAddressOf(); }
        IBindCtx **ReleaseAndGetAddressOf() { return m_bindCtx.ReleaseAndGetAddressOf(); }
        HRESULT CopyTo(REFIID riid, void **ppv) const { return m_bindCtx.CopyTo(riid, ppv); }

        HRESULT SetObject(LPCWSTR pszParam, IUnknown *punk)
        {
            return m_bindCtx.Get() ? m_bindCtx->RegisterObjectParam((LPOLESTR)pszParam, punk) : E_OUTOFMEMORY;
        }

        HRESULT SetNamedBoolean(LPCWSTR key)
        {
            ComPtr<details::DummyUnknown> dummyUnknown;
            HRESULT hr = MakeAndInitialize<details::DummyUnknown>(&dummyUnknown, GUID_NULL, nullptr);
            if (SUCCEEDED(hr))
                hr = SetObject(key, (IPersist *)dummyUnknown.Get());
            return hr;
        }

    private:
        ComPtr<IBindCtx> m_bindCtx;
    };
}
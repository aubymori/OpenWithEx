#pragma once
#include "openwithex_priv.h"

class CFileSysBindData : public IFileSystemBindData2
{
public:
    CFileSysBindData();

    //~ Begin IUnknown Interface
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;
    //~ End IUnknown Interface

    //~ Begin IFileSystemBindData Interface
    STDMETHODIMP SetFindData(const WIN32_FIND_DATAW* pfd) override;
    STDMETHODIMP GetFindData(WIN32_FIND_DATAW* pfd) override;
    //~ End IFileSystemBindData Interface

    //~ Begin IFileSystemBindData2 Interface
    STDMETHODIMP SetFileID(LARGE_INTEGER liFileID) override;
    STDMETHODIMP GetFileID(LARGE_INTEGER* pliFileID) override;
    STDMETHODIMP SetJunctionCLSID(REFIID clsid) override;
    STDMETHODIMP GetJunctionCLSID(CLSID* pclsid) override;
    //~ End IFileSystemBindData2 Interface

    LONG _cRef;
    LARGE_INTEGER _liFileID;
    GUID _clsidJunction;
    WIN32_FIND_DATAW _fd;
};

HRESULT SHSimpleItemFromAttributes(PCWSTR pszName, DWORD dwFileAttributes, REFIID riid, void **ppv);
#include "file_sys_bind_data.h"
#include <initguid.h>
#include "util.h"

DEFINE_GUID(CLSID_UnknownJunction, 0xFC0A77E6, 0x9D70, 0x4258, 0x97,0x83, 0x6D,0xAB,0x1D,0x0F,0xE3,0x1E);

CFileSysBindData::CFileSysBindData()
    : _cRef(1)
    , _liFileID()
    , _clsidJunction(CLSID_UnknownJunction)
    , _fd()
{
}

HRESULT CFileSysBindData::QueryInterface(REFIID riid, void **ppvObject)
{
    static const QITAB qit[] =
    {
        QITABENT(CFileSysBindData, IFileSystemBindData),
        QITABENT(CFileSysBindData, IFileSystemBindData2),
        {},
    };

    return QISearch(this, qit, riid, ppvObject);
}

ULONG CFileSysBindData::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CFileSysBindData::Release()
{
    ULONG refCount = _InterlockedDecrement(&_cRef);
    if (refCount == 0)
        delete this;
    return refCount;
}

HRESULT CFileSysBindData::SetFindData(const WIN32_FIND_DATAW *pfd)
{
    _fd = *pfd;
    return S_OK;
}

HRESULT CFileSysBindData::GetFindData(WIN32_FIND_DATAW *pfd)
{
    *pfd = _fd;
    return S_OK;
}

HRESULT CFileSysBindData::SetFileID(LARGE_INTEGER liFileID)
{
    _liFileID = liFileID;
    return S_OK;
}

HRESULT CFileSysBindData::GetFileID(LARGE_INTEGER *pliFileID)
{
    *pliFileID = _liFileID;
    return S_OK;
}

HRESULT CFileSysBindData::SetJunctionCLSID(REFIID clsid)
{
    _clsidJunction = clsid;
    return S_OK;
}

HRESULT CFileSysBindData::GetJunctionCLSID(CLSID *pclsid)
{
    HRESULT hr;
    const GUID *pguid;
    if (CLSID_UnknownJunction == _clsidJunction)
    {
        pguid = &GUID_NULL;
        hr = E_FAIL;
    }
    else
    {
        pguid = &_clsidJunction;
        hr = S_OK;
    }
    *pclsid = *pguid;
    return hr;
}

HRESULT CreateFileSystemBindData(REFIID riid, void **ppv)
{
    *ppv = nullptr;
    HRESULT hr = E_OUTOFMEMORY;

    CFileSysBindData *p = new CFileSysBindData();
    if (p)
    {
        hr = p->QueryInterface(riid, ppv);
        p->Release();
    }

    return hr;
}

HRESULT _CreateFileSysBindCtx(IBindCtx *pbcIn, const WIN32_FIND_DATAW *pfd, IBindCtx **ppbc)
{
    *ppbc = nullptr;

    IFileSystemBindData *pfsbd;
    HRESULT hr = CreateFileSystemBindData(IID_PPV_ARGS(&pfsbd));
    if (SUCCEEDED(hr))
    {
        if (pfd)
            pfsbd->SetFindData(pfd);

        hr = BindCtx_SetMode(pbcIn, STGM_CREATE, ppbc);
        if (SUCCEEDED(hr))
            (*ppbc)->RegisterObjectParam((LPOLESTR)L"File System Bind Data", pfsbd);

        pfsbd->Release();
    }

    return hr;
}

HRESULT SHCreateFileSysBindCtx(const WIN32_FIND_DATAW *pfd, IBindCtx **ppbc)
{
    return _CreateFileSysBindCtx(nullptr, pfd, ppbc);
}

HRESULT SHSimpleItemFromAttributes(PCWSTR pszName, DWORD dwFileAttributes, REFIID riid, void **ppv)
{
    *ppv = nullptr;

    WIN32_FIND_DATAW fd = {};
    fd.dwFileAttributes = dwFileAttributes;

    IBindCtx *pbc;
    HRESULT hr = SHCreateFileSysBindCtx(&fd, &pbc);
    if (SUCCEEDED(hr))
    {
        hr = SHCreateItemFromParsingName(pszName, pbc, riid, ppv);
        pbc->Release();
    }

    return hr;
}
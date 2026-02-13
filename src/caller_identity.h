#pragma once
#include "openwithex_priv.h"
#include "interfaces.h"
#include "util.h"

enum RUNTIMEBROKER_CALLERIDENTITY_CHECK
{
    RCC_ASSERT_IF_RUNTIMEBROKER,
    RCC_FAIL_IF_RUNTIMEBROKER,
    RCC_ALLOW_IF_RUNTIMEBROKER,
};

enum PROCESS_UICONTEXT
{
	PROCESS_UICONTEXT_DESKTOP           = 0x0,
	PROCESS_UICONTEXT_IMMERSIVE         = 0x1,
	PROCESS_UICONTEXT_IMMERSIVE_BROKER  = 0x2,
	PROCESS_UICONTEXT_IMMERSIVE_BROWSER = 0x3,
};

enum PROCESS_UI_FLAGS
{
	PROCESS_UIF_NONE = 0,
	PROCESS_UIF_AUTHORING_MODE = 0x1,
	PROCESS_UIF_RESTRICTIONS_DISABLED = 0x2,
};

struct PROCESS_UICONTEXT_INFORMATION
{
	PROCESS_UICONTEXT processUIContext;
	PROCESS_UI_FLAGS flags;
};

STDAPI_(BOOL) GetProcessUIContextInformation(HANDLE hProcess, PROCESS_UICONTEXT_INFORMATION *pProcessUIContextInformation);

namespace CallerIdentity
{
    static DWORD g_dwRuntimeBrokerProcessId = DWORD_MAX;
    static bool g_fRuntimeBrokerProcessIdInitialize;

    inline void _EnsureRuntimeBrokerPID()
    {
        if (g_fRuntimeBrokerProcessIdInitialize)
            return;

        HANDLE dwProcessId = GetCurrentProcess();
        WCHAR szImageName[260];
        DWORD dwSize = ARRAYSIZE(szImageName);
        if (SUCCEEDED(ResultFromWin32Bool(QueryFullProcessImageNameW(dwProcessId, 0, szImageName, &dwSize))))
        {
            WCHAR szExpandedPath[260];
            if (ExpandEnvironmentStringsW(
                L"%SystemRoot%\\System32\\RuntimeBroker.exe", szExpandedPath, ARRAYSIZE(szExpandedPath)))
            {
                if (CompareStringOrdinal(szImageName, -1, szExpandedPath, -1, TRUE) == CSTR_EQUAL)
                {
                    g_dwRuntimeBrokerProcessId = GetProcessId(dwProcessId);
                }
            }
        }
        g_fRuntimeBrokerProcessIdInitialize = true;
    }

    inline HRESULT GetCallingProcessHandle(
        DWORD dwProcessAccessFlags, RUNTIMEBROKER_CALLERIDENTITY_CHECK runtimeBrokerCheck, HANDLE *phProcess)
    {
        *phProcess = nullptr;

        Microsoft::WRL::ComPtr<ICallingProcessInfo> spCallingProcessInfo;
        HRESULT hr = CoGetCallContext(IID_PPV_ARGS(&spCallingProcessInfo));
        if (FAILED(hr))
        {
            RETURN_HR_IF(hr, hr != RPC_E_CALL_COMPLETE);
            *phProcess = GetCurrentProcess();
        }
        else
        {
            hr = spCallingProcessInfo->OpenCallerProcessHandle(dwProcessAccessFlags, phProcess);
            if (FAILED(hr))
                return hr;
        }

        if (runtimeBrokerCheck == RCC_FAIL_IF_RUNTIMEBROKER)
        {
            _EnsureRuntimeBrokerPID();
            if (GetProcessId(*phProcess) == g_dwRuntimeBrokerProcessId)
            {
                CloseHandle(*phProcess);
                *phProcess = nullptr;
                return E_FAIL;
            }
        }

        return S_OK;
    }

    inline HRESULT GetCallingProcessType(PROCESS_UICONTEXT *pProcessUIContext)
    {
        HANDLE hProcess;
        HRESULT hr = GetCallingProcessHandle(PROCESS_QUERY_LIMITED_INFORMATION, RCC_ASSERT_IF_RUNTIMEBROKER, &hProcess);
        if (SUCCEEDED(hr))
        {
            hr = S_OK;
        }

        if (hr == RPC_E_CALL_COMPLETE)
        {
            hProcess = GetCurrentProcess();
            hr = S_OK;
        }

        if (SUCCEEDED(hr))
        {
            PROCESS_UICONTEXT_INFORMATION uicontextInfo;
            if (GetProcessUIContextInformation(hProcess, &uicontextInfo))
            {
                hr = S_OK;
                *pProcessUIContext = uicontextInfo.processUIContext;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
            }

            CloseHandle(hProcess);
        }

        return hr;
    }
}
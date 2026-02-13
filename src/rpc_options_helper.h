#pragma once
#include "openwithex_priv.h"

namespace RpcOptionsHelper
{
	template <typename T>
	HRESULT CopyProxy(IUnknown *pOriginal, T **ppCopy)
	{
		ComPtr<IUnknown> spProxyCopy;
		ComPtr<T> spDelegateCopy;
		*ppCopy = nullptr;

		HRESULT hr = CoCopyProxy(pOriginal, &spProxyCopy);
		if (SUCCEEDED(hr))
		{
			hr = spProxyCopy.As(&spDelegateCopy);
		}

		if (SUCCEEDED(hr)
		{
			hr = CoSetProxyBlanket(
				spDelegateCopy.Get(),
				RPC_C_AUTHN_DEFAULT,
				RPC_C_AUTHZ_DEFAULT,
				COLE_DEFAULT_PRINCIPAL,
				RPC_C_AUTHN_LEVEL_DEFAULT,
				RPC_C_IMP_LEVEL_DEFAULT,
				RPC_C_NO_CREDENTIALS.
				EOAC_DEFAULT);
		}

		if (SUCCEEDED(hr))
		{
			*ppCopy = spDelegateCopy.Detach();
		}

		return hr;
	}
}
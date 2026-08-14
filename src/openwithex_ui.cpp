#include "openwithex_ui.h"
#include "file_sys_bind_data.h"
#include "interfaces.h"
#include "undoc.h"
#include "util.h"
#include "noopen_dlg.h"
#include "internet_openas_dlg.h"
#include "vista_openas_dlg.h"
#include <wrl/wrappers/corewrappers.h>
#include <bcrypt.h>
#include <sddl.h>
#include <wincrypt.h>
#include <string>

namespace
{
constexpr GUID CLSID_AppUrlDefaults =
	{ 0x4e29e41b, 0xd022, 0x4d7d, { 0x9a, 0xde, 0xe4, 0x23, 0x52, 0xc2, 0x3d, 0xd6 } };
constexpr GUID CLSID_ServiceHostBrokerProvider =
	{ 0x3480a401, 0xbde9, 0x4407, { 0xbc, 0x02, 0x79, 0x8a, 0x86, 0x6a, 0xc0, 0x51 } };
constexpr GUID SID_QueryAssociationBroker =
	{ 0x9089c807, 0xd432, 0x4025, { 0xb7, 0xab, 0x62, 0x2b, 0x7a, 0x25, 0x3e, 0xb3 } };

constexpr AHTYPE AHTYPE_PROGID_MASK =
	static_cast<AHTYPE>(AHTYPE_PROGID | AHTYPE_CLASS_APPLICATION | AHTYPE_ANY_PROGID);

HRESULT GenerateUserAssocChangeNotification()
{
	SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
	return S_OK;
}

HRESULT EnsurePerUserAssociationIdentifier(LPCWSTR pszTypeID)
{
	HKEY key = nullptr;
	if (RegOpenKeyExW(HKEY_CLASSES_ROOT, pszTypeID, 0, KEY_READ, &key) == ERROR_SUCCESS)
	{
		RegCloseKey(key);
		return S_OK;
	}

	WCHAR keyPath[MAX_PATH];
	RETURN_IF_FAILED(StringCchPrintfW(
		keyPath,
		ARRAYSIZE(keyPath),
		L"Software\\Classes\\%s",
		pszTypeID));

	LSTATUS status = RegCreateKeyExW(
		HKEY_CURRENT_USER,
		keyPath,
		0,
		nullptr,
		0,
		KEY_WRITE,
		nullptr,
		&key,
		nullptr);
	RETURN_HR_IF(HRESULT_FROM_WIN32(status), status != ERROR_SUCCESS);

	if (pszTypeID[0] != L'.')
	{
		const WCHAR empty[] = L"";
		status = RegSetValueExW(
			key,
			L"URL Protocol",
			0,
			REG_SZ,
			reinterpret_cast<const BYTE *>(empty),
			sizeof(empty));
	}
	RegCloseKey(key);
	return HRESULT_FROM_WIN32(status);
}

DWORD WordSwap(DWORD value)
{
	return (value >> 16) | (value << 16);
}

HRESULT GetCurrentUserStringSid(std::wstring &sid)
{
	wil::unique_handle token;
	RETURN_LAST_ERROR_IF(!OpenProcessToken(
		GetCurrentProcess(),
		TOKEN_QUERY,
		token.put()));

	DWORD tokenInformationSize = 0;
	const BOOL gotTokenInformation = GetTokenInformation(
		token.get(),
		TokenUser,
		nullptr,
		0,
		&tokenInformationSize);
	const DWORD tokenInformationError = GetLastError();
	RETURN_HR_IF(E_UNEXPECTED, gotTokenInformation);
	RETURN_HR_IF(
		HRESULT_FROM_WIN32(tokenInformationError),
		tokenInformationError != ERROR_INSUFFICIENT_BUFFER);

	std::vector<BYTE> tokenInformation(tokenInformationSize);
	RETURN_LAST_ERROR_IF(!GetTokenInformation(
		token.get(),
		TokenUser,
		tokenInformation.data(),
		tokenInformationSize,
		&tokenInformationSize));

	LPWSTR rawSid = nullptr;
	RETURN_LAST_ERROR_IF(!ConvertSidToStringSidW(
		reinterpret_cast<TOKEN_USER *>(tokenInformation.data())->User.Sid,
		&rawSid));
	auto freeSid = wil::scope_exit([&]
	{
		LocalFree(rawSid);
	});
	sid.assign(rawSid);

	return S_OK;
}

HRESULT GetTruncatedSystemTimeFromRegKey(HKEY key, SYSTEMTIME *systemTime)
{
	FILETIME lastWriteTime{};
	LSTATUS status = RegQueryInfoKeyW(
		key,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		&lastWriteTime);
	RETURN_HR_IF(HRESULT_FROM_WIN32(status), status != ERROR_SUCCESS);
	RETURN_LAST_ERROR_IF(!FileTimeToSystemTime(&lastWriteTime, systemTime));
	systemTime->wSecond = 0;
	systemTime->wMilliseconds = 0;
	return S_OK;
}

HRESULT ComputeUserChoiceHash(
	LPCWSTR pszTypeID,
	LPCWSTR pszUserSid,
	LPCWSTR pszProgID,
	const SYSTEMTIME &timestamp,
	std::wstring &hash)
{
	SYSTEMTIME truncatedTimestamp = timestamp;
	truncatedTimestamp.wSecond = 0;
	truncatedTimestamp.wMilliseconds = 0;

	FILETIME fileTime{};
	RETURN_LAST_ERROR_IF(!SystemTimeToFileTime(&truncatedTimestamp, &fileTime));

	WCHAR timeString[17];
	RETURN_IF_FAILED(StringCchPrintfW(
		timeString,
		ARRAYSIZE(timeString),
		L"%08lx%08lx",
		fileTime.dwHighDateTime,
		fileTime.dwLowDateTime));

	constexpr LPCWSTR userExperience =
		L"User Choice set via Windows User Experience "
		L"{D18B6DD5-6124-4341-9318-804003BAFA0B}";

	std::wstring input;
	input.reserve(
		wcslen(pszTypeID) +
		wcslen(pszUserSid) +
		wcslen(pszProgID) +
		ARRAYSIZE(timeString) - 1 +
		wcslen(userExperience));
	input.append(pszTypeID);
	input.append(pszUserSid);
	input.append(pszProgID);
	input.append(timeString);
	input.append(userExperience);
	CharLowerBuffW(&input[0], static_cast<DWORD>(input.size()));

	const auto inputBytes = reinterpret_cast<const BYTE *>(input.c_str());
	const ULONG inputByteCount =
		static_cast<ULONG>((input.size() + 1) * sizeof(WCHAR));
	const ULONG blockCount = inputByteCount / (2 * sizeof(DWORD));
	RETURN_HR_IF(E_INVALIDARG, blockCount == 0);

	BCRYPT_ALG_HANDLE algorithm = nullptr;
	NTSTATUS status = BCryptOpenAlgorithmProvider(
		&algorithm,
		BCRYPT_MD5_ALGORITHM,
		nullptr,
		0);
	RETURN_HR_IF(HRESULT_FROM_NT(status), status < 0);
	auto closeAlgorithm = wil::scope_exit([&]
	{
		BCryptCloseAlgorithmProvider(algorithm, 0);
	});

	BCRYPT_HASH_HANDLE hashHandle = nullptr;
	status = BCryptCreateHash(algorithm, &hashHandle, nullptr, 0, nullptr, 0, 0);
	RETURN_HR_IF(HRESULT_FROM_NT(status), status < 0);
	auto destroyHash = wil::scope_exit([&]
	{
		BCryptDestroyHash(hashHandle);
	});

	status = BCryptHashData(
		hashHandle,
		const_cast<BYTE *>(inputBytes),
		inputByteCount,
		0);
	RETURN_HR_IF(HRESULT_FROM_NT(status), status < 0);

	DWORD md5[4];
	status = BCryptFinishHash(
		hashHandle,
		reinterpret_cast<BYTE *>(md5),
		sizeof(md5),
		0);
	RETURN_HR_IF(HRESULT_FROM_NT(status), status < 0);

	const DWORD scramble0[2][5] =
	{
		{ md5[0] | 1, 0xCF98B111, 0x87085B9F, 0x12CEB96D, 0x257E1D83 },
		{ md5[1] | 1, 0xA27416F5, 0xD38396FF, 0x7C932B89, 0xBFA49F69 }
	};
	const DWORD scramble1[2][5] =
	{
		{ md5[0] | 1, 0xEF0569FB, 0x689B6B9F, 0x79F8A395, 0xC3EFEA97 },
		{ md5[1] | 1, 0xC31713DB, 0xDDCD1F0F, 0x59C3AF2D, 0x35BD1EC9 }
	};

	DWORD h0 = 0;
	DWORD h1 = 0;
	DWORD h0Accumulator = 0;
	DWORD h1Accumulator = 0;
	for (ULONG block = 0; block < blockCount; ++block)
	{
		for (ULONG word = 0; word < 2; ++word)
		{
			DWORD value;
			memcpy(
				&value,
				inputBytes + (block * 2 + word) * sizeof(DWORD),
				sizeof(value));

			h0 += value;
			h0 *= scramble0[word][0];
			h0 = WordSwap(h0) * scramble0[word][1];
			h0 = WordSwap(h0) * scramble0[word][2];
			h0 = WordSwap(h0) * scramble0[word][3];
			h0 = WordSwap(h0) * scramble0[word][4];
			h0Accumulator += h0;

			h1 += value;
			h1 = WordSwap(h1) * scramble1[word][1] +
				h1 * scramble1[word][0];
			h1 = (h1 >> 16) * scramble1[word][2] +
				h1 * scramble1[word][3];
			h1 = WordSwap(h1) * scramble1[word][4] + h1;
			h1Accumulator += h1;
		}
	}

	const DWORD result[2] =
	{
		h0 ^ h1,
		h0Accumulator ^ h1Accumulator
	};
	DWORD hashLength = 0;
	RETURN_LAST_ERROR_IF(!CryptBinaryToStringW(
		reinterpret_cast<const BYTE *>(result),
		sizeof(result),
		CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
		nullptr,
		&hashLength));

	std::vector<WCHAR> encodedHash(hashLength);
	RETURN_LAST_ERROR_IF(!CryptBinaryToStringW(
		reinterpret_cast<const BYTE *>(result),
		sizeof(result),
		CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
		encodedHash.data(),
		&hashLength));
	hash.assign(encodedHash.data());
	return S_OK;
}

bool AreSystemTimesEqual(const SYSTEMTIME &left, const SYSTEMTIME &right)
{
	return left.wYear == right.wYear &&
		left.wMonth == right.wMonth &&
		left.wDay == right.wDay &&
		left.wHour == right.wHour &&
		left.wMinute == right.wMinute;
}

HRESULT SetUserChoiceAndHash(
	LPCWSTR pszTypeID,
	LPCWSTR pszProgID)
{
	std::wstring userSid;
	RETURN_IF_FAILED(GetCurrentUserStringSid(userSid));

	const LPCWSTR associationPathFormat = pszTypeID[0] == L'.'
		? L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\%s"
		: L"Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\%s";
	const int associationPathLength =
		_scwprintf(associationPathFormat, pszTypeID) + 1;
	RETURN_HR_IF(E_INVALIDARG, associationPathLength <= 0);

	std::vector<WCHAR> associationPath(associationPathLength);
	RETURN_IF_FAILED(StringCchPrintfW(
		associationPath.data(),
		associationPath.size(),
		associationPathFormat,
		pszTypeID));

	HKEY rawAssociationKey = nullptr;
	LSTATUS regStatus = RegCreateKeyExW(
		HKEY_CURRENT_USER,
		associationPath.data(),
		0,
		nullptr,
		0,
		KEY_READ | KEY_WRITE,
		nullptr,
		&rawAssociationKey,
		nullptr);
	RETURN_HR_IF(HRESULT_FROM_WIN32(regStatus), regStatus != ERROR_SUCCESS);
	auto closeAssociationKey = wil::scope_exit([&]
	{
		RegCloseKey(rawAssociationKey);
	});

	HKEY rawUserChoiceKey = nullptr;
	regStatus = RegCreateKeyExW(
		rawAssociationKey,
		L"UserChoice",
		0,
		nullptr,
		0,
		KEY_READ | KEY_WRITE,
		nullptr,
		&rawUserChoiceKey,
		nullptr);
	RETURN_HR_IF(HRESULT_FROM_WIN32(regStatus), regStatus != ERROR_SUCCESS);
	auto closeUserChoiceKey = wil::scope_exit([&]
	{
		RegCloseKey(rawUserChoiceKey);
	});

	regStatus = RegSetValueExW(
		rawUserChoiceKey,
		L"ProgId",
		0,
		REG_SZ,
		reinterpret_cast<const BYTE *>(pszProgID),
		static_cast<DWORD>((wcslen(pszProgID) + 1) * sizeof(WCHAR)));
	RETURN_HR_IF(HRESULT_FROM_WIN32(regStatus), regStatus != ERROR_SUCCESS);

	for (unsigned int attempt = 0; attempt < 2; ++attempt)
	{
		SYSTEMTIME hashTime{};
		RETURN_IF_FAILED(GetTruncatedSystemTimeFromRegKey(
			rawUserChoiceKey,
			&hashTime));

		std::wstring hash;
		RETURN_IF_FAILED(ComputeUserChoiceHash(
			pszTypeID,
			userSid.c_str(),
			pszProgID,
			hashTime,
			hash));

		regStatus = RegSetValueExW(
			rawUserChoiceKey,
			L"Hash",
			0,
			REG_SZ,
			reinterpret_cast<const BYTE *>(hash.c_str()),
			static_cast<DWORD>((hash.size() + 1) * sizeof(WCHAR)));
		RETURN_HR_IF(HRESULT_FROM_WIN32(regStatus), regStatus != ERROR_SUCCESS);

		SYSTEMTIME writeTime{};
		RETURN_IF_FAILED(GetTruncatedSystemTimeFromRegKey(
			rawUserChoiceKey,
			&writeTime));
		if (AreSystemTimesEqual(hashTime, writeTime))
		{
			return S_OK;
		}
	}

	return E_FAIL;
}

HRESULT SetNewFileTypeDescription(
	LPCWSTR pszTypeID,
	LPCWSTR pszDescription,
	IAssocHandler *pah)
{
	if (!pszDescription || !*pszDescription || !pszTypeID ||
		pszTypeID[0] != L'.')
	{
		return S_FALSE;
	}

	ComPtr<IAssocHandlerInfo> handlerInfo;
	RETURN_IF_FAILED(pah->QueryInterface(IID_PPV_ARGS(&handlerInfo)));

	AHTYPE handlerType = AHTYPE_UNDEFINED;
	RETURN_IF_FAILED(handlerInfo->GetHandlerType(&handlerType));

	wil::unique_cotaskmem_string sourceProgID;
	RETURN_IF_FAILED(handlerInfo->GetInternalProgID(
		(handlerType & AHTYPE_PROGID_MASK) != 0
			? APF_INTERNAL_DEFAULT
			: APF_INTERNAL_APPLICATION,
		&sourceProgID));

	WCHAR progID[MAX_PATH];
	RETURN_IF_FAILED(StringCchPrintfW(
		progID,
		ARRAYSIZE(progID),
		L"%s_auto_file",
		pszTypeID + 1));

	WCHAR progIDPath[MAX_PATH];
	RETURN_IF_FAILED(StringCchPrintfW(
		progIDPath,
		ARRAYSIZE(progIDPath),
		L"Software\\Classes\\%s",
		progID));

	HKEY progIDKey = nullptr;
	LSTATUS status = RegCreateKeyExW(
		HKEY_CURRENT_USER,
		progIDPath,
		0,
		nullptr,
		0,
		KEY_SET_VALUE | KEY_CREATE_SUB_KEY,
		nullptr,
		&progIDKey,
		nullptr);
	RETURN_HR_IF(HRESULT_FROM_WIN32(status), status != ERROR_SUCCESS);
	auto closeProgIDKey = wil::scope_exit([&]
	{
		RegCloseKey(progIDKey);
	});

	const DWORD descriptionSize =
		static_cast<DWORD>((wcslen(pszDescription) + 1) * sizeof(WCHAR));
	status = RegSetValueExW(
		progIDKey,
		nullptr,
		0,
		REG_SZ,
		reinterpret_cast<const BYTE *>(pszDescription),
		descriptionSize);
	RETURN_HR_IF(HRESULT_FROM_WIN32(status), status != ERROR_SUCCESS);

	status = RegSetValueExW(
		progIDKey,
		L"FriendlyTypeName",
		0,
		REG_SZ,
		reinterpret_cast<const BYTE *>(pszDescription),
		descriptionSize);
	RETURN_HR_IF(HRESULT_FROM_WIN32(status), status != ERROR_SUCCESS);
	HKEY sourceShellKey = nullptr;
	WCHAR sourceShellPath[MAX_PATH];
	RETURN_IF_FAILED(StringCchPrintfW(
		sourceShellPath,
		ARRAYSIZE(sourceShellPath),
		L"%s\\shell",
		sourceProgID.get()));
	status = RegOpenKeyExW(
		HKEY_CLASSES_ROOT,
		sourceShellPath,
		0,
		KEY_READ,
		&sourceShellKey);
	RETURN_HR_IF(HRESULT_FROM_WIN32(status), status != ERROR_SUCCESS);
	auto closeSourceShellKey = wil::scope_exit([&]
	{
		RegCloseKey(sourceShellKey);
	});

	HKEY destinationShellKey = nullptr;
	status = RegCreateKeyExW(
		progIDKey,
		L"shell",
		0,
		nullptr,
		0,
		KEY_WRITE,
		nullptr,
		&destinationShellKey,
		nullptr);
	RETURN_HR_IF(HRESULT_FROM_WIN32(status), status != ERROR_SUCCESS);
	auto closeDestinationShellKey = wil::scope_exit([&]
	{
		RegCloseKey(destinationShellKey);
	});

	status = SHCopyKeyW(
		sourceShellKey,
		nullptr,
		destinationShellKey,
		0);
	RETURN_HR_IF(HRESULT_FROM_WIN32(status), status != ERROR_SUCCESS);
	RETURN_IF_FAILED(SetUserChoiceAndHash(pszTypeID, progID));
	EnsurePerUserAssociationIdentifier(pszTypeID);
	return GenerateUserAssocChangeNotification();
}

HRESULT SetDefaultAssociationForAssocHandler(LPCWSTR pszTypeID, IAssocHandler *pah)
{
	ComPtr<IAssocHandlerInfo> handlerInfo;
	RETURN_IF_FAILED(pah->QueryInterface(IID_PPV_ARGS(&handlerInfo)));

	AHTYPE handlerType = AHTYPE_UNDEFINED;
	RETURN_IF_FAILED(handlerInfo->GetHandlerType(&handlerType));

	const bool isProgID = (handlerType & AHTYPE_PROGID_MASK) != 0;
	wil::unique_cotaskmem_string progID;
	RETURN_IF_FAILED(handlerInfo->GetInternalProgID(
		isProgID ? APF_INTERNAL_DEFAULT : APF_INTERNAL_APPLICATION,
		&progID));

	HRESULT hr = SetUserChoiceAndHash(pszTypeID, progID.get());
	if (SUCCEEDED(hr))
	{
		EnsurePerUserAssociationIdentifier(pszTypeID);
	}
	if (isProgID)
	{
		RETURN_IF_FAILED(hr);
		return GenerateUserAssocChangeNotification();
	}

	// The original ignores SetUserAssoc failures for application handlers, then
	// registers the application association through the handler itself.
	ComPtr<IAssocHandlerMakeDefault> makeDefault;
	RETURN_IF_FAILED(pah->QueryInterface(IID_PPV_ARGS(&makeDefault)));
	RETURN_IF_FAILED(makeDefault->TryRegisterApplicationAssoc());
	return GenerateUserAssocChangeNotification();
}

HRESULT FindExtensionInfoForAppUriHandlersCore(
	LPCWSTR pszHost,
	IAppUriExtensionInfoVectorView **extensions)
{
	ComPtr<IServiceHostBrokerProvider> brokerProvider;
	RETURN_IF_FAILED(CoCreateInstance(
		CLSID_ServiceHostBrokerProvider,
		nullptr,
		CLSCTX_LOCAL_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
		IID_PPV_ARGS(&brokerProvider)));

	ComPtr<IQueryAssociationBrokerStatics> queryBroker;
	RETURN_IF_FAILED(brokerProvider->GetBroker(
		SID_QueryAssociationBroker,
		__uuidof(IQueryAssociationBrokerStatics),
		reinterpret_cast<void **>(queryBroker.GetAddressOf())));

	Microsoft::WRL::Wrappers::HString host;
	RETURN_IF_FAILED(host.Set(pszHost));

	return queryBroker->FindAppUriAssociations(
		host.Get(),
		nullptr,
		nullptr,
		extensions);
}

HRESULT FindExtensionInfoForAppUriHandlers(
	LPCWSTR pszHost,
	IAppUriExtensionInfoVectorView **extensions)
{
	struct WorkerContext
	{
		LPCWSTR host;
		IAppUriExtensionInfoVectorView **extensions;
		HRESULT result;
	} context{ pszHost, extensions, E_PENDING };

	*extensions = nullptr;
	wil::unique_handle thread(CreateThread(
		nullptr,
		0,
		[](void *parameter) -> DWORD
		{
			auto worker = static_cast<WorkerContext *>(parameter);
			HRESULT hrInitialize = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
			if (SUCCEEDED(hrInitialize))
			{
				worker->result = FindExtensionInfoForAppUriHandlersCore(
					worker->host,
					worker->extensions);
				CoUninitialize();
			}
			else
			{
				worker->result = hrInitialize;
			}
			return 0;
		},
		&context,
		0,
		nullptr));
	RETURN_LAST_ERROR_IF(!thread);

	HANDLE handle = thread.get();
	DWORD index = 0;
	RETURN_IF_FAILED(CoWaitForMultipleHandles(
		COWAIT_DISPATCH_CALLS | COWAIT_DISPATCH_WINDOW_MESSAGES,
		INFINITE,
		1,
		&handle,
		&index));
	return context.result;
}

bool ShouldRegisteredAppUriHandlerBeDisabled(
	HSTRING selectedHost,
	HSTRING registeredHost)
{
	PCWSTR pszSelected = WindowsGetStringRawBuffer(selectedHost, nullptr);
	PCWSTR pszRegistered = WindowsGetStringRawBuffer(registeredHost, nullptr);
	if (CompareStringOrdinal(pszSelected, -1, pszRegistered, -1, TRUE) == CSTR_EQUAL)
	{
		return true;
	}

	if (pszSelected[0] != L'*')
	{
		return false;
	}
	if (pszRegistered[0] != L'*')
	{
		return true;
	}

	auto CountLabels = [](LPCWSTR value)
	{
		unsigned int count = 0;
		bool inLabel = false;
		for (; *value; ++value)
		{
			if (*value == L'.')
			{
				inLabel = false;
			}
			else if (!inLabel)
			{
				++count;
				inLabel = true;
			}
		}
		return count;
	};

	return CountLabels(pszRegistered) >= CountLabels(pszSelected);
}

HRESULT MakeDefaultAppUriHandler(LPCWSTR pszHost, IAssocHandler *pah)
{
	ComPtr<IAppUrlDefaults> appUrlDefaults;
	RETURN_IF_FAILED(CoCreateInstance(
		CLSID_AppUrlDefaults,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&appUrlDefaults)));

	ComPtr<IAppUriExtensionInfoVectorView> handlers;
	RETURN_IF_FAILED(FindExtensionInfoForAppUriHandlers(pszHost, &handlers));

	UINT32 handlerCount = 0;
	RETURN_IF_FAILED(handlers->get_Size(&handlerCount));

	ComPtr<IObjectWithProgID> objectWithProgID;
	RETURN_IF_FAILED(pah->QueryInterface(IID_PPV_ARGS(&objectWithProgID)));

	wil::unique_cotaskmem_string selectedProgID;
	RETURN_IF_FAILED(objectWithProgID->GetProgID(&selectedProgID));

	Microsoft::WRL::Wrappers::HString selectedHost;
	for (UINT32 i = 0; i < handlerCount; ++i)
	{
		ComPtr<IAppUriExtensionInfo> handler;
		RETURN_IF_FAILED(handlers->GetAt(i, &handler));

		Microsoft::WRL::Wrappers::HString progID;
		RETURN_IF_FAILED(handler->get_ProgId(progID.GetAddressOf()));
		if (CompareStringOrdinal(
			WindowsGetStringRawBuffer(progID.Get(), nullptr),
			-1,
			selectedProgID.get(),
			-1,
			TRUE) == CSTR_EQUAL)
		{
			RETURN_IF_FAILED(handler->get_UriSchemeOrFileExtensionOrHostName(selectedHost.GetAddressOf()));
			RETURN_IF_FAILED(appUrlDefaults->SetAppUrlChoice(
				selectedProgID.get(),
				WindowsGetStringRawBuffer(selectedHost.Get(), nullptr),
				TRUE));
			break;
		}
	}

	RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_NOT_FOUND), !selectedHost.IsValid());

	for (UINT32 i = 0; i < handlerCount; ++i)
	{
		ComPtr<IAppUriExtensionInfo> handler;
		RETURN_IF_FAILED(handlers->GetAt(i, &handler));

		Microsoft::WRL::Wrappers::HString progID;
		Microsoft::WRL::Wrappers::HString registeredHost;
		RETURN_IF_FAILED(handler->get_ProgId(progID.GetAddressOf()));
		RETURN_IF_FAILED(handler->get_UriSchemeOrFileExtensionOrHostName(registeredHost.GetAddressOf()));

		PCWSTR pszProgID = WindowsGetStringRawBuffer(progID.Get(), nullptr);
		if (CompareStringOrdinal(pszProgID, -1, selectedProgID.get(), -1, TRUE) != CSTR_EQUAL
			&& ShouldRegisteredAppUriHandlerBeDisabled(selectedHost.Get(), registeredHost.Get()))
		{
			RETURN_IF_FAILED(appUrlDefaults->SetAppUrlChoice(
				pszProgID,
				WindowsGetStringRawBuffer(selectedHost.Get(), nullptr),
				FALSE));
		}
	}

	return S_OK;
}
}

HRESULT COpenWithExUI::_CreateAndShow()
{
	wil::unique_cotaskmem_string spsz;
	_spItem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &spsz);

	HRESULT hr = S_OK;

	if (_spszTypeID.get())
	{
		if (_spszTypeID.get()[0] == L'.')
		{
			_openwithflags &= ~IMMERSIVE_OPENWITH_PROTOCOL;
		}
		else
		{
			_openwithflags |= IMMERSIVE_OPENWITH_PROTOCOL;
		}
	}
	else if (!(_openwithflags & IMMERSIVE_OPENWITH_PROTOCOL)
			 && !(_openwithflags & IMMERSIVE_OPENWITH_URL))
	{
		hr = _spItem->GetString(PKEY_FileExtension, &_spszTypeID);
		if (hr == E_NOT_SET)
		{
			_spszTypeID = wil::make_cotaskmem_string(L"");
			if (!_spszTypeID.get())
				hr = E_OUTOFMEMORY;
		}
	}
	else if (!(_openwithflags & IMMERSIVE_OPENWITH_URL))
	{
		hr = GetUrlPartFromShellItemName(_spItem.Get(), SIGDN_URL, URL_PART_SCHEME, &_spszTypeID);
	}
	else
	{
		wil::unique_cotaskmem_string spszUrl;
		hr = _spItem->GetDisplayName(SIGDN_URL, &spszUrl);
		if (SUCCEEDED(hr))
		{
			hr = GetUrlPartFromString(spszUrl.get(), URL_PART_HOSTNAME, &_spszTypeID);
		}
	}

	RETURN_IF_FAILED(_spItem->BindToHandler(nullptr, BHID_AssociationArray, IID_PPV_ARGS(&_spQueryAssoc)));

	OPENAS_DLG_TYPE dlgType = OPENAS_DLG_NOTYPE;

	if (SUCCEEDED(hr) && !(_openwithflags & IMMERSIVE_OPENWITH_URL))
	{
		_fEmptyExt = (_spszTypeID.get()[0] == L'\0') 
			|| (CSTR_EQUAL == CompareStringOrdinal(_spszTypeID.get(), -1, L".", -1, TRUE));

		bool fHasCommand = false;
		ComPtr<IApplicationAssociationRegistrationInternal> spIAAR;
		hr = SHCreateAssociationRegistration(IID_PPV_ARGS(&spIAAR));
		if (SUCCEEDED(hr) && !_fEmptyExt)
		{
			if (_openwithflags & IMMERSIVE_OPENWITH_PROTOCOL)
			{
				hr = spIAAR->QueryCurrentDefault(_spszTypeID.get(), AT_URLPROTOCOL, AL_EFFECTIVE, &_spszDefaultProgID);
			}
			else
			{
				hr = spIAAR->QueryCurrentDefault(_spszTypeID.get(), AT_FILEEXTENSION, AL_EFFECTIVE, &_spszDefaultProgID);
			}

			if (SUCCEEDED(hr))
			{
				MessageBoxW(
					NULL,
					_spszDefaultProgID.get(),
					L"Default ProgID:",
					MB_ICONINFORMATION);

				WCHAR szCmd[MAX_PATH];
				DWORD cch = ARRAYSIZE(szCmd);
				if (SUCCEEDED(_spQueryAssoc->GetString(ASSOCF_IGNOREBASECLASS, ASSOCSTR_COMMAND, nullptr, szCmd, &cch)))
				{
					MessageBoxW(
						NULL,
						szCmd,
						L"Command:",
						MB_ICONINFORMATION);
					fHasCommand = true;
				}
			}

			if (hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION))
				hr = S_OK;
		}

		if (!(_openwithflags & IMMERSIVE_OPENWITH_PROTOCOL) && !_fEmptyExt)
		{
			WCHAR szNoOpenMsg[MAX_PATH];
			DWORD cchNoOpenMsg = ARRAYSIZE(szNoOpenMsg);
			WCHAR szTypeName[MAX_PATH];
			DWORD cchTypeName = ARRAYSIZE(szTypeName);
			wil::unique_cotaskmem_string spszFileName;

			if (!fHasCommand)
			{
				if (g_style != OPENWITHEX_STYLE_NT4)
				{
					HRESULT hrNoOpen = _spQueryAssoc->GetString(ASSOCF_IGNOREBASECLASS, ASSOCSTR_NOOPEN, nullptr, szNoOpenMsg, &cchNoOpenMsg);

					if (SUCCEEDED(hrNoOpen))
					{
						hrNoOpen = _spItem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &spszFileName);
					}

					if (SUCCEEDED(hrNoOpen))
					{
						hrNoOpen = _spQueryAssoc->GetString(0, ASSOCSTR_FRIENDLYDOCNAME, nullptr, szTypeName, &cchTypeName);
					}

					if (SUCCEEDED(hrNoOpen))
					{
						CNoOpenDlg dlg(this, szNoOpenMsg);
						INT_PTR result = dlg.ShowDialog(_hwndOwner);
						if (result == IDCANCEL)
						{
							return hr;
						}
					}
				}

				if (g_style <= OPENWITHEX_STYLE_XP
					&& !SHRestricted(REST_NOINTERNETOPENWITH))
				{
					CInternetOpenAsDlg dlg(this);
					INT_PTR result = dlg.ShowDialog(_hwndOwner);
					if (result != IDOK)
					{
						return hr;
					}
				}
			}
		}
	}

	// Empty extensions can't be associated.
	if (_fEmptyExt)
	{
		dlgType = OPENAS_DLG_NORMAL;
	}
	else if (_openwithflags & IMMERSIVE_OPENWITH_PROTOCOL)
	{
		dlgType = OPENAS_DLG_PROTOCOL;
	}
	// The original XP code uses COM here to check if the class key
	// exists. However, that method will now return the key from
	// FileExts as a valid "class key". This is problematic because
	// *every* file extension that ever reaches the Open with UI gets
	// a key there. Only preregistered types and user types with
	// handlers get a *true* class key.
	else
	{
		HKEY hkey;
		if (ERROR_SUCCESS == RegOpenKeyExW(
			HKEY_CLASSES_ROOT,
			_spszTypeID.get(),
			0, KEY_READ, &hkey))
		{
			dlgType = OPENAS_DLG_NORMAL;
			RegCloseKey(hkey);
		}
	}

	switch (g_style)
	{
		case OPENWITHEX_STYLE_7:
		case OPENWITHEX_STYLE_VISTA:
			_pdlg = new CVistaOpenAsDlg(this, dlgType, _openwithflags);
			break;
		default:
			goto SkipDialog;
	}

	_pdlg->ShowDialog(_hwndOwner);
	delete _pdlg;
	
SkipDialog:
	WCHAR szMessage[MAX_PATH * 2];
	swprintf_s(
		szMessage, 
		L"Item: %s\nType: %s\nFlags: 0x%X",
		spsz.get(), _spszTypeID.get(), _openwithflags);
	MessageBoxW(NULL, szMessage, L"OpenWithEx", MB_ICONINFORMATION);

	return hr;
}

HRESULT COpenWithExUI::_MakeDefault(IAssocHandler *pah)
{
	RETURN_HR_IF(E_INVALIDARG, !pah || !_spszTypeID);

	if (_openwithflags & IMMERSIVE_OPENWITH_URL)
	{
		return MakeDefaultAppUriHandler(_spszTypeID.get(), pah);
	}

	return SetDefaultAssociationForAssocHandler(_spszTypeID.get(), pah);
}

STDMETHODIMP COpenWithExUI::SetSite(IUnknown *punkSite)
{
	if (punkSite)
	{
		IUnknown_GetParentWindow(punkSite, &_hwndOwner);

		ComPtr<IOpenWithTypeOverride> spTypeOverride;
		if (SUCCEEDED(punkSite->QueryInterface(IID_PPV_ARGS(&spTypeOverride))))
		{
			spTypeOverride->GetOpenWithTypeOverride(&_spszTypeID);
		}
	}

	IUnknown_Set(&_spunkSite, punkSite);
	return S_OK;
}

STDMETHODIMP COpenWithExUI::GetSite(REFIID riid, LPVOID *ppvSite)
{
	*ppvSite = nullptr;

	if (_spunkSite)
	{
		return _spunkSite->QueryInterface(riid, ppvSite);
	}
	return E_FAIL;
}

COpenWithExUI::COpenWithExUI()
	: _hwndOwner(NULL)
	, _openwithflags(IMMERSIVE_OPENWITH_NONE)
	, _ptPosition{ 0, 0 }
	, _fEmptyExt(false)
{

}

HRESULT COpenWithExUI::CreateAndShow(HWND hwndOwner, LPCWSTR pszFileName, IMMERSIVE_OPENWITH_FLAGS flags)
{
	HRESULT hr;
	if (flags & IMMERSIVE_OPENWITH_PROTOCOL)
		hr = SHCreateItemFromParsingName(pszFileName, nullptr, IID_PPV_ARGS(&_spItem));
	else
		hr = SHSimpleItemFromAttributes(pszFileName, FILE_ATTRIBUTE_NORMAL, IID_PPV_ARGS(&_spItem));

	if (SUCCEEDED(hr))
	{
		hr = SHCreateShellItemArrayFromShellItem(_spItem.Get(), IID_PPV_ARGS(&_spItems));
		if (SUCCEEDED(hr))
		{
			_hwndOwner = hwndOwner;
			_openwithflags = flags;
			return _CreateAndShow();
		}
	}

	return hr;
}

HRESULT COpenWithExUI::CreateAndShowFromDelegateExecute(IExecuteCommand *pxc, IMMERSIVE_OPENWITH_FLAGS flags)
{
	_openwithflags = flags;
	
	RETURN_IF_FAILED(IUnknown_GetSelection(pxc, IID_PPV_ARGS(&_spItems)));
	RETURN_IF_FAILED(IShellItemArray_GetItemAt(_spItems.Get(), 0, IID_PPV_ARGS(&_spItem)));

	wil::unique_cotaskmem_string spsz;
	if ((_openwithflags & IMMERSIVE_OPENWITH_URL)
		&& SUCCEEDED_LOG(_spItem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &spsz)))
	{
		if (PathIsURLW(spsz.get()))
			_openwithflags |= IMMERSIVE_OPENWITH_PROTOCOL;
	}

	RETURN_HR(_CreateAndShow());
}

HRESULT COpenWithExUI::SetPosition(POINT pt)
{
	_ptPosition = pt;
	return S_OK;
}

HRESULT COpenWithExUI::GetItem(IShellItem2 **ppItem)
{
	return _spItem.CopyTo(ppItem);
}

HRESULT COpenWithExUI::GetItemName(SIGDN sigdnName, LPWSTR *ppszOut)
{
	return _spItem->GetDisplayName(sigdnName, ppszOut);
}

HRESULT COpenWithExUI::GetTypeID(LPWSTR *ppszOut)
{
	return SHStrDupW(_spszTypeID.get(), ppszOut);
}

HRESULT COpenWithExUI::GetDescription(LPWSTR pszOut, DWORD cchOut)
{
	return !_spQueryAssoc ? E_FAIL : _spQueryAssoc->GetString(0, ASSOCSTR_FRIENDLYDOCNAME, nullptr, pszOut, &cchOut);
}

bool COpenWithExUI::AllowRegistration()
{
	if (_fEmptyExt)
		return false;

	if (!(_openwithflags & IMMERSIVE_OPENWITH_OVERRIDE)
		|| (_openwithflags & IMMERSIVE_OPENWITH_DONOT_SETDEFAULT))
		return false;

	return true;
}

void COpenWithExUI::FillListByEnumHandlers()
{
	_handlers.clear();

	ComPtr<IEnumAssocHandlers> spEnumAssocHandlers;
	HRESULT hr;
	if (_openwithflags & (IMMERSIVE_OPENWITH_PROTOCOL | IMMERSIVE_OPENWITH_URL))
		hr = SHAssocEnumHandlersForProtocolByApplication(
			_spszTypeID.get(),
			IID_PPV_ARGS(&spEnumAssocHandlers));
	// Is this even worth using? Exported by C++ name... ew.
	//else if (_openwithflags & IMMERSIVE_OPENWITH_URL)
		//SHEnumAssocHandlersForUrl(
			//_spszTypeID.get(),
			//ASSOC_FILTER_NONE,
			//false,
			//IID_PPV_ARGS(&spEnumAssocHandlers));
	else
		hr = SHAssocEnumHandlers(_spszTypeID.get(), ASSOC_FILTER_NONE, &spEnumAssocHandlers);

	if (SUCCEEDED(hr))
	{
		bool fFirst = true;
		ComPtr<IAssocHandler> spah;
		while (S_OK == spEnumAssocHandlers->Next(1, &spah, nullptr))
		{
			if (fFirst)
			{
				if (S_OK == spah->IsRecommended())
				{
					_pdlg->SetupCategories();
				}
				fFirst = false;
			}

			_pdlg->AddItem(spah.Get());
			_handlers.push_back(std::move(spah));
		}
	}
}

void COpenWithExUI::OpenAsOther()
{
	WCHAR szApp[MAX_PATH];
	WCHAR szPath[MAX_PATH];
	WCHAR szFilter[MAX_PATH];
	WCHAR szPrograms[MAX_PATH];
	WCHAR szAllFiles[MAX_PATH];

	szApp[0] = L'\0';

	LPCWSTR pszFormat = L"%s#*.exe;*.pif;*.com;*.bat;*.cmd#%s#*.*##";
	if (g_style == OPENWITHEX_STYLE_NT4) // NT4 has a (*.*) on all files that seems to be unchanged between locales
		pszFormat = L"%s#*.exe;*.pif;*.com;*.bat;*.cmd#%s (*.*)#*.*##";

	LoadStringW(g_hinst, IDS_PROGRAMSFILTER, szPrograms, ARRAYSIZE(szPrograms));
	LoadStringW(g_hinst, IDS_ALLFILESFILTER, szAllFiles, ARRAYSIZE(szAllFiles));

	swprintf_s(szFilter, pszFormat, szPrograms, szAllFiles);

	// Replace # with null bytes
	size_t length = wcslen(szFilter);
	for (size_t i = 0; i < length; i++)
	{
		if (szFilter[i] == L'#')
			szFilter[i] = L'\0';
	}

	ExpandEnvironmentStringsW(L"%ProgramFiles%", szPath, ARRAYSIZE(szPath));
	
	WCHAR szTitle[MAX_PATH];
	UINT idStr = (g_style >= OPENWITHEX_STYLE_XP) ? IDS_OPENAS_VISTA : IDS_OPENAS;
	LoadStringW(g_hinst, idStr, szTitle, ARRAYSIZE(szTitle));

	if (GetFileNameFromBrowse(_pdlg->_hwnd, szApp, ARRAYSIZE(szApp), szPath,
			L"exe", szFilter, szTitle))
	{
		LPWSTR pszFileName = PathFindFileNameW(szApp);
		if (IsBlockedFromOpenWithBrowse(pszFileName))
		{
			static HMODULE hmodShell = GetModuleHandleW(L"shell32.dll");
			ShellMessageBoxW(
				hmodShell,
				_pdlg->_hwnd,
				MAKEINTRESOURCEW(0x7503),
				MAKEINTRESOURCEW(0x7502),
				MB_ICONERROR);
			return;
		}

		// If the requested EXE already exists as a handler,
		// select it.
		int nHandlers = (int)_handlers.size();
		for (int i = 0; i < nHandlers; i++)
		{
			ComPtr<IAssocHandler> &spah = _handlers.at(i);
			wil::unique_cotaskmem_string spszHandlerPath;

			if (SUCCEEDED(spah->GetName(&spszHandlerPath))
			&& !_wcsicmp(spszHandlerPath.get(), szApp))
			{
				_pdlg->SelectItemByIndex(i);
				return;
			}
		}

		ComPtr<IAssocHandler> spah;
		if (SUCCEEDED(SHCreateAssocHandler(AHTYPE_USER_APPLICATION, _spszTypeID.get(), szApp, &spah)))
		{
			_pdlg->AddItem(spah.Get());
			_pdlg->SelectItemByIndex((int)_handlers.size());
			_handlers.push_back(std::move(spah));
		}
	}
}

void COpenWithExUI::OpenDownloadURL(HWND hwnd)
{
	WCHAR szUrl[1024];
	swprintf_s(
		szUrl,
		L"http://go.microsoft.com/fwlink/?LinkId=57426&Ext=%s",
		_spszTypeID.get());
	ShellExecuteW(
		hwnd,
		nullptr,
		szUrl,
		nullptr, nullptr,
		SW_SHOWNORMAL);
}

void COpenWithExUI::OnOk(bool fMakeAssoc, LPCWSTR pszDescription)
{
	for (ComPtr<IAssocHandler> &spAssocHandler : _handlers)
	{
		ComPtr<IAssocHandlerPromptCount> spPromptCount;
		if (SUCCEEDED(spAssocHandler.As(&spPromptCount)))
		{
			spPromptCount->UpdatePromptCount(ASSOCHANDLER_PROMPTUPDATE_BEHAVIOR_CLEAR);
		}
	}

	ComPtr<IDataObject> dataObj;
	if (SUCCEEDED(_spItems->BindToHandler(
		nullptr,
		BHID_DataObject,
		IID_PPV_ARGS(&dataObj)
	)))
	{
		IAssocHandler *pah = _pdlg->GetSelectedItem();
		if (pah)
		{
			if (fMakeAssoc)
			{
				if (pszDescription && *pszDescription)
				{
					SetNewFileTypeDescription(
						_spszTypeID.get(),
						pszDescription,
						pah);
				}
				else
				{
					_MakeDefault(pah);
				}
			}
			pah->Invoke(dataObj.Get());
		}
	}
}
#pragma once

#include <windows.h>

class CVersionHelper
{
public:
	// Data structure returned by GetVersionInfo().
	struct VersionStruct
	{
		bool isInitialized = false;
		DWORD dwMajorVersion = 0;
		DWORD dwMinorVersion = 0;
		DWORD dwBuildNumber = 0;
		DWORD dwPlatformId = 0;
	};

	typedef void (WINAPI *RtlGetVersion_t)(OSVERSIONINFOEXW *);

	// Gets the precise OS version.
	static const VersionStruct *GetVersionInfo()
	{
		static VersionStruct s_versionStruct = { 0 };

		// Skip if cached.
		if (!s_versionStruct.isInitialized)
		{
			HMODULE hMod = LoadLibraryW(L"ntdll.dll");

			if (hMod)
			{
				RtlGetVersion_t func = (RtlGetVersion_t)GetProcAddress(hMod, "RtlGetVersion");

				if (!func)
				{
					FreeLibrary(hMod);

					// TODO: error handling.
					return &s_versionStruct;
				}

				OSVERSIONINFOEXW osVersionInfo = { 0 };
				osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);

				func(&osVersionInfo);

				s_versionStruct.dwBuildNumber = osVersionInfo.dwBuildNumber;
				s_versionStruct.dwMajorVersion = osVersionInfo.dwMajorVersion;
				s_versionStruct.dwMinorVersion = osVersionInfo.dwMinorVersion;
				s_versionStruct.dwPlatformId = osVersionInfo.dwPlatformId;

				s_versionStruct.isInitialized = true;

				FreeLibrary(hMod);
			}
		}

		return &s_versionStruct;
	}

	// Specific version helpers.
	inline static const bool IsWindows10OrGreater()
	{
		return GetVersionInfo()->dwMajorVersion >= 10 &&
			   GetVersionInfo()->dwBuildNumber >= 10240;
	}

	inline static const bool IsWindows10_1703OrGreater()
	{
		return GetVersionInfo()->dwMajorVersion >= 10 &&
			   GetVersionInfo()->dwBuildNumber >= 15063;
	}
};
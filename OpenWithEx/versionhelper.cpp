#include "versionhelper.h"

// Private implementation

typedef void (WINAPI *RtlGetVersion_t)(OSVERSIONINFOEXW *);

// static
const CVersionHelper::VersionStruct *CVersionHelper::GetVersionInfo()
{
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
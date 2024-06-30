#include <windows.h>
#include <shlobj.h>

#include "versionhelper.h"
#include "assocuserchoice.h"

HRESULT SetDefaultAssoc(LPCWSTR pszExtension, IAssocHandler *pAssocHandler)
{
	if (!CVersionHelper::IsWindows10OrGreater())
	{
		// We don't currently handle pre-Windows 10 cases.
		return E_FAIL;
	}
}
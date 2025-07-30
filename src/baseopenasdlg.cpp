#include "baseopenasdlg.h"
#include <shlwapi.h>
#include <shlobj.h>

#include "iassochandler_internal.h"
#include "SetDefaultAssociation.h"

#include <memory>

#include "wil/com.h"
#include "wil/resource.h"

/*
 * Get the app icon.
 * 
 * @param lpszIconPath  The path of the binary containing the icon, or an
 *                      indirect string in the case of UWP applications.
 * @param iIndex        The index of the icon in the binary.
 * 
 */
int GetAppIconIndex(LPCWSTR lpszIconPath, int iIndex)
{
	/*
	 * This seems to be an edge case handled in TWinUI.
	 * 
	 * I did not bother looking into it further.
	 */
	if (iIndex == -1)
	{
		return Shell_GetCachedImageIndexW(L"shell32.dll", 2, 0);
	}

	/*
	 * Get the app icon for UWP applications.
	 *
	 * In the documentation for Shell_GetCachedImageIndex, Microsoft straight up
	 * lies about the purpose of the `uIconFlags` argument, stating that it is
	 * not used. It is, in fact, used.
	 *
	 * TWinUI happily supplies an argument for it in
	 * `CImmersiveOpenWithUI::_GetIcon()`.
	 *
	 * https://learn.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-shell_getcachedimageindexa
	 *
	 * I copied this logic from TWinUI, but I spent some time figuring out why it
	 * works as well.
	 *
	 * The gist of it is that this uIconFlags argument, like arguments of the same
	 * name on other functions, is one of the GIL_ variables. Of course, Microsoft
	 * didn't bother documenting this, but it can be observed by a simple GitHub
	 * search of the variable name, which will show you leaked Windows source code
	 * in the search results with proper documentation available.
	 *
	 * https://files.catbox.moe/z4xgza.png
	 *
	 * When you call IAssocHandler::GetIconLocation() on a UWP handler, instead of
	 * getting an absolute file system path, you get a uniform resource locator
	 * wrapped in a custom format which begins with "@". I have no idea what this
	 * is exactly, but apparently it's called an "indirect string" and will always
	 * start with an "@" character.
	 *
	 * https://learn.microsoft.com/en-us/windows/win32/api/shlwapi/nf-shlwapi-shloadindirectstring
	 *
	 * Anyway, TWinUI doesn't bother with this indirect string, and neither will
	 * we. It just sets the flags to GIL_ASYNC | GIL_NOTFILENAME and calls into
	 * `Shell_GetCachedImageIndexW()` like normal. This is enough to get the
	 * icon of a UWP application, and has worked perfectly in my testing.
	 * 
	 * According to Microsoft's C headers, GIL_NOTFILENAME causes the system to
	 * call into ExtractIcon() to get the icon. This flag results in no icon
	 * being displayed at all without GIL_ASYNC, so it seems that this only works
	 * asynchronously.
	 * 
	 * https://github.com/tpn/winsdk-10/blob/master/Include/10.0.10240.0/um/ShlObj.h#L258
	 * 
	 * The aforementioned function `CImmersiveOpenWithUI::_GetIcon()` in TWinUI
	 * does perform some additional work in case `Shell_GetCachedImageIndex()`
	 * doesn't work, but we don't handle any such case as of right now.
	 */
	UINT uIconFlags = 0;
	if (lpszIconPath && lpszIconPath[0] == L'@')
		uIconFlags = GIL_ASYNC | GIL_NOTFILENAME;

	return Shell_GetCachedImageIndexW(
		lpszIconPath, iIndex, uIconFlags
	);
}

INT_PTR CALLBACK CBaseOpenAsDlg::v_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			/* Set icon */
			HMODULE hShell32 = LoadLibraryW(L"shell32.dll");
			HICON hiOpenWith = LoadIconW(hShell32, MAKEINTRESOURCEW(134));
			SendDlgItemMessageW(
				hWnd,
				IDD_OPENWITH_ICON,
				STM_SETICON,
				(WPARAM)hiOpenWith,
				NULL
			);

			/* Set path text */
			SetDlgItemTextW(
				hWnd,
				IDD_OPENWITH_FILE,
				m_fUri ? m_szPath : m_pszFileName
			);

			/* Set up checkbox */
			if (!m_szExtOrProtocol || !*m_szExtOrProtocol
			|| !(m_flags & IMMERSIVE_OPENWITH_OVERRIDE)
			|| (m_flags & IMMERSIVE_OPENWITH_DONOT_EXEC))
			{
				EnableWindow(
					GetDlgItem(hWnd, IDD_OPENWITH_ASSOC),
					FALSE
				);
			}
			
			if (m_szExtOrProtocol && *m_szExtOrProtocol)
			{
				if (m_flags & IMMERSIVE_OPENWITH_DONOT_EXEC
				|| ((m_flags & IMMERSIVE_OPENWITH_OVERRIDE) && !m_fPreregistered))
				{
					SendDlgItemMessageW(
						hWnd,
						IDD_OPENWITH_ASSOC,
						BM_SETCHECK,
						BST_CHECKED,
						NULL
					);
				}
			}
			
			if (!m_szExtOrProtocol || !*m_szExtOrProtocol || m_fUri)
			{
				ShowWindow(
					GetDlgItem(hWnd, IDD_OPENWITH_LINK),
					SW_HIDE
				);
			}

			_InitProgList();
			_GetHandlers();
			if (m_fRecommended)
				_SetupCategories();

			for (size_t i = 0; i < m_handlers.size(); i++)
			{
				// I haven't seen it experienced on any other system,
				// but I'm getting handlers that are completely blank.
				// The display name is simply not received rather than
				// it being an empty string, so it should be safe to dismiss
				// these handlers. They don't do anything anyway.
				//     - aubymori
				wil::com_ptr<IAssocHandler> &pHandler = m_handlers.at(i);
				wil::unique_cotaskmem_string pszDisplayName;
				pHandler->GetUIName(&pszDisplayName);
				if (pszDisplayName.get())
					_AddItem(pHandler, i, false);
			}

			return TRUE;
		}
		case WM_CLOSE:
			EndDialog(hWnd, IDCANCEL);
			return TRUE;
		case WM_COMMAND:
			switch (wParam)
			{
				case IDOK:
					_OnOk();
					break;
				case IDCANCEL:
					EndDialog(hWnd, IDCANCEL);
					break;
				case IDD_OPENWITH_BROWSE:
					_BrowseForProgram();
					break;
			}
			break;
		case WM_NOTIFY:
		{
			LPNMHDR nmh = (LPNMHDR)lParam;
			switch (nmh->code)
			{
				case TVN_SELCHANGED:
				case LVN_ITEMCHANGED:
					EnableWindow(
						GetDlgItem(hWnd, IDOK),
						_GetSelectedItem() != nullptr
					);
					break;
				case NM_CLICK:
					if (nmh->idFrom == IDD_OPENWITH_LINK)
					{
						PNMLINK link = (PNMLINK)nmh;
						if (0 == wcscmp(link->item.szID, L"Browse"))
						{
							OpenDownloadURL(m_szExtOrProtocol);
							EndDialog(hWnd, IDCANCEL);
						}
					}
					break;
				case NM_DBLCLK:
					if (nmh->idFrom == IDD_OPENWITH_PROGLIST)
					{
						_OnOk();
					}
					break;
			}
			break;
		}
	}

	return FALSE;
}

void CBaseOpenAsDlg::_SelectOrAddItem(LPCWSTR lpszPath)
{
	if (lpszPath)
	{
		LPCWSTR lpszFileName = PathFindFileNameW(lpszPath);
		if (IsBlockedFromOpenWithBrowse(lpszFileName))
		{
			ShellMessageBoxW(g_hShell32, m_hWnd, MAKEINTRESOURCEW(0x7503), MAKEINTRESOURCEW(0x7502), MB_ICONERROR);
			return;
		}

		int index = _FindItemIndex(lpszPath);
		if (index != -1)
		{
			_SelectItemByIndex(index);
		}
		else
		{
			wil::com_ptr<IAssocHandler> pHandler = nullptr;
			SHCreateAssocHandler(AHTYPE_USER_APPLICATION, m_szExtOrProtocol, lpszPath, &pHandler);
			if (pHandler)
			{
				m_handlers.push_back(pHandler);
				_AddItem(pHandler, m_handlers.size() - 1, true);
				_SelectItemByIndex(m_handlers.size() - 1);
			}
		}
	}
}

int CBaseOpenAsDlg::_FindItemIndex(LPCWSTR lpszPath)
{
	wil::com_ptr<IAssocHandler> pResult = nullptr;

	for (size_t i = 0; i < m_handlers.size(); i++)
	{
		wil::com_ptr<IAssocHandler> pHandler = m_handlers.at(i);
		wil::unique_cotaskmem_string lpszCurPath = nullptr;
		pHandler->GetName(&lpszCurPath);
		if (lpszCurPath)
		{
			if (0 == _wcsicmp(lpszPath, lpszCurPath.get()))
				pResult = pHandler;
		}

		if (pResult)
			return i;
	}
	return -1;
}

void CBaseOpenAsDlg::_GetHandlers()
{
	wil::com_ptr<IEnumAssocHandlers> pEnumHandler = nullptr;
	if (m_fUri)
		SHAssocEnumHandlersForProtocolByApplication(
			m_szExtOrProtocol,
			IID_PPV_ARGS(&pEnumHandler)
		);
	else
		SHAssocEnumHandlers(
			m_szExtOrProtocol,
			ASSOC_FILTER_NONE,
			&pEnumHandler
		);

	wil::com_ptr<IAssocHandler> pAssoc = nullptr;
	ULONG pceltFetched = 0;
	while (SUCCEEDED(pEnumHandler->Next(1, &pAssoc, &pceltFetched)) && pAssoc)
	{
		if (!m_fRecommended && S_OK == pAssoc->IsRecommended())
		{
			m_fRecommended = true;
		}
		m_handlers.push_back(pAssoc);
	}
}

// This is the implementation which is shared across CXPOpenAsDlg and
// CClassicOpenAsDlg
void CBaseOpenAsDlg::_BrowseForProgram()
{
	WCHAR szFile[MAX_PATH] = { 0 };

	// Set up filter string
	WCHAR szFilter[MAX_PATH];
	WCHAR szPrograms[128];
	WCHAR szAllFiles[128];
	LoadStringW(g_hInst, IDS_PROGRAMS, szPrograms, 128);
	LoadStringW(g_hInst, IDS_ALLFILES, szAllFiles, 128);
	LPCWSTR pszFormat = L"%s#*.exe;*.pif;*.com;*.bat;*.cmd#%s#*.*##";
	if (g_style == OWXS_NT4) // NT4 has a (*.*) on all files that seems to be unchanged between locales
		pszFormat = L"%s#*.exe;*.pif;*.com;*.bat;*.cmd#%s (*.*)#*.*##";
	swprintf_s(szFilter, pszFormat, szPrograms, szAllFiles);
	
	// Replace # with null bytes so GetOpenFileNameW accepts it
	int length = wcslen(szFilter);
	for (int i = 0; i < length; i++)
	{
		if (szFilter[i] == L'#')
			szFilter[i] = L'\0';
	}

	WCHAR szTitle[256];
	LoadStringW(g_hInst, IDS_BROWSETITLE_XP, szTitle, 256);

	OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW) };
	ofn.hwndOwner = m_hWnd;
	ofn.lpstrFilter = szFilter;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = szTitle;
	ofn.lpstrDefExt = L"exe";
	ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
	if (GetOpenFileNameW(&ofn) && *szFile)
	{
		_SelectOrAddItem(szFile);
	}
}

/**
 * Clears the recently-installed flags to avoid a restart loop when new applications have been
 * installed.
 */
HRESULT CBaseOpenAsDlg::_ClearRecentlyInstalled()
{
	for (wil::com_ptr<IAssocHandler> pHandler : m_handlers)
	{
		wil::com_ptr<IAssocHandlerPromptCount> pPromptCount;
		RETURN_IF_FAILED(pHandler->QueryInterface(&pPromptCount));
		pPromptCount->UpdatePromptCount(ASSOCHANDLER_PROMPTUPDATE_BEHAVIOR_CLEAR);
	}

	return S_OK;
}

void CBaseOpenAsDlg::_OnOk()
{
	bool fAssoc = IsDlgButtonChecked(m_hWnd, IDD_OPENWITH_ASSOC);
	WCHAR szDesc[MAX_PATH] = { 0 };
	GetDlgItemTextW(
		m_hWnd,
		IDD_OPENWITH_DESC,
		szDesc,
		MAX_PATH
	);

	wil::com_ptr<IAssocHandler> pSelected = _GetSelectedItem();
	if (pSelected)
	{
		EndDialog(m_hWnd, IDOK);

		if (fAssoc)
		{
			LOG_IF_FAILED(SetDefaultAssociation(m_szExtOrProtocol, pSelected.get()));
		}

		// Change the association description if applicable
		if (m_uDlgId == IDD_OPENWITH_WITHDESC)
		{
			HWND hWndDescEditBox = GetDlgItem(m_hWnd, IDD_OPENWITH_DESC);

			DWORD cchDescText = GetWindowTextLengthW(hWndDescEditBox);
			std::unique_ptr<WCHAR[]> pszDescription = std::make_unique<WCHAR[]>(cchDescText + sizeof('\0'));
			GetWindowTextW(hWndDescEditBox, pszDescription.get(), cchDescText + sizeof('\0'));

			// Find the association handler path in the registry:
			wil::unique_hkey hkAssocKey = nullptr;
			RegOpenKeyExW(
				HKEY_CLASSES_ROOT,
				m_szExtOrProtocol,
				NULL,
				KEY_READ,
				&hkAssocKey
			);

			DWORD cbProgId = 0;
			RegGetValueW(
				hkAssocKey.get(),
				nullptr,
				nullptr,
				RRF_RT_REG_SZ,
				nullptr,
				nullptr,
				&cbProgId
			);
			std::unique_ptr<WCHAR[]> pszProgId = std::make_unique<WCHAR[]>(cbProgId);
			RegGetValueW(
				hkAssocKey.get(),
				nullptr,
				nullptr,
				RRF_RT_REG_SZ,
				nullptr,
				pszProgId.get(),
				&cbProgId
			);

			wil::unique_hkey hkProgId = nullptr;
			RegOpenKeyExW(
				HKEY_CLASSES_ROOT,
				pszProgId.get(),
				NULL,
				KEY_READ | KEY_WRITE,
				&hkProgId
			);

			RegSetKeyValueW(
				hkProgId.get(),
				nullptr,
				nullptr,
				REG_SZ,
				pszDescription.get(),
				(cchDescText + 1) * sizeof(WCHAR)
			);

			// Notify shell to refresh icons:
			SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
		}

		// Don't launch when changing default from properties.
		if (!(m_flags & IMMERSIVE_OPENWITH_DONOT_EXEC))
		{
			// Start the program with the file:
			wil::com_ptr<IShellItem> pShellItem = nullptr;
			HRESULT hr = SHCreateItemFromParsingName(m_szPath, nullptr, IID_PPV_ARGS(&pShellItem));

			if (SUCCEEDED(hr) && pShellItem)
			{
				wil::com_ptr<IShellItemArray> pShellItemArray = nullptr;
				hr = SHCreateShellItemArrayFromShellItem(pShellItem.get(), IID_PPV_ARGS(&pShellItemArray));

				if (SUCCEEDED(hr) && pShellItemArray)
				{
					// When invoking, we must clear the recently-installed list, or we will get into a
					// restart loop as the operating system will invoke the open with UI when a new
					// application is installed and the recently-installed list is marked. This includes
					// programmatic use of IAssocHandler::Invoke. TWinUI also does this.
					_ClearRecentlyInstalled();

					wil::com_ptr<IDataObject> pInvocationObj = nullptr;
					hr = pShellItemArray->BindToHandler(nullptr, BHID_DataObject, IID_PPV_ARGS(&pInvocationObj));

					if (SUCCEEDED(hr))
					{
						pSelected->Invoke(pInvocationObj.get());
					}
				}
			}
		}
	}
}

CBaseOpenAsDlg::CBaseOpenAsDlg(LPCWSTR lpszPath, IMMERSIVE_OPENWITH_FLAGS flags, bool fUri, bool fPreregistered, UINT uDlgId, UINT uDlgWithDescId, UINT uDlgProtocolId)
	: CImpDialog(g_hInst, uDlgWithDescId)
	, m_szExtOrProtocol{ 0 }
	, m_flags(flags)
	, m_fUri(fUri)
	, m_fPreregistered(fPreregistered)
	, m_fRecommended(false)
{
	wcscpy_s(m_szPath, lpszPath);
	m_pszFileName = PathFindFileNameW(m_szPath);
	if (m_fUri)
	{
		WCHAR *pColon = wcschr(m_szPath, L':');
		/* Protocol length */
		int length = ((long long)pColon - (long long)m_szPath) / sizeof(WCHAR);
		wcsncpy_s(m_szExtOrProtocol, m_szPath, length);
	}
	else
	{
		LPWSTR pszExtension = PathFindExtensionW(m_szPath);
		if (pszExtension)
		{
			wcscpy_s(m_szExtOrProtocol, pszExtension);
		}
	}

	if (m_fUri)
	{
		m_uDlgId = uDlgProtocolId;
	}
	else if (
		!m_szExtOrProtocol || !*m_szExtOrProtocol ||
		!(flags & IMMERSIVE_OPENWITH_OVERRIDE) || m_fPreregistered
	)
	{
		m_uDlgId = uDlgId;
	}
}

CBaseOpenAsDlg::~CBaseOpenAsDlg()
{
	for (size_t i = 0; i < m_handlers.size(); i++)
	{
		wil::com_ptr<IAssocHandler> pItem = m_handlers.at(i);
		if (pItem)
		{
			pItem.reset();
		}
	}
	m_handlers.clear();
}
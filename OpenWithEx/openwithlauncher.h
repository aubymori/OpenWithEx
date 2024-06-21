#include "openwithex.h"

#ifndef _openwithlauncher_h_
#define _openwithlauncher_h_

enum IMMERSIVE_OPENWITH_FLAGS
{

};

class IOpenWithLauncher : public IUnknown
{
public:
	/* Begin IUnknown implementation */
	virtual HRESULT QueryInterface(REFIID riid, void **ppvObject) = 0;
	virtual ULONG AddRef() = 0;
	virtual ULONG Release() = 0;
	/* End IUnknown implementation */

	virtual HRESULT Launch(HWND hWndParent, LPCWSTR lpszPath, IMMERSIVE_OPENWITH_FLAGS flags) = 0;
};

#endif
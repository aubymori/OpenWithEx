#include "openwithex.h"

#ifndef _openwithlauncher_h_
#define _openwithlauncher_h_

enum IMMERSIVE_OPENWITH_FLAGS
{

};

class IOpenWithLauncher : public IUnknown
{
public:
	virtual HRESULT Launch(HWND hWndParent, LPCWSTR lpszPath, IMMERSIVE_OPENWITH_FLAGS flags) = 0;
};

#endif
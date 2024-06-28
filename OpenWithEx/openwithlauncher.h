#pragma once

#include "openwithex.h"

enum IMMERSIVE_OPENWITH_FLAGS
{

};

class IOpenWithLauncher : public IUnknown
{
public:
	virtual HRESULT Launch(HWND hWndParent, LPCWSTR lpszPath, IMMERSIVE_OPENWITH_FLAGS flags) = 0;
};
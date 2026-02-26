#pragma once

#include "openwithex.h"
#include "resource.h"

#include <shlwapi.h>
#include <shlobj.h>
#include <propkey.h>

#include <wil/result_macros.h>
#include <wil/resource.h>

#include <wrl/implements.h>

using namespace Microsoft::WRL;

extern HINSTANCE g_hinst;
extern OPENWITHEX_STYLE g_style;

enum IMMERSIVE_OPENWITH_FLAGS
{
	IMMERSIVE_OPENWITH_NONE              = 0x0,
	IMMERSIVE_OPENWITH_OVERRIDE          = 0x1,
	IMMERSIVE_OPENWITH_DONOT_EXEC        = 0x4,
	IMMERSIVE_OPENWITH_PROTOCOL          = 0x8,
	IMMERSIVE_OPENWITH_URL               = 0x10,
	IMMERSIVE_OPENWITH_USEPOSITION       = 0x20,
	IMMERSIVE_OPENWITH_DONOT_SETDEFAULT  = 0x40,
	IMMERSIVE_OPENWITH_ACTION            = 0x80,
	IMMERSIVE_OPENWITH_ALLOW_EXECDEFAULT = 0x100,
	IMMERSIVE_OPENWITH_NONEDP_TO_EDP     = 0x200,
	IMMERSIVE_OPENWITH_EDP_TO_NONEDP     = 0x400,
	IMMERSIVE_OPENWITH_CALLING_IN_APP    = 0x800,
};

DEFINE_ENUM_FLAG_OPERATORS(IMMERSIVE_OPENWITH_FLAGS);
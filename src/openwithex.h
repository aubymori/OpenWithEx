#pragma once
#include <windows.h>

#define VER_MAJOR              2
#define VER_MINOR              0
#define VER_REVISION           0
#define VER_STRING       "2.0.0"

typedef enum _OPENWITHEX_STYLE
{
	OPENWITHEX_STYLE_VISTA = 0,
	OPENWITHEX_STYLE_XP,
	OPENWITHEX_STYLE_2000,
	OPENWITHEX_STYLE_NT4,
	OPENWITHEX_STYLE_COUNT,
} OPENWITHEX_STYLE;
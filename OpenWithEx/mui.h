/**
  * MUI - Load MUI files like Windows.
  * 
  * The default LoadMUILibraryW function does not support EXE
  * files. It will simply reload the exact same EXE. This
  * library will allow you to load MUI files from any module,
  * including EXEs.
  */

#pragma once
#include <windows.h>

#ifndef _mui_h_
#define _mui_h_

#ifdef __cplusplus
extern "C"
{
#endif
	#define FALLBACK_LOCALE  L"en-US"

	HMODULE GetMUIModule(HMODULE hMod);
	HMODULE LoadMUIModule(LPWSTR lpLibFileName);
#ifdef __cplusplus
}
#endif

#endif
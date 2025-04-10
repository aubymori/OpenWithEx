!include MUI2.nsh
!include LogicLib.nsh
!include x64.nsh
!include WinVer.nsh

# The install path is hardcoded.
# We need both a x64 and x86 OpenWith.exe and putting both
# in Program Files won't work. Having two directory choices
# would also be a mess.

Unicode true
Name "OpenWithEx"
Outfile "build\OpenWithEx-setup-x64.exe"
RequestExecutionLevel admin
ManifestSupportedOS all

!define MUI_ICON "installer.ico"
!define MUI_UNICON "installer.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP "welcome.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "welcome.bmp"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "header.bmp"
!define MUI_UNHEADERIMAGE_BITMAP "header.bmp"
!define MUI_ABORTWARNING
!define MUI_UNABORTWARNING
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_UNFINISHPAGE_NOAUTOCLOSE

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\LICENSE"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!macro LANG_LOAD LANGLOAD
    !insertmacro MUI_LANGUAGE "${LANGLOAD}"
    !include "l10n\${LANGLOAD}.nsh"
    !undef LANG
!macroend
 
!macro LANG_STRING NAME VALUE
    LangString "${NAME}" "${LANG_${LANG}}" "${VALUE}"
!macroend

!insertmacro LANG_LOAD "English"
!insertmacro LANG_LOAD "Japanese"
!insertmacro LANG_LOAD "Polish"
!insertmacro LANG_LOAD "Korean"
!insertmacro LANG_LOAD "Portuguese"
!insertmacro LANG_LOAD "Turkish"

Function .onInit
    # NSIS produces an x86-32 installer. Deny installation if
    # we're not on a x86-64 system running WOW64.
    ${IfNot} ${RunningX64}
        MessageBox MB_OK|MB_ICONSTOP "$(STRING_NOT_X64)"
        Quit
    ${EndIf}
    
    # Need at least Windows 8.
    ${IfNot} ${AtLeastWin8}
        MessageBox MB_OK|MB_ICONSTOP "$(STRING_NOT_WIN8)"
        Quit
    ${EndIf}
FunctionEnd

Section "OpenWithEx" OpenWithEx
    # Required
    SectionIn RO

    # Make sure install directories are clean
    RMDir /r "$PROGRAMFILES64\OpenWithEx"
    RMDir /r "$PROGRAMFILES32\OpenWithEx"

    # Install x86-64 files
    SetOutPath "$PROGRAMFILES64\OpenWithEx"
    WriteUninstaller "$PROGRAMFILES64\OpenWithEx\uninstall.exe"
    File "..\build\Release-x64\OpenWith.exe"
    File "..\build\Release-x64\config\OpenWithExConfig.exe"

    # Install x86-32 files
    SetOutPath "$PROGRAMFILES32\OpenWithEx"
    File "..\build\Release-Win32\OpenWith.exe"

    # Create configurator shortcut
    SetShellVarContext all
    CreateDirectory "$SMPROGRAMS\OpenWithEx"
    CreateShortCut "$SMPROGRAMS\OpenWithEx\$(STRING_CONFIG_SHORTCUT).lnk" \
        "$PROGRAMFILES64\OpenWithEx\OpenWithExConfig.exe"
    
    # Create Uninstall entry
    SetRegView 64
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenWithEx" \
                 "DisplayName" "OpenWithEx"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenWithEx" \
                 "DisplayIcon" "$PROGRAMFILES64\OpenWithEx\OpenWith.exe,0"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenWithEx" \
                 "UninstallString" "$\"$PROGRAMFILES64\OpenWithEx\uninstall.exe$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenWithEx" \
                 "Publisher" "aubymori"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenWithEx" \
                 "DisplayVersion" "1.1.0"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenWithEx" \
                 "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenWithEx" \
                 "NoRepair" 1

    # Make Open With use our server
    ReadEnvStr $0 "USERNAME"
    AccessControl::SetRegKeyOwner HKCR "CLSID\{e44e9428-bdbc-4987-a099-40dc8fd255e7}\LocalServer32" $0
    AccessControl::GrantOnRegKey HKCR "CLSID\{e44e9428-bdbc-4987-a099-40dc8fd255e7}\LocalServer32" $0 FullAccess
    WriteRegExpandStr HKCR "CLSID\{e44e9428-bdbc-4987-a099-40dc8fd255e7}\LocalServer32" \
        "" "$PROGRAMFILES64\OpenWithEx\OpenWith.exe"
    AccessControl::SetRegKeyOwner HKCR "WOW6432Node\CLSID\{e44e9428-bdbc-4987-a099-40dc8fd255e7}\LocalServer32" $0
    AccessControl::GrantOnRegKey HKCR "WOW6432Node\CLSID\{e44e9428-bdbc-4987-a099-40dc8fd255e7}\LocalServer32" $0 FullAccess
    WriteRegExpandStr HKCR "WOW6432Node\CLSID\{e44e9428-bdbc-4987-a099-40dc8fd255e7}\LocalServer32" \
        "" "$PROGRAMFILES32\OpenWithEx\OpenWith.exe"
SectionEnd

!macro InstallLang lang
    SetOutPath "$PROGRAMFILES64\OpenWithEx\${lang}"
    File "..\build\Release-x64\${lang}\OpenWith.exe.mui"
	File "..\build\Release-x64\config\${lang}\OpenWithExConfig.exe.mui"
    SetOutPath "$PROGRAMFILES32\OpenWithEx\${lang}"
    File "..\build\Release-Win32\${lang}\OpenWith.exe.mui"
!macroend

SectionGroup "$(STRING_LANGS)"
    Section "English (United States)"
        SectionIn RO
        !insertmacro InstallLang "en-US"
    SectionEnd

	Section "日本語"
        !insertmacro InstallLang "ja-JP"
    SectionEnd

    Section "Polish"
        !insertmacro InstallLang "pl-PL"
    SectionEnd

    Section "Português (Brasil)"
        !insertmacro InstallLang "pt-BR"
    SectionEnd

    Section "Türkçe"
        !insertmacro InstallLang "tr-TR"
    SectionEnd

    Section "한국어"
	!insertmacro InstallLang "ko-KR"
    SectionEnd
SectionGroupEnd

Section "Uninstall"
    # Delete files
    RMDir /r "$PROGRAMFILES64\OpenWithEx"
    RMDir /r "$PROGRAMFILES32\OpenWithEx"

    # Delete config shortcut
    SetShellVarContext all
    RMDir /r "$SMPROGRAMS\OpenWithEx"

    # Revert to default OpenWith server.
    SetRegView 64
    ReadEnvStr $0 "USERNAME"
    AccessControl::SetRegKeyOwner HKCR "CLSID\{e44e9428-bdbc-4987-a099-40dc8fd255e7}\LocalServer32" $0
    AccessControl::GrantOnRegKey HKCR "CLSID\{e44e9428-bdbc-4987-a099-40dc8fd255e7}\LocalServer32" $0 FullAccess
    WriteRegExpandStr HKCR "CLSID\{e44e9428-bdbc-4987-a099-40dc8fd255e7}\LocalServer32" \
        "" "%SystemRoot%\system32\OpenWith.exe"
    AccessControl::SetRegKeyOwner HKCR "WOW6432Node\CLSID\{e44e9428-bdbc-4987-a099-40dc8fd255e7}\LocalServer32" $0
    AccessControl::GrantOnRegKey HKCR "WOW6432Node\CLSID\{e44e9428-bdbc-4987-a099-40dc8fd255e7}\LocalServer32" $0 FullAccess
    WriteRegExpandStr HKCR "WOW6432Node\CLSID\{e44e9428-bdbc-4987-a099-40dc8fd255e7}\LocalServer32" \
        "" "%SystemRoot%\system32\OpenWith.exe"

    # Delete uninstall entry
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenWithEx"
SectionEnd

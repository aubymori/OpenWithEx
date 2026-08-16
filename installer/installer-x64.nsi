!include MUI2.nsh
!include LogicLib.nsh
!include x64.nsh
!include WinVer.nsh

!define VERSION "2.0.0"

Unicode true
Name "OpenWithEx"
Outfile "build\OpenWithEx-${VERSION}-x64.exe"
InstallDir "$PROGRAMFILES64\OpenWithEx"
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
!insertmacro MUI_PAGE_DIRECTORY
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
!insertmacro LANG_LOAD "Russian"
!insertmacro LANG_LOAD "Portuguese"
!insertmacro LANG_LOAD "Spanish"
!insertmacro LANG_LOAD "Turkish"

Function .onInit
    # NSIS produces an x86-32 installer. Deny installation if
    # we're not on a x86-64 system running WOW64.
    ${IfNot} ${RunningX64}
        MessageBox MB_OK|MB_ICONSTOP "$(STRING_NOT_X64)"
        Quit
    ${EndIf}
    
    # Need at least Windows 10.
    ${IfNot} ${AtLeastWin10}
        MessageBox MB_OK|MB_ICONSTOP "$(STRING_NOT_WIN10)"
        Quit
    ${EndIf}
FunctionEnd

Section "OpenWithEx" OpenWithEx
    # Required
    SectionIn RO

    # Make sure install directories are clean
    RMDir /r "$INSTDIR"

    # Install x86-64 files
    SetOutPath "$INSTDIR"
    WriteUninstaller "$INSTDIR\uninstall.exe"
    File "..\bin\Release-x64\OpenWith.exe"
    File "..\bin\Release-x64\config\OpenWithExConfig.exe"

    # Create configurator shortcut
    SetShellVarContext all
    CreateDirectory "$SMPROGRAMS\OpenWithEx"
    CreateShortCut "$SMPROGRAMS\OpenWithEx\$(STRING_CONFIG_SHORTCUT).lnk" \
        "$INSTDIR\OpenWithExConfig.exe"
    
    # Create Uninstall entry
    SetRegView 64
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenWithEx" \
                 "DisplayName" "OpenWithEx"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenWithEx" \
                 "DisplayIcon" "$INSTDIR\OpenWith.exe,0"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenWithEx" \
                 "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenWithEx" \
                 "Publisher" "aubymori"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenWithEx" \
                 "DisplayVersion" "${VERSION}"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenWithEx" \
                 "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenWithEx" \
                 "NoRepair" 1

    # Make Open With use our server
    ReadEnvStr $0 "USERNAME"
    AccessControl::SetRegKeyOwner HKCR "CLSID\{e44e9428-bdbc-4987-a099-40dc8fd255e7}\LocalServer32" $0
    AccessControl::GrantOnRegKey HKCR "CLSID\{e44e9428-bdbc-4987-a099-40dc8fd255e7}\LocalServer32" $0 FullAccess
    WriteRegExpandStr HKCR "CLSID\{e44e9428-bdbc-4987-a099-40dc8fd255e7}\LocalServer32" \
        "" "$INSTDIR\OpenWith.exe"
    AccessControl::SetRegKeyOwner HKCR "WOW6432Node\CLSID\{e44e9428-bdbc-4987-a099-40dc8fd255e7}\LocalServer32" $0
    AccessControl::GrantOnRegKey HKCR "WOW6432Node\CLSID\{e44e9428-bdbc-4987-a099-40dc8fd255e7}\LocalServer32" $0 FullAccess
    WriteRegExpandStr HKCR "WOW6432Node\CLSID\{e44e9428-bdbc-4987-a099-40dc8fd255e7}\LocalServer32" \
        "" "$INSTDIR\OpenWith.exe"
SectionEnd

!macro InstallLang lang
    SetOutPath "$INSTDIR\${lang}"
    File "..\bin\Release-x64\${lang}\OpenWith.exe.mui"
!macroend

!macro InstallConfigLang lang
    SetOutPath "$INSTDIR\${lang}"
    File "..\bin\Release-x64\config\${lang}\OpenWithExConfig.exe.mui"
!macroend

SectionGroup "$(STRING_LANGS)"
    Section "English (United States)"
        SectionIn RO
        !insertmacro InstallLang "en-US"
        !insertmacro InstallConfigLang "en-US"
    SectionEnd

    Section "العربية (المملكة العربية السعودية)"
        !insertmacro InstallLang "ar-SA"
    SectionEnd

    Section "Български"
        !insertmacro InstallLang "bg-BG"
    SectionEnd

    Section "Čeština"
        !insertmacro InstallLang "cs-CZ"
    SectionEnd

    Section "Dansk"
        !insertmacro InstallLang "da-DK"
    SectionEnd

    Section "Deutsch"
        !insertmacro InstallLang "de-DE"
    SectionEnd

    Section "Ελληνικά"
        !insertmacro InstallLang "el-GR"
    SectionEnd

    Section "Español"
        !insertmacro InstallLang "es-ES"
        !insertmacro InstallConfigLang "es-ES"
    SectionEnd

    Section "Eesti"
        !insertmacro InstallLang "et-EE"
    SectionEnd

    Section "Suomi"
        !insertmacro InstallLang "fi-FI"
    SectionEnd

    Section "Français"
        !insertmacro InstallLang "fr-FR"
    SectionEnd

    Section "עברית"
        !insertmacro InstallLang "he-IL"
    SectionEnd

    Section "Hrvatski"
        !insertmacro InstallLang "hr-HR"
    SectionEnd

    Section "Magyar"
        !insertmacro InstallLang "hu-HU"
    SectionEnd

    Section "Italiano"
        !insertmacro InstallLang "it-IT"
    SectionEnd

    Section "日本語"
        !insertmacro InstallLang "ja-JP"
        !insertmacro InstallConfigLang "ja-JP"
    SectionEnd

    Section "한국어"
        !insertmacro InstallLang "ko-KR"
        !insertmacro InstallConfigLang "ko-KR"
    SectionEnd

    Section "Lietuvių"
        !insertmacro InstallLang "lt-LT"
    SectionEnd

    Section "Latviešu"
        !insertmacro InstallLang "lv-LV"
    SectionEnd

    Section "Norsk bokmål"
        !insertmacro InstallLang "nb-NO"
    SectionEnd

    Section "Nederlands"
        !insertmacro InstallLang "nl-NL"
    SectionEnd

    Section "Polski"
        !insertmacro InstallLang "pl-PL"
        !insertmacro InstallConfigLang "pl-PL"
    SectionEnd

    Section "Português (Brasil)"
        !insertmacro InstallLang "pt-BR"
        !insertmacro InstallConfigLang "pt-BR"
    SectionEnd

    Section "Português (Portugal)"
        !insertmacro InstallLang "pt-PT"
    SectionEnd

    Section "Română"
        !insertmacro InstallLang "ro-RO"
    SectionEnd

    Section "Русский"
        !insertmacro InstallLang "ru-RU"
        !insertmacro InstallConfigLang "ru-RU"
    SectionEnd

    Section "Slovenčina"
        !insertmacro InstallLang "sk-SK"
    SectionEnd

    Section "Slovenščina"
        !insertmacro InstallLang "sl-SI"
    SectionEnd

    Section "Srpski (latinica)"
        !insertmacro InstallLang "sr-Latn-RS"
    SectionEnd

    Section "Svenska"
        !insertmacro InstallLang "sv-SE"
    SectionEnd

    Section "ไทย"
        !insertmacro InstallLang "th-TH"
    SectionEnd

    Section "Türkçe"
        !insertmacro InstallLang "tr-TR"
        !insertmacro InstallConfigLang "tr-TR"
    SectionEnd

    Section "Українська"
        !insertmacro InstallLang "uk-UA"
    SectionEnd

    Section "简体中文"
        !insertmacro InstallLang "zh-CN"
    SectionEnd

    Section "繁體中文"
        !insertmacro InstallLang "zh-TW"
    SectionEnd
SectionGroupEnd

Section "Uninstall"
    # Delete files
    RMDir /r "$INSTDIR"

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

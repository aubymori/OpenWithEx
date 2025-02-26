# OpenWithEx
OpenWithEx is an application meant to replicate the classic Open With dialog with styles
ranging from Windows 95 to Windows 7.

## Compilation

### Building the application
**Needed**:
- Visual Studio 2022 with C++ desktop development workload

To build OpenWithEx and its configurator, simply open OpenWithEx.sln and build the projects in the configurations you want.
If you want to build the installer, you must build the following projects and configurations:

- OpenWithEx, Release x64
- OpenWithEx, Release x86
- OpenWithExConfig, Release x64

### Building the installer
**Needed**:
- Nullsoft Installer System
- [AccessControl plugin for NSIS](https://nsis.sourceforge.io/AccessControl_plug-in)

**NOTE**: After copying the files for AccessControl into your NSIS installation, you need to go into your NSIS plugins directory (typically located at `C:\Program Files (x86)\NSIS\Plugins`) and copy the AccessControl.dll files from `i386-ansi` and `i386-unicode` to `x86-ansi` and `x86-unicode`. I'm not sure why this distinction is made, but not having the files in the x86 folder breaks compilation.

Run the script at `installer\build_installer.cmd`.
Unicode True

!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"

!define PRODUCT_NAME "OBS Studio 播放列表版"
!define PRODUCT_VERSION "32.2.1"
!define PRODUCT_PUBLISHER "OBS Project / CodeYan01"
!define PRODUCT_WEB_SITE "https://github.com/obsproject/obs-studio"
!define PRODUCT_DIR_REGKEY "Software\OBS Studio Playlist Edition"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\OBS Studio Playlist Edition"
!define UNINSTALL_EXE "uninstall-playlist-edition.exe"
!define REPO_ROOT "${__FILEDIR__}\.."

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "${REPO_ROOT}\dist\OBS-Studio-32.2.1-Playlist-Edition-Setup.exe"
InstallDir "$PROGRAMFILES64\OBS Studio Playlist Edition"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" "InstallLocation"
RequestExecutionLevel admin
SetCompressor /SOLID zlib
ShowInstDetails show
ShowUninstDetails show
BrandingText "OBS Studio 32.2.1 + Media Playlist Source 0.1.3"

VIProductVersion "32.2.1.0"
VIAddVersionKey /LANG=2052 "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey /LANG=2052 "ProductVersion" "${PRODUCT_VERSION}"
VIAddVersionKey /LANG=2052 "CompanyName" "${PRODUCT_PUBLISHER}"
VIAddVersionKey /LANG=2052 "FileDescription" "包含媒体播放列表源的 OBS Studio 中文安装器"
VIAddVersionKey /LANG=2052 "FileVersion" "32.2.1.0"
VIAddVersionKey /LANG=2052 "LegalCopyright" "OBS Project and CodeYan01 contributors; GPL licensed"

!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"
!define MUI_WELCOMEPAGE_TITLE "欢迎安装 ${PRODUCT_NAME}"
!define MUI_WELCOMEPAGE_TEXT "安装程序将安装 OBS Studio ${PRODUCT_VERSION} 和 Media Playlist Source 0.1.3。安装完成后，OBS 中会直接出现“媒体播放列表源”，可用于循环、随机播放和切换本地素材。"
!insertmacro MUI_PAGE_WELCOME
!define MUI_LICENSEPAGE_CHECKBOX
!insertmacro MUI_PAGE_LICENSE "${__FILEDIR__}\LICENSE-NOTICE.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT "运行 ${PRODUCT_NAME}"
!define MUI_FINISHPAGE_RUN_FUNCTION LaunchOBS
!define MUI_FINISHPAGE_LINK "查看 OBS Studio 项目主页"
!define MUI_FINISHPAGE_LINK_LOCATION "${PRODUCT_WEB_SITE}"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "SimpChinese"

Function CheckOBSRunning
  nsExec::ExecToStack 'cmd.exe /C tasklist /FI "IMAGENAME eq obs64.exe" /NH | find /I "obs64.exe" >nul'
  Pop $0
  Pop $1
  ${If} $0 == 0
    IfSilent silent_obs_running interactive_obs_running
    interactive_obs_running:
      MessageBox MB_OK|MB_ICONEXCLAMATION "检测到 OBS Studio 正在运行。请先保存工作并关闭 OBS，然后重新运行安装程序。"
      Abort
    silent_obs_running:
      SetErrorLevel 10
      Quit
  ${EndIf}
FunctionEnd

Function .onInit
  ${IfNot} ${RunningX64}
    MessageBox MB_OK|MB_ICONSTOP "本安装包只支持 64 位 Windows 10/11。"
    Abort
  ${EndIf}
  Call CheckOBSRunning
FunctionEnd

Section "OBS Studio 播放列表版（必选）" SecMain
  SectionIn RO
  SetRegView 64
  SetShellVarContext all
  SetOverwrite on

  SetOutPath "$INSTDIR"
  File /r "${REPO_ROOT}\build\payload\*.*"

  FileOpen $0 "$INSTDIR\.obs-playlist-edition.install" w
  FileWrite $0 "OBS Studio Playlist Edition ${PRODUCT_VERSION}"
  FileClose $0

  WriteUninstaller "$INSTDIR\${UNINSTALL_EXE}"

  CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
  SetOutPath "$INSTDIR\bin\64bit"
  CreateShortCut "$DESKTOP\${PRODUCT_NAME}.lnk" "$INSTDIR\bin\64bit\obs64.exe" "" "$INSTDIR\bin\64bit\obs64.exe" 0 SW_SHOWNORMAL "" "启动包含媒体播放列表源的 OBS Studio"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk" "$INSTDIR\bin\64bit\obs64.exe" "" "$INSTDIR\bin\64bit\obs64.exe" 0 SW_SHOWNORMAL "" "启动包含媒体播放列表源的 OBS Studio"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\卸载 ${PRODUCT_NAME}.lnk" "$INSTDIR\${UNINSTALL_EXE}"
  SetOutPath "$INSTDIR"

  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\bin\64bit\obs64.exe"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "UninstallString" '"$INSTDIR\${UNINSTALL_EXE}"'
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "QuietUninstallString" '"$INSTDIR\${UNINSTALL_EXE}" /S'
  WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "NoModify" 1
  WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "NoRepair" 1
  WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "EstimatedSize" 416000
SectionEnd

Function LaunchOBS
  SetOutPath "$INSTDIR\bin\64bit"
  Exec '"$INSTDIR\bin\64bit\obs64.exe"'
FunctionEnd

Function un.CheckOBSRunning
  nsExec::ExecToStack 'cmd.exe /C tasklist /FI "IMAGENAME eq obs64.exe" /NH | find /I "obs64.exe" >nul'
  Pop $0
  Pop $1
  ${If} $0 == 0
    IfSilent un_silent_obs_running un_interactive_obs_running
    un_interactive_obs_running:
      MessageBox MB_OK|MB_ICONEXCLAMATION "检测到 OBS Studio 正在运行。请先保存工作并关闭 OBS，然后重新运行卸载程序。"
      Abort
    un_silent_obs_running:
      SetErrorLevel 10
      Quit
  ${EndIf}
FunctionEnd

Function un.onInit
  Call un.CheckOBSRunning
FunctionEnd

Section "Uninstall"
  SetRegView 64
  SetShellVarContext all

  IfFileExists "$INSTDIR\.obs-playlist-edition.install" marker_ok
    MessageBox MB_OK|MB_ICONSTOP "没有找到本产品的安装标记。为避免误删文件，卸载已中止。"
    Abort
  marker_ok:

  Delete "$DESKTOP\${PRODUCT_NAME}.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\卸载 ${PRODUCT_NAME}.lnk"
  RMDir "$SMPROGRAMS\${PRODUCT_NAME}"

  DeleteRegKey HKLM "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"

  SetOutPath "$TEMP"
  RMDir /r "$INSTDIR"
SectionEnd

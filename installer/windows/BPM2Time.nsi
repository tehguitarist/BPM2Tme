; BPM2Time VST3 installer (NSIS). Windows has no AU format, so there's no plugin-type choice
; here — VST3 only, installed to the shared system VST3 folder.
;
; Build with (from repo root, after building BPM2Time_VST3):
;   makensis /DVERSION=1.2.0 /DARTEFACTS_DIR=build\BPM2Time_artefacts\Release\VST3 installer\windows\BPM2Time.nsi
;
; ARTEFACTS_DIR should point at the directory CONTAINING BPM2Time.vst3 (i.e. the VST3 release
; output folder), not BPM2Time.vst3 itself.

!ifndef VERSION
  !define VERSION "0.0.0"
!endif
!ifndef ARTEFACTS_DIR
  !define ARTEFACTS_DIR "..\..\build\BPM2Time_artefacts\Release\VST3"
!endif

Name "BPM2Time"
OutFile "BPM2Time-Windows-v${VERSION}-Installer.exe"
InstallDir "$COMMONFILES64\VST3"
RequestExecutionLevel admin
SetCompressor /SOLID lzma

Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

Section "BPM2Time VST3 Plugin" SecVST3
    SetOutPath "$INSTDIR\BPM2Time.vst3"
    File /r "${ARTEFACTS_DIR}\BPM2Time.vst3\*.*"

    WriteUninstaller "$INSTDIR\BPM2Time.vst3\Uninstall-BPM2Time.exe"

    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BPM2Time" \
        "DisplayName" "BPM2Time VST3"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BPM2Time" \
        "UninstallString" "$INSTDIR\BPM2Time.vst3\Uninstall-BPM2Time.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BPM2Time" \
        "DisplayVersion" "${VERSION}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BPM2Time" \
        "Publisher" "Leigh Pierce"
SectionEnd

Section "Uninstall"
    RMDir /r "$INSTDIR\BPM2Time.vst3"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BPM2Time"
SectionEnd

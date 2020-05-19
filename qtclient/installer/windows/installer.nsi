; Copyright (C) 2012-2020 Stefan Hajnoczi <stefanha@gmail.com>
;
; Wahjam is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.
;
; Wahjam is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with Wahjam; if not, write to the Free Software
; Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

!include x64.nsh
!include Sections.nsh

!ifndef PROGRAM_NAME
  !define PROGRAM_NAME Wahjam
!endif
!ifndef EXECUTABLE
  !define EXECUTABLE "${PROGRAM_NAME}.exe"
!endif
!ifndef EXECUTABLE64
  !define EXECUTABLE64 "${PROGRAM_NAME}64.exe"
!endif
!ifdef VERSION
  VIProductVersion "${VERSION}"
  Caption "${PROGRAM_NAME} ${VERSION} Setup"
!endif

!define REG_UNINSTALL "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PROGRAM_NAME}"

; Install for current user without User Account Control elevation
RequestExecutionLevel user

OutFile ${PROGRAM_NAME}-installer.exe
Name ${PROGRAM_NAME}
InstallDir "$LOCALAPPDATA\${PROGRAM_NAME}"
SetCompressor /SOLID lzma
LicenseData license.txt

Page license
Page components
Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

Section "-Install"
  SetOutPath $INSTDIR
  File license.txt
  WriteUninstaller uninstall.exe

  ; Add/Remove Programs
  WriteRegStr HKCU "${REG_UNINSTALL}" "DisplayName" "${PROGRAM_NAME}"
  WriteRegStr HKCU "${REG_UNINSTALL}" "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
  WriteRegStr HKCU "${REG_UNINSTALL}" "QuietUninstallString" "$\"$INSTDIR\uninstall.exe$\" /S"
  WriteRegStr HKCU "${REG_UNINSTALL}" "InstallLocation" "$\"$INSTDIR$\""
  WriteRegDWORD HKCU "${REG_UNINSTALL}" "NoModify" 1
  WriteRegDWORD HKCU "${REG_UNINSTALL}" "NoRepair" 1
SectionEnd

SectionGroup /e "32-bit" SEC_32BIT
Section /o "${PROGRAM_NAME} 32-bit"
  File "${EXECUTABLE}"
  CreateShortCut "$SMPROGRAMS\${PROGRAM_NAME}.lnk" "$INSTDIR\${PROGRAM_NAME}.exe"
SectionEnd

Section /o "Desktop shortcut"
  CreateShortCut "$DESKTOP\${PROGRAM_NAME}.lnk" "$INSTDIR\${PROGRAM_NAME}.exe"
SectionEnd

Section /o "Launch ${PROGRAM_NAME} on completion"
  Exec '"$INSTDIR\${PROGRAM_NAME}.exe"'
SectionEnd
SectionGroupEnd

SectionGroup /e "64-bit" SEC_64BIT
Section /o "${PROGRAM_NAME} 64-bit"
  File "${EXECUTABLE64}"
  CreateShortCut "$SMPROGRAMS\${PROGRAM_NAME} (64-bit).lnk" "$INSTDIR\${PROGRAM_NAME}64.exe"
SectionEnd

Section /o "Desktop shortcut"
  CreateShortCut "$DESKTOP\${PROGRAM_NAME} (64-bit).lnk" "$INSTDIR\${PROGRAM_NAME}64.exe"
SectionEnd

Section /o "Launch ${PROGRAM_NAME} on completion"
  Exec '"$INSTDIR\${PROGRAM_NAME}64.exe"'
SectionEnd
SectionGroupEnd

Section Uninstall
  Delete "$DESKTOP\${PROGRAM_NAME}.lnk"
  ${If} ${RunningX64}
  Delete "$DESKTOP\${PROGRAM_NAME} (64-bit).lnk"
  ${EndIf}

  DeleteRegKey HKCU "${REG_UNINSTALL}"
  DeleteRegKey HKCU "Software\${PROGRAM_NAME}\${PROGRAM_NAME}"
  DeleteRegKey /ifempty HKCU "Software\${PROGRAM_NAME}"
  Delete "$SMPROGRAMS\${PROGRAM_NAME}.lnk"
  ${If} ${RunningX64}
  Delete "$SMPROGRAMS\${PROGRAM_NAME} (64-bit).lnk"
  ${EndIf}

  SetOutPath $INSTDIR
  Delete license.txt
  Delete ${PROGRAM_NAME}.exe
  ${If} ${RunningX64}
  Delete ${PROGRAM_NAME}64.exe
  ${EndIf}
  Delete uninstall.exe
  Delete "${PROGRAM_NAME}\log.txt"
  RMDir "${PROGRAM_NAME}"
  SetOutPath $TEMP
  RMDir $INSTDIR
SectionEnd

Function .onInit
  ${If} ${RunningX64}
    !insertmacro SetSectionFlag ${SEC_64BIT} ${SF_SELECTED}
  ${Else}
    !insertmacro SetSectionFlag ${SEC_64BIT} ${SF_RO}
    !insertmacro SetSectionFlag ${SEC_32BIT} ${SF_SELECTED}
    !insertmacro ClearSectionFlag ${SEC_32BIT} ${SF_EXPAND}
  ${EndIf}

  ; Try to get focus in case we've been hidden behind another window
  BringToFront
FunctionEnd

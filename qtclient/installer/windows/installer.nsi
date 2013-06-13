; Copyright (C) 2012 Stefan Hajnoczi <stefanha@gmail.com>
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

!ifndef PROGRAM_NAME
  !define PROGRAM_NAME Wahjam
!endif
!ifndef EXECUTABLE
  !define EXECUTABLE "../../release/${PROGRAM_NAME}.exe"
!endif

!define REG_UNINSTALL "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PROGRAM_NAME}"

; Install system-wide by default
RequestExecutionLevel admin

OutFile ${PROGRAM_NAME}-installer.exe
Name ${PROGRAM_NAME}
InstallDir $PROGRAMFILES\${PROGRAM_NAME}
SetCompressor /SOLID lzma
LicenseData license.txt

Page license
Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

Function .onInit
  ; Try to get focus in case we've been hidden behind another window
  BringToFront
FunctionEnd

Section Install
  SetOutPath $INSTDIR
  File license.txt
  File "${EXECUTABLE}"
  WriteUninstaller uninstall.exe

  ; Start Menu entry
  CreateShortCut "$SMPROGRAMS\${PROGRAM_NAME}.lnk" "$INSTDIR\${PROGRAM_NAME}.exe"

  ; Add/Remove Programs
  WriteRegStr HKLM "${REG_UNINSTALL}" "DisplayName" "${PROGRAM_NAME}"
  WriteRegStr HKLM "${REG_UNINSTALL}" "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
  WriteRegStr HKLM "${REG_UNINSTALL}" "QuietUninstallString" "$\"$INSTDIR\uninstall.exe$\" /S"
  WriteRegStr HKLM "${REG_UNINSTALL}" "InstallLocation" "$\"$INSTDIR$\""
  WriteRegDWORD HKLM "${REG_UNINSTALL}" "NoModify" 1
  WriteRegDWORD HKLM "${REG_UNINSTALL}" "NoRepair" 1
SectionEnd

Section Uninstall
  DeleteRegKey HKLM "${REG_UNINSTALL}"
  Delete "$SMPROGRAMS\${PROGRAM_NAME}.lnk"

  SetOutPath $INSTDIR
  Delete license.txt
  Delete ${PROGRAM_NAME}.exe
  Delete uninstall.exe
  SetOutPath $TEMP
  RMDir $INSTDIR
SectionEnd

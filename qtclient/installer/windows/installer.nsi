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

Section /o "${PROGRAM_NAME} 32-bit" SEC_32BIT
  CreateDirectory "$INSTDIR\32"
  SetOutPath "$INSTDIR\32"
  File "32\${EXECUTABLE}"
  File "32\*.dll"
  CreateDirectory "$INSTDIR\32\plugins"
  CreateDirectory "$INSTDIR\32\plugins\imageformats"
  SetOutPath "$INSTDIR\32\plugins\imageformats"
  File "32\plugins\imageformats\*.dll"
  CreateDirectory "$INSTDIR\32\plugins\iconengines"
  SetOutPath "$INSTDIR\32\plugins\iconengines"
  File "32\plugins\iconengines\*.dll"
  CreateDirectory "$INSTDIR\32\plugins\platforms"
  SetOutPath "$INSTDIR\32\plugins\platforms"
  File "32\plugins\platforms\*.dll"
  CreateDirectory "$INSTDIR\32\plugins\platformthemes"
  SetOutPath "$INSTDIR\32\plugins\platformthemes"
  File "32\plugins\platformthemes\*.dll"
  CreateDirectory "$INSTDIR\32\plugins\styles"
  SetOutPath "$INSTDIR\32\plugins\styles"
  File "32\plugins\styles\*.dll"
  CreateShortCut "$SMPROGRAMS\${PROGRAM_NAME}.lnk" "$INSTDIR\32\${EXECUTABLE}"
SectionEnd

Section /o "${PROGRAM_NAME} 64-bit" SEC_64BIT
  CreateDirectory "$INSTDIR\64"
  SetOutPath "$INSTDIR\64"
  File "64\${EXECUTABLE}"
  File "64\*.dll"
  CreateDirectory "$INSTDIR\64\plugins"
  CreateDirectory "$INSTDIR\64\plugins\imageformats"
  SetOutPath "$INSTDIR\64\plugins\imageformats"
  File "64\plugins\imageformats\*.dll"
  CreateDirectory "$INSTDIR\64\plugins\iconengines"
  SetOutPath "$INSTDIR\64\plugins\iconengines"
  File "64\plugins\iconengines\*.dll"
  CreateDirectory "$INSTDIR\64\plugins\platforms"
  SetOutPath "$INSTDIR\64\plugins\platforms"
  File "64\plugins\platforms\*.dll"
  CreateDirectory "$INSTDIR\64\plugins\platformthemes"
  SetOutPath "$INSTDIR\64\plugins\platformthemes"
  File "64\plugins\platformthemes\*.dll"
  CreateDirectory "$INSTDIR\64\plugins\styles"
  SetOutPath "$INSTDIR\64\plugins\styles"
  File "64\plugins\styles\*.dll"
  CreateShortCut "$SMPROGRAMS\${PROGRAM_NAME} (64-bit).lnk" "$INSTDIR\64\${EXECUTABLE}"
SectionEnd

Section "Desktop shortcut"
  ${If} ${SectionIsSelected} ${SEC_32BIT}
    CreateShortCut "$DESKTOP\${PROGRAM_NAME}.lnk" "$INSTDIR\32\${EXECUTABLE}"
  ${EndIf}
  ${If} ${SectionIsSelected} ${SEC_64BIT}
    CreateShortCut "$DESKTOP\${PROGRAM_NAME} (64-bit).lnk" "$INSTDIR\64\${EXECUTABLE}"
  ${EndIf}
SectionEnd

Section "Launch ${PROGRAM_NAME} on completion"
  ${If} ${SectionIsSelected} ${SEC_64BIT}
    Exec '"$INSTDIR\64\${EXECUTABLE}"'
  ${Else}
    Exec '"$INSTDIR\32\${EXECUTABLE}"'
  ${EndIf}
SectionEnd

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
  Delete "32\${EXECUTABLE}"
  Delete "32\*.dll"
  Delete "32\plugins\imageformats\*.dll"
  RMDir "$INSTDIR\32\plugins\imageformats"
  Delete "32\plugins\iconengines\*.dll"
  RMDir "$INSTDIR\32\plugins\iconengines"
  Delete "32\plugins\platforms\*.dll"
  RMDir "$INSTDIR\32\plugins\platforms"
  Delete "32\plugins\platformthemes\*.dll"
  RMDir "$INSTDIR\32\plugins\platformthemes"
  Delete "32\plugins\styles\*.dll"
  RMDir "$INSTDIR\32\plugins\styles"
  RMDir "$INSTDIR\32\plugins"
  RMDir "$INSTDIR\32"
  ${If} ${RunningX64}
  Delete "64\${EXECUTABLE}"
  Delete "64\*.dll"
  Delete "64\plugins\imageformats\*.dll"
  RMDir "$INSTDIR\64\plugins\imageformats"
  Delete "64\plugins\iconengines\*.dll"
  RMDir "$INSTDIR\64\plugins\iconengines"
  Delete "64\plugins\platforms\*.dll"
  RMDir "$INSTDIR\64\plugins\platforms"
  Delete "64\plugins\platformthemes\*.dll"
  RMDir "$INSTDIR\64\plugins\platformthemes"
  Delete "64\plugins\styles\*.dll"
  RMDir "$INSTDIR\64\plugins\styles"
  RMDir "$INSTDIR\64\plugins"
  RMDir "$INSTDIR\64"
  ${EndIf}
  Delete uninstall.exe
  Delete "${PROGRAM_NAME}\log.txt"
  RMDir "$INSTDIR\${PROGRAM_NAME}"
  SetOutPath $TEMP
  RMDir $INSTDIR
SectionEnd

Function .onInit
  ${If} ${RunningX64}
    !insertmacro SetSectionFlag ${SEC_64BIT} ${SF_SELECTED}
  ${Else}
    !insertmacro SetSectionFlag ${SEC_64BIT} ${SF_RO}
    !insertmacro SetSectionFlag ${SEC_32BIT} ${SF_SELECTED}
  ${EndIf}

  ; Try to get focus in case we've been hidden behind another window
  BringToFront
FunctionEnd

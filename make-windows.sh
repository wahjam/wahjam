#!/bin/bash
# Build for Windows and generate an installer in qtclient/installer/windows/.
# Make sure 32- and 64-bit compiler toolchains are in PATH before running.
#
# Usage: make-windows.sh wahjam|jammr

target=${1:-wahjam}

make distclean
set -e

i686-w64-mingw32.static-qmake-qt5 CONFIG+="$target" CONFIG+=qtclient
make -j$(nproc)
mv qtclient/release/"$target".exe qtclient/installer/windows/
make distclean

x86_64-w64-mingw32.static-qmake-qt5 CONFIG+="$target" CONFIG+=qtclient
make -j$(nproc)
mv qtclient/release/"$target".exe qtclient/installer/windows/"$target"64.exe
make distclean

cd qtclient/installer/windows
makensis -DPROGRAM_NAME="$target" installer.nsi

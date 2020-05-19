#!/bin/bash
# Build for Windows and generate an installer in qtclient/installer/windows/.
# Make sure 32- and 64-bit compiler toolchains are in PATH before running.
#
# Usage: make-windows.sh wahjam|jammr

target=${1:-wahjam}
version=${2:-$(grep 'VERSION =' qtclient/qtclient.pro | awk '{ print $3 }')}

make distclean
set -e

i686-w64-mingw32.static-qmake-qt5 CONFIG+="$target" CONFIG+=qtclient VERSION="$version"
make -j$(nproc)
mv qtclient/release/"$target".exe qtclient/installer/windows/
make distclean

x86_64-w64-mingw32.static-qmake-qt5 CONFIG+="$target" CONFIG+=qtclient VERSION="$version"
make -j$(nproc)
mv qtclient/release/"$target".exe qtclient/installer/windows/"$target"64.exe
make distclean

cd qtclient/installer/windows
makensis -DPROGRAM_NAME="$target" -DVERSION="$version" installer.nsi

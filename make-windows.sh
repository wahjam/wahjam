#!/bin/bash
# Build for Windows and generate an installer in qtclient/installer/windows/.
# Make sure 32- and 64-bit compiler toolchains are in PATH before running.
#
# Usage: make-windows.sh wahjam|jammr

target=${1:-wahjam}
version=${2:-$(grep 'VERSION =' qtclient/qtclient.pro | awk '{ print $3 }')}

common_dlls=(bin/libogg-0.dll
	     bin/libportaudio-2.dll
	     bin/libportmidi.dll
	     bin/libqt5keychain.dll
	     bin/libvorbis-0.dll
	     bin/libvorbisenc-2.dll
	     bin/libstdc++-6.dll
	     bin/libwinpthread-1.dll
	     bin/libpcre-1.dll
	     bin/libpcre2-16-0.dll
	     bin/zlib1.dll
	     bin/libzstd.dll
	     bin/libharfbuzz-0.dll
	     bin/libpng16-16.dll
	     bin/libfreetype-6.dll
	     bin/libglib-2.0-0.dll
	     bin/libintl-8.dll
	     bin/libbz2.dll
	     bin/libiconv-2.dll
	     qt5/bin/Qt5Core.dll
	     qt5/bin/Qt5Gui.dll
	     qt5/bin/Qt5Network.dll
	     qt5/bin/Qt5Widgets.dll)

i686_dlls=(bin/libgcc_s_sjlj-1.dll
	   bin/libcrypto-1_1.dll
	   bin/libssl-1_1.dll)

x86_64_dlls=(bin/libgcc_s_seh-1.dll
	     bin/libcrypto-1_1-x64.dll
	     bin/libssl-1_1-x64.dll)

copy_dlls() {
	dll_dir="$1"
	dest_dir="$2"
	shift
	shift

	mkdir -p "$dest_dir"
	for f in "${common_dlls[@]}" "$@"; do
	       echo cp "$dll_dir/$f" "$dest_dir/"
	       cp "$dll_dir/$f" "$dest_dir/"
	done
}

copy_qt_plugins() {
	dll_dir="$1"
	dest_dir="$2"

	mkdir -p "$dest_dir/plugins"
	cp -r "$dll_dir/qt5/plugins/imageformats" \
	      "$dll_dir/qt5/plugins/iconengines" \
	      "$dll_dir/qt5/plugins/platforms" \
	      "$dll_dir/qt5/plugins/platformthemes" \
	      "$dll_dir/qt5/plugins/styles" \
	      "$dest_dir/plugins/"
}

make distclean
rm -rf qtclient/installer/windows/32
rm -rf qtclient/installer/windows/64

set -e

i686-w64-mingw32.shared-qmake-qt5 CONFIG+="$target" CONFIG+=qtclient VERSION="$version"
make -j$(nproc)
copy_dlls ~/mxe/usr/i686-w64-mingw32.shared qtclient/installer/windows/32/ "${i686_dlls[@]}"
copy_qt_plugins ~/mxe/usr/i686-w64-mingw32.shared qtclient/installer/windows/32/
mv qtclient/release/"$target".exe qtclient/installer/windows/32/
make distclean

x86_64-w64-mingw32.shared-qmake-qt5 CONFIG+="$target" CONFIG+=qtclient VERSION="$version"
make -j$(nproc)
copy_dlls ~/mxe/usr/x86_64-w64-mingw32.shared qtclient/installer/windows/64/ "${x86_64_dlls[@]}"
copy_qt_plugins ~/mxe/usr/x86_64-w64-mingw32.shared qtclient/installer/windows/64/
mv qtclient/release/"$target".exe qtclient/installer/windows/64/
make distclean

cd qtclient/installer/windows
makensis -DPROGRAM_NAME="$target" -DVERSION="$version" installer.nsi

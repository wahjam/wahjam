#!/bin/sh
rm -fr dist
mkdir dist
cp -Rp build/NINJAM.app dist/NINJAM.app
cp ../license.txt dist/
hdiutil create -ov -fs HFS+ -volname NINJAM_GUI_OSX -format UDZO -srcfolder dist/ ninjam_gui_osx.dmg && hdiutil internet-enable ninjam_gui_osx.dmg

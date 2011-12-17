#!/bin/sh
rm -fr dist
mkdir dist
cp -Rp build/Wahjam.app dist/Wahjam.app
cp ../license.txt dist/
hdiutil create -ov -fs HFS+ -volname WAHJAM_GUI_OSX -format UDZO -srcfolder dist/ wahjam_gui_osx.dmg && hdiutil internet-enable wahjam_gui_osx.dmg

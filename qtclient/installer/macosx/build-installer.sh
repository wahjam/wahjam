#!/bin/bash
#
# Mac OS X installer build script
#
# Copyright (C) 2013 Stefan Hajnoczi <stefanha@gmail.com>
#
# Wahjam is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Wahjam is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Wahjam; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

rm -f $1.dmg

# Only configure a background image if available
if [ -e background-$1.png ]
then
	background_opts="--window-size 800 600 --background background-$1.png --icon $1 200 351 --app-drop-link 596 351"
fi

yoursway-create-dmg/create-dmg \
	--volname $1 \
	--eula license.txt \
	$background_opts \
	$1.dmg ../../$1.app

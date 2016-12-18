#!/usr/bin/env python
#
# Make library dependency paths relative to bundle:
#
# $ fix-install-names.py qtclient/jammr.app
#
# Copyright (C) 2012 Stefan Hajnoczi <stefanha@gmail.com>
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

import subprocess
import sys
import os
import re

DEPENDENCY_RE = re.compile(r'\s+(/usr/local/.+\.dylib) \(compatibility version [^,]+, current version [^)]+\)')

executable_paths = {}

def usage(argv):
    print 'usage: %s APP_BUNDLE_PATH' % argv[0]
    sys.exit(1)

def process_dylib(filename):
    print 'Processing %s...' % filename

    output = subprocess.check_output(['otool', '-L', filename], stderr=subprocess.STDOUT)
    for line in output.splitlines():
        m = DEPENDENCY_RE.match(line)
        if not m:
            continue

        dependency = m.group(1)
        new_path = executable_paths[os.path.basename(dependency)]

        print 'Changing %s to %s...' % (dependency, new_path)
        subprocess.check_call(['install_name_tool', '-change', dependency, new_path, filename])

def main(argv):
    if len(argv) != 2:
        usage(argv)

    executable_dir = os.path.join(argv[1], 'Contents', 'MacOS')
    dylibs = []

    for dirpath, dirnames, filenames in os.walk(argv[1]):
        for filename in filter(lambda s: s.endswith('.dylib'), filenames):
            path = os.path.join(dirpath, filename)
            rel_path = os.path.relpath(path, executable_dir)
            executable_paths[filename] = os.path.join('@executable_path', rel_path)
            dylibs.append(path)

    for filename in dylibs:
        process_dylib(filename)

    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv))

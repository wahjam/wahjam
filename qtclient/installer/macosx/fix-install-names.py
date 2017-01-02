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

DEPENDENCY_RE = re.compile(r'\s+(/usr/local/[^\s]+) \(compatibility version [^,]+, current version [^)]+\)')

processed = set()

def usage(argv):
    print 'usage: %s APP_BUNDLE_PATH' % argv[0]
    sys.exit(1)

def get_executable_rel_path(filename):
    components = filename.split(os.sep)
    if 'lib' in components:
        idx = components.index('lib')
        libdir = 'Frameworks'
    elif 'plugins' in components:
        idx = components.index('plugins')
        libdir = 'PlugIns'
    else:
        raise ValueError('get_bundle_path: unable to handle file %s' % filename)
    return os.path.join('..', libdir, *components[idx + 1:])

def process_macho(filename):
    if filename in processed:
        return
    processed.add(filename)

    print 'Processing %s...' % filename

    # Update shared library id
    output = subprocess.check_output(['otool', '-D', filename], stderr=subprocess.STDOUT)
    macho_id = output.splitlines()[-1]
    if macho_id.startswith('/usr/local/'):
        new_id = os.path.join('@executable_path', '..', get_executable_rel_path(macho_id))
        print 'Updating shared library id %s to %s...' % (macho_id, new_id)
        subprocess.check_call(['install_name_tool', '-id', new_id, filename])

    # Change dependencies to bundle-relative paths
    pending = set()
    output = subprocess.check_output(['otool', '-L', filename], stderr=subprocess.STDOUT)
    for line in output.splitlines():
        m = DEPENDENCY_RE.match(line)
        if not m:
            continue

        dependency = m.group(1)
        rel_path = get_executable_rel_path(dependency)

        pending.add(os.path.join(executable_dir, rel_path))

        new_path = os.path.join('@executable_path', rel_path)

        print 'Changing %s to %s...' % (dependency, new_path)
        subprocess.check_call(['install_name_tool', '-change', dependency, new_path, filename])

    for path in pending:
        process_macho(path)

def main(argv):
    global executable_dir

    if len(argv) != 2:
        usage(argv)

    executable_dir = os.path.join(argv[1], 'Contents', 'MacOS')

    for dirpath, dirnames, filenames in os.walk(executable_dir):
        for filename in filenames:
            path = os.path.join(dirpath, filename)
            process_macho(path)

    for dirpath, dirnames, filenames in os.walk(argv[1]):
        for filename in filter(lambda s: s.endswith('.dylib'), filenames):
            path = os.path.join(dirpath, filename)
            process_macho(path)

    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv))

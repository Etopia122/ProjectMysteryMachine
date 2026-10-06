#!/usr/bin/env python3
# Print the PS2 DWARF layout of RenderWare (or other) classes, first definition found.
# usage: python pc/tools/dwarfclass.py <dwarf flat dir> <Name>...
import glob, re, sys
d = sys.argv[1]
files = glob.glob(d + '/*.cpp') + glob.glob(d + '/*/*.c')
for name in sys.argv[2:]:
    for f in files:
        s = open(f, encoding='latin1').read()
        m = re.search(r'^(class|enum|union|struct) %s \{\n.*?^\};\n(// total size: \S+\n)?' % re.escape(name), s, re.S | re.M)
        if m:
            print(m.group(0).replace('    // Members\npublic:\n', ''))
            break
    else:
        print('// %s: not found' % name)

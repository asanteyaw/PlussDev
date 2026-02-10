#!/usr/bin/env python3
import os
import re

ROOT = os.path.join(os.path.dirname(__file__), '../include/internals/core/PlussQit')

for dirpath, dirnames, filenames in os.walk(ROOT):
    for filename in filenames:
        if not (filename.endswith('.h') or filename.endswith('.cpp') or filename.endswith('.hpp') or filename.endswith('.cxx')):
            continue
        filepath = os.path.join(dirpath, filename)
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
        new_content = re.sub(r'#include\s+<PlussQit/', '#include <PlussQit/', content)
        if new_content != content:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(new_content)

print('All #include <PlussQit/...> replaced with #include <PlussQit/...> in PlussQit directory.')

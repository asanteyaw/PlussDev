#!/usr/bin/env python3
import os
import re

ROOT = os.path.join(os.path.dirname(__file__), '../include/internals/core/Self')

# File renaming
for dirpath, dirnames, filenames in os.walk(ROOT):
    for filename in filenames:
        if 'Series' in filename:
            new_filename = filename.replace('Series', 'Series')
            os.rename(os.path.join(dirpath, filename), os.path.join(dirpath, new_filename))

# In-file replacement
pattern = re.compile(r'\bTensor\b')
for dirpath, dirnames, filenames in os.walk(ROOT):
    for filename in filenames:
        filepath = os.path.join(dirpath, filename)
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
        new_content = pattern.sub('Series', content)
        # Also handle include guards and includes
        new_content = new_content.replace('TENSOR_', 'SERIES_').replace('Tensor_', 'Series_')
        if new_content != content:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(new_content)

print('Renaming and replacement complete.')

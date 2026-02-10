#!/usr/bin/env python3
"""Safely rename Series -> Series across the repository.

Creates .bak backups for modified files and renames files/dirs containing 'Series'.
Use with caution; intended as an automated first pass. Review changes before committing.
"""
import os
import re
import shutil

ROOT = os.path.dirname(os.path.dirname(__file__))
EXCLUDE_DIRS = {"build", ".git", "dist", "node_modules"}

file_pattern = re.compile(r"\bTensorBody\b|\bTensor\b|\btensor\b")

def should_skip(path):
    parts = path.split(os.sep)
    return any(p in EXCLUDE_DIRS for p in parts)

def backup_file(path):
    bak = path + '.bak'
    if not os.path.exists(bak):
        shutil.copy2(path, bak)

def replace_in_file(path):
    try:
        with open(path, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception:
        return False
    if not file_pattern.search(content):
        return False
    new = content
    # Order matters: SeriesBody -> SeriesBody first
    new = re.sub(r"\bTensorBody\b", "SeriesBody", new)
    # Types and identifiers
    new = re.sub(r"\bTensor\b", "Series", new)
    # lowercase identifiers
    new = re.sub(r"\btensor\b", "series", new)

    if new != content:
        backup_file(path)
        with open(path, 'w', encoding='utf-8') as f:
            f.write(new)
        return True
    return False

def rename_paths(root):
    # Walk bottom-up to rename files/dirs safely
    for dirpath, dirnames, filenames in os.walk(root, topdown=False):
        if should_skip(dirpath):
            continue
        # Rename files containing 'Series' in name
        for fname in filenames:
            if 'Series' in fname:
                old = os.path.join(dirpath, fname)
                newfname = fname.replace('Series', 'Series')
                new = os.path.join(dirpath, newfname)
                if os.path.exists(new):
                    print(f"Skipping rename, target exists: {new}")
                else:
                    print(f"Renaming file: {old} -> {new}")
                    os.rename(old, new)
        # Rename directories containing 'Series'
        for dname in dirnames:
            if 'Series' in dname:
                oldd = os.path.join(dirpath, dname)
                newdname = dname.replace('Series', 'Series')
                newd = os.path.join(dirpath, newdname)
                if os.path.exists(newd):
                    print(f"Skipping dir rename, target exists: {newd}")
                else:
                    print(f"Renaming dir: {oldd} -> {newd}")
                    os.rename(oldd, newd)

def main():
    changed = 0
    for dirpath, dirnames, filenames in os.walk(ROOT):
        if should_skip(dirpath):
            continue
        for fname in filenames:
            path = os.path.join(dirpath, fname)
            if replace_in_file(path):
                print(f"Updated: {path}")
                changed += 1
    print(f"Files updated: {changed}")
    # Now rename paths
    rename_paths(ROOT)

if __name__ == '__main__':
    main()

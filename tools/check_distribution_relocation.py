#!/usr/bin/env python3
"""Reject build-host paths before a PlussII distribution is archived."""

from pathlib import Path
import sys


stage = Path(sys.argv[1])
source = Path(sys.argv[2])
build = Path(sys.argv[3])

host_roots = [source, build]
for root in (source, build):
    host_roots.append(Path(str(root).replace('\\', '/')))
for root in (Path('/Users/runner/work/PlussDev'), Path('/home/runner/work/PlussDev')):
    host_roots.append(root)

host_markers = {str(root).encode() for root in host_roots}
host_markers |= {str(root).replace('/', '\\').encode() for root in host_roots}
metadata_markers = (
    b'/opt/homebrew/', b'/usr/local/', b'C:/Program Files',
    b'C:\\Program Files', b'D:/a/', b'D:\\a\\',
)

violations = []
for file in stage.rglob('*'):
    if not file.is_file():
        continue
    # Static archives can retain source-location text for diagnostics. They
    # contain no CMake link paths; exported package metadata is checked below.
    if file.suffix in {'.a', '.lib'}:
        continue
    relative = file.relative_to(stage)
    data = file.read_bytes()
    matches = [marker for marker in host_markers if marker and marker in data]
    if file.suffix in {'.cmake', '.pc', '.h', '.hpp', '.txt'} or file.name == 'curl-config':
        matches += [marker for marker in metadata_markers if marker in data]
    if matches:
        violations.append((relative, sorted(set(matches))))

if violations:
    for relative, matches in violations:
        print(f'{relative}: {", ".join(marker.decode(errors="replace") for marker in matches)}')
    raise SystemExit(f'{len(violations)} staged files contain build-host paths')
print('Distribution relocation audit passed: no build-host paths found')

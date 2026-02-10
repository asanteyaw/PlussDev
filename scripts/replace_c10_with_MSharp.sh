#!/usr/bin/env bash
set -euo pipefail

# Find candidate source files and do in-place replacements, keep backups with .bak
find . -type f \( -name '*.h' -o -name '*.hpp' -o -name '*.cpp' -o -name '*.cc' -o -name '*.cxx' -o -name '*.inl' -o -name '*.ipp' \) \
  -not -path './build/*' -not -path './.git/*' -print0 | \
  xargs -0 perl -0777 -i.bak -pe \
  's/\bnamespace\s+c10\b/namespace MSharp/g; s/\bc10::/MSharp::/g; s/#include\s+<c10\//#include <MSharp\//g;'

echo "Replacements complete. Backups have .bak suffix." 

#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(pwd)"

echo "Renaming files containing 'Tensor' or 'tensor' in their names..."
find . -type f \( -name "*Tensor*" -o -name "*tensor*" \) -not -path './build/*' -not -path './.git/*' -not -path './Plussible/*' -print0 | \
  while IFS= read -r -d '' f; do
    dir=$(dirname "$f")
    base=$(basename "$f")
    newbase=$(echo "$base" | sed -E "s/Tensor/Series/g; s/tensor/series/g")
    newpath="$dir/$newbase"
    if [ "$f" != "$newpath" ]; then
      echo "mv: $f -> $newpath"
      git mv -f "$f" "$newpath" 2>/dev/null || mv -f "$f" "$newpath"
    fi
  done

echo "Replacing identifiers in source files (backups with .bak)..."
find . -type f \( -name '*.cpp' -o -name '*.c' -o -name '*.h' -o -name '*.hpp' -o -name '*.cc' -o -name '*.cxx' -o -name '*.in' -o -name '*.cmake' -o -name '*.py' -o -name '*.ipp' \) -not -path './build/*' -not -path './.git/*' -not -path './Plussible/*' -print0 | \
  xargs -0 sed -i.bak -E 's/\bTensorImpl\b/SeriesImpl/g; s/\bTensor\b/Series/g; s/\btensor\b/series/g'

echo "Running CMake configure + build"
cmake -S . -B build
cmake --build build -- -j8

echo "Done"

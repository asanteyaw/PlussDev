#!/usr/bin/env bash
set -euo pipefail

INTERNALS_DIR="include/internals/PlussQit/CodeM/core/MSharp"
TARGET_DIR="include/MSharp"

mkdir -p "$TARGET_DIR"

echo "Generating wrappers from $INTERNALS_DIR to $TARGET_DIR"
while IFS= read -r -d '' f; do
  rel=${f#${INTERNALS_DIR}/}
  target="$TARGET_DIR/${rel}"
  targetdir=$(dirname "$target")
  if [ ! -f "$target" ]; then
    mkdir -p "$targetdir"
    echo "Creating wrapper: $target -> $f"
    cat > "$target" <<EOF
#pragma once

// Forwarding header to internal implementation
#include <${f}>

EOF
  fi
done < <(find "$INTERNALS_DIR" -type f -name '*.h' -print0)

INTERNALS_UTIL="$INTERNALS_DIR/util"
while IFS= read -r -d '' f; do
  rel=${f#${INTERNALS_UTIL}/}
  target="$TARGET_DIR/util/${rel}"
  targetdir=$(dirname "$target")
  if [ ! -f "$target" ]; then
    mkdir -p "$targetdir"
    echo "Creating wrapper util: $target -> $f"
    cat > "$target" <<EOF
#pragma once

// Forwarding header to internal implementation
#include <${f}>

EOF
  fi
done < <(find "$INTERNALS_UTIL" -type f -name '*.h' -print0)

echo "Done generating wrappers."

#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
if [ "$#" -eq 0 ]; then
  set -- html --site all
fi
exec python3 "$REPO_ROOT/docs/scripts/build_unified_docs.py" "$@"

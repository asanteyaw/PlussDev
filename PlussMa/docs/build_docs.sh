#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT_DIR"
PACKAGE_ROOT="$(cd "$ROOT_DIR/.." && pwd)"
REPO_ROOT="$(cd "$ROOT_DIR/../.." && pwd)"

if ! command -v sphinx-build >/dev/null 2>&1; then
  echo "sphinx-build not found. Install deps first:"
  echo "  python -m pip install -r docs/sphinx/requirements.txt"
  exit 1
fi

echo "[0/2] Aggregating local docs/ folders nested under PlussMa/ into _local/ ..."
python3 "$REPO_ROOT/docs/scripts/aggregate_docs.py" --into "$ROOT_DIR" --scan "$PACKAGE_ROOT"

echo "[1/2] Building HTML..."
sphinx-build -b html . _build/html

echo "[2/2] Done."
echo "HTML: PlussMa/docs/_build/html/index.html"

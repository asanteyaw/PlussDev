#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT_DIR"
REPO_ROOT="$(cd "$ROOT_DIR/../.." && pwd)"

if ! command -v sphinx-build >/dev/null 2>&1; then
  echo "sphinx-build not found. Install deps first:"
  echo "  python -m pip install -r docs/sphinx/requirements.txt"
  exit 1
fi

echo "[0/4] Refreshing each package's own local docs aggregation first..."
python3 "$REPO_ROOT/docs/scripts/aggregate_docs.py" --into "$REPO_ROOT/PlussQit/docs" --scan "$REPO_ROOT/PlussQit"
python3 "$REPO_ROOT/docs/scripts/aggregate_docs.py" --into "$REPO_ROOT/PlussMic/docs/sphinx" --scan "$REPO_ROOT/PlussMic"
python3 "$REPO_ROOT/docs/scripts/aggregate_docs.py" --into "$REPO_ROOT/PlussMa/docs" --scan "$REPO_ROOT/PlussMa"
python3 "$REPO_ROOT/docs/scripts/aggregate_docs.py" --into "$REPO_ROOT/PlussPl/docs/sphinx" --scan "$REPO_ROOT/PlussPl"
python3 "$REPO_ROOT/docs/scripts/aggregate_docs.py" --into "$REPO_ROOT/PlussAI/docs/sphinx" --scan "$REPO_ROOT/PlussAI"
python3 "$REPO_ROOT/docs/scripts/aggregate_docs.py" --into "$REPO_ROOT/PlussTils/view/docs" --scan "$REPO_ROOT/PlussTils"

echo "Aggregating each package's docs hub into this site's _local/ ..."
python3 "$REPO_ROOT/docs/scripts/aggregate_docs.py" \
  --into "$ROOT_DIR" \
  --package-hub "PlussQit=$REPO_ROOT/PlussQit/docs" \
  --package-hub "PlussMic=$REPO_ROOT/PlussMic/docs/sphinx" \
  --package-hub "PlussMa=$REPO_ROOT/PlussMa/docs" \
  --package-hub "PlussPl=$REPO_ROOT/PlussPl/docs/sphinx" \
  --package-hub "PlussAI=$REPO_ROOT/PlussAI/docs/sphinx" \
  --package-hub "PlussTils=$REPO_ROOT/PlussTils/view/docs"

echo "[1/4] Building HTML..."
sphinx-build -b html . _build/html

echo "[2/4] Building LaTeX..."
sphinx-build -b latex . _build/latex

if command -v latexmk >/dev/null 2>&1; then
  echo "[3/4] Building PDF with latexmk..."
  (cd _build/latex && latexmk -pdf -quiet *.tex)
else
  echo "latexmk not found. Skipping PDF compile."
  echo "Install TeX tooling and rerun for PDF output."
fi

echo "[4/4] Done."
echo "HTML: docs/sphinx/_build/html/index.html"
if command -v latexmk >/dev/null 2>&1; then
  echo "PDF:  docs/sphinx/_build/latex/"
fi

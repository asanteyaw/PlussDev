#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT_DIR"

if ! command -v sphinx-build >/dev/null 2>&1; then
  echo "sphinx-build not found. Install deps first:"
  echo "  python -m pip install -r PlussPl/docs/sphinx/requirements.txt"
  exit 1
fi

echo "[1/3] Building HTML..."
sphinx-build -b html . _build/html

echo "[2/3] Building LaTeX..."
sphinx-build -b latex . _build/latex

if command -v latexmk >/dev/null 2>&1; then
  echo "[3/3] Building PDF with latexmk..."
  (cd _build/latex && latexmk -pdf -quiet *.tex)
else
  echo "latexmk not found. Skipping PDF compile."
  echo "Install TeX tooling and rerun for PDF output."
fi

echo "Done."
echo "HTML: PlussPl/docs/sphinx/_build/html/index.html"
if command -v latexmk >/dev/null 2>&1; then
  echo "PDF:  PlussPl/docs/sphinx/_build/latex/"
fi

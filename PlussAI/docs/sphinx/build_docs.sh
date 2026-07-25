#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
SPHINX="/Users/yawasante/software/miniconda3/envs/cpp_env/bin/sphinx-build"

python3 "$ROOT_DIR/PlussAI/docs/sphinx/generate_nodej_lymph_api_docs.py"
python3 "$ROOT_DIR/docs/scripts/aggregate_docs.py" --into "$ROOT_DIR/PlussAI/docs/sphinx" --scan "$ROOT_DIR/PlussAI"
"$SPHINX" -b html "$ROOT_DIR/PlussAI/docs/sphinx" "$ROOT_DIR/PlussAI/docs/sphinx/_build/html"
"$SPHINX" -M latexpdf "$ROOT_DIR/PlussAI/docs/sphinx" "$ROOT_DIR/PlussAI/docs/sphinx/_build"

# Build Outputs

The supported docs build path is the unified repo-level driver. It refreshes
package generators, package `_local/` mirrors, package HTML sites, and the root
Pluss site in one pass.

## Prerequisites

Install in your active Python/conda environment:

```bash
python -m pip install -r docs/sphinx/requirements.txt
```

For PDF, ensure TeX tools are available (`pdflatex`, `latexmk`).

## Generate HTML

```bash
python3 docs/scripts/build_unified_docs.py html
```

Equivalent wrapper:

```bash
make -C docs/sphinx html
```

HTML outputs are written to each package's own `docs/.../_build/html/` plus
`docs/sphinx/_build/html/`.

## Generate PDF (via LaTeX)

```bash
python3 docs/scripts/build_unified_docs.py pdf --site root
```

## One-shot PDF builder

```bash
python3 docs/scripts/build_unified_docs.py latexpdf --site root
```

## Live preview

```bash
sphinx-autobuild docs/sphinx docs/sphinx/_build/html
```

## Combined build helper

```bash
./docs/sphinx/build_docs.sh
```

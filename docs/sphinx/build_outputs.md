# Build Outputs

## Prerequisites

Install in your active Python/conda environment:

```bash
python -m pip install -r docs/sphinx/requirements.txt
```

For PDF, ensure TeX tools are available (`pdflatex`, `latexmk`).

## Generate HTML

```bash
sphinx-build -b html docs/sphinx docs/sphinx/_build/html
```

## Generate PDF (via LaTeX)

```bash
sphinx-build -b latex docs/sphinx docs/sphinx/_build/latex
cd docs/sphinx/_build/latex
latexmk -pdf -quiet *.tex
```

## One-shot PDF builder

```bash
sphinx-build -b latexpdf docs/sphinx docs/sphinx/_build/latex
```

## Live preview

```bash
sphinx-autobuild docs/sphinx docs/sphinx/_build/html
```

## Combined build helper

```bash
./docs/sphinx/build_docs.sh
```

# Build Outputs

## Prerequisites

Install in your active Python/conda environment:

```bash
python -m pip install -r PlussPl/docs/sphinx/requirements.txt
```

For PDF output, ensure TeX tools are available (`pdflatex`, `latexmk`).

## Generate HTML

```bash
sphinx-build -b html PlussPl/docs/sphinx PlussPl/docs/sphinx/_build/html
```

## Regenerate Flash Method Outputs (Real Examples)

This runs the single capture demo (`flash_api_capture_demo`) once per method and writes SVG (and PNG fallbacks).

```bash
./PlussPl/docs/sphinx/generate_flash_method_outputs.sh
```

## Generate PDF (via LaTeX)

```bash
sphinx-build -b latex PlussPl/docs/sphinx PlussPl/docs/sphinx/_build/latex
cd PlussPl/docs/sphinx/_build/latex
latexmk -pdf -quiet *.tex
```

## One-shot PDF builder

```bash
sphinx-build -b latexpdf PlussPl/docs/sphinx PlussPl/docs/sphinx/_build/latex
```

## Live preview

```bash
sphinx-autobuild PlussPl/docs/sphinx PlussPl/docs/sphinx/_build/html
```

## Combined helper

```bash
./PlussPl/docs/sphinx/build_docs.sh
```

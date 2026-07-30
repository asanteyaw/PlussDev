# Unified Sphinx Docs for Pluss

Every docs entrypoint in this repo now delegates to one unified build graph.
If you edit a package page, a hand-authored API method page, or a nested local
`docs/` page, one build refreshes package-local aggregations, package HTML
sites, and the root Pluss site together.

## Install dependencies

```bash
python -m pip install -r docs/sphinx/requirements.txt
```

## Build HTML

```bash
make -C docs/sphinx html
```

Equivalent direct command:

```bash
python3 docs/scripts/build_unified_docs.py html
```

Output:

- package HTML sites under each package's own `docs/.../_build/html/`
- `docs/sphinx/_build/html/index.html`

## Build PDF

```bash
make -C docs/sphinx pdf
```

or:

```bash
make -C docs/sphinx latexpdf
```

Output is generated under:

- each package's own `docs/.../_build/latex/`
- `docs/sphinx/_build/latex/`

## Convenience wrapper

```bash
./docs/sphinx/build_docs.sh
```

## Notes

- Running `make html` from any supported docs folder now triggers the same
  unified build graph.
- `latexpdf` requires Sphinx with latexpdf builder and TeX tooling.
- `pdf` target uses `latexmk` after generating LaTeX.

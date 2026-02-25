# Sphinx Docs for PlussQit

This folder contains user-facing documentation for PlussQit.

## Install dependencies

```bash
python -m pip install -r docs/sphinx/requirements.txt
```

## Build HTML

```bash
make -C docs/sphinx html
```

Output:

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

- `docs/sphinx/_build/latex/`

## Build both (HTML + PDF if latexmk exists)

```bash
./docs/sphinx/build_docs.sh
```

## Notes

- `latexpdf` requires Sphinx with latexpdf builder and TeX tooling.
- `pdf` target uses `latexmk` after generating LaTeX.

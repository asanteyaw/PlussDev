# Sphinx Docs for PlussPl

This folder contains API-reference documentation for PlussPl core and Flash.

## Install dependencies

```bash
python -m pip install -r PlussPl/docs/sphinx/requirements.txt
```

## Build HTML

```bash
make -C PlussPl/docs/sphinx html
```

Output:

- `PlussPl/docs/sphinx/_build/html/index.html`

## Build PDF

```bash
make -C PlussPl/docs/sphinx pdf
```

or:

```bash
make -C PlussPl/docs/sphinx latexpdf
```

Output is generated under:

- `PlussPl/docs/sphinx/_build/latex/`

## Build both (HTML + PDF if latexmk exists)

```bash
./PlussPl/docs/sphinx/build_docs.sh
```

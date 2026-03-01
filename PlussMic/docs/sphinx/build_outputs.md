# Build Outputs

## HTML

```bash
cd docs/sphinx
make html
```

Expected output:

- `_build/html/index.html`

## PDF

```bash
cd docs/sphinx
make pdf
```

Expected output:

- `_build/latex/plussmic_user_docs.pdf`

## Notes

- HTML uses the same custom left-sidebar/top-bar shell as the PlussQit docs.
- PDF is built through `latex` plus `latexmk`.
- Run the commands from an environment where Sphinx and the PDF toolchain are already available.
- If HTML looks stale after a rebuild, hard refresh the browser once.

# Build Outputs

## HTML

```bash
cd /Users/yawasante/software/cxx/PlussDev/PlussMic/docs/sphinx
PATH=/Users/yawasante/software/miniconda3/envs/cpp_env/bin:$PATH make html
```

Expected output:

- `_build/html/index.html`

## PDF

```bash
cd /Users/yawasante/software/cxx/PlussDev/PlussMic/docs/sphinx
PATH=/Users/yawasante/software/miniconda3/envs/cpp_env/bin:$PATH make pdf
```

Expected output:

- `_build/latex/plussmic_user_docs.pdf`

## Notes

- HTML uses the same custom left-sidebar/top-bar shell as the PlussQit docs.
- PDF is built through `latex` plus `latexmk`.
- If HTML looks stale after a rebuild, hard refresh the browser once.

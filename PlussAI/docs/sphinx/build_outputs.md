# Build Outputs

- HTML root: `PlussAI/docs/sphinx/_build/html/index.html`
- PDF output: `PlussAI/docs/sphinx/_build/latex/plussai_api_docs.pdf`

Build commands:

```bash
sphinx-build -b html PlussAI/docs/sphinx PlussAI/docs/sphinx/_build/html
sphinx-build -M latexpdf PlussAI/docs/sphinx PlussAI/docs/sphinx/_build
```

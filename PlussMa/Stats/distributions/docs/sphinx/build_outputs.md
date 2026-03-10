# Build Outputs

## Install Doc Dependencies

```bash
python3 -m pip install -r requirements.txt
```

## Build HTML

```bash
make html
```

The HTML output lands in:

```text
_build/html
```

Expected entry files:

- `_build/html/index.html`
- `_build/html/quickstart.html`
- `_build/html/api_reference.html`
- `_build/html/object_api_reference.html`
- `_build/html/distribution_catalog.html`
- `_build/html/build_outputs.html`

## Build PDF

```bash
make pdf
```

This expects a working LaTeX toolchain with `latexmk`.

Generated files:

- `_build/latex/plussma_distribution_docs.tex`
- `_build/latex/plussma_distribution_docs.pdf`

The final PDF manual is:

```text
_build/latex/plussma_distribution_docs.pdf
```

## Current Outputs

The current checked build produced:

- HTML site root: `_build/html/index.html`
- PDF manual: `_build/latex/plussma_distribution_docs.pdf`

## Relevant Code Artifacts

The docs describe the production surface implemented in:

- `PlussMa/Stats/distributions/distr.cpp`
- `PlussMa/Stats/distributions/distr_objects_continuous.cpp`
- `PlussMa/Stats/distributions/distr_objects_discrete.cpp`
- `PlussMa/Stats/distributions/distr_objects_special.cpp`
- `PlussMa/include/pluss/subsp/stats/distr_fwd.h`
- `PlussMa/include/pluss/subsp/stats/distr.h`
- `PlussMa/include/pluss/ma/stats.h`
- `PlussMa/examples/Numerical/distributions_demo.cpp`
- `PlussMa/Stats/distributions/examples/distributions_objects_demo.cpp`

## Validation Path

The runtime coverage for the documented surface is in:

```text
PlussMa/Stats/distributions/test/distr_test.cpp
```

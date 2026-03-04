# PlussMa Distribution Documentation

The legacy Matlab `randraw.m` surface has been retired. Probability distributions now live in the C++ API as `pluss::distr`, available directly from:

```cpp
#include <pluss/pluss.h>
```

This manual is organized around how users actually consume the layer:

- start with `quickstart` for the short-path workflow
- use `api_reference` when you need the function surface
- use `object_api_reference` when you want parameter-bound distribution objects
- use `distribution_catalog` when you need names, aliases, and parameter conventions
- use `build_outputs` when you want to build the docs or the example binary locally

```{toctree}
:maxdepth: 2

quickstart
api_reference
object_api_reference
distribution_catalog
build_outputs
```

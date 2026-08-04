# Factories and public surface

PlussQit follows a strict policy:

- **If the Series already exists**, operations are **methods** (e.g. `x.add(y)`, `x.index_put_(...)`).
- **If the Series does not exist yet**, creation is done via **free factories** in the `pluss::` namespace
  exposed by the single include:

```cpp
#include <pluss/pluss.h>
```

## Where implementations live

- `pluss::*` functions in `pluss.h` are **thin inline wrappers** that call `pluss::codem::*` implementations.
- `pluss::codem::*` factory implementations live in `src/CodeM/core/SeriesBody.h`.

This keeps the public header stable and avoids accidental recursion/bloat where a public factory calls another public
factory (e.g. `pluss::series::nan()` must not call `pluss::series::full()`), ensuring a single source of truth for behavior.

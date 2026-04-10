# Optima Documentation

This Sphinx set is for optimization algorithms owned by `PlussMa`.

Module boundary:

- `PlussQit` owns `Series`, autodiff, and backend compute
- `PlussMa` owns numerical optimization logic
- `optima(...)` is the public constrained-optimization entry point

```{toctree}
:maxdepth: 2

optima_algorithms
optimizer_surfaces
optima_sequential_qp
optima_internal_protocol
```

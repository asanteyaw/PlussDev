# Patch 54: Method surface expansion

This patch adds a large unary-math family to `ops_internal` and exposes them as **Series methods** via the Patch 53 codegen.

New methods (CPU-only):
- `sin`, `cos`, `tan`, `asin`, `acos`, `atan`
- `sinh`, `cosh`, `tanh`
- `log1p`, `expm1`, `reciprocal`
- `erf`, `erfc`, `lgamma`, `asinh`, `acosh`, `atanh`
- `isfinite`, `logical_not`

Design:
- kernels live under `native::cpu::*`
- `ops_internal` is the canonical API for method exposure
- method wrappers are generated and skip any methods already implemented manually in `Series`

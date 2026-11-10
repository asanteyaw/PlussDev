<!-- generated from geometry.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Differential geometry

Calculus on curved spaces: the Levi-Civita connection of a metric, the covariant derivative of tensor fields, and the absolute (intrinsic) derivative along a curve. Indices follow one convention everywhere: contravariant axes first, then covariant axes.

## The Surface

| Symbol | What it is |
|---|---|
| [`TensorSignature`](geometry/TensorSignature.md) | How many contravariant (upper) and covariant (lower) indices a tensor field has. |
| [`absolute_differential`](geometry/absolute_differential.md) | The absolute (intrinsic) derivative of a tensor along a curve: how fast it changes relative to parallel transport. |
| [`covariant`](geometry/covariant.md) | The covariant derivative of a tensor field in the direction of one coordinate: the ordinary derivative plus the connection terms for each index. |
| [`levi_civita`](geometry/levi_civita.md) | The Levi-Civita connection (Christoffel symbols) of a metric, as a function of the coordinates. |

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

geometry/TensorSignature
geometry/absolute_differential
geometry/covariant
geometry/levi_civita
```
<!-- apiref:toctree:end -->

<!-- generated from measure.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Measures

Integration against a measure. There is no measure type: a measure is carried by the operator that integrates against it. Lebesgue measure, a density, a finite set of point masses and a Stieltjes mixture each have an operator, and measure algebra is operator algebra.

## The Surface

| Symbol | What it is |
|---|---|
| [`atomic`](measure/atomic.md) | Integration against a finite set of point masses: the exact sum `Σ wᵢ f(xᵢ)`. |
| [`lebesgue`](measure/lebesgue.md) | Integration against Lebesgue measure on an interval, with optional null points that are never sampled. |
| [`stieltjes`](measure/stieltjes.md) | Lebesgue–Stieltjes integration: a continuous part given by a distribution function plus a list of jumps. |
| [`weighted`](measure/weighted.md) | Integration against a density: `∫ f(x) ρ(x) dx`. |

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

measure/atomic
measure/lebesgue
measure/stieltjes
measure/weighted
```
<!-- apiref:toctree:end -->

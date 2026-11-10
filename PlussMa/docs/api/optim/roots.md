<!-- generated from roots.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Root finding

Batched scalar root finding. Every solver advances all lanes of a `Series` in the same `Series` operations, so one call solves a whole parameter sweep. A function maps a `Series` of any shape to a `Series` of the same shape, and each element is an independent lane.

## The Surface

| Symbol | What it is |
|---|---|
| [`Derivatives`](roots/Derivatives.md) | The value of a function and its first three derivatives at the same points: what the `householder` callback returns. |
| [`Options`](roots/Options.md) | Iteration limit, stopping tolerances and checking cadence shared by every root finder. |
| [`Result`](roots/Result.md) | What every root finder returns: the roots, the residuals, the iteration count and whether the brackets were valid. |
| [`bisect`](roots/bisect.md) | Finds a root of `f` in a bracket by repeatedly halving the interval, for every lane at once. |
| [`householder`](roots/householder.md) | Fourth-order Householder iteration, given the function and its first three derivatives; the root is differentiable with respect to anything the function captures. |
| [`illinois`](roots/illinois.md) | Finds a root in a bracket by regula falsi with the Illinois modification: bracketed like bisection, but superlinear. |
| [`newton`](roots/newton.md) | Newton–Raphson iteration from a starting value, with an analytic derivative or a finite-difference slope. |
| [`secant`](roots/secant.md) | The secant method: Newton's idea without a derivative, using the line through the last two iterates. |

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

roots/Derivatives
roots/Options
roots/Result
roots/bisect
roots/householder
roots/illinois
roots/newton
roots/secant
```
<!-- apiref:toctree:end -->

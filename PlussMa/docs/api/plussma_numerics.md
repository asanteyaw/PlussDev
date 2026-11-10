<!-- generated from plussma_numerics.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Numerical Methods API Reference

The numerical methods of PlussMa live in `pluss::subsp::math`. They take their data and functions as `Series` values, so a whole batch of integrals, trajectories or systems is solved in one call, and anything built from differentiable `Series` operations can be differentiated by the same tape that records the rest of your program.

```cpp
#include <pluss/ma.h>

namespace integ = pluss::subsp::math::integ;
```

Every page of this reference documents one function, enumeration or options structure: what it is, when to call it, every parameter and field, what it returns and what it refuses, and programs with real output that cover each overload and each option.

Not sure which method fits? The {guide}`numerical methods guide <numerics>` opens with a chooser.

## The families

| Family | What it covers |
|---|---|
| [Numerical integration](numerics/integ.md) | Trapezoid, quadrature, Monte Carlo, automatic choice, Lebesgue integration, closed forms |
| [Differential equations](numerics/ode.md) | Fixed and adaptive Runge-Kutta, stiff linear systems, event detection, adjoint gradients |
| [Krylov solvers](numerics/krylov.md) | Conjugate gradients, GMRES, the action of a matrix exponential |
| [Interpolation](numerics/interp.md) | Piecewise-linear interpolation of batches of curves |
| [Multigrid](numerics/multigrid.md) | Geometric multigrid operator and V-cycle preconditioner on structured grids |

## Conventions

- Examples print with `pluss::tils::OutputEngine`; a `Series` is read with `.items<double>()` for a vector and `.item<double>()` for a single number.
- A callable integrand takes a `Series` of nodes or samples and returns the values in the same shape.
- A result structure reports why a run stopped; check `converged` and the error estimate before using the value.
- Errors are `pluss::Error`; the message names the operation and the problem.

```{toctree}
:maxdepth: 3
:hidden:

numerics/integ
numerics/ode
numerics/krylov
numerics/interp
numerics/multigrid
```

<!-- generated from variational.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Calculus of variations

The functional derivative and the Euler–Lagrange equation for a Lagrangian of one dependent variable and its derivatives. The equations are exact functions of the independent variable and the jet variables, ready to evaluate along candidate solutions.

## The Surface

| Symbol | What it is |
|---|---|
| [`euler_lagrange`](variational/euler_lagrange.md) | The Euler–Lagrange expression of a Lagrangian: the function whose zeros are the stationary functions. |
| [`functional_derivative`](variational/functional_derivative.md) | The functional derivative `δF/δu` of the functional `F[u] = ∫ L(x, u, u′, …) dx`. |

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

variational/euler_lagrange
variational/functional_derivative
```
<!-- apiref:toctree:end -->

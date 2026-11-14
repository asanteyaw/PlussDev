# Numerical methods

Numerical integration, differential equations, interpolation and large linear systems.

## Choosing a method

**An integral.** The [integration](numerics/integration.md) chapters cover sampled data (`trapezoid`, `cumulative_trapezoid`), functions of one variable (adaptive Simpson or Gauss-Legendre `quadra`, finite or infinite intervals, whole batches of integrals in one call), functions of a few variables (tensor-product rules), functions of many variables (Monte Carlo `aleatoria`), an automatic choice between quadrature and Monte Carlo (`integra`), range-partitioned Lebesgue integration, and closed forms for the common integrand families.

**A differential equation.** The [ode](numerics/ode.md) chapters solve initial-value problems with adaptive stepping (`fluxa`), linear problems (`fluxa_linear`), event detection (`fluxa_until`) and gradients by the adjoint method (`fluxa_adjoint`).

**Values between samples.** The [interpolation](numerics/interpolation.md) chapters read values between tabulated samples by linear interpolation.

**A large linear system, or the action of a matrix exponential.** The [krylov](numerics/krylov.md) chapters solve symmetric systems by conjugate gradients (`cg`), general systems by restarted GMRES (`gmres`), and apply an exponential of a matrix to a vector without forming the matrix.

## The chapters

- [integration](numerics/integration.md)
- [ode](numerics/ode.md)
- [interpolation](numerics/interpolation.md)
- [krylov](numerics/krylov.md)

```{toctree}
:maxdepth: 3
:hidden:

numerics/integration
numerics/ode
numerics/interpolation
numerics/krylov
```

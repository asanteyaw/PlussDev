<!-- generated from ode.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Ordinary differential equations

Initial-value problems `dy/dt = f(t, y, θ)` over a whole state `Series`: fixed-step and adaptive Runge-Kutta solvers, a stiff solver for linear systems, event location, and gradients of a result with respect to the initial state and parameters by the adjoint method. The state `y` is a one-dimensional `Series` (flatten a structured state into one), `t` is a plain number, and `θ` is an optional `Series` of parameters.

| You want | Use |
|---|---|
| The state at the end time | [`fluxa`](ode/fluxa.md) (fixed-step RK4 or adaptive RK45) |
| A linear system `y' = A(t) y`, possibly stiff | [`fluxa_linear`](ode/fluxa_linear.md) |
| The time a quantity crosses zero (an impact, a threshold) | [`fluxa_until`](ode/fluxa_until.md) |
| Gradients of a result with respect to `y₀` and `θ` | [`fluxa_adjoint`](ode/fluxa_adjoint.md) |

## The Surface

| Symbol | What it is |
|---|---|
| [`LinearOdeOptions`](ode/LinearOdeOptions.md) | Tolerances, step bounds and inner-solver settings of `fluxa_linear`. |
| [`OdeAdjointResult`](ode/OdeAdjointResult.md) | The final state and the gradients returned by the adjoint solve. |
| [`OdeEventDirection`](ode/OdeEventDirection.md) | Which sign changes of the event function count. |
| [`OdeEventOptions`](ode/OdeEventOptions.md) | Direction and refinement tolerances of event location. |
| [`OdeEventResult`](ode/OdeEventResult.md) | The outcome of an event-terminated solve: everything in `OdeResult`, plus whether and when the event happened. |
| [`OdeMethod`](ode/OdeMethod.md) | The step rule of `fluxa`. |
| [`OdeOptions`](ode/OdeOptions.md) | The method, step count and adaptive-step controls of `fluxa`. |
| [`OdeResult`](ode/OdeResult.md) | The state at the end time and the work the solver did. |
| [`fluxa`](ode/fluxa.md) | Solves an initial-value problem `dy/dt = f(t, y, θ)` from `t0` to `t1` and returns the state at `t1`. |
| [`fluxa_adjoint`](ode/fluxa_adjoint.md) | Solves an initial-value problem and returns the gradients of a loss with respect to the initial state and the parameters, by the adjoint method. |
| [`fluxa_linear`](ode/fluxa_linear.md) | Solves a linear system `y' = A(t) y`, stiff or not, by an adaptive implicit method that never forms the matrix. |
| [`fluxa_until`](ode/fluxa_until.md) | Solves an initial-value problem until an event function changes sign, and returns the state and time of the event. |

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

ode/LinearOdeOptions
ode/OdeAdjointResult
ode/OdeEventDirection
ode/OdeEventOptions
ode/OdeEventResult
ode/OdeMethod
ode/OdeOptions
ode/OdeResult
ode/fluxa
ode/fluxa_adjoint
ode/fluxa_linear
ode/fluxa_until
```
<!-- apiref:toctree:end -->

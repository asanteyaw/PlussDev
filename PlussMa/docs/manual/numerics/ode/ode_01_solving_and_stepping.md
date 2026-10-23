<!-- generated from ode_01_solving_and_stepping.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# ODE 1: Solving and Stepping

An ordinary differential equation says how a state changes: `dy/dt = f(t, y)`. An initial-value problem gives the state at a start time and asks for the state at a later one. After this chapter you can solve one, check the order of the method by halving the step, let the solver choose its own steps, and read what each solver reports.

Every program is complete and its output is the output of that program. Step and evaluation counts are shown because they are the same on every machine; no running times are claimed.

## The call

```cpp
ode::OdeResult ode::fluxa(const ode::OdeFn& f, const Series& y0, double t0, double t1,
                          const Series& theta = undefined, const ode::OdeOptions& options = {});
```

- `f(t, y, theta)` returns `dy/dt` as a Series with the shape of `y`. Time `t` is a plain `double`; the state and the parameters are Series. The state is **one-dimensional**: a system of several variables is a vector, and structured states are flattened by the caller.
- `theta` carries parameters you will want gradients against (Chapter 2). Pass nothing when the function needs none.
- Integrating backward is allowed (`t1 < t0`).

| `OdeOptions` | Meaning | Default |
|---|---|---|
| `method` | `RK4` (fixed step) or `RK45` (adaptive Dormand–Prince 5(4)) | `RK4` |
| `steps` | number of fixed steps (`RK4`) | 100 |
| `relative_tolerance`, `absolute_tolerance` | adaptive error control: a step is accepted when its weighted RMS error is below `absolute + relative · |y|` | 1e-6, 1e-9 |
| `initial_step`, `minimum_step`, `maximum_step` | adaptive step size limits (0 and infinity mean unrestricted) | 0, 0, ∞ |
| `max_steps` | adaptive cap on accepted plus rejected steps | 100000 |

| `OdeResult` | Meaning |
|---|---|
| `y_final` | the state at `t1` |
| `steps`, `rejected_steps` | accepted and rejected steps |
| `function_evaluations` | calls of `f` |
| `converged` | whether the solve completed |

## Programs

- [A first solve, and the order of the method](ode_01_solving_and_stepping/a_first_solve_and_the_order_of_the_method.md)
- [Letting the solver choose the steps](ode_01_solving_and_stepping/letting_the_solver_choose_the_steps.md)

## Checklist

1. State as one vector; `f(t, y, theta)` with `t` a double.
2. RK4 with a step count for a fixed grid, RK45 with tolerances for everything else.
3. Confirm the order by halving the step.
4. Ask for a tolerance tighter than the final accuracy you need.
5. Read `rejected_steps` and `function_evaluations` when judging cost.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

ode_01_solving_and_stepping/a_first_solve_and_the_order_of_the_method
ode_01_solving_and_stepping/letting_the_solver_choose_the_steps
```
<!-- apiref:toctree:end -->

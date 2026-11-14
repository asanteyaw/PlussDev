<!-- generated from fluxa_linear.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `ode::fluxa_linear`

Solves a linear system `y' = A(t) y`, stiff or not, by an adaptive implicit method that never forms the matrix.

## Signature

```cpp
ode::OdeResult ode::fluxa_linear(const ode::LinearOdeFn &action, const pluss::Series &y0, double t0, double t1, const ode::LinearOdeOptions &options = {});
```

## What It Does

For a linear system the right-hand side is a matrix-vector product, and `action(t, y)` returns `A(t) y` without you building `A`. The solver is the **implicit midpoint rule with step doubling**: each step solves a linear system with restarted GMRES ([`gmres`](../krylov/gmres.md)) and compares one full step with two half steps to control the error, so it is stable for **stiff** systems where an explicit method such as RK45 would need absurdly small steps. `solve_dims` names the trailing state axes (default the last); any leading axes are independent lanes that are solved together, and `linear` holds the GMRES settings.

The tolerance and step controls are those of [`OdeOptions`](OdeOptions.md) (see [`LinearOdeOptions`](LinearOdeOptions.md)). The result is an [`OdeResult`](OdeResult.md). It does not support parameter gradients; for those use [`fluxa_adjoint`](fluxa_adjoint.md).

**Why implicit.** For `y' = −λ y` with a large `λ`, an explicit method is stable only for steps below about `1/λ`, however smooth the solution has become; the implicit midpoint rule `y_{n+1} = y_n + h·A((y_n + y_{n+1})/2)` is stable for any step on a decaying linear problem. The price is a linear solve per step, `(I − (h/2)A) y_{n+1} = (I + (h/2)A) y_n`, which is done matrix-free by GMRES using only `action`. The step-doubling comparison (one step of size `h` against two of size `h/2`) gives an error estimate for a second-order method, from which the step size is adapted.

**Accuracy expectations.** The midpoint rule is second order, so for a given tolerance it takes many more steps than `RK45` does on a non-stiff problem; use `fluxa` there and keep `fluxa_linear` for stiffness or for operators that are only available as actions.

## Parameters

`const ode::LinearOdeFn &action`
: `Series (double t, const Series &y)`: the product `A(t) y`.

`const pluss::Series &y0`
: The initial state; leading axes are independent lanes.

`double t0, double t1`
: Start and end times.

`const ode::LinearOdeOptions &options`
: Tolerances, step bounds and the GMRES settings.

## Returns

An [`OdeResult`](OdeResult.md).

## Errors

- A missing action or an undefined `y0` throws.
- A step that would fall below `minimum_step`, or an exhausted `max_steps`, throws.

## Practical Examples

### Example 1

A linear system with a known answer. `y' = −y` for two components from `[1, 2]`: the solution is `[e^{−1}, 2e^{−1}] = [0.36788, 0.73576]`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const ode::LinearOdeFn action = [](double, const Series &y) { return y * -1.0; };
  const ode::OdeResult r = ode::fluxa_linear(action, pluss::make_series({1.0, 2.0}), 0.0, 1.0);
  out.writeln("y(1) =", r.y_final.items<double>(), ", exact =", std::exp(-1.0), 2.0 * std::exp(-1.0));
  out.writeln("steps =", r.steps, ", converged =", r.converged);
}
```

Output

```text
y(1) = [0.367871, 0.735742] , exact = 0.367879 0.735759
steps = 31 , converged = true
```

### Example 2

A stiff system. `y' = −1000 y` decays on a time scale of a thousandth, and the solver reaches `t = 1` with the state decayed to zero; its implicit steps stay stable at a size an explicit method could not use.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const ode::LinearOdeFn action = [](double, const Series &y) { return y * -1000.0; };
  const ode::OdeResult r = ode::fluxa_linear(action, pluss::make_series({1.0}), 0.0, 1.0);
  out.writeln("y(1) =", r.y_final.items<double>(), ", steps =", r.steps);
}
```

Output

```text
y(1) = [-0.0000000000] , steps = 294
```

### Example 3

A matrix that changes in time. `A(t) = −t` gives `y(1) = e^{−1/2} = 0.60653066`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const ode::LinearOdeFn action = [](double t, const Series &y) { return y * -t; };
  const ode::OdeResult r = ode::fluxa_linear(action, pluss::make_series({1.0}), 0.0, 1.0);
  out.writeln("y(1) =", r.y_final.items<double>(), ", exact =", std::exp(-0.5));
}
```

Output

```text
y(1) = [0.606524] , exact = 0.606531
```

### Example 4

A real matrix applied through the action. `A = [[0, 1], [−1, 0]]` rotates the state: `[1, 0]` goes to `[cos 1, −sin 1]` at `t = 1`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const Series A = pluss::make_series({0.0, 1.0, -1.0, 0.0}, {2, 2});
  const ode::LinearOdeFn action = [&](double, const Series &y) { return A.matmul(y.unsqueeze(-1)).squeeze(-1); };
  ode::LinearOdeOptions options;
  options.relative_tolerance = 1e-9;
  const ode::OdeResult r = ode::fluxa_linear(action, pluss::make_series({1.0, 0.0}), 0.0, 1.0, options);
  out.writeln("y(1) =", r.y_final.items<double>(), ", exact =", std::cos(1.0), -std::sin(1.0));
}
```

Output

```text
y(1) = [0.540303, -0.841471] , exact = 0.540302 -0.841471
```

## See Also

- [`LinearOdeOptions`](LinearOdeOptions.md): the settings
- [`fluxa`](fluxa.md): the general solver

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fluxa_adjoint.html">
    <span class="label">Previous</span>
    <span class="title">fluxa_adjoint</span>
  </a>
  <a class="next-link" href="fluxa_until.html">
    <span class="label">Next</span>
    <span class="title">fluxa_until</span>
  </a>
</div>
<!-- apiref:nav:end -->

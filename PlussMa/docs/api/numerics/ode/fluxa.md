<!-- generated from fluxa.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `ode::fluxa`

Solves an initial-value problem `dy/dt = f(t, y, θ)` from `t0` to `t1` and returns the state at `t1`.

## Signature

```cpp
ode::OdeResult ode::fluxa(const ode::OdeFn &f, const pluss::Series &y0, double t0, double t1, const pluss::Series &theta = pluss::series::undefined(), const ode::OdeOptions &options = {});
```

## What It Does

`f(t, y, θ)` returns the derivative as a `Series` with the shape of `y`; `t` is a plain `double`, and `θ` is the parameter `Series` passed as `theta` (undefined when the right-hand side has none, in which case the closure can capture whatever it needs). The solution is advanced from `y0` at `t0` to `t1`; `t1 < t0` integrates backward.

Two methods are available ([`OdeMethod`](OdeMethod.md)). **RK4** takes `options.steps` equal steps of the classical fourth-order Runge-Kutta rule: predictable cost, error of order `h⁴`. **RK45** is the adaptive Dormand-Prince 5(4) pair: it estimates the local error of each step as a weighted RMS against `absolute_tolerance + relative_tolerance·|y|`, accepts or rejects the step, adapts the step size within `minimum_step` and `maximum_step`, and lands exactly on `t1`. The result reports the steps taken, the right-hand-side evaluations, the steps rejected and whether the run completed (an adaptive run that cannot meet the tolerance within `max_steps` or `minimum_step` throws rather than returning a wrong answer).

The solve is built from ordinary `Series` operations, so a parameter or initial state marked for differentiation carries its gradient through the steps. For long trajectories that is memory-hungry, since every step stays on the tape; use [`fluxa_adjoint`](fluxa_adjoint.md) for gradients of a long fixed-step solve.

**The methods.** `RK4` is the classical fourth-order Runge-Kutta rule: each step evaluates the right-hand side at four points and combines them, giving a local error of order `h⁵` and a global error of order `h⁴`; its cost (four evaluations per step) and accuracy are both predictable. `RK45` is the Dormand-Prince 5(4) pair: it computes a fifth-order and an embedded fourth-order estimate from the same seven evaluations, the difference between them estimates the local error, and the step is scaled by `(tolerance/error)^{1/5}` with a safety factor, so the solver takes large steps where the solution is smooth and small ones where it changes quickly.

**Choosing between them.** Take `RK45` when you want an answer to a stated accuracy without knowing the step in advance; it is the default choice for non-stiff problems. Take `RK4` when you need a fixed cost, a fixed grid of steps, or the event and adjoint solvers, which are defined for it. For a **stiff** problem (a fast decaying mode forces tiny steps on every explicit method) use [`fluxa_linear`](fluxa_linear.md) when the system is linear; an explicit solver on a stiff problem shows as a huge `steps` count or an exhausted `max_steps`.

## Parameters

`const ode::OdeFn &f`
: `(double t, const Series &y, const Series &theta) -> Series`: the derivative, shaped like `y`.

`const pluss::Series &y0`
: The initial state, a one-dimensional `Series`.

`double t0, double t1`
: The start and end times; `t1 < t0` integrates backward.

`const pluss::Series &theta`
: The parameters passed to `f`; undefined for none.

`const ode::OdeOptions &options`
: Method, steps, tolerances.

## Returns

An [`OdeResult`](OdeResult.md).

## Errors

- A missing callback, an undefined `y0`, or `steps ≤ 0` throws.
- An adaptive run that cannot meet its tolerance within `max_steps` or `minimum_step` throws.

## Practical Examples

### Example 1

Exponential decay. `y' = −y` with `y(0) = 1` has the solution `e^{−t}`; a hundred RK4 steps reach `e^{−1} = 0.36787944`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const ode::OdeFn decay = [](double, const Series &y, const Series &) { return y * -1.0; };   // y' = -y, exact solution e^{-t}

  const ode::OdeResult r = ode::fluxa(decay, pluss::make_series({1.0}), 0.0, 1.0);
  out.writeln("y(1) =", r.y_final.items<double>(), ", exact =", std::exp(-1.0));
  out.writeln("steps =", r.steps, ", evaluations =", r.function_evaluations, ", converged =", r.converged);
}
```

Output

```text
y(1) = [0.36787944] , exact = 0.36787944
steps = 100 , evaluations = 400 , converged = true
```

### Example 2

A system. The harmonic oscillator `x'' = −x` as `[x, v]`, started at `[1, 0]`, returns to its start after one period, `2π`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const ode::OdeFn oscillator = [](double, const Series &y, const Series &) {   // x'' = -x as a first-order system
    return y.select(0, 1).reshape({1}).unite({(y.select(0, 0) * -1.0).reshape({1})}, 0);
  };

  const ode::OdeResult r = ode::fluxa(oscillator, pluss::make_series({1.0, 0.0}), 0.0, 2.0 * 3.14159265358979323846);
  out.writeln("state after one period =", r.y_final.items<double>());
}
```

Output

```text
state after one period = [0.99999996, 0.00000081]
```

### Example 3

Adaptive stepping. With `RK45` the solver chooses its own steps to meet the tolerance; here it needs 12 steps and 84 evaluations where 100 RK4 steps use 400.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const ode::OdeFn decay = [](double, const Series &y, const Series &) { return y * -1.0; };   // y' = -y, exact solution e^{-t}

  ode::OdeOptions options;
  options.method = ode::OdeMethod::RK45;
  options.relative_tolerance = 1e-8;
  options.absolute_tolerance = 1e-10;
  const ode::OdeResult r = ode::fluxa(decay, pluss::make_series({1.0}), 0.0, 1.0, pluss::series::undefined(), options);
  out.writeln("y(1) =", r.y_final.items<double>(), ", exact =", std::exp(-1.0));
  out.writeln("steps =", r.steps, ", evaluations =", r.function_evaluations, ", rejected =", r.rejected_steps);
}
```

Output

```text
y(1) = [0.3678794420] , exact = 0.3678794412
steps = 12 , evaluations = 84 , rejected = 0
```

### Example 4

A parameter. The right-hand side reads `θ`: `y' = −θ y`, so with `θ = 2` the solution at `t = 1` is `e^{−2}`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const ode::OdeFn decay = [](double, const Series &y, const Series &theta) { return y * theta * -1.0; };
  const ode::OdeResult r = ode::fluxa(decay, pluss::make_series({1.0}), 0.0, 1.0, pluss::make_series({2.0}));
  out.writeln("y(1) =", r.y_final.items<double>(), ", exact =", std::exp(-2.0));
}
```

Output

```text
y(1) = [0.13533528] , exact = 0.13533528
```

### Example 5

Time dependence and backward integration. `y' = t y` has `y(t) = e^{t²/2}`; integrating from `t = 1` back to 0 from `y(1) = e^{1/2}` recovers `y(0) = 1`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const ode::OdeFn growth = [](double t, const Series &y, const Series &) { return y * t; };
  const ode::OdeResult r = ode::fluxa(growth, pluss::make_series({std::exp(0.5)}), 1.0, 0.0);
  out.writeln("y(0) =", r.y_final.items<double>(), ", exact = 1");
}
```

Output

```text
y(0) = [1.00000000] , exact = 1
```

## See Also

- [`OdeOptions`](OdeOptions.md): the settings
- [`fluxa_adjoint`](fluxa_adjoint.md): gradients
- [`fluxa_until`](fluxa_until.md): stopping at an event
- [`fluxa_linear`](fluxa_linear.md): stiff linear systems

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="OdeResult.html">
    <span class="label">Previous</span>
    <span class="title">OdeResult</span>
  </a>
  <a class="next-link" href="fluxa_adjoint.html">
    <span class="label">Next</span>
    <span class="title">fluxa_adjoint</span>
  </a>
</div>
<!-- apiref:nav:end -->

<!-- generated from OdeResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `ode::OdeResult`

The state at the end time and the work the solver did.

`y_final` is the state at `t1`. `steps` counts the steps taken (for RK4 it equals `options.steps`), `function_evaluations` the calls of the right-hand side, and `rejected_steps` the adaptive steps that failed the error test and were retried (always 0 for RK4). `converged` is true when the solve reached `t1`.

## Fields

### `y_final`

`pluss::Series`, default `undefined`.

The state at the end time `t1`, shaped like `y0`. For a backward integration (`t1 < t0`) it is the state at the earlier time.


### `steps`

`std::int64_t`, default `0`.

The number of accepted steps. For `RK4` it is exactly `OdeOptions::steps`; for `RK45` it is whatever the tolerance needed, and a count far above what the smoothness of the solution suggests points to stiffness.


### `function_evaluations`

`std::int64_t`, default `0`.

How many times the right-hand side was called, the honest measure of cost. `RK4` makes four per step; `RK45` makes seven per attempted step, rejected attempts included, so its count is seven times `steps + rejected_steps`.


### `rejected_steps`

`std::int64_t`, default `0`.

How many attempted steps failed the error test and were retried smaller (always 0 for `RK4`). A large number relative to `steps` means the solver is repeatedly overshooting: a rough initial step, a discontinuity, or a stiff region.


### `converged`

`bool`, default `false`.

True when the run reached the end time. The adaptive solvers throw rather than return a partial trajectory, so on a returned result this is true; it is kept so result structures of all the solvers share the same contract.

## Guidance

Returned by `fluxa` and `fluxa_linear`.

## Practical Examples

### Example 1

Reading a result.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const ode::OdeFn decay = [](double, const Series &y, const Series &) { return y * -1.0; };   // y' = -y, exact solution e^{-t}

  const ode::OdeResult r = ode::fluxa(decay, pluss::make_series({1.0}), 0.0, 1.0);
  out.writeln("y_final =", r.y_final.items<double>(), ", steps =", r.steps, ", evaluations =", r.function_evaluations, ", rejected =", r.rejected_steps, ", converged =", r.converged);
}
```

Output

```text
y_final = [0.36787944] , steps = 100 , evaluations = 400 , rejected = 0 , converged = true
```

### Example 2

An adaptive run that rejects steps. A rapidly changing solution makes the solver reduce its step and retry.

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
  const ode::OdeFn stiffish = [](double t, const Series &y, const Series &) { return (y * -50.0) + std::cos(t); };
  ode::OdeOptions options;
  options.method = ode::OdeMethod::RK45;
  options.initial_step = 1.0;
  const ode::OdeResult r = ode::fluxa(stiffish, pluss::make_series({0.0}), 0.0, 2.0, pluss::series::undefined(), options);
  out.writeln("steps =", r.steps, ", rejected =", r.rejected_steps, ", evaluations =", r.function_evaluations);
}
```

Output

```text
steps = 105 , rejected = 6 , evaluations = 777
```

### Example 3

A zero-length interval returns the initial state untouched.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const ode::OdeFn decay = [](double, const Series &y, const Series &) { return y * -1.0; };   // y' = -y, exact solution e^{-t}

  const ode::OdeResult r = ode::fluxa(decay, pluss::make_series({3.0}), 1.0, 1.0);
  out.writeln("y_final =", r.y_final.items<double>(), ", converged =", r.converged);
}
```

Output

```text
y_final = [3.0000] , converged = true
```

## See Also

- [`fluxa`](fluxa.md): produces it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="OdeOptions.html">
    <span class="label">Previous</span>
    <span class="title">OdeOptions</span>
  </a>
  <a class="next-link" href="fluxa.html">
    <span class="label">Next</span>
    <span class="title">fluxa</span>
  </a>
</div>
<!-- apiref:nav:end -->

<!-- generated from OdeOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `ode::OdeOptions`

The method, step count and adaptive-step controls of `fluxa`.

`method` picks the rule ([`OdeMethod`](OdeMethod.md)). `steps` is the number of equal steps of `RK4`; `RK45` ignores it except that it must be positive. The remaining fields drive `RK45`: a step is accepted when its weighted RMS error against `absolute_tolerance + relative_tolerance·|y|` is at most one; `initial_step` seeds the first step (0 lets the solver choose), `minimum_step` and `maximum_step` bound the step size (0 and infinity mean no bound), and `max_steps` caps the number of attempted steps so that a stiff or singular problem fails with an error instead of running on.

## Fields

### `method`

`ode::OdeMethod`, default `RK4`.

The step rule: `RK4` (fixed steps, the default) or `RK45` (adaptive Dormand-Prince). Choose `RK4` when you want a cost known in advance or need the adjoint or event solvers, which are defined for it; choose `RK45` when you want the solver to meet a tolerance and to spend steps only where the solution changes.


### `steps`

`std::int64_t`, default `100`.

The number of equal steps `RK4` takes across the interval, 100 by default; the error shrinks like `h⁴`, so doubling the steps cuts it about sixteenfold. `RK45` does not use it as a count, but it must still be positive.


### `relative_tolerance`

`double`, default `1e-6`.

`RK45` accepts a step when its weighted RMS error is within `absolute_tolerance + relative_tolerance·|y|`; this is the relative part, `1e-6` by default, so it is roughly the number of correct significant digits you ask for. The global error is usually within a small multiple of it, not exactly bounded by it.


### `absolute_tolerance`

`double`, default `1e-9`.

The absolute part of the same test, `1e-9` by default. It governs the error when components of `y` are near zero, where the relative part vanishes. Scale it to the natural size of each component: for a state measured in kilometres a value of `1e-9` is far stricter than one measured in millimetres.


### `initial_step`

`double`, default `0`.

The size of the first `RK45` step, default 0 which makes the solver estimate one from the derivative at the start. Supply it when you know the time scale of the problem; a wildly wrong value is only corrected by rejecting steps, which costs evaluations.


### `minimum_step`

`double`, default `0`.

The smallest step `RK45` may take, default 0 (no bound). When the tolerance would need a smaller step the solver throws instead of stalling or silently exceeding the tolerance, which is the signal of a stiff or singular problem.


### `maximum_step`

`double`, default `+inf`.

The largest step `RK45` may take, default infinity. Cap it when the solution has features narrower than the solver would otherwise step over, such as a short pulse in the forcing, or when you want output at a minimum density.


### `max_steps`

`std::int64_t`, default `100000`.

The most steps `RK45` may attempt, rejected ones included, default 100000. Running out throws rather than returning a partial trajectory, which turns a runaway stiff problem into an error you can see.

## Guidance

Passed to `fluxa`, `fluxa_until` and `fluxa_adjoint`.

## Practical Examples

### Example 1

The number of RK4 steps. The error falls like `h⁴`: doubling the steps cuts the error about sixteenfold.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(12));
  const ode::OdeFn decay = [](double, const Series &y, const Series &) { return y * -1.0; };   // y' = -y, exact solution e^{-t}

  for (const std::int64_t steps : {5, 10, 20, 40}) {
    ode::OdeOptions options;
    options.steps = steps;
    const ode::OdeResult r = ode::fluxa(decay, pluss::make_series({1.0}), 0.0, 1.0, pluss::series::undefined(), options);
    out.writeln("steps", steps, ": error =", std::abs(r.y_final.items<double>()[0] - std::exp(-1.0)));
  }
}
```

Output

```text
steps 5 : error = 0.000005796954
steps 10 : error = 0.000000333241
steps 20 : error = 0.000000019976
steps 40 : error = 0.000000001223
```

### Example 2

Tolerances and a maximum step. Capping the step size forces more steps even on an easy problem.

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

  for (const double max_step : {1.0, 0.05}) {
    ode::OdeOptions options;
    options.method = ode::OdeMethod::RK45;
    options.maximum_step = max_step;
    const ode::OdeResult r = ode::fluxa(decay, pluss::make_series({1.0}), 0.0, 1.0, pluss::series::undefined(), options);
    out.writeln("maximum step", max_step, ": steps =", r.steps);
  }
}
```

Output

```text
maximum step 1.0000 : steps = 6
maximum step 0.0500 : steps = 21
```

### Example 3

The step budget. A step budget too small to reach the end time makes the adaptive solver throw rather than return a partial answer.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const ode::OdeFn decay = [](double, const Series &y, const Series &) { return y * -1.0; };   // y' = -y, exact solution e^{-t}

  ode::OdeOptions options;
  options.method = ode::OdeMethod::RK45;
  options.max_steps = 2;
  options.relative_tolerance = 1e-12;
  try {
    (void)ode::fluxa(decay, pluss::make_series({1.0}), 0.0, 10.0, pluss::series::undefined(), options);
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::ode::fluxa: adaptive step budget exhausted
```

### Example 4

The method. The same problem with each rule: RK4 spends a known number of evaluations, RK45 spends what the tolerance demands.

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

  for (const ode::OdeMethod method : {ode::OdeMethod::RK4, ode::OdeMethod::RK45}) {
    ode::OdeOptions options;
    options.method = method;
    const ode::OdeResult r = ode::fluxa(decay, pluss::make_series({1.0}), 0.0, 5.0, pluss::series::undefined(), options);
    out.writeln(method == ode::OdeMethod::RK4 ? "RK4 " : "RK45", ": steps =", r.steps, ", evaluations =", r.function_evaluations);
  }
}
```

Output

```text
RK4  : steps = 100 , evaluations = 400
RK45 : steps = 23 , evaluations = 161
```

### Example 5

The relative tolerance. Tightening it by a factor of a hundred costs steps but buys accuracy, as the error against the exact `e^{-1}` shows.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(12));
  const ode::OdeFn decay = [](double, const Series &y, const Series &) { return y * -1.0; };   // y' = -y, exact solution e^{-t}

  for (const double tol : {1e-4, 1e-6, 1e-8}) {
    ode::OdeOptions options;
    options.method = ode::OdeMethod::RK45;
    options.relative_tolerance = tol;
    const ode::OdeResult r = ode::fluxa(decay, pluss::make_series({1.0}), 0.0, 1.0, pluss::series::undefined(), options);
    out.writeln("relative tolerance", tol, ": steps =", r.steps, ", error =", std::abs(r.y_final.items<double>()[0] - std::exp(-1.0)));
  }
}
```

Output

```text
relative tolerance 0.000100000000 : steps = 4 , error = 0.000010684570
relative tolerance 0.000001000000 : steps = 6 , error = 0.000000078208
relative tolerance 0.000000010000 : steps = 12 , error = 0.000000000932
```

### Example 6

The absolute tolerance. When the solution decays toward zero a purely relative test becomes meaningless, and the absolute tolerance is what bounds the error. A larger one lets the solver take fewer steps on the tail.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(12));
  const ode::OdeFn decay = [](double, const Series &y, const Series &) { return y * -1.0; };   // y' = -y, exact solution e^{-t}

  for (const double atol : {1e-3, 1e-12}) {
    ode::OdeOptions options;
    options.method = ode::OdeMethod::RK45;
    options.relative_tolerance = 1e-3;
    options.absolute_tolerance = atol;
    const ode::OdeResult r = ode::fluxa(decay, pluss::make_series({1.0}), 0.0, 20.0, pluss::series::undefined(), options);
    out.writeln("absolute tolerance", atol, ": steps =", r.steps);
  }
}
```

Output

```text
absolute tolerance 0.001000000000 : steps = 11
absolute tolerance 0.000000000001 : steps = 24
```

### Example 7

The initial step. By default the solver estimates a first step itself; a first step that is far too large is rejected and retried smaller, which shows in `rejected_steps`.

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

  for (const double h0 : {0.0, 5.0}) {
    ode::OdeOptions options;
    options.method = ode::OdeMethod::RK45;
    options.initial_step = h0;
    const ode::OdeResult r = ode::fluxa(decay, pluss::make_series({1.0}), 0.0, 10.0, pluss::series::undefined(), options);
    out.writeln("initial step", h0, ": steps =", r.steps, ", rejected =", r.rejected_steps);
  }
}
```

Output

```text
initial step 0.0000 : steps = 41 , rejected = 0
initial step 5.0000 : steps = 40 , rejected = 2
```

### Example 8

The minimum step. A tolerance that needs steps smaller than the minimum cannot be met, and the solver says so instead of ignoring the bound.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const ode::OdeFn decay = [](double, const Series &y, const Series &) { return y * -1.0; };   // y' = -y, exact solution e^{-t}

  ode::OdeOptions options;
  options.method = ode::OdeMethod::RK45;
  options.relative_tolerance = 1e-10;
  options.minimum_step = 0.5;
  try {
    (void)ode::fluxa(decay, pluss::make_series({1.0}), 0.0, 5.0, pluss::series::undefined(), options);
    out.writeln("completed");
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::ode::fluxa: required step is below minimum_step
```

## See Also

- [`fluxa`](fluxa.md): uses it
- [`OdeMethod`](OdeMethod.md): the methods

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="OdeMethod.html">
    <span class="label">Previous</span>
    <span class="title">OdeMethod</span>
  </a>
  <a class="next-link" href="OdeResult.html">
    <span class="label">Next</span>
    <span class="title">OdeResult</span>
  </a>
</div>
<!-- apiref:nav:end -->

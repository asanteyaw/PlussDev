<!-- generated from OdeEventOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `ode::OdeEventOptions`

Direction and refinement tolerances of event location.

`direction` selects the crossings that count. Once a step brackets a sign change it is refined by bisection until the bracket is narrower than `time_tolerance` or the event value is within `value_tolerance` of zero, or `max_refinements` bisections have been made; each bisection integrates a sub-step and evaluates the event once, counted in `event_evaluations`.

## Fields

### `direction`

`ode::OdeEventDirection`, default `Any`.

Which sign changes of the event function count, `Any` by default. Use `Falling` for a quantity that crosses zero from above (a ball reaching the ground) and `Rising` for the opposite; crossings in the other direction are stepped over. See [`OdeEventDirection`](OdeEventDirection.md).


### `time_tolerance`

`double`, default `1e-10`.

Bisection stops when the bracket around the crossing is narrower than this, `1e-10` by default, so the event time is located to about this much, provided the integration itself is that accurate: below the accuracy of the underlying steps (see `OdeOptions`) tightening it buys nothing, as the last row of the example shows. Each halving costs one event evaluation, so each factor of ten costs about three.


### `value_tolerance`

`double`, default `1e-10`.

Bisection also stops as soon as the event function is within this of zero, `1e-10` by default. Set it to 0 to rely on the time tolerance alone, or raise it when the event function is noisy and an exact zero is meaningless.


### `max_refinements`

`std::int64_t`, default `48`.

The most bisections made on one bracket, 48 by default (enough to reach machine precision from a unit bracket). Lowering it caps the cost at the price of an approximate event time, as the example shows.

## Guidance

Passed to `fluxa_until`.

## Practical Examples

### Example 1

Direction. Only a falling crossing of the oscillator's `x = cos t` is wanted: the first is at `π/2`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const ode::OdeFn oscillator = [](double, const Series &y, const Series &) { return y.select(0, 1).reshape({1}).unite({(y.select(0, 0) * -1.0).reshape({1})}, 0); };
  const ode::OdeEventFn zero = [](double, const Series &y, const Series &) { return y.select(0, 0); };
  for (const ode::OdeEventDirection d : {ode::OdeEventDirection::Falling, ode::OdeEventDirection::Rising}) {
    ode::OdeEventOptions event;
    event.direction = d;
    const ode::OdeEventResult r = ode::fluxa_until(oscillator, zero, pluss::make_series({1.0, 0.0}), 0.0, 7.0, pluss::series::undefined(), ode::OdeOptions{}, event);
    out.writeln(static_cast<int>(d), ": t =", r.t_final, ", triggered =", r.triggered);
  }
}
```

Output

```text
-1 : t = 1.570797 , triggered = true
1 : t = 4.712390 , triggered = true
```

### Example 2

The refinement budget. With few bisections the event time is only approximate.

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
  const ode::OdeFn decay = [](double, const Series &y, const Series &) { return y * -1.0; };
  const ode::OdeEventFn threshold = [](double, const Series &y, const Series &) { return y.select(0, 0) - 0.5; };
  for (const std::int64_t n : {2, 8, 48}) {
    ode::OdeEventOptions event;
    event.max_refinements = n;
    event.time_tolerance = 1e-14;
    event.value_tolerance = 0.0;
    const ode::OdeEventResult r = ode::fluxa_until(decay, threshold, pluss::make_series({1.0}), 0.0, 5.0, pluss::series::undefined(), ode::OdeOptions{}, event);
    out.writeln("refinements", n, ": error =", std::abs(r.t_final - std::log(2.0)));
  }
}
```

Output

```text
refinements 2 : error = 0.006852819440
refinements 8 : error = 0.000016881940
refinements 48 : error = 0.000000035381
```

### Example 3

The value tolerance. The event is accepted as soon as its value is within the tolerance of zero.

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
  const ode::OdeFn decay = [](double, const Series &y, const Series &) { return y * -1.0; };
  const ode::OdeEventFn threshold = [](double, const Series &y, const Series &) { return y.select(0, 0) - 0.5; };
  for (const double tol : {1e-2, 1e-9}) {
    ode::OdeEventOptions event;
    event.value_tolerance = tol;
    event.time_tolerance = 1e-14;
    const ode::OdeEventResult r = ode::fluxa_until(decay, threshold, pluss::make_series({1.0}), 0.0, 5.0, pluss::series::undefined(), ode::OdeOptions{}, event);
    out.writeln("value tolerance", tol, ": |event value| =", std::abs(r.event_value.item<double>()));
  }
}
```

Output

```text
value tolerance 0.010000000000 : |event value| = 0.003414677333
value tolerance 0.000000001000 : |event value| = 0.000000000347
```

### Example 4

The time tolerance. Bisection stops once the bracket around the crossing is narrower than the time tolerance, so the error in the event time is bounded by it.

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
  const ode::OdeFn decay = [](double, const Series &y, const Series &) { return y * -1.0; };
  const ode::OdeEventFn threshold = [](double, const Series &y, const Series &) { return y.select(0, 0) - 0.5; };
  for (const double tol : {1e-2, 1e-6, 1e-10}) {
    ode::OdeEventOptions event;
    event.time_tolerance = tol;
    event.value_tolerance = 0.0;
    const ode::OdeEventResult r = ode::fluxa_until(decay, threshold, pluss::make_series({1.0}), 0.0, 5.0, pluss::series::undefined(), ode::OdeOptions{}, event);
    out.writeln("time tolerance", tol, ": error =", std::abs(r.t_final - std::log(2.0)), ", event evaluations =", r.event_evaluations);
  }
}
```

Output

```text
time tolerance 0.010000000000 : error = 0.000602819440 , event evaluations = 18
time tolerance 0.000001000000 : error = 0.000000097272 , event evaluations = 31
time tolerance 0.000000000100 : error = 0.000000035432 , event evaluations = 44
```

## See Also

- [`fluxa_until`](fluxa_until.md): uses it
- [`OdeEventDirection`](OdeEventDirection.md): the directions

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="OdeEventDirection.html">
    <span class="label">Previous</span>
    <span class="title">OdeEventDirection</span>
  </a>
  <a class="next-link" href="OdeEventResult.html">
    <span class="label">Next</span>
    <span class="title">OdeEventResult</span>
  </a>
</div>
<!-- apiref:nav:end -->

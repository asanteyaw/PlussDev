<!-- generated from OdeEventDirection.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `ode::OdeEventDirection`

Which sign changes of the event function count.

An event is a sign change of the event function along the trajectory. `Rising` counts a change from negative to non-negative, `Falling` from positive to non-positive, and `Any` either. Choosing a direction ignores the crossings you do not want: a bouncing ball's height falls through zero at every impact and rises through it at none.

## Fields

### `Any`

`enumerator`, default `0`.

Either sign change.

### `Rising`

`enumerator`, default `1`.

Negative to non-negative.

### `Falling`

`enumerator`, default `-1`.

Positive to non-positive.

## Guidance

Set as `OdeEventOptions::direction`.

## Practical Examples

### Example 1

A ball dropped from 10 m. Height crosses zero once, falling, at `t = √(2h/g) = 1.4278`.

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
  const ode::OdeFn ball = [](double, const Series &y, const Series &) {   // y = [height, velocity] under gravity
    return y.select(0, 1).reshape({1}).unite({(y.select(0, 1) * 0.0 - 9.81).reshape({1})}, 0);
  };

  const ode::OdeEventFn hit = [](double, const Series &y, const Series &) { return y.select(0, 0); };
  ode::OdeEventOptions event;
  event.direction = ode::OdeEventDirection::Falling;
  const ode::OdeEventResult r = ode::fluxa_until(ball, hit, pluss::make_series({10.0, 0.0}), 0.0, 5.0, pluss::series::undefined(), ode::OdeOptions{}, event);
  out.writeln("triggered =", r.triggered, ", t =", r.t_final, ", exact =", std::sqrt(2.0 * 10.0 / 9.81));
}
```

Output

```text
triggered = true , t = 1.427843 , exact = 1.427843
```

### Example 2

The wrong direction finds nothing. Looking for a rising crossing of the height, the ball never triggers it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const ode::OdeFn ball = [](double, const Series &y, const Series &) {   // y = [height, velocity] under gravity
    return y.select(0, 1).reshape({1}).unite({(y.select(0, 1) * 0.0 - 9.81).reshape({1})}, 0);
  };

  const ode::OdeEventFn hit = [](double, const Series &y, const Series &) { return y.select(0, 0); };
  ode::OdeEventOptions event;
  event.direction = ode::OdeEventDirection::Rising;
  const ode::OdeEventResult r = ode::fluxa_until(ball, hit, pluss::make_series({10.0, 0.0}), 0.0, 1.0, pluss::series::undefined(), ode::OdeOptions{}, event);
  out.writeln("triggered =", r.triggered, ", t reached =", r.t_final);
}
```

Output

```text
triggered = false , t reached = 1.0000
```

### Example 3

`Any` takes whichever crossing comes first. A harmonic oscillator `x = cos t` crosses zero (falling) at `π/2`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const ode::OdeFn oscillator = [](double, const Series &y, const Series &) {
    return y.select(0, 1).reshape({1}).unite({(y.select(0, 0) * -1.0).reshape({1})}, 0);
  };
  const ode::OdeEventFn zero = [](double, const Series &y, const Series &) { return y.select(0, 0); };
  const ode::OdeEventResult r = ode::fluxa_until(oscillator, zero, pluss::make_series({1.0, 0.0}), 0.0, 5.0);
  out.writeln("t =", r.t_final, ", pi/2 =", 3.14159265358979323846 / 2.0);
}
```

Output

```text
t = 1.570796 , pi/2 = 1.570796
```

## See Also

- [`fluxa_until`](fluxa_until.md): uses it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="OdeAdjointResult.html">
    <span class="label">Previous</span>
    <span class="title">OdeAdjointResult</span>
  </a>
  <a class="next-link" href="OdeEventOptions.html">
    <span class="label">Next</span>
    <span class="title">OdeEventOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->

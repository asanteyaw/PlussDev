<!-- generated from stopping_at_an_event.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Stopping at an event

[Back to Events, Stiffness and Gradients](../ode_02_events_stiffness_and_gradients.md)


`ode::fluxa_until(f, event, y0, t0, t1, theta, options, event_options)` integrates until the scalar `event(t, y, theta)` changes sign, then refines the crossing and returns the state there. It uses the fixed-step RK4 and refines the crossing time within the step where it occurred.

| `OdeEventOptions` | Meaning | Default |
|---|---|---|
| `direction` | `Any`, `Rising` (negative to positive) or `Falling` | `Any` |
| `time_tolerance`, `value_tolerance` | refinement targets | 1e-10, 1e-10 |
| `max_refinements` | cap on bisection steps | 48 |

The result extends `OdeResult` with `triggered`, `t_final`, `event_value`, and `event_evaluations`.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
int main() {
  const ode::OdeFn fall = [](double, const Series& y, const Series&) {
    return y.select(0, 1).reshape({1}).unite({vec({-9.81})}, 0);
  };
  const ode::OdeEventFn ground = [](double, const Series& y, const Series&) { return y.select(0, 0); };
  ode::OdeOptions o;
  o.steps = 64;
  ode::OdeEventOptions e;
  e.direction = ode::OdeEventDirection::Falling;
  const auto r = ode::fluxa_until(fall, ground, vec({10.0, 0.0}), 0.0, 5.0, {}, o, e);
  const auto y = r.y_final.host<double>();
  std::cout << std::fixed << std::setprecision(6);
  std::cout << "triggered : " << r.triggered << "\n";
  std::cout << "t         : " << r.t_final << "   (exact " << std::sqrt(2.0 * 10.0 / 9.81) << ")\n";
  std::cout << std::scientific << std::setprecision(1) << "height    : " << std::abs(y[0]) << "   (should be 0)\n" << std::fixed << std::setprecision(6);
  std::cout << "velocity  : " << y[1] << "   (exact " << -std::sqrt(2.0 * 9.81 * 10.0) << ")\n";
  e.direction = ode::OdeEventDirection::Rising;
  const auto none = ode::fluxa_until(fall, ground, vec({10.0, 0.0}), 0.0, 5.0, {}, o, e);
  std::cout << "rising event: triggered = " << none.triggered << ", stopped at t = " << none.t_final << "\n";
}
```

Output

```text
triggered : 1
t         : 1.427843   (exact 1.427843)
height    : 2.8e-10   (should be 0)
velocity  : -14.007141   (exact -14.007141)
rising event: triggered = 0, stopped at t = 5.000000
```
The impact time and speed agree with the closed form to the printed digits, and the event value (the height) is zero to the refinement tolerance. Asking for a rising crossing, which never happens, returns `triggered = 0` and the state at `t1`: always check `triggered`.

// Stopping at an event. `ode::fluxa_until(f, event, y0, t0, t1, theta, options, event_options)` integrates until the scalar `event(t, y,
// theta)` changes sign, then refines the crossing time (by bisection on the step, to `time_tolerance` and `value_tolerance`) and returns
// the state at that time. `direction` selects Rising, Falling or Any crossings. The result is an OdeEventResult: `triggered`, `t_final`,
// `y_final`, the event value there, and the usual step counts. Here: a ball dropped from height 10 under gravity 9.81, state (height,
// velocity). The event is the height (zero at the ground), falling; the exact time is sqrt(2 * 10 / 9.81) = 1.4278 and the impact speed
// 14.007 m/s. A rising event is never triggered, so asking for one returns `triggered = false` at t1.
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

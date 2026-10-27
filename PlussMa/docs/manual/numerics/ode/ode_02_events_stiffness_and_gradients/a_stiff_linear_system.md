<!-- generated from a_stiff_linear_system.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A stiff linear system

[Back to Events, Stiffness and Gradients](../ode_02_events_stiffness_and_gradients.md)


A stiff system has a fast mode that dies out quickly and a slow mode you care about. An explicit method must keep its step below the stability limit of the *fast* mode for the whole run, even when that mode is long gone. `ode::fluxa_linear(action, y0, t0, t1, options)` solves `y' = A(t) y` with an adaptive implicit midpoint rule: `action(t, y)` applies `A` without forming it, and each implicit step is solved by the shared Krylov (GMRES) solver, configured through `LinearOdeOptions::linear`.

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
  const ode::LinearOdeFn action = [](double, const Series& y) { return y * vec({-100000.0, -1.0}); };
  ode::LinearOdeOptions lo;
  lo.initial_step = 0.01;
  lo.relative_tolerance = 1e-6;
  lo.absolute_tolerance = 1e-10;
  lo.linear.restart = 2;
  lo.linear.max_iterations = 8;
  const auto implicit = ode::fluxa_linear(action, vec({1.0, 1.0}), 0.0, 1.0, lo);

  const ode::OdeFn rhs = [](double, const Series& y, const Series&) { return y * vec({-100000.0, -1.0}); };
  ode::OdeOptions eo;
  eo.method = ode::OdeMethod::RK45;
  eo.relative_tolerance = 1e-6;
  eo.absolute_tolerance = 1e-10;
  const auto explicit_r = ode::fluxa(rhs, vec({1.0, 1.0}), 0.0, 1.0, {}, eo);

  auto show = [](const char* label, const ode::OdeResult& r) {
    const auto y = r.y_final.host<double>();
    std::cout << std::left << std::setw(26) << label << std::setw(10) << r.steps << std::setw(12) << r.rejected_steps << std::scientific << std::setprecision(2)
              << std::abs(y[1] - std::exp(-1.0)) << "\n";
  };
  std::cout << std::left << std::setw(26) << "method" << std::setw(10) << "steps" << std::setw(12) << "rejected" << "error in slow component\n";
  show("implicit midpoint (GMRES)", implicit);
  show("explicit RK45", explicit_r);
}
```

Output

```text
method                    steps     rejected    error in slow component
implicit midpoint (GMRES) 352       7           1.03e-05
explicit RK45             30276     764         5.16e-15
```
The explicit method is stable only for steps below about 3e-5 here, so it needs about 30,000 steps; the implicit method steps by the accuracy of the slow mode and needs about 350, nearly a hundred times fewer. The price is accuracy per step: the implicit midpoint rule is second order, and its final error (1e-5) is larger than the explicit run's. Use it for systems that are linear in the state and stiff; for a nonlinear stiff problem, linearise or choose the step budget accordingly.

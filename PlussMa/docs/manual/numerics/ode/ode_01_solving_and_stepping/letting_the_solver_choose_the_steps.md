<!-- generated from letting_the_solver_choose_the_steps.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Letting the solver choose the steps

[Back to Solving and Stepping](../ode_01_solving_and_stepping.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
int main() {
  const ode::OdeFn oscillator = [](double, const Series& y, const Series&) {
    return y.select(0, 1).reshape({1}).unite({(y.select(0, 0) * -1.0).reshape({1})}, 0);
  };
  const double t1 = 20.0 * 3.14159265358979323846;
  std::cout << std::left << std::setw(34) << "setting" << std::setw(10) << "steps" << std::setw(12) << "rejected" << std::setw(12) << "f evals" << "error\n";
  auto row = [&](const std::string& label, const ode::OdeResult& r) {
    const auto y = r.y_final.host<double>();
    const double err = std::max(std::abs(y[0] - 1.0), std::abs(y[1]));
    std::cout << std::left << std::setw(34) << label << std::setw(10) << r.steps << std::setw(12) << r.rejected_steps << std::setw(12) << r.function_evaluations
              << std::scientific << std::setprecision(1) << err << "\n";
  };
  for (std::int64_t steps : {500, 2000}) {
    ode::OdeOptions o;
    o.steps = steps;
    const auto r = ode::fluxa(oscillator, vec({1.0, 0.0}), 0.0, t1, {}, o);
    auto copy = r;
    copy.function_evaluations = 4 * steps;
    row("RK4, " + std::to_string(steps) + " steps", copy);
  }
  for (double tol : {1e-4, 1e-7, 1e-10}) {
    ode::OdeOptions o;
    o.method = ode::OdeMethod::RK45;
    o.relative_tolerance = tol;
    o.absolute_tolerance = tol * 1e-3;
    const auto r = ode::fluxa(oscillator, vec({1.0, 0.0}), 0.0, t1, {}, o);
    std::ostringstream label;
    label << "RK45, relative tolerance " << std::scientific << std::setprecision(0) << tol;
    row(label.str(), r);
  }
}
```

Output

```text
setting                           steps     rejected    f evals     error
RK4, 500 steps                    500       0           2000        1.3e-04
RK4, 2000 steps                   2000      0           8000        5.1e-07
RK45, relative tolerance 1e-04    113       27          980         8.9e-04
RK45, relative tolerance 1e-07    489       59          3836        7.9e-07
RK45, relative tolerance 1e-10    2008      56          14448       7.4e-10
```
An adaptive method puts steps where they are needed and keeps the **local** error of each step near the tolerance. The **global** error after many steps is larger than the tolerance per step (the 1e-4 row ends at 9e-4), because errors accumulate over the 10 periods: ask for a tolerance a few orders tighter than the accuracy you need at the end.

Compare the last RK45 row with RK4 at 2000 steps. RK45 takes about the same number of steps, spends about 1.8 times the function evaluations, and is about 700 times more accurate. The fifth-order method is far cheaper for a given accuracy, and it reports the effort honestly: 7 evaluations per attempted step, rejected steps included. Use a fixed-step method when you need the same grid every run (for example to compare with another computation, or for the adjoint of Chapter 2); use RK45 otherwise.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="a_first_solve_and_the_order_of_the_method.html">
    <span class="label">Previous</span>
    <span class="title">A first solve, and the order of the method</span>
  </a>
</div>
<!-- apiref:nav:end -->

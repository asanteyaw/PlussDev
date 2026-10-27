// A stiff linear system. y' = A(t) y with widely separated rates forces an explicit method to take tiny steps for stability, long after the
// fast component has died away. `ode::fluxa_linear(action, y0, t0, t1, options)` solves y' = A y with an adaptive implicit midpoint rule, applying
// A through a callable `action(t, y)` (a matrix is never formed) and solving each implicit step with a Krylov method (GMRES). Here:
// the diagonal system with rates -100000 and -1, started at (1, 1), integrated to t = 1; exact (0, e^-1) = (0, 0.36788). The same system
// by explicit RK45 is shown for comparison: look at the number of steps it needs.
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

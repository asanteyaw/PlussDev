// Adaptive steps. With `method = RK45` (Dormand-Prince 5(4)) the solver chooses its own step: it takes a step, estimates the local error
// from the difference between the fourth- and fifth-order results, accepts the step if that error is within `absolute_tolerance +
// relative_tolerance * |y|` (a weighted RMS over the state), and sizes the next step from it. It lands exactly on t1. `steps` is ignored
// (set `initial_step` to give a starting size, `maximum_step` to cap it). The result reports accepted `steps`, `rejected_steps` and
// `function_evaluations` (7 per attempted step). Here: a harmonic oscillator y'' = -y, state (y, y'), over 10 periods (t = 20 pi).
// Exact: y(20 pi) = (1, 0). Rows: fixed RK4 with increasing steps, then RK45 at decreasing tolerances.
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
    const auto y = r.y_final.to_vec<double>();
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

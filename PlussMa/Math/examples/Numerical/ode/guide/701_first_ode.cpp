// Solving dy/dt = f(t, y, theta) from t0 to t1. `ode::fluxa(f, y0, t0, t1, theta, options)` takes the right-hand side as a callable of
// time (a plain double), the state y and a parameter Series theta (pass an undefined Series, or leave it out, when f needs none); the state
// is a one-dimensional Series, so a system is a vector. The default method is the classical fourth-order Runge-Kutta with a fixed number
// of `steps`. The result holds the final state, the steps taken and whether it completed. Here: y' = -0.5 y, y(0) = 1, solved to t = 4
// (exact e^-2 = 0.1353352832366127), with the number of steps doubled each time: the error falls by a factor near 16 (fourth order).
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
int main() {
  const ode::OdeFn decay = [](double, const Series& y, const Series&) { return y * -0.5; };
  const double exact = std::exp(-2.0);
  std::cout << std::left << std::setw(8) << "steps" << std::setw(22) << "y(4)" << std::setw(14) << "error" << "error ratio\n";
  double previous = 0.0;
  for (std::int64_t steps : {5, 10, 20, 40, 80}) {
    ode::OdeOptions o;
    o.steps = steps;
    const auto r = ode::fluxa(decay, vec({1.0}), 0.0, 4.0, pluss::series::undefined(), o);
    const double y = r.y_final.squeeze().item<double>();
    const double err = std::abs(y - exact);
    std::cout << std::left << std::setw(8) << steps << std::setprecision(15) << std::fixed << std::setw(22) << y << std::scientific << std::setprecision(2) << std::setw(14) << err;
    if (previous > 0.0) std::cout << std::fixed << std::setprecision(1) << previous / err;
    std::cout << "\n";
    previous = err;
  }
}

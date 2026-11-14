// Gradients through a solve. `ode::fluxa_adjoint(f, y0, t0, t1, theta, grad_y_final, options)` returns the final state together with
// the gradient of a scalar loss L with respect to the initial state (`grad_y0`) and to the parameters (`grad_theta`), given the seed
// dL/dy(t1) you supply. It runs the fixed-step RK4 forward, storing the trajectory at every step, then walks it backward computing one
// step's vector-Jacobian product at a time; it never builds one graph over the whole trajectory. Here: y' = -theta y, y(0) = y0, loss = y(T)
// with T = 2, y0 = 1.5, theta = 0.7. Exact: dL/dy0 = e^{-theta T} = 0.2466 and dL/dtheta = -T y0 e^{-theta T} = -0.7398. The adjoint is
// the exact gradient of the discrete RK4 map, so it agrees with the true gradient up to the method's own error, which shrinks with `steps`.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
int main() {
  const ode::OdeFn decay = [](double, const Series& y, const Series& theta) { return y * theta * -1.0; };
  const double T = 2.0, y0 = 1.5, th = 0.7;
  const double dy0 = std::exp(-th * T), dth = -T * y0 * std::exp(-th * T);
  std::cout << std::left << std::setw(8) << "steps" << std::setw(18) << "dL/dy0" << std::setw(18) << "dL/dtheta" << "error in dL/dtheta\n";
  for (std::int64_t steps : {4, 8, 16, 32}) {
    ode::OdeOptions o;
    o.steps = steps;
    const auto g = ode::fluxa_adjoint(decay, vec({y0}), 0.0, T, vec({th}), vec({1.0}), o);
    std::cout << std::left << std::fixed << std::setprecision(8) << std::setw(8) << steps << std::setw(18) << g.grad_y0.squeeze().item<double>() << std::setw(18)
              << g.grad_theta.squeeze().item<double>() << std::scientific << std::setprecision(1) << std::abs(g.grad_theta.squeeze().item<double>() - dth) << "\n";
  }
  std::cout << "exact   " << std::fixed << std::setprecision(8) << std::setw(18) << dy0 << std::setw(18) << dth << "\n";
}

// The derivative machinery the solvers use is available on its own, on the same parameter packs. This program takes
// f(x, y) = x^2 y + sin(y) at (1.5, 0.7) and the vector function F(x, y) = (x y, x + y^2), and computes the gradient
// (`grad`, `value_and_grad`), the Jacobian (`jacrev`, and `jacfwd`, which returns the same matrix), a
// Jacobian-vector product (`jvp`) and an exact Hessian-vector product (`hvp`). Each is checked against the formula worked out
// by hand; `max error` is the largest absolute difference.
#include <pluss/ma.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> pair(double x, double y) { return optim::pack_scalars({{"x", scalar(x)}, {"y", scalar(y)}}); }
double at(const optim::ParameterPack<pluss::Series>& p, const char* name) { return as_f64(p.at(name)); }

// The two functions, written once on plain Series. Every derivative tool below accepts them as they are.
pluss::Series f_of(const optim::ParameterPack<pluss::Series>& p) {
  return p.at("x").square() * p.at("y") + p.at("y").sin();
}
pluss::Series big_f_of(const optim::ParameterPack<pluss::Series>& p) {
  return (p.at("x") * p.at("y")).reshape({1}).unite({(p.at("x") + p.at("y").square()).reshape({1})}, 0);
}

int main() {
  const double x = 1.5, y = 0.7;
  const auto point = pair(x, y);
  std::cout << std::fixed << std::setprecision(6);
  double worst = 0.0;
  const auto check = [&](const char* label, std::initializer_list<std::pair<double, double>> pairs) {
    double err = 0.0;
    for (const auto& [got, want] : pairs) err = std::max(err, std::abs(got - want));
    worst = std::max(worst, err);
    std::cout << std::left << std::setw(28) << label << "max error " << std::scientific << std::setprecision(1) << err << std::fixed << std::setprecision(6) << "\n";
  };

  const auto [value, g] = optim::value_and_grad(f_of, point);
  std::cout << "f = " << value.item<double>() << ", df/dx = " << at(g, "x") << ", df/dy = " << at(g, "y") << "\n";
  check("value_and_grad", {{value.item<double>(), x * x * y + std::sin(y)}, {at(g, "x"), 2 * x * y}, {at(g, "y"), x * x + std::cos(y)}});

  const auto [fv, jr] = optim::jacrev(big_f_of, point);
  check("jacrev", {{as_f64(jr.at("x").select(0, 0)), y}, {as_f64(jr.at("x").select(0, 1)), 1.0}, {as_f64(jr.at("y").select(0, 0)), x}, {as_f64(jr.at("y").select(0, 1)), 2 * y}});
  const auto [fv2, jf] = optim::jacfwd(big_f_of, point);
  check("jacfwd", {{as_f64(jf.at("x").select(0, 0)), y}, {as_f64(jf.at("x").select(0, 1)), 1.0}, {as_f64(jf.at("y").select(0, 0)), x}, {as_f64(jf.at("y").select(0, 1)), 2 * y}});

  // Directional derivative of f along (1, -2): df/dx - 2 df/dy.
  const auto [fj, directional] = optim::jvp(f_of, point, pair(1.0, -2.0));
  check("jvp along (1, -2)", {{as_f64(directional), 2 * x * y - 2 * (x * x + std::cos(y))}});

  // Hessian of f times the direction (1, -2): [f_xx f_xy; f_xy f_yy] (1, -2) with f_xx = 2y, f_xy = 2x, f_yy = -sin y.
  const auto [fh, hv] = optim::hvp(f_of, point, pair(1.0, -2.0));
  check("hvp along (1, -2)", {{at(hv, "x"), 2 * y * 1.0 + 2 * x * -2.0}, {at(hv, "y"), 2 * x * 1.0 - std::sin(y) * -2.0}});

  std::cout << "\nall five agree with the hand-worked formulas to better than 1e-12: " << std::boolalpha << (worst < 1e-12) << "\n";
  return 0;
}

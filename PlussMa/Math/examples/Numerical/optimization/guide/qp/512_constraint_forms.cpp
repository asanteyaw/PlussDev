// Three ways to write constraints with the single form lower <= A x <= upper.
//   equality       lower = upper:   minimise 0.5|x|^2 with x0 + x1 + x2 = 1            -> x = (1/3, 1/3, 1/3)
//   one-sided      upper = +inf:    minimise 0.5|x - c|^2 with x0 + x1 >= 4, c = (1, 1) -> x = (2, 2)
//   free variables A = identity, bounds +-inf: unconstrained minimiser of 0.5 x'Px + q'x   -> -P^{-1} q
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>

namespace qp = pluss::subsp::math::qp;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
const double inf = std::numeric_limits<double>::infinity();
Series mat(std::vector<std::vector<double>> rows) {
  std::vector<double> flat;
  for (const auto& r : rows) flat.insert(flat.end(), r.begin(), r.end());
  return pluss::make_series(flat, {static_cast<std::int64_t>(rows.size()), static_cast<std::int64_t>(rows[0].size())}).to(f64);
}
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
int main() {
  std::cout << std::fixed << std::setprecision(4);
  {
    const auto r = qp::solve(mat({{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}), vec({0, 0, 0}), mat({{1, 1, 1}}), vec({1.0}), vec({1.0}));
    const auto x = r.x.to_vec<double>();
    std::cout << "equality    : " << x[0] << ", " << x[1] << ", " << x[2] << "   converged " << r.converged << "\n";
  }
  {
    // 0.5|x - c|^2 = 0.5 x'x - c'x + const, so q = -c
    const auto r = qp::solve(mat({{1, 0}, {0, 1}}), vec({-1.0, -1.0}), mat({{1, 1}}), vec({4.0}), vec({inf}));
    const auto x = r.x.to_vec<double>();
    std::cout << "one-sided   : " << x[0] << ", " << x[1] << "   converged " << r.converged << "\n";
  }
  {
    const auto p = mat({{4.0, 1.0}, {1.0, 2.0}});
    const auto r = qp::solve(p, vec({1.0, 1.0}), mat({{1, 0}, {0, 1}}), vec({-inf, -inf}), vec({inf, inf}));
    const auto x = r.x.to_vec<double>();
    std::cout << "unconstrained: " << x[0] << ", " << x[1] << "   (exact " << -1.0 / 7.0 << ", " << -3.0 / 7.0 << ")\n";
  }
}

// A quadratic program: minimise 0.5 x'Px + q'x subject to  lower <= A x <= upper  (one row of A per constraint; an equality has
// lower = upper, a missing side is +-infinity, and a simple bound on x_i is a row of the identity). `qp::solve(P, q, A, lower, upper)`
// returns the solution x, the objective, the multipliers y, the final primal and dual residuals, the iteration count and whether it
// converged. Here: P = [[4,1],[1,2]], q = (1,1), with  x0 + x1 = 1,  0 <= x0 <= 0.7,  0 <= x1 <= 0.7. The solution is
// x = (0.3, 0.7), objective 1.88: the sum constraint is active and x1 sits at its upper bound.
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
  const auto p = mat({{4.0, 1.0}, {1.0, 2.0}});
  const auto q = vec({1.0, 1.0});
  const auto a = mat({{1.0, 1.0}, {1.0, 0.0}, {0.0, 1.0}});
  const auto lower = vec({1.0, 0.0, 0.0});
  const auto upper = vec({1.0, 0.7, 0.7});
  const auto r = qp::solve(p, q, a, lower, upper);
  const auto x = r.x.host<double>();
  std::cout << std::fixed << std::setprecision(4) << "x          : " << x[0] << ", " << x[1] << "\n";
  std::cout << "objective  : " << r.objective.item<double>() << "\n";
  std::cout << "converged  : " << (r.converged ? "yes" : "no") << " after " << r.iterations << " iterations\n";
  std::cout << std::scientific << std::setprecision(1) << "residuals  : primal " << r.primal_residual << ", dual " << r.dual_residual << "\n";
  const auto y = r.y.host<double>();
  std::cout << std::fixed << std::setprecision(4) << "multipliers: " << y[0] << ", " << y[1] << ", " << y[2] << "  (one per constraint row)\n";
}

// The smallest useful linear program. A workshop makes two products. Product 1 earns 3 per unit and product 2 earns 5. Each unit
// of product 1 needs 1 hour on machine A; each unit of product 2 needs 2 hours on machine B; and each unit of product 1 and of
// product 2 needs 3 and 2 hours respectively on machine C. The machines offer 4, 12 and 18 hours. How much of each should be
// made? The solver minimizes, so the profit is negated: minimize -3 x - 5 y. The problem is solved with both methods, which must
// agree: the interior-point method (the default) and the simplex method.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace lp = pluss::subsp::math::linprog;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }

int main() {
  lp::Problem problem;
  problem.c = vec({-3.0, -5.0});                         // minimize c . x, so a profit is entered negated
  problem.a_ub = mat({1, 0,                              // x          <= 4
                      0, 2,                              //     2 y    <= 12
                      3, 2}, 3, 2);                      // 3 x + 2 y  <= 18
  problem.b_ub = vec({4.0, 12.0, 18.0});
  problem.lower = vec({0.0, 0.0});                       // x, y >= 0

  std::cout << std::left << std::setw(16) << "method" << std::setw(12) << "status" << std::setw(8) << "iters" << std::setw(14) << "x, y" << "profit\n";
  for (const auto method : {lp::Method::InteriorPoint, lp::Method::Simplex}) {
    lp::Options options;
    options.method = method;
    const auto r = lp::solve(problem, options);
    const auto x = r.x.to_vec<double>();
    std::ostringstream point;
    point << std::fixed << std::setprecision(3) << x[0] << ", " << x[1];
    std::cout << std::left << std::setw(16) << (method == lp::Method::InteriorPoint ? "interior point" : "simplex") << std::setw(12)
              << (lp::success(static_cast<lp::Status>(static_cast<int>(r.status.item<double>()))) ? "optimal" : "other") << std::setw(8) << r.iterations
              << std::setw(14) << point.str() << std::fixed << std::setprecision(4) << -r.objective.item<double>() << "\n";
  }
  return 0;
}

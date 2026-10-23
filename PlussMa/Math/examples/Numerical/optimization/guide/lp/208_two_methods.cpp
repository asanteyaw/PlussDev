// The two methods agree on the optimal value but not always on the point. The program minimizes x0 + x1 subject to
// x0 + x1 >= 1 and 0 <= x <= 1. Every point on the segment from (1, 0) to (0, 1) is optimal, with the objective 1. A simplex
// method stops at a corner of the feasible region, so it returns one end of the segment; an interior-point method approaches the
// optimal face from inside and converges to a point in its middle. Neither is wrong. Which one you want depends on what the
// answer is for: a corner is a plan that uses as few variables as possible, a central point spreads the weight evenly.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace lp = pluss::subsp::math::linprog;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }

int main() {
  lp::Problem p;
  p.c = vec({1, 1});
  p.a_ub = mat({-1, -1}, 1, 2);
  p.b_ub = vec({-1});
  p.lower = vec({0, 0});
  p.upper = vec({1, 1});

  std::cout << std::fixed << std::setprecision(4);
  for (const auto method : {lp::Method::InteriorPoint, lp::Method::Simplex}) {
    lp::Options options;
    options.method = method;
    const auto r = lp::solve(p, options);
    const auto x = r.x.to_vec<double>();
    std::cout << std::left << std::setw(16) << (method == lp::Method::InteriorPoint ? "interior point" : "simplex") << "objective " << r.objective.item<double>()
              << "   x = (" << x[0] << ", " << x[1] << ")\n";
  }
  return 0;
}

// The ways a solve can end, each produced on purpose. Optimal: a point with a matching dual certificate. PrimalInfeasible: no
// point satisfies the constraints, and the result carries Farkas multipliers that prove it. Unbounded: the objective can be made as
// small as you like, and the result carries a ray along which it falls. IterationLimit: the budget ran out first. The status is
// read from the `status` Series, and `lp::success` accepts Optimal and NearOptimal (a stalled solve whose residuals are still
// small).
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <limits>

namespace lp = pluss::subsp::math::linprog;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }

int main() {
  const double inf = std::numeric_limits<double>::infinity();
  const auto run = [&](const char* what, const lp::Problem& p, const lp::Options& o = {}) {
    const auto r = lp::solve(p, o);
    const auto status = static_cast<lp::Status>(static_cast<int>(r.status.item<double>()));
    std::cout << std::left << std::setw(44) << what << std::setw(18) << lp::to_string(status) << "success: " << std::boolalpha << lp::success(status) << "\n";
    return r;
  };

  lp::Problem optimal;  // minimize x0 + x1 with x0 + x1 >= 1
  optimal.c = vec({1, 1});
  optimal.a_ub = mat({-1, -1}, 1, 2);
  optimal.b_ub = vec({-1});
  optimal.lower = vec({0, 0});
  run("x0 + x1 >= 1", optimal);

  lp::Problem infeasible;  // x0 + x1 <= 1 and x0 + x1 >= 3
  infeasible.c = vec({1, 1});
  infeasible.a_ub = mat({1, 1, -1, -1}, 2, 2);
  infeasible.b_ub = vec({1, -3});
  infeasible.lower = vec({0, 0});
  const auto r_infeasible = run("x0 + x1 <= 1 and x0 + x1 >= 3", infeasible);
  const auto farkas = r_infeasible.farkas.host<double>();
  std::cout << "    Farkas multipliers (y for each row): " << std::fixed << std::setprecision(3) << farkas[0] << ", " << farkas[1] << "\n";

  lp::Problem unbounded;  // minimize -x0 with x0 free to grow
  unbounded.c = vec({-1, 0});
  unbounded.a_ub = mat({0, 1}, 1, 2);
  unbounded.b_ub = vec({1});
  unbounded.lower = vec({0, 0});
  unbounded.upper = vec({inf, inf});
  const auto r_unbounded = run("minimize -x0 with x0 unrestricted above", unbounded);
  const auto ray = r_unbounded.ray.host<double>();
  std::cout << "    a ray along which the objective falls: (" << std::fixed << std::setprecision(3) << ray[0] << ", " << ray[1] << ")\n";

  lp::Options tiny;
  tiny.max_iterations = 1;
  run("an iteration budget of 1", optimal, tiny);
  return 0;
}

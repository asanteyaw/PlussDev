// A mixed problem with every constraint family, and a routine for deciding whether to believe the answer. A plant makes
// five products. Profit from a product has diminishing returns, 8 sqrt(q) style, so the objective is concave in the
// quantities; two shared resources limit production (linear inequalities), a total-volume rule ties three products
// together (a linear equality), a pollution measure is nonlinear and capped (a nonlinear inequality), and each quantity has
// a capacity (bounds). After solving, the program runs the checks that chapter 2 recommends before trusting a result: the
// exit flag, the constraint violation, the first-order optimality measure, and which constraints are active according
// to their multipliers.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t r, std::int64_t c) { return pluss::make_series(v, {r, c}).to(f64); }

int main() {
  const std::vector<double> value = {9, 7, 8, 6, 5};
  auto negative_profit = [&](const pluss::Series& q) {
    return -1.0 * (vec(value) * (q + 1.0).pow(0.5)).sum();  // diminishing returns
  };
  const auto plant =
      optim::constraints()
          .lbs(vec({0, 0, 0, 0, 0}))
          .ubs(vec({40, 40, 40, 40, 40}))
          .ln_ineq(mat({2, 1, 3, 1, 2,      // machine hours
                        1, 3, 1, 2, 1},     // labour hours
                       2, 5),
                   vec({100, 90}))
          .ln_eq(mat({1, 1, 1, 0, 0}, 1, 5), vec({30}))  // products 1-3 together make exactly 30 units
          .nl_ineq([](const Pack& p) { return p.at("x").square().sum() / 100.0 - 12.0; });  // pollution cap

  const auto r = optim::optima(negative_profit, vec({5, 5, 5, 5, 5}), {}, plant);
  const auto q = r.params.at("x").host<double>();

  std::cout << std::fixed << std::setprecision(4);
  std::cout << "quantities:";
  for (double v : q) std::cout << " " << v;
  std::cout << "\nprofit measure " << -r.evaluation.value.item<double>() << "\n\n";

  std::cout << "checks before trusting the result\n";
  std::cout << "  exit flag " << static_cast<int>(r.exit_flag) << " (1 means converged)\n";
  std::cout << std::scientific << std::setprecision(1) << "  max constraint violation " << r.evaluation.max_constraint_violation << "\n";
  std::cout << "  first-order optimality " << r.output.first_order_optimality << "\n" << std::fixed << std::setprecision(4);
  const auto machine = r.multipliers.linear_inequalities.host<double>();
  const auto pollution = r.multipliers.nonlinear_inequalities.host<double>();
  std::cout << "  multipliers: machine hours " << machine[0] << ", labour hours " << machine[1] << ", pollution cap " << pollution[0]
            << "\n  a multiplier of zero means that limit does not bind\n";
  return 0;
}

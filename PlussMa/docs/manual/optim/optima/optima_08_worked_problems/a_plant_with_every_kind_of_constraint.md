<!-- generated from a_plant_with_every_kind_of_constraint.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A plant with every kind of constraint

[Back to Worked Problems](../optima_08_worked_problems.md)


A plant makes five products. Profit from each has diminishing returns, so the objective, the negative of a weighted sum of square roots, is convex. Two shared resources limit production (linear inequalities: machine hours and labour hours), products 1 to 3 must together make exactly 30 units (a linear equality), a pollution measure is capped (a nonlinear inequality), and each quantity has a capacity (bounds). After the solve the program prints the checks of the routine.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

namespace optim = pluss::optim;
using Pack = optim::ParameterPack<optim::Trace>;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t r, std::int64_t c) { return pluss::make_series(v, {r, c}).to(f64); }

int main() {
  const std::vector<double> value = {9, 7, 8, 6, 5};
  auto negative_profit = [&](const optim::Trace& q) {
    return -1.0 * optim::sum(optim::trace(vec(value), false) * optim::pow(q + 1.0, 0.5));  // diminishing returns
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
          .nl_ineq([](const Pack& p) { return optim::sum(optim::square(p.at("x"))) / 100.0 - 12.0; });  // pollution cap

  const auto r = optim::optima(negative_profit, vec({5, 5, 5, 5, 5}), {}, plant);
  const auto q = r.params.at("x").to_vec<double>();

  std::cout << std::fixed << std::setprecision(4);
  std::cout << "quantities:";
  for (double v : q) std::cout << " " << v;
  std::cout << "\nprofit measure " << -r.evaluation.value.item<double>() << "\n\n";

  std::cout << "checks before trusting the result\n";
  std::cout << "  exit flag " << static_cast<int>(r.exit_flag) << " (1 means converged)\n";
  std::cout << std::scientific << std::setprecision(1) << "  max constraint violation " << r.evaluation.max_constraint_violation << "\n";
  std::cout << "  first-order optimality " << r.output.first_order_optimality << "\n" << std::fixed << std::setprecision(4);
  const auto machine = r.multipliers.linear_inequalities.to_vec<double>();
  const auto pollution = r.multipliers.nonlinear_inequalities.to_vec<double>();
  std::cout << "  multipliers: machine hours " << machine[0] << ", labour hours " << machine[1] << ", pollution cap " << pollution[0]
            << "\n  a multiplier of zero means that limit does not bind\n";
  return 0;
}
```

Output

```text
quantities: 15.7105 6.6804 7.6091 18.0691 10.5011
profit measure 122.8206

checks before trusting the result
  exit flag 1 (1 means converged)
  max constraint violation 1.1e-12
  first-order optimality 3.4e-06
  multipliers: machine hours 0.2624, labour hours 0.2123, pollution cap 0.0000
  a multiplier of zero means that limit does not bind
```
The solve converges with the quantities 15.71, 6.68, 7.61, 18.07 and 10.50, a violation of 1.1e-12 and an optimality measure of 3.4e-06, so steps 4 and 5 pass. The multipliers, step 6, tell the story of the plan. Both resource limits bind and have positive multipliers: machine hours 0.2624 and labour hours 0.2123, the extra profit measure from one more hour of each. The pollution cap has a multiplier of zero: the plant is well inside it, and loosening it would change nothing. You can confirm all three without the multipliers: the machine hours used are `2(15.7105) + 6.6804 + 3(7.6091) + 18.0691 + 2(10.5011) = 100`, the labour hours are 90, and the pollution measure, the sum of squares divided by 100, is 7.86 against a cap of 12. Products 1 to 3 sum to 30.0000. The multipliers say which limits are worth relaxing, and here they point at the two resources, not the cap.

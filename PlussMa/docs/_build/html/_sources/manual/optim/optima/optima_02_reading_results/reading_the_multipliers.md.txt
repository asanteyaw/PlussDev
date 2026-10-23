<!-- generated from reading_the_multipliers.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Reading the multipliers

[Back to Reading Results and Watching a Run](../optima_02_reading_results.md)


A Lagrange multiplier measures how hard a constraint pushes on the answer. For an inequality it is the first-order price: relaxing the limit by one unit lowers the objective by about that amount. The result carries six multiplier groups, one per constraint family. This program uses three problems whose multipliers can be worked out by hand and checks each.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  using Pack = optim::ParameterPack<optim::Trace>;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const optim::Trace& x) { return optim::sum(optim::square(x - 3.0)); };
  std::cout << std::fixed << std::setprecision(4);

  // Bounds x <= (2, 2.5): the gradient at the answer is (-2, -1), so the upper multipliers are (2, 1).
  const auto bounded = optim::optima(objective, vec({0.0, 0.0}), {}, optim::constraints().ubs(vec({2.0, 2.5})));
  const auto upper = bounded.multipliers.upper_bounds.to_vec<double>();
  const auto lower = bounded.multipliers.lower_bounds.to_vec<double>();
  std::cout << "bounds:            upper = [" << upper[0] << ", " << upper[1] << "] (expected 2, 1); lower multipliers: "
            << lower.size() << " (no lower bound set)\n";

  // x0 + x1 <= 4: at (2, 2) the gradient is (-2, -2) and the constraint row is (1, 1), so the multiplier is 2.
  const auto linear = optim::optima(
      objective, vec({0.0, 0.0}), {},
      optim::constraints().ln_ineq(pluss::make_series(std::vector<double>{1, 1}, {1, 2}).to(f64), vec({4.0})));
  std::cout << "linear inequality: " << linear.multipliers.linear_inequalities.to_vec<double>()[0]
            << " (expected 2)\n";

  // x0^2 + x1^2 = 1: at (1,1)/sqrt(2) the gradient is -(3 - 0.7071) * (2, 2); the row is 2*(0.7071, 0.7071).
  const auto nonlinear = optim::optima(
      objective, vec({1.0, 0.0}), {},
      optim::constraints().nl_eq([](const Pack& p) { return optim::sum(optim::square(p.at("x"))) - 1.0; }));
  std::cout << "nonlinear equality: " << nonlinear.multipliers.nonlinear_equalities.to_vec<double>()[0]
            << " (expected 3.2426)\n";
  return 0;
}
```

Output

```text
bounds:            upper = [2.0000, 1.0000] (expected 2, 1); lower multipliers: 0 (no lower bound set)
linear inequality: 2.0000 (expected 2)
nonlinear equality: 3.2426 (expected 3.2426)
```
The bound problem has gradient `(-2, -1)` at its answer, so the upper multipliers are `(2, 1)` and the lower group is empty because no lower bound was set. The linear inequality has multiplier `2`, the price of tightening `x0 + x1 <= 4`. The nonlinear equality has multiplier `3.2426`, which equals `(3 - 1/sqrt 2) * sqrt 2`, the gradient magnitude divided by the constraint-gradient magnitude. A family the problem does not use has an empty multiplier series; an inactive constraint has multiplier zero.

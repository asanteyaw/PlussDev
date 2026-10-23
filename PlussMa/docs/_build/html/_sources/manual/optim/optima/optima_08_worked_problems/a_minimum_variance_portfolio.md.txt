<!-- generated from a_minimum_variance_portfolio.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A minimum-variance portfolio

[Back to Worked Problems](../optima_08_worked_problems.md)


Four assets with a covariance matrix and expected returns. The weights must sum to 1 (a linear equality), the expected return must reach 9% (a linear inequality, written as `-expected . w <= -0.09`) and each weight lies between 0 and 0.6 (bounds). The objective is the variance `w' S w`, written on a column of weights because a matrix product needs matrices.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

namespace optim = pluss::optim;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t r, std::int64_t c) { return pluss::make_series(v, {r, c}).to(f64); }

pluss::Series covariance() {
  return mat({0.10, 0.02, 0.04, 0.00,
              0.02, 0.08, 0.01, 0.02,
              0.04, 0.01, 0.20, 0.03,
              0.00, 0.02, 0.03, 0.06}, 4, 4);
}
const std::vector<double> expected = {0.12, 0.10, 0.15, 0.05};

optim::OptimaState solve(double target) {
  auto risk = [](const optim::Trace& w) {
    const auto column = pluss::subsp::math::diff::reshape(w, {4, 1});  // matmul wants matrices
    return optim::sum(column * optim::matmul(optim::trace(covariance(), false), column));
  };
  // The return constraint expected . w >= target is written as -expected . w <= -target.
  const auto constraints = optim::constraints()
                               .lbs(vec({0, 0, 0, 0}))
                               .ubs(vec({0.6, 0.6, 0.6, 0.6}))
                               .ln_eq(mat({1, 1, 1, 1}, 1, 4), vec({1.0}))
                               .ln_ineq(mat({-0.12, -0.10, -0.15, -0.05}, 1, 4), vec({-target}));
  return optim::optima(risk, vec({0.25, 0.25, 0.25, 0.25}), {}, constraints);
}

int main() {
  const auto r = solve(0.09);
  const auto w = r.params.at("x").to_vec<double>();
  double total = 0.0, ret = 0.0;
  for (std::size_t i = 0; i < 4; ++i) { total += w[i]; ret += expected[i] * w[i]; }
  std::cout << std::fixed << std::setprecision(4) << "exit flag " << static_cast<int>(r.exit_flag) << "\n";
  std::cout << "weights: " << w[0] << " " << w[1] << " " << w[2] << " " << w[3] << "\n";
  std::cout << "sum of weights " << total << ", expected return " << ret << " (target 0.0900)\n";
  std::cout << "variance " << std::setprecision(6) << r.evaluation.value.item<double>() << "\n";

  const double price = r.multipliers.linear_inequalities.to_vec<double>()[0];
  const double above = solve(0.0905).evaluation.value.item<double>();
  const double below = solve(0.0895).evaluation.value.item<double>();
  std::cout << std::setprecision(4) << "\nmultiplier of the return constraint " << price << "\n";
  std::cout << "change in variance per unit of target, by re-solving at 0.0905 and 0.0895: " << (above - below) / 0.001 << "\n";
  return 0;
}
```

Output

```text
exit flag 1
weights: 0.2875 0.2594 0.0691 0.3841
sum of weights 1.0000, expected return 0.0900 (target 0.0900)
variance 0.033957

multiplier of the return constraint 0.1826
change in variance per unit of target, by re-solving at 0.0905 and 0.0895: 0.1826
```
The solver converges. The weights are 0.2875, 0.2594, 0.0691 and 0.3841; they sum to 1, the expected return is exactly the 9% target, and the minimum variance is 0.033957. No weight is at a bound, so only the equality and the return constraint shape the answer.

The last two lines are step 7. The multiplier of the return constraint is 0.1826: the price, in variance, of asking for one more unit of return. The program checks that this is true without using the multiplier at all, by solving again with the target raised to 0.0905 and lowered to 0.0895 and dividing the change in the minimum variance by 0.001. The two numbers agree to four digits. A multiplier that matches the re-solved price is a strong sign that the whole solution is right.

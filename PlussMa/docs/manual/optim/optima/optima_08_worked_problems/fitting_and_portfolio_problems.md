<!-- generated from fitting_and_portfolio_problems.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A portfolio and a maximum-likelihood fit

[Back to Optima 8: Worked Problems](../optima_08_worked_problems.md)

Two more worked problems, from finance and statistics. A minimum-variance portfolio is a convex problem with a linear equality (the weights sum to one), a return requirement written as a linear inequality, and bounds on each weight, and its multiplier on the return constraint has a direct meaning: the price, in variance, of asking for one more unit of return. The program checks that meaning without using the multiplier at all, by solving again with the target raised and lowered and dividing the change in variance by the change in target.

The second problem is a maximum-likelihood fit of a Weibull distribution, and it carries the most important lesson of the chapter: **a flag cannot tell you everything.** One start returns `Converged` with an excellent-looking optimality measure and a wrong answer, because the solver judged optimality relative to the size of a gradient that was enormous at that start. The program exposes it with a check the solver never saw.

## Program 1: A minimum-variance portfolio

Four assets with a covariance matrix and expected returns. The weights must sum to 1 (a linear equality), the expected return must reach 9% (a linear inequality, written as `-expected . w <= -0.09`) and each weight lies between 0 and 0.6 (bounds). The objective is the variance `w' S w`, written on a column of weights because a matrix product needs matrices.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

namespace optim = pluss::subsp::math::optim;
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
  auto risk = [](const pluss::Series& w) {
    const auto column = w.reshape({4, 1});  // matmul wants matrices
    return (column * covariance().matmul(column)).sum();
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
  const auto w = r.params.at("x").host<double>();
  double total = 0.0, ret = 0.0;
  for (std::size_t i = 0; i < 4; ++i) { total += w[i]; ret += expected[i] * w[i]; }
  std::cout << std::fixed << std::setprecision(4) << "exit flag " << static_cast<int>(r.exit_flag) << "\n";
  std::cout << "weights: " << w[0] << " " << w[1] << " " << w[2] << " " << w[3] << "\n";
  std::cout << "sum of weights " << total << ", expected return " << ret << " (target 0.0900)\n";
  std::cout << "variance " << std::setprecision(6) << r.evaluation.value.item<double>() << "\n";

  const double price = r.multipliers.linear_inequalities.host<double>()[0];
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

## Program 4: A maximum-likelihood fit, and what a flag cannot tell you

Forty lifetimes are placed at the evenly spaced quantiles of a Weibull distribution with shape 2.5 and scale 100. The unknowns are the shape `k` and the scale `s`, both positive. The objective is the negative log-likelihood. At the maximum of the likelihood the scale satisfies `s^k = mean(x^k)` exactly, which gives a check the solver never sees: the last column of the output is `s^k / mean(x^k)`, and it should be 1.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> ks(double k, double s) { return optim::pack_scalars({{"k", scalar(k)}, {"s", scalar(s)}}); }

std::vector<double> lifetimes() {
  std::vector<double> x;
  for (int i = 0; i < 40; ++i) x.push_back(100.0 * std::pow(-std::log(1.0 - (i + 0.5) / 40.0), 1.0 / 2.5));
  return x;
}

int main() {
  const auto x = lifetimes();
  std::vector<double> log_x;
  for (double v : x) log_x.push_back(std::log(v));
  const auto log_data = pluss::make_series(log_x, {40}).to(f64);

  // negative log-likelihood = -sum[ ln k - k ln s + (k - 1) ln x - (x / s)^k ]
  auto nll = [&](const Pack& p) {
    const pluss::Series constants = log_data;
    const pluss::Series k = p.at("k"), log_s = p.at("s").log();
    const pluss::Series term = k.log() - k * log_s + (k - 1.0) * constants - (k * (constants - log_s)).exp();
    return -1.0 * term.sum();
  };
  const auto fit = optim::constraints().lbs(ks(0.1, 1.0)).ubs(ks(20.0, 1000.0));

  const auto mean_power = [&](double k) {
    double total = 0.0;
    for (double v : x) total += std::pow(v, k);
    return total / static_cast<double>(x.size());
  };

  std::cout << std::fixed << std::setprecision(4);
  std::cout << std::left << std::setw(14) << "start" << std::setw(6) << "flag" << std::setw(7) << "iters" << std::setw(20) << "fitted k, s"
            << std::setw(14) << "optimality" << "s^k / mean(x^k)\n";
  const std::pair<double, double> starts[] = {{1.0, 50.0}, {5.0, 200.0}, {2.0, 80.0}, {10.0, 20.0}};
  for (const auto& [k0, s0] : starts) {
    const auto r = optim::optima(nll, ks(k0, s0), {}, fit);
    const double k = as_f64(r.params.at("k")), s = as_f64(r.params.at("s"));
    std::ostringstream label, fitted, optimality;
    label << "(" << k0 << ", " << s0 << ")";
    fitted << std::fixed << std::setprecision(4) << k << ", " << s;
    optimality << std::scientific << std::setprecision(1) << r.output.first_order_optimality;
    std::cout << std::left << std::setw(14) << label.str() << std::setw(6) << static_cast<int>(r.exit_flag) << std::setw(7) << r.iteration
              << std::setw(20) << fitted.str() << std::setw(14) << optimality.str() << std::fixed << std::setprecision(4)
              << std::pow(s, k) / mean_power(k) << "\n";
  }
  return 0;
}
```

Output

```text
start         flag  iters  fitted k, s         optimality    s^k / mean(x^k)
(1, 50)       1     11     2.5436, 99.9382     2.9e-04       1.0000
(5, 200)      1     17     2.5437, 99.9388     5.7e-06       1.0000
(2, 80)       1     8      2.5437, 99.9387     1.6e-05       1.0000
(10, 20)      1     19     3.0579, 21.4785     6.2e-07       0.0082
```
The first three starts are ordinary and all return `k` close to 2.54 and `s` close to 99.94, near the 2.5 and 100 that generated the data (the data sit exactly on the quantiles, so the fit is close to the generating values but not equal to them), and the check column is 1.0000 each time. Several starting points that agree is evidence that you have the global answer, which chapter 3 showed you cannot assume for every problem.

The fourth start, `(10, 20)`, is the instructive one. The solver reports flag `1` and an optimality measure of 6.2e-07, which looks excellent, and the answer, `k = 3.0579`, `s = 21.4785`, is wrong: the check column reads 0.0082. At that start the objective is 6.4e9 and its gradient 1.4e10. The default algorithm, sequential QP, judges optimality relative to the size of the gradient at the start (never less than 1), so after the gradient has fallen by six orders of magnitude it counts as small, although the gradient is still 8.4e3 in absolute terms. The flag and the optimality measure are both honest answers to a question about the starting scale, not to the question you cared about. Three habits protect you:

- start where the objective and its gradient are of ordinary size, as the first three starts do;
- run from more than one start when a wrong answer would be costly;
- check the answer by an identity or a closed form that the solver did not use, as the last column does.

## Decisions and pitfalls

- **A multiplier is a derivative you can verify.** Re-solving with a perturbed constraint is the independent check; the two numbers should agree to several digits.
- **Start where the objective and its gradient are of ordinary size.** The scaled optimality test can pass trivially from a start with a huge gradient.
- **Run from more than one start when a wrong answer is costly.** Several starts that agree are evidence of the global answer.
- **Check with an identity the solver did not use.** The Weibull maximum satisfies an equation exactly; evaluating it at the answer is a one-line test that caught the bad fit.
- **`Converged` is a statement about tolerances, not about truth.** It tells you the stopping rule was met; whether the point is the one you want is for you to verify.

Where next: That completes the Optima chapters; [Septima](../../septima.md) is the library's first-order and quasi-Newton solver family.

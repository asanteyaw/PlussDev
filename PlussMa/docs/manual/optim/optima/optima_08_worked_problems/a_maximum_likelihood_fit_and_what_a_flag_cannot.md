<!-- generated from a_maximum_likelihood_fit_and_what_a_flag_cannot.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A maximum-likelihood fit, and what a flag cannot tell you

[Back to Worked Problems](../optima_08_worked_problems.md)


Forty lifetimes are placed at the evenly spaced quantiles of a Weibull distribution with shape 2.5 and scale 100. The unknowns are the shape `k` and the scale `s`, both positive. The objective is the negative log-likelihood. At the maximum of the likelihood the scale satisfies `s^k = mean(x^k)` exactly, which gives a check the solver never sees: the last column of the output is `s^k / mean(x^k)`, and it should be 1.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

namespace optim = pluss::optim;
using Pack = optim::ParameterPack<optim::Trace>;
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
    const optim::Trace constants = optim::trace(log_data, false);
    const optim::Trace k = p.at("k"), log_s = optim::log(p.at("s"));
    const optim::Trace term = optim::log(k) - k * log_s + (k - 1.0) * constants - optim::exp(k * (constants - log_s));
    return -1.0 * optim::sum(term);
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
    const double k = r.params.at("k").item<double>(), s = r.params.at("s").item<double>();
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

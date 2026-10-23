<!-- generated from mixtures.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Mixtures

[Back to Heavy Tails and Mixtures](../dist_03_heavy_tails_and_mixtures.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace distr = pluss::subsp::stats::distr;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
void row(const char* label, const Series& s) {
  std::cout << std::left << std::setw(12) << label << std::right;
  for (double v : s.reshape({s.numel()}).to_vec<double>()) std::cout << std::setw(10) << v;
  std::cout << "\n";
}
int main() {
  const auto cfg = pluss::Config{}.with_dtype(f64);
  const distr::GaussianMixture truth{vec({0.3, 0.7}), vec({-2.0, 1.5}), vec({0.8, 1.2})};
  const Series x = truth.sample({600}, cfg, 77ULL);
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "true            weights 0.3 0.7   means -2.0 1.5   standard deviations 0.8 1.2\n\n";

  // Two very different starting points reach the same maximum.
  for (const auto& start : std::vector<std::vector<double>>{{-1.0, 1.0}, {-3.0, 3.0}}) {
    const auto fit = distr::GaussianMixture{vec({0.5, 0.5}), vec(start), vec({1.0, 1.0})}.MLE(x);
    std::cout << "start at means (" << start[0] << ", " << start[1] << ")\n";
    row("  weights", fit.weights);
    row("  means", fit.means);
    row("  stddevs", fit.stddevs);
    std::cout << "  log-likelihood " << fit.loglik(x).item<double>() << "\n";
  }

  const auto fit = distr::GaussianMixture{vec({0.5, 0.5}), vec({-1.0, 1.0}), vec({1.0, 1.0})}.MLE(x);
  std::cout << "\nstandard errors of (w1, w2, m1, m2, s1, s2)\n";
  row("  stderr", fit.stderr(x));
  const auto ci = fit.confint(x).to_vec<double>();
  std::cout << "  95% interval of the first mean   " << ci[4] << "  to  " << ci[5] << "\n";

  // One normal against two: the likelihood gain pays for the three extra parameters.
  const auto one = distr::Normal{}.MLE(x);
  std::cout << "\nlog-likelihood: one normal " << std::setprecision(2) << one.loglik(x).item<double>() << ", two components " << fit.loglik(x).item<double>() << "   (Akaike 2k - 2 loglik: "
            << 2 * 2 - 2 * one.loglik(x).item<double>() << " against " << 2 * 5 - 2 * fit.loglik(x).item<double>() << ")\n";
}
```

Output

```text
true            weights 0.3 0.7   means -2.0 1.5   standard deviations 0.8 1.2

start at means (-1.0000, 1.0000)
  weights       0.2797    0.7203
  means        -2.0551    1.4474
  stddevs       0.8873    1.2661
  log-likelihood -1211.7021
start at means (-3.0000, 3.0000)
  weights       0.2797    0.7203
  means        -2.0551    1.4474
  stddevs       0.8873    1.2661
  log-likelihood -1211.7021

standard errors of (w1, w2, m1, m2, s1, s2)
  stderr        0.0451    0.0451    0.2089    0.1389    0.1354    0.0945
  95% interval of the first mean   -2.4645  to  -1.6456

log-likelihood: one normal -1255.49, two components -1211.70   (Akaike 2k - 2 loglik: 2514.98 against 2433.40)
```
Two starts that differ greatly (component means at −1 and 1, or at −3 and 3) converge to the same fit, the maximum of the likelihood (−1211.70). Its weights, means and standard deviations are within about one standard error of the generating ones (weights 0.3 and 0.7 for 0.28 and 0.72; means −2.0 and 1.5 for −2.06 and 1.45; standard deviations 0.8 and 1.2 for 0.89 and 1.27: the first standard deviation is 0.6 standard errors from the truth). The interval printed for the first mean is −2.46 to −1.65. A single normal fits the same data much worse (log-likelihood −1255.5 against −1211.7, Akaike's criterion 2515 against 2433): the mixture, with five parameters, is clearly preferred.

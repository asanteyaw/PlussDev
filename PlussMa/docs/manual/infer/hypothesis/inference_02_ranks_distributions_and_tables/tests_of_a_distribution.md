<!-- generated from tests_of_a_distribution.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Tests of a distribution

[Back to Ranks, Distributions and Tables](../inference_02_ranks_distributions_and_tables.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
Series table(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }
const char* yes_no(bool b) { return b ? "reject" : "keep"; }
// two samples used throughout: cup fill volumes (cl) from two filling machines
const std::vector<double> machine_a{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1};
const std::vector<double> machine_b{4.8, 5.2, 5.0, 4.7, 5.6, 4.9, 5.1, 4.6, 5.3};
int main() {
  const Series a = vec(machine_a), b = vec(machine_b);
  std::cout << std::fixed << std::setprecision(4);
  const auto k1 = hyp::kstest(a, 5.5, 0.5);
  std::cout << "KS, A against N(5.5, 0.5^2)  : D = " << k1.stat << " (D+ " << k1.d_plus << ", D- " << k1.d_minus << "), p = " << k1.pvalue << ", " << yes_no(k1.reject) << "\n";
  hyp::TestOptions approx;
  approx.method = hyp::PValueMethod::Approximate;
  std::cout << "   with the Kolmogorov series: p = " << hyp::kstest(a, 5.5, 0.5, approx).pvalue << "  (the exact value is the right one at n = 8)\n";
  const auto k2 = hyp::kstest2(a, b);
  std::cout << "KS two-sample, A against B   : D = " << k2.stat << ", p = " << k2.pvalue << ", " << yes_no(k2.reject) << "\n";
  std::vector<double> sample;
  for (int i = 0; i < 30; ++i) sample.push_back(std::sin(i * 1.3) * 2.0 + (i % 5) * 0.3);
  const auto jb = hyp::jbtest(vec(sample));
  std::cout << "Jarque-Bera, 30 values       : JB = " << jb.stat << ", p = " << jb.pvalue << ", skewness " << jb.skewness << ", kurtosis " << jb.kurtosis << ", " << yes_no(jb.reject) << "\n";
  const auto chi = hyp::chisq_gof(vec({18, 22, 31, 29}), vec({25, 25, 25, 25}));
  std::cout << "chi-squared goodness of fit  : chi2 = " << chi.stat << " on " << chi.dof << " df, p = " << chi.pvalue << ", " << yes_no(chi.reject) << "  (a fair four-sided die?)\n";
}
```

Output

```text
KS, A against N(5.5, 0.5^2)  : D = 0.2257 (D+ 0.0808, D- 0.2257), p = 0.7316, keep
   with the Kolmogorov series: p = 0.7534  (the exact value is the right one at n = 8)
KS two-sample, A against B   : D = 0.5278, p = 0.1423, keep
Jarque-Bera, 30 values       : JB = 2.1218, p = 0.3461, skewness -0.0003, kurtosis 1.6971, keep
chi-squared goodness of fit  : chi2 = 4.4000 on 3.0000 df, p = 0.2214, keep  (a fair four-sided die?)
```
- **Kolmogorov–Smirnov, one sample.** `kstest(x, mean0, stddev0)` measures the largest gap between the sample's cumulative distribution and the normal one. The reported statistic is D, with D⁺ and D⁻ the gaps in each direction. The null parameters must be fixed beforehand: if the mean and standard deviation are estimated from the same data, the test is conservative and its p-value too large. At n = 8 the exact p-value (0.7316) and the Kolmogorov-series approximation (0.7534) differ in the second digit.
- **Kolmogorov–Smirnov, two samples.** `kstest2(x, y)` asks whether two samples have the same distribution, detecting differences in location, spread or shape. Here the machines are not distinguishable (p = 0.14, exact).
- **Jarque–Bera.** `jbtest(x)` tests normality from the skewness and kurtosis. The 30 values here have a kurtosis of 1.70 (flat compared with the normal's 3) but the sample is too small to reject.
- **Chi-squared goodness of fit.** `chisq_gof(observed, expected, options)` compares counts with expected counts; `ChiGofOptions::ddof` removes degrees of freedom for estimated parameters. 18, 22, 31, 29 against a fair die's 25 each gives chi² = 4.4 on 3 degrees of freedom, p = 0.22: no evidence of bias.

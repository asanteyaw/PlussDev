<!-- generated from tests_of_means.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Tests of means

[Back to Means and Variances](../inference_01_means_and_variances.md)


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
  const auto t = hyp::ttest(a, 5.0);
  const auto ci = t.ci.to_vec<double>();
  std::cout << "one-sample, A against 5.0 : t = " << t.stat << " on " << t.dof << " df, p = " << t.pvalue << ", " << yes_no(t.reject) << "; mean " << t.mean
            << ", 95% interval [" << ci[0] << ", " << ci[1] << "]\n";
  const auto tb = hyp::ttest(b, 5.0);
  std::cout << "one-sample, B against 5.0 : t = " << tb.stat << ", p = " << tb.pvalue << ", " << yes_no(tb.reject) << "\n";
  const auto pooled = hyp::ttest2(a, b);
  const auto pci = pooled.ci.to_vec<double>();
  std::cout << "A vs B, pooled variance   : t = " << pooled.stat << " on " << pooled.dof << " df, p = " << pooled.pvalue << ", " << yes_no(pooled.reject)
            << "; difference of means in [" << pci[0] << ", " << pci[1] << "]\n";
  hyp::TTest2Options welch;
  welch.equal_variance = false;
  const auto w = hyp::ttest2(a, b, welch);
  std::cout << "A vs B, Welch             : t = " << w.stat << " on " << w.dof << " df, p = " << w.pvalue << ", " << yes_no(w.reject) << "\n";
  const Series b8 = vec({4.8, 5.2, 5.0, 4.7, 5.6, 4.9, 5.1, 4.6});
  const auto paired = hyp::ttest_paired(a, b8);
  std::cout << "paired (first 8 of B)     : t = " << paired.stat << " on " << paired.dof << " df, p = " << paired.pvalue << ", " << yes_no(paired.reject) << "\n";
}
```

Output

```text
one-sample, A against 5.0 : t = 3.5696 on 7.0000 df, p = 0.0091, reject; mean 5.6125, 95% interval [5.2068, 6.0182]
one-sample, B against 5.0 : t = 0.2114, p = 0.8379, keep
A vs B, pooled variance   : t = 3.0093 on 15.0000 df, p = 0.0088, reject; difference of means in [0.1722, 1.0084]
A vs B, Welch             : t = 2.9334 on 11.7874 df, p = 0.0127, reject
paired (first 8 of B)     : t = 2.9492 on 7.0000 df, p = 0.0214, reject
```
How to read the output. Machine A is clearly above target: the mean is 5.6125, the t statistic 3.57 on 7 degrees of freedom, the p-value 0.0091, and the 95% interval [5.21, 6.02] excludes 5.0. Machine B is not distinguishable from the target (p = 0.84). Comparing the machines, the pooled-variance test gives p = 0.0088 and Welch's test, which does not assume equal variances and uses fewer, fractional degrees of freedom (11.8), gives a slightly larger p = 0.0127. Prefer Welch's test unless you have a reason to believe the variances are equal; here both agree. The paired test uses the same cups in the same order, which is a different design: it tests the mean of the differences, with 7 degrees of freedom for 8 pairs, and is only appropriate when each element of `x` is matched to the same-index element of `y`.

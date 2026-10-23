<!-- generated from a_known_standard_deviation_and_tests_of_variance.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A known standard deviation, and tests of variances

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
  const auto z = hyp::ztest(a, 5.0, 0.5);
  std::cout << "z test, sigma = 0.5       : z = " << z.stat << ", p = " << z.pvalue << ", critical " << z.critical << ", " << yes_no(z.reject) << "\n";
  const auto v = hyp::vartest(a, 0.25);
  const auto vci = v.ci.to_vec<double>();
  std::cout << "variance of A vs 0.25     : chi2 = " << v.stat << " on " << v.dof << " df, p = " << v.pvalue << ", sample variance " << v.sample_variance
            << ", 95% interval [" << vci[0] << ", " << vci[1] << "]\n";
  const auto f = hyp::vartest2(a, b);
  const auto fci = f.ci.to_vec<double>();
  std::cout << "variance A vs B           : F = " << f.stat << " on (" << f.dof1 << ", " << f.dof2 << ") df, p = " << f.pvalue << ", " << yes_no(f.reject)
            << "; ratio in [" << fci[0] << ", " << fci[1] << "]\n";
  const auto g = hyp::f_ratio(a, b);
  std::cout << "f_ratio gives the same F  : " << g.stat << "\n";
}
```

Output

```text
z test, sigma = 0.5       : z = 3.4648, p = 0.0005, critical 1.9600, reject
variance of A vs 0.25     : chi2 = 6.5950 on 7.0000 df, p = 0.9445, sample variance 0.2355, 95% interval [0.1030, 0.9757]
variance A vs B           : F = 2.3685 on (7.0000, 8.0000) df, p = 0.2501, keep; ratio in [0.5230, 11.6042]
f_ratio gives the same F  : 2.3685
```
When the standard deviation is known (from long experience with the machine), the z test replaces the t test and its critical value is the normal 1.96. The variance of machine A (0.2355) is consistent with 0.25: the p-value of 0.94 is nowhere near rejection, and the 95% interval for the variance, [0.103, 0.976], is wide, which is what 8 observations buy. The F test compares the two machines' variances: the ratio is 2.37, p = 0.25, and the interval for the ratio, [0.52, 11.6], is wider still. A p-value of 0.25 does not prove the variances are equal; it says these data cannot tell. The variance tests assume normal data and are sensitive to departures from it.

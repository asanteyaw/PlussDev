// A z test, and tests about variances. `hyp::ztest(x, mean0, sigma)` tests a mean when the standard deviation sigma is known (the statistic is
// standard normal). `hyp::vartest(x, variance0)` tests a variance against variance0 with the chi-squared distribution and returns an interval for
// the variance; `hyp::vartest2(x, y)` (and its alias `f_ratio`) compares two variances by the ratio, with the F distribution. Same cups as before.
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

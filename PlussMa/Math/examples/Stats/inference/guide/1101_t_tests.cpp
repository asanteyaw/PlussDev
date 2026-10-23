// Tests about means. Every test returns a result with `stat` (the test statistic), `pvalue`, `critical` (the critical value of the statistic at
// level alpha) and `reject` (true when pvalue < alpha), plus test-specific fields. `hyp::ttest(x, mean0)` tests whether the mean of one sample equals
// mean0 and gives a confidence interval for the mean; `ttest_paired` tests the mean of the differences of two matched samples; `ttest2` compares the
// means of two independent samples, assuming equal variances by default (a pooled test) or not (Welch's test, `equal_variance = false`). Data: fill
// volumes of two machines, A (8 cups) and B (9 cups); the target volume is 5.0 cl.
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

// Tests that do not assume normality. `hyp::ranksum(x, y)` (Wilcoxon rank-sum / Mann-Whitney) asks whether one sample tends to be larger than the other;
// `hyp::signrank(x, median0)` and `hyp::signrank(x, y)` (Wilcoxon signed-rank) test a median or the paired differences, using the sizes of the
// differences as well as their signs; `hyp::signtest` uses only the signs and its p-value is an exact binomial. `stat` is a z score for the rank tests
// and the p-value comes from the exact distribution of the rank statistic when the samples are small (the smaller sample under 10 and under 20
// observations in all for `ranksum`; at most 15 nonzero differences for `signrank`) and from the normal approximation (with continuity correction
// and tie adjustment) otherwise. `TestOptions::method` forces Exact or Approximate. Cup data again.
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
  const Series a = vec(machine_a), b = vec(machine_b), b8 = vec({4.8, 5.2, 5.0, 4.7, 5.6, 4.9, 5.1, 4.6});
  std::cout << std::fixed << std::setprecision(5);
  hyp::TestOptions exact, approx;
  exact.method = hyp::PValueMethod::Exact;
  approx.method = hyp::PValueMethod::Approximate;
  std::cout << std::left << std::setw(26) << "test" << std::setw(14) << "default p" << std::setw(14) << "exact p" << std::setw(14) << "approx. p" << "\n";
  auto row = [&](const char* name, double d, double e, double ap) {
    std::cout << std::left << std::setw(26) << name << std::setw(14) << d << std::setw(14) << e << std::setw(14) << ap << "\n";
  };
  row("ranksum A vs B", hyp::ranksum(a, b).pvalue, hyp::ranksum(a, b, exact).pvalue, hyp::ranksum(a, b, approx).pvalue);
  row("signrank A vs median 5.5", hyp::signrank(a, 5.5).pvalue, hyp::signrank(a, 5.5, exact).pvalue, hyp::signrank(a, 5.5, approx).pvalue);
  row("signrank A vs B (paired)", hyp::signrank(a, b8).pvalue, hyp::signrank(a, b8, exact).pvalue, hyp::signrank(a, b8, approx).pvalue);
  const auto s = hyp::signtest(a, b8);
  std::cout << "\nsign test A vs B (paired): " << s.positives << " positive and " << s.negatives << " negative differences, exact binomial p = " << s.pvalue << "\n";
  const auto r = hyp::ranksum(a, b);
  std::cout << "rank sum of A = " << std::setprecision(1) << r.rank_sum_x << ", Mann-Whitney U = " << r.u_stat << " (ties get average ranks)\n";
}

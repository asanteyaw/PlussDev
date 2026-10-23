// Several groups, and tables of counts. `hyp::anova_1w(y, groups)` is one-way analysis of variance: `groups` labels each observation, and the F test asks
// whether the group means all agree; the result has the between- and within-group sums of squares, mean squares, degrees of freedom and the group
// means and sizes. `hyp::chisq_indep(table)` tests independence of the rows and columns of a contingency table and returns the expected counts.
// Data: yields under three fertilisers (4 plots each), and a 2 x 3 table of counts.
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
  const Series yield = vec({4.1, 5.2, 6.3, 5.5, 6.8, 7.1, 5.9, 6.4, 7.7, 8.2, 6.9, 7.5});
  const Series group = vec({0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2});
  const auto a = hyp::anova_1w(yield, group);
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "one-way ANOVA: F = " << a.stat << " on (" << a.dof_between << ", " << a.dof_within << ") df, p = " << a.pvalue << ", " << yes_no(a.reject) << "\n";
  std::cout << "  between: SS " << a.ss_between << ", MS " << a.ms_between << "   within: SS " << a.ss_within << ", MS " << a.ms_within << "\n";
  const auto means = a.group_means.to_vec<double>();
  std::cout << "  group means: " << means[0] << ", " << means[1] << ", " << means[2] << "  (" << a.groups << " groups, " << a.effective_n << " observations)\n";

  const auto c = hyp::chisq_indep(table({20, 15, 30, 35, 25, 15}, 2, 3));
  std::cout << "\nchi-squared test of independence: chi2 = " << c.stat << " on " << c.dof << " df, p = " << c.pvalue << ", " << yes_no(c.reject) << "\n";
  const auto e = c.expected.to_vec<double>();
  std::cout << "  expected counts: " << std::setprecision(2) << e[0] << " " << e[1] << " " << e[2] << " / " << e[3] << " " << e[4] << " " << e[5] << "\n";
}

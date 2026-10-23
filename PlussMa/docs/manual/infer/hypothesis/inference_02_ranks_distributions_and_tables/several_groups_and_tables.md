<!-- generated from several_groups_and_tables.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Several groups, and tables

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
```

Output

```text
one-way ANOVA: F = 11.4760 on (2.0000, 9.0000) df, p = 0.0033, reject
  between: SS 10.6217, MS 5.3108   within: SS 4.1650, MS 0.4628
  group means: 5.2750, 6.5500, 7.5750  (3 groups, 12 observations)

chi-squared test of independence: chi2 = 10.9324 on 2.0000 df, p = 0.0042, reject
  expected counts: 25.54 18.57 20.89 / 29.46 21.43 24.11
```
One-way analysis of variance splits the variation into between-group and within-group parts. The F statistic is their ratio of mean squares; a large F says the group means differ more than the scatter within groups explains. Here F = 11.5 on (2, 9) degrees of freedom, p = 0.003: the three fertilisers do not all give the same mean yield (5.28, 6.55, 7.58). The test says *that* the means differ, not *which*; follow it with pairwise comparisons, adjusted for the number of comparisons. `groups` holds a label for each observation, and `group_means` and `group_counts` come back in the order of the labels.

The chi-squared test of independence compares a table of counts with the counts expected if rows and columns were independent (row total times column total over the grand total). The statistic 10.9 on 2 degrees of freedom gives p = 0.004: the outcome depends on the row. The expected counts are returned so you can see where the table departs from independence; expected counts below about 5 make the chi-squared approximation unreliable.

<!-- generated from rank_and_sign_tests.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Rank and sign tests

[Back to Ranks, Distributions and Tables](../inference_02_ranks_distributions_and_tables.md)


| Test | Question | Uses |
|---|---|---|
| `ranksum(x, y)` | does one sample tend to be larger than the other? | ranks of the pooled data |
| `signrank(x, median0)`, `signrank(x, y)` | is the median `median0` / are paired differences centred on zero? | ranks of the absolute differences and their signs |
| `signtest(x, median0)`, `signtest(x, y)` | the same, using only the signs | counts of positives and negatives; exact binomial p-value |

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
```

Output

```text
test                      default p     exact p       approx. p     
ranksum A vs B            0.01637       0.01637       0.02069       
signrank A vs median 5.5  0.50000       0.50000       0.49819       
signrank A vs B (paired)  0.03125       0.03125       0.03547       

sign test A vs B (paired): 7 positive and 1 negative differences, exact binomial p = 0.07031
rank sum of A = 96.5, Mann-Whitney U = 60.5 (ties get average ranks)
```
For these small samples the default p-value is the exact one; the approximate p-value differs in the second digit (0.0207 against 0.0164 for the rank-sum test, and 0.0355 against 0.0313 for the paired signed-rank test), always because a continuous curve is fitted to a discrete distribution with few support points. The sign test is the weakest of the three because it ignores the sizes of the differences: with seven positive differences and one negative it gives p = 0.07, where the signed-rank test, which also sees that the positive differences are large, gives 0.03. None of the three assumes normality.

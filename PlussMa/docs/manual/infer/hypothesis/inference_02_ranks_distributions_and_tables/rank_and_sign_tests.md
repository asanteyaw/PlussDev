<!-- generated from rank_and_sign_tests.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Rank and sign tests

[Back to Ranks, Distributions and Tables](../inference_02_ranks_distributions_and_tables.md)

The t-tests of Chapter 1 compare means and assume the data are roughly normal. Rank tests replace each observation by its rank in the sorted data, so a wild value counts only as "the largest" and a skewed distribution does no harm. They answer a slightly different question: does one sample tend to give larger values than the other (`ranksum`), is the median of a sample (or of paired differences) at a stated value (`signrank`), or is the share of positive differences one half (`signtest`). This page uses the fill volumes of two machines, A (8 cups) and B (9 cups), and the same cups weighed on two scales.

## Two independent samples

`ranksum(x, y)` is the Wilcoxon rank-sum test, equivalent to the Mann–Whitney U test. All observations are ranked together, tied values share the average of their ranks, and the statistic is the sum of the ranks of `x`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace hyp = pluss::subsp::stats::inference::hypothesis;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const auto a = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());
  const auto b = pluss::make_series(std::vector<double>{4.8, 5.2, 5.0, 4.7, 5.6, 4.9, 5.1, 4.6, 5.3}, {9}, as_f64());

  const auto r = hyp::ranksum(a, b);
  out.writeln("rank sum of A", r.rank_sum_x, " Mann-Whitney U", r.u_stat, " z", r.stat, " p", r.pvalue, r.reject ? " -> reject" : " -> keep");
  out.writeln("observations used", r.effective_nx, r.effective_ny);

  // One-sided: does A tend to be larger than B?
  hyp::TestOptions larger;
  larger.tail = hyp::Tail::Right;
  const auto up = hyp::ranksum(a, b, larger);
  out.writeln("H1: A larger than B: p", up.pvalue);

  // For comparison, the t-test on the same data.
  out.writeln("two-sample t-test p", hyp::ttest2(a, b).pvalue);
}
```

Output

```text
rank sum of A 96.5000  Mann-Whitney U 60.5000  z 2.3137  p 0.0164  -> reject
observations used 8 9
H1: A larger than B: p 0.0082
two-sample t-test p 0.0088
```

The rank sum of machine A is 96.5 (the fractional part is a tie shared between two cups); the Mann–Whitney U statistic, the rank sum minus its smallest possible value, is 60.5, where 36 would be expected if the machines were alike and 72 is the maximum. The p-value is 0.0164, so the test rejects; `stat` reports the corresponding z score (2.3137). With 8 and 9 observations the p-value is exact. The one-sided version, "A tends to be larger than B", gives p = 0.0082, and the two-sample t-test on the same data gives 0.0088. The tests need not agree to the digit: they ask different questions, and under normal data the rank test loses very little power compared with the t-test.

## Paired data and a single median

`signrank(x, y)` is the Wilcoxon signed-rank test of paired data: it ranks the sizes of the differences and adds up the ranks of the positive ones (`wplus`) and the negative ones (`wminus`). `signrank(x, median0)` does the same for one sample against a stated median. `signtest` uses only the signs of the differences and gives an exact binomial p-value.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace hyp = pluss::subsp::stats::inference::hypothesis;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  // Eight cups weighed on two scales (matched pairs).
  const auto first = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());
  const auto second = pluss::make_series(std::vector<double>{5.3, 5.0, 6.3, 6.1, 6.1, 5.8, 5.4, 6.4}, {8}, as_f64());

  const auto sr = hyp::signrank(first, second);
  out.writeln("signed-rank: W+", sr.wplus, " W-", sr.wminus, " z", sr.stat, " p", sr.pvalue, " pairs used", sr.effective_n);

  const auto sg = hyp::signtest(first, second);
  out.writeln("sign test:   positive", sg.positives, " negative", sg.negatives, " p", sg.pvalue);

  // One sample against a hypothesised median.
  const auto med = hyp::signrank(first, 5.0);
  out.writeln("median of the first scale vs 5.0: W+", med.wplus, " W-", med.wminus, " p", med.pvalue);
}
```

Output

```text
signed-rank: W+ 0.0000  W- 36.0000  z -2.5361  p 0.0078  pairs used 8
sign test:   positive 0  negative 8  p 0.0078
median of the first scale vs 5.0: W+ 34.5000  W- 1.5000  p 0.0234
```

Every one of the eight cups reads higher on the second scale, so all eight differences are negative: W+ = 0 and W− = 36 (the sum of the ranks 1 to 8). The signed-rank p-value is 0.0078, which is 2 × (1/2)⁸, the probability that all eight signs agree if signs are equally likely; the sign test gives the same 0.0078 because every difference has the same sign. The two tests differ when the signs are mixed: the signed-rank test also uses how large each difference is, so it is the more powerful of the two when differences are informative. The last line tests the median of the first scale against 5.0: W+ = 34.5 and W− = 1.5 (a half-rank because of a tie), p = 0.0234.

## Exact and approximate p-values, and ties

A rank statistic takes finitely many values, so its null distribution can be listed exactly; for larger samples a normal curve is used. `TestOptions::method` forces `Exact` or `Approximate`; `Auto` is exact for small samples (the smaller of the two `ranksum` samples under 10 and fewer than 20 observations in all; at most 15 nonzero differences for `signrank`). The `ranksum` approximation uses a continuity correction and a variance adjusted for ties; the `signrank` approximation uses the adjusted variance and no continuity correction.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace hyp = pluss::subsp::stats::inference::hypothesis;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));

  // Two samples with repeated values (ties). The exact p-value is computed from the distribution of the rank statistic,
  // the approximate one from a normal curve with the variance adjusted for ties.
  const auto x = pluss::make_series(std::vector<double>{3, 5, 5, 7, 8, 8, 8, 10, 12, 12, 14, 15}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{4, 5, 6, 8, 8, 9, 11, 11, 13, 13, 13, 16, 18, 20}, {14}, as_f64());

  out.writeln("ranksum  method   p");
  const char* names[] = {"Auto", "Exact", "Approximate"};
  const hyp::PValueMethod methods[] = {hyp::PValueMethod::Auto, hyp::PValueMethod::Exact, hyp::PValueMethod::Approximate};
  for (int i = 0; i < 3; ++i) {
    hyp::TestOptions options;
    options.method = methods[i];
    out.writeln("        ", names[i], hyp::ranksum(x, y, options).pvalue);
  }
  out.writeln("signrank (x against median 6)  method   p");
  for (int i = 0; i < 3; ++i) {
    hyp::TestOptions options;
    options.method = methods[i];
    out.writeln("        ", names[i], hyp::signrank(x, 6.0, options).pvalue);
  }
}
```

Output

```text
ranksum  method   p
         Auto 0.26633
         Exact 0.26582
         Approximate 0.26633
signrank (x against median 6)  method   p
         Auto 0.02393
         Exact 0.02393
         Approximate 0.02751
```

The two samples here (12 and 14 observations) have repeated values. `Auto` chooses the approximation for the rank-sum test (the samples are too large for the exact rule) and its p-value, 0.26633, matches the forced `Approximate` result; the exact p-value is 0.26582. The three agree to two decimals and the decision is unaffected. For the signed-rank test of the first sample against a median of 6, `Auto` uses the exact distribution (the number of nonzero differences is under 16), and the exact p-value 0.02393 differs from the approximation 0.02751 in the second digit. The difference matters most in the tails of small samples, which is where the decision is often made.

## Why use ranks: an outlier

The simulation compares the t-test and the rank-sum test on two groups of 12 whose centres differ by about 1.6 standard deviations. Then one observation in the second group is replaced by a typing error, 45.0 for a value near 0.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace hyp = pluss::subsp::stats::inference::hypothesis;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  // Twelve measurements from each of two groups whose centres differ by about one unit. One reading in the second group
  // is mistyped (45.0 for 4.5).
  const auto cfg = as_f64();
  const auto g1 = pluss::series::randn({12}, cfg, 11).add(1.6);
  const auto clean = pluss::series::randn({12}, cfg, 12);
  auto values = clean.pull<double>();
  auto dirty_values = values;
  dirty_values[3] = 45.0;
  const auto g2 = pluss::make_series(values, {12}, cfg);
  const auto g2_dirty = pluss::make_series(dirty_values, {12}, cfg);

  out.writeln("clean data:  t-test p", hyp::ttest2(g1, g2).pvalue, " rank-sum p", hyp::ranksum(g1, g2).pvalue);
  out.writeln("with outlier: t-test p", hyp::ttest2(g1, g2_dirty).pvalue, " rank-sum p", hyp::ranksum(g1, g2_dirty).pvalue);
  out.writeln("sample means of group 2: clean", hyp::ttest(g2, 0.0).mean, " with outlier", hyp::ttest(g2_dirty, 0.0).mean);
}
```

Output

```text
clean data:  t-test p 0.0047  rank-sum p 0.0073
with outlier: t-test p 0.5320  rank-sum p 0.0464
sample means of group 2: clean 0.0905  with outlier 3.9203
```

On the clean data both tests find the difference: t-test p = 0.0047, rank-sum p = 0.0073. With the single mistyped value the t-test gives p = 0.5320 and no longer sees anything: the outlier moves the mean of the second group from 0.0905 to 3.9203 and inflates its variance. The rank-sum test still rejects, at p = 0.0464, because the outlier changes only one rank (the largest value is still just the largest). The rank test is not immune, since its p-value moved from 0.0073 to 0.0464, but it degrades gracefully.

## Decisions and pitfalls

- **Rank tests are not tests of the mean.** `ranksum` asks whether values from one sample tend to exceed values from the other. For distributions of different shapes that is not the same as equal means or equal medians.
- **Prefer `signrank` to `signtest` when sizes are informative.** The sign test ignores magnitudes and has lower power; it needs the fewest assumptions.
- **Ties are handled with average ranks.** The exact distribution is computed from the actual (average) ranks, and the approximation adjusts its variance for ties.
- **Zero differences are dropped.** `signrank` and `signtest` ignore pairs with no difference; `effective_n` reports the number that remain.
- **Force `Exact` or `Approximate` only to check.** The default has the right trade-off for each sample size.

Where next: [Tests of a distribution](tests_of_a_distribution.md) tests whether data follow a stated distribution.

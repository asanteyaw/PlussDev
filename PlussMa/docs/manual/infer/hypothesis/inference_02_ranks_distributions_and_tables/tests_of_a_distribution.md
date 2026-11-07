<!-- generated from tests_of_a_distribution.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Tests of a distribution

[Back to Ranks, Distributions and Tables](../inference_02_ranks_distributions_and_tables.md)

The tests so far ask about a location or a spread. These ask about the whole distribution: do the data come from a stated normal distribution (`kstest`), do two samples come from the same distribution (`kstest2`), are the data consistent with normality at all (`jbtest`). `kstest` and `kstest2` compare distribution functions and so notice differences in shape and spread as well as in centre; `jbtest` looks only at skewness and kurtosis.

## One sample against a stated normal distribution

`kstest(x, mean0, stddev0)` computes D, the largest vertical gap between the sample's empirical distribution function and the normal distribution function with the stated parameters. The result reports D together with its two one-sided parts, D⁺ (the sample's distribution function above the stated one) and D⁻ (below it).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace hyp = pluss::subsp::stats::inference::hypothesis;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  // Is the fill volume of machine A normally distributed with mean 5.5 and standard deviation 0.5?
  const auto a = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());

  const auto r = hyp::kstest(a, 5.5, 0.5);
  out.writeln("D", r.stat, " (D+", r.d_plus, " D-", r.d_minus, ") p", r.pvalue, r.reject ? " -> reject" : " -> keep", " critical", r.critical);

  const char* names[] = {"Auto", "Exact", "Approximate"};
  const hyp::PValueMethod methods[] = {hyp::PValueMethod::Auto, hyp::PValueMethod::Exact, hyp::PValueMethod::Approximate};
  for (int i = 0; i < 3; ++i) {
    hyp::TestOptions options;
    options.method = methods[i];
    out.writeln(names[i], hyp::kstest(a, 5.5, 0.5, options).pvalue);
  }

  // One-sided versions: Right looks only at D+ (the sample's distribution function above the proposed one),
  // Left only at D- (below it).
  hyp::TestOptions right;
  right.tail = hyp::Tail::Right;
  hyp::TestOptions left;
  left.tail = hyp::Tail::Left;
  out.writeln("one-sided: Right p", hyp::kstest(a, 5.5, 0.5, right).pvalue, " Left p", hyp::kstest(a, 5.5, 0.5, left).pvalue);
}
```

Output

```text
D 0.2257  (D+ 0.0808  D- 0.2257 ) p 0.7316  -> keep  critical 0.4546
Auto 0.7316
Exact 0.7316
Approximate 0.7534
one-sided: Right p 0.8609  Left p 0.3852
```

The largest gap between machine A's empirical distribution function and the normal distribution with mean 5.5 and standard deviation 0.5 is D = 0.2257, entirely on the D⁻ side (D⁺ is only 0.0808). The p-value is 0.7316, the critical value 0.4546, so the test keeps the null hypothesis; with 8 observations only a gap of about 0.45 would be convincing. The three p-value rows are the methods: `Auto` and `Exact` agree (0.7316; up to 140 observations the exact distribution is used), and the Kolmogorov approximation `Approximate` gives 0.7534, which is why the exact value matters for small samples. The last line gives the one-sided p-values: `Right` looks only at D⁺ and gives 0.8609, `Left` looks only at D⁻ and gives 0.3852.

## Two samples

`kstest2(x, y)` asks whether two samples come from the same distribution without saying which. Its statistic is the largest gap between the two empirical distribution functions.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace hyp = pluss::subsp::stats::inference::hypothesis;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const auto a = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());
  const auto b = pluss::make_series(std::vector<double>{4.8, 5.2, 5.0, 4.7, 5.6, 4.9, 5.1, 4.6, 5.3}, {9}, as_f64());

  const auto r = hyp::kstest2(a, b);
  out.writeln("machine A vs B: D", r.stat, " (D+", r.d_plus, " D-", r.d_minus, ") p", r.pvalue, r.reject ? " -> reject" : " -> keep");

  // The test compares whole distributions, so it sees differences in spread as well as in centre. Same centre, different spread:
  const auto cfg = as_f64();
  const auto narrow = pluss::series::randn({60}, cfg, 21);
  const auto wide = pluss::series::randn({60}, cfg, 22).mul(2.5);
  out.writeln("same centre, spreads 1 and 2.5: KS p", hyp::kstest2(narrow, wide).pvalue, " t-test p", hyp::ttest2(narrow, wide).pvalue);

  // Repeated values are handled by evaluating both distribution functions at the pooled values.
  const auto x = pluss::make_series(std::vector<double>{3, 5, 5, 7, 8, 8, 8, 10, 12, 12, 14, 15}, {12}, cfg);
  const auto y = pluss::make_series(std::vector<double>{4, 5, 6, 8, 8, 9, 11, 11, 13, 13, 13, 16, 18, 20}, {14}, cfg);
  const auto ties = hyp::kstest2(x, y);
  out.writeln("with ties: D", ties.stat, " p", ties.pvalue);
}
```

Output

```text
machine A vs B: D 0.5278  (D+ 0.0000  D- 0.5278 ) p 0.1423  -> keep
same centre, spreads 1 and 2.5: KS p 0.0003  t-test p 0.1707
with ties: D 0.2619  p 0.6844
```

Machines A and B give D = 0.5278 (all of it D⁻) and p = 0.1423, so the test keeps the null hypothesis even though the t-test on the same data rejects (p = 0.0088, Chapter 1). With 8 and 9 observations the test looks at the whole distribution and so has less power than a test aimed at the mean. The second line shows what it is for: two samples of 60 with the same centre and standard deviations 1 and 2.5. The t-test sees nothing (p = 0.1707) and the Kolmogorov–Smirnov test rejects firmly (p = 0.0003). The third line compares samples with repeated values: both distribution functions are evaluated at the pooled values, so a tie is counted at once and not one observation at a time, giving D = 0.2619 and an exact p-value of 0.6844.

## Normality: the Jarque–Bera test

`jbtest(x)` measures how far the sample skewness and kurtosis are from the values of a normal distribution (0 and 3) and combines them into the statistic JB = n/6 · (S² + (K − 3)²/4). It needs no parameters.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace hyp = pluss::subsp::stats::inference::hypothesis;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  namespace distr = pluss::subsp::stats::distr;
  const auto cfg = as_f64();
  const int n = 200;
  const auto normal = pluss::series::randn({n}, cfg, 31);
  const auto skewed = pluss::series::randn({n}, cfg, 32).square();                 // chi-squared with one degree of freedom
  const auto heavy = distr::StudentT{3.0, 0.0, 1.0}.sample({n}, cfg, 33);        // heavy tails, symmetric

  const char* names[] = {"normal", "skewed", "heavy-tailed"};
  const pluss::Series samples[] = {normal, skewed, heavy};
  for (int i = 0; i < 3; ++i) {
    const auto r = hyp::jbtest(samples[i]);
    out.writeln(names[i], " skewness", r.skewness, " kurtosis", r.kurtosis, " JB", r.stat, " p", r.pvalue, r.reject ? " -> reject" : " -> keep");
  }
}
```

Output

```text
normal  skewness 0.1655  kurtosis 3.3637  JB 2.0153  p 0.3121  -> keep
skewed  skewness 2.4012  kurtosis 9.6191  JB 557.3073  p 0.0000  -> reject
heavy-tailed  skewness -0.8449  kurtosis 6.3158  JB 115.4137  p 0.0000  -> reject
```

The normal sample of 200 has skewness 0.1655 and kurtosis 3.3637, giving JB = 2.0153 and p = 0.3121: no evidence against normality. The sample of squared normals (a chi-squared distribution with one degree of freedom) is strongly right-skewed: skewness 2.4012, kurtosis 9.6191, JB = 557.3, p ≈ 0. The heavy-tailed sample (Student's t with 3 degrees of freedom) is symmetric in theory but has a few extreme values: kurtosis 6.3158, JB = 115.4, p ≈ 0. The statistic reports kurtosis as the raw fourth moment ratio, so a normal distribution has kurtosis 3, not 0.

## How the tests behave on small samples

The simulation draws 2000 normal samples of 25 observations, so every null hypothesis is true, and counts the share each test rejects at the 5% level.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>
#include <cmath>

namespace hyp = pluss::subsp::stats::inference::hypothesis;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));

  // Size of the normality tests when the sample is small, by simulation: 2000 normal samples of 25 observations.
  // The Jarque-Bera p-value comes from the simulated finite-sample distribution; the chi-squared(2) limit would
  // reject at 5.99 (5%). The Kolmogorov-Smirnov test is run twice: against the TRUE parameters, and against the sample's own.
  // Seeds are spaced far apart: consecutive seeds give correlated streams, which would distort a simulation.
  const auto cfg = as_f64();
  const int reps = 2000, n = 25;
  int jb_finite = 0, jb_limit = 0, ks_true = 0, ks_fitted = 0;
  for (int r = 0; r < reps; ++r) {
    const auto x = pluss::series::randn({n}, cfg, 7919 * r + 7000);
    const auto jb = hyp::jbtest(x);
    jb_finite += jb.reject;
    jb_limit += jb.stat > 5.991464547107979;
    ks_true += hyp::kstest(x, 0.0, 1.0).reject;
    const auto fit = hyp::ttest(x, 0.0);
    const double sd = fit.stderr * std::sqrt(static_cast<double>(n));
    ks_fitted += hyp::kstest(x, fit.mean, sd).reject;
  }
  out.writeln("Jarque-Bera, finite-sample p-value: share rejected", static_cast<double>(jb_finite) / reps);
  out.writeln("Jarque-Bera, chi-squared limit:     share rejected", static_cast<double>(jb_limit) / reps);
  out.writeln("Kolmogorov-Smirnov, true parameters:   share rejected", static_cast<double>(ks_true) / reps);
  out.writeln("Kolmogorov-Smirnov, fitted parameters: share rejected", static_cast<double>(ks_fitted) / reps);
}
```

Output

```text
Jarque-Bera, finite-sample p-value: share rejected 0.043
Jarque-Bera, chi-squared limit:     share rejected 0.028
Kolmogorov-Smirnov, true parameters:   share rejected 0.044
Kolmogorov-Smirnov, fitted parameters: share rejected 0.001
```

`jbtest` rejects in 0.043 of the samples, close to the 5% it promises. The p-value comes from the simulated distribution of JB for each sample size: the usual chi-squared(2) limit is a poor guide below a few hundred observations, and if the limit were used the test would reject in only 0.028 of the samples, protecting the level too strictly and losing power. The Kolmogorov–Smirnov test against the true parameters rejects in 0.044. Against the sample's own mean and standard deviation it rejects in 0.001 of the samples, almost never: the fitted distribution is chosen to resemble the data, so the gap is always small and the test's critical values, which assume fixed parameters, are far too lenient.

## Decisions and pitfalls

- **Fix the parameters before the data.** `kstest` is valid for a completely specified distribution. With parameters estimated from the same sample it hardly ever rejects (0.001 above); use `jbtest` for normality.
- **`jbtest` is a test of normality only**, and a test of skewness and kurtosis only: a distribution that matches the normal in both (a symmetric mixture, for instance) passes.
- **Small samples cannot reject much.** A normality test on 8 observations says little either way; look at a plot as well.
- **A rejection says "not this distribution"**, not how. Check skewness and kurtosis (the fields of `jbtest`) or the gap location in `d_plus` and `d_minus`.
- **The Kolmogorov–Smirnov test is most sensitive near the middle of the distribution** and has less power for differences in the tails.

Where next: [Several groups, and tables](several_groups_and_tables.md) tests several means at once and tables of counts.

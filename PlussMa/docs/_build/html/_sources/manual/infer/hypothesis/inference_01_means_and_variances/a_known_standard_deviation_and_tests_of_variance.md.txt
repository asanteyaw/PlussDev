<!-- generated from a_known_standard_deviation_and_tests_of_variance.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A known standard deviation, and tests of variances

[Back to Means and Variances](../inference_01_means_and_variances.md)

The spread of a measurement is sometimes known from outside the sample, for instance from a machine's specification or a long history of readings. The mean can then be tested with `ztest`, which uses that standard deviation instead of estimating one. The same family of tests also covers the spread itself: `vartest` compares one variance with a stated value and `vartest2` (or `f_ratio`) compares two variances. This page uses the fill volumes of machine A (8 cups) and machine B (9 cups).

## A known standard deviation

`ztest(x, mean0, sigma)` is the same test as `ttest`, with the standard error `sigma / sqrt(n)` and the standard normal distribution in place of Student's t. The program runs both on machine A with a specified standard deviation of 0.5 cl, and then shows how much the answer depends on that number.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace hyp = pluss::subsp::stats::inference::hypothesis;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const auto a = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());

  // The scale of the machine is known from its specification: sigma = 0.5 cl. ztest uses it; ttest estimates it.
  const auto z = hyp::ztest(a, 5.0, 0.5);
  out.writeln("ztest (sigma = 0.5): z", z.stat, " p", z.pvalue, " standard error", z.stderr, " 95% interval", z.ci.pull<double>());

  const auto t = hyp::ttest(a, 5.0);
  out.writeln("ttest (estimated):   t", t.stat, " p", t.pvalue, " standard error", t.stderr, " 95% interval", t.ci.pull<double>());

  // Same data, a wrong 'known' sigma: the answer moves with it.
  for (double sigma : {0.3, 0.5, 0.8, 1.2}) {
    const auto r = hyp::ztest(a, 5.0, sigma);
    out.writeln("sigma", sigma, " z", r.stat, " p", r.pvalue);
  }
}
```

Output

```text
ztest (sigma = 0.5): z 3.4648  p 0.0005  standard error 0.1768  95% interval [5.2660, 5.9590]
ttest (estimated):   t 3.5696  p 0.0091  standard error 0.1716  95% interval [5.2068, 6.0182]
sigma 0.3000  z 5.7747  p 0.0000
sigma 0.5000  z 3.4648  p 0.0005
sigma 0.8000  z 2.1655  p 0.0303
sigma 1.2000  z 1.4437  p 0.1488
```

With σ = 0.5 the standard error is 0.1768 (0.5 divided by √8), the statistic is z = 3.4648 and the p-value is 0.0005, with a 95% interval of [5.2660, 5.9590]. The t-test, which estimates the spread from the data (a sample standard deviation giving a standard error of 0.1716), gives t = 3.5696, p = 0.0091 and a wider interval [5.2068, 6.0182]. The t distribution on 7 degrees of freedom has heavier tails than the normal, which is what pays for not knowing σ.

The loop shows the risk. The sample is the same in every line; only the assumed σ changes. At σ = 0.3 the z statistic is 5.7747, at 0.5 it is 3.4648, at 0.8 it is 2.1655 (p = 0.0303), and at 1.2 it is 1.4437 (p = 0.1488), where the test no longer rejects. The decision flips somewhere between σ = 0.8 and σ = 1.2. A `ztest` is therefore exactly as trustworthy as the σ supplied to it, which has to come from information other than this sample.

## One variance against a stated value

`vartest(x, variance0)` tests whether the variance of `x` equals `variance0`. The statistic is (n − 1) s² / σ₀², referred to a chi-squared distribution with n − 1 degrees of freedom. The result also carries a confidence interval for the variance.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace hyp = pluss::subsp::stats::inference::hypothesis;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const auto a = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());

  // Is the variance of machine A equal to the specified 0.25 cl^2?
  const auto v = hyp::vartest(a, 0.25);
  out.writeln("sample variance", v.sample_variance, " null variance", v.null_variance, " chi-squared", v.stat, " df", v.dof, " p", v.pvalue);
  out.writeln("95% interval for the variance", v.ci.pull<double>());

  // The interval is asymmetric: variance estimates have a skewed distribution.
  const auto ci = v.ci.pull<double>();
  out.writeln("distance below / above the estimate", v.sample_variance - ci[0], ci[1] - v.sample_variance);

  // One-sided: is the variance larger than specified?
  hyp::TestOptions upper;
  upper.tail = hyp::Tail::Right;
  const auto up = hyp::vartest(a, 0.25, upper);
  out.writeln("H1: variance > 0.25: p", up.pvalue, " critical value", up.critical);
}
```

Output

```text
sample variance 0.2355  null variance 0.2500  chi-squared 6.5950  df 7.0000  p 0.9445
95% interval for the variance [0.1030, 0.9757]
distance below / above the estimate 0.1326 0.7401
H1: variance > 0.25: p 0.4722  critical value 14.0671
```

Machine A's sample variance is 0.2355 against a specified 0.25, which gives a chi-squared statistic of 6.5950 on 7 degrees of freedom and a p-value of 0.9445: nothing suggests the variance differs from the specification. The two-sided p-value is twice the smaller tail area; the right-hand tail probability alone is 0.4722 (the `H1: variance > 0.25` line), and 0.9445 is 2 × 0.4722.

The interval for the variance is [0.1030, 0.9757]. It is far from symmetric about the estimate: it extends 0.1326 below 0.2355 and 0.7401 above it, because a variance estimated from eight observations can be too large by a long way but cannot go below zero. With eight observations the data allow the true variance to be anywhere from about 0.10 to nearly 1.0, which is a reminder of how little a variance estimate from a small sample says. The `critical` value printed for the one-sided version, 14.0671, is the point beyond which the statistic would have to fall for a rejection at the 5% level.

## Two variances

`vartest2(x, y)` compares the variances of two independent samples through their ratio, which follows an F distribution with (nx − 1, ny − 1) degrees of freedom under the null hypothesis. `f_ratio(x, y)` is the same test under another name.

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

  // Do the two machines have the same variance? vartest2 and f_ratio are the same F test.
  const auto f = hyp::vartest2(a, b);
  out.writeln("variances", f.var_x, f.var_y, " F", f.stat, " df", f.dof1, f.dof2, " p", f.pvalue);
  out.writeln("95% interval for the ratio var_x / var_y", f.ci.pull<double>());

  const auto g = hyp::f_ratio(a, b);
  out.writeln("f_ratio gives the same statistic:", g.stat == f.stat ? "yes" : "no");

  // The two-sided critical values of F are not symmetric about 1.
  out.writeln("critical value (upper)", f.critical);
}
```

Output

```text
variances 0.2355 0.0994  F 2.3685  df 7.0000 8.0000  p 0.2501
95% interval for the ratio var_x / var_y [0.5230, 11.6042]
f_ratio gives the same statistic: yes
critical value (upper) 4.5286
```

Machine A's variance (0.2355) is 2.4 times machine B's (0.0994), giving F = 2.3685 on (7, 8) degrees of freedom and a p-value of 0.2501: the test cannot tell the two variances apart. The 95% interval for the ratio is [0.5230, 11.6042], which is the more honest statement. The true ratio could be about a half or as large as eleven. With samples of 8 and 9 the data are compatible with equal variances and also with variances differing by an order of magnitude. `f_ratio` returns the same statistic as `vartest2`. The two-sided critical value (upper) is 4.5286; because F is not symmetric about 1, the lower critical value is not simply the reciprocal of the upper one: it is the reciprocal of the upper critical value with the two degrees of freedom swapped.

## How much does non-normality matter?

The t-test is fairly forgiving of non-normal data. The variance tests are not, and the simulation shows it. It draws 2000 pairs of samples of 15 observations from the same distribution, so the null hypothesis is true in every pair, and counts how often the F test rejects at the 5% level.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace hyp = pluss::subsp::stats::inference::hypothesis;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));

  // How often does the F test reject when the two variances ARE equal? Draw 2000 pairs of samples of 15 and count.
  // Seeds are spaced far apart: consecutive seeds give correlated streams, which would distort a simulation.
  const auto cfg = as_f64();
  namespace distr = pluss::subsp::stats::distr;
  const int reps = 2000, n = 15;
  int normal_rejects = 0, heavy_rejects = 0;
  for (int r = 0; r < reps; ++r) {
    const auto x = pluss::series::randn({n}, cfg, 7919 * r + 1);
    const auto y = pluss::series::randn({n}, cfg, 7919 * r + 3);
    normal_rejects += hyp::vartest2(x, y).reject;
    const auto hx = distr::StudentT{4.0, 0.0, 1.0}.sample({n}, cfg, 7919 * r + 5);
    const auto hy = distr::StudentT{4.0, 0.0, 1.0}.sample({n}, cfg, 7919 * r + 7);
    heavy_rejects += hyp::vartest2(hx, hy).reject;
  }
  out.writeln("normal data,        share of false rejections at the 5% level:", static_cast<double>(normal_rejects) / reps);
  out.writeln("heavy-tailed data,  share of false rejections at the 5% level:", static_cast<double>(heavy_rejects) / reps);
}
```

Output

```text
normal data,        share of false rejections at the 5% level: 0.054
heavy-tailed data,  share of false rejections at the 5% level: 0.192
```

With normal data the F test rejects in 0.054 of the pairs, close to the 5% it promises. With heavy-tailed data (Student's t with 4 degrees of freedom, a mild departure from normality) it rejects in 0.192 of the pairs, nearly four times too often. A few extreme observations inflate one sample's variance and the test reads that as a real difference. The simulation uses seeds spaced far apart: consecutive seeds give correlated streams, which would distort a simulation like this.

## Decisions and pitfalls

- **`ztest` needs a σ that does not come from the sample.** If you estimate it from the same data, use `ttest`. If you are unsure of σ, do not use `ztest` and hope; look at how the p-value moves as σ changes.
- **Variance tests are very sensitive to non-normal data.** A rejection from `vartest` or `vartest2` on skewed or heavy-tailed data may be a sign of the shape, not of the variance. Check the distribution first (the tests of a distribution in Chapter 2 help).
- **Intervals for variances are asymmetric and wide.** Do not read them like intervals for a mean; with small samples they are wide enough to be compatible with very different variances.
- **A large p-value for equal variances is not a licence to pool.** With 8 and 9 observations the interval for the variance ratio spans [0.52, 11.60]; prefer Welch's test unless you have outside reasons.
- **The one-sided variance test is not the half of the two-sided one.** The two-sided p-value doubles the smaller tail; the chi-squared and F distributions are not symmetric, so the critical values are not mirror images.

Where next: [One-sided alternatives and the level](one_sided_alternatives_and_the_level.md) covers the `tail` and `alpha` options that every test shares.

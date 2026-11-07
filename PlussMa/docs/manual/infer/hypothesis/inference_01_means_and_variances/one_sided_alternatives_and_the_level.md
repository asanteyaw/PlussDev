<!-- generated from one_sided_alternatives_and_the_level.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# One-sided alternatives and the level

[Back to Means and Variances](../inference_01_means_and_variances.md)

Every test takes two options that change how the result is read: the **tail**, which says which departures from the null hypothesis count as evidence (`Both`, `Right` or `Left`), and the **level** `alpha`, which says how small a p-value has to be to reject. This page works through both on one test, the one-sample t-test of machine A (fill volumes of 8 cups, target 5.0), and finishes with a simulation of what the level and the choice of tail do to the error rates.

## Choosing the tail

A two-sided test (`Tail::Both`, the default) rejects for a mean that is either too high or too low. A one-sided test rejects only for one direction: `Tail::Right` for a mean larger than the target, `Tail::Left` for smaller. The choice has to come from the question, made before looking at the data.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace hyp = pluss::subsp::stats::inference::hypothesis;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const auto a = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());

  // The same data and the same null (mean 5.0), three alternatives.
  const char* names[] = {"two-sided (mean != 5)", "right (mean > 5)", "left (mean < 5)"};
  const hyp::Tail tails[] = {hyp::Tail::Both, hyp::Tail::Right, hyp::Tail::Left};
  for (int i = 0; i < 3; ++i) {
    hyp::TestOptions options;
    options.tail = tails[i];
    const auto r = hyp::ttest(a, 5.0, options);
    out.writeln(names[i], " t", r.stat, " p", r.pvalue, " critical", r.critical, r.reject ? " -> reject" : " -> keep", " interval", r.ci.pull<double>());
  }
}
```

Output

```text
two-sided (mean != 5)  t 3.5696  p 0.0091  critical 2.3646  -> reject  interval [5.2068, 6.0182]
right (mean > 5)  t 3.5696  p 0.0046  critical 1.8946  -> reject  interval [5.2874, inf]
left (mean < 5)  t 3.5696  p 0.9954  critical -1.8946  -> keep  interval [-inf, 5.9376]
```

All three lines share the statistic t = 3.5696 and differ only in what counts as extreme. The two-sided p-value is 0.0091, with critical value 2.3646. The right-sided p-value is half of that, 0.0046, with critical value 1.8946, because all the rejection probability sits in one tail. The left-sided p-value is 0.9954, which is 1 − 0.0046: the sample mean is above the target, which is as far from "smaller than 5.0" as the data can be. The two one-sided p-values always add to 1, and the two-sided p-value is twice the smaller of them.

The intervals follow the tail. The two-sided interval is [5.2068, 6.0182]. The right-sided interval is [5.2874, ∞): a lower bound only, and it is higher than the two-sided lower bound because the whole 5% is spent on one side. The left-sided interval is (−∞, 5.9376], an upper bound only.

## The level

`alpha` is the probability of a false rejection that you accept. The p-value does not depend on it; the critical value, the decision and the interval do.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace hyp = pluss::subsp::stats::inference::hypothesis;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const auto a = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());

  // The p-value does not depend on the level; the decision and the critical value do.
  for (double alpha : {0.10, 0.05, 0.01, 0.001}) {
    hyp::TestOptions options;
    options.alpha = alpha;
    const auto r = hyp::ttest(a, 5.0, options);
    out.writeln("alpha", alpha, " p", r.pvalue, " |t|", std::fabs(r.stat), " critical", r.critical, r.reject ? " -> reject" : " -> keep", " interval", r.ci.pull<double>());
  }
}
```

Output

```text
alpha 0.1000  p 0.0091  |t| 3.5696  critical 1.8946  -> reject  interval [5.2874, 5.9376]
alpha 0.0500  p 0.0091  |t| 3.5696  critical 2.3646  -> reject  interval [5.2068, 6.0182]
alpha 0.0100  p 0.0091  |t| 3.5696  critical 3.4995  -> reject  interval [5.0120, 6.2130]
alpha 0.0010  p 0.0091  |t| 3.5696  critical 5.4079  -> keep  interval [4.6846, 6.5404]
```

The p-value is 0.0091 on every line. At α = 0.10 the critical value is 1.8946, at 0.05 it is 2.3646, at 0.01 it is 3.4995 and at 0.001 it is 5.4079. The statistic 3.5696 exceeds the first three and falls short of the last, so the test rejects at the first three levels and keeps the null hypothesis at 0.001. The interval tells the same story: the 99% interval [5.0120, 6.2130] only just excludes 5.0, and the 99.9% interval [4.6846, 6.5404] contains it. Reject exactly when the p-value is below α, equivalently when |t| exceeds the critical value, equivalently when the interval excludes the null value.

## What the level and the tail do to the error rates

The level is a promise about false rejections, and the tail changes the chance of catching a real effect. The simulation draws 2000 samples of 20 observations from a normal distribution with standard deviation 1 and a mean shifted by 0, 0.25, 0.5, 0.75 or 1.0 from the tested value of 0, and counts the share of samples the test rejects.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace hyp = pluss::subsp::stats::inference::hypothesis;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));

  // Size and power by simulation: 2000 samples of 20 observations from a normal distribution with standard deviation 1.
  // The tested mean is 0; the true mean is shifted by `shift`. The share of rejections is the size (shift 0) or the power.
  // Seeds are spaced far apart: consecutive seeds give correlated streams, which would distort a simulation.
  const auto cfg = as_f64();
  const int reps = 2000, n = 20;
  out.writeln("shift   two-sided   right-sided");
  for (double shift : {0.0, 0.25, 0.5, 0.75, 1.0}) {
    int both = 0, right = 0;
    for (int r = 0; r < reps; ++r) {
      const auto x = pluss::series::randn({n}, cfg, 7919 * r + 100).add(shift);
      hyp::TestOptions two, one;
      one.tail = hyp::Tail::Right;
      both += hyp::ttest(x, 0.0, two).reject;
      right += hyp::ttest(x, 0.0, one).reject;
    }
    out.writeln(shift, static_cast<double>(both) / reps, static_cast<double>(right) / reps);
  }
}
```

Output

```text
shift   two-sided   right-sided
0.000 0.047 0.046
0.250 0.193 0.282
0.500 0.585 0.700
0.750 0.893 0.949
1.000 0.988 0.994
```

With no shift the null hypothesis is true and the share of rejections is the **size** of the test: 0.047 for the two-sided test and 0.046 for the right-sided one, both close to the 5% promised (the simulation's own standard error is about 0.005). As the true mean moves away the share becomes the **power**: 0.193 at a shift of 0.25, 0.585 at 0.5, 0.893 at 0.75 and 0.988 at 1.0 for the two-sided test. The right-sided test is more powerful in the direction it was built for, 0.700 against 0.585 at a shift of 0.5, but it cannot detect a shift in the other direction at all. A one-sided test is a bet on the direction.

## Decisions and pitfalls

- **Choose the tail from the question, before the data.** Switching to a one-sided test after seeing the sign of the effect halves the p-value without earning it and doubles the false-rejection rate.
- **Report the p-value, not just the decision.** The decision depends on α, which is a convention; the p-value lets the reader apply their own.
- **Do not lower α to save a result or raise it to find one.** Decide the level first.
- **A one-sided interval is a bound.** `[5.2874, ∞)` says "at least 5.29", not a range.
- **Size is about the null; power is about the alternative.** A test that rarely rejects may be correctly sized and still useless if the sample is too small to find the effect you care about; with 20 observations a shift of 0.75 standard deviations is found 89% of the time, and a smaller one would often go undetected.

Where next: [Chapter 2](../inference_02_ranks_distributions_and_tables.md) covers tests that do not assume normal data.

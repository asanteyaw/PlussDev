<!-- generated from ttest.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `ttest`

One-sample Student t-test of a mean.

## Signature

```cpp
TTestResult ttest(const pluss::Series& x, double mean0 = 0.0, const TestOptions& options = {}, pluss::Config cfg = {});
```

## What It Does

`ttest` tests whether the mean of the sample `x` equals `mean0`. The statistic is `(mean − mean0) / (s / √n)`, with `s` the sample standard deviation, referred to Student's t distribution with `n − 1` degrees of freedom. The result also carries the standard error and a confidence interval for the mean at level `1 − alpha`; for a one-sided test the interval is one-sided (it extends to ±∞ on the open side).

Non-finite values in `x` are dropped and `effective_n` reports how many observations were used. For a two-sided test, `reject` is equivalent to the interval excluding `mean0` and to `|stat| > critical`.

## Parameters

`x`
: The sample, a vector (or a matrix with one singleton dimension).

`mean0`
: The mean under the null hypothesis.

`options` (`TestOptions`)
: `alpha` (default 0.05), `tail` (`Tail::Both`, `Tail::Right` or `Tail::Left`) and `method` (`PValueMethod::Auto`, `Exact`, `Approximate`; used only by the tests that have both forms).

`cfg`
: the dtype and platform used for the computation; the default is evaluated in double precision on the CPU.

## Returns

A `TTestResult`. Its fields:

| Field | Meaning |
|---|---|
| `stat` | the test statistic |
| `pvalue` | the probability, if the null hypothesis is true, of a statistic at least as extreme as the observed one |
| `critical` | the critical value of the statistic at level `alpha` |
| `reject` | true when `pvalue < alpha` |
| `dof` | degrees of freedom, `n − 1` |
| `mean` | the sample mean |
| `null_mean` | `mean0` |
| `std_err` | the standard error of the mean |
| `ci` | a two-element Series: the confidence interval for the mean |
| `effective_n` | the number of finite observations used |

## Errors

- `alpha` outside (0, 1).
- Fewer than two finite observations.
- A sample variance of zero (all observations equal).
- `x` is not a vector.

## Practical Examples

### Example 1

Test the mean of one sample against a target, and read the statistic, the p-value and the interval.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <limits>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto a = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());

  const auto r = hyp::ttest(a, 5.0);
  std::cout << "mean " << r.mean << ", t " << r.stat << " on " << r.dof << " df, p " << r.pvalue << (r.reject ? " (reject)" : " (keep)") << "\n";
  std::cout << "standard error " << r.std_err << ", critical value " << r.critical << ", observations " << r.effective_n << "\n";
  const auto ci = r.ci.items<double>();
  std::cout << "95% interval [" << ci[0] << ", " << ci[1] << "]\n";
}
```

Output

```text
mean 5.6125, t 3.5696 on 7.0000 df, p 0.0091 (reject)
standard error 0.1716, critical value 2.3646, observations 8
95% interval [5.2068, 6.0182]
```

The mean 5.61 is 3.57 standard errors (0.172) from the target, with 7 degrees of freedom: p 0.0091 rejects, and the 95% interval [5.21, 6.02] does not contain the target. The interval and the test always agree at the same level.

### Example 2

Change the alternative and the level: a one-sided test at the 1% level.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <limits>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto a = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());

  hyp::TestOptions options;
  options.tail = hyp::Tail::Right;   // H1: the mean is greater than 5.0
  options.alpha = 0.01;
  const auto r = hyp::ttest(a, 5.0, options);
  std::cout << "t " << r.stat << ", p " << r.pvalue << ", critical " << r.critical << (r.reject ? " (reject)" : " (keep)") << "\n";
  const auto ci = r.ci.items<double>();
  std::cout << "99% one-sided interval [" << ci[0] << ", " << ci[1] << "]\n";
}
```

Output

```text
t 3.5696, p 0.0046, critical 2.9980 (reject)
99% one-sided interval [5.0981, inf]
```

One-sided at the 1% level: p halves to 0.0046 and the critical value is 3.00, still below the statistic 3.57. The one-sided interval is open on the right, `[5.10, inf]`: it states only a lower bound for the mean.

### Example 3

Missing values are dropped; `effective_n` says how many were used.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <limits>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const auto gappy = pluss::make_series(std::vector<double>{5.1, nan, 6.2, 5.8, 6.0, nan, 5.3, 6.1}, {8}, as_f64());
  const auto r = hyp::ttest(gappy, 5.0);
  std::cout << "observations used " << r.effective_n << ", mean " << r.mean << ", t " << r.stat << " on " << r.dof << " df, p " << r.pvalue << "\n";
}
```

Output

```text
observations used 6, mean 5.7500, t 4.0775 on 5.0000 df, p 0.0096
```

Missing values are dropped, and `effective_n` reports the 6 observations used; the degrees of freedom, 5, follow from that count, not from the length of the input.

## See Also

- [`ttest_paired`](ttest_paired.md)
- [`ttest2`](ttest2.md)
- [`ztest`](ztest.md)
- [`signrank`](../ranks_and_signs/signrank.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="ttest_paired.html">
    <span class="label">Next</span>
    <span class="title">ttest_paired</span>
  </a>
</div>
<!-- apiref:nav:end -->

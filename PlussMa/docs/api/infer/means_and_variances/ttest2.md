<!-- generated from ttest2.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `ttest2`

Two-sample t-test for the means of two independent samples, pooled or Welch.

## Signature

```cpp
TTest2Result ttest2(const pluss::Series& x, const pluss::Series& y, const TTest2Options& options = {}, pluss::Config cfg = {});
```

## What It Does

`ttest2` tests whether two independent samples have the same mean. With `equal_variance = true` (the default) it pools the two sample variances and uses `nx + ny − 2` degrees of freedom. With `equal_variance = false` it uses Welch's statistic, which does not assume equal variances, with the Welch–Satterthwaite approximation to the degrees of freedom (generally not an integer). The confidence interval is for `mean_x − mean_y`.

Welch's test costs little when the variances are equal and protects the level when they are not, so prefer it unless equal variances are justified on grounds outside the data.

## Parameters

`x`
: The first sample.

`y`
: The second sample.

`options` (`TTest2Options`)
: `alpha` (default 0.05), `tail` and `equal_variance` (default `true`, the pooled test).

`cfg`
: the dtype and platform used for the computation; the default is evaluated in double precision on the CPU.

## Returns

A `TTest2Result`. Its fields:

| Field | Meaning |
|---|---|
| `stat` | the test statistic |
| `pvalue` | the probability, if the null hypothesis is true, of a statistic at least as extreme as the observed one |
| `critical` | the critical value of the statistic at level `alpha` |
| `reject` | true when `pvalue < alpha` |
| `dof` | degrees of freedom (pooled: `nx + ny − 2`; Welch: approximate) |
| `mean_x` | the mean of `x` |
| `mean_y` | the mean of `y` |
| `std_err` | the standard error of the difference of the means |
| `equal_variance` | whether the pooled test was used |
| `ci` | the confidence interval for `mean_x − mean_y` |
| `effective_nx` | finite observations used from `x` |
| `effective_ny` | finite observations used from `y` |

## Errors

- Fewer than two finite observations in either sample.
- A sample with zero variance.
- `alpha` outside (0, 1).

## Practical Examples

### Example 1

The default is the pooled test.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto a = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());
  const auto b = pluss::make_series(std::vector<double>{4.8, 5.2, 5.0, 4.7, 5.6, 4.9, 5.1, 4.6, 5.3}, {9}, as_f64());

  const auto r = hyp::ttest2(a, b);
  std::cout << "difference " << r.mean_x - r.mean_y << ", t " << r.stat << " on " << r.dof << " df, p " << r.pvalue << "\n";
  const auto ci = r.ci.items<double>();
  std::cout << "95% interval [" << ci[0] << ", " << ci[1] << "]\n";
}
```

Output

```text
difference 0.5903, t 3.0093 on 15.0000 df, p 0.0088
95% interval [0.1722, 1.0084]
```

The pooled test, which assumes equal variances, gives a difference of 0.59 with t 3.01 on 15 degrees of freedom and p 0.0088; the 95% interval [0.17, 1.01] excludes zero.

### Example 2

Welch's test: set `equal_variance` to false.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto a = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());
  const auto b = pluss::make_series(std::vector<double>{4.8, 5.2, 5.0, 4.7, 5.6, 4.9, 5.1, 4.6, 5.3}, {9}, as_f64());

  hyp::TTest2Options options;
  options.equal_variance = false;
  const auto r = hyp::ttest2(a, b, options);
  std::cout << "t " << r.stat << " on " << r.dof << " df, p " << r.pvalue << ", standard error " << r.std_err << "\n";
  const auto ci = r.ci.items<double>();
  std::cout << "95% interval [" << ci[0] << ", " << ci[1] << "]\n";
}
```

Output

```text
t 2.9334 on 11.7874 df, p 0.0127, standard error 0.2012
95% interval [0.1510, 1.0296]
```

Welch's test does not assume equal variances: the degrees of freedom become fractional, 11.79, the standard error 0.201 and p 0.0127, a little more cautious than the pooled test, and the interval widens to [0.15, 1.03].

### Example 3

One-sided: is A larger than B?

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto a = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());
  const auto b = pluss::make_series(std::vector<double>{4.8, 5.2, 5.0, 4.7, 5.6, 4.9, 5.1, 4.6, 5.3}, {9}, as_f64());

  hyp::TTest2Options options;
  options.equal_variance = false;
  options.tail = hyp::Tail::Right;
  const auto r = hyp::ttest2(a, b, options);
  std::cout << "p " << r.pvalue << ", critical " << r.critical << (r.reject ? " (reject)" : " (keep)") << "\n";
}
```

Output

```text
p 0.0064, critical 1.7850 (reject)
```

The one-sided test of whether A is larger gives p 0.0064 against the critical value 1.785, and rejects.

## See Also

- [`ttest`](ttest.md)
- [`ttest_paired`](ttest_paired.md)
- [`vartest2`](vartest2.md)
- [`ranksum`](../ranks_and_signs/ranksum.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ttest_paired.html">
    <span class="label">Previous</span>
    <span class="title">ttest_paired</span>
  </a>
  <a class="next-link" href="ztest.html">
    <span class="label">Next</span>
    <span class="title">ztest</span>
  </a>
</div>
<!-- apiref:nav:end -->

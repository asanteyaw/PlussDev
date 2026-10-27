<!-- generated from vartest.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `vartest`

Chi-squared test of one variance against a stated value.

## Signature

```cpp
VarTestResult vartest(const pluss::Series& x, double variance0, const TestOptions& options = {}, pluss::Config cfg = {});
```

## What It Does

`vartest` tests whether the variance of `x` equals `variance0`. The statistic is `(n − 1) s² / variance0`, referred to a chi-squared distribution with `n − 1` degrees of freedom. The interval is for the variance and is asymmetric about the estimate. The test is sensitive to non-normal data: heavy tails make it reject too often.

## Parameters

`x`
: The sample.

`variance0`
: The variance under the null hypothesis (positive).

`options` (`TestOptions`)
: `alpha` (default 0.05), `tail` (`Tail::Both`, `Tail::Right` or `Tail::Left`) and `method` (`PValueMethod::Auto`, `Exact`, `Approximate`; used only by the tests that have both forms).

`cfg`
: the dtype and platform used for the computation; the default is evaluated in double precision on the CPU.

## Returns

A `VarTestResult`. Its fields:

| Field | Meaning |
|---|---|
| `stat` | the test statistic |
| `pvalue` | the probability, if the null hypothesis is true, of a statistic at least as extreme as the observed one |
| `critical` | the critical value of the statistic at level `alpha` |
| `reject` | true when `pvalue < alpha` |
| `dof` | `n − 1` |
| `sample_variance` | the sample variance |
| `null_variance` | `variance0` |
| `ci` | the confidence interval for the variance |
| `effective_n` | the number of finite observations used |

## Errors

- `variance0` not positive or not finite.
- Fewer than two finite observations.
- A sample variance of zero.
- `alpha` outside (0, 1).

## Practical Examples

### Example 1

Is the variance of a machine at its specified value?

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto a = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());

  const auto r = hyp::vartest(a, 0.25);
  std::cout << "sample variance " << r.sample_variance << ", chi-squared " << r.stat << " on " << r.dof << " df, p " << r.pvalue << "\n";
  const auto ci = r.ci.pull<double>();
  std::cout << "95% interval [" << ci[0] << ", " << ci[1] << "]\n";
}
```

Output

```text
sample variance 0.2355, chi-squared 6.5950 on 7.0000 df, p 0.9445
95% interval [0.1030, 0.9757]
```

### Example 2

One-sided: is the variance larger than specified?

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto a = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());

  hyp::TestOptions options;
  options.tail = hyp::Tail::Right;
  const auto r = hyp::vartest(a, 0.25, options);
  std::cout << "p " << r.pvalue << ", critical " << r.critical << (r.reject ? " (reject)" : " (keep)") << "\n";
}
```

Output

```text
p 0.4722, critical 14.0671 (keep)
```

## See Also

- [`vartest2`](vartest2.md)
- [`ztest`](ztest.md)
- [`jbtest`](jbtest.md)

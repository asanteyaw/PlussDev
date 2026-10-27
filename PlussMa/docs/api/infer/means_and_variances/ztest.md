<!-- generated from ztest.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `ztest`

One-sample z-test of a mean when the standard deviation is known.

## Signature

```cpp
ZTestResult ztest(const pluss::Series& x, double mean0, double sigma, const TestOptions& options = {}, pluss::Config cfg = {});
```

## What It Does

`ztest` is the one-sample test of a mean with a standard deviation `sigma` that is known from outside the sample. The statistic is `(mean − mean0) / (sigma / √n)` and is referred to the standard normal distribution; the interval for the mean is `mean ± z · sigma / √n`. The test is exactly as reliable as `sigma`: if the value is estimated from the same data, use `ttest`.

## Parameters

`x`
: The sample.

`mean0`
: The mean under the null hypothesis.

`sigma`
: The known standard deviation (positive).

`options` (`TestOptions`)
: `alpha` (default 0.05), `tail` (`Tail::Both`, `Tail::Right` or `Tail::Left`) and `method` (`PValueMethod::Auto`, `Exact`, `Approximate`; used only by the tests that have both forms).

`cfg`
: the dtype and platform used for the computation; the default is evaluated in double precision on the CPU.

## Returns

A `ZTestResult`. Its fields:

| Field | Meaning |
|---|---|
| `stat` | the test statistic |
| `pvalue` | the probability, if the null hypothesis is true, of a statistic at least as extreme as the observed one |
| `critical` | the critical value of the statistic at level `alpha` |
| `reject` | true when `pvalue < alpha` |
| `mean` | the sample mean |
| `null_mean` | `mean0` |
| `sigma` | the standard deviation used |
| `stderr` | `sigma / √n` |
| `ci` | the confidence interval for the mean |
| `effective_n` | the number of finite observations used |

## Errors

- `sigma` not positive or not finite.
- No finite observations.
- `alpha` outside (0, 1).

## Practical Examples

### Example 1

A machine specified to fill with a standard deviation of 0.5 cl.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto a = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());

  const auto r = hyp::ztest(a, 5.0, 0.5);
  std::cout << "mean " << r.mean << ", z " << r.stat << ", p " << r.pvalue << ", standard error " << r.stderr << "\n";
  const auto ci = r.ci.pull<double>();
  std::cout << "95% interval [" << ci[0] << ", " << ci[1] << "]\n";
}
```

Output

```text
mean 5.6125, z 3.4648, p 0.0005, standard error 0.1768
95% interval [5.2660, 5.9590]
```

### Example 2

The answer depends on the assumed sigma.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto a = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());

  for (double sigma : {0.3, 0.5, 0.8, 1.2}) {
    const auto r = hyp::ztest(a, 5.0, sigma);
    std::cout << "sigma " << sigma << ": z " << r.stat << ", p " << r.pvalue << (r.reject ? " (reject)" : " (keep)") << "\n";
  }
}
```

Output

```text
sigma 0.3000: z 5.7747, p 0.0000 (reject)
sigma 0.5000: z 3.4648, p 0.0005 (reject)
sigma 0.8000: z 2.1655, p 0.0303 (reject)
sigma 1.2000: z 1.4437, p 0.1488 (keep)
```

## See Also

- [`ttest`](ttest.md)
- [`vartest`](vartest.md)

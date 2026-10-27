<!-- generated from jbtest.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `jbtest`

Jarque–Bera test of normality, from the skewness and kurtosis.

## Signature

```cpp
JBTestResult jbtest(const pluss::Series& x, double alpha = 0.05, pluss::Config cfg = {});
```

## What It Does

`jbtest` combines the sample skewness `S` and kurtosis `K` into `JB = n/6 · (S² + (K − 3)²/4)`, which is large for data that are skewed or have heavy or light tails. It needs no parameters. The p-value comes from the simulated finite-sample distribution of `JB` under normality for samples of up to 1000 observations (the chi-squared limit with two degrees of freedom is a poor approximation for small samples) and from the limit beyond that; `critical` is the matching critical value at level `alpha`.

The test only looks at skewness and kurtosis: a distribution that matches the normal in both passes. The reported `kurtosis` is the raw fourth-moment ratio, 3 for a normal distribution.

## Parameters

`x`
: The sample (at least four finite observations).

`alpha`
: The level of the test, in (0, 1); default 0.05.

`cfg`
: the dtype and platform used for the computation; the default is evaluated in double precision on the CPU.

## Returns

A `JBTestResult`. Its fields:

| Field | Meaning |
|---|---|
| `stat` | the test statistic |
| `pvalue` | the probability, if the null hypothesis is true, of a statistic at least as extreme as the observed one |
| `critical` | the critical value of the statistic at level `alpha` |
| `reject` | true when `pvalue < alpha` |
| `skewness` | the sample skewness |
| `kurtosis` | the sample kurtosis (3 for a normal distribution) |
| `effective_n` | the number of finite observations used |

## Errors

- Fewer than four finite observations.
- A sample variance too small to compute the moments.
- `alpha` outside (0, 1).

## Practical Examples

### Example 1

A normal, a skewed and a heavy-tailed sample of 200 observations.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  namespace distr = pluss::subsp::stats::distr;
  const auto cfg = as_f64();
  const auto normal = pluss::series::randn({200}, cfg, 31);
  const auto skewed = pluss::series::randn({200}, cfg, 32).square();
  const auto heavy = distr::StudentT{3.0, 0.0, 1.0}.sample({200}, cfg, 33);
  const char* names[] = {"normal", "skewed", "heavy-tailed"};
  const pluss::Series samples[] = {normal, skewed, heavy};
  for (int i = 0; i < 3; ++i) {
    const auto r = hyp::jbtest(samples[i]);
    std::cout << names[i] << ": skewness " << r.skewness << ", kurtosis " << r.kurtosis << ", JB " << r.stat << ", p " << r.pvalue << (r.reject ? " (reject)" : " (keep)") << "\n";
  }
}
```

Output

```text
normal: skewness 0.1655, kurtosis 3.3637, JB 2.0153, p 0.3121 (keep)
skewed: skewness 2.4012, kurtosis 9.6191, JB 557.3073, p 0.0000 (reject)
heavy-tailed: skewness -0.8449, kurtosis 6.3158, JB 115.4137, p 0.0000 (reject)
```

### Example 2

A small sample: the finite-sample critical value is not the chi-squared one.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto a = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());

  const auto r = hyp::jbtest(a);
  std::cout << "n " << r.effective_n << ", JB " << r.stat << ", p " << r.pvalue << ", 5% critical value " << r.critical << " (the chi-squared limit would be 5.9915)\n";
}
```

Output

```text
n 8, JB 0.7251, p 0.4093, 5% critical value 2.0938 (the chi-squared limit would be 5.9915)
```

### Example 3

A stricter level.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto x = pluss::series::randn({60}, as_f64(), 41).square();
  const auto strict = hyp::jbtest(x, 0.01);
  std::cout << "JB " << strict.stat << ", critical " << strict.critical << ", p " << strict.pvalue << (strict.reject ? " (reject)" : " (keep)") << "\n";
}
```

Output

```text
JB 25.2657, critical 12.4959, p 0.0021 (reject)
```

## See Also

- [`kstest`](kstest.md)
- [`vartest`](vartest.md)

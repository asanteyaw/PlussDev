<!-- generated from kstest.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `kstest`

One-sample Kolmogorov–Smirnov test against a completely specified normal distribution.

## Signature

```cpp
KSTestResult kstest(const pluss::Series& x, double mean0 = 0.0, double stddev0 = 1.0, const TestOptions& options = {}, pluss::Config cfg = {});
```

## What It Does

`kstest` measures the largest vertical gap `D` between the empirical distribution function of `x` and the normal distribution function with mean `mean0` and standard deviation `stddev0`. `d_plus` and `d_minus` are the largest gaps in each direction, and `stat` is `D` (or `d_plus` / `d_minus` for the one-sided tests). The distribution must be fixed before the data are seen: with parameters estimated from the same sample the test hardly ever rejects.

The p-value is exact for up to 140 observations and otherwise comes from the Kolmogorov series with a small-sample correction; `TestOptions::method` forces either.

## Parameters

`x`
: The sample.

`mean0`
: The mean of the normal distribution under the null hypothesis.

`stddev0`
: Its standard deviation (positive).

`options` (`TestOptions`)
: `alpha` (default 0.05), `tail` (`Tail::Both`, `Tail::Right` or `Tail::Left`) and `method` (`PValueMethod::Auto`, `Exact`, `Approximate`; used only by the tests that have both forms).

`cfg`
: the dtype and platform used for the computation; the default is evaluated in double precision on the CPU.

## Returns

A `KSTestResult`. Its fields:

| Field | Meaning |
|---|---|
| `stat` | the test statistic |
| `pvalue` | the probability, if the null hypothesis is true, of a statistic at least as extreme as the observed one |
| `critical` | the critical value of the statistic at level `alpha` |
| `reject` | true when `pvalue < alpha` |
| `d_plus` | the largest amount by which the sample distribution function exceeds the stated one |
| `d_minus` | the largest amount by which it falls short |
| `mean0` | the mean used |
| `stddev0` | the standard deviation used |
| `effective_n` | the number of finite observations used |

## Errors

- `stddev0` not positive or not finite.
- No finite observations.
- `alpha` outside (0, 1).

## Practical Examples

### Example 1

Are the fill volumes of a machine normal with mean 5.5 and standard deviation 0.5?

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto a = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());

  const auto r = hyp::kstest(a, 5.5, 0.5);
  std::cout << "D " << r.stat << " (D+ " << r.d_plus << ", D- " << r.d_minus << "), p " << r.pvalue << ", critical " << r.critical << "\n";
}
```

Output

```text
D 0.2257 (D+ 0.0808, D- 0.2257), p 0.7316, critical 0.4546
```

The largest gap between the empirical distribution function and the normal one is D 0.2257, and it comes from the left gap `D-`; the right gap `D+` is only 0.0808. The p-value 0.73 and the critical value 0.4546 both say the fill volumes are consistent with a normal of mean 5.5 and standard deviation 0.5.

### Example 2

Exact and approximate p-values.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto a = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());

  hyp::TestOptions exact, approximate;
  exact.method = hyp::PValueMethod::Exact;
  approximate.method = hyp::PValueMethod::Approximate;
  std::cout << "exact       p " << hyp::kstest(a, 5.5, 0.5, exact).pvalue << "\n";
  std::cout << "approximate p " << hyp::kstest(a, 5.5, 0.5, approximate).pvalue << "\n";
}
```

Output

```text
exact       p 0.7316
approximate p 0.7534
```

For a small sample the exact p-value (0.7316) and the large-sample approximation (0.7534) differ in the second decimal. Both lead to the same decision; the exact one is the default for small samples, and the approximate one is available when it is wanted for comparison.

### Example 3

One-sided tests look at one of the two gaps.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto a = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());

  hyp::TestOptions right, left;
  right.tail = hyp::Tail::Right;
  left.tail = hyp::Tail::Left;
  std::cout << "Right (D+) p " << hyp::kstest(a, 5.5, 0.5, right).pvalue << "\n";
  std::cout << "Left  (D-) p " << hyp::kstest(a, 5.5, 0.5, left).pvalue << "\n";
}
```

Output

```text
Right (D+) p 0.8609
Left  (D-) p 0.3852
```

The one-sided tests look at one gap each: the right gap `D+` gives p 0.8609 and the left gap `D-` gives p 0.3852. The left gap is the larger, so its p-value is the smaller, and it is the one that carries the two-sided result of the previous examples.

## See Also

- [`kstest2`](kstest2.md)
- [`jbtest`](jbtest.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="kstest2.html">
    <span class="label">Next</span>
    <span class="title">kstest2</span>
  </a>
</div>
<!-- apiref:nav:end -->

<!-- generated from kstest2.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `kstest2`

Two-sample Kolmogorov–Smirnov test: do two samples come from the same distribution?

## Signature

```cpp
KSTest2Result kstest2(const pluss::Series& x, const pluss::Series& y, const TestOptions& options = {}, pluss::Config cfg = {});
```

## What It Does

`kstest2` measures the largest gap between the empirical distribution functions of the two samples, each evaluated at the pooled values (so repeated values are counted together). It makes no assumption about the distribution, and it reacts to any difference in shape, spread or location. The p-value is exact when `nx · ny < 10000` and otherwise from the Kolmogorov series; `TestOptions::method` forces either.

## Parameters

`x`
: The first sample.

`y`
: The second sample.

`options` (`TestOptions`)
: `alpha` (default 0.05), `tail` (`Tail::Both`, `Tail::Right` or `Tail::Left`) and `method` (`PValueMethod::Auto`, `Exact`, `Approximate`; used only by the tests that have both forms).

`cfg`
: the dtype and platform used for the computation; the default is evaluated in double precision on the CPU.

## Returns

A `KSTest2Result`. Its fields:

| Field | Meaning |
|---|---|
| `stat` | the test statistic |
| `pvalue` | the probability, if the null hypothesis is true, of a statistic at least as extreme as the observed one |
| `critical` | the critical value of the statistic at level `alpha` |
| `reject` | true when `pvalue < alpha` |
| `d_plus` | the largest amount by which the distribution function of `x` exceeds that of `y` |
| `d_minus` | the largest amount by which it falls short |
| `effective_nx` | finite observations used from `x` |
| `effective_ny` | finite observations used from `y` |

## Errors

- A sample with no finite observation.
- `alpha` outside (0, 1).

## Practical Examples

### Example 1

Two machines.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto a = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());
  const auto b = pluss::make_series(std::vector<double>{4.8, 5.2, 5.0, 4.7, 5.6, 4.9, 5.1, 4.6, 5.3}, {9}, as_f64());

  const auto r = hyp::kstest2(a, b);
  std::cout << "D " << r.stat << " (D+ " << r.d_plus << ", D- " << r.d_minus << "), p " << r.pvalue << "\n";
}
```

Output

```text
D 0.5278 (D+ 0.0000, D- 0.5278), p 0.1423
```

### Example 2

Same centre, different spread: the t-test sees nothing, this test does.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto cfg = as_f64();
  const auto narrow = pluss::series::randn({60}, cfg, 21);
  const auto wide = pluss::series::randn({60}, cfg, 22).mul(2.5);
  std::cout << "Kolmogorov-Smirnov p " << hyp::kstest2(narrow, wide).pvalue << "\n";
  std::cout << "t-test             p " << hyp::ttest2(narrow, wide).pvalue << "\n";
}
```

Output

```text
Kolmogorov-Smirnov p 0.0003
t-test             p 0.1707
```

### Example 3

Repeated values are handled.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto x = pluss::make_series(std::vector<double>{3, 5, 5, 7, 8, 8, 8, 10, 12, 12, 14, 15}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{4, 5, 6, 8, 8, 9, 11, 11, 13, 13, 13, 16, 18, 20}, {14}, as_f64());
  const auto r = hyp::kstest2(x, y);
  std::cout << "D " << r.stat << ", p " << r.pvalue << "\n";
}
```

Output

```text
D 0.2619, p 0.6844
```

## See Also

- [`kstest`](kstest.md)
- [`ranksum`](ranksum.md)
- [`ttest2`](ttest2.md)

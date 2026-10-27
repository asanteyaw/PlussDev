<!-- generated from signtest.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `signtest`

Sign test for a median or for paired differences.

## Signature

```cpp
SignTestResult signtest(const pluss::Series& x, double median0 = 0.0, const TestOptions& options = {}, pluss::Config cfg = {});
SignTestResult signtest(const pluss::Series& x, const pluss::Series& y, const TestOptions& options = {}, pluss::Config cfg = {});
```

## What It Does

`signtest` counts how many differences (from `median0`, or `x − y`) are positive and how many are negative, ignoring zeros and ignoring size. Under the null hypothesis each sign is equally likely, so the number of positive signs is binomial with probability one half, and the p-value is an exact binomial probability. It needs the fewest assumptions of any location test and has the least power.

## Parameters

`x`
: The sample, or the first reading of each pair.

`median0 / y`
: The median under the null hypothesis (default 0), or the second reading of each pair.

`options` (`TestOptions`)
: `alpha` (default 0.05), `tail` (`Tail::Both`, `Tail::Right` or `Tail::Left`) and `method` (`PValueMethod::Auto`, `Exact`, `Approximate`; used only by the tests that have both forms).

`cfg`
: the dtype and platform used for the computation; the default is evaluated in double precision on the CPU.

## Returns

A `SignTestResult`. Its fields:

| Field | Meaning |
|---|---|
| `stat` | the test statistic |
| `pvalue` | the probability, if the null hypothesis is true, of a statistic at least as extreme as the observed one |
| `critical` | the critical value of the statistic at level `alpha` |
| `reject` | true when `pvalue < alpha` |
| `positives` | the number of positive differences |
| `negatives` | the number of negative differences |
| `effective_n` | the number of nonzero finite differences used |

## Errors

- No nonzero finite difference.
- The two samples of a paired test have different lengths.
- `alpha` outside (0, 1).

## Practical Examples

### Example 1

Paired readings.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto first = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());
  const auto second = pluss::make_series(std::vector<double>{5.3, 5.0, 6.3, 6.1, 6.1, 5.8, 5.4, 6.4}, {8}, as_f64());

  const auto r = hyp::signtest(first, second);
  std::cout << "positive " << r.positives << ", negative " << r.negatives << ", p " << r.pvalue << "\n";
}
```

Output

```text
positive 0, negative 8, p 0.0078
```

### Example 2

One sample against a median; the signs of the differences are mixed.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto a = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());

  const auto r = hyp::signtest(a, 5.5);
  std::cout << "positive " << r.positives << ", negative " << r.negatives << ", used " << r.effective_n << ", p " << r.pvalue << "\n";
}
```

Output

```text
positive 4, negative 3, used 7, p 1.0000
```

## See Also

- [`signrank`](signrank.md)
- [`ttest_paired`](ttest_paired.md)

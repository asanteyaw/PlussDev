<!-- generated from signrank.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `signrank`

Wilcoxon signed-rank test for a median or for paired differences.

## Signature

```cpp
SignRankResult signrank(const pluss::Series& x, double median0 = 0.0, const TestOptions& options = {}, pluss::Config cfg = {});
SignRankResult signrank(const pluss::Series& x, const pluss::Series& y, const TestOptions& options = {}, pluss::Config cfg = {});
```

## What It Does

`signrank` takes the differences from `median0` (one sample) or `x − y` (paired samples), drops zero differences, ranks the absolute differences (ties share the average rank) and adds up the ranks of the positive differences (`wplus`) and of the negative ones (`wminus`). It uses the sizes of the differences as well as their signs, so it is more powerful than `signtest` when sizes carry information.

The p-value is exact for at most 15 nonzero differences and otherwise from the normal approximation with a variance adjusted for ties and no continuity correction; `TestOptions::method` forces either. `stat` is the z score.

## Parameters

`x`
: The sample, or the first reading of each pair.

`median0 / y`
: The median under the null hypothesis (default 0), or the second reading of each pair (same length as `x`).

`options` (`TestOptions`)
: `alpha` (default 0.05), `tail` (`Tail::Both`, `Tail::Right` or `Tail::Left`) and `method` (`PValueMethod::Auto`, `Exact`, `Approximate`; used only by the tests that have both forms).

`cfg`
: the dtype and platform used for the computation; the default is evaluated in double precision on the CPU.

## Returns

A `SignRankResult`. Its fields:

| Field | Meaning |
|---|---|
| `stat` | the test statistic |
| `pvalue` | the probability, if the null hypothesis is true, of a statistic at least as extreme as the observed one |
| `critical` | the critical value of the statistic at level `alpha` |
| `reject` | true when `pvalue < alpha` |
| `wplus` | the sum of the ranks of the positive differences |
| `wminus` | the sum of the ranks of the negative differences |
| `effective_n` | the number of nonzero finite differences used |

## Errors

- No nonzero finite difference.
- The two samples of a paired test have different lengths.
- `alpha` outside (0, 1).

## Practical Examples

### Example 1

Paired readings: every cup reads higher on the second scale.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto first = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());
  const auto second = pluss::make_series(std::vector<double>{5.3, 5.0, 6.3, 6.1, 6.1, 5.8, 5.4, 6.4}, {8}, as_f64());

  const auto r = hyp::signrank(first, second);
  std::cout << "W+ " << r.wplus << ", W- " << r.wminus << ", z " << r.stat << ", p " << r.pvalue << ", pairs used " << r.effective_n << "\n";
}
```

Output

```text
W+ 0.0000, W- 36.0000, z -2.5361, p 0.0078, pairs used 8
```

### Example 2

One sample against a hypothesised median.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto a = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());

  const auto r = hyp::signrank(a, 5.0);
  std::cout << "W+ " << r.wplus << ", W- " << r.wminus << ", p " << r.pvalue << "\n";
}
```

Output

```text
W+ 34.5000, W- 1.5000, p 0.0234
```

### Example 3

Exact and approximate p-values compared.

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
  std::cout << "exact       p " << hyp::signrank(a, 5.5, exact).pvalue << "\n";
  std::cout << "approximate p " << hyp::signrank(a, 5.5, approximate).pvalue << "\n";
}
```

Output

```text
exact       p 0.5000
approximate p 0.4461
```

## See Also

- [`signtest`](signtest.md)
- [`ttest_paired`](ttest_paired.md)
- [`ranksum`](ranksum.md)

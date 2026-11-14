<!-- generated from chisq_gof.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `chisq_gof`

Chi-squared goodness-of-fit test of observed counts against expected counts.

## Signature

```cpp
ChiGofResult chisq_gof(const pluss::Series& observed, const pluss::Series& expected = pluss::series::undefined(), const ChiGofOptions& options = {}, pluss::Config cfg = {});
```

## What It Does

`chisq_gof` compares observed category counts `O` with expected counts `E` through the statistic `Σ (O − E)² / E`, which follows a chi-squared distribution with `k − 1 − ddof` degrees of freedom, `k` being the number of categories and `ddof` the number of parameters estimated from the data. With no `expected` argument, all categories are taken to be equally likely. With `normalize_expected` (the default) the expected values are treated as weights and rescaled to the observed total, so proportions can be passed instead of counts.

The approximation needs expected counts that are not tiny; a common rule is that all be at least 5.

## Parameters

`observed`
: The observed counts, one per category (non-negative, finite, at least two categories).

`expected`
: The expected counts or weights, one per category and positive; omit (or pass an undefined Series) for equal probabilities.

`options` (`ChiGofOptions`)
: `alpha` (default 0.05), `ddof` (parameters estimated from the data; default 0) and `normalize_expected` (default `true`).

`cfg`
: the dtype and platform used for the computation; the default is evaluated in double precision on the CPU.

## Returns

A `ChiGofResult`. Its fields:

| Field | Meaning |
|---|---|
| `stat` | the test statistic |
| `pvalue` | the probability, if the null hypothesis is true, of a statistic at least as extreme as the observed one |
| `critical` | the critical value of the statistic at level `alpha` |
| `reject` | true when `pvalue < alpha` |
| `dof` | degrees of freedom, `k − 1 − ddof` |
| `total_count` | the total of the observed counts |
| `bins` | the number of categories |
| `expected` | the expected counts used, after rescaling |

## Errors

- Fewer than two categories.
- `observed` not finite or negative.
- `expected` of a different length, not finite or not positive.
- Non-positive degrees of freedom (`ddof` too large).
- `alpha` outside (0, 1).

## Practical Examples

### Example 1

Is a die fair? 120 rolls.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto rolls = pluss::make_series(std::vector<double>{13, 17, 24, 22, 19, 25}, {6}, as_f64());
  const auto r = hyp::chisq_gof(rolls);
  std::cout << "chi-squared " << r.stat << " on " << r.dof << " df, p " << r.pvalue << ", critical " << r.critical << "\n";
  std::cout << "each expected count " << r.expected.items<double>()[0] << " of " << r.total_count << " rolls\n";
}
```

Output

```text
chi-squared 5.2000 on 5.0000 df, p 0.3920, critical 11.0705
each expected count 20.0000 of 120.0000 rolls
```

For 120 rolls of a fair die each face is expected 20 times. The chi-squared statistic 5.20 on 5 degrees of freedom is below the critical value 11.07, and p 0.39 gives no evidence against a fair die.

### Example 2

Expected proportions instead of equal probabilities.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto rolls = pluss::make_series(std::vector<double>{13, 17, 24, 22, 19, 25}, {6}, as_f64());
  const auto weights = pluss::make_series(std::vector<double>{1, 1, 1, 1, 1, 2}, {6}, as_f64());
  const auto r = hyp::chisq_gof(rolls, weights);
  const auto expected = r.expected.items<double>();
  std::cout << "expected counts " << expected[0] << " (five faces) and " << expected[5] << " (the sixth)\n";
  std::cout << "chi-squared " << r.stat << ", p " << r.pvalue << "\n";
}
```

Output

```text
expected counts 17.1429 (five faces) and 34.2857 (the sixth)
chi-squared 7.8375, p 0.1654
```

With expected proportions instead of equal probabilities, five faces are expected 17.14 times and the sixth 34.29 times; the statistic 7.84 on 5 degrees of freedom gives p 0.17. The expected counts are computed from the proportions and the total, so they always add up to the number of observations.

### Example 3

Each estimated parameter costs a degree of freedom.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto rolls = pluss::make_series(std::vector<double>{13, 17, 24, 22, 19, 25}, {6}, as_f64());
  hyp::ChiGofOptions options;
  options.ddof = 1;
  const auto r = hyp::chisq_gof(rolls, pluss::series::undefined(), options);
  std::cout << "degrees of freedom " << r.dof << ", p " << r.pvalue << ", critical " << r.critical << "\n";
}
```

Output

```text
degrees of freedom 4.0000, p 0.2674, critical 9.4877
```

Each parameter estimated from the data removes one degree of freedom: with one estimated parameter the test has 4 instead of 5, the critical value falls to 9.49, and p is 0.27. Forgetting this makes the test too lenient.

## See Also

- [`chisq_indep`](chisq_indep.md)
- [`kstest`](../distributions/kstest.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="anova_1w.html">
    <span class="label">Previous</span>
    <span class="title">anova_1w</span>
  </a>
  <a class="next-link" href="chisq_indep.html">
    <span class="label">Next</span>
    <span class="title">chisq_indep</span>
  </a>
</div>
<!-- apiref:nav:end -->

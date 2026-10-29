<!-- generated from chisq_indep.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `chisq_indep`

Chi-squared test of independence for a table of counts.

## Signature

```cpp
ChiIndepResult chisq_indep(const pluss::Series& contingency, double alpha = 0.05, pluss::Config cfg = {});
```

## What It Does

`chisq_indep` tests whether the row and column classifications of a table of counts are independent. The expected count of each cell is `(row total × column total) / grand total`; the statistic `Σ (O − E)² / E` follows a chi-squared distribution with `(rows − 1)(columns − 1)` degrees of freedom. The expected table is returned so that you can see where the observed counts depart from independence, and so that you can check that the expected counts are large enough (a common rule is at least 5 in every cell). The test uses counts, not percentages.

## Parameters

`contingency`
: A matrix of non-negative counts with at least two rows and two columns.

`alpha`
: The level of the test, in (0, 1); default 0.05.

`cfg`
: the dtype and platform used for the computation; the default is evaluated in double precision on the CPU.

## Returns

A `ChiIndepResult`. Its fields:

| Field | Meaning |
|---|---|
| `stat` | the test statistic |
| `pvalue` | the probability, if the null hypothesis is true, of a statistic at least as extreme as the observed one |
| `critical` | the critical value of the statistic at level `alpha` |
| `reject` | true when `pvalue < alpha` |
| `dof` | `(rows − 1)(columns − 1)` |
| `rows` | the number of rows |
| `cols` | the number of columns |
| `expected` | the matrix of expected counts under independence |

## Errors

- A table that is not two-dimensional or is smaller than 2 × 2.
- Counts that are not finite or are negative.
- A table with total count zero.
- `alpha` outside (0, 1).

## Practical Examples

### Example 1

Preferred drink by age band, 89 people.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto counts = pluss::make_series(std::vector<double>{12, 8, 15,
                                                            20, 25, 9}, {2, 3}, as_f64());
  const auto r = hyp::chisq_indep(counts);
  std::cout << "chi-squared " << r.stat << " on " << r.dof << " df, p " << r.pvalue << ", critical " << r.critical << "\n";
}
```

Output

```text
chi-squared 8.5930 on 2.0000 df, p 0.0136, critical 5.9915
```

For the drink and age table of 89 people the chi-squared statistic 8.59 on 2 degrees of freedom exceeds the 5% critical value 5.99, and p 0.0136 rejects independence: the preferred drink depends on the age band.

### Example 2

The expected counts show where the table departs from independence.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto counts = pluss::make_series(std::vector<double>{12, 8, 15,
                                                            20, 25, 9}, {2, 3}, as_f64());
  const auto r = hyp::chisq_indep(counts);
  const auto top = r.expected.select(0, 0).pull<double>();
  const auto bottom = r.expected.select(0, 1).pull<double>();
  std::cout << "row 1 expected " << top[0] << ", " << top[1] << ", " << top[2] << "\n";
  std::cout << "row 2 expected " << bottom[0] << ", " << bottom[1] << ", " << bottom[2] << "\n";
  std::cout << "table " << r.rows << " x " << r.cols << "\n";
}
```

Output

```text
row 1 expected 12.5843, 12.9775, 9.4382
row 2 expected 19.4157, 20.0225, 14.5618
table 2 x 3
```

The expected counts under independence are the row total times the column total over the grand total; comparing them with the observed table shows which cells depart most. The table printed is 2 by 3, and the first row is expected 12.58, 12.98 and 9.44.

### Example 3

A stricter level.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto counts = pluss::make_series(std::vector<double>{12, 8, 15, 20, 25, 9}, {2, 3}, as_f64());
  const auto r = hyp::chisq_indep(counts, 0.01);
  std::cout << "p " << r.pvalue << ", critical " << r.critical << (r.reject ? " (reject)" : " (keep)") << "\n";
}
```

Output

```text
p 0.0136, critical 9.2103 (keep)
```

At the 1% level the critical value is 9.21, above the statistic 8.59, so the same table that was rejected at 5% is kept here (p is unchanged, 0.0136). The decision depends on the level, which is why the level is a parameter and the p-value is reported alongside.

## See Also

- [`chisq_gof`](chisq_gof.md)
- [`anova_1w`](anova_1w.md)

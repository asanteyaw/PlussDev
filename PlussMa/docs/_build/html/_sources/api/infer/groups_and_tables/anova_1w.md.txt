<!-- generated from anova_1w.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `anova_1w`

One-way analysis of variance: do several group means agree?

## Signature

```cpp
Anova1WResult anova_1w(const pluss::Series& y, const pluss::Series& groups, const Anova1WOptions& options = {}, pluss::Config cfg = {});
```

## What It Does

`anova_1w` splits the variation of `y` into the part between the group means and the part within groups. The ratio of their mean squares is the F statistic, referred to an F distribution with `groups − 1` and `n − groups` degrees of freedom. A rejection says that the group means are not all equal; it does not say which differ. The test assumes roughly normal data with similar spreads in the groups.

`groups` holds one non-negative integer label per observation, in a Series of the same length as `y` (labels need not be consecutive). Non-finite values in `y` are dropped unless `drop_missing` is false, in which case they are an error.

## Parameters

`y`
: The observations.

`groups`
: Integer group labels, one per observation.

`options` (`Anova1WOptions`)
: `alpha` (default 0.05) and `drop_missing` (default `true`).

`cfg`
: the dtype and platform used for the computation; the default is evaluated in double precision on the CPU.

## Returns

A `Anova1WResult`. Its fields:

| Field | Meaning |
|---|---|
| `stat` | the test statistic |
| `pvalue` | the probability, if the null hypothesis is true, of a statistic at least as extreme as the observed one |
| `critical` | the critical value of the statistic at level `alpha` |
| `reject` | true when `pvalue < alpha` |
| `dof_between` | `groups − 1` |
| `dof_within` | `n − groups` |
| `ss_between` | sum of squares between group means |
| `ss_within` | sum of squares within groups |
| `ms_between` | mean square between |
| `ms_within` | mean square within |
| `groups` | the number of non-empty groups |
| `effective_n` | the number of observations used |
| `group_means` | the mean of each group |
| `group_counts` | the number of observations in each group |

## Errors

- `y` and `groups` have different lengths.
- Fewer than two groups or fewer than two finite observations.
- Group labels that are not non-negative integers.
- `effective_n` not larger than the number of groups.
- Non-finite values with `drop_missing = false`.

## Practical Examples

### Example 1

Strength of a component from three suppliers.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto strength = pluss::make_series(std::vector<double>{4.6, 5.9, 4.2, 5.6, 5.4, 5.1, 4.5,
                                                              6.4, 5.3, 6.6, 5.6, 6.1, 5.8, 7.0, 5.2, 5.6,
                                                              6.0, 7.2, 5.5, 7.4, 5.7, 7.1, 7.9, 6.3}, {24}, as_f64());
  const auto supplier = pluss::make_series(std::vector<double>{1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3}, {24}, as_f64());
  const auto r = hyp::anova_1w(strength, supplier);
  std::cout << "F " << r.stat << " on (" << r.dof_between << ", " << r.dof_within << ") df, p " << r.pvalue << ", critical " << r.critical << "\n";
  std::cout << "SS between " << r.ss_between << ", SS within " << r.ss_within << ", MS between " << r.ms_between << ", MS within " << r.ms_within << "\n";
}
```

Output

```text
F 9.2697 on (2.0000, 21.0000) df, p 0.0013, critical 3.4668
SS between 9.5152, SS within 10.7781, MS between 4.7576, MS within 0.5132
```

### Example 2

The group means and counts.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto y = pluss::make_series(std::vector<double>{4.6, 5.9, 4.2, 5.6, 6.4, 5.3, 6.6, 6.0, 7.2, 5.5}, {10}, as_f64());
  const auto g = pluss::make_series(std::vector<double>{10, 10, 10, 10, 20, 20, 20, 30, 30, 30}, {10}, as_f64());
  const auto r = hyp::anova_1w(y, g);
  const auto means = r.group_means.pull<double>();
  const auto counts = r.group_counts.pull<double>();
  for (int i = 0; i < r.groups; ++i) std::cout << "group " << i << ": n " << counts[i] << ", mean " << means[i] << "\n";
  std::cout << "F " << r.stat << ", p " << r.pvalue << "\n";
}
```

Output

```text
group 0: n 4.0000, mean 5.0750
group 1: n 3.0000, mean 6.1000
group 2: n 3.0000, mean 6.2333
F 2.2684, p 0.1740
```

## See Also

- [`chisq_gof`](chisq_gof.md)
- [`ttest2`](ttest2.md)
- [`ranksum`](ranksum.md)

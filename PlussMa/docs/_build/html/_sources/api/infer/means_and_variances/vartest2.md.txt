<!-- generated from vartest2.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `vartest2`

F test for the equality of two variances.

## Signature

```cpp
VarTest2Result vartest2(const pluss::Series& x, const pluss::Series& y, const TestOptions& options = {}, pluss::Config cfg = {});
```

## What It Does

`vartest2` compares the variances of two independent samples through their ratio `var_x / var_y`, which follows an F distribution with `(nx − 1, ny − 1)` degrees of freedom under the null hypothesis. The interval is for the variance ratio. `f_ratio` is the same test under another name. Like `vartest` it is very sensitive to non-normal data.

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

A `VarTest2Result`. Its fields:

| Field | Meaning |
|---|---|
| `stat` | the test statistic |
| `pvalue` | the probability, if the null hypothesis is true, of a statistic at least as extreme as the observed one |
| `critical` | the critical value of the statistic at level `alpha` |
| `reject` | true when `pvalue < alpha` |
| `dof1` | `nx − 1` |
| `dof2` | `ny − 1` |
| `var_x` | the variance of `x` |
| `var_y` | the variance of `y` |
| `ci` | the confidence interval for `var_x / var_y` |
| `effective_nx` | finite observations used from `x` |
| `effective_ny` | finite observations used from `y` |

## Errors

- Fewer than two finite observations in either sample.
- A sample with zero variance.
- `alpha` outside (0, 1).

## Practical Examples

### Example 1

Do two machines have the same variance?

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto a = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());
  const auto b = pluss::make_series(std::vector<double>{4.8, 5.2, 5.0, 4.7, 5.6, 4.9, 5.1, 4.6, 5.3}, {9}, as_f64());

  const auto r = hyp::vartest2(a, b);
  std::cout << "variances " << r.var_x << " and " << r.var_y << ", F " << r.stat << " on (" << r.dof1 << ", " << r.dof2 << ") df, p " << r.pvalue << "\n";
  const auto ci = r.ci.pull<double>();
  std::cout << "95% interval [" << ci[0] << ", " << ci[1] << "]\n";
}
```

Output

```text
variances 0.2355 and 0.0994, F 2.3685 on (7.0000, 8.0000) df, p 0.2501
95% interval [0.5230, 11.6042]
```

### Example 2

The interval for the ratio shows how much the data leave open.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto a = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());
  const auto b = pluss::make_series(std::vector<double>{4.8, 5.2, 5.0, 4.7, 5.6, 4.9, 5.1, 4.6, 5.3}, {9}, as_f64());

  hyp::TestOptions options;
  options.alpha = 0.10;
  const auto r = hyp::vartest2(a, b, options);
  const auto ci = r.ci.pull<double>();
  std::cout << "90% interval for the variance ratio [" << ci[0] << ", " << ci[1] << "], upper critical value " << r.critical << "\n";
}
```

Output

```text
90% interval for the variance ratio [0.6766, 8.8244], upper critical value 3.5005
```

## See Also

- [`f_ratio`](f_ratio.md)
- [`vartest`](vartest.md)
- [`ttest2`](ttest2.md)

<!-- generated from f_ratio.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `f_ratio`

The F test for two variances, under its other name.

## Signature

```cpp
VarTest2Result f_ratio(const pluss::Series& x, const pluss::Series& y, const TestOptions& options = {}, pluss::Config cfg = {});
```

## What It Does

`f_ratio` is `vartest2` under another name: it returns the same `VarTest2Result` for the same arguments. See [`vartest2`](vartest2.md) for the statistic and the interpretation.

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
| (same fields as `vartest2`) | see [`vartest2`](vartest2.md) |

## Errors

- The same conditions as `vartest2`.

## Practical Examples

### Example 1

The two names give the same result.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace hyp = pluss::subsp::stats::inference::hypothesis;

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto a = pluss::make_series(std::vector<double>{5.1, 4.9, 6.2, 5.8, 6.0, 5.5, 5.3, 6.1}, {8}, as_f64());
  const auto b = pluss::make_series(std::vector<double>{4.8, 5.2, 5.0, 4.7, 5.6, 4.9, 5.1, 4.6, 5.3}, {9}, as_f64());

  const auto f = hyp::f_ratio(a, b);
  const auto v = hyp::vartest2(a, b);
  std::cout << "f_ratio  F " << f.stat << ", p " << f.pvalue << "\n";
  std::cout << "vartest2 F " << v.stat << ", p " << v.pvalue << "\n";
}
```

Output

```text
f_ratio  F 2.3685, p 0.2501
vartest2 F 2.3685, p 0.2501
```

### Example 2

One-sided: is the variance of A larger than that of B?

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
  options.tail = hyp::Tail::Right;
  const auto r = hyp::f_ratio(a, b, options);
  std::cout << "F " << r.stat << ", one-sided p " << r.pvalue << ", critical " << r.critical << "\n";
}
```

Output

```text
F 2.3685, one-sided p 0.1251, critical 3.5005
```

## See Also

- [`vartest2`](vartest2.md)
- [`vartest`](vartest.md)

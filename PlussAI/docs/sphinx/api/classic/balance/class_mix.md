# `pluss::ai::classic::class_mix`

Summarizes a label vector's class distribution — counts, proportions,
inverse-frequency weights, and a single imbalance ratio — the shared
starting point every other function in this family reads:
[`resolve_weight_plan`](resolve_weight_plan.md) reads `inverse_frequency`
for `ClassWeightMode::Balanced`; [`plan_balance`](plan_balance.md) reads
`counts`/`labels` to resolve target counts.

## Signature

```cpp
classic::StatusOr<classic::ClassMix> classic::class_mix(const pluss::Series& labels);
```

## Parameters

`labels`
: Must be defined, non-empty, and finite (no `NaN`/`Inf`) — reshaped
  flat and cast to `Float64` internally regardless of its own dtype or
  rank, so a `[n, 1]` or `[1, n]` label tensor works the same as a plain
  `[n]` one.

## Returns

A `StatusOr<ClassMix>` — see [`ClassMix`](ClassMix.md) for every field.
Classes are sorted **ascending** by value — `labels[0]` is always the
smallest distinct value seen, regardless of the order they first appear
in the input. See [Example 3](#example-3).

## Errors

- `labels` undefined — `Status::invalid_argument`.
- `labels` empty (`numel() == 0`) — `Status::invalid_argument`.
- `labels` contains a non-finite value — `Status::invalid_argument`.

## Practical Examples

A three-class imbalanced distribution with every field hand-verified, a
perfectly balanced case (`imbalance_ratio == 1.0`), classes sorted
ascending regardless of input order, and an error roundup.

### Example 1

Three classes, uneven counts — every returned field checked against the
hand-computed value.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // class 0: 6 rows, class 1: 3 rows, class 2: 1 row -- 10 total.
  const pluss::Series labels = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 2.0}, {10}, f64);

  const cls::ClassMix mix = cls::class_mix(labels).value();
  std::cout << "labels = " << mix.labels << "\n";
  std::cout << "counts = " << mix.counts << "\n";
  std::cout << "proportions (expect [0.6, 0.3, 0.1]) = " << mix.proportions << "\n";
  std::cout << "inverse_frequency (expect [10/(3*6), 10/(3*3), 10/(3*1)] = [0.5556, 1.1111, 3.3333]) = " << mix.inverse_frequency << "\n";
  std::cout << "sample_count = " << mix.sample_count << "\n";
  std::cout << "class_count = " << mix.class_count << "\n";
  std::cout << "imbalance_ratio (expect 6/1 = 6.0) = " << mix.imbalance_ratio << "\n";

  return 0;
}
```

Output

```text
labels = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 1, 2]
counts = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[6, 3, 1]
proportions (expect [0.6, 0.3, 0.1]) = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.6, 0.3, 0.1]
inverse_frequency (expect [10/(3*6), 10/(3*3), 10/(3*1)] = [0.5556, 1.1111, 3.3333]) = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.55555556, 1.1111111, 3.3333333]
sample_count = 10
class_count = 3
imbalance_ratio (expect 6/1 = 6.0) = 6
```

### Example 2

A perfectly balanced two-class case: `imbalance_ratio == 1.0` and both
classes get identical `inverse_frequency` weights.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series labels = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);
  const cls::ClassMix mix = cls::class_mix(labels).value();
  std::cout << "imbalance_ratio = " << mix.imbalance_ratio << "\n";
  std::cout << "inverse_frequency = " << mix.inverse_frequency << "\n";

  return 0;
}
```

Output

```text
imbalance_ratio = 1
inverse_frequency = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
```

### Example 3

Classes are sorted ascending by value in the result, regardless of the
order they first appear in `labels`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // 5.0 appears first in the data, but 1.0 is the smaller value.
  const pluss::Series labels = pluss::make_series({5.0, 1.0, 5.0, 3.0, 1.0}, {5}, f64);
  const cls::ClassMix mix = cls::class_mix(labels).value();
  std::cout << "labels (expect [1, 3, 5], sorted ascending) = " << mix.labels << "\n";
  std::cout << "counts (expect [2, 1, 2], matching that order) = " << mix.counts << "\n";

  return 0;
}
```

Output

```text
labels (expect [1, 3, 5], sorted ascending) = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 3, 5]
counts (expect [2, 1, 2], matching that order) = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2, 1, 2]
```

### Example 4

An error roundup: undefined labels, empty labels, and a non-finite
label.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <limits>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  std::cout << "undefined labels ok = " << std::boolalpha << cls::class_mix(pluss::Series::undefined()).ok() << "\n";

  const pluss::Series empty = pluss::make_series(std::vector<double>{}, {0}, f64);
  std::cout << "empty labels ok = " << cls::class_mix(empty).ok() << "\n";

  const double nan = std::numeric_limits<double>::quiet_NaN();
  const pluss::Series with_nan = pluss::make_series({0.0, 1.0, nan}, {3}, f64);
  std::cout << "non-finite label ok = " << cls::class_mix(with_nan).ok() << "\n";

  return 0;
}
```

Output

```text
undefined labels ok = false
empty labels ok = false
non-finite label ok = false
```

## Changelog

- **0.1** — `class_mix`.

## See Also

- [`ClassMix`](ClassMix.md) — the returned type, every field explained.
- [`resolve_weight_plan`](resolve_weight_plan.md) — reads
  `inverse_frequency` for `ClassWeightMode::Balanced`.
- [`plan_balance`](plan_balance.md) — reads `counts`/`labels` to resolve
  target counts.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

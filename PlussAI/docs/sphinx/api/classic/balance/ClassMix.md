# `pluss::ai::classic::ClassMix`

The class-distribution summary shared across this family — produced by
[`class_mix`](class_mix.md), and embedded inside
[`WeightPlan`](resolve_weight_plan.md#returns) and
[`BalancePlan`](BalancePlan.md) (as `.before`/`.after`) so every stage of
weighting or rebalancing can be read back against the distribution it
started or ended with.

## Signature

```cpp
pluss::Series classic::ClassMix::labels;              // Float64, [class_count], sorted ascending
pluss::Series classic::ClassMix::counts;               // Float64, [class_count]
pluss::Series classic::ClassMix::proportions;          // Float64, [class_count]
pluss::Series classic::ClassMix::inverse_frequency;    // Float64, [class_count]
std::int64_t classic::ClassMix::sample_count;
std::int64_t classic::ClassMix::class_count;
double classic::ClassMix::imbalance_ratio;
```

## Construction

Produced only by [`class_mix`](class_mix.md) (directly, or embedded in
[`WeightPlan`](resolve_weight_plan.md#returns)/
[`BalancePlan`](BalancePlan.md)). A plain value type — copy or store it
freely.

## State

`labels` — `pluss::Series`
: Every distinct value seen in the source label vector, sorted
  **ascending** — index `i` here is class `i` in every other field.

`counts` — `pluss::Series`
: How many rows carry each class, in `labels`' own order. Every entry is
  `>= 1` (a class with zero rows never appears at all).

`proportions` — `pluss::Series`
: `counts / sample_count`, elementwise — sums to `1.0` across the whole
  vector.

`inverse_frequency` — `pluss::Series`
: `sample_count / (class_count * counts)`, elementwise — the standard
  "balanced" class-weight formula: a class exactly at the average count
  gets weight `1.0`; a rarer class gets a weight `> 1.0`, proportionally
  larger the rarer it is. This is exactly what
  [`resolve_weight_plan`](resolve_weight_plan.md) reads for
  `ClassWeightMode::Balanced`.

`sample_count` — `std::int64_t`
: The total row count the mix was computed from — `sum(counts)`.

`class_count` — `std::int64_t`
: How many distinct classes were found — `labels.numel()`.

`imbalance_ratio` — `double`
: `max(counts) / min(counts)` — `1.0` for a perfectly balanced
  distribution, larger the more skewed it is. `1.0` also for the
  (unreachable in practice, since [`class_mix`](class_mix.md) already
  rejects an empty label vector) defensive case where no finite minimum
  count exists.

## Methods

None — `ClassMix` is a plain data holder, read directly.

## Practical Example

A three-class imbalanced distribution, every field read back — the same
data as [`class_mix`'s own Example 1](class_mix.md#example-1).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series labels = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 2.0}, {10}, f64);
  const cls::ClassMix mix = cls::class_mix(labels).value();

  std::cout << "class_count = " << mix.class_count << ", sample_count = " << mix.sample_count << "\n";
  std::cout << "imbalance_ratio = " << mix.imbalance_ratio << "\n";
  std::cout << "labels = " << mix.labels << "\n";
  std::cout << "counts = " << mix.counts << "\n";
  std::cout << "proportions = " << mix.proportions << "\n";
  std::cout << "inverse_frequency = " << mix.inverse_frequency << "\n";

  return 0;
}
```

Output

```text
class_count = 3, sample_count = 10
imbalance_ratio = 6
labels = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 1, 2]
counts = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[6, 3, 1]
proportions = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.6, 0.3, 0.1]
inverse_frequency = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.55555556, 1.1111111, 3.3333333]
```

## Changelog

- **0.1** — `labels`, `counts`, `proportions`, `inverse_frequency`,
  `sample_count`, `class_count`, `imbalance_ratio`.

## See Also

- [`class_mix`](class_mix.md) — the producing function.
- [`resolve_weight_plan`](resolve_weight_plan.md) — reads
  `inverse_frequency`; its `WeightPlan` embeds a `ClassMix`.
- [`BalancePlan`](BalancePlan.md) — embeds a `ClassMix` as both
  `.before` and `.after`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

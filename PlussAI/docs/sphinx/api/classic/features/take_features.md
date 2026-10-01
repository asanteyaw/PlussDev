# `pluss::ai::classic::take_features`

Selects an explicit, arbitrary set of feature columns from a
[`FeatureBatch`](../data/FeatureBatch.md) by index, building the reduced
matrix directly rather than filtering in place. The primitive every other
function in this family (`variance_prune`, `pick_features`) calls
internally to materialize its own reduced result — reach for it directly
when you already know exactly which columns you want, in whatever order
you want them.

## Signature

```cpp
classic::StatusOr<classic::FeatureBatch> classic::take_features(
    const classic::FeatureBatch& batch,
    /* a range of std::int64_t -- pass a std::vector<std::int64_t> or braced list */ feature_indices);
```

A second overload accepts `std::initializer_list<std::int64_t>` in place
of the range — the common case when indices are spelled directly as a
braced `{...}` list. Both forms are exercised in the examples below.

## What It Does

Builds a new `FeatureBatch` holding exactly the columns named by
`feature_indices`, in exactly the order given -- no scoring, no filtering
logic, just direct column selection by index, including repeats. This is
the low-level primitive every score-based selector in this family
ultimately calls to materialize its own reduced result once it has decided
which indices to keep.

## Parameters

`batch`
: Must pass [`validate_features`](../data/FeatureBatch.md) and have
  rank-2 `x`. Both
  [`ObservationLayout`](../data/FeatureBatch.md#state) values are
  supported — `Columns` is transposed internally, selected from, then
  transposed back, so the result always keeps `batch`'s own layout.

`feature_indices`
: At least one index, each in `[0, batch.feature_count())`. **Order and
  repetition are both taken literally** — indices need not be sorted,
  and the same index may appear more than once, duplicating that column
  in the output. See [Example 3](#example-3) and [Example 4](#example-4).

## Returns

A `StatusOr<FeatureBatch>` with `feature_count() == feature_indices.size()`
and `sample_count()` unchanged. **The result's `x` is always `Float64`**,
regardless of `batch.x`'s own dtype — `take_features` densifies through a
`Float64` intermediate on the way to selecting columns, and never restores
a narrower source dtype. See [Example 5](#example-5).

## Errors

- `feature_indices` empty — `Status::invalid_argument`.
- `batch` fails [`validate_features`](../data/FeatureBatch.md), or
  `batch.x` is not rank-2 — whatever `validate_features` returns, or
  `Status::shape_mismatch`.
- Any index outside `[0, batch.feature_count())` — `Status::shape_mismatch`.

## Practical Examples

A basic selection reordering columns, the braced-list overload, a
duplicated index, the `Columns`-layout path proven to round-trip
correctly, the dtype-widening-to-`Float64` fact, and an error roundup.

### Example 1

Five columns, three kept and reordered — column `2` first, then `0`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {0.0, 10.0, 20.0, 30.0, 40.0,
       1.0, 11.0, 21.0, 31.0, 41.0,
       2.0, 12.0, 22.0, 32.0, 42.0},
      {15}, f64).reshape({3, 5});
  cls::FeatureBatch batch{};
  batch.x = x;

  const std::vector<std::int64_t> keep = {2, 0};
  const cls::FeatureBatch reduced = cls::take_features(batch, keep).value();
  std::cout << "feature_count() = " << reduced.feature_count() << "\n";
  std::cout << "reduced.x = " << reduced.x << "\n";

  return 0;
}
```

Output

```text
feature_count() = 2
reduced.x = pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[20, 0],
  [21, 1],
  [22, 2]]
```

### Example 2

The braced-list overload — indices spelled directly, no intermediate
`std::vector`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {0.0, 10.0, 20.0, 30.0,
       1.0, 11.0, 21.0, 31.0},
      {8}, f64).reshape({2, 4});
  cls::FeatureBatch batch{};
  batch.x = x;

  const cls::FeatureBatch reduced = cls::take_features(batch, {3, 1}).value();
  std::cout << "reduced.x = " << reduced.x << "\n";

  return 0;
}
```

Output

```text
reduced.x = pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[30, 10],
  [31, 11]]
```

### Example 3

A repeated index duplicates that column in the output — never rejected.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({1, 3});
  cls::FeatureBatch batch{};
  batch.x = x;

  const cls::FeatureBatch reduced = cls::take_features(batch, {0, 0, 2}).value();
  std::cout << "feature_count() = " << reduced.feature_count() << "\n";
  std::cout << "reduced.x (column 0 duplicated) = " << reduced.x << "\n";

  return 0;
}
```

Output

```text
feature_count() = 3
reduced.x (column 0 duplicated) = pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[1, 1, 3]]
```

### Example 4

`ObservationLayout::Columns` is transposed internally and back — the
result keeps the source's own layout, and the selected values are
identical to selecting from the `Rows` equivalent of the same data.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // 3 samples, 4 features, Rows form:
  const cls::Series row_data = pluss::make_series(
      {0.0, 1.0, 2.0, 3.0,
       10.0, 11.0, 12.0, 13.0,
       20.0, 21.0, 22.0, 23.0},
      {12}, f64).reshape({3, 4});
  cls::FeatureBatch row_batch{};
  row_batch.x = row_data;
  row_batch.layout = cls::ObservationLayout::Rows;

  cls::FeatureBatch col_batch{};
  col_batch.x = row_data.transpose(0, 1).contiguous();
  col_batch.layout = cls::ObservationLayout::Columns;

  const cls::FeatureBatch row_reduced = cls::take_features(row_batch, {1, 3}).value();
  const cls::FeatureBatch col_reduced = cls::take_features(col_batch, {1, 3}).value();
  std::cout << "row_reduced.layout is Rows    = " << std::boolalpha << (row_reduced.layout == cls::ObservationLayout::Rows) << "\n";
  std::cout << "col_reduced.layout is Columns = " << (col_reduced.layout == cls::ObservationLayout::Columns) << "\n";

  const double diff = (row_reduced.x - col_reduced.x.transpose(0, 1).contiguous())
                           .abs().amax(0, false).amax(0, false).item<double>();
  std::cout << "max abs diff (same values, either layout) = " << diff << "\n";

  return 0;
}
```

Output

```text
row_reduced.layout is Rows    = true
col_reduced.layout is Columns = true
max abs diff (same values, either layout) = 0
```

### Example 5

A `Float32` source batch is still widened to `Float64` on the way out.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::Series x32 = pluss::make_series(
      {1.0f, 2.0f, 3.0f, 4.0f}, {4},
      pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU)).reshape({1, 4});
  cls::FeatureBatch batch{};
  batch.x = x32;

  const cls::FeatureBatch reduced = cls::take_features(batch, {0, 2}).value();
  std::cout << "source dtype is Float32 = " << std::boolalpha << (batch.x.scalar_type() == pluss::ScalarType::Float32) << "\n";
  std::cout << "result dtype is Float64 = " << (reduced.x.scalar_type() == pluss::ScalarType::Float64) << "\n";

  return 0;
}
```

Output

```text
source dtype is Float32 = true
result dtype is Float64 = true
```

### Example 6

An error roundup: empty indices and an out-of-range index.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({1, 3});
  cls::FeatureBatch batch{};
  batch.x = x;

  const std::vector<std::int64_t> empty{};
  std::cout << "empty indices ok = " << std::boolalpha << cls::take_features(batch, empty).ok() << "\n";
  std::cout << "out-of-range index ok = " << cls::take_features(batch, {5}).ok() << "\n";

  return 0;
}
```

Output

```text
empty indices ok = false
out-of-range index ok = false
```

## Changelog

- **0.1** — `take_features`, a range and an initializer-list overload.

## See Also

- [`variance_prune`](variance_prune.md) / [`pick_features`](pick_features.md)
  — both build their own reduced `FeatureBatch` by calling this function
  internally.
- [`FeatureBatch`](../data/FeatureBatch.md) — the input and output type,
  including `ObservationLayout`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

# `pluss::ai::classic::prep::VectorNormalizer`

Scale every **row** to a unit L1, L2, or L∞ norm, computed across that row's
own features — not a per-feature scaler; see
[`transform/Introduction`](Introduction.md#per-feature-versus-per-row) for
why this lives in `transform` rather than [`scaling`](../scaling.md).
Reach for this when a row's overall magnitude carries no information you
want to keep (text-frequency vectors, embeddings) and only its *direction*
— the relative proportions between its features — matters to the
downstream model.

## Signature

```cpp
explicit classic::prep::VectorNormalizer(classic::prep::VectorNormalizerOptions options = {});
```

## Construction

A concrete value type, constructed directly with an optional
[`VectorNormalizerOptions`](VectorNormalizerOptions.md).

## State

`learn` fits nothing but the feature count and layout — the norm is
computed fresh from each row at `map` time, since row magnitude is not a
property any single row's `learn`-time value could summarize for other
rows. `trained()` reports whether `learn` has succeeded yet, needed only to
validate the shape `map` later receives.

## Methods

`trained()` — `bool`
: `true` once `learn` has succeeded at least once.

`learn(batch, options = {})`
: Records `feature_count()` and `layout` for later shape validation.
  Fails with `Status::invalid_argument` if `floor` is not a positive finite
  number. Does not read the batch's actual values.

`map(batch, options = {})`
: For each row, computes the L1 (sum of absolute values), L2 (Euclidean),
  or L∞ (maximum absolute value) norm per `norm`, over that row's non-`NaN`
  features only, then divides every non-`NaN` entry in the row by it. If
  the computed norm does not reach `floor` (an all-zero, or entirely
  missing, row), the row is left unchanged instead of dividing by (near)
  zero. `NaN` entries pass through unchanged.

`learn_map(batch, learn_options = {}, infer_options = {})`
: `learn` then `map` on the same data, in one call.

`dump_state()` / `load_state(state)`
: `[metadata]` — a single tensor (`trained`, `layout`, `feature_count`); no
  norm is ever fitted from data, the same reason
  [`HashingEncoder`](../encoding/HashingEncoder.md)'s state is metadata-only.

## Errors

- `floor <= 0`, non-finite, or `NaN` — `Status::invalid_argument` from
  `learn`.
- `map`/`learn_map` before a successful `learn` — `Status::precondition`.
- A `map` batch whose layout or feature count does not match what `learn`
  was fit on — `Status::shape_mismatch`.

## Practical Examples

L1 versus L2 versus L∞ normalization on the same row, and the degenerate
all-zero-row fallback.

### Example 1

The same row normalized under all three norms — each producing a
different, verifiable result.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({3.0, 4.0}, {2}, f64).reshape({1, 2});  // L2 norm is exactly 5

  auto normalize = [&](cls::prep::VectorNorm norm) {
    cls::prep::VectorNormalizerOptions options{};
    options.norm = norm;
    cls::prep::VectorNormalizer normalizer(options);
    return normalizer.learn_map(batch).value();
  };

  std::cout << "L1:   " << normalize(cls::prep::VectorNorm::L1) << "\n";
  std::cout << "L2:   " << normalize(cls::prep::VectorNorm::L2) << "\n";
  std::cout << "Linf: " << normalize(cls::prep::VectorNorm::Linf) << "\n";

  return 0;
}
```

Output

```text
L1:   pluss::Series(shape=[1,2], strides=[2,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[0.42857143, 0.57142857]]
L2:   pluss::Series(shape=[1,2], strides=[2,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[0.6, 0.8]]
Linf: pluss::Series(shape=[1,2], strides=[2,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[0.75, 1]]
```

### Example 2

A missing entry is excluded from the row's norm and left as `NaN`, not
treated as `0`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <limits>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const double nan = std::numeric_limits<double>::quiet_NaN();

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({3.0, nan, 4.0}, {3}, f64).reshape({1, 3});  // L2 norm over {3, 4} is 5

  cls::prep::VectorNormalizer normalizer;  // L2 by default
  std::cout << "map = " << normalizer.learn_map(batch).value() << "\n";

  return 0;
}
```

Output

```text
map = pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[0.6, nan, 0.8]]
```

### Example 3

An all-zero row's norm does not reach `floor` — it is left unchanged
instead of dividing by zero.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({0.0, 0.0}, {2}, f64).reshape({1, 2});

  cls::prep::VectorNormalizer normalizer;
  std::cout << "all-zero row -> " << normalizer.learn_map(batch).value() << "\n";

  return 0;
}
```

Output

```text
all-zero row -> pluss::Series(shape=[1,2], strides=[2,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[0, 0]]
```

## Changelog

- **0.1** — `learn`/`map`/`learn_map`, `norm`/`floor`, `dump_state`/
  `load_state`.

## See Also

- [`VectorNormalizerOptions`](VectorNormalizerOptions.md) — the options
  struct, including the `VectorNorm` enum.
- [`scaling`](../scaling.md) — the per-feature (not per-row) counterpart.
- [`transform/Introduction`](Introduction.md) — why this type lives in
  `transform` rather than `scaling`.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

VectorNormalizerOptions
```
<!-- apiref:toctree:end -->

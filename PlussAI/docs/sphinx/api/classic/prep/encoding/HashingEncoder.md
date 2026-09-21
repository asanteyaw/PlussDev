# `pluss::ai::classic::prep::HashingEncoder`

The "hashing trick": map every value into one of `n_components` fixed
buckets with a deterministic multiplicative hash, and emit an
`n_components`-wide one-hot indicator — no vocabulary fitted or stored at
all. Output width is fixed up front and independent of how many distinct
categories actually appear, so an unbounded or unknown cardinality (a raw ID
column, free-text categories with no closed set) is handled for free, at the
cost of possible collisions (two different values landing in the same
bucket, indistinguishable afterward) and no reverse mapping back to the
original category. Reach for [`BaseNEncoder`](BaseNEncoder.md) instead when
you want a bounded, collision-free vocabulary and can tolerate the fit step.

**`HashingEncoder` never consults `InferOptions::unknown_categorical`** —
there is no concept of an unseen category here, since nothing is fitted for
`map` to compare against; every value, seen during `learn` or not, hashes
through the same formula.

## Signature

```cpp
explicit classic::prep::HashingEncoder(classic::prep::HashingEncoderOptions options = {});
```

## Construction

A concrete value type, constructed directly with an optional
[`HashingEncoderOptions`](HashingEncoderOptions.md).

## State

`learn` records only the feature count and layout — there is no vocabulary
to fit, since the hash formula is a pure function of the value and the
constructed `seed`/`n_components`. `trained()` reports whether `learn` has
succeeded yet (needed only to validate the batch shape `map` later receives,
not to unlock any fitted mapping).

## Methods

`trained()` — `bool`
: `true` once `learn` has succeeded at least once.

`learn(batch, options = {})`
: Records `feature_count()` and `layout` for later shape validation.
  Fails with `Status::invalid_argument` if `n_components < 1`. Does not
  read the batch's actual values at all — no vocabulary is fitted, so a
  later `learn` on entirely different data changes nothing about how `map`
  will behave (only the recorded shape).

`learn(thread, options = {})`
: The single-column convenience overload.

`map(batch, options = {})`
: For each feature, computes `bucket = floor(frac(value * golden_ratio +
  seed_offset) * n_components)` (clamped to `n_components - 1`, guarding
  the rare case `frac` lands exactly at `1.0`), then emits the
  `n_components`-wide one-hot indicator for that bucket — entirely
  vectorized `Series` arithmetic, no host-side loop. `seed_offset` is
  derived from `seed` (`0` by default), so two encoders constructed with the
  same `n_components` and `seed` hash every value identically; a different
  `seed` reshuffles which bucket each value lands in without changing the
  bucket count. **A missing (`NaN`) value hashes to `NaN` throughout and
  compares false against every bucket, producing an all-zero row** for that
  feature — the one deliberate exception to "every value hashes through the
  same formula," since `NaN` cannot equal any concrete bucket index.

`map(thread, options = {})`
: The single-column convenience overload.

`learn_map(batch_or_thread, learn_options = {}, infer_options = {})`
: `learn` then `map` on the same data, in one call.

`dump_state()` / `load_state(state)`
: `[metadata]` — a single tensor (`trained`, `layout`, `feature_count`);
  `n_components` and `seed` live on the constructed `HashingEncoderOptions`,
  not in the fitted state, since nothing about them is actually fitted from
  data.

## Errors

- `n_components < 1` — `Status::invalid_argument` from `learn`.
- `map`/`learn_map` before a successful `learn` — `Status::precondition`.
- A `map` batch whose layout or feature count does not match what `learn`
  was fit on — `Status::shape_mismatch`.
- **`unknown_categorical` on the `InferOptions` passed to `map` is silently
  ignored** — there is no unseen-category concept for a hash-based encoder.

## Practical Examples

The deterministic bucket assignment for a fixed `seed`, the same values
reshuffled by a different `seed`, a missing value's all-zero row, and the
single-column `Thread` overload proven equivalent to a `FeatureBatch`.

### Example 1

Five raw values hashed into `4` buckets with the default `seed = 0` — the
same two encoders constructed identically reproduce the same buckets, shown
by hashing twice.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::HashingEncoderOptions options{};
  options.n_components = 4;

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64).reshape({5, 1});

  cls::prep::HashingEncoder first(options);
  first.learn(probe);
  cls::prep::HashingEncoder second(options);
  second.learn(probe);

  std::cout << "first  = " << first.map(probe).value() << "\n";
  std::cout << "second = " << second.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
first  = pluss::Series(shape=[5,4], strides=[4,1], dtype=float64, platform=cpu, numel=20, contiguous=true)
data=[[0, 0, 1, 0],
  [1, 0, 0, 0],
  [0, 0, 0, 1],
  [0, 1, 0, 0],
  [1, 0, 0, 0]]
second = pluss::Series(shape=[5,4], strides=[4,1], dtype=float64, platform=cpu, numel=20, contiguous=true)
data=[[0, 0, 1, 0],
  [1, 0, 0, 0],
  [0, 0, 0, 1],
  [0, 1, 0, 0],
  [1, 0, 0, 0]]
```

### Example 2

A different `seed` reshuffles the same values into different buckets.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});

  cls::prep::HashingEncoderOptions seed0{};
  seed0.n_components = 4;
  cls::prep::HashingEncoder encoder0(seed0);
  encoder0.learn(probe);

  cls::prep::HashingEncoderOptions seed7 = seed0;
  seed7.seed = 7;
  cls::prep::HashingEncoder encoder7(seed7);
  encoder7.learn(probe);

  std::cout << "seed=0 -> " << encoder0.map(probe).value() << "\n";
  std::cout << "seed=7 -> " << encoder7.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
seed=0 -> pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[0, 0, 1, 0],
  [1, 0, 0, 0],
  [0, 0, 0, 1]]
seed=7 -> pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[0, 0, 0, 1],
  [0, 0, 1, 0],
  [1, 0, 0, 0]]
```

### Example 3

A missing value produces an all-zero row rather than landing in any bucket.

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

  cls::prep::HashingEncoderOptions options{};
  options.n_components = 4;
  cls::prep::HashingEncoder encoder(options);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({1.0, nan}, {2}, f64).reshape({2, 1});
  encoder.learn(probe);
  std::cout << "map([1, NaN]) = " << encoder.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
map([1, NaN]) = pluss::Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[0, 0, 1, 0],
  [0, 0, 0, 0]]
```

### Example 4

The single-column `Thread` overload — the same training/probe values as
Example 1, fit and mapped through `learn(thread, ...)`/`map(thread, ...)`
directly instead of a `FeatureBatch`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  using pluss::table::Thread;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::HashingEncoderOptions options{};
  options.n_components = 4;

  const Thread probe(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64));

  cls::prep::HashingEncoder via_thread(options);
  via_thread.learn(probe);
  const pluss::Series thread_result = via_thread.map(probe).value();

  cls::FeatureBatch probe_batch{};
  probe_batch.x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64).reshape({5, 1});

  cls::prep::HashingEncoder via_batch(options);
  via_batch.learn(probe_batch);
  const pluss::Series batch_result = via_batch.map(probe_batch).value();

  const double max_abs_diff = (thread_result - batch_result).abs().amax(0, false).amax(0, false).item<double>();
  std::cout << "max abs diff between Thread and FeatureBatch forms = " << max_abs_diff << "\n";

  return 0;
}
```

Output

```text
max abs diff between Thread and FeatureBatch forms = 0
```

## Changelog

- **0.1** — `learn`/`map`/`learn_map` (`FeatureBatch` and `Thread`
  overloads), `n_components`/`seed`, `dump_state`/`load_state`.

## See Also

- [`HashingEncoderOptions`](HashingEncoderOptions.md) — the options struct.
- [`BaseNEncoder`](BaseNEncoder.md) — the bounded, collision-free
  alternative.
- [`encoding/Introduction`](Introduction.md) — the contract shared across
  this whole sub-family.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

HashingEncoderOptions
```
<!-- apiref:toctree:end -->

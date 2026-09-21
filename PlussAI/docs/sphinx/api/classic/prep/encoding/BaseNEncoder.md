# `pluss::ai::classic::prep::BaseNEncoder`

Assign each fitted category a 1-based integer code, then expand that code
into its digits in a chosen radix (`base`) — `K` categories cost
`ceil(log_base(K + 1))` output columns instead of the `K` (or `K + 1`)
[`OneHotEncoder`](OneHotEncoder.md) would spend. `base = 2` is the popular
"binary encoding" special case. Reach for this over one-hot encoding once
cardinality is high enough that column count matters, but you still want a
bounded, deterministic vocabulary (as opposed to
[`HashingEncoder`](HashingEncoder.md), which trades the vocabulary away
entirely for an unbounded input domain).

**`BaseNEncoder` never consults `InferOptions::unknown_categorical`** — see
`## Methods` below for what an unseen or missing value produces instead.

## Signature

```cpp
explicit classic::prep::BaseNEncoder(classic::prep::BaseNEncoderOptions options = {});
```

## Construction

A concrete value type, constructed directly with an optional
[`BaseNEncoderOptions`](BaseNEncoderOptions.md).

## State

The fitted per-feature category list and per-feature digit count live inside
the object, set by `learn`, read back only through `map` / `dump_state`.
`trained()` reports whether `learn` has succeeded yet.

## Methods

`trained()` — `bool`
: `true` once `learn` has succeeded at least once.

`learn(batch, options = {})`
: Fits a vocabulary per feature the same way
  [`OneHotEncoder::learn`](OneHotEncoder.md) does (count, cap against
  `max_categories` per `overflow`, sort). For a feature whose fitted
  vocabulary has `K` categories, also computes and stores the digit count
  `ceil(log_base(K + 1))` — enough digits to represent every code `0..K`
  (`0` reserved for missing/unseen, `1..K` the fitted categories in sorted
  order). Fails with `Status::invalid_argument` if `base < 2`.

`learn(thread, options = {})`
: The single-column convenience overload.

`map(batch, options = {})`
: For each feature, assigns every row its category's 1-based code (`0` for
  a missing **or genuinely unseen** value — the two are indistinguishable in
  the output, since there is no `unknown_categorical`-style branch here),
  then expands that code into exactly the digit count fitted for that
  feature, **least-significant digit first**. The output for one feature is
  that many columns; a batch's full output is those column blocks
  concatenated in feature order. Entirely vectorized `Series` arithmetic —
  no host-side loop over rows, and no distinction drawn between "category
  dropped by `overflow`" and "category never seen at `learn` time" either;
  both produce code `0`.

`map(thread, options = {})`
: The single-column convenience overload.

`learn_map(batch_or_thread, learn_options = {}, infer_options = {})`
: `learn` then `map` on the same data, in one call.

`dump_state()` / `load_state(state)`
: `[metadata, key_sizes, flat_keys, digit_counts]` — metadata also carries
  `base` and the total output width, since both are needed to interpret the
  digit columns on reload.

## Errors

- `base < 2` — `Status::invalid_argument` from `learn`.
- `max_categories < 0` — `Status::invalid_argument` from `learn`.
- `overflow = Error` and a feature's cardinality exceeds `max_categories` —
  `Status::invalid_argument` from `learn`.
- `map`/`learn_map` before a successful `learn` — `Status::precondition`.
- **`unknown_categorical` on the `InferOptions` passed to `map` is silently
  ignored** — not an error, but worth calling out explicitly since every
  other encoder in this sub-family (except [`HashingEncoder`](HashingEncoder.md))
  honors it.

## Practical Examples

The default binary (`base = 2`) encoding's digit order, a missing/unseen
value collapsing to the all-zero code, a higher radix (`base = 3`)
shrinking the digit count further, and the single-column `Thread`
overload proven equivalent to a `FeatureBatch`.

### Example 1

Three categories, `base = 2` (binary encoding): codes `1`/`2`/`3` need 2
bits, emitted least-significant digit first.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({10.0, 20.0, 30.0}, {3}, f64).reshape({3, 1});

  cls::prep::BaseNEncoder encoder;  // base = 2 by default
  encoder.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({10.0, 20.0, 30.0}, {3}, f64).reshape({3, 1});
  std::cout << "codes 1, 2, 3 in binary (LSB first) = " << encoder.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
codes 1, 2, 3 in binary (LSB first) = pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 0],
  [0, 1],
  [1, 1]]
```

### Example 2

A value `map` has never seen collapses to the same all-zero code a missing
(`NaN`) value gets — there is no way to tell the two apart from the output
alone.

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

  cls::FeatureBatch train{};
  train.x = pluss::make_series({10.0, 20.0}, {2}, f64).reshape({2, 1});
  cls::prep::BaseNEncoder encoder;
  encoder.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({99.0, nan}, {2}, f64).reshape({2, 1});
  std::cout << "unseen (99) and missing (NaN) both -> " << encoder.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
unseen (99) and missing (NaN) both -> pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0, 0],
  [0, 0]]
```

### Example 3

`base = 3`: with only 2 fitted categories, codes `0..2` already fit in a
single trit — `digit_count` shrinks to `1`, one output column instead of
`base = 2`'s two.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({10.0, 20.0}, {2}, f64).reshape({2, 1});  // 2 categories -> codes 1, 2

  cls::prep::BaseNEncoderOptions options{};
  options.base = 3;
  cls::prep::BaseNEncoder encoder(options);
  encoder.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({10.0, 20.0}, {2}, f64).reshape({2, 1});
  std::cout << "base=3, codes 1 and 2 -> " << encoder.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
base=3, codes 1 and 2 -> pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[1],
  [2]]
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

  const Thread train(pluss::make_series({10.0, 20.0, 30.0}, {3}, f64));
  const Thread probe(pluss::make_series({10.0, 20.0, 30.0}, {3}, f64));

  cls::prep::BaseNEncoder via_thread;  // base = 2 by default
  via_thread.learn(train);
  const pluss::Series thread_result = via_thread.map(probe).value();

  cls::FeatureBatch train_batch{};
  train_batch.x = pluss::make_series({10.0, 20.0, 30.0}, {3}, f64).reshape({3, 1});
  cls::FeatureBatch probe_batch{};
  probe_batch.x = pluss::make_series({10.0, 20.0, 30.0}, {3}, f64).reshape({3, 1});

  cls::prep::BaseNEncoder via_batch;
  via_batch.learn(train_batch);
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
  overloads), `base`/`max_categories`/`overflow`, `dump_state`/`load_state`.

## See Also

- [`BaseNEncoderOptions`](BaseNEncoderOptions.md) — the options struct.
- [`HashingEncoder`](HashingEncoder.md) — the unbounded-cardinality
  alternative, no vocabulary at all.
- [`OneHotEncoder`](OneHotEncoder.md) — the wider-output alternative with
  full `unknown_categorical` support.
- [`encoding/Introduction`](Introduction.md) — the contract shared across
  this whole sub-family.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

BaseNEncoderOptions
```
<!-- apiref:toctree:end -->

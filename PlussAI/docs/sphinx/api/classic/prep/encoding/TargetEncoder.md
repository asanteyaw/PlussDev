# `pluss::ai::classic::prep::TargetEncoder`

Replace each category with a smoothed average of the **target** values rows
carrying that category had — mean/target encoding via an additive
m-estimate:
`(sum_y_in_category + smoothing * prior) / (count_in_category + smoothing)`,
where `prior` is the target's own overall mean. A category seen often pulls
its encoded value close to its own average target; a rarely-seen category
is pulled instead toward `prior`, with `smoothing` controlling how fast that
pull-back happens. This is a **supervised** encoder — it needs the target at
fit time — appropriate for regression or a binary `0`/`1` target, one output
column per input feature regardless of cardinality. Reach for this over
[`FrequencyEncoder`](FrequencyEncoder.md) when the category's relationship
to the *target* matters more than its raw popularity, and accept the
target-leakage risk that comes with fitting on the same rows you will train
on (validate with held-out folds rather than trusting training-set metrics
computed after this encoding).

## Signature

```cpp
explicit classic::prep::TargetEncoder(classic::prep::TargetEncoderOptions options = {});
```

## Construction

A concrete value type, constructed directly with an optional
[`TargetEncoderOptions`](TargetEncoderOptions.md). Inherits its entire
public interface from `SupervisedEncoderBase` — `TargetEncoder` itself adds
only the constructor and the internal fitting formula.

## State

The fitted per-feature category list, matching encoded values, and
per-feature "unknown" fallback (the target's own prior) live inside the
object, set by `learn_supervised`, read back only through `map` /
`dump_state`. `trained()` reports whether fitting has succeeded yet.

## Methods

`trained()` — `bool`
: `true` once `learn_supervised` has succeeded at least once.

`needs_target()` — `bool`
: `true`, always — this is what routes a target automatically to this stage
  when it is added inside a [`compose::Flow`](../../compose/Flow.md).

`learn(batch, options = {})`
: **Always fails** with `Status::invalid_argument` — call
  `learn_supervised` instead. Exists only to satisfy the
  [`TransformerBase`](../../estimators/TransformerBase.md) interface shape.

`learn_supervised(batch, target, options = {})`
: Fits a vocabulary per feature the same way
  [`OneHotEncoder::learn`](OneHotEncoder.md) does (count, cap against
  `max_categories` per `overflow`, sort), then computes `prior` as the mean
  of `target`'s non-`NaN` values and, per fitted category, the m-estimate
  average above using only the rows whose target is non-`NaN` too. `prior`
  itself becomes the fallback value an unseen category gets at `map` time —
  see `## Errors`. Fails with `Status::shape_mismatch` if `target`'s length
  does not match the batch's sample count.

`map(batch, options = {})`
: Replaces each fitted category with its computed value. **There is no
  `unknown_categorical`-driven branch here** — every unseen (or
  `overflow`-dropped) category always maps to `prior`, the target's own
  overall mean, regardless of what `InferOptions::unknown_categorical` is
  set to. `NaN` passes through `NaN`.

`learn_map(batch, ...)`
: **Always fails**, the same as `learn` — there is no target-free
  `learn_map` for a supervised encoder. Use `learn_supervised` then `map`
  separately.

`dump_state()` / `load_state(state)`
: `[metadata, key_sizes, flat_keys, flat_values, unknown_values]`, shared
  verbatim with [`WOEEncoder`](WOEEncoder.md) (both inherit
  `SupervisedEncoderBase`'s persistence).

## Errors

- `max_categories < 0` — `Status::invalid_argument` from `learn_supervised`.
- `overflow = Error` and a feature's cardinality exceeds `max_categories` —
  `Status::invalid_argument` from `learn_supervised`.
- `target.numel()` not equal to the batch's sample count —
  `Status::shape_mismatch` from `learn_supervised`.
- `learn(batch)` / `learn_map(batch, ...)` called directly (no target) —
  `Status::invalid_argument` naming `learn_supervised` as the required call.
- `map`/`learn_supervised` before a successful `learn_supervised` (for
  `map`) — `Status::precondition`.

## Practical Examples

The m-estimate formula on a small fitted vocabulary, `smoothing` pulling a
rare category toward the prior, and the always-`prior` fallback for an
unseen category — shown via `Flow::learn` routing the target automatically,
the way a `TargetEncoder` stage is meant to be used in practice.

### Example 1

Two categories with clearly different average targets — `learn_supervised`
directly, verifying the m-estimate values by hand.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // category 1: targets 0, 0, 0 (mean 0). category 2: targets 10, 10 (mean 10). prior = 4.
  cls::FeatureBatch train{};
  train.x = pluss::make_series({1.0, 1.0, 1.0, 2.0, 2.0}, {5}, f64).reshape({5, 1});
  const pluss::Series target = pluss::make_series({0.0, 0.0, 0.0, 10.0, 10.0}, {5}, f64);

  cls::prep::TargetEncoderOptions options{};
  options.smoothing = 1.0;
  cls::prep::TargetEncoder encoder(options);
  encoder.learn_supervised(train, target);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({2, 1});
  std::cout << "map([category 1, category 2]) = " << encoder.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
map([category 1, category 2]) = pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[1],
  [8]]
```

### Example 2

Raising `smoothing` pulls the same rare category's encoded value further
toward the overall prior.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // category 1 seen once with target 100; four other rows average target 0. prior = 20.
  cls::FeatureBatch train{};
  train.x = pluss::make_series({1.0, 2.0, 2.0, 2.0, 2.0}, {5}, f64).reshape({5, 1});
  const pluss::Series target = pluss::make_series({100.0, 0.0, 0.0, 0.0, 0.0}, {5}, f64);

  auto fit_category_1 = [&](double smoothing) {
    cls::prep::TargetEncoderOptions options{};
    options.smoothing = smoothing;
    cls::prep::TargetEncoder encoder(options);
    encoder.learn_supervised(train, target);
    cls::FeatureBatch probe{};
    probe.x = pluss::make_series({1.0}, {1}, f64).reshape({1, 1});
    return encoder.map(probe).value().item<double>();
  };

  std::cout << "prior = 20\n";
  std::cout << "smoothing=0.1  -> " << fit_category_1(0.1) << "\n";
  std::cout << "smoothing=100  -> " << fit_category_1(100.0) << "\n";

  return 0;
}
```

Output

```text
prior = 20
smoothing=0.1  -> 92.7273
smoothing=100  -> 20.7921
```

### Example 3

An unseen category always maps to `prior`, and `TargetEncoder` routed
automatically inside `Flow::learn` — no manual `learn_supervised` call.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow;
  flow.add_target_encoder();

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 1.0, 2.0, 2.0}, {4}, f64).reshape({4, 1});
  batch.y = pluss::make_series({0.0, 0.0, 10.0, 10.0}, {4}, f64);
  flow.learn(batch);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({99.0}, {1}, f64).reshape({1, 1});
  std::cout << "unseen category (any unknown_categorical) -> " << flow.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
unseen category (any unknown_categorical) -> pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[5]]
```

## Changelog

- **0.1** — `learn_supervised`/`map` (via `SupervisedEncoderBase`),
  `smoothing`/`max_categories`/`overflow`, `dump_state`/`load_state`.

## See Also

- [`TargetEncoderOptions`](TargetEncoderOptions.md) — the options struct.
- [`WOEEncoder`](WOEEncoder.md) — the binary-target, log-odds-scale
  alternative.
- [`compose::Flow::learn`](../../compose/Flow/learn.md) — how a supervised
  encoder stage actually receives its target in practice.
- [`encoding/Introduction`](Introduction.md) — the contract shared across
  this whole sub-family.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

TargetEncoderOptions
```
<!-- apiref:toctree:end -->

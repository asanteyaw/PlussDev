# `pluss::ai::classic::prep::FunctionTransformer`

Wrap a `Series -> Series` callable you already have as an ordinary
[`TransformerBase`](../../estimators/TransformerBase.md) stage — the escape
hatch for a one-off transform none of the named `prep` kernels cover, so it
can still be routed inside a [`FeatureTransformer`](FeatureTransformer.md)
or a [`compose::Flow`](../../compose/Flow.md) alongside every named kernel.
Unlike every other type in this reference, construction itself — not
`learn` — is where the actual transform logic comes from; `learn` here only
checks that a callable was supplied and (optionally) validates shape.

## Signature

```cpp
using classic::prep::FunctionTransformer::SeriesMapFn =
    std::function<classic::StatusOr<pluss::Series>(const pluss::Series&)>;

explicit classic::prep::FunctionTransformer(
    SeriesMapFn map_fn = {}, classic::prep::FunctionTransformerOptions options = {});
```

## Construction

Pass the callable at construction — a lambda, function pointer, or any
`SeriesMapFn`-compatible callable taking a whole batch's feature `Series`
and returning a `StatusOr<Series>`. A default-constructed (empty)
`map_fn` is accepted syntactically but every call fails at `learn`/`map`
time (see `## Errors`) rather than at construction, since `FunctionTransformer{}`
has to remain default-constructible to satisfy the same interface every
other `prep` kernel offers.

## State

`trained()` reflects whether a callable was supplied **and** `learn` has
run: `true` immediately at construction if `map_fn` is non-empty (per
`trained() { return map_fn_ != nullptr; }` — there is no separate "fit"
step for a callable, so having one at all counts as trained), `false` for a
default-constructed instance until `learn` is called with a real callable.

## Methods

`trained()` — `bool`
: `true` once a non-empty callable exists on this instance.

`learn(batch, options = {})`
: Fails with `Status::invalid_argument` if no callable was supplied.
  Otherwise, if `validate_input` is on (the default), runs
  [`validate_features`](../../data/FeatureBatch.md) against `batch` and
  fails the same way that would; records `feature_count()` and `layout`.
  The callable itself is never invoked here — only at `map` time.

`map(batch, options = {})`
: Calls the wrapped callable with `batch.x` and wraps the result back into
  a `FeatureBatch` using `batch.layout`. Fails if the callable itself
  returns a non-`ok` `StatusOr`, if the result fails
  [`validate_features`](../../data/FeatureBatch.md) (`Status::shape_mismatch`,
  `"... emitted invalid feature shape."`), or — when
  `preserve_sample_count` is on (the default) — if the result's sample
  count differs from the input's (`Status::shape_mismatch`,
  `"... callable changed sample count."`).

`learn_map(batch, learn_options = {}, infer_options = {})`
: `learn` then `map` on the same data, in one call.

`dump_state()` / `load_state(state)`
: Both **always fail** with `Status::not_implemented` — a captured
  callable has no serializable state of its own; there is nothing this type
  could persist and later reconstruct into a working callable.

## Errors

- No callable supplied — `Status::invalid_argument`,
  `"FunctionTransformer::learn requires a valid callable."` (from `learn`)
  or `"...::map requires a valid callable."` (from `map`, if `learn` was
  somehow skipped).
- `validate_input` on and `batch` fails `validate_features` — whatever
  `Status` that check itself returns.
- The callable's own `StatusOr<Series>` is not `ok` — that same `Status`,
  propagated directly.
- The callable's output fails shape validation, or (with
  `preserve_sample_count`) changes the sample count —
  `Status::shape_mismatch`.
- `dump_state()` / `load_state(...)` — always `Status::not_implemented`.

## Practical Examples

A callable doing something no named kernel does directly, the
`preserve_sample_count` guard catching a callable that drops rows, and the
`dump_state` limitation made explicit.

### Example 1

Wrapping `Series::abs()` — a transform none of the named kernels expose
directly.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::FunctionTransformer transformer(
      [](const pluss::Series& x) -> cls::StatusOr<pluss::Series> { return cls::StatusOr<pluss::Series>(x.abs()); });

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({-3.0, 2.0, -1.0}, {3}, f64).reshape({3, 1});
  std::cout << "map = " << transformer.learn_map(batch).value() << "\n";

  return 0;
}
```

Output

```text
map = pluss::Series(shape=[3,1], strides=[1,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[3],
  [2],
  [1]]
```

### Example 2

A callable that (deliberately, for this example) drops a row is caught by
`preserve_sample_count` rather than silently returning a shrunk batch.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::FunctionTransformer transformer(
      [&](const pluss::Series& x) -> cls::StatusOr<pluss::Series> {
        (void)x;
        // returns one row no matter how many the input had -- a deliberately
        // broken callable, to demonstrate the guard it trips.
        return cls::StatusOr<pluss::Series>(pluss::make_series({1.0}, {1}, f64).reshape({1, 1}));
      });

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({2, 1});
  const cls::StatusOr<pluss::Series> mapped = transformer.learn_map(batch);
  std::cout << "ok = " << std::boolalpha << mapped.ok() << "\n";
  std::cout << "message = \"" << mapped.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "FunctionTransformer::map callable changed sample count."
```

### Example 3

`dump_state()` always fails — there is nothing about a captured callable
that could be serialized.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::FunctionTransformer transformer(
      [](const pluss::Series& x) -> cls::StatusOr<pluss::Series> { return cls::StatusOr<pluss::Series>(x); });

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({1.0}, {1}, f64).reshape({1, 1});
  transformer.learn(batch);

  const cls::StatusOr<std::vector<pluss::Series>> state = transformer.dump_state();
  std::cout << "ok = " << std::boolalpha << state.ok() << "\n";
  std::cout << "message = \"" << state.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "FunctionTransformer::dump_state is unavailable for callable-based stages."
```

## Changelog

- **0.1** — `learn`/`map`/`learn_map`, `validate_input`/
  `preserve_sample_count`, `dump_state`/`load_state` (always
  `not_implemented`).

## See Also

- [`FunctionTransformerOptions`](FunctionTransformerOptions.md) — the
  options struct.
- [`FeatureTransformer`](FeatureTransformer.md) — routes any
  `TransformerBase`, including a `FunctionTransformer`, into a composed
  sub-pipeline.
- [`Flow::add_function_transformer`](../../compose/Flow/add_function_transformer.md) —
  routing this into a `compose::Flow`.
- [`transform/Introduction`](Introduction.md) — the parent sub-family.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

FunctionTransformerOptions
```
<!-- apiref:toctree:end -->

# `pluss::ai::classic::prep::FeatureTransformer`

A routed sub-pipeline of other transformers — one or more stages, each a
list of one or more `TransformerBase`s applied in sequence, each routed to
a subset of the input's feature columns, with a policy for whatever columns
no stage claims. Structurally the same mechanics
[`compose::Flow`](../../compose/Flow.md) uses for its own stages, packaged
as a standalone `TransformerBase` you can build, fit, and nest — including
as one stage inside a `Flow` itself (via
[`Flow::add_feature_transformer`](../../compose/Flow/add_feature_transformer.md)),
or use entirely on its own outside any `Flow`.

## Signature

```cpp
explicit classic::prep::FeatureTransformer(
    classic::prep::FeatureRemainderPolicy remainder = classic::prep::FeatureRemainderPolicy::Drop);
```

Move-only: `FeatureTransformer` has a defaulted move constructor/assignment
and a **deleted** copy constructor/assignment — the same ownership shape as
[`Flow`](../../compose/Flow.md), for the same reason (it owns a
`std::vector<std::unique_ptr<TransformerBase>>` per stage).

## Construction

`FeatureTransformer(remainder)` builds an empty sub-pipeline — no stages —
with the given remainder policy fixed at construction time (mutable
afterward via `set_remainder_policy`). Add stages with `add_stage` /
`add_pipeline` before calling `learn`/`learn_map`.

## State

`stage_count()` — `std::int64_t`
: The number of stages added so far via `add_stage`/`add_pipeline`.

`remainder_policy()` — `FeatureRemainderPolicy`
: The current policy for unrouted columns — `Drop` (the default) or
  `PassThrough`.

`trained()` — `bool`
: `true` once `learn`/`learn_map` has succeeded. `false` on a fresh
  instance and after `clear_stages()`.

## Methods

`add_stage(transformer, feature_route)`
: Adds a single-transformer stage routed to `feature_route` (a non-empty
  list of feature indices — no negative-index wraparound, every index must
  already be within `[0, feature_count)`, checked at `learn`/`learn_map`
  time once the actual feature count is known). Equivalent to
  `add_pipeline({std::move(transformer)}, feature_route)`. Resets
  `trained()` to `false`.

`add_pipeline(transformers, feature_route)`
: Adds a stage that chains **multiple** transformers in sequence, all
  routed to the same `feature_route` — stage `i`'s transformers run one
  after another, each one's output feeding the next. Fails with
  `Status::invalid_argument` if `transformers` is empty, contains a null
  pointer, or `feature_route` is empty. Resets `trained()` to `false`.

`clear_stages()`
: Removes every stage and resets `trained()`, `stage_count()`, and the
  fitted output feature count back to their empty-instance values.

`remainder_policy()` / `set_remainder_policy(policy)`
: `Drop` (the default) excludes every column no stage's route claims from
  the output entirely; `PassThrough` appends them, unmodified, as the last
  block of output columns. Setting it resets `trained()` to `false`.

`learn(batch, options = {})`
: Fits every stage and discards the mapped output — internally calls
  `learn_map` and keeps only its `Status`. Call `learn_map` directly
  instead if you also need the transformed training data, to avoid paying
  for the transform twice (once inside `learn`, again in a following
  `map` call).

`map(batch, options = {})`
: Routes `batch`'s columns to each fitted stage per its stored route, runs
  that stage's transformers in sequence, concatenates every stage's output
  (plus the passed-through remainder, if any) in stage order, and validates
  the merged result's sample count and feature count against what `learn`
  fitted. `Status::precondition` if not yet trained. With zero stages: the
  whole batch unchanged if `remainder = PassThrough`, otherwise
  `Status::invalid_argument`.

`learn_map(batch, learn_options = {}, infer_options = {})`
: Fits every stage's route (validating and normalizing it against the
  batch's actual feature count) and every stage's transformers together,
  in one pass, returning the transformed training data directly. This is
  the call `learn` uses internally — prefer it directly whenever you need
  both the fit and its output on the same data.

`dump_state()` / `load_state(state)`
: Concatenates every stage's own `dump_state()` output behind one metadata
  tensor — a per-stage failure (any stage's own `Status::not_implemented`,
  for instance) propagates as this call's own failure.

## Errors

- `add_stage`/`add_pipeline`: an empty `transformers` list, a null
  transformer pointer, or an empty `feature_route` — `Status::invalid_argument`.
- A route index out of `[0, feature_count)`, or containing a duplicate —
  `Status::shape_mismatch` / `Status::invalid_argument` respectively, from
  `learn`/`learn_map`/`map` once the feature count is known.
- Zero stages with `remainder = Drop` — `Status::invalid_argument` from
  `learn_map`/`map`.
- Any stage's own `learn`/`map` call fails — that `Status`, propagated
  directly (naming the failing stage index in the message).
- A stage's output changes sample count, or the merged output's shape does
  not match what was fitted — `Status::shape_mismatch`.
- `map`/`learn` before a successful `learn_map` — `Status::precondition`.

## Practical Examples

Two stages routed to disjoint columns with `PassThrough` for the rest, a
multi-transformer pipeline chained within one stage, and the
`learn_map`-versus-`learn`-then-`map` cost distinction made concrete.

### Example 1

Column 0 scaled, column 1 one-hot encoded, column 2 passed through
untouched — three feature blocks concatenated into one output.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <memory>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::FeatureTransformer transformer(cls::prep::FeatureRemainderPolicy::PassThrough);
  transformer.add_stage(std::make_unique<cls::prep::StandardScaler>(), {0});
  transformer.add_stage(std::make_unique<cls::prep::OneHotEncoder>(), {1});

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series(
      {0.0, 1.0, 100.0, 10.0, 2.0, 200.0}, {6}, f64).reshape({2, 3});
  std::cout << "stage_count() = " << transformer.stage_count() << "\n";
  std::cout << "map = " << transformer.learn_map(batch).value() << "\n";

  return 0;
}
```

Output

```text
stage_count() = 2
map = pluss::Series(shape=[2,5], strides=[5,1], dtype=float64, platform=cpu, numel=10, contiguous=true)
data=[[-1, 1, 0, 0, 100],
  [1, 0, 1, 0, 200]]
```

### Example 2

`add_pipeline` chains two transformers within one stage — `PowerMap` then
`StandardScaler`, run one after the other on the same routed column.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <memory>
#include <vector>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::FeatureTransformer transformer;  // remainder = Drop by default
  std::vector<std::unique_ptr<cls::TransformerBase>> pipeline{};
  pipeline.push_back(std::make_unique<cls::prep::PowerMap>());
  pipeline.push_back(std::make_unique<cls::prep::StandardScaler>());
  transformer.add_pipeline(std::move(pipeline), {0});

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({1.0, 2.0, 4.0, 8.0}, {4}, f64).reshape({4, 1});
  const pluss::Series mapped = transformer.learn_map(batch).value();
  std::cout << "map = " << mapped << "\n";

  return 0;
}
```

Output

```text
map = pluss::Series(shape=[4,1], strides=[1,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[-1.3099764],
  [-0.48803175],
  [0.43566288],
  [1.3623453]]
```

### Example 3

Zero stages with the default `Drop` remainder policy fails outright — there
is nothing for the transformer to do.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::FeatureTransformer empty;  // no stages, remainder = Drop
  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({1, 2});
  const cls::StatusOr<pluss::Series> mapped = empty.learn_map(batch);
  std::cout << "ok = " << std::boolalpha << mapped.ok() << "\n";
  std::cout << "message = \"" << mapped.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "FeatureTransformer::learn_map requires at least one stage when remainder policy is Drop."
```

## Changelog

- **0.1** — `add_stage`/`add_pipeline`/`clear_stages`/`stage_count`,
  `remainder_policy`/`set_remainder_policy`, `learn`/`map`/`learn_map`,
  `dump_state`/`load_state`.

## See Also

- [`compose::Flow`](../../compose/Flow.md) — the same routing/remainder
  mechanics at the whole-pipeline level, with a model attached.
- [`FunctionTransformer`](FunctionTransformer.md) — often one of the
  transformers routed inside a `FeatureTransformer` stage.
- [`Flow::add_feature_transformer`](../../compose/Flow/add_feature_transformer.md) —
  nesting this as one stage of a larger `Flow`.
- [`transform/Introduction`](Introduction.md) — the parent sub-family.

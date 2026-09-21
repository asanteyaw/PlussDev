<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b50a25b9a6c7 -->
# `pluss::ai::classic::Flow::add_feature_transformer`

`add_feature_transformer` wraps a pre-built `prep::FeatureTransformer` — a
small, self-contained routed sub-pipeline of one or more transformers, with
its own remainder policy — as a single [`Flow`](../Flow.md) stage. Reach for
it when several columns need independent, differently-routed treatment as one
logical unit (rather than several separate
[`add_stage`](add_stage.md)/[`add_named_transform`](add_named_transform.md)
calls), or when you are composing a reusable sub-pipeline that needs its own
remainder control. Like [`add_function_transformer`](add_function_transformer.md),
it forwards to [`append_transform`](append_transform.md) internally, so the
resulting stage is **opaque**: it does its own internal routing on the whole
batch (`Flow`'s own `feature_route` does not apply to it), and adding one sets
[`has_opaque_transforms()`](../Flow.md#state) — which blocks
[`clone()`](clone.md) and blueprint round-tripping.

## Signature

```cpp
classic::Status classic::Flow::add_feature_transformer(classic::prep::FeatureTransformer transformer);
```

## Parameters

`transformer` — `prep::FeatureTransformer`, by value (moved in)
: Built independently of the `Flow`, via its own constructor and methods:

  ```cpp
  classic::prep::FeatureTransformer sub(classic::prep::FeatureRemainderPolicy::PassThrough);  // ctor arg default: Drop
  sub.add_stage(std::make_unique<classic::prep::StandardScaler>(), /* feature_route */ {0});
  sub.add_pipeline(std::move(some_transformers), /* feature_route */ {1, 2});  // chain several on one route
  sub.clear_stages();
  ```

  `add_stage` is a one-transformer convenience over `add_pipeline`, which
  chains several transformers on the same route in sequence. Every stage's
  `feature_route` is required and non-empty (unlike a `Flow`-level `add_*`
  call, where an empty route means "every column"). `remainder_policy()`
  controls what happens to columns no stage's route names: `Drop` (the
  constructor default) removes them from this sub-pipeline's output;
  `PassThrough` carries them through unchanged.

## Returns

A `Status`. `.ok()` is `true` whenever `transformer` is non-null (mirrors
[`append_transform`](append_transform.md)) — the stage is appended and
[`has_opaque_transforms()`](../Flow.md#state) updates immediately. The
sub-pipeline's own stages are not fit until [`learn`](learn.md)/[`learn_map`](learn_map.md)
runs.

## Errors

- `Status::invalid_argument` — a default-constructed `transformer`.
- The sub-pipeline's own preconditions surface later, from
  [`map`](map.md)/[`learn`](learn.md)/[`learn_map`](learn_map.md): calling
  `map()`/`infer()`-style access before a `learn()`/`learn_map()` call fails
  with `FeatureTransformer::map requires learn() before map().` — see
  Example 3.

## Practical Examples

### Example 1

One routed stage with `PassThrough` remainder: column 0 is scaled, column 1
(named by no stage) carries through unchanged.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::FeatureTransformer sub(cls::prep::FeatureRemainderPolicy::PassThrough);
  sub.add_stage(std::make_unique<cls::prep::StandardScaler>(), {0});

  cls::Flow flow{};
  const cls::Status s = flow.add_feature_transformer(std::move(sub));
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "has_opaque_transforms() = " << flow.has_opaque_transforms() << "\n";

  const pluss::Series x = pluss::make_series({1.0, 10.0, 2.0, 20.0, 3.0, 30.0}, {6}, f64).reshape({3, 2});
  cls::FeatureBatch batch{};
  batch.x = x;
  const pluss::Series mapped = flow.learn_map(batch).value();
  std::cout << "mapped = " << mapped << "\n";

  return 0;
}
```

Output

```text
ok = true
has_opaque_transforms() = true
mapped = pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-1.2247449, 10],
  [0, 20],
  [1.2247449, 30]]
```

### Example 2

Two routed stages plus the constructor's default `Drop` remainder: the third
column, named by no stage, is dropped from the output entirely — the
sub-pipeline's output width is the sum of its stages' outputs, not the
original column count.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::FeatureTransformer sub{};  // default remainder = Drop
  sub.add_stage(std::make_unique<cls::prep::StandardScaler>(), {0});
  sub.add_stage(std::make_unique<cls::prep::StandardScaler>(), {1});
  std::cout << "sub.stage_count() = " << sub.stage_count() << "\n";
  std::cout << "sub.remainder_policy() == Drop: "
            << std::boolalpha << (sub.remainder_policy() == cls::prep::FeatureRemainderPolicy::Drop) << "\n";

  cls::Flow flow{};
  flow.add_feature_transformer(std::move(sub));

  // 3 columns in; column 2 is named by no stage.
  const pluss::Series x =
      pluss::make_series({1.0, 10.0, 100.0, 2.0, 20.0, 200.0, 3.0, 30.0, 300.0}, {9}, f64).reshape({3, 3});
  cls::FeatureBatch batch{};
  batch.x = x;
  const pluss::Series mapped = flow.learn_map(batch).value();
  std::cout << "mapped = " << mapped << "\n";

  return 0;
}
```

Output

```text
sub.stage_count() = 2
sub.remainder_policy() == Drop: true
mapped = pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-1.2247449, -1.2247449],
  [0, 0],
  [1.2247449, 1.2247449]]
```

### Example 3

`map()` before `learn()`/`learn_map()` — the sub-pipeline's own precondition,
surfaced through [`Flow::map`](map.md) unchanged.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::FeatureTransformer sub(cls::prep::FeatureRemainderPolicy::PassThrough);
  sub.add_stage(std::make_unique<cls::prep::StandardScaler>(), {0});

  cls::Flow flow{};
  flow.add_feature_transformer(std::move(sub));

  const pluss::Series x = pluss::make_series({1.0, 10.0, 2.0, 20.0}, {4}, f64).reshape({2, 2});
  cls::FeatureBatch batch{};
  batch.x = x;

  // map() without a prior learn()/learn_map() call.
  const cls::StatusOr<pluss::Series> mapped = flow.map(batch);
  std::cout << "map ok = " << std::boolalpha << mapped.ok() << "\n";
  std::cout << "map message = \"" << mapped.status().message << "\"\n";

  return 0;
}
```

Output

```text
map ok = false
map message = "FeatureTransformer::map requires learn() before map()."
```

## See Also

- [`add_function_transformer`](add_function_transformer.md) — the same
  opaque-stage mechanism, for a bare callable instead of a sub-pipeline.
- [`append_transform`](append_transform.md) — the general primitive both of
  these forward to.
- [`clone`](clone.md) / [`save_blueprint`](save_blueprint.md) — the two
  operations an opaque stage blocks.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="add_column_stage.html">
    <span class="label">Previous</span>
    <span class="title">add_column_stage</span>
  </a>
  <a class="next-link" href="add_function_transformer.html">
    <span class="label">Next</span>
    <span class="title">add_function_transformer</span>
  </a>
</div>
<!-- apiref:nav:end -->

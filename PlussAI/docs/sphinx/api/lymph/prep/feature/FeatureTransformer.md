# `lymph::prep::FeatureTransformer`

`FeatureTransformer` is the fitted composite object of the
[`prep::feature`](../feature.md) subfamily: it holds an ordered set of
column-routed stage pipelines (the **branches**), fits them all in one `learn`,
and in `map` runs each branch on its slice of the input and concatenates the
results — plus the remainder — into one feature matrix.

You rarely construct it directly. Build one with
[`prep::feature_transformer(...)`](feature_transformer.md), then either hand it
to a model with [`Model::feature_transformer`](../../models/Model/feature_transformer.md) or
drive it directly on a `FeatureBatch`.

## Scope And Alias

The type is `lymph::prep::FeatureTransformer`, re-exported into `lymph::prep`
from the transform layer. It is **move-only** — copy construction and copy
assignment are deleted — so it is passed by `std::move`.

## Construction

```cpp
explicit FeatureTransformer(FeatureRemainderPolicy remainder = FeatureRemainderPolicy::Drop);
```

The direct constructor makes an empty transformer with a remainder policy;
branches are then added with the [low-level assembly hooks](#add_stage--add_pipeline--clear_stages).
In practice [`prep::feature_transformer(...)`](feature_transformer.md) does both
in one call — and note its remainder default is `PassThrough`, not `Drop`.

```cpp
enum class FeatureRemainderPolicy : std::uint8_t { Drop = 0, PassThrough = 1 };
```

## State

Before `learn` the transformer holds its branch definitions and remainder
policy; [`trained()`](#trained) reads `false` and [`map`](#map) fails. `learn`
fits every branch's pipeline and records the output feature count. Re-running
`learn` refits from scratch.

## Methods

### `trained`

```cpp
bool trained() const noexcept;
```

`true` once [`learn`](#learn) or [`learn_map`](#learn_map) has completed for
every branch.

### `stage_count`

```cpp
std::int64_t stage_count() const noexcept;
```

The number of branches on the transformer (each `compose` / builder in the
factory call is one). Not the total stage count across branches.

### `remainder_policy` / `set_remainder_policy`

```cpp
FeatureRemainderPolicy remainder_policy() const noexcept;
void set_remainder_policy(FeatureRemainderPolicy policy) noexcept;
```

Read or change what happens to columns no branch claims — `Drop` removes them,
`PassThrough` appends them unchanged after the branch outputs. See
[the remainder policy](feature_transformer.md#the-remainder-policy).

### `learn`

```cpp
Status learn(const FeatureBatch& batch, const TrainOptions& options = {});
```

Fit every branch's pipeline from `batch`. Wrap the returned `Status` with
`lymph::resolve(...)`. A `FeatureBatch` comes from a
[`workflow::Payload`](../../model/Payload.md) (`payload::make(frame).features({...}).feature_batch()`)
or from the model that owns the transformer.

### `map`

```cpp
StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const;
```

Run each fitted branch on `batch` and concatenate. Returns the rank-2 feature
`Series` (`[rows, output_features]`). `const`; fails if [`trained()`](#trained)
is `false`.

### `learn_map`

```cpp
StatusOr<Series> learn_map(const FeatureBatch& batch,
                           const TrainOptions& learn_options = {},
                           const InferOptions& infer_options = {});
```

`learn` then `map` in one call — the shape used when fitting and transforming
the same batch once.

### `add_stage` / `add_pipeline` / `clear_stages`

```cpp
Status add_stage(/* one transformer */,     /* integer column route */);
Status add_pipeline(/* several transformers */, /* integer column route */);
void   clear_stages() noexcept;
```

The low-level assembly hooks behind the factory. `add_stage` appends one branch
whose route (given as integer column positions) runs a single transformer;
`add_pipeline` appends one branch whose route runs several in sequence;
`clear_stages` drops every branch, returning the transformer to empty and
untrained. The [`branch`](branch.md) surface and
[`feature_transformer(...)`](feature_transformer.md) are the higher-level way to
do all of this.

### `dump_state` / `load_state`

```cpp
StatusOr<std::vector<Series>> dump_state() const;
Status load_state(/* a range of Series */ state);
```

Serialize / restore the fitted parameters of every branch — the hook the model
uses to persist a transformer inside a saved model.

## Practical Examples

<!-- these examples establish: the concatenation contract, the remainder policy split, and model integration. -->

### Example 1

Two branches — scale two numeric columns, one-hot a categorical — over a small
frame, driven directly with `learn_map`. The output is the branch outputs
concatenated left to right; `Drop` keeps only those.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income",  Thread(pluss::make_series({1., 2., 3., 4.}, {4}, f64))},
      {"region",  Thread(pluss::make_series({0., 1., 0., 1.}, {4}, f64))},
      {"balance", Thread(pluss::make_series({10., 20., 30., 40.}, {4}, f64))},
  });

  lymph::prep::FeatureTransformer transformer = lymph::prep::feature_transformer(
      raw,
      {
          lymph::prep::branch::standard_scaler({}, {"income", "balance"}),
          lymph::prep::branch::one_hot_encoder({}, {"region"}),
      },
      lymph::prep::FeatureRemainderPolicy::Drop);

  const lymph::FeatureBatch batch =
      lymph::workflow::payload::make(raw).features({"income", "region", "balance"}).feature_batch();
  const Series out = lymph::resolve(transformer.learn_map(batch),
                                    "feature_transformer.learn_map");

  std::cout << "trained     = " << std::boolalpha << transformer.trained() << "\n";
  std::cout << "stage_count = " << transformer.stage_count() << "\n";
  std::cout << "output shape = [" << out.sizes()[0] << ", " << out.sizes()[1] << "]\n";
  std::cout << "columns: income(scaled) balance(scaled) region=0 region=1 region_unknown\n\n";
  std::cout << out << "\n";

  return 0;
}
```

Output

```text
trained     = true
stage_count = 2
output shape = [4, 5]
columns: income(scaled) balance(scaled) region=0 region=1 region_unknown

pluss::Series(shape=[4,5], strides=[5,1], dtype=float64, platform=cpu, numel=20, contiguous=true)
data=[[-1.3416408, -1.3416408, 1, 0, 0],
  [-0.4472136, -0.4472136, 0, 1, 0],
  [0.4472136, 0.4472136, 1, 0, 0],
  [1.3416408, 1.3416408, 0, 1, 0]]
```

### Example 2

The remainder — every routed column no branch claimed — under each policy.
`tenure` is unrouted here.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1., 2., 3., 4.}, {4}, f64))},
      {"tenure", Thread(pluss::make_series({7., 8., 9., 10.}, {4}, f64))},
  });
  const auto batch = [&] {
    return lymph::workflow::payload::make(raw).features({"income", "tenure"}).feature_batch();
  };

  for (const auto policy : {lymph::prep::FeatureRemainderPolicy::Drop,
                            lymph::prep::FeatureRemainderPolicy::PassThrough}) {
    lymph::prep::FeatureTransformer t = lymph::prep::feature_transformer(
        raw, {lymph::prep::branch::standard_scaler({}, {"income"})}, policy);
    const Series out = lymph::resolve(t.learn_map(batch()), "learn_map");
    std::cout << (policy == lymph::prep::FeatureRemainderPolicy::Drop ? "Drop" : "PassThrough")
              << " -> shape [" << out.sizes()[0] << ", " << out.sizes()[1] << "]\n"
              << out << "\n\n";
  }

  return 0;
}
```

Output

```text
Drop -> shape [4, 1]
pluss::Series(shape=[4,1], strides=[1,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[-1.3416408],
  [-0.4472136],
  [0.4472136],
  [1.3416408]]

PassThrough -> shape [4, 2]
pluss::Series(shape=[4,2], strides=[2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[-1.3416408, 7],
  [-0.4472136, 8],
  [0.4472136, 9],
  [1.3416408, 10]]
```

### Example 3

Handed to a model, which fits and replays it as the first step of `learn` /
`infer`. `feature_transformer` is move-only, so it is passed with `std::move`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame train({
      {"income",  Thread(pluss::make_series({1.0, 1.5, 2.0, 3.5, 4.0, 4.5}, {6}, f64))},
      {"region",  Thread(pluss::make_series({0., 1., 0., 1., 0., 1.}, {6}, f64))},
      {"target",  Thread(pluss::make_series({0., 0., 0., 1., 1., 1.}, {6}, f64))},
  });

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "region"};
  data_options.label_feature = "target";

  lymph::prep::FeatureTransformer preprocessing = lymph::prep::feature_transformer(
      train, data_options,
      {
          lymph::prep::branch::standard_scaler({}, {"income"}),
          lymph::prep::branch::one_hot_encoder({}, {"region"}),
      },
      lymph::prep::FeatureRemainderPolicy::PassThrough);

  lymph::models::Model model = lymph::models::logistic_regression();
  model.feature_transformer(std::move(preprocessing));
  model.learn_(train, data_options);

  std::cout << "trained = " << std::boolalpha << model.trained() << "\n";
  std::cout << DataFrame({
      {"income",    Thread(train.features("income"))},
      {"region",    Thread(train.features("region"))},
      {"actual",    Thread(train.features("target"))},
      {"predicted", Thread(model.infer(train, data_options))},
  });

  return 0;
}
```

Output

```text
trained = true
========================================
     | income  region  actual  predicted
----------------------------------------
0    | 1       0       0       0        
1    | 1.5     1       0       0        
2    | 2       0       0       0        
3    | 3.5     1       1       1        
4    | 4       0       1       1        
5    | 4.5     1       1       1        
----------------------------------------
[6 cs x 4 feat]
```

## Notes

- Move-only: hold it in a named local and `std::move` it into the model.
- `stage_count()` counts **branches**, not stages.
- Branch outputs are concatenated in the order the branches were given; the
  remainder (if kept) comes last.
- A branch's route may be given as column **names** or integer **positions** —
  see [`prep::branch`](branch.md).

## Changelog

- **0.1** — `FeatureTransformer` with `learn` / `map` / `learn_map` /
  `trained` / `stage_count`, the remainder policy accessors, `add_stage` /
  `add_pipeline` / `clear_stages`, and `dump_state` / `load_state`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

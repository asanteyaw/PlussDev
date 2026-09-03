<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 641e618808a8 -->

# `lymph::models::Model::feature_transformer`

[Back to `lymph::models::Model`](../Model.md)

`feature_transformer` attaches a pre-built
[`prep::FeatureTransformer`](../../prep/feature/FeatureTransformer.md) — a
multi-branch, column-routed preprocessing contract — to the model. The model
fits it and replays it as the **first** step of `learn` / `infer`, before any
estimator sees the frame.

Use it when the preprocessing plan has structure that a chain of single
[`append`](append.md) stages does not express cleanly: different column groups
needing different pipelines, run in parallel and concatenated.

## Signature

```cpp
lymph::models::Model& lymph::models::Model::feature_transformer(
    lymph::prep::FeatureTransformer transformer);

// Sibling -- attach a whole-frame Series -> Series transform instead:
lymph::models::Model& lymph::models::Model::function_transformer(
    lymph::prep::FunctionTransformer transformer);
```

## What It Does

The passed `FeatureTransformer` is stored on the model. Nothing is fitted yet.
When [`learn`](learn.md) runs, the model fits every branch of the transformer on
the training frame, then feeds the concatenated branch output (plus the kept
remainder) to the estimator. [`infer`](infer.md) replays the same fitted
transform.

Build the argument with
[`prep::feature_transformer(...)`](../../prep/feature/feature_transformer.md) and
its [`branch`](../../prep/feature/branch.md) helpers. It is **move-only**, so it
is passed with `std::move`.

`function_transformer` is the sibling for the single-function case: it takes a
[`prep::FunctionTransformer`](../../prep/feature/feature_transformer.md) wrapping
one `Series -> Series` callable applied to the whole feature frame.
[`Model::add(prep::stage::custom(...))`](../../prep/stage/custom.md) is the
ergonomic wrapper around it.

## Parameters

`lymph::prep::FeatureTransformer transformer` (or
`lymph::prep::FunctionTransformer` for `function_transformer`)
: The pre-built, unfitted transform to attach. Move-only — pass with
  `std::move`. Constructed via
  [`prep::feature_transformer(...)`](../../prep/feature/feature_transformer.md).

## Returns

`Model&` — the same model, so the call chains with other configuration calls.
The attachment mutates the model in place.

## Errors

- Fitting errors from a branch's own stages surface through [`learn`](learn.md).
- A branch route naming a column absent from the transformer's feature space is
  rejected when the transformer is built, not here.

## Composition with `append`

A `FeatureTransformer` and [`append`](append.md)'d stages compose: the
transformer runs **first** (fitting and replaying every branch), then each
appended stage sees its concatenated output. So `append` a scaler on `"spend"`
after a transformer that one-hot-expands `"region"`, and the scaler operates on
the widened frame. Example 3 shows this.

## Practical Examples

<!-- these examples establish: end-to-end model use, driving the transformer without a model, and composition with append. -->

### Example 1

A two-branch transformer — standardise `income`, one-hot `region` — attached to
a classifier and trained end to end.

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

### Example 2

The same transformer needs no model — drive it directly on a `FeatureBatch`
built from a [`workflow::Payload`](../../model/Payload.md).

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
  std::cout << "stage_count = " << transformer.stage_count() << " (branches)\n";
  std::cout << "output shape = [" << out.sizes()[0] << ", " << out.sizes()[1] << "]\n";
  std::cout << "columns: income(scaled) balance(scaled) region=0 region=1 region_unknown\n\n";
  std::cout << out << "\n";

  return 0;
}
```

Output

```text
trained     = true
stage_count = 2 (branches)
output shape = [4, 5]
columns: income(scaled) balance(scaled) region=0 region=1 region_unknown

pluss::Series(shape=[4,5], strides=[5,1], dtype=float64, platform=cpu, numel=20, contiguous=true)
data=[[-1.3416408, -1.3416408, 1, 0, 0],
  [-0.4472136, -0.4472136, 0, 1, 0],
  [0.4472136, 0.4472136, 1, 0, 0],
  [1.3416408, 1.3416408, 0, 1, 0]]
```

### Example 3

Composition with [`append`](append.md): the transformer one-hot-expands
`region`, then an appended scaler standardises `spend` on the widened frame. The
transformer runs first.

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
      {"spend",  Thread(pluss::make_series({10., 20., 30., 40., 50., 60.}, {6}, f64))},
      {"region", Thread(pluss::make_series({0., 1., 0., 1., 0., 1.}, {6}, f64))},
      {"target", Thread(pluss::make_series({0., 0., 0., 1., 1., 1.}, {6}, f64))},
  });

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"spend", "region"};
  data_options.label_feature = "target";

  lymph::prep::FeatureTransformer pre = lymph::prep::feature_transformer(
      train, data_options,
      {lymph::prep::branch::one_hot_encoder({}, {"region"})},
      lymph::prep::FeatureRemainderPolicy::PassThrough);

  lymph::models::Model model = lymph::models::logistic_regression();
  model.feature_transformer(std::move(pre));
  model.append(lymph::prep::stage::standard_scaler(),
               lymph::prep::route::names({"spend"}));
  model.learn_(train, data_options);

  std::cout << "trained = " << std::boolalpha << model.trained() << "\n";
  std::cout << DataFrame({
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
================================
     | region  actual  predicted
--------------------------------
0    | 0       0       0        
1    | 1       0       0        
2    | 0       0       0        
3    | 1       1       1        
4    | 0       1       1        
5    | 1       1       1        
--------------------------------
[6 cs x 3 feat]
```

## See Also

- [`prep::feature_transformer`](../../prep/feature/feature_transformer.md) — the
  factory that builds the argument.
- [`prep::FeatureTransformer`](../../prep/feature/FeatureTransformer.md) — the
  object, and how to drive it without a model.
- [`prep::branch`](../../prep/feature/branch.md) — describing each branch.
- [`Model::append`](append.md) — single-stage preparation, the alternative for
  flat plans.
- [`prep::stage::custom`](../../prep/stage/custom.md) — the ergonomic wrapper for
  `function_transformer`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="assess.html">
    <span class="label">Previous</span>
    <span class="title">assess</span>
  </a>
  <a class="next-link" href="infer.html">
    <span class="label">Next</span>
    <span class="title">infer</span>
  </a>
</div>
<!-- apiref:nav:end -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3e4fdac3e0cc -->

# `lymph::workflow::Workflow::infer`

[Back to `lymph::workflow::Workflow`](../Workflow.md)

## Signature

```cpp
Series Workflow::infer(const FeatureBatch& batch, const InferOptions& options = {}) const;
Series Workflow::infer(const pluss::table::DataFrame& raw_frame, const InferOptions& options = {}) const;
Series Workflow::infer(
    const pluss::table::DataFrame& raw_frame,
    const DataFrameBatchOptions& data_options,
    const InferOptions& options = {}) const;
Series Workflow::infer(const Payload& payload, const InferOptions& options = {}) const;

// Same four input shapes, returning the pre-threshold scores instead of labels:
Series Workflow::infer_scores(const FeatureBatch& batch, const InferOptions& options = {}) const;
Series Workflow::infer_scores(const pluss::table::DataFrame& raw_frame, const InferOptions& options = {}) const;
Series Workflow::infer_scores(
    const pluss::table::DataFrame& raw_frame,
    const DataFrameBatchOptions& data_options,
    const InferOptions& options = {}) const;
Series Workflow::infer_scores(const Payload& payload, const InferOptions& options = {}) const;
```

## What It Does

`infer` predicts labels for new data with a trained workflow, and `infer_scores` returns the scores before thresholding. Each accepts the four input shapes a workflow takes. A raw frame is first run through the workflow's policy, then through the held model.

## What `Workflow` Adds

`infer`/`infer_scores` on a `Workflow` is `Policy::apply(...)` (on any
raw-frame argument) followed by the identically-shaped call on the held
`Model`. **`Policy` runs on every call**, not just once at training time — new
incoming data can still carry a column `Policy` drops or forbids, and it is
stripped the same way each time (Example 3). The prediction mechanics
themselves — replaying the fitted preparation, then the estimator, then
(for a classifier) thresholding the decision score into a hard label — are
entirely [`models::Model::infer`](../../models/Model/infer.md)'s concern.

## Input Shapes

Three of the four shapes are identical to `Model::infer`'s — see
[Input Shapes there](../../models/Model/infer.md#input-shapes) for the full
treatment. Named here: `batch` (a pre-assembled `FeatureBatch`), `raw_frame`
alone or with `data_options` (naming the feature columns). The fourth shape
is `Workflow`-only:

`payload`
: A [`Payload`](../Payload.md). Its `feature_batch()` (or frame, run through
  `Policy` first) is used; a label role on it, if any, is not read — `infer`
  never needs a label (Example 2).

`options` (`lymph::options::Infer`) is unchanged from `Model::infer` — see
[its Parameters section](../../models/Model/infer.md#parameters) for the full
field list (`positive_threshold`, `missing`, `unknown_categorical`, `layout`).

## Returns

Same as `Model::infer`/`infer_scores`: a `Series` of length equal to the
number of input rows (fewer if `missing = Drop` removed some) — hard labels
from `infer`, decision scores from `infer_scores`, continuous predictions for
a regressor from either.

## Errors

- Calling either method before the `Workflow` is trained raises the same
  error `Model::infer` raises ("Flow must be learned before infer").
- Every other error `Model::infer` can raise (a missing feature column, an
  unseen category under the default `unknown_categorical = Error`) applies
  unchanged.
- Unlike [`map`](map.md), `infer`/`infer_scores` do not separately guard
  against a pending `on(...)`-appended stage — a `Workflow` that is
  `trained()` has, by construction, already resolved every stage through the
  `learn`/`learn_` call that trained it.

## Practical Examples

### Example 1

Baseline: `infer` for hard labels, `infer_scores` for the underlying decision
scores each label came from.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Slate(pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64))},
      {"balance", Slate(pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64))},
      {"default", Slate(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  workflow::Workflow wf = workflow::make(lymph::models::logistic_regression());

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  const workflow::Workflow trained = wf.learn(raw, data_options);

  const pluss::Series labels = trained.infer(raw, data_options);
  const pluss::Series scores = trained.infer_scores(raw, data_options);
  std::cout << "labels = " << labels << "\n";
  std::cout << "scores = " << scores << "\n";

  return 0;
}
```

Output

```text
labels = pluss::Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[0, 0, 0, 0, 1, 1, 1, 1]
scores = pluss::Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[-24.2787, -20.308394, -16.03298, -9.9245761, 9.926955, 15.424362, 20.92177, 27.335281]
```

### Example 2

`infer(payload)`, with the `Payload` holding no label role at all — `infer`
only ever reads the feature side.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Slate(pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64))},
      {"balance", Slate(pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64))},
  });
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);

  const workflow::Payload train_payload = workflow::payload::make(raw).features({"income", "balance"}).label(y);
  const workflow::Workflow trained =
      workflow::make(lymph::models::logistic_regression()).learn(train_payload);

  const workflow::Payload query_payload = workflow::payload::make(raw).features({"income", "balance"});
  const pluss::Series pred = trained.infer(query_payload);
  std::cout << "pred = " << pred << "\n";

  return 0;
}
```

Output

```text
pred = pluss::Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[0, 0, 0, 0, 1, 1, 1, 1]
```

### Example 3

`Policy` runs on every `infer` call, not only at `learn` time: new incoming
rows still carry `customer_id`, and it is dropped exactly as it was during
training.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame train_raw({
      {"customer_id", Slate(pluss::make_series({1, 2, 3, 4, 5, 6, 7, 8}, {8}, f64))},
      {"income", Slate(pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64))},
      {"balance", Slate(pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64))},
      {"default", Slate(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  workflow::Policy policy{};
  policy.drop({"customer_id"});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  const workflow::Workflow trained =
      workflow::make(lymph::models::logistic_regression(), policy).learn(train_raw, data_options);

  const DataFrame incoming({
      {"customer_id", Slate(pluss::make_series({9, 10, 11}, {3}, f64))},
      {"income", Slate(pluss::make_series({1.2, 4.4, 1.6}, {3}, f64))},
      {"balance", Slate(pluss::make_series({0.3, 2.3, 0.5}, {3}, f64))},
  });

  const pluss::Series pred = trained.infer(incoming, data_options);
  std::cout << "pred = " << pred << "\n";

  return 0;
}
```

Output

```text
pred = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 1, 0]
```

## See Also

- [`models::Model::infer`](../../models/Model/infer.md) — the prediction
  mechanics `Workflow::infer` delegates to.
- [`learn`](learn.md) — produces the trained state `infer` reads.
- [`assess`](assess.md) — `infer` plus a scoring step against known labels.
- [`Policy`](../Policy.md) — applied on every call, not just at `learn` time.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="assess.html">
    <span class="label">Previous</span>
    <span class="title">assess</span>
  </a>
  <a class="next-link" href="learn.html">
    <span class="label">Next</span>
    <span class="title">learn</span>
  </a>
</div>
<!-- apiref:nav:end -->

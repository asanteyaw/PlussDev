<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 710eb1342645 -->

# `lymph::models::Model::infer`

[Back to `lymph::models::Model`](../Model.md)

`infer` runs a trained model on new feature rows and returns its predictions. The
companion `infer_scores` returns the raw decision scores that `infer` thresholds
into labels.

## Signature

```cpp
lymph::Series lymph::models::Model::infer(
    const pluss::table::DataFrame& x,
    const lymph::options::Infer& options = {}) const;

lymph::Series lymph::models::Model::infer(
    const pluss::table::DataFrame& frame,
    const lymph::options::DFBatch& data_options,
    const lymph::options::Infer& options = {}) const;

lymph::Series lymph::models::Model::infer(
    const lymph::FeatureBatch& batch,
    const lymph::options::Infer& options = {}) const;

// Same three input shapes, returning the pre-threshold scores instead of labels:
lymph::Series lymph::models::Model::infer_scores(
    const pluss::table::DataFrame& x,
    const lymph::options::Infer& options = {}) const;
// ... (DFBatch and FeatureBatch overloads as above)
```

## What It Does

`infer` takes raw feature columns — the same schema training saw — replays the
model's fitted preparation over them, runs the estimator, and returns one value
per input row as a `pluss::Series`.

For a classifier the returned values are hard labels. `infer` produces them by
taking the estimator's decision score for each row and comparing it against a
threshold: by default, the score that corresponds to probability `0.5`. For a
regressor the returned values are the continuous predictions directly.

`infer_scores` stops one step earlier and returns those decision scores
themselves — log-odds for a linear classifier. Two uses: inspecting how confident
the model is (a score near zero is a borderline row), and applying your own
threshold downstream instead of `positive_threshold`.

The feature contract is fixed at training time. Pass the untransformed columns;
the model's scalers, encoders, and imputers run inside `infer`. Passing a frame
you have already transformed yourself produces silently wrong predictions.
`infer` does not need the label column — omit it, or leave it present and unused
when you go through the `DFBatch` overload (Example 3).

## Input Shapes

`const pluss::table::DataFrame& x`
: A frame of exactly the feature columns. The everyday form.

`const pluss::table::DataFrame& frame, const lymph::options::DFBatch& data_options`
: A frame that may still carry the label column (or other extras), plus a batch
  naming the feature columns. Match this to how the model was trained; only the
  named columns are read.

`const lymph::FeatureBatch& batch`
: A pre-assembled row-major `samples x features` tensor in `batch.x`. Use it when
  the features already live in one numeric tensor.

## Parameters

### `options` — `lymph::options::Infer`

`double positive_threshold` — default `0.5`
: Classification only. The probability a row's score must reach before `infer`
  labels it `1`. Raise it toward `1.0` to make the model conservative about
  positives — borderline rows fall back to `0` (Example 2). Lower it to catch
  more positives at the cost of false alarms. `infer_scores` ignores this
  field.

`MissingPolicy missing` — default `MissingPolicy::Reject`
: What to do with `NaN` in the incoming feature columns. `Reject` fails;
  `Drop` skips affected rows (the result is then shorter than the input);
  `ImputeMean` / `ImputeMedian` fill from statistics computed at inference time;
  `KeepNaN` passes them through. If the fill should match training, attach an
  imputer stage instead.

`UnknownCategoryPolicy unknown_categorical` — default `UnknownCategoryPolicy::Error`
: What a fitted encoder does when it meets a category it never saw during
  training. `Error` stops; `MissingBucket` routes it to a catch-all column;
  `Ignore` drops it.

`ObservationLayout layout` — default `ObservationLayout::Rows`
: Whether observations run down the rows (the normal case) or across the columns
  of a `FeatureBatch` tensor.

## Returns

A `pluss::Series` of length equal to the number of input rows (fewer if
`missing = Drop` removed some). Hard labels for a classifier from `infer`;
decision scores from `infer_scores`; continuous predictions for a regressor from
either.

## Errors

- Calling `infer` on a model that was never trained, or on the template left
  behind by the value-returning [`learn`](learn.md), raises `pluss::Error`
  ("Flow must be learned before infer").
- A feature column named at training time that is missing from the inference
  frame is an error.
- An unseen category with the default `unknown_categorical = Error` stops
  inference.

## Practical Examples

### Example 1

Predict labels for rows the model has never seen. Train on one frame, build a
small frame of new applicants, and read one label per row.

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

  const pluss::Series income = pluss::make_series({1.0, 1.4, 1.8, 3.4, 3.8, 4.2}, {6}, f64);
  const pluss::Series balance = pluss::make_series({0.3, 0.5, 0.7, 1.8, 2.1, 2.4}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x_train({{"income", Thread(income)}, {"balance", Thread(balance)}});

  const lymph::models::Model model = lymph::models::logistic_regression().learn(x_train, y);

  // Predict for three applicants the model has never seen.
  const pluss::Series new_income = pluss::make_series({1.2, 2.6, 4.0}, {3}, f64);
  const pluss::Series new_balance = pluss::make_series({0.4, 1.2, 2.2}, {3}, f64);
  const DataFrame x_new({{"income", Thread(new_income)}, {"balance", Thread(new_balance)}});

  const DataFrame result({
      {"income", Thread(new_income)},
      {"balance", Thread(new_balance)},
      {"predicted_label", Thread(model.infer(x_new))},
  });
  std::cout << result;

  return 0;
}
```

Output

```text
=======================================
     | income  balance  predicted_label
---------------------------------------
0    | 1.2     0.4      0              
1    | 2.6     1.2      0              
2    | 4       2.2      1              
---------------------------------------
[3 cs x 3 feat]
```

### Example 2

`infer` versus `infer_scores`, and what `positive_threshold` does. The score
column shows the raw log-odds; raising the threshold to `0.95` makes row 2 — a
weak positive at score `2.28` — fall back to label `0`.

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

  // Rows 2 and 3 sit right on the boundary and carry mixed labels, so the
  // fitted scores near the middle are close to the decision point.
  const pluss::Series income = pluss::make_series({1.0, 1.8, 2.6, 2.8, 3.6, 4.4}, {6}, f64);
  const pluss::Series balance = pluss::make_series({0.5, 1.0, 1.5, 1.6, 2.1, 2.6}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 1, 0, 1, 1}, {6}, f64);
  const DataFrame x({{"income", Thread(income)}, {"balance", Thread(balance)}});

  const lymph::models::Model model = lymph::models::logistic_regression().learn(x, y);

  // infer_scores is the raw decision score. infer thresholds it: by default a
  // score above the point that corresponds to probability 0.5 becomes label 1.
  // Raising positive_threshold makes the model demand more evidence before it
  // says 1, so borderline rows flip back to 0.
  lymph::options::Infer strict{};
  strict.positive_threshold = 0.95;

  const DataFrame compare({
      {"actual", Thread(y)},
      {"score", Thread(model.infer_scores(x))},
      {"label_at_0_5", Thread(model.infer(x))},
      {"label_at_0_95", Thread(model.infer(x, strict))},
  });
  std::cout << compare;

  return 0;
}
```

Output

```text
========================================================
     | actual  score         label_at_0_5  label_at_0_95
--------------------------------------------------------
0    | 0       -11.52245368  0             0            
1    | 0       -4.619419853  0             0            
2    | 1       2.283613978   1             0            
3    | 0       -2.283613978  0             0            
4    | 1       4.619419853   1             1            
5    | 1       11.52245368   1             1            
--------------------------------------------------------
[6 cs x 4 feat]
```

### Example 3

The `DataFrame + DFBatch` overload. The model was trained through that contract,
so inference uses it too: the scoring frame may still carry a `defaulted` column,
and `infer` ignores it because the batch lists only the two feature columns.

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

  // The model was trained through the DataFrame + DFBatch contract, so
  // inference uses the same contract: a frame that may still carry the label
  // column, plus a batch that names the feature columns.
  const pluss::Series income = pluss::make_series({1.0, 1.4, 1.8, 3.4, 3.8, 4.2}, {6}, f64);
  const pluss::Series balance = pluss::make_series({0.3, 0.5, 0.7, 1.8, 2.1, 2.4}, {6}, f64);
  const pluss::Series defaulted = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame train_frame({
      {"income", Thread(income)},
      {"balance", Thread(balance)},
      {"defaulted", Thread(defaulted)},
  });

  lymph::options::DFBatch batch{};
  batch.feature_names = {"income", "balance"};
  batch.label_feature = "defaulted";

  const lymph::models::Model model =
      lymph::models::logistic_regression().learn(train_frame, batch);

  // A fresh frame. It carries a `defaulted` column too (unknown, filled with
  // zeros); infer ignores it because the batch only lists the two features.
  const DataFrame score_frame({
      {"income", Thread(pluss::make_series({1.2, 2.6, 4.0}, {3}, f64))},
      {"balance", Thread(pluss::make_series({0.4, 1.2, 2.2}, {3}, f64))},
      {"defaulted", Thread(pluss::make_series({0, 0, 0}, {3}, f64))},
  });

  const DataFrame result({
      {"income", score_frame.features("income")},
      {"balance", score_frame.features("balance")},
      {"predicted", Thread(model.infer(score_frame, batch))},
  });
  std::cout << result;

  return 0;
}
```

Output

```text
=================================
     | income  balance  predicted
---------------------------------
0    | 1.2     0.4      0        
1    | 2.6     1.2      0        
2    | 4       2.2      1        
---------------------------------
[3 cs x 3 feat]
```

## See Also

- [learn](learn.md) — establishes the fitted state `infer` consumes.
- [assess](assess.md) — `infer` plus a comparison against known labels.
- [Introduction](Introduction.md) — model lifecycle and state, including why `infer` on a template raises.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="feature_transformer.html">
    <span class="label">Previous</span>
    <span class="title">feature_transformer</span>
  </a>
  <a class="next-link" href="learn.html">
    <span class="label">Next</span>
    <span class="title">learn</span>
  </a>
</div>
<!-- apiref:nav:end -->

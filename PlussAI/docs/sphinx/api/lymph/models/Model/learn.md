<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a1237ea723a0 -->

# `lymph::models::Model::learn`

[Back to `lymph::models::Model`](../Model.md)

`learn` fits a model: it resolves and fits every attached preparation stage, then
fits the estimator on the prepared features. One page covers `learn_` and
`learn` because they are the same operation with two ownership styles.

## Signature

```cpp
// In place: the left-hand model becomes the fitted artifact.
lymph::models::Model& lymph::models::Model::learn_(
    const lymph::LabeledBatch& batch,
    const lymph::options::Train& options = {});

lymph::models::Model& lymph::models::Model::learn_(
    const pluss::table::DataFrame& frame,
    const lymph::options::DFBatch& data_options,
    const lymph::options::Train& options = {});

lymph::models::Model& lymph::models::Model::learn_(
    const pluss::table::DataFrame& x,
    const pluss::Series& y,
    const lymph::options::Train& options = {});

lymph::models::Model& lymph::models::Model::learn_(
    const pluss::table::DataFrame& x,
    const pluss::table::Slate& y,
    const lymph::options::Train& options = {});

// By value: `*this` is left untouched; the trained model is returned.
lymph::models::Model lymph::models::Model::learn(
    const lymph::LabeledBatch& batch,
    const lymph::options::Train& options = {}) const;

lymph::models::Model lymph::models::Model::learn(
    const pluss::table::DataFrame& frame,
    const lymph::options::DFBatch& data_options,
    const lymph::options::Train& options = {}) const;

lymph::models::Model lymph::models::Model::learn(
    const pluss::table::DataFrame& x,
    const pluss::Series& y,
    const lymph::options::Train& options = {}) const;

lymph::models::Model lymph::models::Model::learn(
    const pluss::table::DataFrame& x,
    const pluss::table::Slate& y,
    const lymph::options::Train& options = {}) const;
```

## What It Does

A model produced by a factory such as `lymph::models::logistic_regression()` is a
*template*: it has chosen an algorithm and it may carry a preparation plan, but it
holds no learned parameters. `learn` turns that template into a fitted artifact in
two ordered phases:

1. **Preparation is fitted, stage by stage, in attachment order.** A scaler
   computes its centre and spread, an encoder records its category vocabulary, an
   imputer measures its fill values. Each stage then transforms the training
   features before the next stage sees them. Deferred routes that were described
   against column names or dtypes are resolved here, against the concrete frame,
   which is why `stage_count()` can read `0` on a configured template and only
   becomes non-zero after `learn`.
2. **The estimator is fitted** on the fully prepared feature representation,
   together with the label.

The model records both results as one unit. Every later call to
[`infer`](infer.md) or [`assess`](assess.md) replays the *same*
fitted preparation before touching the estimator, so you always hand those
methods raw feature columns in the original schema, never a pre-transformed
frame. Fitting a scaler yourself and calling `learn` on the already-scaled data
is the classic cause of train/serve skew: the model then has no scaler of its
own and silently trusts whatever you pass at inference.

`learn` does not create an evaluation split and does not decide whether your
estimator is a good choice. It fits on exactly the rows you give it. Hold-out and
cross-validation belong to the `selection` family; comparing candidates belongs
to [`Builder`](../builder/Builder.md).

## Choosing `learn_` Or `learn`

`Model` has value semantics and a cheap copy, so the two forms cost the same. The
choice is about which name ends up holding the fitted parameters.

- **`learn_`** mutates the receiver. After `model.learn_(...)`, the variable
  `model` *is* the trained artifact. Reach for this when the object on the
  left-hand side is the thing you will immediately infer, assess, or save.
- **`learn`** leaves the receiver untouched and returns the fitted model as a
  separate value. The original stays a reusable template. Reach for this when you
  want to fit the same configuration several times — different option sets,
  different data slices, a controlled comparison — without the first fit
  contaminating the next. It is also the only form callable on a `const` model.

A frequent bug is writing `fitted = template.learn(x, y)` and then calling
`template.infer(...)`: the template was never trained, only `fitted` was. See
Example 5.

## Input Shapes

All four `(features, label)` shapes reach the same fitting code; they differ only
in how you have the data on hand.

`const pluss::table::DataFrame& x, const pluss::Series& y`
: Feature columns in a frame, label in a standalone numeric `Series` of the same
  length. The lightest form; correct when the frame holds only features and the
  label lives in its own vector.

`const pluss::table::DataFrame& x, const pluss::table::Slate& y`
: Same, but the label is a `Slate` — typically one lifted from another frame
  with `frame.features("col")`. Use it to avoid a `Slate`-to-`Series`
  conversion at the call site.

`const pluss::table::DataFrame& frame, const lymph::options::DFBatch& data_options`
: One frame that contains the features *and* the label column. `data_options`
  names which columns are features and which single column is the label, so a
  label column can never be pulled in as a feature by accident. This is the form
  to prefer in reusable programs and whenever the frame carries a `Note` (text)
  column, because the batch is where text handling is stated.

`const lymph::LabeledBatch& batch`
: A pre-assembled `batch.features.x` (one row-major `samples x features` tensor)
  and `batch.y`. Use it when the features already exist as a single numeric
  tensor and building a `DataFrame` around them would be pointless ceremony.

## Parameters

### `data_options` — `lymph::options::DFBatch`

Consulted only by the `DataFrame + DFBatch` overload. Default construction is not
useful here; at minimum set the label.

`std::vector<std::string> feature_names`
: The columns to treat as features, in order. If left empty, every column except
  the label (and the sample-weight column, if set) is used. Naming them
  explicitly makes the model's input contract reviewable and stable if the frame
  later grows columns.

`std::optional<std::string> label_feature`
: The single column to fit against. Required for `learn`. Leaving it unset is the
  error you will hit first when adapting an inference example to training.

`std::optional<std::string> sample_weight_feature`
: A column to read per-row training weights from, as an alternative to passing
  `train_options.sample_weights`. The column is not treated as a feature.

`DataNotePolicy note_policy`
: How to handle text (`Note`) columns among the features. The default rejects
  them, so a frame with a text column trains only once you either state a policy
  here or attach an encoder stage that consumes the column.

### `train_options` — `lymph::options::Train`

Accepted by every overload. The fields that matter most to `learn`:

`std::int64_t max_iterations` — default `1000`
: Upper bound on the optimiser's loop for iterative estimators (linear and
  kernel classifiers, gradient-boosted trees, and similar). Closed-form fits
  ignore it. A small value produces an under-optimised model whose decision
  scores are compressed toward zero (Example 2); a generous value costs more time
  for little gain once the fit has converged.

`double tolerance` — default `1e-6`
: The convergence gate. Training also stops early, before `max_iterations`, once
  the improvement between successive steps falls below this. Lower it to squeeze
  out a tighter fit; raise it to stop sooner on noisy data.

`bool warm_start` — default `false`
: When `true`, a subsequent `learn_` on the same model continues from the current
  parameters instead of restarting from scratch. This makes it possible to train
  in bursts — useful for online-style updates or for extending a fit you decided
  was too short. With the default `false`, every `learn_` is an independent fit.

`pluss::Series sample_weights` — default undefined
: Per-row multipliers on each observation's contribution to the loss, length
  equal to the number of rows. Up-weight rows you trust more, or down-weight a
  known outlier so it barely moves the fit.

`ClassWeightMode class_weight` — default `ClassWeightMode::None`
: Classification only. `Balanced` rescales each class inversely to its frequency,
  so a rare positive class is not drowned out by a common negative one
  (Example 4). `Custom` reads an explicit pair from `class_weights`
  (`[negative_weight, positive_weight]` for a binary classifier).

`MissingPolicy missing` — default `MissingPolicy::Reject`
: What to do with `NaN` in the feature columns at fit time. `Reject` fails
  loudly; `Drop` discards affected rows; `ImputeMean` / `ImputeMedian` fill from
  column statistics; `KeepNaN` passes them through for an estimator that handles
  them natively. Prefer an explicit imputer stage when the fill itself should be
  learned once and replayed at inference.

`RandomState random` — default seeded deterministically
: The seed for estimators with a stochastic fit (bagging, random forests,
  stochastic solvers). Set it to make a run reproducible; vary it to probe
  fit-to-fit stability.

`BalanceTarget balance` — default keep-all
: Optional row resampling or cleaning applied to classification data *before* the
  estimator sees it (for example, under-sampling the majority class). The default
  leaves the training rows unchanged.

## Returns

`learn_` returns a reference to the receiver (now trained) so calls can be
chained. `learn` returns a new `Model` that is trained; the receiver is
unchanged. After either, `trained()` is `true` and `stage_count()` reflects the
preparation stages that were resolved and fitted.

## Errors

- The label column named by `data_options.label_feature` must exist in the frame;
  for the two-argument overloads the label vector's length must match the frame's
  row count.
- A text (`Note`) feature column with no stated `note_policy` and no encoder
  stage is rejected.
- A classification estimator requires a classification-shaped label; a regression
  or transform estimator has a different label contract.
- Calling `infer` or `assess` on a model that was never trained — or on
  the *template* after using the value-returning `learn` — raises
  `pluss::Error` ("Flow must be learned before infer").

## Practical Examples

The five programs below are complete: each constructs its own data, fits a model,
and prints the result with `std::cout`. The output block under each program is
its exact captured run.

### Example 1

Baseline in-place training with the `DataFrame x, Series y` overload. `learn_`
flips the model from template to fitted, and `infer` on the training frame
returns one prediction per row through the same feature contract.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // Eight applicants: two numeric features and a 0/1 default outcome.
  const pluss::Series income = pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64);
  const pluss::Series balance = pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  const DataFrame x({{"income", Slate(income)}, {"balance", Slate(balance)}});

  // A logistic-regression template: an algorithm choice, nothing learned yet.
  lymph::models::Model model = lymph::models::logistic_regression();
  std::cout << "before: trained = " << std::boolalpha << model.trained() << "\n";

  // learn_ fits in place; the name `model` now IS the fitted artifact.
  model.learn_(x, y);
  std::cout << "after:  trained = " << model.trained() << "\n\n";

  // Inference reuses the same feature contract training saw.
  const pluss::Series predicted = model.infer(x);
  const DataFrame review({
      {"income", Slate(income)},
      {"balance", Slate(balance)},
      {"actual", Slate(y)},
      {"predicted", Slate(predicted)},
  });
  std::cout << review;

  return 0;
}
```

Output

```text
before: trained = false
after:  trained = true

=========================================
     | income  balance  actual  predicted
-----------------------------------------
0    | 1       0.2      0       0        
1    | 1.3     0.4      0       0        
2    | 1.7     0.5      0       0        
3    | 2.1     0.9      0       0        
4    | 3.6     1.9      1       1        
5    | 4       2.2      1       1        
6    | 4.4     2.5      1       1        
7    | 4.9     2.8      1       1        
-----------------------------------------
[8 cs x 4 feat]
```

### Example 2

The value-returning `learn` keeps a template reusable. Two fits from one `const`
blueprint, with different iteration budgets, are independent values. Their hard
predictions happen to agree on this clean data, but `infer_scores` shows the
converged fit is far more decisive — proof the two models carry different learned
parameters.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series income = pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64);
  const pluss::Series balance = pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  const DataFrame x({{"income", Slate(income)}, {"balance", Slate(balance)}});

  // A template held by const: it must never become trained itself.
  const lymph::models::Model blueprint = lymph::models::logistic_regression();

  // learn returns a trained copy and leaves `blueprint` untouched. Two fits
  // from the same template with different iteration budgets are independent
  // values.
  lymph::options::Train converged{};
  converged.max_iterations = 800;
  const lymph::models::Model fitted_converged = blueprint.learn(x, y, converged);

  lymph::options::Train hurried{};
  hurried.max_iterations = 2;
  const lymph::models::Model fitted_hurried = blueprint.learn(x, y, hurried);

  std::cout << "blueprint.trained()        = " << std::boolalpha << blueprint.trained() << "\n";
  std::cout << "fitted_converged.trained() = " << fitted_converged.trained() << "\n";
  std::cout << "fitted_hurried.trained()   = " << fitted_hurried.trained() << "\n\n";

  // Hard predictions can agree while the underlying decision scores differ,
  // which is how you see the two fitted models really are distinct values.
  const DataFrame compare({
      {"actual", Slate(y)},
      {"score_800_iters", Slate(fitted_converged.infer_scores(x))},
      {"score_2_iters", Slate(fitted_hurried.infer_scores(x))},
  });
  std::cout << compare;

  return 0;
}
```

Output

```text
blueprint.trained()        = false
fitted_converged.trained() = true
fitted_hurried.trained()   = true

=============================================
     | actual  score_800_iters  score_2_iters
---------------------------------------------
0    | 0       -24.27869985     -3.922192476 
1    | 0       -20.30839364     -3.270724712 
2    | 0       -16.03298032     -3.023742287 
3    | 0       -9.924576073     -1.657805595 
4    | 1       9.92695497       1.599533223  
5    | 1       15.42436224      2.59248516   
6    | 1       20.92176951      3.585437097  
7    | 1       27.33528087      4.546888451  
---------------------------------------------
[8 cs x 3 feat]
```

### Example 3

The explicit `DataFrame + DFBatch` contract. One frame holds `income`, `balance`,
and the `defaulted` label together; the batch states which columns are features
and which is the label, so `defaulted` cannot leak into the feature set. The same
frame and batch are handed to `infer`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // One frame carries the features and the label side by side, the way a
  // real table loaded from disk usually does.
  const pluss::Series income = pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64);
  const pluss::Series balance = pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64);
  const pluss::Series defaulted = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  const DataFrame frame({
      {"income", Slate(income)},
      {"balance", Slate(balance)},
      {"defaulted", Slate(defaulted)},
  });

  // The batch spells out the data contract: which columns are features and
  // which single column is the label. `defaulted` can no longer be pulled in
  // as a feature by accident.
  lymph::options::DFBatch batch{};
  batch.feature_names = {"income", "balance"};
  batch.label_feature = "defaulted";

  lymph::models::Model model = lymph::models::logistic_regression();
  model.learn_(frame, batch);
  std::cout << "trained = " << std::boolalpha << model.trained() << "\n\n";

  // Inference takes the same frame and batch; only the feature columns are read.
  const DataFrame review({
      {"income", Slate(income)},
      {"balance", Slate(balance)},
      {"defaulted", Slate(defaulted)},
      {"predicted", Slate(model.infer(frame, batch))},
  });
  std::cout << review;

  return 0;
}
```

Output

```text
trained = true

============================================
     | income  balance  defaulted  predicted
--------------------------------------------
0    | 1       0.2      0          0        
1    | 1.3     0.4      0          0        
2    | 1.7     0.5      0          0        
3    | 2.1     0.9      0          0        
4    | 3.6     1.9      1          1        
5    | 4       2.2      1          1        
6    | 4.4     2.5      1          1        
7    | 4.9     2.8      1          1        
--------------------------------------------
[8 cs x 4 feat]
```

### Example 4

The `DataFrame x, Slate y` overload, plus `class_weight = Balanced` for an
imbalanced label (one default in eight). The balanced fit gives the lone default
a stronger pull: its score rises and the majority rows' scores relax toward the
boundary.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // Imbalanced: seven non-defaults, one default, and the classes overlap so
  // the minority row is genuinely hard to separate.
  const pluss::Series income = pluss::make_series({1.0, 1.4, 1.8, 2.2, 2.6, 3.0, 3.4, 2.8}, {8}, f64);
  const pluss::Series balance = pluss::make_series({0.3, 0.6, 0.9, 1.2, 1.5, 1.8, 2.1, 1.6}, {8}, f64);
  const pluss::Series defaulted = pluss::make_series({0, 0, 0, 0, 0, 0, 0, 1}, {8}, f64);

  // The label can be handed in as a Slate lifted straight from a frame
  // column -- the fourth accepted (features, label) input shape.
  const DataFrame frame({
      {"income", Slate(income)},
      {"balance", Slate(balance)},
      {"defaulted", Slate(defaulted)},
  });
  const DataFrame x({{"income", Slate(income)}, {"balance", Slate(balance)}});
  const Slate y = frame.features("defaulted");

  const lymph::models::Model blueprint = lymph::models::logistic_regression();

  // A plain fit is dominated by the seven negative rows.
  lymph::models::Model plain = blueprint.learn(x, y);

  // class_weight = Balanced rescales each class inversely to its frequency,
  // so the lone default row pulls on the boundary as hard as all seven
  // non-defaults together.
  lymph::options::Train balanced{};
  balanced.class_weight = lymph::ClassWeightMode::Balanced;
  lymph::models::Model rebalanced = blueprint.learn(x, y, balanced);

  // infer_scores is the raw decision score (log-odds); infer is its sign as
  // a 0/1 label.
  const DataFrame compare({
      {"actual", Slate(defaulted)},
      {"score_plain", Slate(plain.infer_scores(x))},
      {"score_balanced", Slate(rebalanced.infer_scores(x))},
      {"label_plain", Slate(plain.infer(x))},
      {"label_balanced", Slate(rebalanced.infer(x))},
  });
  std::cout << compare;

  return 0;
}
```

Output

```text
========================================================================
     | actual  score_plain   score_balanced  label_plain  label_balanced
------------------------------------------------------------------------
0    | 0       -6.693870592  -6.054801423    0            0             
1    | 0       -6.284159332  -5.642714073    0            0             
2    | 0       -5.874448073  -5.230626724    0            0             
3    | 0       -5.464736814  -4.818539375    0            0             
4    | 0       -5.055025554  -4.406452026    0            0             
5    | 0       -4.645314295  -3.994364676    0            0             
6    | 0       -4.235603035  -3.582277327    0            0             
7    | 1       3.171985153   4.50666551      1            1             
------------------------------------------------------------------------
[8 cs x 5 feat]
```

### Example 5

The failure mode and `warm_start`. `candidate.learn(x, y)` returns a fitted copy;
`candidate` itself stays a template, and asking it to infer raises. Then two
`warm_start` bursts of 50 iterations reach the same separation as one 100-iter
run, because the second burst continued from where the first stopped.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series income = pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64);
  const pluss::Series balance = pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  const DataFrame x({{"income", Slate(income)}, {"balance", Slate(balance)}});

  // The common mistake: assuming `learn` mutates the caller.
  lymph::models::Model candidate = lymph::models::logistic_regression();
  const lymph::models::Model fitted = candidate.learn(x, y);
  std::cout << "candidate.trained() = " << std::boolalpha << candidate.trained()
            << "   (still a template -- learn returned a copy)\n";
  std::cout << "fitted.trained()    = " << fitted.trained()
            << "   (the value you must keep)\n\n";

  // Asking an untrained model to infer is a hard error, not a silent guess.
  try {
    const pluss::Series ignored = candidate.infer(x);
    (void)ignored;
  } catch (const std::exception& err) {
    std::cout << "candidate.infer(x) threw: " << err.what() << "\n\n";
  }

  // warm_start controls whether a second learn_ continues from current
  // parameters or restarts. Two short bursts with warm_start should land in
  // the same place as one longer run.
  lymph::options::Train burst{};
  burst.max_iterations = 50;
  burst.warm_start = true;

  lymph::models::Model incremental = lymph::models::logistic_regression();
  incremental.learn_(x, y, burst);
  incremental.learn_(x, y, burst);

  lymph::models::Model one_shot = lymph::models::logistic_regression();
  lymph::options::Train single{};
  single.max_iterations = 100;
  one_shot.learn_(x, y, single);

  const DataFrame compare({
      {"income", Slate(income)},
      {"balance", Slate(balance)},
      {"two_warm_bursts", Slate(incremental.infer(x))},
      {"one_100_iter_run", Slate(one_shot.infer(x))},
  });
  std::cout << compare;

  return 0;
}
```

Output

```text
candidate.trained() = false   (still a template -- learn returned a copy)
fitted.trained()    = true   (the value you must keep)

candidate.infer(x) threw: infer: Flow must be learned before infer().
Triggered at: /path/to/pluss/ai/lymph/common.h:122 (T pluss::ai::lymph::resolve(StatusOr<T>, std::string_view) [T = pluss::codem::Series])

=========================================================
     | income  balance  two_warm_bursts  one_100_iter_run
---------------------------------------------------------
0    | 1       0.2      0                0               
1    | 1.3     0.4      0                0               
2    | 1.7     0.5      0                0               
3    | 2.1     0.9      0                0               
4    | 3.6     1.9      1                1               
5    | 4       2.2      1                1               
6    | 4.4     2.5      1                1               
7    | 4.9     2.8      1                1               
---------------------------------------------------------
[8 cs x 4 feat]
```

## See Also

- [prepare](prepare.md) and [append](append.md) — describe the preparation that
  `learn` fits before the estimator.
- [infer](infer.md) and [assess](assess.md) — consume the fitted state; both
  replay the same fitted preparation.
- [save](save.md) and [load](load.md) — persist a trained model across a process
  boundary.
- [Introduction](Introduction.md) — model lifecycle, value semantics, and cross-cutting notes.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="infer.html">
    <span class="label">Previous</span>
    <span class="title">infer</span>
  </a>
  <a class="next-link" href="load.html">
    <span class="label">Next</span>
    <span class="title">load</span>
  </a>
</div>
<!-- apiref:nav:end -->

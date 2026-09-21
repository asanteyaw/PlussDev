# `pluss::ai::classic::online::OnlineLinearClassifier`

A binary `Logistic` or `Hinge` linear classifier, fit by gradient descent
one step at a time rather than by a closed-form or batch-iterative solve.
`learn` establishes the fit (internally taking many such steps — see
[`Introduction`](Introduction.md#learn-is-not-one-step)); `advance` takes
exactly one more step against a fresh batch, without discarding anything
`learn` (or a previous `advance`) already learned — the operation the rest
of this reference has no equivalent for.

## Signature

```cpp
explicit classic::online::OnlineLinearClassifier(
    const classic::online::OnlineLinearOptions& options = {});

bool classic::online::OnlineLinearClassifier::trained() const noexcept;
std::int64_t classic::online::OnlineLinearClassifier::seen_samples() const noexcept;

classic::Status classic::online::OnlineLinearClassifier::learn(
    const classic::LabeledBatch& batch,
    const classic::TrainOptions& options = {});
classic::Status classic::online::OnlineLinearClassifier::advance(
    const classic::LabeledBatch& batch,
    const classic::TrainOptions& options = {});

classic::StatusOr<pluss::Series> classic::online::OnlineLinearClassifier::infer_scores(
    const classic::FeatureBatch& batch,
    const classic::InferOptions& options = {}) const;
classic::StatusOr<pluss::Series> classic::online::OnlineLinearClassifier::infer(
    const classic::FeatureBatch& batch,
    const classic::InferOptions& options = {}) const;
classic::StatusOr<pluss::Series> classic::online::OnlineLinearClassifier::assess(
    const classic::LabeledBatch& batch,
    const classic::AssessOptions& options = {}) const;
```

## Construction

The constructor itself never validates `options` — it just stores it;
an `OnlineLinearClassifier` built directly with an invalid
[`OnlineLinearOptions`](OnlineLinearOptions.md) (a bad `objective`, an
unsupported `regularization`, a non-positive `learning_rate`) constructs
successfully and only fails once `learn` is called. Reach for
[`make_online_linear_classifier`](make_online_linear_classifier.md)
instead to catch the same problem immediately, before any data is
involved — see [Example 2](#example-2).

## State

Everything fitted (the weight vector, which two raw label values map to
"negative"/"positive", the label's original dtype) is private — read back
only through `infer`/`infer_scores`/`assess`, never directly. The two
observable state accessors:

`trained()` — `bool`
: `true` once `learn` has succeeded at least once. Every other method
  except the constructor requires this — see `## Errors`.

`seen_samples()` — `std::int64_t`
: The cumulative row count across every `advance` step this estimator has
  taken, **including** the internal steps `learn` itself runs — reset to
  `0` at the start of every `learn` call (a fresh fit discards the old
  count entirely), then incremented by that step's batch size once per
  `advance` call afterward.

## Methods

`learn(batch, options = {})`
: Resets all fitted state, then internally calls `advance`
  `max(1, options.max_iterations)` times against the same (possibly
  rebalanced — see below) `batch` — see
  [`Introduction`](Introduction.md#learn-is-not-one-step) for why this
  means `learn` is not a single gradient step.
  [`TrainOptions::balance`](../core/TrainOptions.md#balance): when set to
  anything other than its default (`kind != Keep` or `clean != None`),
  `batch` is rebalanced via the same mechanism
  [`balance`](../balance.md) exposes, **once**, before any internal
  `advance` step runs — the internal bootstrap steps never re-balance a
  second time. Discovers exactly two distinct classes from `batch.y`
  (fails otherwise — see `## Errors`) and maps the smaller sorted value to
  "negative", the larger to "positive"; these need not be `0`/`1` — see
  [Example 4](#example-4). Every other field of `options` besides
  `balance`/`max_iterations` is accepted but ignored (no `missing`
  handling, no `sample_weights`, nothing beyond what `advance` itself
  reads, which is nothing — see `advance` below).

`advance(batch, options = {})`
: Exactly one gradient step against `batch`, added to whatever `learn` (or
  a previous `advance`) already fit. Requires `trained()` — call `learn`
  first. `batch.y` must contain **only** the two label values `learn`
  established (`Status::invalid_argument` otherwise — a genuinely new
  class can only be introduced by calling `learn` again, which resets
  everything). **`options` is accepted but entirely ignored** — no
  balancing, no `max_iterations`, nothing; it exists only to match
  `learn`'s call shape. See [Example 5](#example-5).

`infer_scores(batch, options = {})`
: The raw linear score (`design · weights`, no sigmoid, no clamping) —
  unbounded, not a probability, and **not comparable to
  `InferOptions::positive_threshold`** the way a batch classifier's
  scores are (see [`InferOptions`](../core/InferOptions.md#positive_threshold)).
  `options` is accepted but ignored entirely — this call reads nothing
  from it.

`infer(batch, options = {})`
: Calls `infer_scores`, then thresholds at exactly `score >= 0.0` —
  **fixed**, not configurable via `options.positive_threshold` the way
  `EstimatorBase::infer` is, since `infer` never reads `options` either.
  Returns the original "negative"/"positive" label values `learn`
  discovered, in `batch.y`'s original dtype from that `learn` call — not
  always `0.0`/`1.0`. See [Example 4](#example-4)/[Example 7](#example-7).

`assess(batch, options = {})`
: Computes `infer`/`infer_scores` internally, then scores against
  `batch.y` (which must again be one of the two known classes).
  `options.metrics` empty (the default) requests `{Accuracy}`; otherwise
  every requested `MetricKind` is computed via the same formulas
  [`metrics`](../metrics.md) uses (`balance_quality` fixed at
  `alpha = 0.1`, squared — not configurable the way
  [`ClassReportOptions`](../metrics/ClassReportOptions.md) allows).
  Supports `Accuracy`, `BalancedAccuracy`, `Precision`, `Recall`/
  `Sensitivity`, `F1`, `Specificity`, `GeometricMean`, `BalanceQuality`,
  `LogLoss`, `Brier`, `Mcc`, `CohenKappa` — every other `MetricKind`,
  including `RocAuc`/`Gini`/`AveragePrecision` (which would need a full
  score sweep this incremental estimator never keeps) and every
  regression metric, is rejected. See [Example 9](#example-9).

## Errors

- `learn`: `batch` fails [`validate_labeled`](../data/LabeledBatch.md) —
  that `Status`.
- `learn` / the factory: `options.family.objective` is neither `Logistic`
  nor `Hinge`, `options.family.regularization` is neither `None` nor
  `L2`, `options.learning_rate` is not finite or `<= 0`, or
  `options.family.regularization_strength` is not finite or `< 0` —
  `Status::invalid_argument`, naming the specific field.
- `learn`: `batch.features.x` is not rank-2, or contains `NaN` —
  `Status::shape_mismatch` / `Status::invalid_argument`.
- `learn`: `batch.y` does not resolve to **exactly two** distinct finite
  values, or contains `NaN` — `Status::invalid_argument`.
- `advance` / `infer_scores` / `infer` / `assess`: called before a
  successful `learn` — `Status::precondition`.
- `advance` / `infer_scores` / `infer`: the batch's feature count does
  not match what `learn` established — `Status::shape_mismatch`.
- `advance` / `assess`: `batch.y` contains a value that is not one of the
  two classes `learn` established — `Status::invalid_argument`.
- `advance` / `infer_scores` / `infer`: the resulting gradient, weights,
  or scores are non-finite (a diverging fit, typically from too large a
  `learning_rate`) — `Status::numerical_failure`.
- `assess`: a requested `MetricKind` this class does not support —
  `Status::invalid_argument`, naming why (see `assess` above).

## Practical Examples

The full `learn` → `advance` → `infer` → `assess` lifecycle, the
constructor-vs-factory validation difference, `max_iterations` visibly
changing fit quality, a non-`0`/`1` label space preserved end to end,
`advance`'s ignored `options` and its own two error modes, raw
`infer_scores` versus thresholded `infer` (and `InferOptions` proven
inert for both), the two learning-rate schedules compared, L2
regularization shrinking weights, `assess`'s metric list and its
rejection of a score-sweep metric, and `TrainOptions::balance` improving
minority-class recall during `learn`.

### Example 1

`learn`, three `advance` calls, `infer`, and `assess` — the ordinary
lifecycle, with `trained()`/`seen_samples()` tracked throughout.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::Logistic;
  cls::online::OnlineLinearClassifier model(options);
  std::cout << "trained() before learn = " << std::boolalpha << model.trained() << "\n";

  const cls::Series x = pluss::make_series(
      {-1.0, -0.8, -0.6, -0.4, 0.4, 0.6, 0.8, 1.0}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  const cls::Status learned = model.learn(batch);
  std::cout << "learn ok = " << learned.ok() << "\n";
  std::cout << "trained() after learn = " << model.trained() << "\n";
  std::cout << "seen_samples() after learn = " << model.seen_samples() << "\n";

  for (int i = 0; i < 3; ++i) model.advance(batch);
  std::cout << "seen_samples() after 3 advance() = " << model.seen_samples() << "\n";

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({-0.5, 0.5}, {2}, f64).reshape({2, 1});
  std::cout << "infer([-0.5, 0.5]) = " << model.infer(probe).value() << "\n";

  const cls::StatusOr<pluss::Series> scored = model.assess(batch);
  std::cout << "assess (default Accuracy) = " << scored.value() << "\n";

  return 0;
}
```

Output

```text
trained() before learn = false
learn ok = true
trained() after learn = true
seen_samples() after learn = 8000
seen_samples() after 3 advance() = 8024
infer([-0.5, 0.5]) = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 1]
assess (default Accuracy) = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
```

### Example 2

The constructor accepts an invalid `objective` (`LeastSquares`, a
regression-only value) without complaint — `learn` is where it actually
fails.
[`make_online_linear_classifier`](make_online_linear_classifier.md)
catches the identical problem immediately instead.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::online::OnlineLinearOptions bad_options{};
  bad_options.family.objective = cls::LinearObjective::LeastSquares;

  cls::online::OnlineLinearClassifier direct(bad_options);
  std::cout << "direct construction always succeeds (no ok() to check)\n";

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({2, 1});
  batch.y = pluss::make_series({0.0, 1.0}, {2}, f64);
  const cls::Status learned = direct.learn(batch);
  std::cout << "direct.learn() ok = " << std::boolalpha << learned.ok() << "\n";
  std::cout << "message = \"" << learned.message << "\"\n";

  const cls::StatusOr<cls::online::OnlineLinearClassifier> made =
      cls::online::make_online_linear_classifier(bad_options);
  std::cout << "make_online_linear_classifier ok = " << made.ok() << "\n";

  return 0;
}
```

Output

```text
direct construction always succeeds (no ok() to check)
direct.learn() ok = false
message = "OnlineLinearClassifier requires LinearObjective::Logistic or LinearObjective::Hinge."
make_online_linear_classifier ok = false
```

### Example 3

`TrainOptions::max_iterations` controls how many internal `advance` steps
`learn` takes — this data is cleanly separable enough that even `1` step
already reaches perfect `Accuracy`, so `LogLoss` (continuous, and still
improving long after hard accuracy has saturated) is the metric that
actually shows the difference: `1` step barely moves the weights off
zero (`LogLoss` near `ln(2) ≈ 0.693`, the score every row gets from an
all-zero model); the default `1000` reaches a confident, low-loss fit.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {-1.0, -0.8, -0.6, -0.4, 0.4, 0.6, 0.8, 1.0}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::Logistic;
  cls::AssessOptions logloss_only{};
  logloss_only.metrics = {cls::MetricKind::LogLoss};

  cls::online::OnlineLinearClassifier one_step(options);
  cls::TrainOptions one{};
  one.max_iterations = 1;
  one_step.learn(batch, one);
  std::cout << "LogLoss after max_iterations=1    = " << one_step.assess(batch, logloss_only).value() << "\n";

  cls::online::OnlineLinearClassifier default_fit(options);
  default_fit.learn(batch);  // max_iterations defaults to 1000
  std::cout << "LogLoss after max_iterations=1000 = " << default_fit.assess(batch, logloss_only).value() << "\n";

  return 0;
}
```

Output

```text
LogLoss after max_iterations=1    = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.68704285]
LogLoss after max_iterations=1000 = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.43414179]
```

### Example 4

Labels that are not `0`/`1` (here, `10.0`/`20.0`) round-trip through
`infer` unchanged — the smaller sorted value becomes "negative", the
larger "positive", and predictions come back in that same space.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::Logistic;
  cls::online::OnlineLinearClassifier model(options);

  const cls::Series x = pluss::make_series(
      {-1.0, -0.8, -0.6, -0.4, 0.4, 0.6, 0.8, 1.0}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series(
      {10.0, 10.0, 10.0, 10.0, 20.0, 20.0, 20.0, 20.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  model.learn(batch);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({-0.5, 0.5}, {2}, f64).reshape({2, 1});
  std::cout << "infer([-0.5, 0.5]) = " << model.infer(probe).value() << "\n";

  return 0;
}
```

Output

```text
infer([-0.5, 0.5]) = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[10, 20]
```

### Example 5

`advance`'s own `TrainOptions` is entirely ignored — a call with
`balance` configured behaves identically to one with default options —
while `seen_samples()` still accumulates across calls.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::Logistic;

  const cls::Series x = pluss::make_series(
      {-1.0, -0.8, -0.6, -0.4, 0.4, 0.6, 0.8, 1.0}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::online::OnlineLinearClassifier plain(options);
  plain.learn(batch);
  plain.advance(batch);
  std::cout << "seen_samples() after learn + 1 advance = " << plain.seen_samples() << "\n";

  cls::online::OnlineLinearClassifier with_balance_opt(options);
  with_balance_opt.learn(batch);
  cls::TrainOptions balance_opts{};
  balance_opts.balance.kind = cls::BalanceTargetKind::Equalize;
  with_balance_opt.advance(batch, balance_opts);  // balance is ignored by advance()

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({0.7}, {1}, f64).reshape({1, 1});
  const double score_plain = plain.infer_scores(probe).value().item<double>();
  const double score_with_opts = with_balance_opt.infer_scores(probe).value().item<double>();
  std::cout << "max abs diff (options ignored) = " << std::abs(score_plain - score_with_opts) << "\n";

  return 0;
}
```

Output

```text
seen_samples() after learn + 1 advance = 8008
max abs diff (options ignored) = 0
```

### Example 6

`advance`'s two own error modes: called before `learn`, and called with a
label value outside the two classes `learn` established.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::Logistic;
  cls::online::OnlineLinearClassifier model(options);

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({2, 1});
  batch.y = pluss::make_series({0.0, 1.0}, {2}, f64);

  const cls::Status before_learn = model.advance(batch);
  std::cout << "advance() before learn() ok = " << std::boolalpha << before_learn.ok() << "\n";
  std::cout << "message = \"" << before_learn.message << "\"\n";

  model.learn(batch);
  cls::LabeledBatch unseen_class{};
  unseen_class.features.x = pluss::make_series({3.0}, {1}, f64).reshape({1, 1});
  unseen_class.y = pluss::make_series({2.0}, {1}, f64);  // never seen at learn() time
  const cls::Status bad_label = model.advance(unseen_class);
  std::cout << "advance() with unseen label ok = " << bad_label.ok() << "\n";
  std::cout << "message = \"" << bad_label.message << "\"\n";

  return 0;
}
```

Output

```text
advance() before learn() ok = false
message = "OnlineLinearClassifier::advance requires learn() first."
advance() with unseen label ok = false
message = "OnlineLinearClassifier::advance labels must match classes seen during learn()."
```

### Example 7

`infer_scores` (raw, unbounded) versus `infer` (thresholded at exactly
`0.0`) on the same probe — and `InferOptions` proven to have zero effect
on either.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::Logistic;
  cls::online::OnlineLinearClassifier model(options);

  const cls::Series x = pluss::make_series(
      {-1.0, -0.8, -0.6, -0.4, 0.4, 0.6, 0.8, 1.0}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  model.learn(batch);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({0.7}, {1}, f64).reshape({1, 1});
  std::cout << "infer_scores([0.7]) = " << model.infer_scores(probe).value() << "\n";
  std::cout << "infer([0.7])        = " << model.infer(probe).value() << "\n";

  cls::InferOptions custom_opts{};
  custom_opts.positive_threshold = 0.99;  // would matter for a batch classifier -- not here
  std::cout << "infer_scores with custom InferOptions == default = "
            << std::boolalpha
            << (model.infer_scores(probe).value().item<double>()
                == model.infer_scores(probe, custom_opts).value().item<double>())
            << "\n";

  return 0;
}
```

Output

```text
infer_scores([0.7]) = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.62224885]
infer([0.7])        = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
infer_scores with custom InferOptions == default = true
```

### Example 8

`decaying_rate = true` (the default) versus `false`: ten identical
`advance` calls move the weights much further under a constant rate than
under a schedule that keeps shrinking.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {-1.0, -0.8, -0.6, -0.4, 0.4, 0.6, 0.8, 1.0}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::online::OnlineLinearOptions decaying{};
  decaying.family.objective = cls::LinearObjective::Logistic;
  decaying.decaying_rate = true;
  cls::online::OnlineLinearClassifier decaying_model(decaying);
  cls::TrainOptions one{};
  one.max_iterations = 1;
  decaying_model.learn(batch, one);
  for (int i = 0; i < 10; ++i) decaying_model.advance(batch);

  cls::online::OnlineLinearOptions constant = decaying;
  constant.decaying_rate = false;
  cls::online::OnlineLinearClassifier constant_model(constant);
  constant_model.learn(batch, one);
  for (int i = 0; i < 10; ++i) constant_model.advance(batch);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({0.7}, {1}, f64).reshape({1, 1});
  std::cout << "decaying_rate=true  score(0.7) = " << decaying_model.infer_scores(probe).value() << "\n";
  std::cout << "decaying_rate=false score(0.7) = " << constant_model.infer_scores(probe).value() << "\n";

  return 0;
}
```

Output

```text
decaying_rate=true  score(0.7) = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.064164462]
decaying_rate=false score(0.7) = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.13029725]
```

### Example 9

`assess`'s default `{Accuracy}`, an explicit multi-metric list, and its
rejection of `RocAuc` — a score-sweep metric this incremental estimator
cannot compute.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::Logistic;
  cls::online::OnlineLinearClassifier model(options);

  const cls::Series x = pluss::make_series(
      {-1.0, -0.8, -0.6, -0.4, 0.4, 0.6, 0.8, 1.0}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  model.learn(batch);

  std::cout << "default {Accuracy}       = " << model.assess(batch).value() << "\n";

  cls::AssessOptions multi{};
  multi.metrics = {cls::MetricKind::F1, cls::MetricKind::Mcc};
  std::cout << "[F1, Mcc]                = " << model.assess(batch, multi).value() << "\n";

  cls::AssessOptions unsupported{};
  unsupported.metrics = {cls::MetricKind::RocAuc};
  const cls::StatusOr<pluss::Series> rejected = model.assess(batch, unsupported);
  std::cout << "RocAuc ok = " << std::boolalpha << rejected.ok() << "\n";
  std::cout << "message = \"" << rejected.status().message << "\"\n";

  return 0;
}
```

Output

```text
default {Accuracy}       = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
[F1, Mcc]                = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
RocAuc ok = false
message = "Unsupported classification metric for online linear classifier (Gini / AveragePrecision need the batch classifier's score sweep)."
```

### Example 10

`TrainOptions::balance` set to equalize the class counts during `learn` —
minority-class recall goes from `0` to `1` once the fit no longer lets the
majority class dominate. The two classes here overlap (no perfect
separator exists), which is what makes an unweighted fit willing to
sacrifice the minority class entirely — on cleanly separable data, an
unweighted fit finds the same separator either way.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // 9 majority rows (class 0) spread across [-1, 0.6]; 3 minority rows (class 1)
  // overlapping the majority's upper tail -- no perfect linear separator exists.
  const cls::Series x = pluss::make_series(
      {-1.0, -0.8, -0.6, -0.4, -0.2, 0.0, 0.2, 0.4, 0.6, 0.5, 0.55, 0.6}, {12}, f64).reshape({12, 1});
  const cls::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {12}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::Logistic;

  cls::online::OnlineLinearClassifier unbalanced(options);
  unbalanced.learn(batch);
  cls::AssessOptions recall_only{};
  recall_only.metrics = {cls::MetricKind::Recall};
  std::cout << "recall, no balance = " << unbalanced.assess(batch, recall_only).value() << "\n";

  cls::online::OnlineLinearClassifier balanced_model(options);
  cls::TrainOptions balance_opts{};
  balance_opts.balance.kind = cls::BalanceTargetKind::Equalize;
  balanced_model.learn(batch, balance_opts);
  std::cout << "recall, equalized   = " << balanced_model.assess(batch, recall_only).value() << "\n";

  return 0;
}
```

Output

```text
recall, no balance = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0]
recall, equalized   = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
```

## Changelog

- **0.1** — constructor, `trained`/`seen_samples`, `learn`/`advance`,
  `infer_scores`/`infer`, `assess`.

## See Also

- [`OnlineLinearOptions`](OnlineLinearOptions.md) — the options struct.
- [`make_online_linear_classifier`](make_online_linear_classifier.md) —
  the eager-validation entry point.
- [`OnlineLinearRegressor`](OnlineLinearRegressor.md) — the regression
  counterpart.
- [`metrics`](../metrics.md) — the formulas `assess` computes.
- [`balance`](../balance.md) — what `TrainOptions::balance` invokes
  inside `learn`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

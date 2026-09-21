# `pluss::ai::classic::online::OnlineLinearRegressor`

A `LeastSquares` or `RobustLeast` linear regressor, fit by gradient
descent one step at a time — the regression counterpart to
[`OnlineLinearClassifier`](OnlineLinearClassifier.md), sharing the same
`learn`/`advance`/`infer`/`assess` shape and the same
[`OnlineLinearOptions`](OnlineLinearOptions.md), without the binary-label
bookkeeping a classifier needs.

## Signature

```cpp
explicit classic::online::OnlineLinearRegressor(
    const classic::online::OnlineLinearOptions& options = {});

bool classic::online::OnlineLinearRegressor::trained() const noexcept;
std::int64_t classic::online::OnlineLinearRegressor::seen_samples() const noexcept;

classic::Status classic::online::OnlineLinearRegressor::learn(
    const classic::LabeledBatch& batch,
    const classic::TrainOptions& options = {});
classic::Status classic::online::OnlineLinearRegressor::advance(
    const classic::LabeledBatch& batch,
    const classic::TrainOptions& options = {});

classic::StatusOr<pluss::Series> classic::online::OnlineLinearRegressor::infer(
    const classic::FeatureBatch& batch,
    const classic::InferOptions& options = {}) const;
classic::StatusOr<pluss::Series> classic::online::OnlineLinearRegressor::assess(
    const classic::LabeledBatch& batch,
    const classic::AssessOptions& options = {}) const;
```

## Construction

The constructor never validates `options` — an `OnlineLinearRegressor`
built directly with an invalid
[`OnlineLinearOptions`](OnlineLinearOptions.md) constructs successfully
and only fails once `learn` is called. Reach for
[`make_online_linear_regressor`](make_online_linear_regressor.md) instead
to catch the same problem immediately — see [Example 2](#example-2).

## State

The fitted weight vector is private, read back only through `infer` /
`assess`. The two observable accessors:

`trained()` — `bool`
: `true` once `learn` has succeeded at least once.

`seen_samples()` — `std::int64_t`
: The cumulative row count across every `advance` step, **including**
  `learn`'s own internal steps — reset to `0` at the start of every
  `learn` call, then incremented by each step's batch size.

## Methods

`learn(batch, options = {})`
: Resets all fitted state, then internally calls `advance`
  `max(1, options.max_iterations)` times against `batch` unchanged —
  see [`Introduction`](Introduction.md#learn-is-not-one-step). Unlike
  [`OnlineLinearClassifier::learn`](OnlineLinearClassifier.md#methods),
  **`options.balance` is never consulted** — row balancing is a
  classification-imbalance concept with no regression analogue here, and
  this `learn` never rebalances `batch` regardless of what `balance` is
  set to. See [Example 11](#example-11). Every field of `options` besides
  `max_iterations` is accepted but otherwise unused.

`advance(batch, options = {})`
: Exactly one gradient step against `batch`. Requires `trained()`.
  **`options` is accepted but entirely ignored** — the same as
  `OnlineLinearClassifier::advance`. See [Example 4](#example-4).

`infer(batch, options = {})`
: The raw linear prediction (`design · weights`) — no threshold, no
  transform; a regressor's prediction needs none. `options` is accepted
  but ignored entirely.

`assess(batch, options = {})`
: Computes `infer` internally, then scores against `batch.y`.
  `options.metrics` empty (the default) requests `{Mse, Rmse, R2}` — the
  same default every regression `assess` in this reference uses. Supports
  `Mse`, `Rmse`, `Mae`, `R2` only — every other `MetricKind`, including
  `QuantileLoss`, is rejected. See [Example 10](#example-10).

## Errors

- `learn`: `batch` fails [`validate_labeled`](../data/LabeledBatch.md) —
  that `Status`.
- `learn` / the factory: `options.family.objective` is neither
  `LeastSquares` nor `RobustLeast`, `options.family.regularization` is
  neither `None` nor `L2`, `options.learning_rate` is not finite or
  `<= 0`, `options.family.regularization_strength` is not finite or
  `< 0`, or `options.family.robust_transition` is not finite or `<= 0`
  (checked **unconditionally**, even when `objective = LeastSquares`
  never reads it — see [Example 7](#example-7)) —
  `Status::invalid_argument`, naming the specific field.
- `learn`: `batch.features.x` is not rank-2, or contains `NaN`, or
  `batch.y`'s length does not match the sample count, or `batch.y`
  contains `NaN` — `Status::shape_mismatch` / `Status::invalid_argument`.
- `advance` / `infer` / `assess`: called before a successful `learn` —
  `Status::precondition`.
- `advance` / `infer`: the batch's feature count does not match what
  `learn` established — `Status::shape_mismatch`.
- `advance`: `batch.y`'s length does not match the sample count, or
  contains `NaN` — `Status::shape_mismatch` / `Status::invalid_argument`.
- `advance` / `infer`: the resulting gradient, weights, or predictions
  are non-finite — `Status::numerical_failure`.
- `assess`: a requested `MetricKind` this class does not support —
  `Status::invalid_argument`.

## Practical Examples

The full lifecycle, the constructor-vs-factory validation difference,
`max_iterations` visibly changing fit quality, incremental `advance` with
its ignored `options`, `advance` called before `learn`, `RobustLeast`
resisting an outlier `LeastSquares` does not, `robust_transition`
breaking a plain `LeastSquares` fit when invalid, the two learning-rate
schedules compared, L2 regularization shrinking weights, `assess`'s
metric list and its rejection of `QuantileLoss`, and confirmation that
`TrainOptions::balance` has no effect here at all.

### Example 1

`learn`, `advance`, `infer`, `assess` — the ordinary lifecycle.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::LeastSquares;
  cls::online::OnlineLinearRegressor model(options);
  std::cout << "trained() before learn = " << std::boolalpha << model.trained() << "\n";

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64).reshape({5, 1});
  const cls::Series y = pluss::make_series({2.0, 4.0, 6.0, 8.0, 10.0}, {5}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  const cls::Status learned = model.learn(batch);
  std::cout << "learn ok = " << learned.ok() << "\n";
  std::cout << "trained() after learn = " << model.trained() << "\n";

  model.advance(batch);
  std::cout << "seen_samples() after learn + 1 advance = " << model.seen_samples() << "\n";

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({6.0}, {1}, f64).reshape({1, 1});
  std::cout << "infer([6]) = " << model.infer(probe).value() << "\n";
  std::cout << "assess (default [Mse, Rmse, R2]) = " << model.assess(batch).value() << "\n";

  return 0;
}
```

Output

```text
trained() before learn = false
learn ok = true
trained() after learn = true
seen_samples() after learn + 1 advance = 5005
infer([6]) = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[11.880392]
assess (default [Mse, Rmse, R2]) = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0059434648, 0.07709387, 0.99925707]
```

### Example 2

The constructor accepts an invalid `objective` (`Logistic`, a
classification-only value) without complaint; `learn` fails.
[`make_online_linear_regressor`](make_online_linear_regressor.md) catches
it immediately instead.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::online::OnlineLinearOptions bad_options{};
  bad_options.family.objective = cls::LinearObjective::Logistic;

  cls::online::OnlineLinearRegressor direct(bad_options);
  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({2, 1});
  batch.y = pluss::make_series({1.0, 2.0}, {2}, f64);
  const cls::Status learned = direct.learn(batch);
  std::cout << "direct.learn() ok = " << std::boolalpha << learned.ok() << "\n";
  std::cout << "message = \"" << learned.message << "\"\n";

  const cls::StatusOr<cls::online::OnlineLinearRegressor> made =
      cls::online::make_online_linear_regressor(bad_options);
  std::cout << "make_online_linear_regressor ok = " << made.ok() << "\n";

  return 0;
}
```

Output

```text
direct.learn() ok = false
message = "OnlineLinearRegressor requires LinearObjective::LeastSquares or LinearObjective::RobustLeast."
make_online_linear_regressor ok = false
```

### Example 3

`TrainOptions::max_iterations`: `1` internal step barely moves off zero;
the default `1000` reaches a close fit.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64).reshape({5, 1});
  const cls::Series y = pluss::make_series({2.0, 4.0, 6.0, 8.0, 10.0}, {5}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::LeastSquares;

  cls::online::OnlineLinearRegressor one_step(options);
  cls::TrainOptions one{};
  one.max_iterations = 1;
  one_step.learn(batch, one);
  cls::AssessOptions r2_only{};
  r2_only.metrics = {cls::MetricKind::R2};
  std::cout << "R2 after max_iterations=1    = " << one_step.assess(batch, r2_only).value() << "\n";

  cls::online::OnlineLinearRegressor default_fit(options);
  default_fit.learn(batch);  // max_iterations defaults to 1000
  std::cout << "R2 after max_iterations=1000 = " << default_fit.assess(batch, r2_only).value() << "\n";

  return 0;
}
```

Output

```text
R2 after max_iterations=1    = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.81]
R2 after max_iterations=1000 = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.99925627]
```

### Example 4

`advance`'s own `TrainOptions` is ignored entirely, while `seen_samples()`
still accumulates.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64).reshape({5, 1});
  const cls::Series y = pluss::make_series({2.0, 4.0, 6.0, 8.0, 10.0}, {5}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::LeastSquares;

  cls::online::OnlineLinearRegressor plain(options);
  plain.learn(batch);
  plain.advance(batch);

  cls::online::OnlineLinearRegressor with_opts(options);
  with_opts.learn(batch);
  cls::TrainOptions custom{};
  custom.max_iterations = 500;  // has no effect on advance()
  with_opts.advance(batch, custom);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({6.0}, {1}, f64).reshape({1, 1});
  const double pred_plain = plain.infer(probe).value().item<double>();
  const double pred_with_opts = with_opts.infer(probe).value().item<double>();
  std::cout << "seen_samples() = " << plain.seen_samples() << "\n";
  std::cout << "max abs diff (options ignored) = " << std::abs(pred_plain - pred_with_opts) << "\n";

  return 0;
}
```

Output

```text
seen_samples() = 5005
max abs diff (options ignored) = 0
```

### Example 5

`advance` called before `learn` — rejected.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::LeastSquares;
  cls::online::OnlineLinearRegressor model(options);

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({2, 1});
  batch.y = pluss::make_series({1.0, 2.0}, {2}, f64);

  const cls::Status before_learn = model.advance(batch);
  std::cout << "advance() before learn() ok = " << std::boolalpha << before_learn.ok() << "\n";
  std::cout << "message = \"" << before_learn.message << "\"\n";

  return 0;
}
```

Output

```text
advance() before learn() ok = false
message = "OnlineLinearRegressor::advance requires learn() first."
```

### Example 6

`RobustLeast` versus `LeastSquares` on data with one large outlier —
the robust objective's clipped gradient is far less pulled off the
underlying `y = 2x` trend.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // y = 2x for every row except the last, which is a wild outlier.
  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64).reshape({5, 1});
  const cls::Series y = pluss::make_series({2.0, 4.0, 6.0, 8.0, 100.0}, {5}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::online::OnlineLinearOptions least_squares{};
  least_squares.family.objective = cls::LinearObjective::LeastSquares;
  cls::online::OnlineLinearRegressor ls_model(least_squares);
  ls_model.learn(batch);

  cls::online::OnlineLinearOptions robust{};
  robust.family.objective = cls::LinearObjective::RobustLeast;
  robust.family.robust_transition = 1.0;
  cls::online::OnlineLinearRegressor robust_model(robust);
  robust_model.learn(batch);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({6.0}, {1}, f64).reshape({1, 1});
  std::cout << "LeastSquares infer([6]) (pulled toward the outlier) = " << ls_model.infer(probe).value() << "\n";
  std::cout << "RobustLeast  infer([6]) (closer to the true trend)  = " << robust_model.infer(probe).value() << "\n";

  return 0;
}
```

Output

```text
LeastSquares infer([6]) (pulled toward the outlier) = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[75.026395]
RobustLeast  infer([6]) (closer to the true trend)  = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[12.751242]
```

### Example 7

`robust_transition` is validated **unconditionally** — an invalid value
breaks a plain `LeastSquares` fit too, even though `LeastSquares` never
reads it.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::LeastSquares;  // never reads robust_transition
  options.family.robust_transition = -1.0;  // invalid regardless

  const cls::StatusOr<cls::online::OnlineLinearRegressor> made =
      cls::online::make_online_linear_regressor(options);
  std::cout << "ok = " << std::boolalpha << made.ok() << "\n";
  std::cout << "message = \"" << made.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "LinearFamilyOptions.robust_transition must be finite and > 0."
```

### Example 8

`decaying_rate = true` versus `false`: the two predictions differ, but
only slightly — unlike
[`OnlineLinearClassifier`'s own comparison](OnlineLinearClassifier.md#example-8),
the least-squares gradient never saturates the way a sigmoid does, so
both schedules land close to the same fit within the same ten steps; the
schedule matters far less here than it does for the classifier.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64).reshape({5, 1});
  const cls::Series y = pluss::make_series({2.0, 4.0, 6.0, 8.0, 10.0}, {5}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::online::OnlineLinearOptions decaying{};
  decaying.family.objective = cls::LinearObjective::LeastSquares;
  decaying.decaying_rate = true;
  cls::online::OnlineLinearRegressor decaying_model(decaying);
  cls::TrainOptions one{};
  one.max_iterations = 1;
  decaying_model.learn(batch, one);
  for (int i = 0; i < 10; ++i) decaying_model.advance(batch);

  cls::online::OnlineLinearOptions constant = decaying;
  constant.decaying_rate = false;
  cls::online::OnlineLinearRegressor constant_model(constant);
  constant_model.learn(batch, one);
  for (int i = 0; i < 10; ++i) constant_model.advance(batch);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({6.0}, {1}, f64).reshape({1, 1});
  std::cout << "decaying_rate=true  infer([6]) = " << decaying_model.infer(probe).value() << "\n";
  std::cout << "decaying_rate=false infer([6]) = " << constant_model.infer(probe).value() << "\n";

  return 0;
}
```

Output

```text
decaying_rate=true  infer([6]) = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[11.689753]
decaying_rate=false infer([6]) = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[11.717688]
```

### Example 9

`regularization = L2` shrinks the fitted weight toward zero, compared to
`None`, given the same data and update count.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64).reshape({5, 1});
  const cls::Series y = pluss::make_series({2.0, 4.0, 6.0, 8.0, 10.0}, {5}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::online::OnlineLinearOptions none{};
  none.family.objective = cls::LinearObjective::LeastSquares;
  none.family.regularization = cls::RegularizationKind::None;
  cls::online::OnlineLinearRegressor none_model(none);
  none_model.learn(batch);

  cls::online::OnlineLinearOptions l2 = none;
  l2.family.regularization = cls::RegularizationKind::L2;
  l2.family.regularization_strength = 5.0;
  cls::online::OnlineLinearRegressor l2_model(l2);
  l2_model.learn(batch);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({6.0}, {1}, f64).reshape({1, 1});
  std::cout << "None infer([6]) = " << none_model.infer(probe).value() << "\n";
  std::cout << "L2   infer([6]) = " << l2_model.infer(probe).value() << "\n";

  return 0;
}
```

Output

```text
None infer([6]) = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[11.880329]
L2   infer([6]) = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[8.8191828]
```

### Example 10

`assess`'s default `{Mse, Rmse, R2}`, an explicit subset, and its
rejection of `QuantileLoss`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::LeastSquares;
  cls::online::OnlineLinearRegressor model(options);

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64).reshape({5, 1});
  const cls::Series y = pluss::make_series({2.0, 4.0, 6.0, 8.0, 10.0}, {5}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  model.learn(batch);

  std::cout << "default [Mse, Rmse, R2] = " << model.assess(batch).value() << "\n";

  cls::AssessOptions mae_only{};
  mae_only.metrics = {cls::MetricKind::Mae};
  std::cout << "[Mae]                   = " << model.assess(batch, mae_only).value() << "\n";

  cls::AssessOptions unsupported{};
  unsupported.metrics = {cls::MetricKind::QuantileLoss};
  const cls::StatusOr<pluss::Series> rejected = model.assess(batch, unsupported);
  std::cout << "QuantileLoss ok = " << std::boolalpha << rejected.ok() << "\n";
  std::cout << "message = \"" << rejected.status().message << "\"\n";

  return 0;
}
```

Output

```text
default [Mse, Rmse, R2] = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0059498211, 0.077135084, 0.99925627]
[Mae]                   = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.066206799]
QuantileLoss ok = false
message = "Unsupported regression metric for online linear regressor."
```

### Example 11

`TrainOptions::balance` set exactly as in
[`OnlineLinearClassifier`'s balancing example](OnlineLinearClassifier.md#example-10)
has **no** effect on the regressor's fit — confirming the asymmetry
[`Introduction`](Introduction.md#two-classes-one-shape) describes.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64).reshape({5, 1});
  const cls::Series y = pluss::make_series({2.0, 4.0, 6.0, 8.0, 10.0}, {5}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::LeastSquares;

  cls::online::OnlineLinearRegressor plain(options);
  plain.learn(batch);

  cls::online::OnlineLinearRegressor with_balance(options);
  cls::TrainOptions balance_opts{};
  balance_opts.balance.kind = cls::BalanceTargetKind::Equalize;
  with_balance.learn(batch, balance_opts);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({6.0}, {1}, f64).reshape({1, 1});
  const double pred_plain = plain.infer(probe).value().item<double>();
  const double pred_balanced = with_balance.infer(probe).value().item<double>();
  std::cout << "max abs diff (balance ignored by the regressor) = "
            << std::abs(pred_plain - pred_balanced) << "\n";

  return 0;
}
```

Output

```text
max abs diff (balance ignored by the regressor) = 0
```

## Changelog

- **0.1** — constructor, `trained`/`seen_samples`, `learn`/`advance`,
  `infer`, `assess`.

## See Also

- [`OnlineLinearOptions`](OnlineLinearOptions.md) — the options struct.
- [`make_online_linear_regressor`](make_online_linear_regressor.md) — the
  eager-validation entry point.
- [`OnlineLinearClassifier`](OnlineLinearClassifier.md) — the
  classification counterpart.
- [`metrics`](../metrics.md) — the formulas `assess` computes.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

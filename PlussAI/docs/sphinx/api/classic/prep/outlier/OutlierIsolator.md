# `pluss::ai::classic::prep::OutlierIsolator`

An isolation forest: an ensemble of randomized binary-split trees, each
built to isolate every training row as quickly as possible. A row that
separates from the rest of the data in very few splits — reachable at a
shallow tree depth — is anomalous; a row that takes many splits to isolate
sits comfortably among its neighbors. `score_samples` reports that
per-row score directly (lower is more anomalous); `infer` thresholds it
into a `-1` (anomaly) / `1` (normal) label using a decision offset fitted
alongside the forest. **Not** a [`TransformerBase`](../../estimators/TransformerBase.md) —
see [`outlier/Introduction`](Introduction.md#why-not-transformerbase) for
why its shape is `learn`/`score_samples`/`infer`/`learn_infer` instead of
`learn`/`map`/`learn_map`.

## Signature

```cpp
explicit classic::prep::OutlierIsolator(classic::prep::OutlierIsolatorOptions options = {});
```

## Construction

A concrete value type, constructed directly with an optional
[`OutlierIsolatorOptions`](OutlierIsolatorOptions.md).

## State

`learned()` — `bool`
: `true` once `learn` has succeeded at least once.

`decision_offset()` — `double`
: The fitted threshold `infer` compares each row's score against. `-0.5`
  when `contamination_auto` was `true` at fit time (a fixed, data-independent
  convention); otherwise the `contamination`-quantile of the *training
  batch's own* scores — so with `contamination = 0.1`, roughly `10%` of the
  training rows themselves score below the fitted offset.

`max_samples_used()` / `max_features_used()` — `std::int64_t`
: The actual per-tree sample and feature subset sizes the forest was built
  with, after resolving `max_samples`/`max_samples_auto` and
  `max_features_ratio` against the training batch's actual shape.

## Methods

`learn(batch, options = {})`
: Validates `estimators`/`max_samples`/`max_features_ratio`/`contamination`
  first. Requires at least 2 samples. Builds `estimators` randomized trees,
  each over a subsample of rows (`max_samples_used()`, bootstrap-sampled if
  `bootstrap`, else without replacement) and a subset of features
  (`max_features_used()`), splitting on a randomly chosen feature and
  threshold at each node until a depth limit derived from
  `max_samples_used()` is reached. Then fits `decision_offset()` per
  `contamination`/`contamination_auto` (see `## State`) — when
  `contamination_auto` is `false`, this re-scores the training batch itself
  through the just-built forest.

`score_samples(batch, options = {})`
: Returns one `Float64` score per row: `-2^(-avg_path_length / c)`, where
  `avg_path_length` is the row's mean isolation depth across every tree and
  `c` is a normalizing constant derived from `max_samples_used()`. Scores
  are unbounded below in principle but concentrate near `-1` (very
  anomalous, isolated almost immediately) to near `0` (very normal, deep in
  the tree before isolation). `Status::precondition` if not yet learned.

`infer(batch, options = {})`
: `score_samples(batch) - decision_offset() < 0 ? -1 : 1`, as `Int64`. The
  conventional isolation-forest label: `-1` for an anomaly, `1` for normal.

`learn_infer(batch, learn_options = {}, infer_options = {})`
: `learn` then `infer` on the same data, in one call.

## Errors

- `estimators <= 0` — `Status::invalid_argument` from `learn`.
- `max_samples <= 0` (only checked when `max_samples_auto = false`) —
  `Status::invalid_argument` from `learn`.
- `max_features_ratio` outside `(0, 1]` — `Status::invalid_argument` from
  `learn`.
- `contamination` outside `(0, 0.5]` (only checked when
  `contamination_auto = false`) — `Status::invalid_argument` from `learn`.
- Fewer than 2 samples — `Status::invalid_argument` from `learn`.
- `score_samples`/`infer` before a successful `learn` —
  `Status::precondition`.

## Practical Examples

A clear outlier scored against a tight cluster, `infer`'s label output next
to the raw score, and `contamination` changing how many training-adjacent
points get flagged.

### Example 1

Nine tightly clustered points and one far outlier — the outlier's score is
visibly more negative (more anomalous).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series(
      {0.0, 0.1, -0.1, 0.05, -0.05, 0.2, -0.2, 0.0, 0.1, 100.0},
      {10}, f64).reshape({10, 1});

  cls::prep::OutlierIsolatorOptions options{};
  options.random = cls::RandomState{};  // seed = 0, deterministic -- the default
  cls::prep::OutlierIsolator isolator(options);
  isolator.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({0.0, 100.0}, {2}, f64).reshape({2, 1});
  std::cout << "scores (normal point, then the outlier) = " << isolator.score_samples(probe).value() << "\n";

  return 0;
}
```

Output

```text
scores (normal point, then the outlier) = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-0.36956562, -0.82965661]
```

### Example 2

`infer`'s `-1`/`1` labels for the same two points, alongside the raw scores
that produced them.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series(
      {0.0, 0.1, -0.1, 0.05, -0.05, 0.2, -0.2, 0.0, 0.1, 100.0},
      {10}, f64).reshape({10, 1});

  cls::prep::OutlierIsolator isolator;
  const pluss::Series labels = isolator.learn_infer(train).value();
  std::cout << "decision_offset() = " << isolator.decision_offset() << "\n";
  std::cout << "labels = " << labels << "\n";

  return 0;
}
```

Output

```text
decision_offset() = -0.5
labels = pluss::Series(shape=[10], strides=[1], dtype=int64, platform=cpu, numel=10, contiguous=true)
data=[1, 1, 1, 1, 1, -1, -1, 1, 1, -1]
```

### Example 3

`contamination_auto = false` with an explicit `contamination` sets
`decision_offset()` from the training scores' own quantile, rather than the
fixed `-0.5` default — visibly different fitted offsets for two different
`contamination` values on the same data.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series(
      {0.0, 0.1, -0.1, 0.05, -0.05, 0.2, -0.2, 0.0, 0.1, 100.0},
      {10}, f64).reshape({10, 1});

  auto fit_offset = [&](double contamination) {
    cls::prep::OutlierIsolatorOptions options{};
    options.contamination_auto = false;
    options.contamination = contamination;
    cls::prep::OutlierIsolator isolator(options);
    isolator.learn(train);
    return isolator.decision_offset();
  };

  std::cout << "contamination_auto=true (default): offset = -0.5\n";
  std::cout << "contamination=0.1: offset = " << fit_offset(0.1) << "\n";
  std::cout << "contamination=0.5: offset = " << fit_offset(0.5) << "\n";

  return 0;
}
```

Output

```text
contamination_auto=true (default): offset = -0.5
contamination=0.1: offset = -0.584537
contamination=0.5: offset = -0.399713
```

## Changelog

- **0.1** — `learn`/`score_samples`/`infer`/`learn_infer`,
  `estimators`/`max_samples`/`max_samples_auto`/`max_features_ratio`/
  `bootstrap`/`contamination`/`contamination_auto`/`random`,
  `decision_offset`/`max_samples_used`/`max_features_used`.

## See Also

- [`OutlierIsolatorOptions`](OutlierIsolatorOptions.md) — the options
  struct.
- [`RandomState`](../../core/RandomState.md) — the seed/determinism pair
  `random` takes.
- [`RobustScaler`](../scaling/RobustScaler.md) — de-emphasizing outliers in
  a fit, rather than flagging the rows.
- [`outlier/Introduction`](Introduction.md) — why this type is not a
  `TransformerBase`.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

OutlierIsolatorOptions
```
<!-- apiref:toctree:end -->

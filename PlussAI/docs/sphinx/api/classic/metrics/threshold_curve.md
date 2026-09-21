# `pluss::ai::classic::threshold_curve` / `best_threshold` / `average_precision`

Sweep every achievable binary decision threshold in one `O(n log n)` pass —
`threshold_curve` returns the full precision/recall/F1 curve;
`best_threshold` picks the single threshold that maximizes a chosen
objective from that same curve; `average_precision` integrates the
precision-recall curve into one number (the area under it, computed the
standard step-sum way — no interpolation between points). All three share
one internal sort-and-cumulative-sum computation, so calling `best_threshold`
or `average_precision` costs the same as computing the curve yourself.

## Signature

```cpp
classic::StatusOr<classic::ThresholdCurve> classic::threshold_curve(
    const pluss::Series& scores, const pluss::Series& labels);

classic::StatusOr<double> classic::best_threshold(
    const pluss::Series& scores,
    const pluss::Series& labels,
    classic::MetricKind objective = classic::MetricKind::F1);

classic::StatusOr<double> classic::average_precision(
    const pluss::Series& scores, const pluss::Series& labels);
```

## Parameters

`scores`
: Rank-any `Series` (reshaped to rank-1) of continuous decision
  values/probabilities — higher means more likely positive. Need not be in
  `[0, 1]`.

`labels`
: Rank-any `Series` (reshaped to rank-1) of binary `0`/`1` labels, the same
  length as `scores`. Every non-zero value is treated as `1` internally, but
  supply exactly `0.0`/`1.0` for a meaningful result.

`objective` (`best_threshold` only)
: Which curve to maximize over: `Precision`, `Recall`/`Sensitivity`, or
  anything else (including the default, `F1`) — every other `MetricKind`
  falls back to `F1` rather than being rejected.

## Returns

`threshold_curve` returns a [`ThresholdCurve`](ThresholdCurve.md) — see its
own page for every field.

`best_threshold` returns the `thresholds` entry at the position where
`objective`'s curve is highest (the first such position, if there is a tie).

`average_precision` returns the step-sum area under the precision-recall
curve, `sum((recall[i] - recall[i-1]) * precision[i])` over the sweep (with
`recall[-1]` taken as `0`) — `0.0` when `labels` contains no positives at
all (recall is then identically `0` everywhere, so every term of the sum is
`0`).

## Errors

- `scores`/`labels` length mismatch, or either empty — `Status::shape_mismatch`.
- `average_precision` / `best_threshold`: an empty resulting curve (only
  reachable via the same empty-input case above) —
  `Status::invalid_argument`.

## Practical Examples

The full curve on five scores with one tie, `best_threshold` under each of
the three distinct objective behaviors, and `average_precision` on a
perfect-ranking case versus a case with no positives at all.

### Example 1

Five scores, one tied pair — four distinct achievable thresholds.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series scores = pluss::make_series({0.9, 0.8, 0.8, 0.4, 0.1}, {5}, f64);
  const pluss::Series labels = pluss::make_series({1.0, 1.0, 0.0, 1.0, 0.0}, {5}, f64);

  const cls::ThresholdCurve curve = cls::threshold_curve(scores, labels).value();
  std::cout << "thresholds = " << curve.thresholds << "\n";
  std::cout << "f1         = " << curve.f1 << "\n";

  return 0;
}
```

Output

```text
thresholds = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.9, 0.8, 0.4, 0.1]
f1         = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.5, 0.66666667, 0.85714286, 0.75]
```

### Example 2

`best_threshold` under `F1` (the default) versus `Precision` — maximizing
precision alone picks the highest-scoring point, at the cost of recall.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series scores = pluss::make_series({0.9, 0.8, 0.8, 0.4, 0.1}, {5}, f64);
  const pluss::Series labels = pluss::make_series({1.0, 1.0, 0.0, 1.0, 0.0}, {5}, f64);

  std::cout << "best F1 threshold        = " << cls::best_threshold(scores, labels).value() << "\n";
  std::cout << "best Precision threshold = "
            << cls::best_threshold(scores, labels, cls::MetricKind::Precision).value() << "\n";

  return 0;
}
```

Output

```text
best F1 threshold        = 0.4
best Precision threshold = 0.9
```

### Example 3

`average_precision` on a perfectly-ranked case (all positives score above
all negatives, `AP = 1.0`) versus a label vector with no positives at all
(`AP = 0.0`, not an error).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series perfect_scores = pluss::make_series({0.9, 0.8, 0.3, 0.1}, {4}, f64);
  const pluss::Series perfect_labels = pluss::make_series({1.0, 1.0, 0.0, 0.0}, {4}, f64);
  std::cout << "perfect ranking AP = " << cls::average_precision(perfect_scores, perfect_labels).value() << "\n";

  const pluss::Series no_pos_scores = pluss::make_series({0.9, 0.5, 0.1}, {3}, f64);
  const pluss::Series no_pos_labels = pluss::make_series({0.0, 0.0, 0.0}, {3}, f64);
  std::cout << "no positives AP    = " << cls::average_precision(no_pos_scores, no_pos_labels).value() << "\n";

  return 0;
}
```

Output

```text
perfect ranking AP = 1
no positives AP    = 0
```

### Example 4

`objective` accepts exactly three distinct behaviors, not three distinct
values: `Recall` and `Sensitivity` are the same case (an alias pair, so
they always agree), and every `MetricKind` that is not `Precision`,
`Recall`, or `Sensitivity` — including one that has nothing to do with a
threshold curve, like `Mcc` — silently falls back to `F1` rather than
being rejected.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series scores = pluss::make_series({0.9, 0.8, 0.8, 0.4, 0.1}, {5}, f64);
  const pluss::Series labels = pluss::make_series({1.0, 1.0, 0.0, 1.0, 0.0}, {5}, f64);

  const double recall = cls::best_threshold(scores, labels, cls::MetricKind::Recall).value();
  const double sensitivity = cls::best_threshold(scores, labels, cls::MetricKind::Sensitivity).value();
  const double default_f1 = cls::best_threshold(scores, labels).value();
  const double mcc_falls_back = cls::best_threshold(scores, labels, cls::MetricKind::Mcc).value();

  std::cout << "Recall == Sensitivity      = " << std::boolalpha << (recall == sensitivity) << "\n";
  std::cout << "Mcc falls back to F1       = " << (mcc_falls_back == default_f1) << "\n";

  return 0;
}
```

Output

```text
Recall == Sensitivity      = true
Mcc falls back to F1       = true
```

## Changelog

- **0.1** — `threshold_curve`, `best_threshold`, `average_precision`.

## See Also

- [`ThresholdCurve`](ThresholdCurve.md) — the returned type.
- [`score_metrics`](score_metrics.md) — computing `AveragePrecision`
  (alongside other metrics) via `MetricKind` instead of calling this
  function by name.

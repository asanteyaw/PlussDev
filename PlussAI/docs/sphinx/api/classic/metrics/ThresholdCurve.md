# `pluss::ai::classic::ThresholdCurve`

Precision, recall, and F1 at every **achievable** binary decision
threshold — every distinct score value in the input, swept in one pass
rather than re-scoring the model once per candidate cutoff. Produced by
[`threshold_curve`](threshold_curve.md).

## Signature

```cpp
pluss::Series classic::ThresholdCurve::thresholds;  // descending
pluss::Series classic::ThresholdCurve::precision;
pluss::Series classic::ThresholdCurve::recall;
pluss::Series classic::ThresholdCurve::f1;
```

## Construction

Produced only by [`threshold_curve`](threshold_curve.md). A plain value
type — copy or store it freely.

## State

All four fields are `Float64`, rank-1, the same length — one entry per
**distinct** score value present in the input (tied scores collapse onto a
single point, since they are not separable by any threshold).

`thresholds` — `pluss::Series`
: Every achievable decision threshold, in **descending** order — the first
  entry is the highest score seen (predicting positive requires a score
  strictly above nothing higher exists, so this is the point where exactly
  one row is predicted positive), the last is the lowest.

`precision` — `pluss::Series`
: The precision achieved if the threshold were set at `thresholds[i]` — the
  fraction of rows scoring `>= thresholds[i]` that are truly positive.

`recall` — `pluss::Series`
: The recall achieved at that same threshold — the fraction of all truly
  positive rows scoring `>= thresholds[i]`. Non-decreasing as `i` increases
  (as the threshold drops, more rows are predicted positive, so recall can
  only rise or stay flat).

`f1` — `pluss::Series`
: The harmonic mean of `precision[i]` and `recall[i]` at each point.

## Methods

None — `ThresholdCurve` is a plain data holder, read directly.

## Practical Example

Five scores with one tie, swept into a 4-point curve (the tied pair
collapses to one threshold), and the point of maximum F1.

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
  std::cout << "precision  = " << curve.precision << "\n";
  std::cout << "recall     = " << curve.recall << "\n";
  std::cout << "f1         = " << curve.f1 << "\n";

  return 0;
}
```

Output

```text
thresholds = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.9, 0.8, 0.4, 0.1]
precision  = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 0.66666667, 0.75, 0.6]
recall     = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.33333333, 0.66666667, 1, 1]
f1         = pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.5, 0.66666667, 0.85714286, 0.75]
```

## Changelog

- **0.1** — `thresholds`, `precision`, `recall`, `f1`.

## See Also

- [`threshold_curve`](threshold_curve.md) — the producing function, plus
  `best_threshold` and `average_precision`, its two siblings built on the
  same sweep.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

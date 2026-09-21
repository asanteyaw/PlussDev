# `pluss::ai::classic::DetectFamilyOptions`

`DetectFamilyOptions` configures an outlier/anomaly detector. Today it is a
thin wrapper: one method selector plus the isolation-forest options that
method actually uses.

## Fields

### `method`

`DetectMethod method = DetectMethod::Isolation`

The detection algorithm. `Isolation` (isolation forest — outliers are the
points that take fewer random splits to isolate) is the only value today.

### `isolation`

`prep::OutlierIsolatorOptions isolation{}`

The full isolation-forest configuration — tree count, sampling, contamination
threshold, RNG. See
[`prep::outlier::OutlierIsolatorOptions`](../prep/outlier/OutlierIsolatorOptions.md)
for every field; this struct just carries it so it can sit inside a
[`ModelSpec`](ModelSpec.md)'s `FamilyOptions` variant alongside the other
eight families.

## Guidance

There is currently one detection method, so the only real configuration
surface is `isolation` — read
[`OutlierIsolatorOptions`](../prep/outlier/OutlierIsolatorOptions.md) for the
fields that actually change detector behavior (`contamination` /
`contamination_auto` most of all).

## Practical Example

Building a `Detect`-family transformer and reading which points it flags,
`isolation.contamination` set explicitly.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // 9 points clustered near 0, one far outlier.
  const cls::Series x = pluss::make_series(
      {0.0, 0.1, -0.1, 0.2, -0.2, 0.05, -0.05, 0.15, -0.15, 20.0}, {10}, f64).reshape({10, 1});
  cls::FeatureBatch batch{};
  batch.x = x;

  cls::DetectFamilyOptions opts{};
  opts.method = cls::DetectMethod::Isolation;
  opts.isolation.contamination_auto = false;
  opts.isolation.contamination = 0.1;  // expect ~10% outliers, matching 1-in-10 here

  cls::StatusOr<std::unique_ptr<cls::TransformerBase>> made = cls::make_transformer(
      cls::ModelSpec{cls::TaskKind::Transform, cls::ModelFamily::Detect, opts});
  made.value()->learn(batch);
  const cls::StatusOr<pluss::Series> scores = made.value()->map(batch);

  std::cout << "made.ok() = " << std::boolalpha << made.ok() << "\n";
  std::cout << "scores = " << scores.value() << "\n";

  return 0;
}
```

Output

```text
made.ok() = true
scores = pluss::Series(shape=[10,1], strides=[1,1], dtype=int64, platform=cpu, numel=10, contiguous=true)
data=[[1],
  [1],
  ...
 [-1]]
```

## Changelog

- **0.1** — `method` (currently only `Isolation`), `isolation`.

## See Also

- [`ModelSpec`](ModelSpec.md) — carries this as one `FamilyOptions`
  alternative.
- [`prep::outlier::OutlierIsolator`](../prep/outlier/OutlierIsolator.md) — the
  standalone transform-stage form of the same algorithm, for use inside a
  [`Flow`](../compose/Flow.md) rather than as the final model.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

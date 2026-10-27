<!-- pluss-hand-authored -->

# `lymph::metrics::regression`

[Back to `metrics`](../metrics.md)

`metrics::regression::report(truth, prediction, {kinds})` is the regression twin
of [`metrics::classification::report`](classification.md): it scores a truth
`Series` against a prediction `Series` you already hold and returns a
[`MetricReport`](MetricReport.md). `report(...).values` is the raw `Series` of
the requested metrics — there is no separate `*_metric_series` spelling.

## Signature

```cpp
lymph::metrics::MetricReport lymph::metrics::regression::report(
    const pluss::Series& truth,
    const pluss::Series& prediction,
    std::vector<lymph::MetricKind> requested = {});
```

## What It Does

Computes the requested regression `MetricKind`s in order and packs them into a
`MetricReport` — `values` aligned to `requested`, plus a named 1-element
`Series` per metric (`report.mse`, `report.rmse`, `report.r2`, `report.mae`,
`report.quantile_loss`). `requested` empty → the default set `Mse`, `Rmse`,
`R2`. Nothing is memoized.

## Parameters

`truth`
: The observed target values, as a `Series`.

`prediction`
: The predicted target values, same length as `truth`.

`requested` — `std::vector<MetricKind>`
: Which regression `MetricKind`s to compute, in order. Empty → the default set.

## Returns

A [`MetricReport`](MetricReport.md) with the regression slots filled. Read
`report.values` for the raw `Series`, or a named slot with `.item<double>()`.

## Errors

- `truth` and `prediction` of different lengths raise.
- A classification `MetricKind` (e.g. `F1`) passed here is not a regression
  metric and is not computed.

## Practical Examples

<!-- these examples establish: report({...}) + .values, the default set, and reading named slots. -->

### Example 1

`report({...})` for the named bag; `.values` for the raw `Series`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace regression = lymph::metrics::regression;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth =
      pluss::make_series({10., 12., 14., 16., 18., 20., 22., 24.}, {8}, f64);
  const pluss::Series predicted =
      pluss::make_series({10.5, 11.5, 14.2, 15.6, 18.4, 19.7, 22.3, 23.6}, {8}, f64);

  const lymph::metrics::MetricReport report =
      regression::report(truth, predicted, {lymph::MetricKind::R2, lymph::MetricKind::Mse});

  std::cout << "values (r2, mse) = " << report.values << "\n";
  std::cout << "report.r2  = " << report.r2.item<double>() << "\n";
  std::cout << "report.mse = " << report.mse.item<double>() << "\n";

  return 0;
}
```

Output

```text
values (r2, mse) = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.99285714, 0.15]
report.r2  = 0.992857
report.mse = 0.15
```

### Example 2

`report()` with no metric list uses the default regression set.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace regression = lymph::metrics::regression;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth =
      pluss::make_series({3.0, 5.0, 7.0, 9.0, 11.0, 13.0, 15.0, 17.0}, {8}, f64);
  const pluss::Series predicted =
      pluss::make_series({3.2, 4.7, 7.1, 9.4, 10.6, 13.1, 15.2, 16.7}, {8}, f64);

  const lymph::metrics::MetricReport report =
      regression::report(truth, predicted);

  const std::vector<double> values = report.values.host<double>();
  std::cout << "default regression metrics (" << report.requested.size() << "):\n";
  for (std::size_t i = 0; i < report.requested.size(); ++i) {
    std::cout << "  kind " << static_cast<int>(report.requested[i])
              << " = " << values[i] << "\n";
  }

  return 0;
}
```

Output

```text
default regression metrics (3):
  kind 7 = 0.075
  kind 8 = 0.273861
  kind 10 = 0.996429
```

### Example 3

`.values` is the accessor-free raw `Series`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace regression = lymph::metrics::regression;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth =
      pluss::make_series({3.0, 5.0, 7.0, 9.0, 11.0, 13.0, 15.0, 17.0}, {8}, f64);
  const pluss::Series predicted =
      pluss::make_series({3.2, 4.7, 7.1, 9.4, 10.6, 13.1, 15.2, 16.7}, {8}, f64);

  const pluss::Series values =
      regression::report(truth, predicted,
                         {lymph::MetricKind::R2, lymph::MetricKind::Rmse, lymph::MetricKind::Mae})
          .values;

  std::cout << "report values (r2, rmse, mae) = " << values << "\n";

  return 0;
}
```

Output

```text
report values (r2, rmse, mae) = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.99642857, 0.27386128, 0.25]
```

## See Also

- [`MetricReport`](MetricReport.md) — the bag `report` returns.
- [`classification`](classification.md) — the classification twin.
- [`CrossValidationReport`](CrossValidationReport.md) — regression metrics
  aggregated across folds.

<!-- pluss-hand-authored -->

# `lymph::metrics::SeriesCompareReport`

[Back to `metrics`](../metrics.md)

`SeriesCompareReport` is the **element-wise difference** between two aligned
`Series` — the answer to "how far apart are these two vectors?". It is what
`compare_series` returns; `max_abs_diff` is the one-value shortcut.

## Scope And Alias

The type is `lymph::metrics::SeriesCompareReport`, re-exported at `lymph::`
scope with the umbrella alias `lymph::reports::SeriesCompare`.
`compare_series` and `max_abs_diff` are also re-exported unqualified
(`lymph::compare_series`).

## Fields

`Series abs_error`
: `|lhs - rhs|` element-wise — one value per input element, same length as the
  inputs.

`Series max_abs_diff`
: The largest element of `abs_error`, as a **1-element `Series`**. Read with
  `.item<double>()`.

`Series mae`
: The mean of `abs_error` (mean absolute error), as a 1-element `Series`.

Both inputs are cast to `Float64` on CPU and must have the same element count;
a length mismatch raises.

## Producing One

```cpp
lymph::metrics::SeriesCompareReport compare_series(const Series& lhs, const Series& rhs);
double                              max_abs_diff  (const Series& lhs, const Series& rhs);
```

`max_abs_diff(a, b)` is exactly `compare_series(a, b).max_abs_diff.item<double>()`.

## Reading a `SeriesCompareReport`

### Full report

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series expected =
      pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64);
  const pluss::Series actual =
      pluss::make_series({1.0, 2.1, 2.7, 4.0, 5.5}, {5}, f64);

  const lymph::metrics::SeriesCompareReport diff =
      lymph::metrics::compare_series(expected, actual);

  std::cout << "abs_error   =";
  for (const double v : diff.abs_error.to_vec<double>()) std::cout << " " << v;
  std::cout << "\nmax_abs_diff = " << diff.max_abs_diff.item<double>() << "\n";
  std::cout << "mae         = " << diff.mae.item<double>() << "\n";

  return 0;
}
```

Output

```text
abs_error   = 0 0.1 0.3 0 0.5
max_abs_diff = 0.5
mae         = 0.18
```

### The shortcut

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series a =
      pluss::make_series({0.10, 0.20, 0.30, 0.40, 0.50}, {5}, f64);
  const pluss::Series b =
      pluss::make_series({0.101, 0.198, 0.307, 0.400, 0.494}, {5}, f64);

  const double gap = lymph::metrics::max_abs_diff(a, b);
  std::cout << "max_abs_diff = " << gap << "\n";
  std::cout << "within 1e-2 ? " << std::boolalpha << (gap < 1e-2) << "\n";

  return 0;
}
```

Output

```text
max_abs_diff = 0.007
within 1e-2 ? true
```

### Reproducibility check

The common use: run something twice, compare the outputs, assert the gap is
negligible. Identical `Series` give an all-zero `abs_error`.

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

  const pluss::Series x = pluss::make_series(
      {1.0, 1.4, 1.8, 2.2, 2.6, 3.0, 3.4, 3.8, 4.2, 4.6}, {10}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {10}, f64);
  const DataFrame frame({{"x", Thread(x)}, {"target", Thread(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x"};
  data_options.label_feature = "target";

  lymph::TrainOptions fixed{};
  fixed.random = {.seed = 7ULL, .deterministic = true};

  const lymph::models::Model a =
      lymph::models::logistic_regression().learn(frame, data_options, fixed);
  const lymph::models::Model b =
      lymph::models::logistic_regression().learn(frame, data_options, fixed);

  const pluss::Series pred_a = a.infer(frame.features({"x"}));
  const pluss::Series pred_b = b.infer(frame.features({"x"}));

  const lymph::metrics::SeriesCompareReport diff =
      lymph::metrics::compare_series(pred_a, pred_b);
  std::cout << "identical run: max_abs_diff = " << diff.max_abs_diff.item<double>() << "\n";
  std::cout << "abs_error all zero? " << std::boolalpha
            << (diff.abs_error.to_vec<double>() == std::vector<double>(10, 0.0)) << "\n";

  return 0;
}
```

Output

```text
identical run: max_abs_diff = 0
abs_error all zero? true
```

## Methods

None. `SeriesCompareReport` is a plain value type — the three `Series` fields
are its whole surface.

## Changelog

- **0.1** — `abs_error` / `max_abs_diff` / `mae` from `compare_series`;
  `max_abs_diff(a, b)` shortcut; umbrella alias `reports::SeriesCompare`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

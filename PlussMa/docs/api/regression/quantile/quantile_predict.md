<!-- generated from quantile_predict.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `regression::quantile_predict`

Applies a fitted quantile regression to new predictor rows and returns the fitted conditional quantile.

## Signature

```cpp
Series regression::quantile_predict(const regression::QuantileFitResult &fit, const Series &x, Config cfg = {});
```

## What It Does

Multiplies the new rows by the stored coefficients. The intercept convention comes from the fit: give only the original predictors. Call it to read the fitted conditional quantile at new points; to draw a band, fit several τ and predict with each (Example 2), and note that the lines of different τ are fitted separately, so they can cross where the data are thin. The answer is the estimated conditional quantile at the fit's `tau`, not the mean; this function returns no interval.

## Parameters

`fit`
: A completed result from `quantile_fit`.

`x`
: The new rows. For a one-predictor fit, `[m]` is accepted. For several predictors use `[m, p]`; a single new observation is `[1, p]`. A matrix whose width equals `fit.k_params` is read as the full design with its leading column of ones.

`cfg`
: Computation configuration for the returned `Series`.

## Returns

A one-dimensional `Series` with one value per new row.

## Errors

Throws `quantile_predict: fit.coefficients is undefined` for a default-constructed fit, and `quantile_predict: expected X with either num_predictors columns or k_params columns` when the width fits neither.

## Practical Examples

### Example 1: the median beyond the data

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  const auto fit = reg::quantile_fit(y, x);
  const auto future = pluss::make_series(std::vector<double>{13.0, 14.0}, {2}, as_f64());
  out.writeln("median at x = 13, 14", reg::quantile_predict(fit, future).items<double>());
}
```

Output

```text
median at x = 13, 14 [20.34000, 21.86000]
```

### Example 2: a band of quantiles

Three quantile fits give a band at a new point: the 0.1, 0.5 and 0.9 conditional quantiles at x = 13.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  const auto at = pluss::make_series(std::vector<double>{13.0}, {1}, as_f64());
  for (const double tau : {0.1, 0.5, 0.9}) {
    reg::QuantileOptions options;
    options.tau = tau;
    out.writeln("tau", tau, " quantile at x = 13", reg::quantile_predict(reg::quantile_fit(y, x, options), at).items<double>()[0]);
  }
}
```

Output

```text
tau 0.10000  quantile at x = 13 9.45000
tau 0.50000  quantile at x = 13 20.34000
tau 0.90000  quantile at x = 13 21.72000
```

### Example 3: several predictors, one new row

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  std::vector<double> flat;
  for (int i = 0; i < 12; ++i) flat.push_back(1.0 * (i + 1));
  for (int i = 0; i < 12; ++i) flat.push_back((i % 3) * 1.5);
  const auto x2 = pluss::make_series(flat, {2, 12}, as_f64()).transpose(0, 1);
  const auto fit = reg::quantile_fit(y, x2);
  const auto row = pluss::make_series(std::vector<double>{13.0, 1.5}, {1, 2}, as_f64());
  out.writeln("median at (13, 1.5)", reg::quantile_predict(fit, row).items<double>());
}
```

Output

```text
median at (13, 1.5) [19.95444]
```

### Example 4: the fitted values again

Predicting on the training rows reproduces `fitted_values`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  const auto fit = reg::quantile_fit(y, x);
  out.writeln("predict      ", reg::quantile_predict(fit, x).items<double>());
  out.writeln("fitted_values", fit.fitted_values.items<double>());
}
```

Output

```text
predict       [2.10000, 3.62000, 5.14000, 6.66000, 8.18000, 9.70000, 11.22000, 12.74000, 14.26000, 15.78000, 17.30000, 18.82000]
fitted_values [2.10000, 3.62000, 5.14000, 6.66000, 8.18000, 9.70000, 11.22000, 12.74000, 14.26000, 15.78000, 17.30000, 18.82000]
```

### Example 5: the errors

The errors of prediction: a fit that was never made, and rows of the wrong width.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const std::exception &e) {
      const std::string text(e.what());
      out.writeln(text.substr(0, text.find('\n')));
    }
  };
  const auto fit = reg::quantile_fit(y, x);
  report([&] { reg::quantile_predict(reg::QuantileFitResult{}, x); });
  report([&] { reg::quantile_predict(fit, pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {2, 3}, as_f64())); });
}
```

Output

```text
subsp::stats::regression::quantile_predict: fit.coefficients is undefined
subsp::stats::regression::quantile_predict: expected X with either num_predictors columns or k_params columns
```

## See Also

[`quantile_fit`](quantile_fit.md) produces the fit, and [`QuantileFitResult`](QuantileFitResult.md) holds the coefficients applied.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="quantile_fit.html">
    <span class="label">Previous</span>
    <span class="title">quantile_fit</span>
  </a>
  <a class="next-link" href="quantile_summary.html">
    <span class="label">Next</span>
    <span class="title">quantile_summary</span>
  </a>
</div>
<!-- apiref:nav:end -->

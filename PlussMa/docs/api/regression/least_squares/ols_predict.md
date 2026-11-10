<!-- generated from ols_predict.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `regression::ols_predict`

Applies a fitted least-squares model to new predictor rows and returns the fitted values.

## Signature

```cpp
Series regression::ols_predict(const regression::OLSFitResult &fit, const Series &x, Config cfg = {});
```

## What It Does

Multiplies the new rows by the stored coefficients. The intercept convention comes from the fit: give only the original predictors, and do not prepend a column of ones. A matrix whose width equals `fit.k_params` is read as the full design, with its leading column of ones supplied by the caller. The answer is the fitted mean; this function returns no confidence or prediction interval. Call it to evaluate the fitted line or plane at new rows, for example to forecast the response when the predictors are known.

## Parameters

`fit`
: A completed result from `ols_fit`.

`x`
: The new rows. For a one-predictor fit, `[m]` is accepted. For several predictors use `[m, p]`; a single new observation is `[1, p]`.

`cfg`
: Computation configuration for the returned `Series`.

## Returns

A one-dimensional `Series` with one fitted value per new row.

## Errors

Throws `ols_predict: fit.coefficients is undefined` for a default-constructed fit, and `ols_predict: expected X with either num_predictors columns or k_params columns` when the width fits neither.

## Practical Examples

### Example 1: new values of one predictor

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.3, 4.1, 6.2, 7.9, 10.2, 12.1, 13.8, 16.2}, {8}, as_f64());
  const auto fit = reg::ols_fit(y, x);
  const auto future = pluss::make_series(std::vector<double>{9.0, 10.0}, {2}, as_f64());
  out.writeln("fitted at 9 and 10", reg::ols_predict(fit, future).items<double>());
}
```

Output

```text
fitted at 9 and 10 [17.98214, 19.95595]
```

### Example 2: the training rows

Predicting on the training rows reproduces `fitted_values`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.3, 4.1, 6.2, 7.9, 10.2, 12.1, 13.8, 16.2}, {8}, as_f64());
  const auto fit = reg::ols_fit(y, x);
  out.writeln("predict      ", reg::ols_predict(fit, x).items<double>());
  out.writeln("fitted_values", fit.fitted_values.items<double>());
}
```

Output

```text
predict       [2.19167, 4.16548, 6.13929, 8.11310, 10.08690, 12.06071, 14.03452, 16.00833]
fitted_values [2.19167, 4.16548, 6.13929, 8.11310, 10.08690, 12.06071, 14.03452, 16.00833]
```

### Example 3: two predictors

A matrix with one row per new observation. The first call predicts a single new observation, written as a `[1, 2]` matrix, and the second predicts two.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto y = pluss::make_series(std::vector<double>{2.3, 4.1, 6.2, 7.9, 10.2, 12.1, 13.8, 16.2}, {8}, as_f64());
  std::vector<double> flat;
  for (int i = 0; i < 8; ++i) flat.push_back(1.0 * (i + 1));
  for (int i = 0; i < 8; ++i) flat.push_back((i % 3) * 1.5);
  const auto x2 = pluss::make_series(flat, {2, 8}, as_f64()).transpose(0, 1);
  const auto fit = reg::ols_fit(y, x2);
  const auto one = pluss::make_series(std::vector<double>{9.0, 1.5}, {1, 2}, as_f64());
  const auto two = pluss::make_series(std::vector<double>{9.0, 1.5, 10.0, 0.0}, {2, 2}, as_f64());
  out.writeln("one observation ", reg::ols_predict(fit, one).items<double>());
  out.writeln("two observations", reg::ols_predict(fit, two).items<double>());
}
```

Output

```text
one observation  [17.97889]
two observations [17.97889, 19.85833]
```

### Example 4: the full design

A matrix as wide as `k_params` is read as the design itself, with the ones column included.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.3, 4.1, 6.2, 7.9, 10.2, 12.1, 13.8, 16.2}, {8}, as_f64());
  const auto fit = reg::ols_fit(y, x);
  const auto design = pluss::make_series(std::vector<double>{1.0, 9.0, 1.0, 10.0}, {2, 2}, as_f64());
  out.writeln("full design", reg::ols_predict(fit, design).items<double>());
  out.writeln("predictors only", reg::ols_predict(fit, pluss::make_series(std::vector<double>{9.0, 10.0}, {2}, as_f64())).items<double>());
}
```

Output

```text
full design [17.98214, 19.95595]
predictors only [17.98214, 19.95595]
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
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.3, 4.1, 6.2, 7.9, 10.2, 12.1, 13.8, 16.2}, {8}, as_f64());
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const std::exception &e) {
      const std::string text(e.what());
      out.writeln(text.substr(0, text.find('\n')));
    }
  };
  const auto fit = reg::ols_fit(y, x);
  report([&] { reg::ols_predict(reg::OLSFitResult{}, x); });
  report([&] { reg::ols_predict(fit, pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {2, 3}, as_f64())); });
}
```

Output

```text
subsp::stats::regression::ols_predict: fit.coefficients is undefined
subsp::stats::regression::ols_predict: expected X with either num_predictors columns or k_params columns
```

## See Also

[`ols_fit`](ols_fit.md) produces the fit, and [`OLSFitResult`](OLSFitResult.md) holds the coefficients applied.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ols_fit.html">
    <span class="label">Previous</span>
    <span class="title">ols_fit</span>
  </a>
  <a class="next-link" href="ols_summary.html">
    <span class="label">Next</span>
    <span class="title">ols_summary</span>
  </a>
</div>
<!-- apiref:nav:end -->

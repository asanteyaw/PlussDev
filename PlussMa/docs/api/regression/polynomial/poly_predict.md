<!-- generated from poly_predict.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `regression::poly_predict`

Evaluates a fitted polynomial regression at new predictor values.

## Signature

```cpp
Series regression::poly_predict(const regression::PolyFitResult &fit, const Series &x, Config cfg = {});
```

## What It Does

Expands the new rows with the degree and cross-term choice recorded on the fit and applies the stored coefficients, so the caller supplies only the original predictors, never powers. Call it to evaluate the fitted curve at new points; the answer is the fitted mean and this function returns no interval. Extrapolation outside the fitted range is allowed and, for a polynomial, treacherous: a curve that fits well inside the data can swing quickly beyond it.

## Parameters

`fit`
: A completed result from `poly_fit`.

`x`
: The new rows, `[m]` for one predictor or `[m, p]` for `p`.

`cfg`
: Computation configuration for the returned `Series`.

## Returns

A one-dimensional `Series` with one value per new row.

## Errors

Throws `poly_predict: fit.ols.coefficients is undefined` for a default-constructed fit.

## Practical Examples

### Example 1: inside the data

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{3.1, 4.9, 8.2, 12.8, 19.1, 26.8, 36.2, 46.9}, {8}, as_f64());
  const auto fit = reg::poly_fit(y, x);
  const auto at = pluss::make_series(std::vector<double>{2.5, 4.5, 6.5}, {3}, as_f64());
  out.writeln("fitted at 2.5, 4.5, 6.5", reg::poly_predict(fit, at).items<double>());
}
```

Output

```text
fitted at 2.5, 4.5, 6.5 [6.30327, 15.79375, 31.31280]
```

### Example 2: beyond the data

A quadratic and a cubic that agree closely inside the data diverge once x goes past it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{3.1, 4.9, 8.2, 12.8, 19.1, 26.8, 36.2, 46.9}, {8}, as_f64());
  const auto at = pluss::make_series(std::vector<double>{9.0, 12.0, 20.0}, {3}, as_f64());
  for (const std::int64_t degree : {2, 3}) {
    reg::PolyOptions options;
    options.degree = degree;
    out.writeln("degree", degree, " at 9, 12, 20", reg::poly_predict(reg::poly_fit(y, x, options), at).items<double>());
  }
}
```

Output

```text
degree 2  at 9, 12, 20 [59.18929, 105.07500, 293.75119]
degree 3  at 9, 12, 20 [59.31429, 105.96515, 302.79286]
```

### Example 3: the training rows

Predicting on the training rows reproduces the fitted values.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{3.1, 4.9, 8.2, 12.8, 19.1, 26.8, 36.2, 46.9}, {8}, as_f64());
  const auto fit = reg::poly_fit(y, x);
  out.writeln("predict     ", reg::poly_predict(fit, x).items<double>());
  out.writeln("fitted_values", fit.ols.fitted_values.items<double>());
}
```

Output

```text
predict      [3.14167, 4.87262, 8.11071, 12.85595, 19.10833, 26.86786, 36.13452, 46.90833]
fitted_values [3.14167, 4.87262, 8.11071, 12.85595, 19.10833, 26.86786, 36.13452, 46.90833]
```

### Example 4: two predictors

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x1 = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, {10}, as_f64());
  const auto x2 = pluss::make_series(std::vector<double>{3, 1, 4, 1, 5, 9, 2, 6, 5, 3}, {10}, as_f64());
  const auto xx = x1.unsqueeze(1).unite({x2.unsqueeze(1)}, 1);
  const auto yy = pluss::make_series(std::vector<double>{4.22, 3.7, 8.45, 6.6, 13.35, 22.38, 11.7, 21.82, 21.45, 18.6}, {10}, as_f64());
  reg::PolyOptions options;
  options.include_cross = true;
  const auto fit = reg::poly_fit(yy, xx, options);
  const auto row = pluss::make_series(std::vector<double>{4.0, 5.0}, {1, 2}, as_f64());
  out.writeln("fitted at (4, 5)", reg::poly_predict(fit, row).items<double>());
}
```

Output

```text
fitted at (4, 5) [11.50191]
```

### Example 5: the errors

The error of prediction: a fit that was never made.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{3.1, 4.9, 8.2, 12.8, 19.1, 26.8, 36.2, 46.9}, {8}, as_f64());
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const std::exception &e) {
      const std::string text(e.what());
      out.writeln(text.substr(0, text.find('\n')));
    }
  };
  report([&] { reg::poly_predict(reg::PolyFitResult{}, x); });
}
```

Output

```text
subsp::stats::regression::poly_predict: fit.ols.coefficients is undefined
```

## See Also

[`poly_fit`](poly_fit.md) produces the fit, and [`PolyFitResult`](PolyFitResult.md) holds the coefficients and the expansion that is repeated.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="poly_fit.html">
    <span class="label">Previous</span>
    <span class="title">poly_fit</span>
  </a>
  <a class="next-link" href="poly_summary.html">
    <span class="label">Next</span>
    <span class="title">poly_summary</span>
  </a>
</div>
<!-- apiref:nav:end -->

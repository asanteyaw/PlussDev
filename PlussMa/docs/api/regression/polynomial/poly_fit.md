<!-- generated from poly_fit.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `regression::poly_fit`

Fits a polynomial regression of the response on the powers, and optionally the products, of one or more predictors, by least squares.

## Signature

```cpp
regression::PolyFitResult regression::poly_fit(const Series &y, const Series &x, const regression::PolyOptions &options = {}, Config cfg = {});
```

## What It Does

Expands `x` with `poly_expand` to the chosen degree (with the mixed terms when `include_cross` is set), then fits the response on the expanded columns, plus an intercept unless `add_intercept` is false, by ordinary or case-weighted least squares. Call it when the response is curved in the predictors but you still want the standard errors, t tests and intervals of a least-squares fit, and the form of the curve (a polynomial) is acceptable; it saves expanding the columns by hand and names them. For a response that is not Gaussian use `glm_fit` on `poly_expand` columns, and for a quantile use `quantile_fit` on them (see `poly_expand`). The model is nonlinear in `x` and linear in the coefficients, so every property of a least-squares fit holds: the standard errors, t tests, intervals and R² are those of the expanded design. The nested `ols` result carries them, and the expanded columns are named from the predictors: `x1`, `x1^2`, `x1^3`, `x1*x2`.

## Parameters

`y`
: The response, `[n]`.

`x`
: The predictors, `[n]` for one or `[n, p]` for `p`.

`options`
: A [`PolyOptions`](PolyOptions.md) choosing the degree, cross terms, intercept, level, weights and labels.

`cfg`
: Computation configuration for the returned `Series`.

## Returns

A [`PolyFitResult`](PolyFitResult.md).

## Errors

Throws `poly_fit: degree must be >= 1` for a degree below one, `ols_fit: requires n >= number of parameters` when the expansion has more columns than there are observations, and the other errors of `ols_fit` (row mismatch between `y` and `x`, invalid `alpha`, negative weights and so on).

## Practical Examples

### Example 1: a quadratic

Eight points on a curve. The fit recovers the quadratic and its coefficients are all significant.

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
  out.writeln(fit.ols.coefficient_names, fit.ols.coefficients.items<double>());
  out.writeln("R^2", fit.ols.rsquared, " p values", fit.ols.p_values.items<double>());
}
```

Output

```text
[const, x1, x1^2] [2.91786, -0.52976, 0.75357]
R^2 0.99999  p values [0.00000, 0.00010, 0.00000]
```

### Example 2: choosing the degree

Comparing adjusted R² and AIC across degrees. Both jump from degree one to degree two and are then nearly flat: the adjusted R² is 0.99998, 0.99998, 0.99999 for degrees two, three and four. The AIC is slightly lower at degree four, but with eight points and five coefficients a difference that small is not reliable evidence of a better curve; choose the simplest degree whose fit is adequate, here two.

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
  for (const std::int64_t degree : {1, 2, 3, 4}) {
    reg::PolyOptions options;
    options.degree = degree;
    const auto fit = reg::poly_fit(y, x, options);
    out.writeln("degree", degree, " adjusted R^2", fit.ols.adj_rsquared, " AIC", fit.ols.aic);
  }
}
```

Output

```text
degree 1  adjusted R^2 0.93592  AIC 46.53419
degree 2  adjusted R^2 0.99998  AIC -18.24422
degree 3  adjusted R^2 0.99998  AIC -17.71045
degree 4  adjusted R^2 0.99999  AIC -20.03031
```

### Example 3: two predictors with a cross term

The response depends on two predictors and on their product. Without the cross term the fit misses it.

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
  options.degree = 2;
  options.predictor_names = {"a", "b"};
  const auto plain = reg::poly_fit(yy, xx, options);
  options.include_cross = true;
  const auto crossed = reg::poly_fit(yy, xx, options);
  out.writeln("without cross terms: R^2", plain.ols.rsquared);
  out.writeln("with cross terms   : R^2", crossed.ols.rsquared);
  out.writeln(crossed.ols.coefficient_names, crossed.ols.coefficients.items<double>());
}
```

Output

```text
without cross terms: R^2 0.99084
with cross terms   : R^2 0.99955
[const, a, a^2, b, b^2, a*b] [1.09504, 0.97602, 0.00272, 0.47520, 0.00367, 0.19958]
```

### Example 4: a model through the origin

With the intercept removed the curve passes through zero. The names show that no constant column is present.

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
  reg::PolyOptions options;
  options.add_intercept = false;
  const auto fit = reg::poly_fit(y, x, options);
  out.writeln(fit.ols.coefficient_names, fit.ols.coefficients.items<double>());
}
```

Output

```text
[x1, x1^2] [0.83547, 0.61972]
```

### Example 5: the same fit as least squares

A polynomial fit is `ols_fit` on the expanded matrix, so the two agree exactly.

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
  const auto poly = reg::poly_fit(y, x);
  const auto ols = reg::ols_fit(y, reg::poly_expand(x, 2));
  out.writeln("poly_fit", poly.ols.coefficients.items<double>());
  out.writeln("ols_fit ", ols.coefficients.items<double>());
}
```

Output

```text
poly_fit [2.91786, -0.52976, 0.75357]
ols_fit  [2.91786, -0.52976, 0.75357]
```

### Example 6: the errors

Bad calls throw a message that names the problem: a degree below one, an expansion with more columns than observations, and negative weights.

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
  reg::PolyOptions options;
  options.degree = 0;
  report([&] { reg::poly_fit(y, x, options); });
  options.degree = 8;
  report([&] { reg::poly_fit(y, x, options); });
  options.degree = 2;
  options.weights = pluss::make_series(std::vector<double>{1, 1, 1, 1, 1, 1, 1, -1}, {8}, as_f64());
  report([&] { reg::poly_fit(y, x, options); });
}
```

Output

```text
subsp::stats::regression::poly_fit: degree must be >= 1
subsp::stats::regression::ols_fit: requires n >= number of parameters
subsp::stats::regression::ols_fit: weights must be non-negative
```

## See Also

[`poly_predict`](poly_predict.md) evaluates the curve at new points, [`PolyOptions`](PolyOptions.md) lists every control, and [`PolyFitResult`](PolyFitResult.md) describes the output.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="poly_expand.html">
    <span class="label">Previous</span>
    <span class="title">poly_expand</span>
  </a>
  <a class="next-link" href="poly_predict.html">
    <span class="label">Next</span>
    <span class="title">poly_predict</span>
  </a>
</div>
<!-- apiref:nav:end -->

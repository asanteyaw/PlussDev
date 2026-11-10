<!-- generated from quantile_fit.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `regression::quantile_fit`

Fits a linear model for one conditional quantile of the response by minimising the check loss, and returns the coefficients, their uncertainty and the measures of fit.

## Signature

```cpp
regression::QuantileFitResult regression::quantile_fit(const Series &y, const Series &x, const regression::QuantileOptions &options = {}, Config cfg = {});
```

## What It Does

Finds the coefficients β that minimise Σ ρτ(yᵢ − xᵢβ), where ρτ charges τ per unit of a positive residual and 1 − τ per unit of a negative one, so that the fitted line leaves a fraction τ of the observations below it. The loss is not smooth, and the fit minimises it by iteratively reweighted least squares, with the weights τ/|rᵢ| (or (1 − τ)/|rᵢ|) floored at `eps`. Standard errors use the kernel estimate of the residual density at zero and either the robust sandwich or the classical form; tests and intervals use the Student t distribution on `df_resid` degrees of freedom.

Call it when the question is about part of the distribution of the response, not its mean: the median of data with outliers, the 0.9 quantile as a risk or capacity level, or how the spread of the response changes across the predictors. `ols_fit` estimates the mean, which a single wild observation can move a long way; `quantile_fit` at τ = 0.5 estimates the median, which it cannot (Example 4). Fitting several τ on the same data draws the whole conditional distribution (Example 2).

An intercept column is added unless `add_intercept` is false. Rows with non-finite values are dropped unless `drop_missing` is false, and rows of weight zero are not used. A rank-deficient design is solved with a pseudoinverse and the result records the rank and a warning.

## Parameters

`y`
: The response, `[n]`.

`x`
: The predictors. A vector `[n]` is one predictor; a matrix `[n, p]` is `p` predictors.

`options`
: A [`QuantileOptions`](QuantileOptions.md) choosing the quantile, covariance estimator, level, iteration controls, weights and labels.

`cfg`
: Computation configuration for the returned `Series`.

## Returns

A [`QuantileFitResult`](QuantileFitResult.md).

## Errors

The call throws, with the message beginning `subsp::stats::regression::quantile_fit:`, for each of these: `row mismatch between y and X`; `requires n >= number of parameters`; `tau must be in (0,1)`; `alpha must be in (0,1)`; `rcond must be > 0`; `max_iter must be > 0`; `tol must be > 0`; `eps must be > 0`; `weights must be non-negative`; `predictor_names size must equal number of design columns before intercept handling`; and `non-finite values found and drop_missing=false`.

## Practical Examples

### Example 1: the median

Twelve points whose spread grows with x. The median line is the default fit.

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
  out.writeln("coefficients", fit.coefficients.items<double>(), fit.coefficient_names);
  out.writeln("iterations", fit.iterations, " converged", fit.converged, " objective", fit.objective);
}
```

Output

```text
coefficients [0.58000, 1.52000] [const, x1]
iterations 21  converged true  objective 14.04000
```

### Example 2: the spread of the response

The 0.1, 0.5 and 0.9 quantiles of the same data have different slopes: the upper quantile rises fastest because the spread grows. Ordinary least squares would report only the middle.

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
  for (const double tau : {0.1, 0.5, 0.9}) {
    reg::QuantileOptions options;
    options.tau = tau;
    const auto fit = reg::quantile_fit(y, x, options);
    out.writeln("tau", tau, " intercept and slope", fit.coefficients.items<double>());
  }
  const auto ols = reg::ols_fit(y, x);
  out.writeln("least squares mean line   ", ols.coefficients.items<double>());
}
```

Output

```text
tau 0.10000  intercept and slope [1.32500, 0.62500]
tau 0.50000  intercept and slope [0.58000, 1.52000]
tau 0.90000  intercept and slope [0.66000, 1.62000]
least squares mean line    [0.49545, 1.23916]
```

### Example 3: inference

The robust standard errors, t statistics, p-values and 95% interval of the slope of the 0.9 quantile.

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
  reg::QuantileOptions options;
  options.tau = 0.9;
  const auto fit = reg::quantile_fit(y, x, options);
  const auto ci = fit.conf_int.reshape({4}).items<double>();
  out.writeln("std_errors", fit.std_errors.items<double>());
  out.writeln("t_values  ", fit.t_values.items<double>(), " p_values", fit.p_values.items<double>());
  out.writeln("slope interval", ci[2], ci[3]);
}
```

Output

```text
std_errors [1.99561, 0.29707]
t_values   [0.33073, 5.45325]  p_values [0.74768, 0.00028]
slope interval 0.95809 2.28191
```

### Example 4: robustness to an outlier

Corrupting the largest response by a factor of ten moves the least-squares line a long way and leaves the median line exactly where it was: the corrupted point was above the line before and is above it after, and the median fit depends only on which side of the line each point lies.

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
  auto values = y.items<double>();
  values[11] = 201.0;
  const auto wild = pluss::make_series(values, {12}, as_f64());
  out.writeln("median line before", reg::quantile_fit(y, x).coefficients.items<double>());
  out.writeln("median line after ", reg::quantile_fit(wild, x).coefficients.items<double>());
  out.writeln("least squares before", reg::ols_fit(y, x).coefficients.items<double>());
  out.writeln("least squares after ", reg::ols_fit(wild, x).coefficients.items<double>());
}
```

Output

```text
median line before [0.58000, 1.52000]
median line after  [0.58000, 1.52000]
least squares before [0.49545, 1.23916]
least squares after  [-29.65455, 8.19685]
```

### Example 5: two predictors with names

A matrix `x` of two columns, named for the report.

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
  reg::QuantileOptions options;
  options.response_name = "price";
  options.predictor_names = {"size", "age"};
  const auto fit = reg::quantile_fit(y, x2, options);
  out.writeln(fit.coefficient_names, fit.coefficients.items<double>());
}
```

Output

```text
[const, size, age] [0.53209, 1.56791, -0.64034]
```

### Example 6: the errors

Bad calls throw a message that names the problem: a quantile outside (0, 1), a non-positive smoothing floor, and a response of the wrong length.

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
  reg::QuantileOptions options;
  options.tau = 1.0;
  report([&] { reg::quantile_fit(y, x, options); });
  options.tau = 0.5;
  options.eps = 0.0;
  report([&] { reg::quantile_fit(y, x, options); });
  report([&] { reg::quantile_fit(y.slice(0, 0, 5), x); });
}
```

Output

```text
subsp::stats::regression::quantile_fit: tau must be in (0,1)
subsp::stats::regression::quantile_fit: eps must be > 0
subsp::stats::regression::quantile_fit: row mismatch between y and X
```

## See Also

[`quantile_predict`](quantile_predict.md) applies the result to new rows, [`QuantileOptions`](QuantileOptions.md) lists every control, and [`QuantileFitResult`](QuantileFitResult.md) describes each output.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="QuantileFitResult.html">
    <span class="label">Previous</span>
    <span class="title">QuantileFitResult</span>
  </a>
  <a class="next-link" href="quantile_predict.html">
    <span class="label">Next</span>
    <span class="title">quantile_predict</span>
  </a>
</div>
<!-- apiref:nav:end -->

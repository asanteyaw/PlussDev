<!-- generated from QuantileCovariance.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `regression::QuantileCovariance`

Selects the estimator of the covariance of the quantile-regression coefficients, which fixes their standard errors, t statistics, p-values and confidence intervals.

## Signature

```cpp
regression::QuantileCovariance regression::QuantileOptions::covariance = regression::QuantileCovariance::Robust;
```

Both estimators need the density of the residuals at zero, which the fit estimates by a kernel and reports as `residual_density0`. They differ in what they assume about the spread of the errors across the predictors.

## Fields

### `Robust`

The default. A heteroskedasticity-robust sandwich, (X′X)⁻¹ X′ D X (X′X)⁻¹, in which each observation contributes (τ/f₀)² when it lies above the fitted line and ((1 − τ)/f₀)² when it lies below, with f₀ the residual density at zero. It stays valid when the spread of the response changes with the predictors, which is the situation that makes quantile regression worth using. The program fits the 0.9 quantile of data whose spread grows with x and prints the standard errors both ways.

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
  const auto robust = reg::quantile_fit(y, x, options);
  options.covariance = reg::QuantileCovariance::Iid;
  const auto iid = reg::quantile_fit(y, x, options);
  out.writeln("robust std_errors", robust.std_errors.items<double>());
  out.writeln("iid    std_errors", iid.std_errors.items<double>());
}
```

Output

```text
robust std_errors [1.99561, 0.29707]
iid    std_errors [1.21275, 0.16478]
```

### `Iid`

The classical estimator, τ(1 − τ) (X′X)⁻¹ / f₀², which assumes that the errors have the same distribution whatever the predictors. It is smaller and simpler, and wrong when the spread varies. At the median the two estimators coincide exactly, because τ = 1/2 makes the two sandwich weights equal and the sandwich collapses to the classical form. The program checks that at τ = 0.5.

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
  options.tau = 0.5;
  const auto robust = reg::quantile_fit(y, x, options);
  options.covariance = reg::QuantileCovariance::Iid;
  const auto iid = reg::quantile_fit(y, x, options);
  out.writeln("robust std_errors", robust.std_errors.items<double>());
  out.writeln("iid    std_errors", iid.std_errors.items<double>());
}
```

Output

```text
robust std_errors [4.02766, 0.54725]
iid    std_errors [4.02766, 0.54725]
```

### The coefficients do not depend on the choice

Only the uncertainty differs. The program prints the coefficients and the p-value of the slope under both estimators: the coefficients agree to every digit, and the p-values differ because the standard errors do.

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
  const auto robust = reg::quantile_fit(y, x, options);
  options.covariance = reg::QuantileCovariance::Iid;
  const auto iid = reg::quantile_fit(y, x, options);
  out.writeln("coefficients", robust.coefficients.items<double>(), iid.coefficients.items<double>());
  out.writeln("p value of the slope: robust", robust.p_values.items<double>()[1], " iid", iid.p_values.items<double>()[1]);
}
```

Output

```text
coefficients [0.66000, 1.62000] [0.66000, 1.62000]
p value of the slope: robust 0.00028  iid 0.00000
```

## Guidance

Use `Robust` unless there is a reason to believe in identically distributed errors; the cost is a slightly noisier estimate when they are. The choice changes only the uncertainty: the coefficients are the same under both.

## See Also

[`QuantileOptions`](QuantileOptions.md) carries the choice into `quantile_fit`, and [`QuantileFitResult`](QuantileFitResult.md) reports the standard errors and `residual_density0` that follow from it.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="QuantileOptions.html">
    <span class="label">Next</span>
    <span class="title">QuantileOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->

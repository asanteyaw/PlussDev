<!-- generated from OLSCovarianceType.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `regression::OLSCovarianceType`

Selects the estimator of the covariance of the least-squares coefficients, which fixes their standard errors, t statistics, p-values and confidence intervals.

## Signature

```cpp
regression::OLSCovarianceType regression::OLSOptions::covariance = regression::OLSCovarianceType::Nonrobust;
regression::OLSCovarianceType regression::OLSFitResult::covariance_type;
```

The enum has one member today; it exists so the choice is recorded on every result and a robust estimator can be added without changing any signature.

## Fields

### `Nonrobust`

The classical covariance estimate, mse × (X′X)⁻¹ for an unweighted fit (with case weights, X′WX in its place). It assumes that the errors have one common variance whatever the predictors. It is the default and the only choice, and it is recorded on the result as `covariance_type`. The program reads the covariance of the slope, checks that it is the mean squared error over the sum of squares of x about its mean, and prints the recorded type.

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
  const double mean = x.mean().item<double>();
  const double sxx = x.sub(mean).square().sum().item<double>();
  const std::vector<std::vector<double>> cov = fit.covariance.host<double>();
  out.writeln("variance of the slope", cov[1][1], " mse / Sxx", fit.mse / sxx);
  out.writeln("recorded type is Nonrobust:", fit.covariance_type == reg::OLSCovarianceType::Nonrobust);
}
```

Output

```text
variance of the slope 0.00068  mse / Sxx 0.00068
recorded type is Nonrobust: true
```

### With case weights

For a weighted fit the covariance is mse × (X′WX)⁻¹. The program weights the last row by three and checks the slope variance against the weighted sum of squares of x about its weighted mean.

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
  reg::OLSOptions options;
  options.weights = pluss::make_series(std::vector<double>{1, 1, 1, 1, 1, 1, 1, 3}, {8}, as_f64());
  const auto fit = reg::ols_fit(y, x, options);
  const auto w = options.weights;
  const double wsum = w.sum().item<double>();
  const double xw = x.mul(w).sum().item<double>() / wsum;
  const double sxx = x.sub(xw).square().mul(w).sum().item<double>();
  const std::vector<std::vector<double>> cov = fit.covariance.host<double>();
  out.writeln("variance of the slope", cov[1][1], " mse / weighted Sxx", fit.mse / sxx);
}
```

Output

```text
variance of the slope 0.00057  mse / weighted Sxx 0.00057
```

### Carried through a polynomial fit

A polynomial fit is a least-squares fit, so its nested result records the same estimator.

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
  out.writeln("recorded type is Nonrobust:", fit.ols.covariance_type == reg::OLSCovarianceType::Nonrobust);
  out.writeln("standard errors", fit.ols.std_errors.items<double>());
}
```

Output

```text
recorded type is Nonrobust: true
standard errors [0.09384, 0.04784, 0.00519]
```

## Guidance

Do not read `Nonrobust` standard errors as heteroskedasticity-consistent. When the spread of the residuals visibly changes with a predictor, treat the intervals as approximate, or fit a quantile regression, whose default covariance is robust.

## See Also

[`OLSOptions`](OLSOptions.md) carries the choice into `ols_fit`, [`OLSFitResult`](OLSFitResult.md) records it, and [`QuantileCovariance`](../quantile/QuantileCovariance.md) is the choice for quantile regression.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="OLSOptions.html">
    <span class="label">Next</span>
    <span class="title">OLSOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->

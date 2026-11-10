<!-- generated from OLSFitResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `regression::OLSFitResult`

Everything `ols_fit` learned: the coefficients and their uncertainty, the fitted values and residuals, the sums of squares and the measures of fit built from them, the residual diagnostics, the conditioning of the design and the labels.

## Signature

```cpp
regression::OLSFitResult regression::ols_fit(const Series &y, const Series &x, const regression::OLSOptions &options = {}, Config cfg = {});
```

A result is returned by value from `ols_fit` and is read-only data: nothing on it recomputes. Every example below fits the same eight points close to a line, so the numbers can be compared across fields, and each checks the field against a hand computation where there is one.

## Fields

### `coefficients`

`Series` of shape `[k]`.

The estimated coefficients, `[k]`, in the order of `coefficient_names`: the intercept first when there is one, then one per predictor. For one predictor they are the textbook slope Sxy/Sxx and intercept ȳ − slope × x̄; the program recomputes them.

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
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  const double sxy = x.sub(xbar).mul(y.sub(ybar)).sum().item<double>();
  out.writeln("coefficients", fit.coefficients.items<double>(), fit.coefficient_names);
  out.writeln("by hand     ", std::vector<double>{ybar - sxy / sxx * xbar, sxy / sxx});
}
```

Output

```text
coefficients [0.21786, 1.97381] [const, x1]
by hand      [0.21786, 1.97381]
```


### `std_errors`

`Series` of shape `[k]`.

The standard error of each coefficient, `[k]`: the square root of the diagonal of `covariance`. For the slope of a one-predictor fit it is √(mse / Sxx), with Sxx the sum of squares of x about its mean.

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
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  out.writeln("std_errors", fit.std_errors.items<double>());
  out.writeln("slope by hand", std::sqrt(fit.mse / sxx));
}
```

Output

```text
std_errors [0.13162, 0.02606]
slope by hand 0.02606
```


### `t_values`

`Series` of shape `[k]`.

The test statistic of each coefficient, `[k]`: the coefficient over its standard error, compared with a Student t distribution on `df_resid` degrees of freedom.

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
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  out.writeln("t_values", fit.t_values.items<double>());
  out.writeln("coefficient / std_error", fit.coefficients.div(fit.std_errors).items<double>());
}
```

Output

```text
t_values [1.65524, 75.72961]
coefficient / std_error [1.65524, 75.72961]
```


### `p_values`

`Series` of shape `[k]`.

The two-sided p-value of each t statistic against the Student t distribution on `df_resid` degrees of freedom, `[k]`. It is NaN when `df_resid` is zero.

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
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  const pluss::subsp::stats::distr::StudentT t{static_cast<double>(fit.df_resid), 0.0, 1.0};
  out.writeln("p_values", fit.p_values.items<double>());
  out.writeln("2 (1 - F(|t|))", t.cdf(fit.t_values.abs()).neg().add(1.0).mul(2.0).items<double>());
}
```

Output

```text
p_values [0.14896, 0.00000]
2 (1 - F(|t|)) [0.14896, 0.00000]
```


### `conf_int`

`Series` of shape `[k,2]`.

The confidence interval of each coefficient, `[k, 2]` with the lower bound in the first column and the upper in the second: the coefficient plus and minus the Student t quantile of 1 − alpha/2, on `df_resid` degrees, times its standard error. The program recovers the multiplier from the slope interval and checks that the two-sided tail beyond it is alpha.

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
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  const std::vector<std::vector<double>> ci = fit.conf_int.host<double>();
  const auto se = fit.std_errors.items<double>();
  const double multiplier = (ci[1][1] - ci[1][0]) / 2.0 / se[1];
  const pluss::subsp::stats::distr::StudentT t{static_cast<double>(fit.df_resid), 0.0, 1.0};
  const auto m = pluss::make_series(std::vector<double>{multiplier}, {1}, as_f64());
  out.writeln("conf_int rows", ci[0], ci[1]);
  out.writeln("multiplier", multiplier, " two-sided tail beyond it", t.cdf(m).neg().add(1.0).mul(2.0).items<double>(), " alpha", fit.alpha);
}
```

Output

```text
conf_int rows [-0.10420, 0.53991] [1.91003, 2.03759]
multiplier 2.44691  two-sided tail beyond it [0.05000]  alpha 0.05000
```


### `covariance`

`Series` of shape `[k,k]`.

The covariance matrix of the coefficients, `[k, k]`: mse × (X′X)⁻¹. It is symmetric, its diagonal is the squared standard errors, and for one predictor its slope entry is mse / Sxx. The program checks the last two.

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
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  const std::vector<std::vector<double>> cov = fit.covariance.host<double>();
  out.writeln("covariance", cov[0], cov[1]);
  out.writeln("slope variance", cov[1][1], " mse / Sxx", fit.mse / sxx, " std_errors squared", fit.std_errors.square().items<double>());
}
```

Output

```text
covariance [0.01732, -0.00306] [-0.00306, 0.00068]
slope variance 0.00068  mse / Sxx 0.00068  std_errors squared [0.01732, 0.00068]
```


### `fitted_values`

`Series` of shape `[n]`.

The fitted mean of every observation, `[n]`: the design times the coefficients. The program rebuilds it.

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
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  const auto b = fit.coefficients.items<double>();
  out.writeln("fitted_values", fit.fitted_values.items<double>());
  out.writeln("b0 + b1 x    ", x.mul(b[1]).add(b[0]).items<double>());
}
```

Output

```text
fitted_values [2.19167, 4.16548, 6.13929, 8.11310, 10.08690, 12.06071, 14.03452, 16.00833]
b0 + b1 x     [2.19167, 4.16548, 6.13929, 8.11310, 10.08690, 12.06071, 14.03452, 16.00833]
```


### `residuals`

`Series` of shape `[n]`.

The residuals, `[n]`: the observation minus its fitted value. With an intercept they sum to zero and are orthogonal to every predictor, which is what the normal equations say. The program checks both.

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
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  out.writeln("residuals", fit.residuals.items<double>());
  out.writeln("sum", fit.residuals.sum().item<double>(), " sum(x residual)", x.mul(fit.residuals).sum().item<double>());
}
```

Output

```text
residuals [0.10833, -0.06548, 0.06071, -0.21310, 0.11310, 0.03929, -0.23452, 0.19167]
sum -0.00000  sum(x residual) -0.00000
```


### `nobs`

`std::int64_t`.

The number of observations used in the fit, after dropped rows are removed.

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
  reg::OLSOptions options;
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  out.writeln("nobs", fit.nobs);
  auto values = std::vector<double>{1, 2, std::nan(""), 4, 5, 6, 7, 8};
  out.writeln("with a missing value", reg::ols_fit(y, pluss::make_series(values, {8}, as_f64()), options).nobs);
}
```

Output

```text
nobs 8
with a missing value 7
```


### `rank`

`std::int64_t`.

The numerical rank of the design, the number of coefficients that the data identify. It equals `k_params` for a well-conditioned design and falls below it when predictors are nearly or exactly collinear. The program fits a design whose second predictor is exactly twice the first.

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
  reg::OLSOptions options;
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  out.writeln("rank", fit.rank, " of", fit.k_params);
  std::vector<double> flat;
  for (int i = 0; i < 8; ++i) flat.push_back(1.0 * (i + 1));
  for (int i = 0; i < 8; ++i) flat.push_back(2.0 * (i + 1));
  const auto twice = pluss::make_series(flat, {2, 8}, as_f64()).transpose(0, 1);
  const auto deficient = reg::ols_fit(y, twice, options);
  out.writeln("exactly collinear: rank", deficient.rank, " of", deficient.k_params);
  out.writeln(deficient.warnings);
}
```

Output

```text
rank 2  of 2
exactly collinear: rank 2  of 3
[The design matrix is singular or rank deficient; coefficients were estimated using a pseudoinverse., The smallest eigenvalue is 3.788e-30. This might indicate strong multicollinearity or a singular design matrix.]
```


### `k_params`

`std::int64_t`.

The number of coefficients, including the intercept: the width of the design and the length of `coefficients`.

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
  reg::OLSOptions options;
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  out.writeln("k_params", fit.k_params, " coefficients", fit.coefficients.size(0));
  options.add_intercept = false;
  out.writeln("without intercept", reg::ols_fit(y, x, options).k_params);
}
```

Output

```text
k_params 2  coefficients 2
without intercept 1
```


### `num_predictors`

`std::int64_t`.

The number of predictor columns supplied, not counting the intercept: the width `ols_predict` expects of new rows.

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
  reg::OLSOptions options;
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  out.writeln("num_predictors", fit.num_predictors, " k_params", fit.k_params);
}
```

Output

```text
num_predictors 1  k_params 2
```


### `dropped_observations`

`std::int64_t`.

The number of rows left out of the fit: rows with a non-finite value when `drop_missing` is true, and rows of weight zero.

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
  reg::OLSOptions options;
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  auto values = std::vector<double>{1, 2, std::nan(""), 4, 5, 6, 7, 8};
  out.writeln("complete data", fit.dropped_observations);
  out.writeln("one missing value", reg::ols_fit(y, pluss::make_series(values, {8}, as_f64()), options).dropped_observations);
  options.weights = pluss::make_series(std::vector<double>{1, 1, 1, 1, 1, 1, 0, 0}, {8}, as_f64());
  out.writeln("two rows of weight zero", reg::ols_fit(y, x, options).dropped_observations);
}
```

Output

```text
complete data 0
one missing value 1
two rows of weight zero 2
```


### `df_model`

`std::int64_t`.

The degrees of freedom of the model: the rank less one when there is an intercept, the rank itself when there is none.

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
  reg::OLSOptions options;
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  out.writeln("df_model", fit.df_model, " rank", fit.rank);
}
```

Output

```text
df_model 1  rank 2
```


### `df_resid`

`std::int64_t`.

The residual degrees of freedom: the observations minus the rank. It is the degrees of freedom of the Student t distribution behind `p_values` and `conf_int` and the divisor of `mse`.

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
  reg::OLSOptions options;
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  out.writeln("df_resid", fit.df_resid, " nobs - rank", fit.nobs - fit.rank);
}
```

Output

```text
df_resid 6  nobs - rank 6
```


### `alpha`

`double`.

The significance level that the intervals in `conf_int` were built with, copied from the options.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.3, 4.1, 6.2, 7.9, 10.2, 12.1, 13.8, 16.2}, {8}, as_f64());
  reg::OLSOptions options;
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  out.writeln("alpha", fit.alpha);
  options.alpha = 0.10;
  out.writeln("with alpha 0.10", reg::ols_fit(y, x, options).alpha);
}
```

Output

```text
alpha 0.05
with alpha 0.10 0.10
```


### `mse`

`double`.

The mean squared error, `sse / df_resid`: the estimate of the error variance, and the factor that scales the covariance. It divides by the residual degrees of freedom, not by the number of observations.

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
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  out.writeln("mse", fit.mse, " sse / df_resid", fit.sse / static_cast<double>(fit.df_resid), " sse / nobs", fit.sse / static_cast<double>(fit.nobs));
}
```

Output

```text
mse 0.02853  sse / df_resid 0.02853  sse / nobs 0.02140
```


### `rmse`

`double`.

The root mean squared error, √mse: the typical size of a residual in the units of the response.

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
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  out.writeln("rmse", fit.rmse, " sqrt(mse)", std::sqrt(fit.mse));
}
```

Output

```text
rmse 0.16891  sqrt(mse) 0.16891
```


### `sse`

`double`.

The residual sum of squares, Σ (residual)², weighted by the case weights: the quantity least squares minimises.

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
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  out.writeln("sse", fit.sse, " by hand", fit.residuals.square().sum().item<double>());
}
```

Output

```text
sse 0.17119  by hand 0.17119
```


### `ssr`

`double`.

The regression sum of squares, the part of the total that the model explains: `sst − sse`.

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
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  out.writeln("ssr", fit.ssr, " sst - sse", fit.sst - fit.sse);
}
```

Output

```text
ssr 163.62881  sst - sse 163.62881
```


### `sst`

`double`.

The total sum of squares: Σ (y − ȳ)² when there is an intercept, and Σ y² when there is none, the convention that the model degrees of freedom, the adjusted R² and the F statistic follow too.

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
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  out.writeln("sst", fit.sst, " by hand", y.sub(ybar).square().sum().item<double>());
  options.add_intercept = false;
  out.writeln("without intercept", reg::ols_fit(y, x, options).sst, " sum of y squared", y.square().sum().item<double>());
}
```

Output

```text
sst 163.80000  by hand 163.80000
without intercept 826.28000  sum of y squared 826.28000
```


### `rsquared`

`double`.

The proportion of the total sum of squares that the model explains, 1 − sse / sst. It lies in [0, 1] and never falls when a predictor is added, so it cannot choose between models of different size.

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
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  out.writeln("rsquared", fit.rsquared, " 1 - sse / sst", 1.0 - fit.sse / fit.sst);
}
```

Output

```text
rsquared 0.99895  1 - sse / sst 0.99895
```


### `adj_rsquared`

`double`.

R² corrected for the number of coefficients, 1 − (1 − R²) (n − 1) / df_resid with an intercept (n rather than n − 1 without one). It falls when a predictor adds less than chance would, so it can compare models of different size.

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
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  const double n = static_cast<double>(fit.nobs);
  out.writeln("adj_rsquared", fit.adj_rsquared, " by hand", 1.0 - (1.0 - fit.rsquared) * (n - 1.0) / static_cast<double>(fit.df_resid));
}
```

Output

```text
adj_rsquared 0.99878  by hand 0.99878
```


### `f_stat`

`double`.

The F statistic of the whole regression against the constant-only model: (ssr / df_model) / mse. With one predictor it is the square of the slope's t statistic. It is NaN when `df_model` or `df_resid` is zero.

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
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  const auto t = fit.t_values.items<double>();
  out.writeln("f_stat", fit.f_stat, " (ssr / df_model) / mse", fit.ssr / static_cast<double>(fit.df_model) / fit.mse, " t of the slope squared", t[1] * t[1]);
}
```

Output

```text
f_stat 5734.97357  (ssr / df_model) / mse 5734.97357  t of the slope squared 5734.97357
```


### `f_pvalue`

`double`.

The tail probability of `f_stat` under the F distribution with `df_model` and `df_resid` degrees of freedom. With one predictor it equals the slope's p-value. The program recomputes it from the F distribution.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.3, 4.1, 6.2, 7.9, 10.2, 12.1, 13.8, 16.2}, {8}, as_f64());
  reg::OLSOptions options;
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  const auto f = pluss::make_series(std::vector<double>{fit.f_stat}, {1}, as_f64());
  const pluss::subsp::stats::distr::FDistribution dist{static_cast<double>(fit.df_model), static_cast<double>(fit.df_resid)};
  out.writeln("f_pvalue", fit.f_pvalue, " 1 - F cdf", 1.0 - dist.cdf(f).items<double>()[0], " slope p-value", fit.p_values.items<double>()[1]);
}
```

Output

```text
f_pvalue 0.00000000  1 - F cdf 0.00000000  slope p-value 0.00000000
```


### `log_likelihood`

`double`.

The maximised Gaussian log-likelihood, with the error variance estimated by sse / n: −½ n (ln(2π sse / n) + 1) for an unweighted fit. The program recomputes it.

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
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  const double n = static_cast<double>(fit.nobs);
  out.writeln("log_likelihood", fit.log_likelihood, " by hand", -0.5 * n * (std::log(2.0 * std::numbers::pi * fit.sse / n) + 1.0));
}
```

Output

```text
log_likelihood 4.02617  by hand 4.02617
```


### `aic`

`double`.

Akaike's information criterion, −2 × log-likelihood + 2 × rank. Between models fitted to the same observations the smaller value is preferred. The program recomputes it.

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
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  out.writeln("aic", fit.aic, " by hand", -2.0 * fit.log_likelihood + 2.0 * static_cast<double>(fit.rank));
}
```

Output

```text
aic -4.05234  by hand -4.05234
```


### `bic`

`double`.

The Bayesian information criterion, −2 × log-likelihood + ln(n) × rank. It penalises coefficients more heavily than the AIC once n exceeds seven. The program recomputes it.

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
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  out.writeln("bic", fit.bic, " by hand", -2.0 * fit.log_likelihood + std::log(static_cast<double>(fit.nobs)) * static_cast<double>(fit.rank));
}
```

Output

```text
bic -3.89346  by hand -3.89346
```


### `durbin_watson`

`double`.

The Durbin-Watson statistic, Σ (rₜ − rₜ₋₁)² / sse, which looks for correlation between neighbouring residuals. It lies between 0 and 4: near 2 means none, near 0 positive correlation, near 4 negative. It is meaningful only when the rows are in a natural order, such as time. The program recomputes it.

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
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  const auto r = fit.residuals.items<double>();
  double numerator = 0.0;
  for (std::size_t i = 1; i < r.size(); ++i) numerator += (r[i] - r[i - 1]) * (r[i] - r[i - 1]);
  out.writeln("durbin_watson", fit.durbin_watson, " by hand", numerator / fit.sse);
}
```

Output

```text
durbin_watson 2.85976  by hand 2.85976
```


### `omnibus`

`double`.

The D'Agostino-Pearson omnibus statistic for normality of the residuals: the sum of the squared normalised skewness and kurtosis, compared with a chi-squared distribution on two degrees of freedom. It needs at least eight observations; below that it is NaN. A large value, with its p-value, says the residuals are not normal.

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
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  out.writeln("omnibus", fit.omnibus, " nobs", fit.nobs);
  const auto two = reg::ols_fit(y.slice(0, 0, 5), x.slice(0, 0, 5), options);
  out.writeln("with five observations", two.omnibus);
}
```

Output

```text
omnibus 1.16028  nobs 8
with five observations nan
```


### `omnibus_pvalue`

`double`.

The tail probability of `omnibus` under the chi-squared distribution on two degrees of freedom. A small value is evidence against normal residuals. The program recomputes it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.3, 4.1, 6.2, 7.9, 10.2, 12.1, 13.8, 16.2}, {8}, as_f64());
  reg::OLSOptions options;
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  const pluss::subsp::stats::distr::ChiSquare chi(2);
  const auto o = pluss::make_series(std::vector<double>{fit.omnibus}, {1}, as_f64());
  out.writeln("omnibus_pvalue", fit.omnibus_pvalue, " 1 - chi2 cdf", 1.0 - chi.cdf(o).items<double>()[0]);
}
```

Output

```text
omnibus_pvalue 0.55982071  1 - chi2 cdf 0.55982071
```


### `jarque_bera`

`double`.

The Jarque-Bera statistic for normality of the residuals, n (S² + (K − 3)² / 4) / 6, from their skewness S and kurtosis K. It is zero for residuals with the skewness and kurtosis of a normal distribution. The program recomputes it.

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
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  const double n = static_cast<double>(fit.nobs);
  out.writeln("jarque_bera", fit.jarque_bera, " by hand", n * (fit.skewness * fit.skewness + 0.25 * (fit.kurtosis - 3.0) * (fit.kurtosis - 3.0)) / 6.0);
}
```

Output

```text
jarque_bera 0.79085  by hand 0.79085
```


### `jarque_bera_pvalue`

`double`.

The tail probability of `jarque_bera` under the chi-squared distribution on two degrees of freedom. The program recomputes it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.3, 4.1, 6.2, 7.9, 10.2, 12.1, 13.8, 16.2}, {8}, as_f64());
  reg::OLSOptions options;
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  const pluss::subsp::stats::distr::ChiSquare chi(2);
  const auto jb = pluss::make_series(std::vector<double>{fit.jarque_bera}, {1}, as_f64());
  out.writeln("jarque_bera_pvalue", fit.jarque_bera_pvalue, " 1 - chi2 cdf", 1.0 - chi.cdf(jb).items<double>()[0]);
}
```

Output

```text
jarque_bera_pvalue 0.67339226  1 - chi2 cdf 0.67339226
```


### `skewness`

`double`.

The skewness of the residuals, the third central moment over the second to the power 3/2. It is zero for a symmetric distribution; positive means a longer right tail. The program recomputes it from the residuals.

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
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  const auto r = fit.residuals.items<double>();
  double mean = 0.0;
  for (const double v : r) mean += v / static_cast<double>(r.size());
  double m2 = 0.0, m3 = 0.0;
  for (const double v : r) { m2 += (v - mean) * (v - mean) / static_cast<double>(r.size()); m3 += std::pow(v - mean, 3) / static_cast<double>(r.size()); }
  out.writeln("skewness", fit.skewness, " by hand", m3 / std::pow(m2, 1.5));
}
```

Output

```text
skewness -0.51165  by hand -0.51165
```


### `kurtosis`

`double`.

The kurtosis of the residuals, the fourth central moment over the square of the second. It is 3 for a normal distribution; larger means heavier tails. The program recomputes it from the residuals.

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
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  const auto r = fit.residuals.items<double>();
  double mean = 0.0;
  for (const double v : r) mean += v / static_cast<double>(r.size());
  double m2 = 0.0, m4 = 0.0;
  for (const double v : r) { m2 += (v - mean) * (v - mean) / static_cast<double>(r.size()); m4 += std::pow(v - mean, 4) / static_cast<double>(r.size()); }
  out.writeln("kurtosis", fit.kurtosis, " by hand", m4 / (m2 * m2));
}
```

Output

```text
kurtosis 1.84873  by hand 1.84873
```


### `condition_number`

`double`.

The ratio of the largest to the smallest singular value of the weighted design, a measure of how close the predictors are to collinear. Above 1000 the fit attaches a warning. Moving the predictor far from zero makes it collinear with the intercept column; the program shifts x by 1000 and the number rises by orders of magnitude.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.3, 4.1, 6.2, 7.9, 10.2, 12.1, 13.8, 16.2}, {8}, as_f64());
  reg::OLSOptions options;
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  out.writeln("condition_number", fit.condition_number);
  const auto far = reg::ols_fit(y, x.add(1000.0), options);
  out.writeln("x shifted by 1000", far.condition_number);
  out.writeln(far.warnings);
}
```

Output

```text
condition_number 11.478
x shifted by 1000 440375.268
[The condition number is large, 4.404e+05. This might indicate strong multicollinearity or other numerical problems.]
```


### `smallest_eig`

`double`.

The smallest eigenvalue of X′X (the square of the smallest singular value of the design). A value near zero means a direction of the design is almost absent; below 10⁻¹⁰ the fit attaches a multicollinearity warning. The program fits the exactly collinear design.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.3, 4.1, 6.2, 7.9, 10.2, 12.1, 13.8, 16.2}, {8}, as_f64());
  reg::OLSOptions options;
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  out.writeln("smallest_eig", fit.smallest_eig);
  std::vector<double> flat;
  for (int i = 0; i < 8; ++i) flat.push_back(1.0 * (i + 1));
  for (int i = 0; i < 8; ++i) flat.push_back(2.0 * (i + 1));
  const auto twice = pluss::make_series(flat, {2, 8}, as_f64()).transpose(0, 1);
  const auto deficient = reg::ols_fit(y, twice, options);
  out.writeln("exactly collinear", deficient.smallest_eig);
  out.writeln(deficient.warnings);
}
```

Output

```text
smallest_eig 1.597
exactly collinear 0.000
[The design matrix is singular or rank deficient; coefficients were estimated using a pseudoinverse., The smallest eigenvalue is 3.788e-30. This might indicate strong multicollinearity or a singular design matrix.]
```


### `has_intercept`

`bool`.

Whether the design included a constant column, copied from `add_intercept`. It decides which total sum of squares R² and the F test use.

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
  reg::OLSOptions options;
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  out.writeln("has_intercept", fit.has_intercept);
  options.add_intercept = false;
  out.writeln("add_intercept=false", reg::ols_fit(y, x, options).has_intercept);
}
```

Output

```text
has_intercept true
add_intercept=false false
```


### `covariance_type`

`regression::OLSCovarianceType`.

The covariance estimator the standard errors came from; see [`OLSCovarianceType`](OLSCovarianceType.md).

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
  reg::OLSOptions options;
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  out.writeln("Nonrobust:", fit.covariance_type == reg::OLSCovarianceType::Nonrobust);
}
```

Output

```text
Nonrobust: true
```


### `response_name`

`std::string`.

The label of the response, copied from the options and printed by the summary.

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
  reg::OLSOptions options;
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  out.writeln("default", fit.response_name);
  options.response_name = "load";
  out.writeln("named", reg::ols_fit(y, x, options).response_name);
}
```

Output

```text
default y
named load
```


### `coefficient_names`

`std::vector<std::string>`.

The label of each coefficient, in the order of `coefficients`: `const` for the intercept, then the names from `predictor_names` or `x1`, `x2` and so on.

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
  reg::OLSOptions options;
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  out.writeln(fit.coefficient_names);
  options.predictor_names = {"hours"};
  out.writeln(reg::ols_fit(y, x, options).coefficient_names);
}
```

Output

```text
[const, x1]
[const, hours]
```


### `warnings`

`std::vector<std::string>`.

Notes that the fit attaches when something needs attention, as text: a rank-deficient design solved with a pseudoinverse, a tiny smallest eigenvalue or a condition number above 1000, and a model without an intercept. The list is empty for a clean fit.

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
  reg::OLSOptions options;
  const auto fit = reg::ols_fit(y, x, options);
  const double xbar = x.mean().item<double>();
  const double ybar = y.mean().item<double>();
  const double sxx = x.sub(xbar).square().sum().item<double>();
  out.writeln("clean fit:", fit.warnings.size(), "warnings");
  options.add_intercept = false;
  out.writeln(reg::ols_fit(y, x, options).warnings);
}
```

Output

```text
clean fit: 0 warnings
[Model has no explicit intercept; R-squared and F-statistic interpretations differ from intercept models.]
```


## See Also

[`ols_fit`](ols_fit.md) produces the result, [`ols_predict`](ols_predict.md) applies its coefficients to new rows, [`ols_summary`](ols_summary.md) prints it as a report, and [`OLSOptions`](OLSOptions.md) holds the settings it records.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="OLSOptions.html">
    <span class="label">Previous</span>
    <span class="title">OLSOptions</span>
  </a>
  <a class="next-link" href="ols_fit.html">
    <span class="label">Next</span>
    <span class="title">ols_fit</span>
  </a>
</div>
<!-- apiref:nav:end -->

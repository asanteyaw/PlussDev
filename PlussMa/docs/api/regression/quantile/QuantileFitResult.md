<!-- generated from QuantileFitResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `regression::QuantileFitResult`

Everything `quantile_fit` learned: the coefficients and their uncertainty, the fitted quantiles and residuals, the check-loss measures of fit, the residual density, the iteration outcome and the labels.

## Signature

```cpp
regression::QuantileFitResult regression::quantile_fit(const Series &y, const Series &x, const regression::QuantileOptions &options = {}, Config cfg = {});
```

A result is returned by value from `quantile_fit` and is read-only data. Every example below fits the median of the same twelve points, whose spread grows with x, so the numbers can be compared across fields; where a field needs another setting the program says so.

## Fields

### `coefficients`

`Series` of shape `[k]`.

The estimated coefficients, `[k]`, in the order of `coefficient_names`: the intercept first when there is one, then one per predictor. They minimise the check loss, so the fitted line leaves a fraction tau of the observations below it. At the median, half the residuals are negative, half positive, and the observations that lie exactly on the line (at least as many as there are coefficients) are neither. The program counts them.

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
  const auto fit = reg::quantile_fit(y, x, options);
  const auto r = fit.residuals.items<double>();
  int below = 0, on = 0, above = 0;
  for (const double v : r) (v < -1e-6 ? below : v > 1e-6 ? above : on)++;
  out.writeln("coefficients", fit.coefficients.items<double>(), fit.coefficient_names);
  out.writeln("residuals below the line", below, " on the line", on, " above the line", above);
}
```

Output

```text
coefficients [0.58000, 1.52000] [const, x1]
residuals below the line 5  on the line 2  above the line 5
```


### `std_errors`

`Series` of shape `[k]`.

The standard error of each coefficient, `[k]`: the square root of the diagonal of `covariance`, whose estimator is set by `QuantileOptions::covariance`. The program computes them from the covariance matrix.

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
  const auto fit = reg::quantile_fit(y, x, options);
  const std::vector<std::vector<double>> cov = fit.covariance.host<double>();
  out.writeln("std_errors", fit.std_errors.items<double>());
  out.writeln("from covariance", std::sqrt(cov[0][0]), std::sqrt(cov[1][1]));
}
```

Output

```text
std_errors [4.02766, 0.54725]
from covariance 4.02766 0.54725
```


### `t_values`

`Series` of shape `[k]`.

The test statistic of each coefficient, `[k]`: the coefficient over its standard error, compared with a Student t distribution on `df_resid` degrees of freedom. The program recomputes them.

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
  const auto fit = reg::quantile_fit(y, x, options);
  out.writeln("t_values", fit.t_values.items<double>());
  out.writeln("coefficient / std_error", fit.coefficients.div(fit.std_errors).items<double>());
}
```

Output

```text
t_values [0.14400, 2.77751]
coefficient / std_error [0.14400, 2.77751]
```


### `p_values`

`Series` of shape `[k]`.

The two-sided p-value of each t statistic against the Student t distribution on `df_resid` degrees of freedom, `[k]`. It is NaN when `df_resid` is zero. The program recomputes them.

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
  const auto fit = reg::quantile_fit(y, x, options);
  const pluss::subsp::stats::distr::StudentT t{static_cast<double>(fit.df_resid), 0.0, 1.0};
  out.writeln("p_values", fit.p_values.items<double>());
  out.writeln("2 (1 - F(|t|))", t.cdf(fit.t_values.abs()).neg().add(1.0).mul(2.0).items<double>());
}
```

Output

```text
p_values [0.88836, 0.01953]
2 (1 - F(|t|)) [0.88836, 0.01953]
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
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  reg::QuantileOptions options;
  const auto fit = reg::quantile_fit(y, x, options);
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
conf_int rows [-8.39420, 9.55419] [0.30065, 2.73935]
multiplier 2.22814  two-sided tail beyond it [0.05000]  alpha 0.05000
```


### `covariance`

`Series` of shape `[k,k]`.

The covariance matrix of the coefficients, `[k, k]`. It is symmetric to rounding, its diagonal is the squared standard errors, and its off-diagonal entry is what an interval on a combination of coefficients needs.

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
  const auto fit = reg::quantile_fit(y, x, options);
  const std::vector<std::vector<double>> cov = fit.covariance.host<double>();
  out.writeln("covariance", cov[0], cov[1]);
  out.writeln("off-diagonals differ by", std::abs(cov[0][1] - cov[1][0]), " diagonal is std_errors squared", fit.std_errors.square().items<double>());
}
```

Output

```text
covariance [16.22208, -1.94665] [-1.94665, 0.29948]
off-diagonals differ by 0.00000  diagonal is std_errors squared [16.22208, 0.29948]
```


### `fitted_values`

`Series` of shape `[n]`.

The fitted conditional quantile of every observation, `[n]`: the design times the coefficients. The program rebuilds it.

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
  const auto fit = reg::quantile_fit(y, x, options);
  const auto b = fit.coefficients.items<double>();
  out.writeln("fitted_values", fit.fitted_values.items<double>());
  out.writeln("b0 + b1 x    ", x.mul(b[1]).add(b[0]).items<double>());
}
```

Output

```text
fitted_values [2.10000, 3.62000, 5.14000, 6.66000, 8.18000, 9.70000, 11.22000, 12.74000, 14.26000, 15.78000, 17.30000, 18.82000]
b0 + b1 x     [2.10000, 3.62000, 5.14000, 6.66000, 8.18000, 9.70000, 11.22000, 12.74000, 14.26000, 15.78000, 17.30000, 18.82000]
```


### `residuals`

`Series` of shape `[n]`.

The residuals, `[n]`: the observation minus its fitted quantile. They are not squared: the fit minimises the check loss of exactly these values. The program recomputes them.

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
  const auto fit = reg::quantile_fit(y, x, options);
  out.writeln("residuals ", fit.residuals.items<double>());
  out.writeln("y - fitted", y.sub(fit.fitted_values).items<double>());
}
```

Output

```text
residuals  [0.00000, 0.28000, -1.94000, 0.14000, -3.08000, -0.00000, -4.82000, 0.16000, -6.56000, 0.72000, -9.10000, 1.28000]
y - fitted [0.00000, 0.28000, -1.94000, 0.14000, -3.08000, -0.00000, -4.82000, 0.16000, -6.56000, 0.72000, -9.10000, 1.28000]
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
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  reg::QuantileOptions options;
  const auto fit = reg::quantile_fit(y, x, options);
  out.writeln("nobs", fit.nobs);
  auto values = std::vector<double>{1, 2, std::nan(""), 4, 5, 6, 7, 8, 9, 10, 11, 12};
  out.writeln("with a missing value", reg::quantile_fit(y, pluss::make_series(values, {12}, as_f64()), options).nobs);
}
```

Output

```text
nobs 12
with a missing value 11
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
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  reg::QuantileOptions options;
  const auto fit = reg::quantile_fit(y, x, options);
  out.writeln("rank", fit.rank, " of", fit.k_params);
  std::vector<double> flat;
  for (int i = 0; i < 12; ++i) flat.push_back(1.0 * (i + 1));
  for (int i = 0; i < 12; ++i) flat.push_back(2.0 * (i + 1));
  const auto twice = pluss::make_series(flat, {2, 12}, as_f64()).transpose(0, 1);
  const auto deficient = reg::quantile_fit(y, twice, options);
  out.writeln("exactly collinear: rank", deficient.rank, " of", deficient.k_params);
  out.writeln(deficient.warnings);
}
```

Output

```text
rank 2  of 2
exactly collinear: rank 2  of 3
[The design matrix is singular or rank deficient; coefficients were estimated using a pseudoinverse.]
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
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  reg::QuantileOptions options;
  const auto fit = reg::quantile_fit(y, x, options);
  out.writeln("k_params", fit.k_params, " coefficients", fit.coefficients.size(0));
  options.add_intercept = false;
  out.writeln("without intercept", reg::quantile_fit(y, x, options).k_params);
}
```

Output

```text
k_params 2  coefficients 2
without intercept 1
```


### `num_predictors`

`std::int64_t`.

The number of predictor columns supplied, not counting the intercept: the width `quantile_predict` expects of new rows.

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
  reg::QuantileOptions options;
  const auto fit = reg::quantile_fit(y, x, options);
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
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  reg::QuantileOptions options;
  const auto fit = reg::quantile_fit(y, x, options);
  auto values = std::vector<double>{1, 2, std::nan(""), 4, 5, 6, 7, 8, 9, 10, 11, 12};
  out.writeln("complete data", fit.dropped_observations);
  out.writeln("one missing value", reg::quantile_fit(y, pluss::make_series(values, {12}, as_f64()), options).dropped_observations);
  options.weights = pluss::make_series(std::vector<double>{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0}, {12}, as_f64());
  out.writeln("two rows of weight zero", reg::quantile_fit(y, x, options).dropped_observations);
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
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  reg::QuantileOptions options;
  const auto fit = reg::quantile_fit(y, x, options);
  out.writeln("df_model", fit.df_model, " rank", fit.rank);
}
```

Output

```text
df_model 1  rank 2
```


### `df_resid`

`std::int64_t`.

The residual degrees of freedom: the observations minus the rank. It is the degrees of freedom of the Student t distribution behind `p_values` and `conf_int`.

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
  reg::QuantileOptions options;
  const auto fit = reg::quantile_fit(y, x, options);
  out.writeln("df_resid", fit.df_resid, " nobs - rank", fit.nobs - fit.rank);
}
```

Output

```text
df_resid 10  nobs - rank 10
```


### `iterations`

`std::int64_t`.

The number of reweighted least-squares steps taken. Unlike a least-squares fit, a quantile fit is iterative; the count shows how hard the data made it.

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
  reg::QuantileOptions options;
  const auto fit = reg::quantile_fit(y, x, options);
  out.writeln("iterations", fit.iterations, " converged", fit.converged);
}
```

Output

```text
iterations 21  converged true
```


### `tau`

`double`.

The quantile that was fitted, copied from the options.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  reg::QuantileOptions options;
  const auto fit = reg::quantile_fit(y, x, options);
  out.writeln("tau", fit.tau);
  options.tau = 0.9;
  out.writeln("with tau 0.9", reg::quantile_fit(y, x, options).tau);
}
```

Output

```text
tau 0.50
with tau 0.9 0.90
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
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  reg::QuantileOptions options;
  const auto fit = reg::quantile_fit(y, x, options);
  out.writeln("alpha", fit.alpha);
  options.alpha = 0.10;
  out.writeln("with alpha 0.10", reg::quantile_fit(y, x, options).alpha);
}
```

Output

```text
alpha 0.05
with alpha 0.10 0.10
```


### `objective`

`double`.

The minimised check loss, Σ ρ(residual), where ρ charges tau per unit of a positive residual and 1 − tau per unit of a negative one (weighted by the case weights). It is a sum, not a mean. The program recomputes it for the median, where ρ is half the absolute residual.

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
  const auto fit = reg::quantile_fit(y, x, options);
  double by_hand = 0.0;
  for (const double r : fit.residuals.items<double>()) by_hand += r >= 0 ? options.tau * r : (options.tau - 1.0) * r;
  out.writeln("objective", fit.objective, " by hand", by_hand);
}
```

Output

```text
objective 14.04000  by hand 14.04000
```


### `objective_null`

`double`.

The check loss of the model with only a constant, which is the sample tau-quantile of the response. It is the benchmark that `objective` is compared with. At the median the constant is the middle of the sorted response; the program recomputes the loss with it.

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
  const auto fit = reg::quantile_fit(y, x, options);
  auto sorted = y.items<double>();
  std::sort(sorted.begin(), sorted.end());
  const double median = 0.5 * (sorted[5] + sorted[6]);
  double by_hand = 0.0;
  for (const double v : sorted) by_hand += 0.5 * std::abs(v - median);
  out.writeln("objective_null", fit.objective_null, " by hand", by_hand, " median", median);
}
```

Output

```text
objective_null 23.80000  by hand 23.80000  median 7.25000
```


### `pseudo_rsquared`

`double`.

The proportion of the null loss that the model removes, 1 − objective / objective_null. It is zero for a model no better than a constant and one for an exact fit; it is a measure of the same kind as an R² but of the check loss, so it varies with tau. The program recomputes it and prints it at three quantiles.

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
  const auto fit = reg::quantile_fit(y, x, options);
  out.writeln("pseudo_rsquared", fit.pseudo_rsquared, " by hand", 1.0 - fit.objective / fit.objective_null);
  for (const double tau : {0.1, 0.9}) {
    options.tau = tau;
    out.writeln("tau", tau, " pseudo_rsquared", reg::quantile_fit(y, x, options).pseudo_rsquared);
  }
}
```

Output

```text
pseudo_rsquared 0.41008  by hand 0.41008
tau 0.10000  pseudo_rsquared 0.49908
tau 0.90000  pseudo_rsquared 0.76408
```


### `residual_density0`

`double`.

The estimated density of the residuals at zero, f₀: how tightly the errors cluster around the fitted quantile. It scales the standard errors, which are proportional to 1/f₀. With the classical estimator and a single predictor, the slope has standard error √(tau(1 − tau)) / (f₀ √Sxx) with Sxx the sum of squares of x about its mean; the program checks that.

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
  const auto fit = reg::quantile_fit(y, x, options);
  options.covariance = reg::QuantileCovariance::Iid;
  const auto iid = reg::quantile_fit(y, x, options);
  const double mean = x.mean().item<double>();
  const double sxx = x.sub(mean).square().sum().item<double>();
  out.writeln("residual_density0", iid.residual_density0);
  out.writeln("slope std_error", iid.std_errors.items<double>()[1], " by hand", std::sqrt(0.5 * 0.5) / (iid.residual_density0 * std::sqrt(sxx)));
}
```

Output

```text
residual_density0 0.07640
slope std_error 0.54725  by hand 0.54725
```


### `converged`

`bool`.

Whether the iteration settled to `tol` before `max_iter`. The program forces a failure with three iterations.

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
  reg::QuantileOptions options;
  const auto fit = reg::quantile_fit(y, x, options);
  out.writeln("converged", fit.converged);
  options.max_iter = 3;
  out.writeln("with max_iter 3", reg::quantile_fit(y, x, options).converged);
}
```

Output

```text
converged true
with max_iter 3 false
```


### `has_intercept`

`bool`.

Whether the design included a constant column, copied from `add_intercept`.

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
  reg::QuantileOptions options;
  const auto fit = reg::quantile_fit(y, x, options);
  out.writeln("has_intercept", fit.has_intercept);
  options.add_intercept = false;
  out.writeln("add_intercept=false", reg::quantile_fit(y, x, options).has_intercept);
}
```

Output

```text
has_intercept true
add_intercept=false false
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
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  reg::QuantileOptions options;
  const auto fit = reg::quantile_fit(y, x, options);
  out.writeln("default", fit.response_name);
  options.response_name = "price";
  out.writeln("named", reg::quantile_fit(y, x, options).response_name);
}
```

Output

```text
default y
named price
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
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  reg::QuantileOptions options;
  const auto fit = reg::quantile_fit(y, x, options);
  out.writeln(fit.coefficient_names);
  options.predictor_names = {"size"};
  out.writeln(reg::quantile_fit(y, x, options).coefficient_names);
}
```

Output

```text
[const, x1]
[const, size]
```


### `warnings`

`std::vector<std::string>`.

Notes that the fit attaches when something needs attention, as text: an iteration that did not converge, or a rank-deficient design solved with a pseudoinverse. The list is empty for a clean fit.

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
  reg::QuantileOptions options;
  const auto fit = reg::quantile_fit(y, x, options);
  out.writeln("clean fit:", fit.warnings.size(), "warnings");
  options.max_iter = 3;
  out.writeln(reg::quantile_fit(y, x, options).warnings);
}
```

Output

```text
clean fit: 0 warnings
[Quantile IRLS reached max_iter without satisfying tolerance.]
```


## See Also

[`quantile_fit`](quantile_fit.md) produces the result, [`quantile_predict`](quantile_predict.md) applies its coefficients to new rows, [`quantile_summary`](quantile_summary.md) prints it as a report, and [`QuantileOptions`](QuantileOptions.md) holds the settings it records.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="QuantileOptions.html">
    <span class="label">Previous</span>
    <span class="title">QuantileOptions</span>
  </a>
  <a class="next-link" href="quantile_fit.html">
    <span class="label">Next</span>
    <span class="title">quantile_fit</span>
  </a>
</div>
<!-- apiref:nav:end -->

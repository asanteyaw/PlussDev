<!-- generated from GLMFitResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `regression::GLMFitResult`

Everything `glm_fit` learned: the coefficients and their uncertainty, the fitted values and residuals, the deviance-based measures of fit, the iteration outcome and the labels.

## Signature

```cpp
regression::GLMFitResult regression::glm_fit(const Series &y, const Series &x, const regression::GLMOptions &options = {}, Config cfg = {});
```

A result is returned by value from `glm_fit` and is read-only data: nothing on it recomputes. Every example below fits the same ten counts with a Poisson family, so the numbers can be compared across fields; where a field needs another setting the program says so.

## Fields

### `coefficients`

`Series` of shape `[k]`.

The estimated coefficients on the link scale, `[k]`, in the order of `coefficient_names`: the intercept first when there is one, then one per predictor. They maximise the likelihood, so for a canonical link the score equations hold: the residuals sum to zero and are orthogonal to every predictor. The program prints them and checks both.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  out.writeln("coefficients", fit.coefficients.items<double>(), fit.coefficient_names);
  out.writeln("sum(y - mu)", y.sub(fit.fitted_values).sum().item<double>(), " sum(x (y - mu))", x.mul(y.sub(fit.fitted_values)).sum().item<double>());
}
```

Output

```text
coefficients [-0.54808, 0.63117] [const, x1]
sum(y - mu) -0.00000  sum(x (y - mu)) -0.00000
```


### `std_errors`

`Series` of shape `[k]`.

The standard error of each coefficient, `[k]`: the square root of the diagonal of `covariance`. The program computes them from the covariance matrix.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  const std::vector<std::vector<double>> cov = fit.covariance.host<double>();
  out.writeln("std_errors", fit.std_errors.items<double>());
  out.writeln("from covariance", std::sqrt(cov[0][0]), std::sqrt(cov[1][1]));
}
```

Output

```text
std_errors [0.50644, 0.12527]
from covariance 0.50644 0.12527
```


### `z_values`

`Series` of shape `[k]`.

The Wald statistic of each coefficient, `[k]`: the coefficient over its standard error, to be compared with a standard normal. The program recomputes them.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  out.writeln("z_values", fit.z_values.items<double>());
  out.writeln("coefficient / std_error", fit.coefficients.div(fit.std_errors).items<double>());
}
```

Output

```text
z_values [-1.08222, 5.03832]
coefficient / std_error [-1.08222, 5.03832]
```


### `p_values`

`Series` of shape `[k]`.

The two-sided p-value of each Wald statistic against the standard normal distribution, `[k]`. A small value says the coefficient is unlikely to be zero given the standard error. The program recomputes them from the normal distribution.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  const pluss::subsp::stats::distr::Normal standard(0.0, 1.0);
  out.writeln("p_values", fit.p_values.items<double>());
  out.writeln("2 (1 - Phi(|z|))", standard.cdf(fit.z_values.abs()).neg().add(1.0).mul(2.0).items<double>());
}
```

Output

```text
p_values [0.27915, 0.00000]
2 (1 - Phi(|z|)) [0.27915, 0.00000]
```


### `conf_int`

`Series` of shape `[k,2]`.

The confidence interval of each coefficient, `[k, 2]` with the lower bound in the first column and the upper in the second: the coefficient plus and minus the normal quantile of 1 − alpha/2 times its standard error. The program rebuilds the slope interval at the 95% level, where that quantile is 1.95996.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  const std::vector<std::vector<double>> ci = fit.conf_int.host<double>();
  const auto b = fit.coefficients.items<double>();
  const auto se = fit.std_errors.items<double>();
  out.writeln("conf_int rows", ci[0], ci[1]);
  out.writeln("slope +- 1.959964 se", b[1] - 1.959964 * se[1], b[1] + 1.959964 * se[1]);
}
```

Output

```text
conf_int rows [-1.54068, 0.44452] [0.38564, 0.87670]
slope +- 1.959964 se 0.38564 0.87670
```


### `covariance`

`Series` of shape `[k,k]`.

The covariance matrix of the coefficients, `[k, k]`: the inverse of the weighted normal-equation matrix times the dispersion. It is symmetric, its diagonal is the squared standard errors, and its off-diagonal entries are what you need to put an interval on any combination of coefficients.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  const std::vector<std::vector<double>> cov = fit.covariance.host<double>();
  out.writeln("covariance", cov[0], cov[1]);
  out.writeln("symmetric", cov[0][1] == cov[1][0], " diagonal is std_errors squared", fit.std_errors.square().items<double>());
}
```

Output

```text
covariance [0.25648, -0.06081] [-0.06081, 0.01569]
symmetric true  diagonal is std_errors squared [0.25648, 0.01569]
```


### `fitted_values`

`Series` of shape `[n]`.

The fitted mean of every observation, `[n]`, on the scale of the response: the inverse link of the linear predictor. For this log link that is the exponential of `linear_predictor`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  out.writeln("fitted_values", fit.fitted_values.items<double>());
  out.writeln("exp(linear_predictor)", fit.linear_predictor.exp().items<double>());
}
```

Output

```text
fitted_values [0.79255, 1.08664, 1.48985, 2.04267, 2.80062, 3.83982, 5.26462, 7.21811, 9.89647, 13.56865]
exp(linear_predictor) [0.79255, 1.08664, 1.48985, 2.04267, 2.80062, 3.83982, 5.26462, 7.21811, 9.89647, 13.56865]
```


### `linear_predictor`

`Series` of shape `[n]`.

The linear predictor of every observation, `[n]`, on the scale of the link: the design times the coefficients plus any offset. The program rebuilds it from the coefficients.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  const auto b = fit.coefficients.items<double>();
  out.writeln("linear_predictor", fit.linear_predictor.items<double>());
  out.writeln("b0 + b1 x       ", x.mul(b[1]).add(b[0]).items<double>());
}
```

Output

```text
linear_predictor [-0.23249, 0.08309, 0.39867, 0.71426, 1.02984, 1.34543, 1.66101, 1.97659, 2.29218, 2.60776]
b0 + b1 x        [-0.23249, 0.08309, 0.39867, 0.71426, 1.02984, 1.34543, 1.66101, 1.97659, 2.29218, 2.60776]
```


### `residuals`

`Series` of shape `[n]`.

The response residuals, `[n]`: the observation minus its fitted mean, y − mu, on the scale of the response. For a canonical link they sum to zero. They are not variance-stabilised, so for an unusual-observation check divide by the square root of the variance function, as the program does for the Poisson family.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  out.writeln("residuals", fit.residuals.items<double>());
  out.writeln("y - fitted", y.sub(fit.fitted_values).items<double>());
  out.writeln("Pearson residuals", fit.residuals.div(fit.fitted_values.sqrt()).items<double>());
}
```

Output

```text
residuals [0.20745, -1.08664, 0.51015, 0.95733, -0.80062, 1.16018, -1.26462, 0.78189, -0.89647, 0.43135]
y - fitted [0.20745, -1.08664, 0.51015, 0.95733, -0.80062, 1.16018, -1.26462, 0.78189, -0.89647, 0.43135]
Pearson residuals [0.23302, -1.04242, 0.41795, 0.66983, -0.47841, 0.59207, -0.55116, 0.29103, -0.28497, 0.11710]
```


### `nobs`

`std::int64_t`.

The number of observations used in the fit, after dropped rows are removed. The program fits with one missing predictor and prints the count.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  out.writeln("nobs", fit.nobs);
  auto values = std::vector<double>{0.5, 1.0, std::nan(""), 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0};
  const auto holey = reg::glm_fit(y, pluss::make_series(values, {10}, as_f64()), options);
  out.writeln("with a missing value", holey.nobs);
}
```

Output

```text
nobs 10
with a missing value 9
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
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  out.writeln("rank", fit.rank, " of", fit.k_params);
  std::vector<double> flat;
  for (int i = 0; i < 10; ++i) flat.push_back(0.5 * (i + 1));
  for (int i = 0; i < 10; ++i) flat.push_back(1.0 * (i + 1));
  const auto twice = pluss::make_series(flat, {2, 10}, as_f64()).transpose(0, 1);
  const auto deficient = reg::glm_fit(y, twice, options);
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

The number of coefficients, including the intercept: the width of the design. It is the length of `coefficients` and `coefficient_names`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  out.writeln("k_params", fit.k_params, " coefficients", fit.coefficients.size(0));
  options.add_intercept = false;
  out.writeln("without intercept", reg::glm_fit(y, x, options).k_params);
}
```

Output

```text
k_params 2  coefficients 2
without intercept 1
```


### `num_predictors`

`std::int64_t`.

The number of predictor columns supplied, not counting the intercept. It is the width `glm_predict` expects of new rows.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  out.writeln("num_predictors", fit.num_predictors, " k_params", fit.k_params);
}
```

Output

```text
num_predictors 1  k_params 2
```


### `dropped_observations`

`std::int64_t`.

The number of rows left out of the fit: rows with a non-finite value when `drop_missing` is true, and rows of weight zero. The program counts both kinds.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  auto values = std::vector<double>{0.5, 1.0, std::nan(""), 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0};
  const auto holey = pluss::make_series(values, {10}, as_f64());
  out.writeln("complete data", fit.dropped_observations);
  out.writeln("one missing value", reg::glm_fit(y, holey, options).dropped_observations);
  options.weights = pluss::make_series(std::vector<double>{1, 1, 1, 1, 1, 1, 1, 0, 0, 1}, {10}, as_f64());
  out.writeln("two rows of weight zero", reg::glm_fit(y, x, options).dropped_observations);
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

The degrees of freedom of the model: the rank less one when there is an intercept, the rank itself when there is none. It counts the coefficients beyond the constant.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  out.writeln("df_model", fit.df_model, " rank", fit.rank);
}
```

Output

```text
df_model 1  rank 2
```


### `df_resid`

`std::int64_t`.

The residual degrees of freedom: the observations minus the rank. For the Normal family the dispersion is the deviance divided by this number.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  out.writeln("df_resid", fit.df_resid, " nobs - rank", fit.nobs - fit.rank);
}
```

Output

```text
df_resid 8  nobs - rank 8
```


### `iterations`

`std::int64_t`.

The number of iteratively reweighted least-squares steps taken. A canonical link usually converges in a handful; reaching `max_iter` means the fit did not converge.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  out.writeln("iterations", fit.iterations, " converged", fit.converged);
}
```

Output

```text
iterations 6  converged true
```


### `alpha`

`double`.

The significance level that the intervals in `conf_int` were built with, echoed from the options.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  out.writeln("alpha", fit.alpha);
  options.alpha = 0.10;
  out.writeln("with alpha 0.10", reg::glm_fit(y, x, options).alpha);
}
```

Output

```text
alpha 0.05
with alpha 0.10 0.10
```


### `dispersion`

`double`.

The dispersion parameter that scales the covariance. It is fixed at one for the Binomial and Poisson families and estimated as the deviance over the residual degrees of freedom for the Normal family. The program prints it for a Poisson and a Normal fit.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  out.writeln("Poisson dispersion", fit.dispersion);
  options.family = reg::GLMFamily::Normal;
  const auto normal = reg::glm_fit(y, x, options);
  out.writeln("Normal dispersion", normal.dispersion, " deviance / df_resid", normal.deviance / static_cast<double>(normal.df_resid));
}
```

Output

```text
Poisson dispersion 1.00000
Normal dispersion 3.52727  deviance / df_resid 3.52727
```


### `deviance`

`double`.

Twice the log-likelihood ratio between the saturated model, which fits every observation exactly, and the fitted model. For the Poisson family it is 2 Σ [y ln(y/μ) − (y − μ)], with y ln y read as zero when y = 0. Smaller is a closer fit. The program recomputes it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  const auto yv = y.items<double>();
  const auto mu = fit.fitted_values.items<double>();
  double by_hand = 0.0;
  for (std::size_t i = 0; i < yv.size(); ++i) by_hand += 2.0 * ((yv[i] > 0 ? yv[i] * std::log(yv[i] / mu[i]) : 0.0) - (yv[i] - mu[i]));
  out.writeln("deviance", fit.deviance, " by hand", by_hand);
}
```

Output

```text
deviance 3.85733  by hand 3.85733
```


### `null_deviance`

`double`.

The deviance of the model with only a constant, which fits every observation with the sample mean. It is the benchmark that `deviance` is compared with; the gap between the two is what the predictors explain.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  const auto yv = y.items<double>();
  const double mean = y.mean().item<double>();
  double by_hand = 0.0;
  for (const double v : yv) by_hand += 2.0 * ((v > 0 ? v * std::log(v / mean) : 0.0) - (v - mean));
  out.writeln("null_deviance", fit.null_deviance, " by hand", by_hand, " deviance", fit.deviance);
}
```

Output

```text
null_deviance 35.44917  by hand 35.44917  deviance 3.85733
```


### `log_likelihood`

`double`.

The maximised log-likelihood of the fitted model. For the Poisson family it is Σ [y ln μ − μ − ln y!]. The program recomputes it with the log-factorial.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  const auto yv = y.items<double>();
  const auto mu = fit.fitted_values.items<double>();
  double by_hand = 0.0;
  for (std::size_t i = 0; i < yv.size(); ++i) by_hand += yv[i] * std::log(mu[i]) - mu[i] - std::lgamma(yv[i] + 1.0);
  out.writeln("log_likelihood", fit.log_likelihood, " by hand", by_hand);
}
```

Output

```text
log_likelihood -16.65177  by hand -16.65177
```


### `aic`

`double`.

Akaike's information criterion, −2 × log-likelihood + 2 × rank. It trades fit against the number of coefficients; between models of the same family fitted to the same data the smaller value is preferred. The program recomputes it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  out.writeln("aic", fit.aic, " by hand", -2.0 * fit.log_likelihood + 2.0 * static_cast<double>(fit.rank));
}
```

Output

```text
aic 37.30353  by hand 37.30353
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
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  out.writeln("bic", fit.bic, " by hand", -2.0 * fit.log_likelihood + std::log(static_cast<double>(fit.nobs)) * static_cast<double>(fit.rank));
}
```

Output

```text
bic 37.90870  by hand 37.90870
```


### `pseudo_rsquared`

`double`.

The proportion of the null deviance that the model removes, 1 − deviance / null_deviance. It is zero for a model no better than a constant and approaches one as the fit becomes exact; it is not an R² of the response and should not be read as one. The program recomputes it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  out.writeln("pseudo_rsquared", fit.pseudo_rsquared, " by hand", 1.0 - fit.deviance / fit.null_deviance);
}
```

Output

```text
pseudo_rsquared 0.89119  by hand 0.89119
```


### `condition_number`

`double`.

The ratio of the largest to the smallest singular value of the weighted design, a measure of how close the predictors are to collinear. Large values (above about 30 for a standardised design, far more for a raw one) warn that coefficients are poorly determined. Centring a predictor removes the collinearity between it and the intercept; the program shows the number falling.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  out.writeln("condition_number", fit.condition_number);
  const auto centred = x.sub(x.mean().item<double>());
  out.writeln("with the predictor centred", reg::glm_fit(y, centred, options).condition_number);
}
```

Output

```text
condition_number 7.261
with the predictor centred 1.436
```


### `converged`

`bool`.

Whether the iteration reached the tolerance before `max_iter`. Check it before using a fit made with a non-canonical link. The program forces a failure with two iterations.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  out.writeln("converged", fit.converged);
  options.max_iter = 2;
  out.writeln("with max_iter 2", reg::glm_fit(y, x, options).converged);
}
```

Output

```text
converged true
with max_iter 2 false
```


### `has_intercept`

`bool`.

Whether the design included a constant column, copied from `add_intercept`. `glm_predict` uses it to decide how to read new rows.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  out.writeln("has_intercept", fit.has_intercept);
  options.add_intercept = false;
  out.writeln("add_intercept=false", reg::glm_fit(y, x, options).has_intercept);
}
```

Output

```text
has_intercept true
add_intercept=false false
```


### `family`

`regression::GLMFamily`.

The family the fit used, copied from the options.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  out.writeln("Poisson fit records Poisson:", fit.family == reg::GLMFamily::Poisson);
  out.writeln("and not Normal:", fit.family == reg::GLMFamily::Normal);
}
```

Output

```text
Poisson fit records Poisson: true
and not Normal: false
```


### `link`

`regression::GLMLink`.

The link that the fit used. It is the resolved link, never `Canonical`: a Poisson fit with the default link reports the log link.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  out.writeln("default link resolves to Log:", fit.link == reg::GLMLink::Log, " is Canonical:", fit.link == reg::GLMLink::Canonical);
}
```

Output

```text
default link resolves to Log: true  is Canonical: false
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
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  out.writeln("default", fit.response_name);
  options.response_name = "events";
  out.writeln("named", reg::glm_fit(y, x, options).response_name);
}
```

Output

```text
default y
named events
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
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  out.writeln(fit.coefficient_names);
  options.predictor_names = {"dose"};
  out.writeln(reg::glm_fit(y, x, options).coefficient_names);
}
```

Output

```text
[const, x1]
[const, dose]
```


### `warnings`

`std::vector<std::string>`.

Notes that the fit attaches when something needs attention, as text: an iteration that did not converge, or a rank-deficient design solved with a pseudoinverse. The list is empty for a clean fit. The program provokes the first.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  out.writeln("clean fit:", fit.warnings.size(), "warnings");
  options.max_iter = 2;
  out.writeln(reg::glm_fit(y, x, options).warnings);
}
```

Output

```text
clean fit: 0 warnings
[IRLS reached max_iter without satisfying tolerance.]
```


## See Also

[`glm_fit`](glm_fit.md) produces the result, [`glm_predict`](glm_predict.md) applies its coefficients to new rows, [`glm_summary`](glm_summary.md) prints it as a report, and [`GLMOptions`](GLMOptions.md) holds the settings it records.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="GLMOptions.html">
    <span class="label">Previous</span>
    <span class="title">GLMOptions</span>
  </a>
  <a class="next-link" href="glm_fit.html">
    <span class="label">Next</span>
    <span class="title">glm_fit</span>
  </a>
</div>
<!-- apiref:nav:end -->

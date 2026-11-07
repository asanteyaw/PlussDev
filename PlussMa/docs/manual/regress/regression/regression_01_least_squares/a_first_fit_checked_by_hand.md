<!-- generated from a_first_fit_checked_by_hand.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A first fit, checked by hand

[Back to Least Squares](../regression_01_least_squares.md)

`ols_fit` fits a straight line (or a plane, or a hyperplane) through data by making the sum of squared residuals as small as possible, and it reports how uncertain every coefficient is. This page fits the simplest model, one predictor and an intercept, and then recomputes every number from the textbook formulas, so that each field of the result has a meaning you can check.

```cpp
reg::OLSFitResult reg::ols_fit(const Series &y, const Series &x, const reg::OLSOptions &options = {}, Config cfg = {});
```

`y` holds the response, one value per observation. `x` holds the predictors, one column per predictor. By default an intercept column of ones is added in front, so the first coefficient is the intercept and is named `const`.

## Eight points and a line

The program fits eight points that lie close to a line, then computes the same quantities by hand. With one predictor, the slope is S<sub>xy</sub> / S<sub>xx</sub> (the sum of products of the centred values over the sum of squared centred predictor values). The residual variance is the sum of squared residuals divided by the residual degrees of freedom n − 2. The standard errors, the t statistics and the p-values then follow. The confidence interval is checked in a different way: its half-width divided by the standard error is the critical multiplier, and the two-sided t tail beyond it must be exactly `alpha`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace reg = pluss::subsp::stats::regression;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 6.2, 7.8, 10.1, 12.2, 13.8, 16.1}, {8}, as_f64());
  const auto fit = reg::ols_fit(y, x);

  // The textbook formulas for one predictor.
  const auto dx = x.sub(x.mean());
  const auto dy = y.sub(y.mean());
  const double sxx = dx.mul(dx).sum().item<double>();
  const double sxy = dx.mul(dy).sum().item<double>();
  const double slope = sxy / sxx;
  const double intercept = y.mean().item<double>() - slope * x.mean().item<double>();
  const double s2 = y.sub(x.mul(slope).add(intercept)).pow(2.0).sum().item<double>() / 6.0;
  const double se_slope = std::sqrt(s2 / sxx);
  const double se_intercept = std::sqrt(s2 * (1.0 / 8.0 + std::pow(x.mean().item<double>(), 2) / sxx));

  out.writeln("coefficients   ", fit.coefficients.pull<double>(), " names", fit.coefficient_names);
  out.writeln("by hand        ", std::vector<double>{intercept, slope});
  out.writeln("std errors     ", fit.std_errors.pull<double>());
  out.writeln("by hand        ", std::vector<double>{se_intercept, se_slope});
  out.writeln("t values       ", fit.t_values.pull<double>());
  out.writeln("by hand        ", std::vector<double>{intercept / se_intercept, slope / se_slope});

  const distr::StudentT t6{6.0, 0.0, 1.0};
  out.writeln("p values       ", fit.p_values.pull<double>());
  out.writeln("by hand        ", t6.cdf(fit.t_values.abs()).neg().add(1.0).mul(2.0).pull<double>());

  const auto ci = fit.conf_int.host<double>().array().data;
  const double half = (ci[3] - ci[2]) / 2.0;
  out.writeln("interval of the slope", ci[2], ci[3], " half-width / std err", half / se_slope);
  const auto multiplier = pluss::make_series(std::vector<double>{half / se_slope}, {1}, as_f64());
  out.writeln("two-sided t tail beyond that multiplier (alpha = 0.05):", t6.cdf(multiplier).neg().add(1.0).mul(2.0).pull<double>());
  out.writeln("nobs", fit.nobs, " rank", fit.rank, " k_params", fit.k_params, " df_resid", fit.df_resid, " df_model", fit.df_model);
}
```

Output

```text
coefficients    [0.035714, 1.997619]  names [const, x1]
by hand         [0.035714, 1.997619]
std errors      [0.140385, 0.027800]
by hand         [0.140385, 0.027800]
t values        [0.254402, 71.855652]
by hand         [0.254402, 71.855652]
p values        [0.807680, 0.000000]
by hand         [0.807680, 0.000000]
interval of the slope 1.929594 2.065644  half-width / std err 2.446912
two-sided t tail beyond that multiplier (alpha = 0.05): [0.050000]
nobs 8  rank 2  k_params 2  df_resid 6  df_model 1
```

The pairs agree to the digits shown. The slope is 1.9976, so each unit of x adds about 2 to the response. Its standard error is 0.0278, which gives t = 71.86 and a p-value that is zero at six decimals. The intercept, 0.0357, is small next to its standard error of 0.1404 (t = 0.25, p = 0.81): the data cannot tell it from zero. The interval of the slope, [1.9296, 2.0656], is 2.4469 standard errors either side of the estimate, and the two-sided *t* tail beyond 2.4469 on 6 degrees of freedom is exactly 0.05, which is what `alpha = 0.05` asks for. The counts are `nobs` 8, `k_params` 2 and `rank` 2, so `df_resid` is 8 − 2 = 6 and `df_model` is 1.

## Where each field lives

| Field | Shape | Meaning |
|---|---|---|
| `coefficients` | `[k]` | the estimates, intercept first |
| `std_errors` | `[k]` | their standard errors, the square roots of the diagonal of `covariance` |
| `t_values`, `p_values` | `[k]` | coefficient over standard error, and the two-sided Student *t* tail probability with `df_resid` degrees of freedom |
| `conf_int` | `[k, 2]` | lower and upper limits; row *i* belongs to coefficient *i* |
| `covariance` | `[k, k]` | the estimated covariance matrix of the coefficients |
| `fitted_values`, `residuals` | `[n]` | the fitted line at each observation and `y` minus that |
| `nobs`, `rank`, `k_params`, `df_resid`, `df_model` | integers | observations used, rank of the design, number of parameters, and the two degrees of freedom |

Everything is returned in `Float64` on the platform of the predictors, whatever the input dtype was. The next pages read the remaining fields (goodness of fit, information criteria, diagnostics) and the options that change how the fit is made.

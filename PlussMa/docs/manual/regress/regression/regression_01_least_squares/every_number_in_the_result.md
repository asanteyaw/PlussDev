<!-- generated from every_number_in_the_result.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Every number in the result

[Back to Least Squares](../regression_01_least_squares.md)

The coefficient table is only part of `OLSFitResult`. The scalar fields summarise how well the model fits and how much to trust it. This page recomputes every one of them from the residuals, so each definition is fixed by a calculation you can read.

| Field | Definition |
|---|---|
| `sse` | the sum of squared residuals |
| `sst` | the total sum of squares of `y` about its mean (about zero when there is no intercept) |
| `ssr` | the explained sum of squares, `sst − sse` |
| `mse`, `rmse` | `sse / df_resid`, and its square root, the residual standard deviation |
| `rsquared` | `1 − sse / sst` |
| `adj_rsquared` | `1 − (1 − R²)(n − 1) / df_resid`, with `n` in place of `n − 1` when there is no intercept |
| `f_stat`, `f_pvalue` | `(ssr / df_model) / mse`, and its upper tail under an F distribution with `df_model` and `df_resid` degrees of freedom |
| `log_likelihood` | the Gaussian log-likelihood at the fitted line, `−(n/2)(ln(2π sse/n) + 1)` |
| `aic`, `bic` | `−2 log L + 2 r` and `−2 log L + r ln n`, where `r` is the rank of the design |

`df_resid` is `n − rank`, and `df_model` is `rank − 1` with an intercept and `rank` without one.

## Recomputing the fit statistics

The program fits the two-predictor data of the previous page and prints the library value beside the recomputed value for each field.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace reg = pluss::subsp::stats::regression;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const std::vector<double> x1{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
  const std::vector<double> x2{3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 8};
  const std::vector<double> noise{0.3, -0.2, 0.1, -0.4, 0.2, 0.0, -0.3, 0.4, -0.1, 0.2, -0.2, 0.1};
  std::vector<double> ys, flat;
  for (std::size_t i = 0; i < x1.size(); ++i) {
    ys.push_back(2.0 + 1.5 * x1[i] - 0.8 * x2[i] + noise[i]);
    flat.push_back(x1[i]);
    flat.push_back(x2[i]);
  }
  const auto y = pluss::make_series(ys, {12}, as_f64());
  const auto fit = reg::ols_fit(y, pluss::make_series(flat, {12, 2}, as_f64()));

  const double n = 12.0, rank = 3.0, df_model = 2.0, df_resid = 9.0;
  const double sse = fit.residuals.pow(2.0).sum().item<double>();
  const double sst = y.sub(y.mean()).pow(2.0).sum().item<double>();
  const double rsq = 1.0 - sse / sst;
  const double mse = sse / df_resid;
  const double f = ((sst - sse) / df_model) / mse;
  const double loglik = -0.5 * n * (std::log(2.0 * 3.14159265358979323846 * sse / n) + 1.0);
  const auto f_tail = distr::FDistribution{df_model, df_resid}.cdf(pluss::make_series(std::vector<double>{f}, {1}, as_f64())).neg().add(1.0).pull<double>()[0];

  out.writeln("sse            ", fit.sse, " recomputed", sse);
  out.writeln("sst            ", fit.sst, " recomputed", sst);
  out.writeln("ssr            ", fit.ssr, " recomputed", sst - sse);
  out.writeln("mse            ", fit.mse, " recomputed", mse);
  out.writeln("rmse           ", fit.rmse, " recomputed", std::sqrt(mse));
  out.writeln("rsquared       ", fit.rsquared, " recomputed", rsq);
  out.writeln("adj_rsquared   ", fit.adj_rsquared, " recomputed", 1.0 - (1.0 - rsq) * (n - 1.0) / df_resid);
  out.writeln("f_stat         ", fit.f_stat, " recomputed", f);
  out.writeln("f_pvalue       ", fit.f_pvalue, " recomputed", f_tail);
  out.writeln("log_likelihood ", fit.log_likelihood, " recomputed", loglik);
  out.writeln("aic            ", fit.aic, " recomputed", -2.0 * loglik + 2.0 * rank);
  out.writeln("bic            ", fit.bic, " recomputed", -2.0 * loglik + rank * std::log(n));
  out.writeln("rank / df_model / df_resid", fit.rank, "/", fit.df_model, "/", fit.df_resid);
}
```

Output

```text
sse             0.529325  recomputed 0.529325
sst             243.369167  recomputed 243.369167
ssr             242.839842  recomputed 242.839842
mse             0.058814  recomputed 0.058814
rmse            0.242516  recomputed 0.242516
rsquared        0.997825  recomputed 0.997825
adj_rsquared    0.997342  recomputed 0.997342
f_stat          2064.476862  recomputed 2064.476862
f_pvalue        0.000000  recomputed 0.000000
log_likelihood  1.699093  recomputed 1.699093
aic             2.601814  recomputed 2.601814
bic             4.056534  recomputed 4.056534
rank / df_model / df_resid 3 / 2 / 9
```

Every recomputed value equals the library's. The two predictors explain 99.78% of the variation (R² = 0.9978); the adjusted value, 0.9973, is a little lower because it charges for the two predictors. F = 2064.5 on (2, 9) degrees of freedom, with a p-value below the six decimals printed. The log-likelihood is positive (1.70): for continuous data it is a density, and the residual standard deviation of 0.24 makes the density high, so a positive value is correct. AIC and BIC use the rank, 3, as the number of parameters.

## Without an intercept

When the model has no intercept the line is forced through the origin, so the baseline to improve on is the constant zero rather than the mean. The total sum of squares is then taken about zero, `df_model` equals the rank (there is no intercept to discount), and the F test asks whether any coefficient differs from zero. R² keeps its meaning of the share of that baseline removed by the fit, and it cannot be negative. The same data are fitted both ways below, once around a mean of 10 where the response does not depend on the predictor.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, as_f64());
  const auto flat = pluss::make_series(std::vector<double>{10.1, 9.9, 10.2, 9.8, 10.0, 10.1, 9.9, 10.0}, {8}, as_f64());

  const auto with_intercept = reg::ols_fit(flat, x);
  reg::OLSOptions origin;
  origin.add_intercept = false;
  const auto through_origin = reg::ols_fit(flat, x, origin);

  const double about_mean = flat.sub(flat.mean()).pow(2.0).sum().item<double>();
  const double about_zero = flat.pow(2.0).sum().item<double>();
  out.writeln("with an intercept   : sst", with_intercept.sst, " (about the mean", about_mean, ") R2", with_intercept.rsquared, " df_model", with_intercept.df_model);
  out.writeln("through the origin  : sst", through_origin.sst, " (about zero    ", about_zero, ") R2", through_origin.rsquared, " df_model", through_origin.df_model);
  out.writeln("R2 recomputed about zero", 1.0 - through_origin.sse / about_zero);
  out.writeln("F with / without    ", with_intercept.f_stat, "/", through_origin.f_stat);
  out.writeln("warnings", through_origin.warnings);
}
```

Output

```text
with an intercept   : sst 0.120000  (about the mean 0.120000 ) R2 0.031746  df_model 1
through the origin  : sst 800.120000  (about zero     800.120000 ) R2 0.792235  df_model 1
R2 recomputed about zero 0.792235
F with / without     0.196721 / 26.691926
warnings [Model has no explicit intercept; R-squared and F-statistic interpretations differ from intercept models.]
```

With an intercept the data give R² = 0.0317 and F = 0.197, which says there is no relation between x and the response. Through the origin the total sum of squares is 800.12, the sum of squares of the response itself, and R² = 0.7922 measures the improvement over the baseline of zero; the recomputation about zero agrees. The two R² values answer different questions and cannot be compared across the two models: the high value through the origin reflects only that the response sits near 10, not any dependence on x. `df_model` is 1 in both fits: rank 2 less the intercept, and rank 1. The warning names the change of interpretation.

## When a field is not defined

Some statistics need more observations than parameters, or some spread in the residuals. Where a quantity does not exist, the field holds `NaN` (or an infinity for a perfect fit) instead of a made-up number. The program fits three cases: a line through exactly two points (no degrees of freedom are left), a line through points that lie exactly on it (no residual spread), and a normal fit for comparison.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  auto fit_line = [&](const char *label, std::vector<double> xs, std::vector<double> ys) {
    const auto n = static_cast<std::int64_t>(xs.size());
    const auto fit = reg::ols_fit(pluss::make_series(ys, {n}, as_f64()), pluss::make_series(xs, {n}, as_f64()));
    out.writeln(label, "df_resid", fit.df_resid, " mse", fit.mse, " std errors", fit.std_errors.pull<double>(), " p", fit.p_values.pull<double>());
    out.writeln("      R2", fit.rsquared, " adj R2", fit.adj_rsquared, " F", fit.f_stat, " F p", fit.f_pvalue, " aic", fit.aic, " Durbin-Watson", fit.durbin_watson);
    out.writeln("      intervals", fit.conf_int.host<double>().array().data);
  };
  fit_line("two points     :", {1, 2}, {1, 3});
  fit_line("exact line     :", {1, 2, 3, 4, 5}, {3, 5, 7, 9, 11});
  fit_line("ordinary data  :", {1, 2, 3, 4, 5}, {3.1, 4.8, 7.3, 8.9, 11.2});
}
```

Output

```text
two points     : df_resid 0  mse nan  std errors [nan, nan]  p [nan, nan]
      R2 1.0000  adj R2 nan  F nan  F p nan  aic -inf  Durbin-Watson nan
      intervals [nan, nan, nan, nan]
exact line     : df_resid 3  mse 0.0000  std errors [0.0000, 0.0000]  p [0.0000, 0.0000]
      R2 1.0000  adj R2 1.0000  F nan  F p nan  aic -inf  Durbin-Watson nan
      intervals [1.0000, 1.0000, 2.0000, 2.0000]
ordinary data  : df_resid 3  mse 0.0543  std errors [0.2445, 0.0737]  p [0.0286, 0.0001]
      R2 0.9961  adj R2 0.9947  F 758.4479  F p 0.0001  aic 1.0722  Durbin-Watson 3.6049
      intervals [0.1920, 1.7480, 1.7954, 2.2646]
```

Two points leave no residual degrees of freedom. The line passes through both, so R² is 1, but `mse`, the standard errors, the p-values, the intervals, the adjusted R², F and Durbin–Watson are `NaN`, and AIC is −∞ because a fit with zero residuals has an infinite log-likelihood. Points exactly on a line have degrees of freedom but zero residuals: the standard errors are zero, the intervals collapse onto the coefficients (1 and 2), and F is `NaN` because the residual variance it divides by is zero. For ordinary data every field is defined; the Durbin–Watson value of 3.60 reflects residuals that alternate in sign.

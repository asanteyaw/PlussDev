<!-- generated from case_weights.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Case weights

[Back to Least Squares](../regression_01_least_squares.md)

`OLSOptions::weights` gives each row a non-negative weight and turns the fit into *weighted least squares*: the coefficients minimise Σ wᵢ (yᵢ − xᵢ′b)². The weights are read as *precision* weights, so the model is that the variance of the error in row *i* is σ² / wᵢ. A row with a larger weight is measured more precisely and counts for more.

```cpp
reg::OLSOptions options;
options.weights = weights;   // a Series of length n; finite and non-negative; undefined means every weight is 1
```

| Quantity | With weights |
|---|---|
| `coefficients`, `covariance` | the weighted least-squares estimate and σ² (X′WX)⁻¹ |
| `sse`, `sst`, `ssr`, `rsquared` | weighted sums of squares; with an intercept `sst` is about the weighted mean |
| `mse`, `rmse` | `sse / df_resid`, the variance of the weighted residuals √wᵢ rᵢ |
| `residuals`, `fitted_values` | in the units of `y`: `y − Xb`, not multiplied by the weights |
| `durbin_watson`, `skewness`, `kurtosis`, `jarque_bera`, `omnibus` | computed from the weighted residuals √wᵢ rᵢ, which have constant variance when the weights are right |
| `log_likelihood` | the Gaussian likelihood of precision weights, −½ [n (ln(2π sse / n) + 1) − Σ ln wᵢ] |
| `nobs`, `df_resid` | the rows used, and rows less rank; a row of weight zero is not used and counts in `dropped_observations` |

Scaling every weight by a constant leaves the coefficients, standard errors, R² and F unchanged. Only `sse`, `mse` and `rmse`, which are in units of the weights, change.

## Integer weights are repeated rows

For whole-number weights, the coefficients, the sums of squares and R² are those of the data set in which each row is repeated `weight` times. The covariance differs only through the degrees of freedom: the weighted fit has the rows' degrees of freedom, the repeated fit those of the longer data set, so the standard errors differ by the square root of the ratio of the two.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const std::vector<double> xs{1, 2, 3, 4, 5, 6}, ys{1.1, 2.3, 2.8, 4.4, 4.7, 6.5}, counts{3, 1, 2, 1, 2, 1};
  std::vector<double> repeated_x, repeated_y;
  for (std::size_t i = 0; i < xs.size(); ++i) {
    for (int c = 0; c < static_cast<int>(counts[i]); ++c) {
      repeated_x.push_back(xs[i]);
      repeated_y.push_back(ys[i]);
    }
  }
  reg::OLSOptions options;
  options.weights = pluss::make_series(counts, {6}, as_f64());
  const auto weighted = reg::ols_fit(pluss::make_series(ys, {6}, as_f64()), pluss::make_series(xs, {6}, as_f64()), options);
  const auto repeated = reg::ols_fit(pluss::make_series(repeated_y, {10}, as_f64()), pluss::make_series(repeated_x, {10}, as_f64()));

  out.writeln("coefficients   weighted", weighted.coefficients.items<double>(), " repeated rows", repeated.coefficients.items<double>());
  out.writeln("sse            weighted", weighted.sse, " repeated rows", repeated.sse);
  out.writeln("sst            weighted", weighted.sst, " repeated rows", repeated.sst);
  out.writeln("R-squared      weighted", weighted.rsquared, " repeated rows", repeated.rsquared);
  out.writeln("residual df    weighted", weighted.df_resid, " repeated rows", repeated.df_resid);
  out.writeln("std errors     weighted", weighted.std_errors.items<double>(), " repeated rows", repeated.std_errors.items<double>());
  out.writeln("ratio of std errors", weighted.std_errors.div(repeated.std_errors).items<double>(), " = sqrt(df_repeated / df_weighted)", std::sqrt(8.0 / 4.0));

  // Scaling every weight changes nothing but the units of the sums of squares.
  reg::OLSOptions scaled;
  scaled.weights = pluss::make_series(std::vector<double>{30, 10, 20, 10, 20, 10}, {6}, as_f64());
  const auto big = reg::ols_fit(pluss::make_series(ys, {6}, as_f64()), pluss::make_series(xs, {6}, as_f64()), scaled);
  out.writeln("weights times 10: coefficients", big.coefficients.items<double>(), " std errors", big.std_errors.items<double>(), " R2", big.rsquared, " F", big.f_stat, "(F before", weighted.f_stat, ")");
  out.writeln("                  sse", big.sse, " (ten times", 10.0 * weighted.sse, ")");
}
```

Output

```text
coefficients   weighted [0.075081, 0.991909]  repeated rows [0.075081, 0.991909]
sse            weighted 0.762977  repeated rows 0.762977
sst            weighted 31.165000  repeated rows 31.165000
R-squared      weighted 0.975518  repeated rows 0.975518
residual df    weighted 4  repeated rows 8
std errors     weighted [0.279994, 0.078568]  repeated rows [0.197986, 0.055556]
ratio of std errors [1.414214, 1.414214]  = sqrt(df_repeated / df_weighted) 1.414214
weights times 10: coefficients [0.075081, 0.991909]  std errors [0.279994, 0.078568]  R2 0.975518  F 159.386240 (F before 159.386240 )
                  sse 7.629773  (ten times 7.629773 )
```

The coefficients (0.0751 and 0.9919), the sums of squares and R² are identical to the repeated-rows fit, to six decimals. The residual degrees of freedom are 4 against 8, so the standard errors differ by exactly √(8/4) = 1.414, as the last-but-one line shows. Multiplying every weight by 10 changes nothing but the units of the sums of squares: the coefficients, standard errors, R² and F are the same, and `sse` is ten times larger.

## Precision weights: unequal variances

The commonest reason for weights is a response whose variance changes from row to row. If the standard deviation of the error grows in proportion to x, the variance is proportional to x², and weights of 1/x² restore a constant variance. Ordinary least squares is still unbiased, but it spends its effort on the noisy rows; weighted least squares is the efficient estimate. The program fits data with errors proportional to x both ways, and checks that the weighted residuals have the same spread on both halves of the data, which the plain residuals do not.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  std::vector<double> xs, ys, weights;
  for (int i = 1; i <= 40; ++i) {
    const double noise = 2.0 * std::fmod(i * 0.6180339887498949, 1.0) - 1.0;
    xs.push_back(i);
    ys.push_back(2.0 + 1.5 * i + 0.4 * i * noise);  // the error grows in proportion to x
    weights.push_back(1.0 / (static_cast<double>(i) * i));
  }
  const auto x = pluss::make_series(xs, {40}, as_f64());
  const auto y = pluss::make_series(ys, {40}, as_f64());

  const auto plain = reg::ols_fit(y, x);
  reg::OLSOptions options;
  options.weights = pluss::make_series(weights, {40}, as_f64());
  const auto weighted = reg::ols_fit(y, x, options);
  out.writeln("true coefficients [2.0000, 1.5000]");
  out.writeln("OLS      ", plain.coefficients.items<double>(), " std errors", plain.std_errors.items<double>());
  out.writeln("weighted ", weighted.coefficients.items<double>(), " std errors", weighted.std_errors.items<double>());

  // The spread of the residuals in the first and second halves of the data.
  auto spread = [&](const pluss::Series &r, int first, int count) {
    std::vector<double> v = r.items<double>();
    double sum = 0.0;
    for (int i = first; i < first + count; ++i) {
      sum += v[i] * v[i];
    }
    return std::sqrt(sum / count);
  };
  const auto weighted_residuals = weighted.residuals.mul(options.weights.sqrt());
  out.writeln("residual spread, first half / second half");
  out.writeln("  OLS residuals            ", spread(plain.residuals, 0, 20), spread(plain.residuals, 20, 20));
  out.writeln("  weighted fit, y - Xb     ", spread(weighted.residuals, 0, 20), spread(weighted.residuals, 20, 20));
  out.writeln("  weighted fit, sqrt(w) r  ", spread(weighted_residuals, 0, 20), spread(weighted_residuals, 20, 20));
  out.writeln("rmse of the weighted residuals", weighted.rmse, " = sqrt(sse / df_resid)", std::sqrt(weighted.sse / static_cast<double>(weighted.df_resid)));
}
```

Output

```text
true coefficients [2.0000, 1.5000]
OLS       [2.3394, 1.4740]  std errors [1.7892, 0.0761]
weighted  [2.0521, 1.4902]  std errors [0.2167, 0.0436]
residual spread, first half / second half
  OLS residuals             2.5904 7.2016
  weighted fit, y - Xb      2.5857 7.2084
  weighted fit, sqrt(w) r   0.2196 0.2357
rmse of the weighted residuals 0.2337  = sqrt(sse / df_resid) 0.2337
```

Ordinary least squares and the weighted fit are both close to the true coefficients (2, 1.5), but the weighted fit is far more precise: its standard errors are 0.217 and 0.044 against 1.789 and 0.076. The plain residuals, and the weighted fit's residuals in the units of `y`, spread out with x (2.59 in the first half and 7.20 in the second). The weighted residuals √w r have the same spread in both halves (0.220 and 0.236), which is what the weights are for, and it is the weighted residuals on which `rmse` (0.2337) and the diagnostics are based.

## Zero weights and invalid weights

A weight of zero removes the row, and the row is counted in `dropped_observations`. A negative weight, a weight vector of the wrong length, and a case with no row of positive weight are errors; a `NaN` weight makes its row incomplete, so it is dropped by default and an error with `drop_missing = false` ([Missing data](missing_data.md)).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <limits>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1.2, 1.9, 3.4, 3.8, 5.3, 5.9, 8.4, 7.8}, {8}, as_f64());
  const double nan = std::numeric_limits<double>::quiet_NaN();

  reg::OLSOptions zero;
  zero.weights = pluss::make_series(std::vector<double>{1, 1, 1, 1, 1, 1, 0, 1}, {8}, as_f64());
  const auto fit = reg::ols_fit(y, x, zero);
  out.writeln("weight 0 on the 7th row: nobs", fit.nobs, " dropped", fit.dropped_observations, " fitted values", fit.fitted_values.numel(), " coefficients", fit.coefficients.items<double>());

  auto attempt = [&](const char *label, std::vector<double> w, bool drop_missing = true) {
    reg::OLSOptions o;
    o.weights = pluss::make_series(w, {static_cast<std::int64_t>(w.size())}, as_f64());
    o.drop_missing = drop_missing;
    try {
      const auto f = reg::ols_fit(y, x, o);
      out.writeln(label, "-> fitted with", f.nobs, "rows");
    } catch (const std::exception &e) {
      const std::string message = e.what();
      const auto start = message.find("ols_fit: ") + 9;
      out.writeln(label, "->", message.substr(start, message.find('\n') - start));
    }
  };
  attempt("a negative weight          ", {1, 1, 1, -1, 1, 1, 1, 1});
  attempt("seven weights for 8 rows   ", {1, 1, 1, 1, 1, 1, 1});
  attempt("every weight zero          ", {0, 0, 0, 0, 0, 0, 0, 0});
  attempt("a NaN weight               ", {1, 1, 1, nan, 1, 1, 1, 1});
  attempt("a NaN weight, drop_missing off", {1, 1, 1, nan, 1, 1, 1, 1}, false);
}
```

Output

```text
weight 0 on the 7th row: nobs 7  dropped 1  fitted values 7  coefficients [0.2262, 0.9557]
a negative weight           -> weights must be non-negative
seven weights for 8 rows    -> weights must have one value per row
every weight zero           -> no observations left after filtering missing values
a NaN weight                -> fitted with 7 rows
a NaN weight, drop_missing off -> non-finite values found and drop_missing=false
```

A zero weight removes the row (`nobs` 7, `dropped_observations` 1, and the fitted values and residuals have 7 entries). A negative weight, a weight vector of the wrong length, and a case in which every weight is zero are errors. A `NaN` weight makes its row incomplete: it is dropped by default and is an error when `drop_missing` is off.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="options.html">
    <span class="label">Previous</span>
    <span class="title">Options</span>
  </a>
  <a class="next-link" href="missing_data.html">
    <span class="label">Next</span>
    <span class="title">Missing data</span>
  </a>
</div>
<!-- apiref:nav:end -->

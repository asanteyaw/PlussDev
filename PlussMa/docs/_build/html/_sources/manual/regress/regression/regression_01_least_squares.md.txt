<!-- generated from regression_01_least_squares.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Regression 1: Least Squares

Least squares fits a line (or a plane) to data by making the sum of squared residuals as small as possible. It is the foundation of regression, and Pluss reports with it everything needed to judge the fit: uncertainty of each coefficient, goodness of fit, and diagnostics. After this chapter you can fit a model, read the result, predict, control the options, deal with missing values, and recognise collinear predictors.

Every program is complete and its output is the output of that program.

## The call

```cpp
reg::OLSFitResult reg::ols_fit(const Series& y, const Series& x, const reg::OLSOptions& options = {}, Config cfg = {});
Series            reg::ols_predict(const reg::OLSFitResult& fit, const Series& x_new, Config cfg = {});
```

`y` has shape `[n]`; the predictors `x` have shape `[n, p]` (a vector `[n]` is one predictor). By default an intercept column is added, and its coefficient comes first, named `const`; the others are named `x1`, `x2`, ... or by `predictor_names`. A model with no predictors and an intercept (`x` of shape `[n, 0]`) fits the sample mean.

| `OLSOptions` | Meaning | Default |
|---|---|---|
| `alpha` | confidence level: intervals have coverage `1 - alpha` | 0.05 |
| `add_intercept` | add a column of ones | true |
| `drop_missing` | drop rows with a non-finite value in `y` or `x` (false makes them an error) | true |
| `rcond` | relative cutoff for treating a direction of the design as rank deficient | 1e-12 |
| `response_name`, `predictor_names` | labels used by the summary | `"y"`, none |

| `OLSFitResult` | Meaning |
|---|---|
| `coefficients`, `std_errors`, `t_values`, `p_values` | one per parameter |
| `conf_int` | `[k, 2]` lower and upper limits |
| `covariance` | the `[k, k]` covariance matrix of the coefficients |
| `fitted_values`, `residuals` | `[n]` |
| `nobs`, `rank`, `k_params`, `df_model`, `df_resid`, `dropped_observations` | counts |
| `rsquared`, `adj_rsquared`, `f_stat`, `f_pvalue` | goodness of fit |
| `sse`, `ssr`, `sst`, `mse`, `rmse` | sums of squares and residual scale |
| `log_likelihood`, `aic`, `bic` | information criteria |
| `durbin_watson`, `jarque_bera` (+ `_pvalue`), `skewness`, `kurtosis` | residual diagnostics |
| `condition_number`, `smallest_eig`, `warnings` | conditioning of the design |

## Programs

- [A first fit, checked by hand](regression_01_least_squares/a_first_fit_checked_by_hand.md)
- [The summary, and prediction](regression_01_least_squares/the_summary_and_prediction.md)
- [Options and missing data](regression_01_least_squares/options_and_missing_data.md)
- [Collinear predictors](regression_01_least_squares/collinear_predictors.md)

## Checklist

1. `y` is `[n]`, `x` is `[n, p]`; the intercept is added and listed first.
2. Read the standard errors and intervals, not just the coefficients.
3. Check `warnings` and `condition_number` before trusting a coefficient.
4. Keep predictions inside the range of the data.
5. Missing values are dropped by default; read `dropped_observations`.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

regression_01_least_squares/a_first_fit_checked_by_hand
regression_01_least_squares/the_summary_and_prediction
regression_01_least_squares/options_and_missing_data
regression_01_least_squares/collinear_predictors
```
<!-- apiref:toctree:end -->

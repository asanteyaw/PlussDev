<!-- generated from regression_02_generalised_linear_models.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Regression 2: Generalised Linear Models

Least squares assumes a continuous response with constant variance about a straight-line mean. A generalised linear model (GLM) relaxes both: the mean is a function of the linear predictor (the *link*), and the variance is a function of the mean (the *family*). That covers yes/no outcomes (logistic regression), counts (Poisson regression), and least squares itself. After this chapter you can fit each, read what its coefficients mean, choose a link, detect a fit that did not converge, compare models by deviance, and read the report.

Every program is complete, and its output is the output of that program.

## The calls

```cpp
reg::GLMFitResult reg::glm_fit(const Series &y, const Series &x, const reg::GLMOptions &options = {}, Config cfg = {});
Series            reg::glm_predict(const reg::GLMFitResult &fit, const Series &x_new,
                                   reg::GLMPredictionType type = reg::GLMPredictionType::Mean, Config cfg = {});
std::string       reg::glm_summary(const reg::GLMFitResult &fit);
std::ostream     &reg::print_glm_summary(std::ostream &os, const reg::GLMFitResult &fit);
```

Inputs are read as for least squares ([Shapes and inputs](regression_01_least_squares/shapes_and_inputs.md)). The model is fitted by iteratively reweighted least squares (IRLS).

| `GLMOptions` | Meaning | Default |
|---|---|---|
| `family` | `Normal`, `Binomial` (response in [0, 1]) or `Poisson` (response ≥ 0) | `Normal` |
| `link` | `Canonical` (identity, logit, log for the three families), or `Identity`, `Log`, `Logit` | `Canonical` |
| `max_iter`, `tol` | IRLS iteration cap and convergence tolerance | 200, 1e-8 |
| `weights`, `offset` | case weights (finite, non-negative; zero removes the row) and a known term added to the linear predictor | none |
| `alpha` | the intervals have coverage `1 − alpha` | 0.05 |
| `add_intercept`, `drop_missing`, `rcond` | as for least squares | true, true, 1e-12 |
| `response_name`, `predictor_names` | labels used by the summary | `"y"`, none |

| `GLMFitResult` | Meaning |
|---|---|
| `coefficients`, `std_errors`, `z_values`, `p_values` | one per parameter; the p-values and intervals use the normal distribution |
| `conf_int`, `covariance` | `[k, 2]` limits and the `[k, k]` covariance matrix |
| `fitted_values`, `linear_predictor`, `residuals` | the means μ, the linear predictor η, and the response residuals y − μ |
| `deviance`, `null_deviance`, `pseudo_rsquared`, `dispersion` | fit statistics; `dispersion` is estimated for the normal family and 1 for the others |
| `log_likelihood`, `aic`, `bic` | information criteria |
| `iterations`, `converged` | what IRLS did |
| `nobs`, `rank`, `k_params`, `num_predictors`, `df_model`, `df_resid`, `dropped_observations`, `condition_number`, `warnings` | counts and conditioning |
| `family`, `link`, `has_intercept`, `coefficient_names`, `response_name`, `alpha` | what the fit was made with; `link` is never `Canonical` |

`glm_predict` returns the mean (`Mean`, a probability for the binomial family) or the linear predictor (`Linear`); an overload takes the offset for the new rows.

## Programs

- [Logistic regression](regression_02_generalised_linear_models/logistic_regression.md)
- [Poisson regression](regression_02_generalised_linear_models/poisson_regression.md)
- [Families and links](regression_02_generalised_linear_models/families_and_links.md)
- [The normal family and least squares](regression_02_generalised_linear_models/the_normal_family_and_least_squares.md)
- [Iteration controls and separation](regression_02_generalised_linear_models/iteration_controls_and_separation.md)
- [Deviance and model comparison](regression_02_generalised_linear_models/deviance_and_model_comparison.md)
- [Weights and offsets](regression_02_generalised_linear_models/weights_and_offsets.md)
- [Responses, the summary and errors](regression_02_generalised_linear_models/responses_summary_and_errors.md)

## Checklist

1. Binary outcome: `Binomial`. Counts: `Poisson`. A continuous response with errors of constant spread: `Normal`, or least squares.
2. Always read `converged`. A coefficient of enormous size with a huge standard error is separation, not a result.
3. Coefficients of a logistic model are log-odds and of a Poisson model log-rates; exponentiate them (and their interval limits) for odds ratios and rate ratios.
4. The binomial and Poisson families fix the dispersion at 1. Check the Pearson statistic divided by `df_resid` and widen the standard errors if it is much larger than 1.
5. Compare nested models by the drop in `deviance` (chi-squared), and non-nested ones by `aic` or `bic` on the same rows.
6. Grouped data are fitted with the group sizes as `weights`; counts over unequal exposures with the log exposure as `offset`.
7. With few degrees of freedom prefer least squares to the normal-family GLM: its *t* intervals are the right ones.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

regression_02_generalised_linear_models/logistic_regression
regression_02_generalised_linear_models/poisson_regression
regression_02_generalised_linear_models/families_and_links
regression_02_generalised_linear_models/the_normal_family_and_least_squares
regression_02_generalised_linear_models/iteration_controls_and_separation
regression_02_generalised_linear_models/deviance_and_model_comparison
regression_02_generalised_linear_models/weights_and_offsets
regression_02_generalised_linear_models/responses_summary_and_errors
```
<!-- apiref:toctree:end -->

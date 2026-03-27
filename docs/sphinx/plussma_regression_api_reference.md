# PlussMa Regression API Reference

This page documents the full public regression API in `pluss::subsp::stats::regression` (alias: `pluss::ma::reg`).

## Include And Namespace

### API Signature

- `#include <pluss/ma.h>`
- `#include <pluss/ma/stats/regression.h>`
- `namespace reg = pluss::ma::reg`

### Notes

Use `pluss::ma::reg` for concise usage in applications.

### Inputs

- Pluss headers
- `Series` objects for predictors/response

### Output

- Full regression surface available (OLS, GLM, Quantile, Polynomial)

### Options

- Prefer `Float64` config for statistical stability.

### Examples

```cpp
#include <pluss/ma.h>
namespace reg = pluss::ma::reg;
```

### Expected output

- Translation unit has access to all APIs documented below.

## Shared Data Contract

### API Signature

- `y: Series` as shape `[n]`, `[n,1]`, or `[1,n]`
- `x: Series` as shape `[n,p]` or `[n]` (treated as `[n,1]`)

### Notes

Rows represent observations and columns represent predictors.

### Inputs

- Finite numeric values (or use `drop_missing=true` where supported)

### Output

- Internal shape normalization for fitting/prediction

### Options

- `drop_missing` determines filtering behavior on non-finite rows.

### Examples

```cpp
const pluss::Series x = pluss::make_series(std::vector<double>{1,2,3,4}, {4,1}, cfg);
const pluss::Series y = pluss::make_series(std::vector<double>{2,4,6,8}, {4}, cfg);
```

### Expected output

- `x` and `y` accepted directly by `*_fit` APIs.

## Enum: OLSCovarianceType

### API Signature

- `enum OLSCovarianceType { Nonrobust }`

### Notes

Covariance estimator selector for OLS.

### Inputs

- `Nonrobust`

### Output

- Controls covariance/stderr style in OLS fit.

### Options

- Current public option: `Nonrobust`.

### Examples

```cpp
reg::OLSOptions opt;
opt.covariance = reg::OLSCovarianceType::Nonrobust;
```

### Expected output

- OLS fit uses non-robust covariance estimation.

## Struct: OLSOptions

### API Signature

- `OLSOptions{ alpha, add_intercept, drop_missing, rcond, response_name, predictor_names, covariance }`

### Notes

Configuration object for `ols_fit`.

### Inputs

- `alpha: double` in `(0,1)`
- `add_intercept: bool`
- `drop_missing: bool`
- `rcond: double > 0`
- `response_name: string`
- `predictor_names: vector<string>` (size must equal predictor count before intercept handling)
- `covariance: OLSCovarianceType`

### Output

- Fully configured options object.

### Options

- Defaults: `alpha=0.05`, `add_intercept=true`, `drop_missing=true`, `rcond=1e-12`, `response_name="y"`.

### Examples

```cpp
reg::OLSOptions opt;
opt.alpha = 0.01;
opt.add_intercept = true;
opt.drop_missing = true;
opt.rcond = 1e-10;
opt.response_name = "price";
opt.predictor_names = {"sqft", "rooms", "age"};
```

### Expected output

- `opt` can be passed directly to `reg::ols_fit(...)`.

## Enum: GLMFamily

### API Signature

- `enum GLMFamily { Normal, Binomial, Poisson }`

### Notes

Selects response distribution in GLM fitting.

### Inputs

- `Normal`
- `Binomial`
- `Poisson`

### Output

- Determines variance model and target-domain checks.

### Options

- Binomial enforces `y in [0,1]`.
- Poisson enforces `y >= 0`.

### Examples

```cpp
reg::GLMOptions o1; o1.family = reg::GLMFamily::Normal;
reg::GLMOptions o2; o2.family = reg::GLMFamily::Binomial;
reg::GLMOptions o3; o3.family = reg::GLMFamily::Poisson;
```

### Expected output

- GLM fit behavior follows chosen family constraints.

## Enum: GLMLink

### API Signature

- `enum GLMLink { Canonical, Identity, Log, Logit }`

### Notes

Selects link function in GLM.

### Inputs

- `Canonical` (resolved by family)
- `Identity`
- `Log`
- `Logit`

### Output

- Controls transformation between linear predictor and mean response.

### Options

- Canonical defaults:
  - Normal -> Identity
  - Binomial -> Logit
  - Poisson -> Log

### Examples

```cpp
reg::GLMOptions opt;
opt.family = reg::GLMFamily::Binomial;
opt.link = reg::GLMLink::Canonical;
```

### Expected output

- Fitted result reports resolved `fit.link` as `Logit`.

## Enum: GLMPredictionType

### API Signature

- `enum GLMPredictionType { Mean, Linear }`

### Notes

Controls output scale for `glm_predict`.

### Inputs

- `Mean`: inverse-link scale
- `Linear`: raw linear predictor

### Output

- Prediction `Series` on requested scale.

### Options

- Use `Linear` for calibration/debugging; `Mean` for standard inference.

### Examples

```cpp
auto mu = reg::glm_predict(fit, x_test, reg::GLMPredictionType::Mean, cfg);
auto eta = reg::glm_predict(fit, x_test, reg::GLMPredictionType::Linear, cfg);
```

### Expected output

- `mu` and `eta` have same length, different scales.

## Struct: GLMOptions

### API Signature

- `GLMOptions{ family, link, alpha, add_intercept, drop_missing, rcond, max_iter, tol, response_name, predictor_names }`

### Notes

Configuration object for `glm_fit`.

### Inputs

- `family: GLMFamily`
- `link: GLMLink`
- `alpha: double in (0,1)`
- `add_intercept: bool`
- `drop_missing: bool`
- `rcond: double > 0`
- `max_iter: int64 > 0`
- `tol: double > 0`
- `response_name: string`
- `predictor_names: vector<string>`

### Output

- GLM fit settings.

### Options

- Defaults: `family=Normal`, `link=Canonical`, `max_iter=200`, `tol=1e-8`.

### Examples

```cpp
reg::GLMOptions opt;
opt.family = reg::GLMFamily::Poisson;
opt.link = reg::GLMLink::Log;
opt.max_iter = 600;
opt.tol = 1e-9;
opt.predictor_names = {"x"};
```

### Expected output

- IRLS will run with requested tolerance/iteration budget.

## Struct: QuantileOptions

### API Signature

- `QuantileOptions{ tau, alpha, add_intercept, drop_missing, rcond, max_iter, tol, eps, response_name, predictor_names }`

### Notes

Configuration object for `quantile_fit`.

### Inputs

- `tau: double in (0,1)`
- `alpha: double in (0,1)`
- `add_intercept: bool`
- `drop_missing: bool`
- `rcond: double > 0`
- `max_iter: int64 > 0`
- `tol: double > 0`
- `eps: double > 0` (stability floor for weighted steps)
- `response_name: string`
- `predictor_names: vector<string>`

### Output

- Quantile fit settings.

### Options

- Defaults: `tau=0.5`, `max_iter=500`, `tol=1e-7`, `eps=1e-6`.

### Examples

```cpp
reg::QuantileOptions opt;
opt.tau = 0.9;
opt.max_iter = 1000;
opt.tol = 1e-8;
opt.eps = 1e-6;
```

### Expected output

- Quantile IRLS runs with specified robustness and convergence controls.

## Struct: PolyOptions

### API Signature

- `PolyOptions{ degree, add_intercept, include_cross, drop_missing, rcond, alpha, response_name, predictor_names }`

### Notes

Configuration object for `poly_fit` and basis-driven OLS.

### Inputs

- `degree: int64 >= 1`
- `add_intercept: bool`
- `include_cross: bool`
- `drop_missing: bool`
- `rcond: double > 0`
- `alpha: double in (0,1)`
- `response_name: string`
- `predictor_names: vector<string>`

### Output

- Polynomial expansion + OLS fit settings.

### Options

- Defaults: `degree=2`, `include_cross=false`, `add_intercept=true`.

### Examples

```cpp
reg::PolyOptions opt;
opt.degree = 3;
opt.include_cross = true;
opt.predictor_names = {"a", "b"};
```

### Expected output

- Generated polynomial basis includes powers and cross terms up to selected degree.

## Struct: OLSFitResult

### API Signature

- `OLSFitResult{ coefficients, std_errors, t_values, p_values, conf_int, covariance, fitted_values, residuals, ... }`

### Notes

Comprehensive OLS output container (parameters, uncertainty, diagnostics, metadata).

### Inputs

- Produced by `ols_fit(...)`.

### Output

- Parameter estimates, fit vectors, model statistics, and warnings.

### Options

Key scalar fields:
- `nobs`, `rank`, `k_params`, `num_predictors`, `dropped_observations`
- `df_model`, `df_resid`
- `mse`, `rmse`, `sse`, `ssr`, `sst`
- `rsquared`, `adj_rsquared`, `f_stat`, `f_pvalue`
- `log_likelihood`, `aic`, `bic`
- `durbin_watson`, `jarque_bera`, `jarque_bera_pvalue`, `skewness`, `kurtosis`
- `condition_number`, `smallest_eig`

### Examples

```cpp
const auto fit = reg::ols_fit(y, x, {}, cfg);
std::cout << "R2=" << fit.rsquared << " rmse=" << fit.rmse << "\n";
std::cout << fit.coefficients << "\n";
```

### Expected output

- Printed scalar diagnostics and coefficient vector for inspection.

## Struct: GLMFitResult

### API Signature

- `GLMFitResult{ coefficients, std_errors, z_values, p_values, conf_int, covariance, fitted_values, linear_predictor, residuals, ... }`

### Notes

Comprehensive GLM output container.

### Inputs

- Produced by `glm_fit(...)`.

### Output

- Parameters, uncertainty, deviance-based diagnostics, convergence metadata.

### Options

Key scalar fields:
- `nobs`, `rank`, `k_params`, `num_predictors`, `dropped_observations`
- `df_model`, `df_resid`, `iterations`, `converged`
- `dispersion`, `deviance`, `null_deviance`, `pseudo_rsquared`
- `log_likelihood`, `aic`, `bic`, `condition_number`
- `family`, `link`

### Examples

```cpp
const auto fit = reg::glm_fit(y, x, glm_opt, cfg);
std::cout << "converged=" << (fit.converged ? "true" : "false")
          << " iter=" << fit.iterations
          << " pseudo_r2=" << fit.pseudo_rsquared << "\n";
```

### Expected output

- Convergence and quality diagnostics for model acceptance checks.

## Struct: QuantileFitResult

### API Signature

- `QuantileFitResult{ coefficients, std_errors, t_values, p_values, conf_int, covariance, fitted_values, residuals, ... }`

### Notes

Comprehensive quantile fit output container.

### Inputs

- Produced by `quantile_fit(...)`.

### Output

- Parameter estimates plus quantile-specific objective diagnostics.

### Options

Key scalar fields:
- `nobs`, `rank`, `k_params`, `num_predictors`, `dropped_observations`
- `df_model`, `df_resid`, `iterations`, `converged`
- `tau`, `objective`, `objective_null`, `pseudo_rsquared`, `residual_density0`

### Examples

```cpp
const auto fit = reg::quantile_fit(y, x, q_opt, cfg);
std::cout << "tau=" << fit.tau
          << " obj=" << fit.objective
          << " pseudo_r2=" << fit.pseudo_rsquared << "\n";
```

### Expected output

- Quantile-specific diagnostics to compare tail/median models.

## Struct: PolyFitResult

### API Signature

- `PolyFitResult{ ols, degree, include_cross, base_predictor_names }`

### Notes

Polynomial fit wrapper around expanded-basis OLS.

### Inputs

- Produced by `poly_fit(...)`.

### Output

- `ols` contains full regression result on polynomial basis.

### Options

- `degree`: polynomial order
- `include_cross`: cross-term enablement
- `base_predictor_names`: original predictor labels

### Examples

```cpp
const auto fit = reg::poly_fit(y, x, poly_opt, cfg);
std::cout << "degree=" << fit.degree
          << " cross=" << (fit.include_cross ? "true" : "false") << "\n";
```

### Expected output

- Polynomial metadata and OLS statistics accessible in one object.

## Function: ols_fit

### API Signature

- `ols_fit(y: Series, x: Series, options: OLSOptions = {}, cfg: Config = {}) -> OLSFitResult`

### Notes

Fits least-squares linear regression using pseudoinverse-based solve.

### Inputs

- `y`: response vector
- `x`: predictor matrix
- `options`: OLS configuration
- `cfg`: optional runtime config

### Output

- `OLSFitResult`

### Options

- Honors all `OLSOptions` fields, including missing policy and intercept handling.

### Examples

```cpp
reg::OLSOptions opt;
opt.response_name = "target";
opt.predictor_names = {"x1", "x2"};
const auto fit = reg::ols_fit(y, x, opt, cfg);
```

### Expected output

- Fitted coefficients, uncertainty metrics, and diagnostics.

## Function: ols_predict

### API Signature

- `ols_predict(fit: OLSFitResult, x: Series, cfg: Config = {}) -> Series`

### Notes

Generates OLS predictions with automatic intercept-column handling when appropriate.

### Inputs

- `fit`: fitted OLS result
- `x`: prediction design matrix
- `cfg`: optional runtime config

### Output

- Predicted response `Series` of length `n_test`

### Options

- If `fit.has_intercept=true`, accepts either `num_predictors` columns or already-expanded `k_params` columns.

### Examples

```cpp
const auto pred = reg::ols_predict(fit, x_test, cfg);
```

### Expected output

- One predicted value per test row.

## Function: ols_summary

### API Signature

- `ols_summary(fit: OLSFitResult) -> string`

### Notes

Returns formatted OLS summary text.

### Inputs

- `fit`: OLS result

### Output

- Human-readable report string

### Options

- Uses metadata fields (`response_name`, predictor names, warnings) from `fit`.

### Examples

```cpp
std::string report = reg::ols_summary(fit);
std::cout << report << "\n";
```

### Expected output

- OLS report with coefficient table and diagnostics sections.

## Function: print_ols_summary

### API Signature

- `print_ols_summary(os: ostream&, fit: OLSFitResult) -> ostream&`

### Notes

Streams OLS summary directly to output stream.

### Inputs

- `os`: output stream
- `fit`: OLS result

### Output

- Same stream reference

### Options

- Equivalent report content to `ols_summary`.

### Examples

```cpp
reg::print_ols_summary(std::cout, fit) << "\n";
```

### Expected output

- OLS report written directly to stdout (or any stream).

## Function: glm_fit

### API Signature

- `glm_fit(y: Series, x: Series, options: GLMOptions = {}, cfg: Config = {}) -> GLMFitResult`

### Notes

Fits GLM using IRLS.

### Inputs

- `y`: response vector
- `x`: predictor matrix
- `options`: GLM family/link/convergence config
- `cfg`: runtime config

### Output

- `GLMFitResult`

### Options

- `max_iter` and `tol` control convergence.
- Family-specific domain constraints are enforced.

### Examples

```cpp
reg::GLMOptions opt;
opt.family = reg::GLMFamily::Binomial;
opt.link = reg::GLMLink::Logit;
const auto fit = reg::glm_fit(y_bin, x_bin, opt, cfg);
```

### Expected output

- Binomial GLM fit with convergence diagnostics and fitted probabilities.

## Function: glm_predict

### API Signature

- `glm_predict(fit: GLMFitResult, x: Series, type: GLMPredictionType = Mean, cfg: Config = {}) -> Series`

### Notes

Generates GLM predictions on mean scale or linear predictor scale.

### Inputs

- `fit`: GLM result
- `x`: prediction design matrix
- `type`: mean or linear output
- `cfg`: runtime config

### Output

- Prediction `Series`

### Options

- Same intercept-column behavior as `ols_predict`.

### Examples

```cpp
auto p = reg::glm_predict(fit, x_test, reg::GLMPredictionType::Mean, cfg);
auto eta = reg::glm_predict(fit, x_test, reg::GLMPredictionType::Linear, cfg);
```

### Expected output

- `p`: linked mean predictions.
- `eta`: raw linear predictions.

## Function: glm_summary

### API Signature

- `glm_summary(fit: GLMFitResult) -> string`

### Notes

Returns formatted GLM summary text.

### Inputs

- `fit`: GLM result

### Output

- Human-readable report string

### Options

- Displays family/link, deviance, pseudo-R2, convergence, and coefficient table.

### Examples

```cpp
std::cout << reg::glm_summary(fit) << "\n";
```

### Expected output

- GLM report string with fit diagnostics and warnings.

## Function: print_glm_summary

### API Signature

- `print_glm_summary(os: ostream&, fit: GLMFitResult) -> ostream&`

### Notes

Streams GLM summary directly to output stream.

### Inputs

- `os`: output stream
- `fit`: GLM result

### Output

- Same stream reference

### Options

- Equivalent report content to `glm_summary`.

### Examples

```cpp
reg::print_glm_summary(std::cout, fit) << "\n";
```

### Expected output

- GLM report printed directly.

## Function: quantile_fit

### API Signature

- `quantile_fit(y: Series, x: Series, options: QuantileOptions = {}, cfg: Config = {}) -> QuantileFitResult`

### Notes

Fits quantile regression using iterative weighted updates.

### Inputs

- `y`: response vector
- `x`: predictor matrix
- `options`: quantile/convergence settings
- `cfg`: runtime config

### Output

- `QuantileFitResult`

### Options

- `tau` sets target conditional quantile.
- `eps` stabilizes weight updates near zero residuals.

### Examples

```cpp
reg::QuantileOptions opt;
opt.tau = 0.75;
const auto fit = reg::quantile_fit(y, x, opt, cfg);
```

### Expected output

- Fitted model targeting 75th conditional percentile.

## Function: quantile_predict

### API Signature

- `quantile_predict(fit: QuantileFitResult, x: Series, cfg: Config = {}) -> Series`

### Notes

Generates quantile predictions for new inputs.

### Inputs

- `fit`: quantile result
- `x`: prediction design matrix
- `cfg`: runtime config

### Output

- Prediction `Series`

### Options

- Same intercept-column behavior as OLS/GLM predict APIs.

### Examples

```cpp
auto qhat = reg::quantile_predict(fit, x_test, cfg);
```

### Expected output

- One quantile estimate per test row.

## Function: quantile_summary

### API Signature

- `quantile_summary(fit: QuantileFitResult) -> string`

### Notes

Returns formatted quantile regression summary text.

### Inputs

- `fit`: quantile result

### Output

- Human-readable report string

### Options

- Includes `tau`, objective diagnostics, coefficient table, warnings.

### Examples

```cpp
std::cout << reg::quantile_summary(fit) << "\n";
```

### Expected output

- Quantile regression report string.

## Function: print_quantile_summary

### API Signature

- `print_quantile_summary(os: ostream&, fit: QuantileFitResult) -> ostream&`

### Notes

Streams quantile summary directly to output stream.

### Inputs

- `os`: output stream
- `fit`: quantile result

### Output

- Same stream reference

### Options

- Equivalent report content to `quantile_summary`.

### Examples

```cpp
reg::print_quantile_summary(std::cout, fit) << "\n";
```

### Expected output

- Quantile report printed directly.

## Function: poly_expand

### API Signature

- `poly_expand(x: Series, degree: int64, include_cross: bool = false, cfg: Config = {}) -> Series`

### Notes

Builds polynomial basis matrix without fitting.

### Inputs

- `x`: base predictor matrix
- `degree`: polynomial degree (`>=1`)
- `include_cross`: include cross terms when degree >= 2
- `cfg`: runtime config

### Output

- Expanded design matrix `Series`

### Options

- For multi-feature inputs with `include_cross=true`, cross terms are generated by total degree.

### Examples

```cpp
auto z = reg::poly_expand(x, 3, true, cfg);
```

### Expected output

- Matrix with polynomial columns usable by any downstream regressor.

## Function: poly_fit

### API Signature

- `poly_fit(y: Series, x: Series, options: PolyOptions = {}, cfg: Config = {}) -> PolyFitResult`

### Notes

Fits polynomial regression by expanding predictors and applying OLS on the basis.

### Inputs

- `y`: response vector
- `x`: predictor matrix
- `options`: polynomial + OLS controls
- `cfg`: runtime config

### Output

- `PolyFitResult`

### Options

- `degree`, `include_cross`, and naming controls affect basis terms and coefficient labels.

### Examples

```cpp
reg::PolyOptions opt;
opt.degree = 2;
opt.include_cross = true;
opt.predictor_names = {"a", "b"};
const auto fit = reg::poly_fit(y, x, opt, cfg);
```

### Expected output

- Polynomial model fit with embedded OLS diagnostics on expanded basis.

## Function: poly_predict

### API Signature

- `poly_predict(fit: PolyFitResult, x: Series, cfg: Config = {}) -> Series`

### Notes

Predicts by rebuilding polynomial basis from incoming `x` and applying embedded OLS coefficients.

### Inputs

- `fit`: polynomial fit
- `x`: new predictor matrix
- `cfg`: runtime config

### Output

- Predicted response `Series`

### Options

- Uses `fit.degree`, `fit.include_cross`, and `fit.base_predictor_names` to enforce basis consistency.

### Examples

```cpp
auto pred = reg::poly_predict(fit, x_test, cfg);
```

### Expected output

- Predictions aligned with trained polynomial basis structure.

## Function: poly_summary

### API Signature

- `poly_summary(fit: PolyFitResult) -> string`

### Notes

Returns formatted polynomial regression summary text.

### Inputs

- `fit`: polynomial fit

### Output

- Human-readable report string

### Options

- Report starts with polynomial metadata then prints OLS summary.

### Examples

```cpp
std::cout << reg::poly_summary(fit) << "\n";
```

### Expected output

- Polynomial header plus full OLS-style detail.

## Function: print_poly_summary

### API Signature

- `print_poly_summary(os: ostream&, fit: PolyFitResult) -> ostream&`

### Notes

Streams polynomial summary directly to output stream.

### Inputs

- `os`: output stream
- `fit`: polynomial fit

### Output

- Same stream reference

### Options

- Equivalent report content to `poly_summary`.

### Examples

```cpp
reg::print_poly_summary(std::cout, fit) << "\n";
```

### Expected output

- Polynomial summary printed directly.

## Quick Failure-Mode Map

### API Signature

- Common runtime checks across `*_fit` and `*_predict` functions

### Notes

These are the most common failure categories users hit first.

### Inputs

- Invalid shapes, invalid option ranges, invalid response domain, singular or low-information design

### Output

- Clear fail-loud error or warning metadata in result

### Options

- Use: `drop_missing=true`, stricter feature engineering, and larger sample sizes for better conditioning.

### Examples

```cpp
// Examples of option guards:
reg::GLMOptions g; g.max_iter = 0;           // invalid
reg::QuantileOptions q; q.tau = 1.1;         // invalid
reg::OLSOptions o; o.rcond = 0.0;            // invalid
```

### Expected output

- Fitting fails with descriptive validation errors before computation proceeds.


# PlussMa Regression User Guide

This guide is task-first and practical. It covers the full regression surface in `pluss::subsp::stats::regression` (also available as `pluss::ma::reg`).

Covered families:
- OLS (least-squares linear regression)
- GLM (Normal, Binomial, Poisson)
- Quantile regression
- Polynomial regression (including optional cross terms)

## 1) Includes And Namespace

```cpp
#include <pluss/ma.h>
// or narrower:
#include <pluss/ma/stats/regression.h>

namespace reg = pluss::ma::reg;
```

Recommended baseline config for stable numerical work:

```cpp
auto cfg = pluss::Config{}
  .with_dtype(pluss::ScalarType::Float64)
  .with_platform(pluss::pCPU);
```

Help notes:
- Keep `Float64` for diagnostics-heavy statistical workloads.
- `x` is expected as `[n, p]` (rows are observations, columns are predictors).
- `y` is expected as `[n]` (or `n x 1` / `1 x n`, which is reshaped internally).

## 2) Minimal OLS Workflow

```cpp
const pluss::Series x = pluss::make_series(
    std::vector<double>{
      1.0, 2.0,
      2.0, 1.0,
      3.0, 0.0,
      4.0, 2.0,
      5.0, 1.0,
      6.0, 3.0,
      7.0, 4.0,
      8.0, 5.0,
    },
    {8, 2},
    cfg);

const pluss::Series y = pluss::make_series(
    std::vector<double>{0.9, 4.9, 7.2, 6.7, 10.0, 10.8, 10.6, 11.9},
    {8},
    cfg);

reg::OLSOptions opt;
opt.response_name = "target";
opt.predictor_names = {"feat1", "feat2"};

const auto fit = reg::ols_fit(y, x, opt, cfg);
const pluss::Series pred = reg::ols_predict(
    fit,
    pluss::make_series(std::vector<double>{9.0, 6.0, 10.0, 7.0}, {2, 2}, cfg),
    cfg);

std::cout << reg::ols_summary(fit) << "\n";
std::cout << pred << "\n";
```

Expected output:
- OLS summary text with `R-squared`, `coef`, confidence intervals, residual diagnostics, and warnings (if any).
- Prediction `Series` with one value per test row.

## 3) OLS Options You Should Tune First

`reg::OLSOptions` fields:
- `alpha`: confidence interval level control (CI uses `1 - alpha`).
- `add_intercept`: prepend intercept column.
- `drop_missing`: if `true`, rows with non-finite values are dropped; otherwise fitting fails on non-finite rows.
- `rcond`: pseudoinverse cutoff for rank detection and solve stability.
- `response_name`: label shown in summaries.
- `predictor_names`: column labels used in coefficient tables.
- `covariance`: currently `Nonrobust`.

Example (no intercept + strict missing policy):

```cpp
reg::OLSOptions opt;
opt.add_intercept = false;
opt.drop_missing = false;
opt.alpha = 0.01;
opt.rcond = 1e-10;

const auto fit = reg::ols_fit(y, x, opt, cfg);
```

Expected output:
- Fit succeeds only when all rows are finite.
- Summary includes warning that no intercept changes interpretation of `R-squared` and `F-statistic`.

## 4) GLM Workflows (Normal / Binomial / Poisson)

### 4.1 GLM Normal + Identity (OLS-like mean model)

```cpp
reg::GLMOptions opt;
opt.family = reg::GLMFamily::Normal;
opt.link = reg::GLMLink::Identity;
opt.response_name = "target";
opt.predictor_names = {"x1", "x2"};

const auto fit = reg::glm_fit(y, x, opt, cfg);
std::cout << reg::glm_summary(fit) << "\n";
```

Expected output:
- GLM summary with `Deviance`, `Pseudo R-squared`, `AIC/BIC`, z-stats, and convergence status.

### 4.2 GLM Binomial + Logit

```cpp
const pluss::Series x_bin = pluss::make_series(
  std::vector<double>{-2, -1, 0, 1, 2, 3, 4, 5},
  {8, 1},
  cfg);
const pluss::Series y_bin = pluss::make_series(
  std::vector<double>{0.06, 0.12, 0.22, 0.40, 0.62, 0.79, 0.90, 0.96},
  {8},
  cfg);

reg::GLMOptions opt;
opt.family = reg::GLMFamily::Binomial;
opt.link = reg::GLMLink::Logit;

const auto fit = reg::glm_fit(y_bin, x_bin, opt, cfg);
const pluss::Series p = reg::glm_predict(fit, x_bin, reg::GLMPredictionType::Mean, cfg);
```

Expected output:
- `p` values in `[0, 1]`.
- Fitting fails if `y` contains values outside `[0,1]` for binomial.

### 4.3 GLM Poisson + Log

```cpp
reg::GLMOptions opt;
opt.family = reg::GLMFamily::Poisson;
opt.link = reg::GLMLink::Log;

const auto fit = reg::glm_fit(y_count, x_count, opt, cfg);
const pluss::Series mean_count = reg::glm_predict(fit, x_count, reg::GLMPredictionType::Mean, cfg);
const pluss::Series linear_eta = reg::glm_predict(fit, x_count, reg::GLMPredictionType::Linear, cfg);
```

Expected output:
- `mean_count` positive expected counts.
- `linear_eta` is unlinked linear predictor.
- Fitting fails if Poisson response has negatives.

## 5) GLM Options That Control Convergence

`reg::GLMOptions` fields:
- `family`, `link`
- `alpha`
- `add_intercept`
- `drop_missing`
- `rcond`
- `max_iter`
- `tol`
- `response_name`, `predictor_names`

Example with canonical link auto-resolution:

```cpp
reg::GLMOptions opt;
opt.family = reg::GLMFamily::Binomial;
opt.link = reg::GLMLink::Canonical; // resolves to logit for binomial
opt.max_iter = 400;
opt.tol = 1e-9;

const auto fit = reg::glm_fit(y_bin, x_bin, opt, cfg);
```

Expected output:
- `fit.link` reports resolved concrete link (`Logit` here).
- `fit.converged` indicates IRLS stop reason.

## 6) Quantile Regression Workflows

### 6.1 Median regression (`tau = 0.5`)

```cpp
reg::QuantileOptions opt;
opt.tau = 0.5;
opt.predictor_names = {"feature"};

const auto fit = reg::quantile_fit(y, x1, opt, cfg);
const pluss::Series pred = reg::quantile_predict(fit, x1_test, cfg);
std::cout << reg::quantile_summary(fit) << "\n";
```

Expected output:
- Quantile summary with objective, pseudo R-squared, residual density at 0, and convergence info.
- Predictions for the conditional `tau`-quantile.

### 6.2 Upper-tail regression (`tau = 0.9`)

```cpp
reg::QuantileOptions opt;
opt.tau = 0.9;
opt.max_iter = 1200;
opt.tol = 1e-8;
opt.eps = 1e-6;

const auto fit = reg::quantile_fit(y, x1, opt, cfg);
```

Expected output:
- Coefficients biased toward high-tail conditional behavior.
- Warning appears if IRLS reaches `max_iter` before tolerance.

## 7) Polynomial Regression Workflows

### 7.1 Univariate polynomial

```cpp
reg::PolyOptions opt;
opt.degree = 3;
opt.include_cross = false;
opt.predictor_names = {"u"};

const auto fit = reg::poly_fit(y, x1, opt, cfg);
const pluss::Series pred = reg::poly_predict(fit, x1_test, cfg);
std::cout << reg::poly_summary(fit) << "\n";
```

Expected output:
- OLS summary over polynomial-expanded basis (`u`, `u^2`, `u^3`, ...).

### 7.2 Multivariate with cross terms

```cpp
reg::PolyOptions opt;
opt.degree = 2;
opt.include_cross = true;
opt.predictor_names = {"a", "b"};

const auto fit = reg::poly_fit(y2, x2, opt, cfg);
```

Expected output:
- Expanded basis includes terms like `a*b`.
- `fit.ols.coefficient_names` contains generated polynomial/cross labels.

### 7.3 Basis expansion without fitting

```cpp
const pluss::Series basis = reg::poly_expand(x2, /*degree=*/3, /*include_cross=*/true, cfg);
```

Expected output:
- Returned matrix with polynomial basis columns ready for custom learners.

## 8) Prediction Input Shape Rules (Important)

All `*_predict` APIs follow the same rule when model has an intercept:
- If incoming `x` has `num_predictors` columns, intercept column is added automatically.
- If incoming `x` already has `k_params` columns, it is used directly.

Without intercept:
- Incoming `x` must have exactly `num_predictors` columns.

Expected output:
- Consistent behavior across `ols_predict`, `glm_predict`, `quantile_predict`, `poly_predict`.

## 9) Reading Result Structs Correctly

### 9.1 OLS fit result

Useful fields:
- vectors/matrices: `coefficients`, `std_errors`, `t_values`, `p_values`, `conf_int`, `covariance`
- fit vectors: `fitted_values`, `residuals`
- model stats: `rsquared`, `adj_rsquared`, `f_stat`, `f_pvalue`, `aic`, `bic`, `condition_number`
- diagnostics: `durbin_watson`, `jarque_bera`, `jarque_bera_pvalue`, `warnings`

### 9.2 GLM fit result

Useful fields:
- vectors/matrices: `coefficients`, `std_errors`, `z_values`, `p_values`, `conf_int`, `covariance`
- fit vectors: `fitted_values`, `linear_predictor`, `residuals`
- model stats: `deviance`, `null_deviance`, `pseudo_rsquared`, `aic`, `bic`, `dispersion`
- convergence: `iterations`, `converged`, `warnings`

### 9.3 Quantile fit result

Useful fields:
- vectors/matrices: `coefficients`, `std_errors`, `t_values`, `p_values`, `conf_int`, `covariance`
- fit vectors: `fitted_values`, `residuals`
- objective stats: `objective`, `objective_null`, `pseudo_rsquared`, `residual_density0`
- convergence: `iterations`, `converged`, `warnings`

### 9.4 Poly fit result

Useful fields:
- `ols`: full embedded OLS result on expanded basis
- `degree`, `include_cross`, `base_predictor_names`

Expected output:
- You can consume rich diagnostics directly without re-computing basic stats.

## 10) Handling Missing/Invalid Data

All families support `drop_missing`.
- `drop_missing=true`: rows with non-finite values in `x` or `y` are removed.
- `drop_missing=false`: fitting fails if non-finite values exist.

Example:

```cpp
reg::OLSOptions opt;
opt.drop_missing = true;
const auto fit = reg::ols_fit(y_with_nan, x_with_nan, opt, cfg);
std::cout << "dropped rows: " << fit.dropped_observations << "\n";
```

Expected output:
- Successful fit on cleaned subset, with dropped count exposed.

## 11) Failure Modes And How To Fix Them

1. `row mismatch between y and X`
- Fix: ensure `y.numel() == x.rows`.

2. `requires n >= number of parameters`
- Fix: reduce predictors/degree or increase samples.

3. `predictor_names size must equal ...`
- Fix: pass exactly one name per original predictor column (before intercept handling).

4. GLM target-domain errors:
- Binomial: enforce `y in [0,1]`.
- Poisson: enforce `y >= 0`.

5. Rank deficiency warnings:
- Fix: remove collinear features, reduce polynomial degree, or regularize externally.

Expected output:
- Cleaner fits, fewer warnings, and more interpretable uncertainty estimates.

## 12) End-To-End Example: Train, Predict, Summarize

```cpp
#include <pluss/ma.h>
#include <iostream>

int main() {
  namespace reg = pluss::ma::reg;
  auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x = pluss::make_series(
      std::vector<double>{1,2, 2,1, 3,0, 4,2, 5,1, 6,3, 7,4, 8,5},
      {8,2},
      cfg);
  const pluss::Series y = pluss::make_series(
      std::vector<double>{0.9,4.9,7.2,6.7,10.0,10.8,10.6,11.9},
      {8},
      cfg);

  reg::OLSOptions opt;
  opt.response_name = "target";
  opt.predictor_names = {"feat1","feat2"};

  const auto fit = reg::ols_fit(y, x, opt, cfg);
  const auto pred = reg::ols_predict(
      fit,
      pluss::make_series(std::vector<double>{9.0,6.0,10.0,7.0}, {2,2}, cfg),
      cfg);

  std::cout << reg::ols_summary(fit) << "\n";
  std::cout << "pred:\n" << pred << "\n";
  return 0;
}
```

Expected output:
- Complete OLS report.
- Two prediction values for the two test rows.

## 13) Practical Model Selection Guidance

- Start with OLS for baseline linear signal and interpretable diagnostics.
- Move to GLM when response distribution/link structure matters (probabilities/counts).
- Use Quantile when median/tail robustness is more relevant than mean fit.
- Use Polynomial only when curvature is needed; monitor rank/warning diagnostics closely.


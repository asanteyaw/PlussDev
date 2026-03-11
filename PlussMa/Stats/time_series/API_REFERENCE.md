# PlussMa Time-Series API Reference

This page documents the currently implemented time-series core APIs in:

- `pluss::subsp::stats::ts`

## `TsPartition expanding_window(std::int64_t num_observations, std::int64_t num_test_sets, const PartitionOptions& options = {})`

Builds expanding-window backtest splits.

`std::int64_t num_observations`: total observation count (`> 1`).

`std::int64_t num_test_sets`: number of rolling test folds (`> 0`).

`const PartitionOptions& options`: split controls.

Return -> `TsPartition`: partition object with fold windows plus index/mask extractors.

Notes:

- Supports `Direction::Forward` and `Direction::Reverse`.
- Honors `gap_size` and `max_lag` to avoid leakage.
- Throws on invalid combinations (for example `max_lag >= min_train_size`).

Example usage:

```cpp
ts::PartitionOptions opts;
opts.direction = ts::Direction::Reverse;
opts.gap_size = 1;
opts.min_train_size = 6;
opts.test_size = 3;
opts.step_size = 2;
auto p = ts::TsPartition::expanding_window(32, 4, opts);
```

Expected output:

```text
p.num_test_sets() == 4
p.window(0).train_size() >= 6
p.window(0).test_size() == 3
```

## `TsPartition sliding_window(std::int64_t num_observations, std::int64_t num_test_sets, const PartitionOptions& options = {})`

Builds fixed-size rolling-window backtest splits.

`std::int64_t num_observations`: total observation count (`> 1`).

`std::int64_t num_test_sets`: number of test folds (`> 0`).

`const PartitionOptions& options`: requires/uses `train_size`, `test_size`, optional `step_size`, `gap_size`, `max_lag`.

Return -> `TsPartition`: partition with fixed train-width folds.

Notes:

- `max_lag < train_size` must hold.
- Forward and reverse traversal are supported.

Example usage:

```cpp
ts::PartitionOptions opts;
opts.direction = ts::Direction::Forward;
opts.train_size = 8;
opts.test_size = 2;
opts.step_size = 2;
auto p = ts::TsPartition::sliding_window(30, 4, opts);
```

Expected output:

```text
For each fold i: p.window(i).train_size() == 8 and p.window(i).test_size() == 2
```

## `TsPartition holdout(std::int64_t num_observations, double holdout = 0.1, const PartitionOptions& options = {})`

Builds one holdout split.

`std::int64_t num_observations`: total observation count (`> 1`).

`double holdout`: if `< 1.0`, interpreted as fraction; if `>= 1.0`, interpreted as integer count.

`const PartitionOptions& options`: supports `gap_size` and `max_lag`.

Return -> `TsPartition`: single-window holdout partition.

Notes:

- `holdout >= 1.0` must be an integer-valued count.
- `max_lag < train_size` must hold.

Example usage:

```cpp
ts::PartitionOptions opts;
opts.gap_size = 1;
opts.max_lag = 3;
auto p = ts::TsPartition::holdout(40, 0.2, opts);
```

Expected output:

```text
p.num_test_sets() == 1
p.window(0).test_size() == 8
```

## `Series training(std::int64_t i, Config cfg = {}) const`

Extracts integer indices for fold `i` training rows.

`std::int64_t i`: fold index in `[0, num_test_sets)`.

`Config cfg`: output config (`Int64` + target platform is applied internally).

Return -> `Series`: 1D index vector for training rows.

Notes:

- Companion methods: `test(i)`, `test_buffer(i)`, `training_mask(i)`, `test_mask(i)`, `test_buffer_mask(i)`.

Example usage:

```cpp
auto train_idx = p.training(0);
auto test_idx = p.test(0);
auto train_mask = p.training_mask(0);
```

Expected output:

```text
train_idx/test_idx are disjoint index vectors
train_mask has ones only at train_idx positions
```

## `LagPlan build_lag_plan(std::int64_t num_predictors, const LagPlanOptions& options)`

Builds normalized lag metadata and per-horizon predictor-activation mask.

`std::int64_t num_predictors`: number of predictor columns in `X`.

`const LagPlanOptions& options`: horizon/lag configuration.

Return -> `LagPlan`: normalized lag plan (`max_lag`, `max_lead`, `feature_count`, `prepared_predictors_per_horizon`, etc).

Notes:

- `horizon` must be non-empty and all values must be `>= 1`.
- `response_lags` and non-leading predictor lags must be `>= 1`.
- `leading_predictor_lags` values are relative lead/lag offsets and may include `0`.

Example usage:

```cpp
ts::LagPlanOptions opts;
opts.horizon = {1, 2, 3};
opts.response_lags = {1, 2};
opts.leading_predictors = {2};
opts.predictor_lags = {{1, 2}, {1}};
opts.leading_predictor_lags = {{0, 1}};
auto plan = ts::build_lag_plan(3, opts);
```

Expected output:

```text
plan.max_lag == 2
plan.max_lead == 2
plan.feature_count == 9
plan.prepared_predictors_per_horizon has shape [3, 9]
```

## `Series build_lagged_predictors(const Series& x, const Series& y, const LagPlan& plan, Config cfg = {})`

Builds lagged design matrix from `X`, `Y`, and `LagPlan`.

`const Series& x`: predictor matrix (`[n, p]` or vector treated as `[n,1]`).

`const Series& y`: response vector (`[n]` or `[n,1]`).

`const LagPlan& plan`: lag metadata from `build_lag_plan`.

`Config cfg`: output platform/pinning control (`Float64` output dtype is enforced).

Return -> `Series`: lagged design matrix with shape `[n + plan.max_lag + plan.max_lead, plan.feature_count]`.

Notes:

- Output contains `NaN` where shifts are outside observed range.
- This should not be confused with response-only lagging; it combines non-leading predictors, response lags, and leading predictor transformed lags.

Example usage:

```cpp
auto design = ts::build_lagged_predictors(x, y, plan);
std::cout << design << "\n";
```

Expected output:

```text
design.sizes()[1] == plan.feature_count
design.sizes()[0] == n + plan.max_lag + plan.max_lead
design has finite values in in-range shifted regions and NaN padding at boundaries
```

## `DirectForecaster DirectForecaster::fit(const Series& x, const Series& y, const DirectForecasterOptions& options = {}, Config cfg = {})`

Fits a multi-horizon direct forecaster (one linear model per horizon step).

`const Series& x`: predictor matrix (`[n, p]` or `[n]` treated as `[n,1]`).

`const Series& y`: response vector (`[n]` or `[n,1]`).

`const DirectForecasterOptions& options`: typed training options.

`Config cfg`: execution platform/pinning config (`Float64` internal fit path is enforced for stability).

Return -> `DirectForecaster`: fitted object with lag plan, per-step active feature indices, and coefficients.

Notes:

- Uses the lag core (`build_lag_plan` + `build_lagged_predictors`) internally.
- For each horizon step, only features active in `prepared_predictors_per_horizon` are used.
- Boundary `NaN` values from lag construction are mapped to zero in fit/predict linear algebra path to keep solves stable and deterministic.

Example usage:

```cpp
pluss::subsp::stats::ts::DirectForecasterOptions opt;
opt.lag.horizon = {1, 2, 3};
opt.lag.response_lags = {1, 2};
opt.lag.predictor_lags = {{1, 2}};
opt.learner = pluss::subsp::stats::ts::DirectLearnerKind::Ridge;
opt.ridge_lambda = 1e-4;

auto mdl = pluss::subsp::stats::ts::DirectForecaster::fit(x, y, opt);
```

Expected output:

```text
mdl.num_steps() == 3
mdl.lag_plan().feature_count > 0
mdl.step(0).beta is finite
```

## `Series DirectForecaster::predict(const Series& x, const Series& y, Config cfg = {}) const`

Predicts response for each fitted horizon step.

`const Series& x`: predictor input.

`const Series& y`: response input used when response lags are part of model features.

`Config cfg`: output platform/pinning config.

Return -> `Series`: prediction matrix `[n, H]`, where `H = num_steps()`.

Notes:

- If model uses response lags, `y` must be provided.
- Uses horizon-specific active feature subsets for each column of output.

Example usage:

```cpp
auto pred = mdl.predict(x, y);
```

Expected output:

```text
pred.sizes()[0] == n
pred.sizes()[1] == mdl.num_steps()
pred is finite for typical bounded numeric inputs
```

## `Series DirectForecaster::prepared_predictors(const Series& x, const Series& y, std::int64_t horizon_step, Config cfg = {}) const`

Builds horizon-specific prepared predictors used by the fitted step model.

`const Series& x`: predictor input.

`const Series& y`: response input used when response lags are part of model features.

`std::int64_t horizon_step`: one of values in `mdl.lag_plan().horizon`.

`Config cfg`: output platform/pinning config.

Return -> `Series`: matrix `[n, d_h]` where `d_h` is active feature count for the requested horizon (plus intercept when enabled).

Notes:

- Throws if `horizon_step` is not present in the fitted model.

Example usage:

```cpp
auto xh2 = mdl.prepared_predictors(x, y, 2);
```

Expected output:

```text
xh2.sizes()[0] == n
xh2.sizes()[1] > 0
```

## `Series DirectForecaster::forecast(const Series& x, const Series& y, Config cfg = {}) const`

Returns terminal-horizon forecasts from the latest available observation context.

`const Series& x`: predictor input.

`const Series& y`: response input.

`Config cfg`: output platform/pinning config.

Return -> `Series`: vector `[H]` of forecasts ordered by fitted horizon.

Notes:

- Current implementation is for no-future-leading-data path; if leading predictors require future values, this method throws.

Example usage:

```cpp
auto yf = mdl.forecast(x, y);
```

Expected output:

```text
yf.sizes()[0] == mdl.num_steps()
yf is finite for non-leading configurations
```

## `Series DirectForecaster::loss(const Series& x, const Series& y, RegressionLoss kind = RegressionLoss::MSE, Config cfg = {}) const`

Computes per-horizon loss values.

`const Series& x`: predictor input.

`const Series& y`: observed response vector.

`RegressionLoss kind`: loss family (`RegressionLoss::MSE` or `RegressionLoss::MAE`).

`Config cfg`: output platform/pinning config.

Return -> `Series`: 1D vector `[H]` of loss values.

Notes:

- Loss is computed against `predict(x,y)` for each horizon column.

Example usage:

```cpp
auto mse = mdl.loss(x, y, ts::RegressionLoss::MSE);
auto mae = mdl.loss(x, y, ts::RegressionLoss::MAE);
```

Expected output:

```text
mse.sizes()[0] == mdl.num_steps() and mae.sizes()[0] == mdl.num_steps()
loss values are non-negative
```

## `const DirectStepModel& DirectForecaster::step(std::int64_t i) const`

Returns fitted model details for one horizon step.

`std::int64_t i`: step index in `[0, num_steps())`.

Return -> `const DirectStepModel&`: contains `horizon`, `active_columns`, and `beta`.

Notes:

- Throws on out-of-range `i`.

Example usage:

```cpp
const auto& s0 = mdl.step(0);
std::cout << s0.horizon << "\n";
std::cout << s0.active_columns.size() << "\n";
std::cout << s0.beta << "\n";
```

Expected output:

```text
s0.horizon is one of mdl.lag_plan().horizon values
s0.beta length == s0.active_columns.size() + (fit_intercept ? 1 : 0)
```

## `DirectCVForecaster DirectCVForecaster::fit(const Series& x, const Series& y, const TsPartition& partition, const DirectForecasterOptions& options = {}, Config cfg = {})`

Fits fold-wise direct forecasting models using `TsPartition`.

`const Series& x`: predictor matrix.

`const Series& y`: response vector.

`const TsPartition& partition`: time-series partition object.

`const DirectForecasterOptions& options`: base direct-forecast options reused in each fold.

`Config cfg`: output platform/pinning config.

Return -> `DirectCVForecaster`: cross-validated direct forecasting object with one fitted `DirectForecaster` per fold.

Notes:

- Compact naming: `DirectCVForecaster` is used instead of longer partitioned naming.
- Validates that partition observation count matches `y`.
- Validates fold train/test sizes against lag/horizon requirements.

Example usage:

```cpp
auto part = ts::TsPartition::sliding_window(1800, 4, popts);
auto cv = ts::DirectCVForecaster::fit(x, y, part, opt);
```

Expected output:

```text
cv.num_folds() == part.num_test_sets()
cv.num_steps() == opt.lag.horizon.size()
```

## `Series DirectCVForecaster::cvpredict(const Series& x, const Series& y, Config cfg = {}) const`

Computes cross-validated predictions over all observations and horizons.

`const Series& x`: predictor matrix.

`const Series& y`: response vector.

`Config cfg`: output platform/pinning config.

Return -> `Series`: matrix `[n, H]` where each row/horizon uses predictions from fold models covering that row in test windows.

Notes:

- If a row appears in multiple test folds, predictions are averaged.
- Rows not in any test fold are `NaN`.

Example usage:

```cpp
auto cvpred = cv.cvpredict(x, y);
```

Expected output:

```text
cvpred.sizes()[0] == n
cvpred.sizes()[1] == cv.num_steps()
cvpred contains finite values on test-covered rows and NaN elsewhere
```

## `Series DirectCVForecaster::cvloss(const Series& x, const Series& y, CVLossMode mode = CVLossMode::Average, RegressionLoss kind = RegressionLoss::MSE, Config cfg = {}) const`

Computes cross-validated loss.

`const Series& x`: predictor matrix.

`const Series& y`: response vector.

`CVLossMode mode`: loss aggregation mode:

`CVLossMode::Average`: returns `[H]` average over all test-covered rows.

`CVLossMode::Individual`: returns `[F, H]` per-fold loss matrix.

`RegressionLoss kind`: loss family (`RegressionLoss::MSE` or `RegressionLoss::MAE`).

`Config cfg`: output platform/pinning config.

Return -> `Series`: loss values according to `mode`.

Example usage:

```cpp
auto avg_mse = cv.cvloss(x, y, ts::CVLossMode::Average, ts::RegressionLoss::MSE);
auto ind_mse = cv.cvloss(x, y, ts::CVLossMode::Individual, ts::RegressionLoss::MSE);
auto avg_mae = cv.cvloss(x, y, ts::CVLossMode::Average, ts::RegressionLoss::MAE);
```

Expected output:

```text
avg_mse.sizes()[0] == cv.num_steps()
ind_mse.sizes()[0] == cv.num_folds()
ind_mse.sizes()[1] == cv.num_steps()
avg_mae.sizes()[0] == cv.num_steps()
```

## `Series acf(const Series& x, const AcfOptions& options = {}, Config cfg = {})`

Computes sample autocorrelation values at lags `0..max_lag`.

`const Series& x`: input time-series vector (`[n]` or `[n,1]`).

`const AcfOptions& options`: ACF controls.

`Config cfg`: output platform/pinning config (`Float64` output is enforced).

Return -> `Series`: vector `[max_lag+1]` with lag-0 at index `0`.

Notes:

- `options.max_lag` is clipped to `n-1`.
- `options.norm` supports `AcfNorm::Biased` and `AcfNorm::Unbiased`.
- `options.demean` controls whether the series is centered before correlation.

Example usage:

```cpp
ts::AcfOptions aopt;
aopt.max_lag = 20;
aopt.norm = ts::AcfNorm::Biased;
auto a = ts::acf(y, aopt);
```

Expected output:

```text
a.sizes()[0] == 21
a[0] == 1
```

## `Series pacf(const Series& x, const PacfOptions& options = {}, Config cfg = {})`

Computes sample partial autocorrelation values at lags `0..max_lag` via Durbin-Levinson recursion.

`const Series& x`: input time-series vector (`[n]` or `[n,1]`).

`const PacfOptions& options`: PACF controls (`max_lag`, `norm`, `demean`).

`Config cfg`: output platform/pinning config (`Float64` output is enforced).

Return -> `Series`: vector `[max_lag+1]` with lag-0 at index `0`.

Notes:

- Recursive loops are used here for correctness of Durbin-Levinson updates.

Example usage:

```cpp
ts::PacfOptions popt;
popt.max_lag = 20;
auto p = ts::pacf(y, popt);
```

Expected output:

```text
p.sizes()[0] == 21
p[0] == 1
```

## `LjungBoxResult ljung_box(const Series& x, const LjungBoxOptions& options = {}, Config cfg = {})`

Runs Ljung-Box whiteness diagnostics at requested lag cutoffs.

`const Series& x`: input time-series vector (`[n]` or `[n,1]`).

`const LjungBoxOptions& options`: test controls.

`Config cfg`: output platform/pinning config.

Return -> `LjungBoxResult`: struct with:

- `lags`: tested lag cutoffs.
- `stat`: Ljung-Box Q-statistics.
- `pvalue`: upper-tail chi-square p-values.
- `dof`: chi-square degrees of freedom (`max(1, lag - model_df)`).

Notes:

- `options.lags` must be positive; values larger than `n-1` are clipped to `n-1`.
- `model_df` should match the number of fitted dynamic parameters removed from residual diagnostics.

Example usage:

```cpp
ts::LjungBoxOptions lopt;
lopt.lags = {10, 20};
lopt.model_df = 1;
auto lb = ts::ljung_box(y, lopt);
```

Expected output:

```text
lb.lags.sizes()[0] == 2
lb.stat.sizes()[0] == 2
lb.pvalue.sizes()[0] == 2
lb.dof.sizes()[0] == 2
```

## `GarchFit Garch::fit_mle(const Series& y, const GarchOptions& options = {}, Config cfg = {})`

Fits a general `GARCH(m,n)` conditional variance model by Gaussian QMLE.

`const Series& y`: univariate response / return series (`[n]` or `[n,1]`).

`const GarchOptions& options`: typed fit options including:

- `arch_order` (`m`) and `garch_order` (`n`).
- initialization vectors (`arch0`, `garch0`).
- numerical floors/margins and optimizer budget.
- `nm_restarts`: deterministic multi-start count for Nelder-Mead (`>= 1`).

`Config cfg`: execution/output config (`Float64` internal path is enforced).

Return -> `GarchFit`: fitted object with imperative methods.

Notes:

- Supports `GARCH(m,n)` including mixed lag orders.
- Stationarity is enforced through constrained parameterization (`sum(coeffs) < 1 - stationarity_margin`).
- Recursive variance filtering is used (loop is required for correctness).

Example usage:

```cpp
ts::GarchOptions opt;
opt.arch_order = 2;
opt.garch_order = 1;
opt.arch0 = {0.05, 0.05};
opt.garch0 = {0.85};
auto fit = ts::Garch::fit_mle(y, opt);
```

Expected output:

```text
fit.arch.size() == 2
fit.garch.size() == 1
fit.persistence() < 1
```

## `Garch11Fit Garch11::fit_mle(const Series& y, const Garch11Options& options = {}, Config cfg = {})`

Fits a `GARCH(1,1)` conditional variance model by Gaussian QMLE.

`const Series& y`: univariate response / return series (`[n]` or `[n,1]`).

`const Garch11Options& options`: typed fit options (`demean`, initial values, floors/margins, optimizer budget).

`Config cfg`: execution/output config (`Float64` internal path is enforced).

Return -> `Garch11Fit`: fitted object with imperative methods.

Notes:

- Stationarity is enforced through parameterization (`alpha + beta < 1 - stationarity_margin`).
- Recursive variance filtering is used (loop is required for correctness).
- This is a compact convenience wrapper over the general `Garch::fit_mle` path for `(m,n)=(1,1)`.

Example usage:

```cpp
ts::Garch11Options opt;
opt.max_iterations = 1200;
opt.max_evaluations = 12000;
auto fit = ts::Garch11::fit_mle(y, opt);
```

Expected output:

```text
fit.alpha >= 0
fit.beta >= 0
fit.alpha + fit.beta < 1
```

## `double GarchFit::persistence() const`

Returns total persistence (`sum(arch) + sum(garch)`).

Return -> `double`: persistence value.

Notes:

- Stationary fitted models satisfy `persistence() < 1`.

## `double GarchFit::loglik(const Series& y) const`

Returns total log-likelihood for `y` under fitted parameters.

Notes:

- `Garch11Fit` exposes the same method signature for the `(1,1)` convenience object.

`const Series& y`: evaluation series.

Return -> `double`: total log-likelihood.

## `Series GarchFit::loglik_per_obs(const Series& y, Config cfg = {}) const`

Returns per-observation log-likelihood contributions.

`const Series& y`: evaluation series.

`Config cfg`: output config.

Return -> `Series`: vector `[n]`.

## `Series GarchFit::infer(const Series& y, Config cfg = {}) const`

Infers conditional variances along `y`.

`const Series& y`: evaluation series.

`Config cfg`: output config.

Return -> `Series`: positive variance vector `[n]`.

## `Series GarchFit::forecast(std::int64_t horizon, const Series& y, Config cfg = {}) const`

Forecasts future conditional variances.

`std::int64_t horizon`: forecast horizon (`> 0`).

`const Series& y`: observed history used for initialization.

`Config cfg`: output config.

Return -> `Series`: variance forecast vector `[horizon]`.

## `Series GarchFit::simulate(std::int64_t num_observations, Config cfg = {}, std::uint64_t seed = 0) const`

Simulates response paths from fitted `GARCH(1,1)` dynamics.

`std::int64_t num_observations`: output length (`> 0`).

`Config cfg`: output config.

`std::uint64_t seed`: RNG seed.

Return -> `Series`: simulated response vector `[num_observations]`.

## `Series GarchFit::score(const Series& y, Config cfg = {}) const`

Returns numerical score (gradient of `loglik`) w.r.t. all fitted natural parameters.

`const Series& y`: evaluation series.

`Config cfg`: output config.

Return -> `Series`: vector `[k]`, where `k = 1 + arch.size() + garch.size()`.

## `Series GarchFit::observed_inf(const Series& y, Config cfg = {}) const`

Returns numerical observed information matrix (`-Hessian(loglik)`).

`const Series& y`: evaluation series.

`Config cfg`: output config.

Return -> `Series`: matrix `[k,k]`, where `k = 1 + arch.size() + garch.size()`.

## `EgarchFit Egarch::fit_mle(const Series& y, const EgarchOptions& options = {}, Config cfg = {})`

Fits `EGARCH(m,n)` by Gaussian QMLE with log-variance recursion.

`const Series& y`: univariate response / return series (`[n]` or `[n,1]`).

`const EgarchOptions& options`: typed fit options (`arch_order`, `garch_order`, `arch0`, `asym0`, `garch0`, stability controls, optimizer budget).

`Config cfg`: execution/output config (`Float64` internal path is enforced).

Return -> `EgarchFit`: fitted object exposing `infer`, `loglik`, `loglik_per_obs`, `score`, `observed_inf`, `forecast`, and `simulate`.

Notes:

- Uses log-variance recursion; loops are required for state recursion correctness.
- EGARCH equation differs from standard GARCH: update is on `log(h_t)` with magnitude and asymmetry terms.
- Stationarity guard is applied through `sum(garch) < 1 - stationarity_margin`.

## `GjrFit Gjr::fit_mle(const Series& y, const GjrOptions& options = {}, Config cfg = {})`

Fits `GJR(m,n)` by Gaussian QMLE with leverage asymmetry.

`const Series& y`: univariate response / return series (`[n]` or `[n,1]`).

`const GjrOptions& options`: typed fit options (`arch_order`, `garch_order`, `arch0`, `asym0`, `garch0`, stability controls, optimizer budget).

`Config cfg`: execution/output config (`Float64` internal path is enforced).

Return -> `GjrFit`: fitted object exposing `infer`, `loglik`, `loglik_per_obs`, `score`, `observed_inf`, `forecast`, and `simulate`.

Notes:

- GJR equation differs from standard GARCH: includes a negative-shock indicator asymmetry term.
- Persistence is `sum(arch) + 0.5 * sum(asym) + sum(garch)`.
- `unconditional_var()` returns `omega / (1 - persistence)` for stationary fits.

## `MvGarchFit MvGarch::fit_mle(const Series& y, const MvGarchOptions& options = {}, Config cfg = {})`

Fits multivariate conditional variance using column-wise `GARCH(m,n)` marginals and a constant conditional correlation matrix.

`const Series& y`: response matrix (`[n,k]`) or vector (`[n]` treated as `[n,1]`).

`const MvGarchOptions& options`: typed controls:

- `marginal`: univariate `GarchOptions` applied per column.
- `corr_shrinkage`: optional diagonal shrinkage in `[0,1]`.
- `corr_jitter`: stabilization jitter used if correlation is near-singular.

`Config cfg`: execution/output config (`Float64` internal path is enforced, platform follows input/config).

Return -> `MvGarchFit`: fitted object with:

- `marginals`: per-column univariate fits.
- `mean`: unconditional mean vector `[k]`.
- `corr`: estimated constant correlation matrix `[k,k]`.

Notes:

- This is a CCC-style multivariate volatility model (not DCC/BEKK).
- `infer(y)` returns conditional variance matrix `[n,k]`.
- `loglik_per_obs(y)` returns vector `[n]` from multivariate Gaussian likelihood using `corr`.

## `MvEgarchFit MvEgarch::fit_mle(const Series& y, const MvEgarchOptions& options = {}, Config cfg = {})`

Fits multivariate conditional variance using column-wise `EGARCH(m,n)` marginals and constant conditional correlation.

`const Series& y`: response matrix (`[n,k]`) or vector (`[n]` treated as `[n,1]`).

`const MvEgarchOptions& options`: typed controls (`marginal`, `corr_shrinkage`, `corr_jitter`).

`Config cfg`: execution/output config.

Return -> `MvEgarchFit`: fitted object with `marginals`, `mean`, and `corr`.

Notes:

- EGARCH recursion remains per-column and loop-based by necessity.
- Correlation estimation and multivariate log-likelihood are shared with `MvGarch`.

## `MvGjrFit MvGjr::fit_mle(const Series& y, const MvGjrOptions& options = {}, Config cfg = {})`

Fits multivariate conditional variance using column-wise `GJR(m,n)` marginals and constant conditional correlation.

`const Series& y`: response matrix (`[n,k]`) or vector (`[n]` treated as `[n,1]`).

`const MvGjrOptions& options`: typed controls (`marginal`, `corr_shrinkage`, `corr_jitter`).

`Config cfg`: execution/output config.

Return -> `MvGjrFit`: fitted object with `marginals`, `mean`, and `corr`.

Notes:

- GJR asymmetry remains in each marginal recursion.
- Correlation and likelihood aggregation are multivariate Gaussian with CCC correlation.

## `Series MvGarchFit::infer(const Series& y, Config cfg = {}) const`

Infers per-series conditional variances.

`const Series& y`: matrix `[n,k]` (or vector treated as `[n,1]`).

`Config cfg`: output config.

Return -> `Series`: variance matrix `[n,k]`.

## `double MvGarchFit::loglik(const Series& y) const`

Returns total multivariate log-likelihood.

`const Series& y`: matrix `[n,k]`.

Return -> `double`: total log-likelihood across all observations.

## `Series MvGarchFit::loglik_per_obs(const Series& y, Config cfg = {}) const`

Returns per-observation multivariate log-likelihood contributions.

`const Series& y`: matrix `[n,k]`.

`Config cfg`: output config.

Return -> `Series`: vector `[n]`.

## `Series MvGarchFit::forecast(std::int64_t horizon, const Series& y, Config cfg = {}) const`

Forecasts conditional variances horizon-ahead for each series.

`std::int64_t horizon`: forecast horizon (`> 0`).

`const Series& y`: history matrix `[n,k]`.

`Config cfg`: output config.

Return -> `Series`: forecast matrix `[horizon,k]`.

Notes:

- `MvEgarchFit` and `MvGjrFit` expose the same method signatures:
  - `infer`, `loglik`, `loglik_per_obs`, `forecast`.

## `ArimaFit Arima::fit_mle(const Series& y, const ArimaOptions& options = {}, Config cfg = {})`

Fits typed ARIMA/SARIMA dynamics via Gaussian MLE.

`const Series& y`: univariate series (`[n]` or `[n,1]`).

`const ArimaOptions& options`: typed orders and controls:

- non-seasonal: `ar_order`, `diff_order`, `ma_order`.
- seasonal: `seasonal_ar_order`, `seasonal_diff_order`, `seasonal_ma_order`, `seasonal_period`.
- optional regression: `beta0` (ARIMAX initial coefficients).
- optimizer and numeric controls (`variance_floor`, `stationarity_margin`, `finite_diff_eps`, iterations/evaluations).

`Config cfg`: execution/output config (`Float64` internal path is enforced).

Return -> `ArimaFit`: fitted object exposing:

- `infer`: innovations path.
- `loglik`: total log-likelihood.
- `loglik_per_obs`: per-observation contributions (warmup/differencing entries are `NaN`).
- `score`: numerical gradient.
- `observed_inf`: numerical observed information.
- `forecast`: horizon forecasts on original level scale.
- `simulate`: simulated path on original level scale.

Notes:

- Recursive ARMA state updates are loop-based by necessity.
- Differencing and inverse-differencing are handled internally for forecast/simulate.

## `ArimaFit Arima::fit_mle(const Series& y, const Series& x, const ArimaOptions& options = {}, Config cfg = {})`

Fits ARIMAX/SARIMAX-style regression-with-ARIMA-errors model.

`const Series& y`: univariate response (`[n]` or `[n,1]`).

`const Series& x`: exogenous predictor matrix (`[n,k]` or vector treated as `[n,1]`).

`const ArimaOptions& options`: same as above; `beta0` is used as the exogenous coefficient initializer.

`Config cfg`: execution/output config.

Return -> `ArimaFit`: same object, with fitted `beta` coefficients populated.

Notes:

- When `beta` is non-empty, use exogenous-aware methods:
  - `loglik(y, x)`, `loglik_per_obs(y, x)`, `infer(y, x)`, `score(y, x)`, `observed_inf(y, x)`.
  - `forecast(h, y, x_hist, x_future)`.
  - `simulate(x)`.
- Non-exogenous method variants throw if `beta` is non-empty.

## `SarimaFit Sarima::fit_mle(const Series& y, const SarimaOptions& options = {}, Config cfg = {})`

Convenience typed SARIMA wrapper over `Arima::fit_mle`.

`const Series& y`: univariate series (`[n]` or `[n,1]`).

`const SarimaOptions& options`: compact seasonal notation:

- non-seasonal `(p,d,q)`.
- seasonal `(P,D,Q,s)`.

`Config cfg`: execution/output config.

Return -> `SarimaFit`: wrapper containing `core` (`ArimaFit`) and forwarding the same imperative methods.

## `SarimaFit Sarima::fit_mle(const Series& y, const Series& x, const SarimaOptions& options = {}, Config cfg = {})`

Fits seasonal ARIMAX model with compact `(p,d,q) x (P,D,Q,s)` notation and exogenous predictors.

`const Series& y`: univariate response.

`const Series& x`: exogenous predictor matrix (`[n,k]`).

`const SarimaOptions& options`: seasonal options plus `beta0`.

`Config cfg`: execution/output config.

Return -> `SarimaFit`: wrapper whose `core.beta` contains fitted exogenous coefficients.

Example usage:

```cpp
ts::SarimaOptions opt;
opt.p = 1;
opt.d = 1;
opt.q = 1;
opt.P = 1;
opt.D = 0;
opt.Q = 1;
opt.s = 12;
auto fit = ts::Sarima::fit_mle(y, opt);
```

Expected output:

```text
fit.core.ar.size() == 1
fit.core.sar.size() == 1
fit.forecast(12, y).sizes()[0] == 12
```

## `VarFit Var::fit_mle(const Series& y, const VarOptions& options = {}, Config cfg = {})`

Fits a multivariate `VAR(p)` model by closed-form least squares.

`const Series& y`: response matrix (`[n,k]`) or vector (`[n]` treated as `[n,1]`).

`const VarOptions& options`: typed controls (`lag_order`, `include_intercept`, `include_trend`, `ridge`, `pinv_rcond`).

`Config cfg`: execution/output config (`Float64` internal path is enforced).

Return -> `VarFit`: fitted object exposing `infer`, `loglik`, `loglik_per_obs`, `forecast`, and `simulate`.

Notes:

- This is a reduced-form VAR with row-wise recursion semantics.
- `ar` is stored with shape `[p,k,k]`.
- Residual covariance is estimated as `(E^T E)/T` and numerically stabilized for likelihood evaluation.

## `VarFit Var::fit_mle(const Series& y, const Series& x, const VarOptions& options = {}, Config cfg = {})`

Fits a multivariate `VARX(p)` model.

`const Series& y`: response matrix (`[n,k]`).

`const Series& x`: predictor matrix (`[n,m]`).

`const VarOptions& options`: same typed controls as above.

`Config cfg`: execution/output config.

Return -> `VarFit`: fitted object with `beta` coefficients shape `[m,k]`.

Notes:

- Exogenous terms are contemporaneous in the fitted equation.
- Exogenous-aware methods must be used when `beta` is non-empty.

## `Series VarFit::infer(const Series& y, Config cfg = {}) const`

Infers reduced-form innovations (residuals) for a fitted `VAR`.

`const Series& y`: response matrix.

`Config cfg`: output config.

Return -> `Series`: residual matrix `[n - p, k]`.

## `Series VarFit::infer(const Series& y, const Series& x, Config cfg = {}) const`

Infers residuals for fitted `VARX`.

`const Series& y`: response matrix.

`const Series& x`: predictor matrix.

`Config cfg`: output config.

Return -> `Series`: residual matrix `[n - p, k]`.

## `double VarFit::loglik(const Series& y) const`

Returns total Gaussian log-likelihood under fitted `VAR` parameters.

`const Series& y`: response matrix.

Return -> `double`: total log-likelihood.

## `double VarFit::loglik(const Series& y, const Series& x) const`

Returns total Gaussian log-likelihood under fitted `VARX` parameters.

`const Series& y`: response matrix.

`const Series& x`: predictor matrix.

Return -> `double`: total log-likelihood.

## `Series VarFit::loglik_per_obs(const Series& y, Config cfg = {}) const`

Returns per-observation Gaussian log-likelihood contributions for `VAR`.

`const Series& y`: response matrix.

`Config cfg`: output config.

Return -> `Series`: vector `[n - p]`.

## `Series VarFit::loglik_per_obs(const Series& y, const Series& x, Config cfg = {}) const`

Returns per-observation Gaussian log-likelihood contributions for `VARX`.

`const Series& y`: response matrix.

`const Series& x`: predictor matrix.

`Config cfg`: output config.

Return -> `Series`: vector `[n - p]`.

## `Series VarFit::forecast(std::int64_t horizon, const Series& y, Config cfg = {}) const`

Forecasts `horizon` periods ahead from response history for fitted `VAR`.

`std::int64_t horizon`: forecast horizon (`> 0`).

`const Series& y`: response history (`rows >= p`).

`Config cfg`: output config.

Return -> `Series`: forecast matrix `[horizon, k]`.

## `Series VarFit::forecast(std::int64_t horizon, const Series& y, const Series& x_future, Config cfg = {}) const`

Forecasts `VARX` responses using future exogenous predictors.

`std::int64_t horizon`: forecast horizon (`> 0`).

`const Series& y`: response history (`rows >= p`).

`const Series& x_future`: future predictors (`[horizon, m]` or more rows).

`Config cfg`: output config.

Return -> `Series`: forecast matrix `[horizon, k]`.

## `Series VarFit::simulate(std::int64_t num_observations, Config cfg = {}, std::uint64_t seed = 0) const`

Simulates responses from fitted `VAR` with Gaussian innovations.

`std::int64_t num_observations`: simulated length (`> 0`).

`Config cfg`: output config.

`std::uint64_t seed`: RNG seed.

Return -> `Series`: simulated matrix `[num_observations, k]`.

## `Series VarFit::simulate(const Series& x, Config cfg = {}, std::uint64_t seed = 0) const`

Simulates responses from fitted `VARX` with provided exogenous path.

`const Series& x`: exogenous path (`[n,m]`).

`Config cfg`: output config.

`std::uint64_t seed`: RNG seed.

Return -> `Series`: simulated matrix `[n, k]`.

## `std::pair<Series,Series> var_to_vecm(const Series& ar, Config cfg = {})`

Converts `VAR` lag tensor to `VEC` short-run and impact representation.

`const Series& ar`: lag tensor `[p,k,k]`.

`Config cfg`: output config.

Return -> `std::pair<Series,Series>`:

- first: `short_run` tensor `[p-1,k,k]`.
- second: `impact` matrix `[k,k]`.

## `Series vecm_to_var(const Series& short_run, const Series& impact, Config cfg = {})`

Converts `VEC` representation back to `VAR` lag tensor.

`const Series& short_run`: tensor `[q,k,k]` where `q = p-1`.

`const Series& impact`: impact matrix `[k,k]`.

`Config cfg`: output config.

Return -> `Series`: lag tensor `[p,k,k]`.

Example usage:

```cpp
ts::VarOptions opt;
opt.lag_order = 2;
auto fit = ts::Var::fit_mle(y, opt);
auto ll = fit.loglik(y);
auto f = fit.forecast(12, y);
auto vec = ts::var_to_vecm(fit.ar);
auto ar_back = ts::vecm_to_var(vec.first, vec.second);
```

Expected output:

```text
f.sizes() == [12, k]
fit.infer(y).sizes() == [n - p, k]
ar_back has shape [p, k, k]
```

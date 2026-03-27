# PlussMa Time-Series User Guide

This guide is task-first and practical. It covers `pluss::subsp::stats::ts` (also available through `pluss::ma::ts`) with multiple usage styles for each capability.

## 1) Includes And Namespaces

```cpp
#include <pluss/ma.h>                       // easiest umbrella
// or narrower:
#include <pluss/ma/stats/time_series.h>

namespace ts = pluss::ma::ts;               // alias to pluss::subsp::stats::ts
```

Recommended config baseline for deterministic CPU examples:

```cpp
auto cfg = pluss::Config{}
  .with_dtype(pluss::ScalarType::Float64)
  .with_platform(pluss::pCPU);
```

Help note:
- Use `Float64` for diagnostics/estimation by default unless you have a measured reason to reduce precision.

## 2) Core Transformations (Returns, Lags, Rolling, EWM)

### 2.1 Return transforms

```cpp
auto r1 = ts::returns(px);             // period=1
auto r5 = ts::returns(px, 5);          // period=5

auto lr1 = ts::log_returns(px);        // period=1
auto lr10 = ts::log_returns(px, 10);
```

### 2.2 Difference/lag/lead

```cpp
auto d1 = ts::diff(y);                 // first difference
auto d12 = ts::diff(y, 12);            // seasonal difference-like use

auto l1 = ts::lag(y, 1);
auto l3 = ts::lag(y, 3);
auto f1 = ts::lead(y, 1);
```

### 2.3 Rolling statistics

```cpp
auto m = ts::rolling_mean(y, 20, 5);
auto s = ts::rolling_std(y, 20, 5);
auto v = ts::rolling_var(y, 20, 5);
auto mn = ts::rolling_min(y, 20, 5);
auto mx = ts::rolling_max(y, 20, 5);
auto sm = ts::rolling_sum(y, 20, 5);
```

### 2.4 Exponential/expanding

```cpp
auto ew_m = ts::ewm_mean(y, 0.08);
auto ew_s = ts::ewm_std(y, 0.08, 1e-12);
auto e = ts::ema(y, 0.08);             // alias-style mean smoothing

auto ex_m = ts::expanding_mean(y, 20);
auto ex_s = ts::expanding_std(y, 20, 1e-12);

auto z = ts::zscore_rolling(y, 30, 10, 1e-12);
```

Help notes:
- `min_periods` controls warmup stability.
- For very noisy or sparse data, prefer larger windows and a nonzero `epsilon`.

## 3) Gap Filling And Resampling

### 3.1 Gap filling modes

```cpp
auto ff = ts::fill_gaps(y, ts::GapFillMode::Forward);
auto bf = ts::fill_gaps(y, ts::GapFillMode::Backward);
auto zz = ts::fill_gaps(y, ts::GapFillMode::Zero);
auto cc = ts::fill_gaps(y, ts::GapFillMode::Constant, 0.25);
```

### 3.2 Resampling with label output

```cpp
auto out_mean = ts::resample(time_index, y, "1D", ts::ResampleOp::Mean, 1);
auto out_sum  = ts::resample(time_index, y, "1W", ts::ResampleOp::Sum, 1);
auto out_last = ts::resample(time_index, y, "1M", ts::ResampleOp::Last, 1);

if (out_mean.labels.has_value()) {
  std::cout << out_mean.labels.value() << "\n";
}
std::cout << out_mean.values << "\n";
```

Help note:
- `ResampleResult` carries both values and optional timeline labels. Keep both if you plan to plot.

## 4) Time-Series Partitioning (Backtests)

### 4.1 Expanding window

```cpp
ts::PartitionOptions po;
po.direction = ts::Direction::Reverse;
po.min_train_size = 200;
po.test_size = 40;
po.step_size = 20;
po.gap_size = 2;
po.max_lag = 12;

auto p = ts::TsPartition::expanding_window(1200, 6, po);
```

### 4.2 Sliding window

```cpp
ts::PartitionOptions po;
po.direction = ts::Direction::Forward;
po.train_size = 300;
po.test_size = 60;
po.step_size = 30;

auto p = ts::TsPartition::sliding_window(1200, 6, po);
```

### 4.3 Holdout

```cpp
ts::PartitionOptions po;
po.max_lag = 8;
po.gap_size = 1;

auto p1 = ts::TsPartition::holdout(1000, 0.2, po); // fraction
auto p2 = ts::TsPartition::holdout(1000, 200, po); // explicit count
```

### 4.4 Index and mask extraction

```cpp
auto train_idx = p.training(0, cfg);
auto test_idx  = p.test(0, cfg);
auto buf_idx   = p.test_buffer(0, cfg);

auto train_mask = p.training_mask(0, cfg);
auto test_mask  = p.test_mask(0, cfg);
auto buf_mask   = p.test_buffer_mask(0, cfg);
```

Help notes:
- Use `gap_size` + `max_lag` together to reduce leakage when lag features are present.
- `Direction::Forward` is often easier for walk-forward explainability.

## 5) Lag Plan + Lagged Design Matrix

### 5.1 Build plan

```cpp
ts::LagPlanOptions lp;
lp.horizon = {1, 2, 4};
lp.response_lags = {1, 2, 3};
lp.leading_predictors = {2};
lp.predictor_lags = {{1, 2}, {1}};
lp.leading_predictor_lags = {{0, 1}};

auto plan = ts::build_lag_plan(/*num_predictors=*/3, lp);
```

### 5.2 Build lagged predictors

```cpp
auto design = ts::build_lagged_predictors(X, y, plan, cfg);
std::cout << design << "\n";
```

Help notes:
- `leading_predictor_lags` may include `0`.
- Boundary rows will contain `NaN` where shifted data is out of range.

## 6) Direct Multi-Horizon Forecasting

### 6.1 Fit with OLS

```cpp
ts::DirectForecasterOptions opt;
opt.lag.horizon = {1, 2, 3};
opt.lag.response_lags = {1, 2};
opt.lag.predictor_lags = {{1, 2}};
opt.learner = ts::DirectLearnerKind::OLS;

auto m = ts::DirectForecaster::fit(X, y, opt, cfg);
```

### 6.2 Fit with Ridge

```cpp
ts::DirectForecasterOptions opt;
opt.lag.horizon = {1, 3, 6};
opt.lag.response_lags = {1, 2, 12};
opt.learner = ts::DirectLearnerKind::Ridge;
opt.ridge_lambda = 1e-4;
opt.fit_intercept = true;

auto m = ts::DirectForecaster::fit(X, y, opt, cfg);
```

### 6.3 Predict / prepared predictors / forecast / loss

```cpp
auto pred = m.predict(X, y, cfg);
auto prep_h2 = m.prepared_predictors(X, y, 2, cfg);
auto fcast = m.forecast(X, y, cfg);

auto mse = m.loss(X, y, ts::RegressionLoss::MSE, cfg);
auto mae = m.loss(X, y, ts::RegressionLoss::MAE, cfg);

const auto& step0 = m.step(0);
std::cout << step0.horizon << "\n";
std::cout << step0.beta << "\n";
```

Help note:
- `prepared_predictors(..., horizon_step)` is ideal for auditing model inputs at each horizon.

## 7) Cross-Validated Direct Forecasting

```cpp
ts::PartitionOptions po;
po.direction = ts::Direction::Forward;
po.train_size = 700;
po.test_size = 420;
po.step_size = 210;
po.max_lag = 8;
auto part = ts::TsPartition::sliding_window(y.sizes()[0], 4, po);

ts::DirectForecasterOptions opt;
opt.lag.horizon = {1, 2, 4};
opt.lag.response_lags = {1, 2, 3};
opt.lag.predictor_lags = {{1, 2}};
opt.learner = ts::DirectLearnerKind::Ridge;
opt.ridge_lambda = 1e-4;

auto cv = ts::DirectCVForecaster::fit(X, y, part, opt, cfg);
auto cvpred = cv.cvpredict(X, y, cfg);

auto avg_mse = cv.cvloss(X, y, ts::CVLossMode::Average, ts::RegressionLoss::MSE, cfg);
auto ind_mse = cv.cvloss(X, y, ts::CVLossMode::Individual, ts::RegressionLoss::MSE, cfg);
auto avg_mae = cv.cvloss(X, y, ts::CVLossMode::Average, ts::RegressionLoss::MAE, cfg);
```

Help note:
- `CVLossMode::Individual` is best for fold-level stability diagnostics.

## 8) Correlation And Diagnostic Tests

### 8.1 ACF / PACF / Ljung-Box

```cpp
ts::AcfOptions ao{.max_lag = 24, .norm = ts::AcfNorm::Biased, .demean = true};
ts::PacfOptions po{.max_lag = 24, .norm = ts::AcfNorm::Biased, .demean = true};
ts::LjungBoxOptions lo{.lags = {6, 12, 24}, .model_df = 1};

auto a = ts::acf(y, ao, cfg);
auto p = ts::pacf(y, po, cfg);
auto lb = ts::ljung_box(y, lo, cfg);
```

### 8.2 ARCH-LM / ADF / KPSS / DM

```cpp
auto arch = ts::arch_lm(y, ts::ArchLmOptions{.lags = 8, .demean = true, .alpha = 0.05}, cfg);
auto adf  = ts::adf_test(y, ts::AdfOptions{.lags = 1, .model = ts::AdfModel::ARD, .alpha = 0.05}, cfg);
auto kpss = ts::kpss_test(y, ts::KpssOptions{.lags = 8, .trend = false, .alpha = 0.05}, cfg);

ts::DmOptions dm_opt{};
dm_opt.horizon = 1;
dm_opt.loss = ts::DmLoss::Squared;
dm_opt.tail = ts::DmTail::Both;
dm_opt.alpha = 0.05;
dm_opt.hln_adjust = true;
auto dm = ts::dm_test(err_model_a, err_model_b, dm_opt, cfg);
```

Help notes:
- Use ADF + KPSS together for stationarity triangulation.
- `model_df` in Ljung-Box should reflect dynamic parameters already fitted.
- DM compares forecast error quality directly; align `horizon` with your forecast setup.

## 9) Plotting APIs (PlussPl Studio)

### 9.1 Payload-only mode

```cpp
ts::plot::CorrPlotOptions copts;
copts.max_lag = 30;
copts.norm = ts::AcfNorm::Unbiased;

auto acf_payload = ts::plot::acf_payload(y, copts, cfg);
auto pacf_payload = ts::plot::pacf_payload(y, copts, cfg);
auto resid_payload = ts::plot::residuals_payload(residuals, cfg);
```

### 9.2 Draw directly into Studio

```cpp
pluss::pl::Studio acf_fig;
pluss::pl::Studio pacf_fig;
pluss::pl::Studio resid_fig;

ts::plot::acf(acf_fig, y, copts, cfg);
ts::plot::pacf(pacf_fig, y, copts, cfg);
ts::plot::residuals(resid_fig, residuals, ts::plot::ResidualPlotOptions{}, cfg);
```

## 10) Volatility Models

### 10.1 GARCH(1,1)

```cpp
ts::Garch11Options opt;
opt.alpha0 = 0.08;
opt.beta0 = 0.90;
auto fit = ts::Garch11::fit_mle(y, opt, cfg);

auto sigma2 = fit.infer(y, cfg);
auto ll = fit.loglik(y);
auto ll_i = fit.loglik_per_obs(y, cfg);
auto f = fit.forecast(12, y, cfg);
auto sim = fit.simulate(500, cfg, 20260401ULL);
```

### 10.2 GARCH(p,q), EGARCH, GJR

```cpp
auto gfit = ts::Garch::fit_mle(y, ts::GarchOptions{.arch_order = 2, .garch_order = 2}, cfg);
auto efit = ts::Egarch::fit_mle(y, ts::EgarchOptions{.arch_order = 1, .garch_order = 1}, cfg);
auto jfit = ts::Gjr::fit_mle(y, ts::GjrOptions{.arch_order = 1, .garch_order = 1}, cfg);
```

### 10.3 Multivariate families

```cpp
auto mg = ts::MvGarch::fit_mle(Y, ts::MvGarchOptions{}, cfg);
auto me = ts::MvEgarch::fit_mle(Y, ts::MvEgarchOptions{}, cfg);
auto mj = ts::MvGjr::fit_mle(Y, ts::MvGjrOptions{}, cfg);

auto mg_h = mg.forecast(10, Y, cfg);
auto me_h = me.forecast(10, Y, cfg);
auto mj_h = mj.forecast(10, Y, cfg);
```

Help note:
- Check persistence/stationarity settings carefully before long-horizon simulation.

## 11) ARIMA / SARIMA / ARIMAX

### 11.1 ARIMA (univariate)

```cpp
ts::ArimaOptions aopt;
aopt.ar_order = 1;
aopt.ma_order = 1;
aopt.max_iterations = 1200;

auto afit = ts::Arima::fit_mle(y, aopt, cfg);
auto pred = afit.infer(y, cfg);
auto fc = afit.forecast(12, y, cfg);
```

### 11.2 ARIMAX (with regressors)

```cpp
ts::ArimaOptions xopt;
xopt.ar_order = 1;
xopt.beta0 = {0.0, 0.0};

auto ax = ts::Arima::fit_mle(y, X, xopt, cfg);
auto pred = ax.infer(y, X, cfg);
auto fc = ax.forecast(12, y, X_hist, X_future, cfg);
```

### 11.3 SARIMA wrappers

```cpp
ts::SarimaOptions sopt;
sopt.p = 1; sopt.P = 1; sopt.Q = 1; sopt.s = 12;

auto sfit = ts::Sarima::fit_mle(y, sopt, cfg);
auto spred = sfit.infer(y, cfg);
```

Help notes:
- Use SARIMA only when seasonal structure is real and stable.
- Prefer residual diagnostics (`acf/pacf/ljung_box`) after fit.

## 12) State Space Models (SSM)

```cpp
ts::SsmModel m;
m.transition = A;
m.state_loading = B;
m.observation = C;
m.innovation_loading = D;
m.mean0 = mu0;
m.cov0 = P0;

// filtering/smoothing
auto fr = ts::Ssm::filter(y, m, ts::SsmOptions{}, cfg);
auto sr = ts::Ssm::smooth(y, m, ts::SsmOptions{}, cfg);

// forecasting
auto f1 = ts::Ssm::forecast(20, m, cfg);
auto f2 = ts::Ssm::forecast(20, y, m, ts::SsmOptions{}, cfg);

// simulation
auto sim = ts::Ssm::simulate(250, m, 10, cfg, 20260402ULL);
```

Help note:
- Validate matrix dimensions first (`A,B,C,D,mu0,P0`) before interpreting statistical outputs.

## 13) VAR / Johansen / VECM

### 13.1 VAR fit and structural diagnostics

```cpp
ts::VarOptions vopt;
vopt.lag_order = 2;
vopt.include_intercept = true;

auto vf = ts::Var::fit_mle(Y, vopt, cfg);
auto vp = vf.infer(Y, cfg);
auto vfc = vf.forecast(12, Y, cfg);
auto irf = vf.irf(ts::IrfOptions{.horizon = 12, .method = ts::IrfMethod::Orthogonalized}, cfg);
auto fevd = vf.fevd(ts::FevdOptions{.horizon = 12, .method = ts::IrfMethod::Orthogonalized}, cfg);

ts::GrangerOptions gopt;
gopt.type = ts::GrangerType::LeaveOneOut;
gopt.stat = ts::GrangerStat::ChiSquare;
auto g = vf.granger_test(Y, gopt, cfg);
```

### 13.2 Johansen cointegration + VECM

```cpp
auto joh = ts::johansen_test(Y, ts::JohansenOptions{.lag_order = 2, .statistic = ts::JohansenStat::Trace}, cfg);

ts::VecmOptions eopt;
eopt.lag_order = 2;
eopt.rank = joh.selected_rank;

auto vecm = ts::Vecm::fit_mle(Y, eopt, cfg);
auto epred = vecm.infer(Y, cfg);
auto efc = vecm.forecast(8, Y, cfg);
```

### 13.3 VAR <-> VECM conversion helpers

```cpp
auto [short_run, impact] = ts::var_to_vecm(vf.ar, cfg);
auto ar_back = ts::vecm_to_var(short_run, impact, cfg);
```

## 14) Practical Patterns (Recommended)

1. Transform -> diagnose -> fit -> diagnose residuals -> backtest.
2. For production backtests, always use `TsPartition` with explicit `gap_size` and `max_lag`.
3. Keep train/eval metric definitions fixed (MSE vs MAE) before model comparison.
4. Save seeds for reproducibility in every `simulate(...)` call.

## 15) Common Errors And Fixes

1. Dimension mismatch in multivariate models:
- Ensure `Y` is `[T, k]` and all option vectors match model orders.

2. Unstable likelihood optimization:
- Increase `max_iterations/max_evaluations`, tune initial values (`*_0`), and keep `Float64`.

3. NaN-heavy lagged design:
- Check horizon/lag choices and whether effective sample size remains sufficient.

4. Misleading stationarity conclusions:
- Combine ADF and KPSS; inspect residual ACF/PACF and Ljung-Box.

## 16) Where To Find Runnable Raw Demos

- `PlussMa/Stats/time_series/examples/ts_lag_features_raw_demo.cpp`
- `PlussMa/Stats/time_series/examples/ts_partition_raw_demo.cpp`
- `PlussMa/Stats/time_series/examples/ts_direct_forecaster_raw_demo.cpp`
- `PlussMa/Stats/time_series/examples/ts_direct_cv_raw_demo.cpp`
- `PlussMa/Stats/time_series/examples/ts_diagnostics_raw_demo.cpp`
- `PlussMa/Stats/time_series/examples/ts_plotting_raw_demo.cpp`
- `PlussMa/Stats/time_series/examples/ts_garch11_raw_demo.cpp`
- `PlussMa/Stats/time_series/examples/ts_garchpq_raw_demo.cpp`
- `PlussMa/Stats/time_series/examples/ts_egarch_gjr_raw_demo.cpp`
- `PlussMa/Stats/time_series/examples/ts_mv_vol_raw_demo.cpp`
- `PlussMa/Stats/time_series/examples/ts_arima_sarima_raw_demo.cpp`
- `PlussMa/Stats/time_series/examples/ts_arimax_raw_demo.cpp`
- `PlussMa/Stats/time_series/examples/ts_ssm_raw_demo.cpp`
- `PlussMa/Stats/time_series/examples/ts_var_raw_demo.cpp`
- `PlussMa/Stats/time_series/examples/ts_vecm_raw_demo.cpp`

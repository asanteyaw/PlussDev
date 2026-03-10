# PlussMa Time Series Implementation Blueprint

This document is the implementation roadmap for `PlussMa/Stats/time_series`.
It is inspired by behavior in MATLAB `toolbox/stats/timeseries` and related workflows, while staying native to Pluss architecture and style.

## Engineering Doctrine

1. Loops (and nested loops) are used only when required for correctness (for example recursive state models).
2. Prefer linear passes and vectorized `Series`/`Note`/`Timeline` operations.
3. Prefer C++20 APIs and STL algorithms/iterators when they improve clarity.
4. No stringly-typed dispatch for model families/options where typed enums are possible.
5. Avoid alias and overload bloat.
6. Keep modules small and composable; add files freely to preserve maintainability.
7. Keep reading relevant MATLAB `stats` sources during implementation to validate behavior, not to copy code.

## Current Baseline (as of 2026-03-09)

1. Time-series scaffolding exists but core files were empty.
2. `time_series/CMakeLists.txt` path wiring was mismatched and is now fixed to real folders.
3. First production slice added:
   - Public API: `pluss/subsp/stat/ts.h`
   - Core object: `ts::TsPartition` with:
     - `ExpandingWindow`, `SlidingWindow`, `Holdout`
     - `Direction::{Forward, Reverse}`
     - `GapSize`, `MaxLag`, train/test/step controls
     - index and mask outputs (`training`, `test`, `test_buffer`, plus mask variants)

## Scope Mapping From MATLAB `stats/timeseries`

Primary scope from MATLAB `stats/timeseries`:

1. `tspartition`:
   - partition construction
   - fold indexing (`training`, `test`)
   - lag-safe buffer handling
2. `directforecaster` family:
   - direct multi-step forecasting (one model per horizon step)
   - lagged feature preparation with response/predictor/leading lags
   - compact model form
   - partitioned cross-validation model (`cvpredict`, `cvloss`)

Out-of-scope for `stats/timeseries` but important for this module:

1. ARIMA/GARCH/state-space style models (more aligned with econometrics lane).
2. These should still live in this module, but as separate subpackages.

## Target Module Structure

Recommended scalable structure:

1. `ts/core/`
   - partitioning
   - lag/feature preparation primitives
   - validation helpers
2. `ts/forecasting/direct/`
   - `DirectForecaster`
   - `CompactDirectForecaster`
   - `PartitionedDirectForecaster`
3. `ts/modules/arima/`
   - ARIMA-family objects
4. `specialized/vol_models/`
   - GARCH-family objects
5. `ts/modules/plotting/`
   - ACF/PACF and residual diagnostics helpers
6. `ts/modules/diagnostics/`
   - loss, rolling diagnostics, backtest metrics

## Public API Direction (Series-First)

1. Public types use `pluss::Series` and typed enums.
2. No string-dispatch API for model kind/partition direction.
3. Public API should remain explicit:
   - `ts::TsPartition::expanding_window(...)`
   - `ts::TsPartition::sliding_window(...)`
   - `ts::TsPartition::holdout(...)`

## Phased Roadmap

## Phase 0: Build + Scaffolding

Completed:

1. Fix CMake wiring for actual folder locations.
2. Introduce modular `ts/CMakeLists.txt` and `ts/core/CMakeLists.txt`.
3. Add public `ts.h` under PlussMa include path.

## Phase 1: Partitioning Core

Completed initial slice:

1. `TsPartition` with fold window computation.
2. Forward/reverse window support.
3. Gap and lag-buffer support.
4. Index and boolean mask extraction per fold.

Follow-ups:

1. Add stricter parity tests against expected edge behavior.
2. Add optional fraction-based size config for expanding/sliding.

## Phase 2: Lag/Feature Preparation Engine

Implement:

1. Response lag matrix builder.
2. Exogenous predictor lag builder.
3. Leading predictor lag builder.
4. Horizon-specific predictor selection map.

Design goals:

1. Reusable by direct forecaster, ARIMA exogenous paths, and diagnostics.
2. Vectorized index assembly where possible.

Status:

1. Implemented core artifacts:
   - `build_lag_plan(...)`
   - `build_lagged_predictors(...)`
   - per-horizon prepared-predictor mask generation
2. Added deterministic unit test coverage for shape/mask/lag alignment behavior.
3. Added Catch2 rigorous tests for:
   - edge cases
   - numerical stability
   - parameter sensitivity
   - stress/crash resilience
4. Added raw demos and API reference:
   - `time_series/examples/ts_partition_raw_demo.cpp`
   - `time_series/examples/ts_lag_features_raw_demo.cpp`
   - `time_series/API_REFERENCE.md`

MATLAB references consulted for this phase:

1. `/Applications/MATLAB_R2025b.app/toolbox/stats/timeseries/+timeseries/+forecaster/DirectForecaster.m`
   - helper logic around `preparePredictorsPerHorizon` and leading-lag horizon transform.
2. `/Applications/MATLAB_R2025b.app/toolbox/stats/timeseries/+timeseries/+forecaster/CompactDirectForecaster.m`
   - helper logic around `prepareLaggedData`, `createLagsOrLeads`, and max-lag interactions.
3. `/Applications/MATLAB_R2025b.app/toolbox/stats/timeseries/+timeseries/+forecaster/+utils/validateXYInputData.m`
   - input/feature requirements for lag and leading data handling.

## Phase 3: Direct Forecaster Objects

Implement:

1. `DirectForecaster` (fit + full state)
2. `CompactDirectForecaster` (deployment form)
3. `PartitionedDirectForecaster` (cross-validated form)

Core methods:

1. `predict`
2. `forecast`
3. `loss`
4. `prepared_predictors`
5. `crossval`
6. `cvpredict`
7. `cvloss`

Status:

1. Implemented first production slice:
   - `DirectForecaster::fit(...)`
   - `DirectForecaster::predict(...)`
   - `DirectForecaster::prepared_predictors(...)`
   - `DirectForecaster::forecast(...)`
   - `DirectForecaster::loss(...)`
   - per-step model access (`step`, `steps`)
2. Added `ts/forecasting/direct` module wiring with:
   - manual tests
   - Catch2 edge/numeric/sensitivity/stress/crash tests
   - raw usage demo
3. Current learner backend for this slice:
   - typed enum `DirectLearnerKind::{OLS,Ridge}`
   - typed loss enum `RegressionLoss::{MSE,MAE}`
   - stable linear solve path via regularized pseudoinverse on `Series`
4. Implemented compact cross-validated object:
   - `DirectCVForecaster::fit(...)`
   - `DirectCVForecaster::cvpredict(...)`
   - `DirectCVForecaster::cvloss(...)`
5. Added deployment-facing methods:
   - `DirectForecaster::prepared_predictors(...)`
   - `DirectForecaster::forecast(...)`
6. Added cross-validation tests and raw demo for fold-level/aggregate behavior.

MATLAB references consulted for this phase:

1. `/Applications/MATLAB_R2025b.app/toolbox/stats/timeseries/+timeseries/+forecaster/DirectForecaster.m`
   - horizon-wise per-model fitting strategy and prepared-predictor mask usage.
2. `/Applications/MATLAB_R2025b.app/toolbox/stats/timeseries/+timeseries/+forecaster/CompactDirectForecaster.m`
   - predict/loss orchestration over horizon-specific predictor subsets.
3. `/Applications/MATLAB_R2025b.app/toolbox/stats/timeseries/+timeseries/+forecaster/PartitionedDirectForecaster.m`
   - next-stage contract for partitioned model behavior (`cvpredict`/`cvloss` path).

## Phase 4: Learner Backend Bridge

Implement thin learner abstraction:

1. start with robust baseline learners (`Linear`, `Ridge`)
2. optionally add tree/ensemble learners if existing backend support is mature

Use PlussQit/PlussMa functionality where appropriate:

1. optimization stack for training objectives
2. autodiff utilities for gradient-based estimation paths
3. existing container and Series utilities for feature tensor ops

## Phase 5: Diagnostics Core (ACF/PACF/Whiteness)

Implement model-agnostic diagnostics:

1. `acf(...)` with typed normalization and demeaning controls.
2. `pacf(...)` via stable Durbin-Levinson recursion.
3. `ljung_box(...)` with typed lag and model-dof controls.

Status:

1. Implemented in `ts/modules/diagnostics/diagnostics.cpp`.
2. Added modular test slice:
   - manual test: `diagnostics_test.cpp`
   - Catch2 rigorous test: `ts_diagnostics_catch2.cpp`
3. Added raw usage demo:
   - `time_series/examples/ts_diagnostics_raw_demo.cpp`
4. Added API reference section:
   - `acf`, `pacf`, and `ljung_box`.

MATLAB references consulted for this phase:

1. `/Applications/MATLAB_R2025b.app/toolbox/stats/stats/+stats/+mcmc/+utils/computeAutoCorr.m`
   - autocorrelation normalization behavior and lag clipping (`numlags <= N-1`).
2. `/Applications/MATLAB_R2025b.app/toolbox/stats/stats/partialcorr.m`
   - partial-correlation stability patterns and residual-based recursion intuition.

## Phase 6: Econometrics Lane (ARIMA/GARCH and related)

Implement object-oriented models with imperative methods:

1. `fit_mle`
2. `infer`
3. `forecast`
4. `simulate`
5. `loglik`
6. `score`
7. `observed_inf`

Use recursion loops where mathematically unavoidable; vectorize all surrounding transformations.

Status (current production slice):

1. Implemented `Garch(m,n)` core in `specialized/vol_models/garch.cpp`:
   - `Garch::fit_mle(...)`
   - `GarchFit::{persistence,unconditional_var,infer,loglik,loglik_per_obs,forecast,simulate,score,observed_inf}`
2. Added `Garch11` convenience wrapper on top of the generic core:
   - `Garch11::fit_mle(...)`
   - `Garch11Fit::{infer,loglik,loglik_per_obs,forecast,simulate,score,observed_inf}`
3. Implemented `Egarch(m,n)` in `specialized/vol_models/egarch.cpp`:
   - `Egarch::fit_mle(...)`
   - `EgarchFit::{persistence,infer,loglik,loglik_per_obs,forecast,simulate,score,observed_inf}`
4. Implemented `Gjr(m,n)` in `specialized/vol_models/gjr.cpp`:
   - `Gjr::fit_mle(...)`
   - `GjrFit::{persistence,unconditional_var,infer,loglik,loglik_per_obs,forecast,simulate,score,observed_inf}`
5. Implemented `Arima`/`Sarima` in `ts/modules/arima/arima.cpp`:
   - `Arima::fit_mle(...)`, `Sarima::fit_mle(...)`
   - `ArimaFit`/`SarimaFit::{infer,loglik,loglik_per_obs,forecast,simulate,score,observed_inf}`
6. Added volatility test slice:
   - manual: `specialized/vol_models/test/garch11_test.cpp`
   - manual: `specialized/vol_models/test/egarch_gjr_test.cpp`
   - Catch2: `specialized/vol_models/test/ts_vol_catch2.cpp`
7. Added ARIMA/SARIMA test slice:
   - manual: `ts/modules/arima/test/arima_test.cpp`
   - Catch2: `ts/modules/arima/test/ts_arima_catch2.cpp`
8. Added raw usage demos:
   - `time_series/examples/ts_garch11_raw_demo.cpp`
   - `time_series/examples/ts_garchpq_raw_demo.cpp`
   - `time_series/examples/ts_egarch_gjr_raw_demo.cpp`
   - `time_series/examples/ts_arima_sarima_raw_demo.cpp`
9. Added API reference coverage for GARCH/EGARCH/GJR/ARIMA/SARIMA signatures and outputs.

MATLAB references consulted for this phase:

1. `/Applications/MATLAB_R2025b.app/toolbox/econ/econ/@garch/garch.m`
   - core variance recursion contract and unconditional variance definition.
2. `/Applications/MATLAB_R2025b.app/toolbox/econ/econ/@garch/estimate.m`
   - MLE flow, parameter constraints, initialization conventions.
3. `/Applications/MATLAB_R2025b.app/toolbox/econ/econ/@garch/infer.m`
   - inferred conditional variance path behavior.
4. `/Applications/MATLAB_R2025b.app/toolbox/econ/econ/@garch/forecast.m`
   - multi-step conditional variance forecasting behavior.
5. `/Applications/MATLAB_R2025b.app/toolbox/econ/econ/@garch/simulate.m`
   - simulation pathway and presample initialization expectations.
6. `/Applications/MATLAB_R2025b.app/toolbox/econ/econ/@egarch/egarch.m`
   - EGARCH log-variance model contract and coefficient roles.
7. `/Applications/MATLAB_R2025b.app/toolbox/econ/econ/@egarch/estimate.m`
   - EGARCH MLE flow, initialization and constraints expectations.
8. `/Applications/MATLAB_R2025b.app/toolbox/econ/econ/@egarch/infer.m`
   - conditional variance inference behavior for EGARCH.
9. `/Applications/MATLAB_R2025b.app/toolbox/econ/econ/@egarch/forecast.m`
   - multi-step EGARCH variance forecasting behavior.
10. `/Applications/MATLAB_R2025b.app/toolbox/econ/econ/@egarch/simulate.m`
    - EGARCH simulation and presample behavior.
11. `/Applications/MATLAB_R2025b.app/toolbox/econ/econ/@gjr/gjr.m`
    - GJR asymmetry/leverage model contract.
12. `/Applications/MATLAB_R2025b.app/toolbox/econ/econ/@gjr/estimate.m`
    - GJR MLE flow and parameterization behavior.
13. `/Applications/MATLAB_R2025b.app/toolbox/econ/econ/@gjr/infer.m`
    - GJR conditional variance inference behavior.
14. `/Applications/MATLAB_R2025b.app/toolbox/econ/econ/@gjr/forecast.m`
    - GJR forecast recursion behavior.
15. `/Applications/MATLAB_R2025b.app/toolbox/econ/econ/@gjr/simulate.m`
    - GJR simulation behavior.
16. `/Applications/MATLAB_R2025b.app/toolbox/econ/econ/@arima/arima.m`
    - ARIMA object contracts for non-seasonal/seasonal terms.
17. `/Applications/MATLAB_R2025b.app/toolbox/econ/econ/@arima/estimate.m`
    - ARIMA/SARIMA MLE contracts and options surface.
18. `/Applications/MATLAB_R2025b.app/toolbox/econ/econ/@arima/infer.m`
    - innovations inference behavior.
19. `/Applications/MATLAB_R2025b.app/toolbox/econ/econ/@arima/forecast.m`
    - multi-step forecast behavior with seasonal terms.
20. `/Applications/MATLAB_R2025b.app/toolbox/econ/econ/@arima/simulate.m`
    - ARIMA/SARIMA simulation conventions.
21. `/Applications/MATLAB_R2025b.app/toolbox/econ/econ/+internal/+econ/arimaLinearConstraints.m`
    - linear stationarity/invertibility constraint formulation reference.
22. `/Applications/MATLAB_R2025b.app/toolbox/econ/econ/+internal/+econ/arimaNonLinearConstraints.m`
    - nonlinear constraint behavior reference.

## Phase 7: Advanced Diagnostics + Plotting

1. Residual diagnostics overlays in `ts/modules/plotting`.
2. Rolling loss and forecast-error diagnostics.
3. Distributional diagnostics (Q-Q, tail checks, regime diagnostics).

## Phase 8: Testing + Demos

Tests:

1. partition correctness and edge cases
2. lag preparation shape/value checks
3. CV leakage prevention checks
4. deterministic seeded regression tests for model outputs

Demos:

1. partition + direct forecast workflow
2. ARIMA/GARCH estimation and forecast workflow
3. diagnostics workflow

## Naming + Cleanroom Rules

1. Do not use names that suggest copied MATLAB internals.
2. Keep semantics inspired by behavior, not source text.
3. For `.p` sources, infer behavior from callsites/public contracts.
4. Favor clear Pluss-native names and file boundaries.

## Immediate Next Execution Slice

1. Phase 6 hardening:
   - add exogenous regressors (ARIMAX) with typed options.
   - add Student-t likelihood option for volatility and ARIMA lanes.
2. Phase 7 diagnostics:
   - residual diagnostics overlays and rolling error diagnostics in `ts/modules/plotting`.
3. Phase 8 expansion:
   - add more deterministic stress and sensitivity suites for ARIMA/SARIMA/EGARCH/GJR.

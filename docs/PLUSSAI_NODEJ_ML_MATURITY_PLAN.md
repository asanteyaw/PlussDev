# PlussAI NodeJ ML Maturity Plan (Sklearn-Level, Pluss-Native API)

## Scope
Build `PlussAI/NodeJ` into a production-grade classical ML stack with:
- consistent estimator/transformer workflows,
- strong correctness guarantees,
- broad algorithm coverage,
- robust model selection, metrics, and pipeline composition.

This plan follows `docs/doctrine.md`:
- accuracy first, then performance, then ergonomics,
- enum/struct-driven options over string-heavy configs,
- concise Pluss-native naming (no MATLAB/sklearn naming copy as primary API),
- vectorized or linear-pass implementation where possible.

## Non-Negotiable Integration Contract
NodeJ is built strictly on Pluss types and existing Pluss subsystems:
- core numeric type for all NodeJ inputs/outputs/state is `pluss::Series` (from `PlussQit`).
- no foreign matrix/tensor core type in public NodeJ APIs.
- gradient/autodiff comes from `pluss::subsp::math::diff`.
- constrained optimization and shared optimizer runtime come from `pluss::subsp::math::optim`.
- derivative-free and black-box optimizers come from `PlussMa/Math/Numerical/optimization`.
- statistical primitives are reused from `pluss::subsp::stats::distr` and `pluss::subsp::stats::ts`.

NodeJ should wrap these systems through thin adapters and orchestration layers rather than re-implementing parallel math cores.

## Key MATLAB References Used
Primary inspirations reviewed from:
- `toolbox/stats/classreg`: `fitc*`, `fitr*`, `template*`, model classes, internal data prep/encoding, partition models
- `toolbox/stats/stats`: `cvpartition`, `crossval`, and impl classes
- `toolbox/stats/pipeline/learning`: pipeline/component architecture
- `toolbox/stats/incremental`: incremental learner and estimator state logic
- `toolbox/stats/bayesoptim`: auto-model selection (`fitcauto`, `fitrauto`)

## Pluss-Native API Direction
Use this naming map for the public API:
- estimator learning: `learn(...)` (not `fit`)
- estimator inference: `infer(...)` (not `predict`)
- estimator evaluation: `assess(...)` (metric-driven)
- transformer apply: `map(...)` (not `transform`)
- transformer learn+apply: `learn_map(...)` (instead of `fit_transform`)
- holdout split: `holdout_split(...)` (instead of `train_test_split`)
- k-fold planning: `fold_plan(...)`, `strat_fold_plan(...)`, `group_fold_plan(...)`
- pipeline object: `Flow` with `learn`, `infer`, `map`, `learn_map`
- hyperparameter search: `sweep(...)` (grid), `probe(...)` (random), `guide(...)` (bayes/asha later)

Standard technical terms remain unchanged where appropriate (`regression`, `classification`, `kernel`, `ensemble`).

## Target Architecture for `PlussAI/NodeJ`
Create these modules:
- `core/`: base interfaces, options structs, RNG/state, error/status
- `core/bridge`: adapters to `optim::ParameterPack`, `diff`, distributions, and time-series utilities
- `data/`: matrix/table adapters, categorical metadata, missing-value policy
- `prep/`: scalers, encoders, imputers, feature selectors
- `split/`: holdout, k-fold, stratified/group splits, reproducible partition state
- `metrics/`: classification/regression metrics and loss functions
- `classifiers/`: linear, svm, tree, knn, naive bayes, ensemble, multiclass wrappers
- `regressors/`: linear, svm, tree, knn, ensemble
- `compose/`: `Flow`, column-wise routing, leakage-safe learn/infer
- `tune/`: hyperparameter search + CV orchestration
- `online/`: incremental learners and metric windows
- `io/`: model serialization/versioning
- `optim/`: shared optimization kernels

Companion abstraction layer:
- `PlussAI/Lymph`: convenience wrappers built on top of NodeJ for one-call
  make+learn flows while preserving NodeJ's explicit core contracts.

## Cross-Cutting Contracts (Must-Have)
1. Data orientation
- support observations-in-rows by default;
- explicit orientation option in `TrainOpts`/`InferOpts`;
- strict predictor count checks.

2. Categorical pipeline
- learned category ranges at learn time;
- deterministic encoding at infer time;
- unknown category policy (`error`, `missing`, or mapped unknown bucket).

3. Missing values
- explicit policy per algorithm (`reject`, `drop`, `impute`, `route_missing`);
- deterministic behavior documented and tested.

4. Weighted learning/loss
- all assess/loss APIs accept sample weights;
- weighted reduction is normalized (`sum(w * loss) / sum(w)`).

5. Model shape
- Full model keeps training data only when required (diagnostics/resub checks);
- Packed/compact model drops training storage and keeps inference state.

6. Reproducibility
- every split/search/stochastic learn path uses explicit seed+stream state.

## Phased Implementation Plan

### Phase 0: Foundation and Scaffolding
- add `NodeJ` submodule CMake structure for all modules above
- establish hard type contract (`Series` + `ParameterPack<Series>`) in one central NodeJ header
- define base interfaces:
  - `EstimatorBase`
  - `ClassifierBase`
  - `RegressorBase`
  - `TransformerBase`
- define options structs and enums (`TrainOpts`, `InferOpts`, `MetricKind`, `MissingPolicy`, ...)
- add unified diagnostics/status/error layer
- add base test harness for interface contract compliance

Exit criteria:
- module compiles with empty stubs and passing contract tests.

Current status (implemented on 2026-03-10):
- Added NodeJ core type contract headers and compile-time assertions:
  - `pluss::ai::nodej::Series == pluss::Series == pluss::codem::Series`
  - aliases to `diff`, `optim`, `distr`, `ts` bridges.
- Added structured NodeJ API surface in `PlussAI/include/pluss/ai/nodej/`:
  - `types.h`, `status.h`, `options.h`, `dataset.h`, `interfaces.h`, `families.h`, `builders.h`, `nodej.h`.
- Added high-impact family scaffolding through builder entrypoints:
  - `make_classifier(ModelSpec)` and `make_regressor(ModelSpec)`,
  - phase-0 family coverage: `Linear`, `Tree`, `Ensemble`, `Neighbor`, `Kernel`, `Bayes`.
- Added fail-fast placeholder estimator implementations with strict shape/task validation:
  - `PlussAI/NodeJ/classifiers/family_classifier.cpp`
  - `PlussAI/NodeJ/regressors/family_regressor.cpp`
- Added NodeJ module CMake scaffolding for scalability:
  - `core`, `classifiers`, `regressors`, `data`, `prep`, `split`, `metrics`, `compose`, `tune`, `online`, `io`, `optim`.
- Added test harness for Phase 0 interface/build contracts:
  - `PlussAI/test/nodej_phase0_contract_test.cpp` (passes).

### Phase 1: Data, Splits, and Preprocessing Core
- implement dataset adapters and schema metadata
- implement splitters:
  - `holdout_split`
  - `fold_plan`
  - `strat_fold_plan`
  - `group_fold_plan`
- implement preprocessors:
  - standard scaler
  - min-max scaler
  - robust scaler
  - one-hot/frequency encoders
  - simple imputers
- implement `learn_map` lifecycle for transformers

Exit criteria:
- deterministic split behavior and transform round-trip/inference consistency.

Current status (in progress):
- implemented split/CV primitives in NodeJ:
  - `holdout_split(sample_count, options)`
  - `strat_holdout_split(labels, options)`
  - `fold_plan(sample_count, options)`
  - `strat_fold_plan(labels, options)`
- added split contract tests for reproducibility, fold coverage, class-balance behavior, and argument validation.
- implemented preprocessing transformers with NodeJ lifecycle contracts:
  (`learn`, `map`, `learn_map`):
  - `StandardScaler`
  - `MinMaxScaler`
  - `RobustScaler`
  - `SimpleImputer` (mean/median/most-frequent/constant)
  - `NeighborImputer` (KNN-style numeric imputation; uniform or inverse-distance weighting)
  - `IterativeImputer` (round-based chained ridge imputation)
  - `VectorNormalizer` (L1/L2/Linf sample-wise normalization)
  - `MaxAbsScaler`
  - `QuantileMap` (uniform or gaussian output map)
  - `PowerMap` (Yeo-Johnson or Box-Cox, optional standardization)
  - `ThresholdBinarizer`
  - `PolynomialLift` (configurable degree/interactions/bias)
  - `OneHotEncoder` (cardinality caps + unknown-category policy handling)
  - `FrequencyEncoder` (category-frequency mapping with overflow/unknown policies)
- expanded DataFrame integration for local mixed-schema workflows:
  - NodeJ DataFrame adapter now supports Note-backed categorical routing via `DataNotePolicy`:
    - `Reject` (strict),
    - `Drop` (feature omission),
    - `Hash` (deterministic numeric hash encoding)
  - temporal columns (`Timeline`/`TimeIndex`) remain routable via `DataThreadPolicy::CastTemporalToSeries`
  - `Flow` supports DataFrame-native `learn` / `infer` / `infer_scores` / `assess` overloads through the NodeJ adapter
  - PlussMic DataFrame now exposes `matrix()` for grouped typed matrix extraction (`Series`, `Note`, `Timeline`, `TimeIndex`)

### Phase 2: Baseline Estimators (MVP Classical Stack)
- regressors:
  - linear regression (ridge/lasso/elastic options)
  - tree regression
  - knn regression
- classifiers:
  - logistic/linear classifier
  - linear svm classifier
  - tree classifier
  - knn classifier
  - naive bayes
- consistent `learn` / `infer` / `assess` across all models

Exit criteria:
- end-to-end workflow works: split -> prep -> learn -> infer -> assess.

Current status (in progress):
- implemented high-impact linear families with shared `learn` / `infer` / `assess` contracts:
  - `ModelFamily::Linear` regressor:
    - least-squares objective with L2/none regularization
    - robust ridge solve via pinv fallback schedule
    - assess defaults: `Mse`, `Rmse`, `R2` (plus `Mae` support when explicitly requested)
  - `ModelFamily::Linear` classifier:
    - binary logistic objective with Adam optimization
    - binary hinge objective (linear SVM-style) with Adam optimization
    - L2/none regularization and intercept handling
    - `infer_scores` (raw log-odds), `infer` (class labels), assess defaults: `Accuracy`
    - additional assess support: `BalancedAccuracy`, `LogLoss`
- implemented `ModelFamily::Neighbor` baseline estimators:
  - neighbor regressor:
    - euclidean distance KNN with optional feature standardization
    - supports equal or inverse-distance weighting
    - assess defaults: `Mse`, `Rmse`, `R2` (plus `Mae` support when explicitly requested)
  - neighbor classifier (binary):
    - euclidean distance KNN with optional feature standardization
    - supports equal or inverse-distance weighting
    - `infer_scores` returns positive-class probability, `infer` returns class labels
    - assess defaults: `Accuracy` (plus `BalancedAccuracy`, `LogLoss` support)
- implemented `ModelFamily::Tree` baseline estimators (CART-style, deterministic):
  - tree regressor:
    - greedy axis-aligned binary splits using variance reduction (`TreeCriterion::Mse`)
    - options support: `max_depth`, `min_samples_leaf`, `min_samples_split`
    - leaf prediction is node mean response
    - assess defaults: `Mse`, `Rmse`, `R2` (plus `Mae` support when explicitly requested)
  - tree classifier (binary):
    - greedy axis-aligned binary splits with impurity reduction (`TreeCriterion::Gini` or `TreeCriterion::Entropy`)
    - options support: `max_depth`, `min_samples_leaf`, `min_samples_split`
    - `infer_scores` returns positive-class probability from leaf class ratios
    - assess defaults: `Accuracy` (plus `BalancedAccuracy`, `LogLoss` support)
- implemented `ModelFamily::Bayes` baseline classifier:
  - gaussian naive Bayes for binary classification
  - options support: `BayesFamilyOptions.use_gaussian`, `BayesFamilyOptions.smoothing`
  - `infer_scores` returns log-odds, `infer` returns class labels
  - assess defaults: `Accuracy` (plus `BalancedAccuracy`, `LogLoss` support)
- implemented `ModelFamily::Kernel` baseline estimators:
  - kernel regressor:
    - kernel ridge regression in dual form with regularization from `KernelFamilyOptions.c`
    - supports `Linear`, `Rbf`, `Polynomial`, `Sigmoid` kernels
    - assess defaults: `Mse`, `Rmse`, `R2` (plus `Mae` support when explicitly requested)
  - kernel classifier (binary):
    - dual kernel classifier (ridge-style) with thresholded decision scores
    - supports `Linear`, `Rbf`, `Polynomial`, `Sigmoid` kernels
    - assess defaults: `Accuracy` (plus `BalancedAccuracy`, `LogLoss` support)
- implemented `ModelFamily::Ensemble` baseline estimators:
  - ensemble classifier (binary):
    - bagging/random-forest style tree aggregation with bootstrap row sampling
    - random subspace support in random-forest mode
    - class-diverse bootstrap retry logic to avoid degenerate one-class base learners
    - assess defaults: `Accuracy` (plus `BalancedAccuracy`, `LogLoss` support)
  - ensemble regressor:
    - bagging/random-forest style tree averaging
    - bootstrap row sampling and random subspace support in random-forest mode
    - assess defaults: `Mse`, `Rmse`, `R2` (plus `Mae` support when explicitly requested)
- implemented `ModelFamily::Bayes` baseline regressor:
  - gaussian-prior linear bayes regression baseline (closed-form with smoothing regularization)
  - options support: `BayesFamilyOptions.use_gaussian`, `BayesFamilyOptions.smoothing`
  - assess defaults: `Mse`, `Rmse`, `R2` (plus `Mae` support when explicitly requested)
- upgraded estimator contract test coverage to validate concrete linear learn/infer/assess behavior while keeping fail-fast checks for not-yet-implemented families.
- added comprehensive NodeJ demos:
  - core demos in `PlussAI/examples/NodeJ/core` for split/prep and full family workflows
  - convenience demos in `PlussAI/examples/NodeJ/lymph` for one-call make+learn wrappers and preset shortcuts
- evolved Lymph convenience surface with preset wrappers for fast starts:
  - classification presets: `linear_logistic_classifier`, `linear_hinge_classifier`, `forest_classifier`, `rbf_kernel_classifier`, `gaussian_bayes_classifier`
  - regression presets: `ridge_regressor`, `forest_regressor`, `rbf_kernel_regressor`, `gaussian_bayes_regressor`
- expanded rigorous regression/stability coverage:
  - `PlussAI/test/nodej_rigorous_suite_test.cpp` (stress, numerical-finiteness, split determinism, prep edge cases)

### Phase 3: Model Selection and Metrics Breadth
- metrics package expansion:
  - classification: accuracy, balanced accuracy, precision/recall/f1, roc auc, log loss
  - regression: mse, rmse, mae, r2, quantile loss
- CV orchestration:
  - `cross_assess(...)` on any estimator or flow
- hyperparameter tuning:
  - `sweep` (grid)
  - `probe` (random)
- early reporting artifacts: fold-wise and aggregate summaries

Exit criteria:
- reproducible CV and tuning workflows with structured reports.

### Phase 4: Composition and Ensembles
- implement `Flow` (pipeline DAG-lite, series first)
- add leakage-safe learning semantics in composed pipelines
- add ensembles:
  - bagging
  - random forest
  - gradient boosting (tree-based start)
- add multiclass decomposition wrappers for binary learners

Exit criteria:
- strong parity with common sklearn workflows for tabular ML.

Current status (in progress):
- implemented first-class `nodej::Flow` composition for sequential transforms + estimator:
  - supports transform stages: `StandardScaler`, `MinMaxScaler`, `RobustScaler`,
    `SimpleImputer`, `OneHotEncoder`, `FrequencyEncoder`
  - lifecycle methods:
    - `learn(...)`: learns transform stages in order and then learns final estimator
    - `map(...)` / `learn_map(...)`: transform-only execution path
    - `infer(...)`, `infer_scores(...)` (classification), `assess(...)`
  - enforces leakage-safe semantics by learning each transformer only from upstream
    training outputs during `learn(...)`
  - shape contract guardrails: each stage must preserve sample count and emit valid feature matrices
- added `make_flow(ModelSpec)` builder path.
- added Lymph convenience wrappers:
  - `learn_flow(...)`
  - `linear_classifier_flow(...)`
  - `ridge_regressor_flow(...)`
- added compose coverage in tests and demos:
  - tests: `nodej_flow_test`, extended `lymph_quick_test`
  - demos: `nodej_flow_demo`, `lymph_flow_demo`
- implemented multiclass decomposition wrappers (one-vs-rest) across existing binary classifier families:
  - enabled multiclass `learn`/`infer`/`infer_scores`/`assess` through a unified wrapper in `make_classifier(...)`
  - binary paths remain shape-compatible (`infer_scores` stays 1-D for two-class models)
  - multiclass metrics support added for `Accuracy`, `BalancedAccuracy`, `Precision`, `Recall`, `F1`, `LogLoss`

### Phase 5: Online/Incremental and Auto-Selection
- online learners:
  - incremental linear classifier/regressor
  - metric warmup/window tracking
  - update/learn loop (`update_and_learn`)
- auto model selection:
  - first random/grid orchestrator
  - later bayes/asha style `guide(...)`

Exit criteria:
- stable streaming update path and automated model candidate selection.

Current status (in progress):
- implemented NodeJ online baseline in `nodej::online`:
  - `OnlineLinearClassifier` (binary logistic/hinge) with `learn`, `advance`, `infer_scores`, `infer`, `assess`
  - `OnlineLinearRegressor` (least-squares) with `learn`, `advance`, `infer`, `assess`
  - platform-aware Series-native update path (no forced CPU transfer in online compute path)
- added lymph wrappers:
  - `lymph::online_linear_classifier(...)`
  - `lymph::online_linear_regressor(...)`
- added online demos and tests:
  - tests: `nodej_online_test`
  - demos: `nodej_online_demo`, `lymph_online_demo`

### Phase 6: Maturity Hardening
- serialization with schema/version guardrails
- model cards + feature compatibility checks at load/infer
- performance tuning (SIMD, memory layout, parallel folds)
- docs, examples, API references, migration notes

Exit criteria:
- release candidate quality for external users.

## Correctness Rules Borrowed from MATLAB Patterns
- keep entrypoints thin and push heavy logic into shared core classes
- centralize data coercion and argument validation
- separate training model and compact inference model
- make partitioning first-class and reusable across APIs
- keep explicit score/response transforms with clear defaults
- support class priors/costs where classification algorithms allow it
- make missing-value handling explicit and not silent

## Testing Strategy
- unit tests per estimator/transformer/split/metric
- contract tests for all estimators (`learn`, `infer`, `assess`)
- property tests:
  - split reproducibility with seed
  - consistent orientation handling
  - encoder stability across learn/infer
- numeric regression tests with golden fixtures
- integration tests for full `Flow` and tuning loops
- performance smoke tests on medium tabular data

## First Implementation Wave (Start Here)
1. `core` type contract + bridge aliases (`Series`, `ParameterPack<Series>`, `optim`, `diff`, `distr`, `ts`) + CMake wiring.
2. `split`: `holdout_split`, `fold_plan`, `strat_fold_plan`.
3. `prep`: standard scaler + one-hot/frequency encoders with `learn_map`.
4. `regressors`: ridge linear regressor.
5. `classifiers`: logistic linear classifier.
6. `metrics`: accuracy + mse + rmse + r2.
7. `compose`: minimal serial `Flow`.
8. integration example and tests for full path.

## Definition of "Sklearn-Level Maturity" for NodeJ
NodeJ reaches this bar when it has:
- broad, stable estimator coverage for standard tabular ML,
- robust preprocessing + composition + CV/tuning workflows,
- reproducible and well-tested numeric behavior,
- strong docs/examples and backward-compatible API evolution discipline.

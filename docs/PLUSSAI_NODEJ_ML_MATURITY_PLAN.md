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
- statistical primitives are reused from `pluss::subsp::stat::distr` and `pluss::subsp::stat::ts`.

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
- estimator train: `train(...)` (not `fit`)
- estimator inference: `infer(...)` (not `predict`)
- estimator evaluation: `assess(...)` (metric-driven)
- transformer apply: `map(...)` (not `transform`)
- transformer train+apply: `train_map(...)` (instead of `fit_transform`)
- holdout split: `holdout_split(...)` (instead of `train_test_split`)
- k-fold planning: `fold_plan(...)`, `strat_fold_plan(...)`, `group_fold_plan(...)`
- pipeline object: `Flow` with `train`, `infer`, `map`, `train_map`
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
- `compose/`: `Flow`, column-wise routing, leakage-safe train/infer
- `tune/`: hyperparameter search + CV orchestration
- `online/`: incremental learners and metric windows
- `io/`: model serialization/versioning
- `optim/`: shared optimization kernels

## Cross-Cutting Contracts (Must-Have)
1. Data orientation
- support observations-in-rows by default;
- explicit orientation option in `TrainOpts`/`InferOpts`;
- strict predictor count checks.

2. Categorical pipeline
- learned category ranges at train time;
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
- every split/search/stochastic train path uses explicit seed+stream state.

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
- implement `train_map` lifecycle for transformers

Exit criteria:
- deterministic split behavior and transform round-trip/inference consistency.

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
- consistent `train` / `infer` / `assess` across all models

Exit criteria:
- end-to-end workflow works: split -> prep -> train -> infer -> assess.

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
- add leakage-safe training semantics in composed pipelines
- add ensembles:
  - bagging
  - random forest
  - gradient boosting (tree-based start)
- add multiclass decomposition wrappers for binary learners

Exit criteria:
- strong parity with common sklearn workflows for tabular ML.

### Phase 5: Online/Incremental and Auto-Selection
- online learners:
  - incremental linear classifier/regressor
  - metric warmup/window tracking
  - update/train loop (`update_and_train`)
- auto model selection:
  - first random/grid orchestrator
  - later bayes/asha style `guide(...)`

Exit criteria:
- stable streaming update path and automated model candidate selection.

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
- contract tests for all estimators (`train`, `infer`, `assess`)
- property tests:
  - split reproducibility with seed
  - consistent orientation handling
  - encoder stability across train/infer
- numeric regression tests with golden fixtures
- integration tests for full `Flow` and tuning loops
- performance smoke tests on medium tabular data

## First Implementation Wave (Start Here)
1. `core` type contract + bridge aliases (`Series`, `ParameterPack<Series>`, `optim`, `diff`, `distr`, `ts`) + CMake wiring.
2. `split`: `holdout_split`, `fold_plan`, `strat_fold_plan`.
3. `prep`: standard scaler + one-hot encoder with `train_map`.
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

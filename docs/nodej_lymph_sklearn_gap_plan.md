# NodeJ/Lymph vs scikit-learn Gap Plan

## Scope and baseline

This plan compares current `PlussAI/NodeJ` (Lymph API surface) against scikit-learn’s public estimator ecosystem and composition/model-selection stack.

### NodeJ baseline inspected

- Model families/options: `PlussAI/include/pluss/ai/nodej/families.h`
- Training/inference options + CV/tune enums: `PlussAI/include/pluss/ai/nodej/options.h`
- Estimator interfaces: `PlussAI/include/pluss/ai/nodej/interfaces.h`
- Composition/pipeline API: `PlussAI/include/pluss/ai/nodej/compose.h`
- Prep transformers: `PlussAI/include/pluss/ai/nodej/prep.h`
- Tuning + cross-assess/split runtime behavior:
  - `PlussAI/NodeJ/tune/tune.cpp`
  - `PlussAI/NodeJ/metrics/cross_assess.cpp`
  - `PlussAI/NodeJ/split/split.cpp`
- Family implementations and constraints:
  - `PlussAI/NodeJ/classifiers/family_classifier.cpp`
  - `PlussAI/NodeJ/regressors/family_regressor.cpp`
  - `PlussAI/NodeJ/online/online_linear.cpp`

### scikit-learn baseline referenced

- API/code references (stable/main):
  - `sklearn.linear_model`, `sklearn.ensemble`, `sklearn.svm`, `sklearn.preprocessing`, `sklearn.compose`, `sklearn.model_selection`, `sklearn.impute`, `sklearn.decomposition`, `sklearn.cluster`, `sklearn.pipeline`
  - See links in **References**.

---

## Quick diff summary

NodeJ already has a solid Phase-0 base (linear/tree/ensemble/neighbor/kernel/bayes, many preprocessors, flow composition, CV assessment, tuning hooks), but relative to sklearn, major gaps remain in:

1. estimator breadth and algorithm depth,
2. composition ergonomics (`ColumnTransformer`/`Pipeline`-style parameter routing and inverse transforms),
3. hyperparameter search engines (true grid/random/halving/bayesian),
4. metrics/calibration/inspection breadth,
5. sparse/text + feature-selection ecosystem.

---

## Gap matrix (what sklearn has vs NodeJ today)

## 1) Core estimator catalog

| Area | sklearn capability | NodeJ today | Gap |
|---|---|---|---|
| Linear models | `LogisticRegressionCV`, `RidgeCV`, `Lasso`, `ElasticNet`, `SGD*`, `Huber`, `RANSAC`, `TheilSen`, GLMs (`Poisson/Gamma/Tweedie`) | Linear classifier/regressor exist; options enum includes `L1/Elastic` but implementation is `None/L2` only in many paths | Large |
| SVM/kernel | `SVC/SVR`, `LinearSVC/SVR`, `NuSVC/NuSVR`, `OneClassSVM` | Kernel family exists but limited; no one-class path | Medium/Large |
| Trees/ensembles | RF + ExtraTrees + AdaBoost + GradientBoosting + HistGB + Stacking/Voting | Tree/ensemble present, but options enforce subset (e.g., no gradient boost despite enum) | Large |
| Naive Bayes | `GaussianNB`, `BernoulliNB`, `MultinomialNB`, `ComplementNB`, `CategoricalNB` | Bayes family is gaussian-only | Medium |
| Neighbors | KNN + Radius* + NearestNeighbors + graph/transformers | KNN-style classifier/regressor only | Medium |
| Unsupervised | richer clustering/decomposition/outlier set | only KMeans + PCA transformer family, plus custom OutlierIsolator | Medium |

## 2) Multiclass, calibration, probabilistic quality

| Area | sklearn capability | NodeJ today | Gap |
|---|---|---|---|
| Multiclass metrics | multiclass ROC-AUC + broad averaging modes | OvR multiclass works, but multiclass ROC-AUC explicitly not implemented | Medium |
| Probability calibration | `CalibratedClassifierCV` + calibration curve utilities | `ScoreCalibration` enum only; no dedicated calibrator estimator stack | Medium/Large |
| Decision threshold tooling | threshold tuning helper APIs | no tuned-threshold equivalent | Medium |

## 3) Preprocessing and feature engineering

| Area | sklearn capability | NodeJ today | Gap |
|---|---|---|---|
| Preprocessing breadth | rich set includes `KBinsDiscretizer`, spline transforms, target encoder semantics, missing indicators | many present (scalers/imputers/encoders/power/quantile/poly/etc) but no explicit missing-indicator transform API and fewer specialized transforms | Medium |
| Categorical handling | tight integration with column selection + feature names | has route-based transforms + dataframe bridge, but weaker column-wise meta-estimator ergonomics | Medium |
| Inverse transforms | many sklearn transformers provide `inverse_transform` | NodeJ transformer interface is `learn/map/learn_map`; no inverse contract | Medium |

## 4) Composition / pipeline ergonomics

| Area | sklearn capability | NodeJ today | Gap |
|---|---|---|---|
| `Pipeline` parameter routing | `set_params/get_params` with `__` nested tuning | no equivalent reflective parameter routing for nested flow stages | Large |
| `ColumnTransformer` style composition | first-class column selectors and named transformer blocks | feature-route exists, but no equivalent high-level named column transformer UX | Medium/Large |
| Feature unions | `FeatureUnion` and broad composition patterns | partial capability via staged transformers, not same API/behavior parity | Medium |

## 5) Model selection and tuning

| Area | sklearn capability | NodeJ today | Gap |
|---|---|---|---|
| Search engines | `GridSearchCV`, `RandomizedSearchCV`, Halving search CV | `sweep` (candidate list) + `probe` (sampler callback) only; no native grid/random/halving engine objects | Large |
| CV splitter breadth | broad cross-validator set including grouped strategies | `SplitOptions` enum includes `GroupKFold`, but `cross_assess` returns not-implemented for it | Medium |
| Search artifacts | rich `cv_results_`, ranking, refit pipelines | Tune report is lighter; no sklearn-style results table | Medium |

## 6) Metrics, inspection, feature selection

| Area | sklearn capability | NodeJ today | Gap |
|---|---|---|---|
| Metrics breadth | wide classification/regression/ranking/clustering metrics | core metrics only (accuracy/precision/recall/f1/logloss/mse/rmse/mae/r2/quantile) | Medium |
| Inspection | `permutation_importance`, partial dependence, displays | no equivalent inspection module | Large |
| Feature selection | `SelectKBest`, `RFE/RFECV`, `SelectFromModel` | no feature-selection subsystem | Large |

## 7) Data modality + performance ecosystem

| Area | sklearn capability | NodeJ today | Gap |
|---|---|---|---|
| Sparse/text ecosystem | native sparse path across many estimators + text vectorizers | no dedicated sparse/text vectorizer stack in NodeJ/Lymph | Large |
| Incremental learning | broad `partial_fit` families | online API exists but only linear classifier/regressor | Medium/Large |
| Parallel backend | `n_jobs` patterns across many estimators/search | some internal async parallelism, but less standardized/control-rich | Medium |

---

## Closure roadmap (prioritized)

## Milestone A: “sklearn-like workflow parity core” (highest value)

### Goals

- Bring daily workflow parity for tabular supervised tasks.
- Make flow+tuning feel close to sklearn’s `Pipeline` + `GridSearchCV` usage.

### Deliverables

1. Implement true search strategies behind existing `SearchKind`:
   - `Grid`, `Random`, `SuccessiveHalving`.
2. Add `GroupKFold` execution path in `cross_assess`.
3. Expand tune result payload toward sklearn-like `cv_results_` (params + per-fold + rank).
4. Add nested parameter addressing for flow stages (`stage__param` style).

### Files to extend

- `PlussAI/NodeJ/tune/tune.cpp`
- `PlussAI/NodeJ/metrics/cross_assess.cpp`
- `PlussAI/NodeJ/split/split.cpp`
- `PlussAI/NodeJ/compose/flow.cpp`
- `PlussAI/include/pluss/ai/nodej/tune.h`
- `PlussAI/include/pluss/ai/nodej/compose.h`

### Acceptance checks

- New tests for each `SearchKind` mode with deterministic seeds.
- Grouped CV correctness tests.
- End-to-end flow + search + refit parity demos.

## Milestone B: estimator depth upgrade

### Goals

- Close the biggest algorithmic gaps users notice first.

### Deliverables

1. Linear family:
   - add `L1` and `Elastic` regularization where enum already exposes them.
   - add CV variants for ridge/logistic style model selection.
2. Ensemble family:
   - implement `GradientBoost` path (currently enum-level but missing runtime support).
3. Bayes family:
   - add `Multinomial` and `Bernoulli` variants.
4. Neighbor family:
   - add radius-neighbor modes.

### Files to extend

- `PlussAI/NodeJ/classifiers/family_classifier.cpp`
- `PlussAI/NodeJ/regressors/family_regressor.cpp`
- `PlussAI/include/pluss/ai/nodej/families.h`

### Acceptance checks

- Family-specific tests similar to sklearn example cases.
- Numerical/contract tests for each new option path.

## Milestone C: composition + preprocessing ergonomics

### Goals

- Reduce “manual glue code” vs sklearn.

### Deliverables

1. Add a `ColumnTransformer`-like API layer:
   - named blocks,
   - name/dtype selectors,
   - remainder policies.
2. Add missing-indicator transform and optional indicator stacking for imputers.
3. Introduce optional `inverse_map`/`inverse_transform` interface for reversible transforms.

### Files to extend

- `PlussAI/include/pluss/ai/nodej/prep.h`
- `PlussAI/NodeJ/prep/prep.cpp`
- `PlussAI/include/pluss/ai/nodej/compose.h`
- `PlussAI/NodeJ/compose/flow.cpp`

### Acceptance checks

- Pipeline roundtrip tests with dataframe column-name routes.
- Serialization tests with new transform metadata.

## Milestone D: metrics, calibration, and inspection parity

### Goals

- Improve model-evaluation trust and diagnostics.

### Deliverables

1. Implement multiclass ROC-AUC modes in classifier assess path.
2. Add calibration module (`CalibratedClassifierCV` equivalent; sigmoid/isotonic).
3. Add inspection utilities:
   - permutation importance,
   - partial dependence/ICE basics.

### Files to extend

- `PlussAI/NodeJ/classifiers/family_classifier.cpp`
- new module: `PlussAI/NodeJ/calibration/*`
- new module: `PlussAI/NodeJ/inspection/*`
- `PlussAI/include/pluss/ai/nodej/nodej.h` exports

### Acceptance checks

- metric parity tests against known sklearn outputs on toy datasets.
- calibration reliability tests (ECE/Brier improvements).

## Milestone E: feature selection + sparse/text readiness

### Goals

- Cover common sklearn workflows in production tabular/text settings.

### Deliverables

1. Feature-selection package:
   - `VarianceThreshold`,
   - `SelectKBest`,
   - `RFE/RFECV` (or staged alternative).
2. Sparse-friendly estimator paths and transforms.
3. Initial text vectorizer bridge (count/tfidf style) for NodeJ flows.

### Files to extend

- new modules under `PlussAI/NodeJ/feature_selection/*`
- estimator paths in `family_classifier.cpp` / `family_regressor.cpp`
- compose integration in `flow.cpp`

### Acceptance checks

- sparse-matrix workflow tests.
- text pipeline smoke tests with CV/tuning.

---

## Execution order recommendation

1. **Milestone A first** (workflow and tuning parity unlocks immediate user value).
2. **Milestone B next** (algorithm coverage users expect from sklearn).
3. **Milestone C/D in parallel** (ergonomics + trust metrics/calibration).
4. **Milestone E last** (broader ecosystem and modality expansion).

---

## Risks and mitigation

- API drift risk from adding sklearn-like behaviors:
  - Mitigate with additive APIs; keep current `learn/infer/assess` stable.
- Performance regressions from richer search:
  - Mitigate via bounded parallelism defaults and benchmark gates.
- Serialization compatibility:
  - Bump bundle schema and provide migration readers.

---

## References

- NodeJ headers and implementations in this repo:
  - `PlussAI/include/pluss/ai/nodej/*.h`
  - `PlussAI/NodeJ/**/*.cpp`
- scikit-learn references:
  - https://raw.githubusercontent.com/scikit-learn/scikit-learn/main/sklearn/linear_model/__init__.py
  - https://raw.githubusercontent.com/scikit-learn/scikit-learn/main/sklearn/ensemble/__init__.py
  - https://raw.githubusercontent.com/scikit-learn/scikit-learn/main/sklearn/svm/__init__.py
  - https://raw.githubusercontent.com/scikit-learn/scikit-learn/main/sklearn/preprocessing/__init__.py
  - https://raw.githubusercontent.com/scikit-learn/scikit-learn/main/sklearn/compose/__init__.py
  - https://raw.githubusercontent.com/scikit-learn/scikit-learn/main/sklearn/model_selection/__init__.py
  - https://raw.githubusercontent.com/scikit-learn/scikit-learn/main/sklearn/impute/__init__.py
  - https://raw.githubusercontent.com/scikit-learn/scikit-learn/main/sklearn/decomposition/__init__.py
  - https://raw.githubusercontent.com/scikit-learn/scikit-learn/main/sklearn/cluster/__init__.py
  - https://raw.githubusercontent.com/scikit-learn/scikit-learn/main/sklearn/pipeline.py
  - https://scikit-learn.org/stable/api/sklearn.preprocessing.html
  - https://scikit-learn.org/stable/api/sklearn.ensemble.html
  - https://scikit-learn.org/stable/api/sklearn.impute.html
  - https://scikit-learn.org/stable/api/sklearn.compose.html
  - https://scikit-learn.org/stable/api/sklearn.metrics.html
  - https://scikit-learn.org/stable/api/sklearn.calibration.html
  - https://scikit-learn.org/stable/api/sklearn.inspection.html


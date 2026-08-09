# Lymph General ML Expansion Plan

Status: planning document after the Lymph API redesign close-out.

Purpose: compare current Lymph/NodeJ coverage with the breadth of scikit-learn, then
lay out pending expansion by Pluss-native family lines. This is not a naming plan for
copying sklearn. Sklearn docs and source are reference material for correctness and
coverage only; Lymph public names should remain native to Pluss.

## Phase Ledger

### Phase 1/7: Shared Validation Infrastructure

Status: complete.

Implemented in this phase:

- Added the `validate` family as the public validation entry point:
  - `validate::cross(...)`
  - `validate::stratified(...)`
  - `validate::grouped(...)`
  - `validate::time_ordered(...)`
  - `validate::holdout(...)`
- Added `validate::Plan`, which consumes either a single `Model` or a named
  `Builder` and returns existing report/result objects instead of pushing more
  validation methods onto `Model`.
- Added `validate::HoldoutPlan` for train/test slicing without exposing another
  old-style free-function-first surface.
- Fixed model-backed cross-validation on `(DataFrame, DataFrameBatchOptions)` so
  fold training slices the original frame and calls `ModelCore::learn(frame,
  data_options, ...)`. This preserves pending `Stage + ColumnRouter` resolution,
  categorical factorization maps, and final workflow semantics during validation.
- Kept `LabeledBatch` validation as the direct Series path for callers supplying
  platform-owned feature/label data.

Notes:

- `validate::Plan::scores(...)` extracts requested fold values with Series
  reshape/narrow operations over the fold metric matrix.
- The `(DataFrame x, Series y)` staged cross-validation path still has less
  routing context than `(DataFrame frame, DataFrameBatchOptions)`; public examples
  should prefer the latter whenever stages use column names or note policies.

### Phase 2/7: Reports, Metrics, And Accessor Semantics

Status: complete.

Implemented in this phase:

- Added accessor-style family views so users can keep code readable without
  pushing every operation onto the root object:
  - `model.reports().confusion_table(...)`
  - `model.reports().classification_summary(...)`
  - `model.reports().metric_series(...)`
  - `model.reports().threshold_curve(...)`
  - `model.reports().best_threshold(...)`
- Added raw prediction metric views:
  - `metrics::classification(y, prediction).confusion_table()`
  - `metrics::classification(y, prediction).summary()`
  - `metrics::classification(y, prediction).values(...)`
  - `metrics::classification(y, prediction).report(...)`
  - `metrics::regression(y, prediction).values(...)`
  - `metrics::regression(y, prediction).report(...)`
  - `metrics::threshold(scores, y).curve()`
  - `metrics::threshold(scores, y).best(...)`
- Kept existing free functions as compatibility and implementation substrate,
  but Phase 2 establishes accessors as the cleaner public consumption style for
  metrics/reporting work.

Design notes:

- Accessors are allowed when they improve family semantics, like `Series`
  namespace accessors. They should remain narrow and family-owned, not become a
  path for putting unrelated operations onto `Model`.
- Classification hard-label metrics stay on `metrics::classification(...)`;
  model-backed probability/threshold reports stay on `model.reports()` or
  `metrics::threshold(scores, y)`.
- Metric math remains delegated to NodeJ kernels and Series operations. Lymph
  only coordinates names, options, and report shape.

### Phase 3/7: Object-Accessor Preprocessing And Shape Transforms

Status: complete.

Implemented in this phase:

- Added `Model::prepare()` as the object-factory accessor for model-bound
  preprocessing/stage attachment.
- Added `ModelPrep` as the fluent preparation object returned by
  `model.prepare()`.
- Added router helper methods on `ModelPrep` so common preprocessing code can
  avoid repeated `stage::...` and `columnrouter::...` factory calls:
  - `prepare.names(...)`
  - `prepare.where(...)`
  - `prepare.remaining(...)`
- Added method-based stage attachment on `ModelPrep`:
  - `standardize(...)`
  - `rescale(...)`
  - `robust(...)`
  - `fill(...)`
  - `neighbor_fill(...)`
  - `iterative_fill(...)`
  - `encode(...)`
  - `count_encode(...)`
  - `order_encode(...)`
  - `label_encode(...)`
  - `normalize(...)`
  - `maxabs(...)`
  - `gaussianize(...)`
  - `quantiles(...)`
  - `power(...)`
  - `binarize(...)`
  - `polynomial(...)`
- `gaussianize(...)` exposes the already-implemented quantile-map-to-Gaussian
  path through a Pluss-native, method-chained shape transform.

Design notes:

- Existing `model.append(stage::..., columnrouter::...)` remains supported, but
  new user code should prefer `model.prepare().*` when building model-bound
  preprocessing.
- Avoid adding `lymph::*::*::*` public paths. The accepted public boundary is
  roughly `lymph::family::factory(...)`; after that, use object methods.
- `Model::prepare()` is not intended to make `Model` a god object. It only
  manifests a preparation object; all stage attachment methods live on that
  object.
- Stage execution remains delegated to the existing NodeJ/Series-backed
  transform machinery. This phase changes API shape, not numerical kernels.

### Phase 4/7: Unsupervised Result Families

Status: complete.

Implemented in this phase:

- Added `unsupervised.h` to the public Lymph umbrella.
- Added the `cluster` family for clustering-oriented transform models:
  - `cluster::model(model).run(...)`
  - `cluster::Result`
- Added the `embed` family for decomposition/embedding-oriented transform
  models:
  - `embed::model(model).run(...)`
  - `embed::Result`
- Added Series-first overloads for `FeatureBatch`, plus DataFrame overloads for
  table workflows.
- `cluster::Result` exposes:
  - `model()`
  - `labels()`
  - `centers()`
  - `cluster_count()`
  - `board()`
- `embed::Result` exposes:
  - `model()`
  - `coordinates()`
  - `mapped()`
  - `mean()`
  - `components()`
  - `variances()`
  - `component_count()`
  - `board()`

Design notes:

- This phase does not put clustering or embedding methods onto the root
  `Model`. The result families consume a transform-style `Model` and data, then
  return a family-owned result object.
- Result families intentionally do not expose estimator-name shortcuts such as
  `cluster::kmeans(data, ...)` or `embed::pca(data, ...)`. Estimators remain
  under `models`; task families create runners and return task results.
- K-means and PCA numerical work stays in NodeJ. Lymph reads the existing
  transform-state contracts exposed by NodeJ flows: K-means state gives centers;
  PCA state gives mean, components, and variances.
- Summary boards are intentionally small reporting artifacts. Large outputs
  remain `Series` so caller-controlled platform execution is preserved.
- The public names are Pluss-native: `cluster` and `embed`, not sklearn class
  names.

Reference material read for this phase:

- scikit-learn K-means source:
  https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/cluster/_kmeans.py
- scikit-learn PCA source:
  https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/decomposition/_pca.py
- scikit-learn unsupervised cluster metric source:
  https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/metrics/cluster/_unsupervised.py

### Phase 5/7: Shape Profile And Transform Planning

Status: complete.

Implemented in this phase:

- Added `shape.h` to the public Lymph umbrella.
- Added the `shape` family for distribution diagnostics and transform planning:
  - `shape::data(batch, names).run(...)`
  - `shape::data(frame).run(...)`
  - `shape::data(frame, data_options).run(...)`
  - `shape::Result`
  - `shape::Plan`
- `shape::Result` exposes:
  - `features()`
  - `sample_count()`
  - `feature_count()`
  - `mean()`
  - `stddev()`
  - `minimum()`
  - `maximum()`
  - `skewness()`
  - `excess_kurtosis()`
  - `missing_rate()`
  - `negative_rate()`
  - `zero_rate()`
  - `gaussianize_candidate()`
  - `board()`
  - `plan()`
- `shape::Plan::apply(model)` attaches existing Gaussian quantile-map stages to
  selected feature names through `Model::prepare()`.

Design notes:

- `shape` is not a second preprocessing implementation. It profiles data and
  builds plans; actual transforms remain in `stage`/`prep` and execute through
  NodeJ/Series-backed stage machinery.
- Stats are computed through column-wise Series reductions. Host extraction is
  limited to feature-name selection for the small plan metadata.
- `shape::data(...).run()` follows the result-family runner pattern without
  adding distribution diagnostics to `Model`.

Reference material read for this phase:

- scikit-learn preprocessing transform source:
  https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/preprocessing/_data.py
- scikit-learn discretization/binning source:
  https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/preprocessing/_discretization.py
- scikit-learn imputation source:
  https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/impute/_base.py
- scikit-learn preprocessing user guide:
  https://scikit-learn.org/stable/modules/preprocessing.html

### Phase 6/7: Detection And Anomaly Result Family

Status: complete.

Implemented in this phase:

- Added `detect.h` to the public Lymph umbrella.
- Added `models::isolation_detector(...)` as the model-level landing zone for
  isolation-style detection.
- Added the `detect` family for novelty/anomaly/outlier result workflows:
  - `detect::model(model).run(...)`
  - `detect::Result`
- `detect::model(...)` consumes model-level anomaly estimators such as
  `models::one_class_svm(...)` and `models::isolation_detector(...)`, then
  returns a detection result.
- `detect::Result` exposes:
  - `method()`
  - `has_model()`
  - `model()`
  - `labels()`
  - `flagged()`
  - `flagged_count()`
  - `flagged_rate()`
  - `has_scores()`
  - `scores()`
  - `threshold()`
  - `board()`

Design notes:

- Isolation-forest-style tools are model-level estimators when they are the main
  artifact a user tunes, saves, deploys, and reruns. Phase 6 therefore lays down
  `models::isolation_detector(...)` now, backed by the current NodeJ isolation
  engine.
- One-class SVM remains a `Model` because it is already implemented as a
  transform-style model-level estimator. `detect::model(...)` runs it and
  packages labels, mask, threshold, and summaries.
- `detect` owns anomaly result semantics. `Model` stays focused on primitive
  `learn`/`infer`/`map` operations.
- Continuous score access for model-level detectors should move into a generic
  NodeJ transform-score hook later; Phase 6 exposes labels, masks, thresholds,
  and summaries through the model-level runner.

Reference material read for this phase:

- scikit-learn isolation forest source:
  https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/ensemble/_iforest.py
- scikit-learn local outlier factor source:
  https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/neighbors/_lof.py
- scikit-learn elliptic envelope source:
  https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/covariance/_elliptic_envelope.py
- scikit-learn novelty/outlier detection user guide:
  https://scikit-learn.org/stable/modules/outlier_detection.html

### Phase 7/7: Semi-Supervised Result Family

Status: complete.

Implemented in this phase:

- Added `semi.h` to the public Lymph umbrella.
- Added the `semi` family for semi-supervised classification workflows:
  - `semi::model(model).run(...)`
  - `semi::Result`
- `semi::model(...)` consumes an existing classification `Model`; semi-supervised
  work is a result/task family, not a separate estimator namespace.
- Added `semi::Options` with Pluss-native controls:
  - `unlabeled_label`
  - `select`
  - `confidence`
  - `top_k`
  - `max_rounds`
  - `final_refit`
- Added selection modes:
  - `semi::SelectKind::Confidence`
  - `semi::SelectKind::TopK`
- Added stop reasons:
  - `semi::StopReason::AllLabeled`
  - `semi::StopReason::NoChange`
  - `semi::StopReason::MaxRounds`
- `semi::Result` exposes:
  - `model()`
  - `labels()`
  - `labeled_round()`
  - `rounds()`
  - `stop()`
  - `sample_count()`
  - `labeled_count()`
  - `pseudo_labeled_count()`
  - `board()`

Design notes:

- The first semi-supervised base is self-labeling: it repeatedly trains the
  supplied classifier on labeled rows, infers unlabeled rows, accepts confident
  pseudo-labels or the top-k candidates, then optionally refits the final model.
- Label propagation/spreading is still a pending graph-kernel expansion. It
  should land through NodeJ graph/similarity primitives first, then be consumed
  by Lymph without copying sklearn class names.
- Score confidence is normalized inside the runner so linear-style margins and
  probability-style classifiers can both participate.
- The implementation keeps data movement Series-first. Host scalar extraction is
  limited to loop control, stop checks, and small summary decisions.

Reference material read for this phase:

- scikit-learn self-training source:
  https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/semi_supervised/_self_training.py
- scikit-learn label propagation/spreading source:
  https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/semi_supervised/_label_propagation.py
- scikit-learn semi-supervised user guide:
  https://scikit-learn.org/stable/modules/semi_supervised.html

## Doctrine

1. Lymph is for general classical ML: supervised, unsupervised, semi-supervised,
   anomaly/density, preprocessing, validation, search, reports, inspection, and
   deployment. Deep learning remains NodeK/Ice.
2. NodeJ owns numerical engines and reusable kernels. Lymph owns family separation,
   orchestration, Pluss-native names, workflow semantics, and ergonomic consumption.
3. Prefer Series and vectorized kernels over host loops. DataFrame paths are for CPU
   table workflows; Series/FeatureBatch/LabeledBatch paths must remain first-class for
   caller-controlled platform execution.
4. Preserve the redesign rule: factories bring objects into existence; methods operate on
   existing objects; families consume other family values only to create/run their own
   object.
5. No public family should be a one-off downstream workaround. Every expansion item below
   is generic and should be useful across ML tasks.

## Reference Material Read For This Plan

Scikit-learn docs:

- API reference: https://scikit-learn.org/stable/api/index.html
- User guide: https://scikit-learn.org/stable/user_guide

Scikit-learn source files inspected for coverage shape:

- Base/API tags and estimator taxonomy: https://github.com/scikit-learn/scikit-learn/tree/main/sklearn
- Model selection split plans: https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/model_selection/_split.py
- Search orchestration: https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/model_selection/_search.py
- Classification metrics/reporting: https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/metrics/_classification.py
- Column composition: https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/compose/_column_transformer.py
- Preprocessing transforms: https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/preprocessing/_data.py
- Encoders: https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/preprocessing/_encoders.py
- Clustering reference shape: https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/cluster/_kmeans.py
- Decomposition/PCA reference shape: https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/decomposition/_pca.py
- Unsupervised clustering metrics reference shape: https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/metrics/cluster/_unsupervised.py
- Semi-supervised graph propagation: https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/semi_supervised/_label_propagation.py
- Mixture density models: https://github.com/scikit-learn/scikit-learn/blob/main/sklearn/mixture/_gaussian_mixture.py

Additional source areas to read before implementation tranches:

- `sklearn/linear_model/*`
- `sklearn/tree/*`
- `sklearn/ensemble/*`
- `sklearn/svm/*`
- `sklearn/neighbors/*`
- `sklearn/naive_bayes.py`
- `sklearn/discriminant_analysis.py`
- `sklearn/gaussian_process/*`
- `sklearn/covariance/*`
- `sklearn/decomposition/*`
- `sklearn/manifold/*`
- `sklearn/random_projection.py`
- `sklearn/kernel_approximation.py`
- `sklearn/feature_selection/*`
- `sklearn/inspection/*`
- `sklearn/calibration.py`
- `sklearn/impute/*`
- `sklearn/multioutput.py`
- `sklearn/multiclass.py`
- `sklearn/pipeline.py`
- `sklearn/compose/_target.py`

## Current Lymph Coverage Snapshot

This snapshot is based on the current Lymph and NodeJ headers.

Implemented families:

- `models`: linear/logistic/hinge, tree, ensemble, neighbor, kernel, Bayes,
  cluster, decompose, online linear, sparse linear.
- `stage`: scalers, imputers, encoders, normalizers, maxabs, quantile map,
  power map, threshold binarizer, polynomial lift.
- `prep`: standalone transformer wrappers for the same preprocessing engines,
  including distribution-shaping tools.
- `columnrouter`: name/predicate/remainder routing.
- `policy`: raw-frame cleanup.
- `workflow`: deployable raw-frame-in artifact.
- `builder`: named model inventory.
- `spotcheck`: strategy x model comparison.
- `search`: grid/random hyperparameter search.
- `selection`: candidate comparison/tuning substrate.
- `reports`: classification summary, confusion table, threshold curve, best
  threshold.
- `metrics`: classification/regression scalar metrics, cross-validation helpers,
  holdout split.
- `balance`: class-mix and imbalance growth targets.
- `feature`: score/rank/pick feature selection surface.
- `inspection`: partial dependence and permutation impact.
- `transform`: target transforms for regression-style model work.
- `sparse`: sparse linear training/evaluation surface.
- `shape`: feature distribution profile and transform-plan surface.
- `detect`: anomaly/outlier detection result surface.
- `semi`: self-labeling semi-supervised classification result surface.

Current high-level state:

- Supervised learning: strongest coverage.
- Unsupervised learning: present but thin: K-means, PCA, transform-style models,
  one-class style kernel path, and detection result wrappers.
- Semi-supervised learning: present as a self-labeling result family; graph
  propagation/spreading is still pending.
- Distribution-shaping preprocessing: present via `quantile_map` and `power_map`,
  with `shape` diagnostics/planning now started.
- Composite workflow: working and deployable; needs richer family interop for
  target transforms, feature unions, and multi-input/multi-output tasks.

## Family Expansion Map

### 1. `models` Family

Role: trainable or transform-style classical ML estimators.

Keep public construction under `lymph::models::`.

Current coverage:

- Linear: basic regression/classification/logistic/hinge.
- Trees: decision tree classifier/regressor.
- Ensembles: bagging, random forest, gradient boost, AdaBoost classifier, generic
  regressor/classifier.
- Neighbors: KNN and radius variants.
- Kernel: SVM-like classifier/regressor, nu variants, one-class SVM, kernel
  transformer shapes.
- Bayes: Gaussian/Bernoulli/Multinomial/Complement variants.
- Cluster/decompose: K-means and PCA.
- Online/sparse: online linear and sparse linear surfaces.

Pending expansions:

1. Linear and GLM breadth:
   - Penalized linear paths: ridge, lasso, elastic, coordinate-descent paths.
   - Robust regression: Huber-style, RANSAC-style, Theil-Sen-style.
   - Distribution-aware GLMs: Poisson, Gamma, Tweedie, quantile.
   - Passive/aggressive and perceptron-style online margins.
   - Multitask regression/classification variants.
   - NodeJ first: solvers, losses, regularization paths, sample weights, warm-start.
   - Lymph: `models::linear(...)` specializations plus `Search` axes for penalty/loss.

2. Discriminant models:
   - Linear and quadratic discriminant classifiers.
   - NodeJ first: class covariance/statistics, shrinkage, priors.
   - Lymph: keep under `models::linear_discriminant(...)` /
     `models::quadratic_discriminant(...)` or a native shorter naming decision.

3. Tree and forest breadth:
   - Extra-randomized trees.
   - Histogram gradient boosting.
   - Random tree embeddings.
   - Monotonic constraints and missing-value aware splitting where appropriate.
   - NodeJ first: splitters, histogram binning, categorical/missing split policy.
   - Lymph: factories remain under `models`.

4. Ensemble meta-models:
   - Voting and stacking for classification/regression.
   - Bagging/boosting diagnostics and staged prediction.
   - NodeJ first: ensemble composition engine and prediction aggregation.
   - Lymph: likely a new `combine` family for creating ensemble compositions from
     existing `Model` values, instead of overloading `Builder`.

5. Kernel and margin breadth:
   - Full C/SVM-style classifiers/regressors, nu variants, linear optimized path.
   - Kernel ridge.
   - One-class and novelty/anomaly score consistency.
   - NodeJ first: solver stability and kernel matrix memory strategy.
   - Lymph: `models` factories plus `kernelmap` family for approximate feature maps.

6. Gaussian process models:
   - Process regression/classification and kernel objects.
   - NodeJ first: kernel algebra, Cholesky/low-rank solvers, log-marginal likelihood.
   - Lymph: likely `kernel` or `prior` family consumed by `models`.

7. Neighbor and density models:
   - Nearest centroid, nearest-neighbor transformer, density estimation.
   - Local outlier scoring.
   - NodeJ first: reusable nearest-neighbor index, graph output, density scoring.
   - Lymph: `models::neighbor_*` plus `graph` and `density` interop.

8. Naive Bayes completion:
   - Categorical likelihood.
   - Better count smoothing controls and feature-type policy.
   - NodeJ first: categorical/count likelihood kernels.
   - Lymph: extend existing Bayes options/factories.

### 2. `cluster` / Unsupervised Structure Family

Role: unsupervised grouping and cluster diagnostics. Current K-means as a `Model` is
not enough.

Proposed public family:

- `lymph::cluster::plan(...)` or `lymph::cluster::profile(...)` for diagnostic work.
- `lymph::models::*` remains the construction point for cluster estimators.

Pending expansions:

- Mini-batch K-means.
- Bisecting K-means.
- DB-style density clustering.
- OPTICS-style reachability clustering.
- Hierarchical/agglomerative clustering.
- BIRCH-style incremental clustering.
- Spectral clustering and biclustering/coclustering.
- Mean shift / affinity propagation.
- Feature agglomeration.
- Cluster reports: silhouette, Davies-Bouldin, Calinski-Harabasz, adjusted rand,
  mutual information variants, homogeneity/completeness/v-measure.

NodeJ requirements:

- Pairwise distance kernels and neighbor graphs.
- Sparse/dense graph primitives.
- Cluster label metrics.
- Efficient memory policy for O(n^2) algorithms.

Lymph requirements:

- Cluster result object with `.labels()`, `.centers()`, `.score_table()`,
  `.profile()`, `.plot_data()` hooks where appropriate.
- Unsupervised `spotcheck` objective support.

### 3. `density` Family

Role: probability density, mixture, anomaly scoring, covariance-backed models.

Current coverage:

- One-class kernel path exists.
- Outlier isolator exists as preprocessing-ish tool.
- `detect::model(models::one_class_svm(...)).run(...)` produces anomaly labels
  and a detection summary.
- `detect::model(models::isolation_detector(...)).run(...)` exposes the NodeJ
  isolation-style detector with labels, threshold, and summary.

Pending expansions:

- Gaussian mixture and Bayesian mixture.
- Kernel density.
- Elliptic/covariance-envelope style detector.
- Isolation-forest style detector.
- Local-outlier scoring.
- Covariance estimators: empirical, shrinkage, robust minimum covariance,
  sparse inverse covariance.

NodeJ requirements:

- EM loops with log-sum-exp stabilization.
- Covariance parameterizations and positive-definite safeguards.
- Density scoring and anomaly threshold utilities.
- Vectorized Mahalanobis and pairwise kernels.

Lymph requirements:

- New `density` family for score calibration, thresholding, and anomaly reports.
- `models` constructors for density estimators.
- `reports` additions for anomaly detection: score distributions, flagged-rate,
  threshold tables.

### 4. `embed` Family

Role: dimensionality reduction, manifold learning, and low-dimensional embeddings.

Current coverage:

- PCA as a transform-style model.

Pending expansions:

- Incremental PCA.
- Kernel PCA.
- Sparse PCA.
- Truncated SVD.
- NMF.
- ICA.
- Factor analysis.
- Dictionary learning and sparse coding.
- Random projections.
- Manifold methods: spectral embedding, Isomap, MDS, locally linear embedding,
  t-SNE-style embedding.

NodeJ requirements:

- SVD/eigendecomposition paths for Series-backed dense data.
- Sparse matrix path where available.
- Neighbor graph kernels.
- Random projection generators.
- NMF/ICA optimization loops.

Lymph requirements:

- `models::*` for trainable reducers.
- `stage::pca(...)` and future reducer stages when a reducer is one step in a
  supervised workflow.
- `embed` family for diagnostics: explained variance, reconstruction error,
  trustworthiness, component tables.

### 5. `shape` Family

Role: distribution shaping, normality support, skew/heavy-tail diagnostics, and
feature-domain transformations.

Current coverage:

- `stage::quantile_map({.output = QuantileMapOutput::Gaussian})`
- `stage::power_map({.mode = PowerMapMode::YeoJohnson})`
- `stage::power_map({.mode = PowerMapMode::BoxCox})`
- Robust/minmax/standard/maxabs scaling.
- `shape::data(...).run()` for distribution profiles.
- `shape::Result::plan()` and `shape::Plan::apply(model)` for Gaussianizing
  skewed/heavy-tail feature candidates through existing stages.

Pending expansions:

- Promote distribution-shaping ergonomics without adding sklearn-like names.
- Add diagnostics for skewness, kurtosis, heavy-tail, zero/negative constraints,
  monotonicity, and recommended transform eligibility.
- Add reversible shape transforms where mathematically possible.
- Add spline basis and binning stages.
- Add missing indicators and nullable-feature policies.

Proposed native surface:

- `lymph::shape::profile(frame, data)` -> distribution table.
- `lymph::shape::suggest(frame, data)` -> reusable shape plan.
- `shape_plan.apply(model)` or `model.append(shape_plan.stage(...), router)`.

NodeJ requirements:

- Vectorized quantiles/histograms where missing.
- Stable Box-Cox/Yeo-Johnson lambda search.
- Spline/binning kernels.

Lymph requirements:

- Keep actual transforms in `stage`/`prep`.
- `shape` should plan and report; it should not become a second preprocessing
  implementation.

### 6. `semi` Family

Role: learning from partly labeled data.

Current coverage:

- Self-labeling classification through `semi::model(model).run(...)`.

Pending expansions:

- Graph propagation and spreading.
- Label confidence and pseudo-label audit reports.

NodeJ requirements:

- Graph affinity construction.
- Iterative label propagation kernels.
- Confidence-threshold pseudo-label loop.

Lymph requirements:

- `semi::model(model).run(...)` remains the model-consuming runner shape for
  classifier-backed self-labeling.
- Graph propagation should use Pluss-native runner factories once NodeJ graph
  kernels exist; do not copy sklearn class names into the public API.
- Should consume `Model` when self-training, not become a `Model` method.

### 7. `validate` Family

Role: validation plans, splitters, learning/validation curves, permutation tests.

Current coverage:

- Holdout, cross validation, stratified, repeated, group, and time-series plan fields
  exist through options/split plans.
- Search and spotcheck consume CV options.

Pending expansions:

- Dedicated validation plan objects:
  - `validate::cross(...)`
  - `validate::stratified(...)`
  - `validate::grouped(...)`
  - `validate::time_ordered(...)`
  - `validate::rolling_origin(...)`
  - `validate::leave_one(...)`
  - `validate::predefined(...)`
- Cross prediction.
- Learning curves.
- Validation curves.
- Permutation tests.
- Repeated holdout and repeated grouped variants.

NodeJ requirements:

- Split plan kernels already partly present; extend to leave-p/group/predefined.
- Efficient cross-prediction materialization.

Lymph requirements:

- `ValidationPlan` object with `.run(model, data, assess)` and `.scores(...)`.
- `validate` consumes `Model`/`Builder`/`SpotCheck` where appropriate.

### 8. `search` and `spotcheck` Families

Role: hyperparameter search and broad model/strategy comparison.

Current coverage:

- `search`: grid/random axes.
- `spotcheck`: model x strategy comparison.
- `selection`: substrate.

Pending expansions:

- Successive-halving style search.
- Bayesian/adaptive search later, if a generic optimizer family is available.
- Multi-metric selection policy.
- Refitting policy: best, top-k, custom tie-break.
- Early-stop hooks.
- Search over preprocessing stages and column-router alternatives.
- Unsupervised objective support.

NodeJ requirements:

- Resource-aware search scheduler.
- Trial result persistence.
- Optional callback/cancellation hooks.

Lymph requirements:

- Keep `Search` and `SpotCheck` separate:
  - Search tunes one model/workflow family over axes.
  - SpotCheck compares existing model/strategy inventories.

### 9. `reports` and `score` Families

Role: metrics, summaries, display-ready tables, score curves.

Current coverage:

- Classification summary.
- Confusion table.
- Threshold curve and best threshold.
- Several scalar classification/regression metrics.

Pending expansions:

- Regression reports: residual summary, prediction error table, quantile/pinball,
  Poisson/Gamma/Tweedie deviance, explained variance, max error, MAPE, MSLE.
- Classification reports: multilabel/multioutput, top-k, PR/ROC displays,
  DET curve, calibration curve, Brier score, Cohen kappa, Matthews correlation,
  Jaccard, Hamming, zero-one loss.
- Clustering reports: silhouette, adjusted rand, mutual information, v-measure,
  pair confusion.
- Ranking reports: DCG/NDCG, label ranking losses.
- Distance/kernel reports: pairwise distance summaries, kernel matrix checks.

NodeJ requirements:

- Metrics should be Series-vectorized.
- Avoid host loops except for label metadata/table assembly.

Lymph requirements:

- Keep report construction under `reports`.
- Add `score` only if scorer objects become reusable family values for search and
  validation. Otherwise keep metrics as options and reports.

### 10. `inspect` Family

Role: model inspection, explainability, and diagnostics.

Current coverage:

- Partial dependence and permutation impact.

Pending expansions:

- Individual conditional expectation.
- Decision boundaries.
- Prediction error displays.
- Calibration displays.
- Feature importance stability.
- Component/loadings inspection for decompositions.
- Cluster profile inspection.

NodeJ requirements:

- Efficient perturbation grids and batched inference.
- Support for classifier/regressor/transform models.

Lymph requirements:

- Keep inspection outside `Model`.
- Inspection consumes trained `Model`, `Workflow`, or `SpotCheckResult`.

### 11. `feature` Family

Role: feature scoring, ranking, pruning, and feature-space construction.

Current coverage:

- Score/rank/pick wrappers exist.

Pending expansions:

- Variance filtering.
- Univariate statistical tests.
- Mutual information.
- Recursive elimination.
- Sequential selection.
- Model-importance selection.
- Feature agglomeration interop with clustering.

NodeJ requirements:

- Vectorized score kernels.
- Iterative wrapper loops that can reuse validation/search machinery.

Lymph requirements:

- `feature::rank`, `feature::pick`, `feature::eliminate`, `feature::sequence`.
- Feature selectors should be usable as `Stage`s where they transform X.

### 12. `compose` / `workflow` Family

Role: combining preprocessing, target transforms, model ensembles, feature unions,
and deployable raw-input artifacts.

Current coverage:

- `Workflow` = `Policy + Model`, with snapshot save/load and policy sidecar.
- `Model::append(Stage, ColumnRouter)`.
- `workflow_dsl` for branch/feature transformers.

Pending expansions:

- Feature union style composition.
- Target transform interop that is persistable.
- Multi-branch workflows where branches produce named feature groups.
- Frozen/pretrained model handling.
- Production score table generation.
- Batch inference reports.

NodeJ requirements:

- Persistable custom transform story or explicit non-persistable diagnostics.
- Feature-name propagation through transforms.

Lymph requirements:

- Keep `Workflow` deployable and narrow.
- New `compose` family can create complex model/workflow structures from pieces.

### 13. `target` / Multi-output Family

Role: target-shape handling, multilabel, multioutput, chains, and one-vs-* strategies.

Current coverage:

- Binary/one-vs-rest style paths exist in places.
- Target transform for regression exists, but persistence is restricted.

Pending expansions:

- Multi-output regression/classification.
- Classifier/regressor chains.
- Multi-label binarization.
- Multi-class reduction policies.
- Target encoders and target-domain reports.

NodeJ requirements:

- Matrix target support.
- Per-output model/vectorized loss support.
- Target encoding and decoding.

Lymph requirements:

- New `target` family for target policies and transforms.
- Reduction strategies should be family objects, not hidden booleans on every model.

### 14. `calibrate` Family

Role: probability and decision-threshold calibration.

Current coverage:

- Score calibration options exist on training.
- Best-threshold reports exist.

Pending expansions:

- Calibration runner that consumes a trained or unfitted model with validation plan.
- Calibration curves and reliability tables.
- Brier and calibration loss reports.
- Threshold-tuning object with deployable threshold policy.

NodeJ requirements:

- Calibration curves and stable isotonic/logistic calibration kernels.

Lymph requirements:

- `calibrate::scores(model, plan, ...)` produces a calibrated `Model` or calibration
  artifact.
- Threshold tuning should be a family object, not just a scalar helper.

### 15. `data` / `simulate` Family

Role: dataset generation/loading utilities for tests, demos, benchmarking, and docs.

Current coverage:

- DataFrame CSV loading lives elsewhere in Pluss.
- Lymph has no substantial dataset/simulation family.

Pending expansions:

- Synthetic classification/regression/blob/moon/circle/manifold data.
- Sparse-coded signals and low-rank matrices.
- ML benchmark fixtures.
- Dataset metadata and split recipes.

NodeJ requirements:

- Random generators using Series platform where feasible.

Lymph requirements:

- `simulate::classification`, `simulate::regression`, `simulate::clusters`,
  `simulate::manifold`.
- Keep real-world downloading out unless Pluss has a broader data registry story.

### 16. `text` / Feature Extraction Family

Role: non-numeric feature extraction.

Current coverage:

- NodeJ has text option structs for lexicon, tf-idf, hashing maps.
- Lymph public story is thin.

Pending expansions:

- Text featurizers as `Stage` and standalone `prep` objects.
- Hashing and tf-idf workflows.
- Dict/categorical feature hashing.
- Image patch extraction is lower priority unless downstream demand exists.

NodeJ requirements:

- Tokenization, hashing, sparse matrix/Series representation.

Lymph requirements:

- New `text` or `extract` family; avoid overloading `prep` with domain-specific
  constructors if it becomes too broad.

### 17. `kernelmap` and `distance` Families

Role: approximate kernels, pairwise distances, neighbor graphs, affinity matrices.

Current coverage:

- Kernel model family exists.
- Spatial index and neighbor utilities exist at NodeJ level.

Pending expansions:

- Random Fourier features.
- Nystrom approximation.
- Polynomial count sketches.
- Pairwise distances/kernels as reusable values.
- DistanceMetric-like policy objects.

NodeJ requirements:

- Dense/sparse pairwise kernels.
- Chunked computation strategy.
- Approximate feature map generators.

Lymph requirements:

- `kernelmap::*` creates transform stages/models.
- `distance::*` creates metric policies consumed by neighbor, cluster, manifold,
  and density families.

### 18. `meta` Family

Role: wrappers and model adaptation.

Current coverage:

- Builder/SpotCheck/Search cover comparison/tuning, not estimator wrappers.

Pending expansions:

- Frozen model wrapper.
- Callback/progress hooks.
- Output/feature-name propagation.
- Estimator tags/capabilities report.
- Model cloning and introspection tables beyond Builder.

NodeJ requirements:

- Capability tags in `ModelSpec`/`Flow`.

Lymph requirements:

- `meta::freeze(model)`, `meta::capabilities(model)`, or similar native naming.
- Do not let `Model` become a god object.

## Priority Roadmap

### Phase A: Complete General-ML Infrastructure

1. `validate` family objects over existing split plans.
2. Unsupervised objective support in `selection`, `search`, and `spotcheck`.
3. Metric/report expansion for regression, clustering, anomaly, multilabel.
4. Persistable target transforms.
5. Feature-name propagation through Workflow and Stage results.

### Phase B: Broaden Unsupervised ML

1. Cluster breadth: density/hierarchical/spectral/mini-batch.
2. Decomposition breadth: SVD/NMF/ICA/factor/dictionary.
3. `embed` diagnostics and reducer stages.
4. `density` family: mixtures, covariance, anomaly.

### Phase C: Broaden Supervised Estimators

1. Linear/GLM/robust/path solvers.
2. Discriminant models.
3. Histogram/extratree ensembles.
4. Kernel ridge and Gaussian process.
5. Multi-output and target policies.

### Phase D: Semi-supervised and Graph Work

1. Graph propagation/spreading.
2. Self-training wrapper.
3. Neighbor graph and affinity infrastructure.
4. Confidence/audit reports.

### Phase E: Feature Extraction and Simulation

1. Text/hash/tf-idf stages.
2. Kernel approximations/random projections.
3. Synthetic datasets under `simulate`.
4. Benchmark fixtures and smoke-test datasets.

## Definition Of Done For Each Expansion Tranche

1. Read the relevant sklearn docs and source file(s) before implementation.
2. Implement algorithmic kernels in NodeJ first when numerical work is needed.
3. Expose Lymph through an existing or clearly justified new family.
4. Provide DataFrame and Series/FeatureBatch/LabeledBatch paths where relevant.
5. Use Pluss error handling only.
6. Add focused tests:
   - correctness on small deterministic cases,
   - shape/edge cases,
   - weighted/sample-weight behavior where relevant,
   - serialization when model/stage state is persistable,
   - parity-inspired behavior checks without copying sklearn names.
7. Add or update minimal downstream-style demo only after the API is compact enough to
   show real ergonomic value without helper boilerplate.

## Immediate Next Tranches Recommended

1. `validate` family object layer, because it benefits supervised, unsupervised, and
   semi-supervised workflows.
2. `reports` expansion for regression and clustering, because Search/SpotCheck need
   objective breadth.
3. `shape` family diagnostics, because the gaussianizing transforms already exist but
   need discoverable workflow semantics.
4. `density` family start: Gaussian mixture + covariance/elliptic scoring.
5. `cluster` breadth: mini-batch K-means, hierarchical, density clustering, silhouette.
6. `embed` breadth: truncated SVD/NMF/ICA and reducer-stage interop.
7. `semi` family start: self-training and graph propagation.

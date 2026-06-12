# Lymph User Guide

Lymph is the task-first machine learning surface for Pluss. It is built on NodeJ, but its user API is designed for concise workflows.

## What Lymph Is

Lymph is for local, end-to-end ML work:

- data split
- preprocessing
- model training
- inference
- evaluation
- cross-validation
- model selection
- persistence

Use Lymph when you want typed options and predictable behavior with less boilerplate than raw NodeJ.

Use NodeJ directly when you need low-level batch shaping or custom orchestration.

## Install And Include

### API Signature

- `#include <pluss/lymph.h>`
- `namespace lym = pluss::ai::lymph`

### Notes

Lymph reuses NodeJ option structs and enums, so your option tuning transfers directly.

### Inputs

- compiler with C++20 support
- Pluss built with `PlussAI` and `PlussMic`

### Output

- Lymph symbols under `pluss::ai::lymph`

### Options

- no runtime flags required for basic CPU usage
- deterministic behavior is controlled through option structs (`random_state`, `deterministic`)

### Examples

```cpp
#include <pluss/lymph.h>
#include <pluss/mic.h>

namespace lym = pluss::ai::lymph;
```

### Expected output

- Project compiles and Lymph symbols resolve.

## Core Mental Model

### API Signature

- `Workflow(model: WorkflowModel, stages: list<TransformSpec> = {}) -> Workflow`
- `Workflow::learn_(x: DataFrame, y: Series|Thread, options: TrainOptions = {}) -> Workflow&`
- `Workflow::infer(x: DataFrame, options: InferOptions = {}) -> Series`
- `Workflow::assess(x: DataFrame, y: Series|Thread, options: AssessOptions = {}) -> MetricReport`
- `Workflow::save(path: string, x_train: DataFrame, y_train: Series|Thread, options: TrainOptions = {}, metadata: FlowBundleMetadata = {}) -> void`
- `Workflow::load(path: string) -> Workflow`

### Notes

A `Workflow` is an ordered sequence of preprocessing stages followed by one model. `learn_` trains in place. `learn` is out-of-place and returns a trained copy.

### Inputs

- `model`: from a `*_factory`
- `stages`: from `lym::workflow::stage::*`

### Output

- trained workflow and `MetricReport`/`Series` outputs

### Options

- stage options are typed structs (no string configs)
- model options are family-specific typed structs

### Examples

```cpp
lym::Workflow wf{
    .model = lym::linear_factory::linear_regression({}),
    .stages = {
        lym::workflow::stage::simple_imputer(
            lym::prep::SimpleImputerOptions{.strategy = lym::prep::SimpleImputerStrategy::Median}),
        lym::workflow::stage::standard_scaler(),
    },
};
```

### Expected output

- Workflow is valid and executes stages in listed order, then trains/infers with the model.

## Quickstart: Regression

### API Signature

- `linear_factory::linear_regression(options: LinearFamilyOptions = {}) -> WorkflowModel`
- `Workflow::learn_(x: DataFrame, y: Series|Thread, options: TrainOptions = {}) -> Workflow&`
- `Workflow::infer(x: DataFrame, options: InferOptions = {}) -> Series`
- `Workflow::assess(x: DataFrame, y: Series|Thread, options: AssessOptions = {}) -> MetricReport`

### Notes

This is the fastest path for a classical tabular regression flow.

### Inputs

- feature frame (`x_train`, `x_test`)
- target series/thread (`y_train`, `y_test`)

### Output

- predictions as `Series`
- metrics in `MetricReport` (for regression: `mse`, `rmse`, `r2`, `mae` when requested)

### Options

- `TrainOptions.max_iterations`, `TrainOptions.tolerance`, `TrainOptions.random.seed`, `TrainOptions.random.deterministic`
- `AssessOptions.metrics`

### Examples

```cpp
lym::Workflow wf{
    .model = lym::linear_factory::linear_regression({}),
    .stages = {
        lym::workflow::stage::simple_imputer(
            lym::prep::SimpleImputerOptions{.strategy = lym::prep::SimpleImputerStrategy::Median}),
        lym::workflow::stage::standard_scaler(),
    },
};

wf.learn_(x_train, y_train, pluss::ai::nodej::TrainOptions{
    .max_iterations = 3000,
    .tolerance = 1e-8,
    .random = {.seed = 42, .deterministic = true},
});

const Series pred = wf.infer(x_test);
auto metrics = wf.assess(x_test, y_test, pluss::ai::nodej::AssessOptions{
    .metrics = {
      pluss::ai::nodej::MetricKind::Rmse,
      pluss::ai::nodej::MetricKind::R2,
      pluss::ai::nodej::MetricKind::Mae,
    }
});
```

### Expected output

- `pred` has one value per sample in `x_test`.
- `metrics.rmse`, `metrics.r2`, `metrics.mae` are defined `Series` scalars.

## Quickstart: Classification

### API Signature

- `linear_factory::logistic_regression(options: LinearFamilyOptions = {}) -> WorkflowModel`
- `Workflow::infer_scores(x: DataFrame, options: InferOptions = {}) -> Series`

### Notes

Use `infer` for predicted labels and `infer_scores` for score/probability-like values.

### Inputs

- classification labels in `y_train`/`y_test`

### Output

- class predictions and classification metrics (`accuracy`, `precision`, `recall`, etc.)

### Options

- `AssessOptions.metrics` should include classification metrics you care about.

### Examples

```cpp
lym::Workflow clf{
    .model = lym::linear_factory::logistic_regression({}),
    .stages = {lym::workflow::stage::standard_scaler()},
};

clf.learn_(x_train, y_train);
const Series y_hat = clf.infer(x_test);
const Series scores = clf.infer_scores(x_test);
auto report = clf.assess(x_test, y_test, pluss::ai::nodej::AssessOptions{
    .metrics = {
      pluss::ai::nodej::MetricKind::Accuracy,
      pluss::ai::nodej::MetricKind::F1,
      pluss::ai::nodej::MetricKind::RocAuc,
    }
});
```

### Expected output

- `y_hat` shape matches test sample count.
- `scores` contains model confidence/score values.
- `report.accuracy`, `report.f1`, `report.roc_auc` are defined.

## Data Ingestion And Splitting

### API Signature

- `holdout_split(frame: DataFrame, options: HoldoutSplitOptions = {}) -> FrameHoldoutSplit`
- `hold_out_split(frame: DataFrame, options: HoldoutSplitOptions = {}) -> pair<DataFrame, DataFrame>`

### Notes

`hold_out_split` is a one-liner convenience wrapper. Use `holdout_split` when you also want split indices.

### Inputs

- full DataFrame
- optional stratification by `stratify` (Series) or `stratify_feature` (column name)

### Output

- train/test frames and optional split indices

### Options

- `test_size`
- `stratify` or `stratify_feature`
- `random_state`
- `deterministic`
- `shuffle`

### Examples

```cpp
auto [train_df, test_df] = lym::hold_out_split(
    full_df,
    lym::HoldoutSplitOptions{
      .test_size = 0.2,
      .stratify_feature = "target",
      .random_state = 42,
      .deterministic = true,
      .shuffle = true,
    });
```

### Expected output

- approximately 80/20 split
- deterministic repeatability for same seed and data order

## Preprocessing Guide

### Shared Preprocessor Contract

#### API Signature

- `prep::<Transformer>(options: ... = {}) -> Transformer`
- `Transformer::features(names: {string...}) -> Transformer&`
- `Transformer::learn(frame: DataFrame, options: TrainOptions = {}) -> Transformer&`
- `Transformer::map(frame: DataFrame, options: InferOptions = {}) -> DataFrame`
- `Transformer::learn_map(frame: DataFrame, learn_options: TrainOptions = {}, infer_options: InferOptions = {}) -> DataFrame`
- `Transformer::summary() -> <SummaryType>`
- `Transformer::learned() -> bool`

#### Notes

Preprocessors operate on DataFrame and return transformed DataFrame. If `features(...)` is not set, they resolve Series-backed features automatically.

#### Inputs

- DataFrame with supported feature types

#### Output

- transformed DataFrame and learned state in `summary()`

#### Options

- each preprocessor has its typed options struct (see cookbook section for recommendations)

#### Examples

```cpp
auto imputer = lym::prep::SimpleImputer(
    lym::prep::SimpleImputerOptions{.strategy = lym::prep::SimpleImputerStrategy::Median});

const DataFrame x_train_imp = imputer.learn_map(x_train);
const DataFrame x_test_imp = imputer.map(x_test);
auto stats = imputer.summary();
```

#### Expected output

- same row count as input frame
- selected features imputed/transformed
- `stats.statistics` populated for `SimpleImputer`

### Available Preprocessors

- `prep::SimpleImputer`
- `prep::NeighborImputer`
- `prep::IterativeImputer`
- `prep::StandardScaler`
- `prep::MinMaxScaler`
- `prep::RobustScaler`
- `prep::MaxAbsScaler`
- `prep::VectorNormalizer`
- `prep::OneHotEncoder`
- `prep::FrequencyEncoder`
- `prep::OrdinalEncoder`
- `prep::LabelEncoder`
- `prep::CategoryIndexer`
- `prep::QuantileMap`
- `prep::PowerMap`
- `prep::ThresholdBinarizer`
- `prep::PolynomialLift`

### CategoryIndexer And Encoders: Correct Roles

`CategoryIndexer` is an indexing utility for categorical values. It learns a stable
category-to-index map and applies it consistently. It is not a replacement for all
encoders.

Use this mental model:

- `CategoryIndexer`: canonical integer IDs for categories (mapping utility)
- `OrdinalEncoder`: ordered integer encoding for model input
- `LabelEncoder`: integer label encoding surface for categorical targets/labels or compact feature IDs
- `OneHotEncoder`: sparse/binary indicator expansion for nominal categories
- `FrequencyEncoder`: category -> frequency/statistical prevalence encoding

When to use each:

- use `CategoryIndexer` when you need a stable lookup/index map, joins, diagnostics, or pre-indexing
- use `OneHotEncoder` for nominal categories where linear distance between IDs is meaningless
- use `OrdinalEncoder` only when order is meaningful or tree models can tolerate integer bins
- use `LabelEncoder` for compact integer labels (often target-side, sometimes feature-side)
- use `FrequencyEncoder` when dimensionality pressure makes one-hot too wide

Typical combinations:

- `CategoryIndexer` -> `OneHotEncoder`: only if your source is already pre-indexed and you want explicit index control
- raw categorical -> `OneHotEncoder`: most common nominal feature pipeline
- raw categorical -> `OrdinalEncoder`: when ordered categories are intended
- raw categorical -> `FrequencyEncoder`: compact alternative to one-hot

Minimal examples:

```cpp
// 1) Stable indexing utility
auto idx = lym::prep::CategoryIndexer({
    .max_categories = 64,
    .include_unknown_bucket = true,
});
idx.features({"ocean_proximity"});
DataFrame x_idx_train = idx.learn_map(train_df);
DataFrame x_idx_test = idx.map(test_df);

// 2) Model-ready nominal encoding
auto onehot = lym::prep::OneHotEncoder({.max_categories = 64});
onehot.features({"ocean_proximity"});
DataFrame x_oh_train = onehot.learn_map(train_df);
DataFrame x_oh_test = onehot.map(test_df);

// 3) Compact frequency encoding
auto freq = lym::prep::FrequencyEncoder({.max_categories = 64});
freq.features({"ocean_proximity"});
DataFrame x_fr_train = freq.learn_map(train_df);
DataFrame x_fr_test = freq.map(test_df);
```

## Model Factories

### API Signature

- `linear_factory::linear_regression(options: LinearFamilyOptions = {}) -> WorkflowModel`
- `linear_factory::logistic_regression(options: LinearFamilyOptions = {}) -> WorkflowModel`
- `linear_factory::hinge_classifier(options: LinearFamilyOptions = {}) -> WorkflowModel`
- `tree_factory::decision_tree_regressor(options: TreeFamilyOptions = {}) -> WorkflowModel`
- `tree_factory::decision_tree_classifier(options: TreeFamilyOptions = {}) -> WorkflowModel`
- `ensemble_factory::{bagging,random_forest,gradient_boost}_{regressor|classifier}(options: EnsembleFamilyOptions = {}) -> WorkflowModel`
- `neighbor_factory::knn_{regressor|classifier}(options: NeighborFamilyOptions = {}) -> WorkflowModel`
- `neighbor_factory::radius_neighbors_{regressor|classifier}(options: NeighborFamilyOptions = {}) -> WorkflowModel`
- `kernel_factory::svm_{regressor|classifier}(options: KernelFamilyOptions = {}) -> WorkflowModel`
- `kernel_factory::nu_svm_{regressor|classifier}(options: KernelFamilyOptions = {}) -> WorkflowModel`
- `kernel_factory::one_class_svm(options: KernelFamilyOptions = {}) -> WorkflowModel`
- `bayes_factory::gaussian_nb_{regressor|classifier}(options: BayesFamilyOptions = {}) -> WorkflowModel`
- `bayes_factory::bernoulli_nb_{regressor|classifier}(options: BayesFamilyOptions = {}) -> WorkflowModel`
- `bayes_factory::multinomial_nb_{regressor|classifier}(options: BayesFamilyOptions = {}) -> WorkflowModel`
- `bayes_factory::complement_nb_classifier(options: BayesFamilyOptions = {}) -> WorkflowModel`
- `cluster_factory::kmeans(options: ClusterFamilyOptions = {}) -> WorkflowModel`
- `decompose_factory::pca(options: DecomposeFamilyOptions = {}) -> WorkflowModel`

### Notes

Factories create model instances with typed options. For unsupervised tasks, use cluster/decompose factories or `kernel_factory::one_class_svm`.

Radius-neighbor factories set neighbor query mode to radius while preserving radius, fallback, weighting, and standardization options. Radius mode uses all training samples within the configured radius, then falls back to nearest neighbors when a query has no radius hits.

Nu SVM factories set the kernel form to Nu while preserving the rest of the supplied kernel options. `one_class_svm` creates a transform-style workflow model that maps feature batches to inlier/outlier labels.

Bernoulli, Multinomial, and Complement Bayes factories set the Bayes likelihood explicitly. Multinomial and Complement Bayes expect non-negative count-like features and return an error for negative counts. Complement Bayes is classifier-only.

### Inputs

- one options struct per family

### Output

- `WorkflowModel` ready for direct use or embedding in `Workflow`

### Options

- all NodeJ family options are accepted unchanged
- objective/task-safe defaults are applied where necessary (for example logistic objective for logistic regression)
- neighbor query modes include `KNearest` and `Radius`
- kernel forms include `C`, `Nu`, and `OneClass`
- Bayes likelihoods include `Gaussian`, `Bernoulli`, `Multinomial`, and classifier-only `Complement`

### Examples

```cpp
auto lin = lym::linear_factory::linear_regression(
    pluss::ai::nodej::LinearFamilyOptions{.fit_intercept = true});

auto tree = lym::tree_factory::decision_tree_regressor(
    pluss::ai::nodej::TreeFamilyOptions{.max_depth = 12, .min_samples_leaf = 8});

auto nu_svm = lym::kernel_factory::nu_svm_classifier(
    pluss::ai::nodej::KernelFamilyOptions{
        .kernel = pluss::ai::nodej::KernelKind::Rbf,
        .gamma = 1.0,
        .nu = 0.5,
    });

auto one_class = lym::kernel_factory::one_class_svm(
    pluss::ai::nodej::KernelFamilyOptions{
        .kernel = pluss::ai::nodej::KernelKind::Rbf,
        .gamma = 0.8,
        .nu = 0.5,
    });

auto radius = lym::neighbor_factory::radius_neighbors_classifier(
    pluss::ai::nodej::NeighborFamilyOptions{
        .radius = 0.75,
        .radius_fallback_k = 3,
        .distance_weighted = true,
    });

auto multinomial = lym::bayes_factory::multinomial_nb_classifier(
    pluss::ai::nodej::BayesFamilyOptions{.smoothing = 1e-6});

auto complement = lym::bayes_factory::complement_nb_classifier(
    pluss::ai::nodej::BayesFamilyOptions{.smoothing = 1e-6});
```

### Expected output

- `lin`, `tree`, `nu_svm`, `radius`, `multinomial`, and `complement` can immediately `learn`, `infer`, and `assess`.
- `one_class` can immediately `learn_map` and `map`, returning one inlier/outlier label per sample.

## Workflow Patterns

### API Signature

- `workflow::stage::<transform>(options: ... = {}, feature_route: list<int64> = {}) -> TransformSpec`
- `Workflow::learn_(...) -> Workflow&`
- `Workflow::learn(...) -> Workflow`

### Notes

`Workflow` executes stage list in order, then model learn/infer/assess.

### Inputs

- ordered list of stage specs
- one model

### Output

- reusable workflow object

### Options

- `feature_route` lets a stage target specific feature positions.

For feature-branch composition (numeric branch + categorical branch), use
`workflow::feature::compose(...)` with `make_feature_transformer(...)`.

### Examples

```cpp
lym::Workflow wf{
    .model = lym::tree_factory::decision_tree_regressor({.max_depth = 14}),
    .stages = {
      lym::workflow::stage::simple_imputer({
        .strategy = lym::prep::SimpleImputerStrategy::Median
      }),
      lym::workflow::stage::one_hot_encoder({}),
      lym::workflow::stage::standard_scaler(),
    },
};

wf.learn_(x_train, y_train);
```

### Expected output

- stages are fitted/applied before model training.

## Evaluation And Metrics

### API Signature

- `Workflow::assess(...) -> MetricReport`
- `WorkflowModel::assess(...) -> MetricReport`
- `compare_series(lhs: Series, rhs: Series) -> SeriesCompareReport`
- `max_abs_diff(lhs: Series, rhs: Series) -> double`

### Notes

`MetricReport` exposes metric fields as Series scalars for consistent tensor semantics.

### Inputs

- predictions or `(x, y)` evaluation input

### Output

- metrics and comparison diagnostics

### Options

- request exact metrics via `AssessOptions.metrics`

### Examples

```cpp
auto metric = wf.assess(x_test, y_test, pluss::ai::nodej::AssessOptions{
    .metrics = {
      pluss::ai::nodej::MetricKind::Rmse,
      pluss::ai::nodej::MetricKind::R2,
      pluss::ai::nodej::MetricKind::Mae,
    }
});

const Series y_hat = wf.infer(x_test);
auto diff = lym::compare_series(y_hat, y_ref);
```

### Expected output

- requested metric fields are populated in `metric`
- `diff.max_abs_diff` and `diff.mae` provide quick parity checks

## Cross-Validation And Tuning

### API Signature

- `cross_validate(model: WorkflowModel|Workflow|ModelSpec, x: DataFrame, y: Series|Thread, cv: CrossValidationOptions = {}, train_options: TrainOptions = {}, assess_options: AssessOptions = {}) -> CrossValidationReport`
- `selection::tune(candidates: vector<Candidate>, x: DataFrame, y: Series|Thread, cv: CrossValidationOptions = {}, train_options: TrainOptions = {}, assess_options: AssessOptions = {}, objective_metric: MetricKind = Accuracy, objective_policy: TuneObjectivePolicy = Auto) -> TuneResult`
- `selection::tune_specs(...) -> SpecTuneResult`
- `selection::probe_specs(...) -> SpecTuneResult`

### Notes

Use `cross_validate` for one model/workflow. Use `selection::tune` for named workflow candidates. Use `tune_specs`/`probe_specs` for model-spec sweeps.

### Inputs

- workflow(s) or model spec(s)
- CV plan and objective metric

### Output

- fold report + mean/stdev metrics
- best workflow/spec in tuning results

### Options

- `CrossValidationOptions.plan`: `Holdout`, `KFold`, `StratifiedKFold`
- `folds`, `holdout_fraction`, `stratified`, `shuffle`, `random_state`, `deterministic`

### Examples

```cpp
auto cv = lym::cross_validate(
    wf, x_train, y_train,
    lym::CrossValidationOptions{
      .plan = pluss::ai::nodej::CVPlanKind::KFold,
      .folds = 5,
      .random_state = 11,
      .deterministic = true,
    },
    {},
    pluss::ai::nodej::AssessOptions{.metrics = {pluss::ai::nodej::MetricKind::Rmse}});

std::vector<lym::selection::Candidate> cands;
cands.emplace_back("linear", lym::Workflow{.model = lym::linear_factory::linear_regression({})});
cands.emplace_back("tree", lym::Workflow{.model = lym::tree_factory::decision_tree_regressor({.max_depth = 12})});

auto tuned = lym::selection::tune(
    std::move(cands),
    x_train,
    y_train,
    lym::CrossValidationOptions{.plan = pluss::ai::nodej::CVPlanKind::KFold, .folds = 5},
    {},
    pluss::ai::nodej::AssessOptions{.metrics = {pluss::ai::nodej::MetricKind::Rmse}},
    pluss::ai::nodej::MetricKind::Rmse,
    pluss::ai::nodej::TuneObjectivePolicy::Minimize);
```

### Expected output

- `cv.mean.rmse` and `cv.stdev.rmse` are defined.
- `tuned.report.best_name` names the selected workflow.

## Persistence And Reproducibility

### API Signature

- `Workflow::save(path: string, x_train: DataFrame, y_train: Series|Thread, train_options: TrainOptions = {}, metadata: FlowBundleMetadata = {}) -> void`
- `Workflow::load(path: string) -> Workflow`
- `Workflow::restore(bundle: FlowBundle) -> Workflow`

### Notes

Use deterministic seeds in train/CV split options for reproducible experiments and bundle restores.

### Inputs

- model path
- training data used for bundle restore path

### Output

- persisted workflow bundle and restored workflow

### Options

- training seed and deterministic flags
- optional metadata in save call

### Examples

```cpp
wf.save("hour_workflow.bin", x_train, y_train,
        pluss::ai::nodej::TrainOptions{.random = {.seed = 17, .deterministic = true}});

auto restored = lym::Workflow::load("hour_workflow.bin");
const Series pred_a = wf.infer(x_test);
const Series pred_b = restored.infer(x_test);
const double delta = lym::max_abs_diff(pred_a, pred_b);
```

### Expected output

- `delta` should be close to `0.0` under deterministic setup.

## Online Learning Surface

### API Signature

- `online_factory::linear_classifier(options: OnlineLinearOptions = {}) -> OnlineLinearClassifier`
- `online_factory::linear_regressor(options: OnlineLinearOptions = {}) -> OnlineLinearRegressor`
- `OnlineLinear*::learn_(x: DataFrame, y: Series|Thread, options: TrainOptions = {}) -> OnlineLinear*&`
- `OnlineLinear*::advance_(x: DataFrame, y: Series|Thread, options: TrainOptions = {}) -> OnlineLinear*&`
- `OnlineLinear*::infer(x: DataFrame, options: InferOptions = {}) -> Series`
- `OnlineLinear*::assess(x: DataFrame, y: Series|Thread, options: AssessOptions = {}) -> MetricReport`

### Notes

Online models support initial fit (`learn_`) then incremental updates (`advance_`).

### Inputs

- stream batches as DataFrames/Series

### Output

- updated online model and predictions/metrics

### Options

- `OnlineLinearOptions` for forgetting, regularization, and stability behavior

### Examples

```cpp
auto online = lym::online_factory::linear_regressor({});
online.learn_(x0, y0);
online.advance_(x1, y1);
const Series y_hat = online.infer(x2);
auto m = online.assess(x2, y2, pluss::ai::nodej::AssessOptions{
    .metrics = {pluss::ai::nodej::MetricKind::Rmse}
});
```

### Expected output

- `online.seen_samples()` increases after each call.
- `m.rmse` is defined.

## End-To-End Blueprint

1. split dataset with `hold_out_split`
2. define `Workflow{ .model, .stages }`
3. call `learn_`
4. call `cross_validate`
5. call `assess` on holdout
6. call `save`, then `load`
7. compare predictions with `max_abs_diff`

Expected output behavior:

- repeatable metrics for fixed seed
- trained workflow reloads and infers consistently

## Migration Guide (sklearn -> Lymph Terms)

- `fit` -> `learn_`
- `fit_transform` -> `learn_map`
- `transform` -> `map`
- `predict` -> `infer`
- `predict_proba`/`decision_function` -> `infer_scores`
- `score` -> `assess`
- sklearn `Pipeline([...])` concept -> `Workflow{ .model, .stages = {...} }`
- `train_test_split` -> `hold_out_split`
- `GridSearchCV` workflow selection -> `selection::tune` / `selection::tune_specs`

For option recipes, see **Lymph Cookbook**. For exhaustive enum/bool combinations, see **Lymph Combination Atlas**. For failure diagnostics, see **Lymph Troubleshooting**.

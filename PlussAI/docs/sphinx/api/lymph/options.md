# Lymph `options`

`lymph::options` is the **umbrella alias namespace** for option structs — one
short spelling per struct, regardless of which family actually owns and defines
it. `options::DFBatch` and `DataFrameBatchOptions` are the same type; use
whichever reads better in context. It holds no behavior of its own — it is
exactly the list of `using` aliases in `lymph/options.h`.

This page is the field reference: for every option struct, the data members it
carries, their types, their defaults, and the values each one accepts.

## How An Options Struct Works

Every `*Options` type is a plain aggregate. There are no constructors and no
setters — brace-initialize it, then assign the fields you want to change; every
field you leave alone keeps the default shown in the tables below. Pass it **by
value** to the factory or verb that takes it.

Set two training fields on a default options struct, then train a logistic regression model with it. The program prints whether the model trained.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <iostream>
#include <vector>

namespace lymph = pluss::ai::lymph;
using pluss::table::DataFrame;
using pluss::table::Slate;

lymph::LabeledBatch fixture_batch(const pluss::Config& cfg) {
  const pluss::Series x = pluss::make_series(std::vector<double>{
      0.0, 0.1, 0.2, 0.0, 0.1, 0.3, 0.3, 0.2,
      1.0, 1.1, 1.2, 0.9, 1.1, 1.3, 1.4, 1.0}, {8, 2}, cfg);
  const pluss::Series y = pluss::make_series(std::vector<double>{0, 0, 0, 0, 1, 1, 1, 1}, {8}, cfg);
  return lymph::LabeledBatch{
      .features = lymph::FeatureBatch{.x = x, .layout = lymph::ObservationLayout::Rows},
      .y = y,
  };
}

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const lymph::LabeledBatch batch = fixture_batch(cfg);
  lymph::options::Train train{};
  train.max_iterations = 250;
  train.class_weight = lymph::ClassWeightMode::Balanced;
  lymph::models::Model model = lymph::models::logistic_regression();
  model.learn_(batch, train);
  std::cout << "trained = " << (model.trained() ? "true" : "false") << "\n";
  return 0;
}
```

Output

```text
trained = true
```

A `Series`-typed field (`sample_weights`, `class_weights`, `groups`,
`label_order`, `stratify`, `parameter_lower_bounds`, …) defaults to
`pluss::series::undefined()` and means "not set" — leave it undefined to skip that
behavior. Fields typed `std::optional<...>` default to `std::nullopt` with the
same meaning.

## Core Verbs — Batch, Train, Infer, Assess

### `options::DFBatch` — `DataFrameBatchOptions`

Names the columns a `DataFrame` verb should read. Required wherever a verb takes
a `DataFrame` instead of an `(x, y)` pair.

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `feature_names` | `std::vector<std::string>` | `{}` | Column names to use as features, in order. Empty is rejected by the verbs that need features. |
| `label_feature` | `std::optional<std::string>` | `std::nullopt` | Column holding the target. Leave unset for unsupervised / inference-only calls. |
| `sample_weight_feature` | `std::optional<std::string>` | `std::nullopt` | Column of per-row weights. |
| `layout` | `ObservationLayout` | `Rows` | `Rows` = one sample per row; `Columns` = transposed. |
| `slate_policy` | `DataSlatePolicy` | `CastTemporalToSeries` | How temporal columns are coerced into numeric features. |
| `note_policy` | `DataNotePolicy` | `Reject` | What to do with text columns: `Reject`, or `Factorize` to integer-code them. |
| `note_hash_seed` | `std::uint64_t` | `1469598103934665603` | Seed used only when `note_policy == Factorize`. |
| `factorize_unknown_categorical` | `UnknownCategoryPolicy` | `MissingBucket` | Fate of a category unseen at fit time on a reused map: `Error`, `MissingBucket`, `Ignore`. |

### `options::Train` — `TrainOptions`

Per-fit configuration. Every `learn(...)` / `learn_(...)` overload takes one.

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `layout` | `ObservationLayout` | `Rows` | `Rows` or `Columns`. |
| `missing` | `MissingPolicy` | `Reject` | Missing-cell handling: `Reject`, `Drop`, `ImputeMean`, `ImputeMedian`, `KeepNaN`. |
| `unknown_categorical` | `UnknownCategoryPolicy` | `Error` | Unseen category at fit: `Error`, `MissingBucket`, `Ignore`. |
| `max_iterations` | `std::int64_t` | `1000` | Iteration cap for iterative learners. `> 0`. |
| `tolerance` | `double` | `1e-6` | Convergence tolerance. `> 0`. |
| `warm_start` | `bool` | `false` | Reuse the previous fit's parameters as the starting point. |
| `track_diagnostics` | `bool` | `true` | Record per-iteration diagnostics. |
| `random` | `RandomState` | `{seed = 0, deterministic = true}` | Seed + determinism for stochastic training. |
| `sample_weights` | `Series` | undefined | Per-row weights; length must match the sample count. |
| `class_weight` | `ClassWeightMode` | `None` | `None`, `Balanced` (inverse class frequency), or `Custom` (reads `class_weights`). |
| `class_weights` | `Series` | undefined | Used when `class_weight == Custom`. Binary: `[negative_weight, positive_weight]`. |
| `calibration` | `ScoreCalibration` | `None` | Probability calibration: `None`, `Platt`, `Isotonic`, `Temperature`. |
| `calibration_iterations` | `std::int64_t` | `400` | Iteration cap for the calibrator. |
| `calibration_tolerance` | `double` | `1e-6` | Convergence tolerance for the calibrator. |
| `parameter_lower_bounds` / `parameter_upper_bounds` | `Series` | undefined | Optional per-coefficient bounds for bound-constrained linear updates. |
| `balance` | `BalanceTarget` | `{kind = Keep}` | Row rebalancing applied before fitting (over/under-sampling). `Keep` = leave data unchanged. |

### `options::Infer` — `InferOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `layout` | `ObservationLayout` | `Rows` | `Rows` or `Columns`. |
| `missing` | `MissingPolicy` | `Reject` | As `TrainOptions::missing`. |
| `unknown_categorical` | `UnknownCategoryPolicy` | `Error` | As `TrainOptions::unknown_categorical`. |
| `positive_threshold` | `double` | `0.5` | Probability cutoff for the positive class in binary classification. `0 < t < 1`. Feed a tuned [`best_threshold`](reports/threshold_curve.md) here. |

### `options::Assess` — `AssessOptions`

Which metrics `assess(...)` / cross-validation `run(...)` compute.

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `layout` | `ObservationLayout` | `Rows` | `Rows` or `Columns`. |
| `metrics` | `std::vector<MetricKind>` | `{}` | Metrics to compute; empty = the task's default set. See [Shared Enumerations](#shared-enumerations) for `MetricKind`. |
| `sample_weights` | `Series` | undefined | Per-row weights for the metric computation. |
| `positive_threshold` | `double` | `0.5` | Cutoff for metrics that need hard labels (Accuracy, F1, …). `0 < t < 1`. |
| `regression_quantile` | `double` | `0.5` | Quantile for `MetricKind::QuantileLoss`. `0 < q < 1`. |

## Validation And Tuning

### `options::CrossVal` — `selection::CrossValOptions`

Full field reference: [`selection/validate/CrossValOptions`](selection/validate/CrossValOptions.md).

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `plan` | `CVPlanKind` | `CrossVal` | `Holdout`, `CrossVal`, `StratifiedCrossVal`, `GroupCrossVal`, `TSCrossVal`. |
| `holdout_fraction` | `double` | `0.2` | Test share when `plan == Holdout`. `0 < f < 1`. |
| `folds` | `std::int64_t` | `5` | Fold count for the cross-validation plans. `> 1`. |
| `repeats` | `std::int64_t` | `1` | Repeated-CV passes (`StratifiedCrossVal`). `> 0`. |
| `time_gap` / `time_test_size` / `time_max_train_size` | `std::int64_t` | `0` | Chronological-CV windows (`TSCrossVal`). `0` = inferred / unbounded. |
| `stratified` | `bool` | `false` | Force class-stratified folds. |
| `groups` | `Series` | undefined | Per-sample group id for `GroupCrossVal`; length must match the sample count. |
| `reliability_defaults` | `bool` | `true` | Apply the conservative default guards for small folds. |
| `shuffle` | `bool` | `true` | Shuffle before splitting. |
| `max_parallel` | `std::int64_t` | `0` | Fold workers. `<= 0` = auto-size to hardware concurrency. |
| `random_state` | `std::uint64_t` | `0` | Shuffle seed. |
| `deterministic` | `bool` | `true` | Fix the RNG for reproducible folds. |

### `options::Split` — `SplitOptions`

The low-level split policy the search / spot-check verbs accept directly (what
`CrossValOptions` compiles down to).

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `plan` | `CVPlanKind` | `Holdout` | As above. |
| `holdout_fraction` | `double` | `0.2` | `0 < f < 1`. |
| `folds` | `std::int64_t` | `5` | `> 1` for the CV plans. |
| `repeats` | `std::int64_t` | `1` | For `StratifiedCrossVal`. |
| `time_gap` / `time_test_size` / `time_max_train_size` | `std::int64_t` | `0` | Chronological windows; `<= 0` = inferred / expanding. |
| `stratified` | `bool` | `false` | Class-stratified folds. |
| `groups` | `Series` | undefined | Group ids for `GroupCrossVal`. |
| `reliability_defaults` | `bool` | `true` | Small-fold guards. |
| `shuffle` | `bool` | `true` | Shuffle before splitting. |
| `max_parallel` | `std::int64_t` | `0` | Fold workers; `<= 0` = auto. |
| `random` | `RandomState` | `{seed = 0, deterministic = true}` | Split RNG. |

### `options::Tune` — `TuneOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `search` | `SearchKind` | `None` | `None`, `Grid`, `Random`, `Bayesian`, `SuccessiveHalving`. |
| `trials` | `std::int64_t` | `0` | Sampled-search budget. `0` with `Grid` = exhaustive. |
| `max_parallel` | `std::int64_t` | `1` | Concurrent trials. |
| `split` | `SplitOptions` | `{}` | Inner-loop evaluation split (see above). |
| `random` | `RandomState` | `{seed = 0, deterministic = true}` | Sampler RNG. |

### `options::Fit` — `selection::FitOptions`

Re-fit policy after a comparison / search ranks its candidates.

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `fit_top` | `std::int64_t` | `0` | Re-fit the best `n` candidates on the full data. `0` = none. |
| `fit_bottom` | `std::int64_t` | `0` | Re-fit the worst `n` (for diagnostics). `0` = none. |

### `options::HoldoutSplit` — `prep::HoldoutSplitOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `test_size` | `double` | `0.2` | Test share. `0 < f < 1`. |
| `stratify` | `Series` | undefined | Per-row stratum labels. |
| `stratify_feature` | `std::string` | `""` | Column name to stratify on (alternative to `stratify`). |
| `random_state` | `std::uint64_t` | `0` | Shuffle seed. |
| `deterministic` | `bool` | `true` | Fix the RNG. |
| `shuffle` | `bool` | `true` | Shuffle before the cut. |

### `options::TimeSplit` — `prep::TimeSeriesSplitOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `folds` | `std::int64_t` | `5` | Chronological fold count. |
| `gap` | `std::int64_t` | `0` | Rows skipped between each train block and its test block. |
| `test_size` | `std::int64_t` | `0` | Rows per test block. `0` = inferred from layout. |
| `max_train_size` | `std::int64_t` | `0` | Cap on train-block rows. `0` = expanding window. |
| `random_state` | `std::uint64_t` | `0` | Seed (only affects tie-breaks). |
| `deterministic` | `bool` | `true` | Fix the RNG. |

## Workflow And Shape

### `options::Workflow` — `workflow::WorkflowOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `workers` | `std::int64_t` | `1` | Branch-parallel workers. |
| `schedule` | `WorkflowSchedule` | `Deterministic` | `Deterministic` (stable order) or `Adaptive` (work-stealing). |
| `cache` | `WorkflowCache` | `Off` | `Off`, or `Stage` to memoize per-stage output between runs. |

### `options::Time` — `workflow::TimeOptions`

Full field reference: [`workflow/TimeOptions`](workflow/TimeOptions.md).

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `index` | `std::string` | `""` | Timeline column name. Empty = the workflow is not time-aware. |
| `observed_at` / `target_at` | `std::string` | `""` | Reserved: observation-time / target-time column names for a future split. |
| `horizon` | `TimeOffset` | `0` | How far ahead a fold's test window reaches, in rows. `>= 0`. |
| `gap` | `TimeOffset` | `0` | Rows skipped between train and test windows. `>= 0`. |
| `lookback` | `TimeOffset` | `0` | Cap on training rows per fold. `0` = no cap. |
| `require_ordered` | `bool` | `true` | Reserved: whether the index must already be sorted ascending. |
| `reject_missing_index` | `bool` | `true` | When true, an empty `index` is a hard error. |

### `options::Profile` — `shape::ProfileOptions`

Thresholds for the feature-distribution profiler.

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `skew_threshold` | `double` | `1.0` | \|skew\| above this flags a feature as skewed. |
| `heavy_tail_threshold` | `double` | `3.0` | Excess kurtosis above this flags a heavy tail. |
| `low_variance_threshold` | `double` | `1e-12` | Variance below this flags a near-constant feature. |

## Reporting

### `options::ClassReport` — `ClassReportOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `label_order` | `Series` | undefined | Explicit class order for the report rows. |
| `zero_division` | `double` | `0.0` | Value substituted when a precision/recall denominator is zero. Keep in `[0, 1]`. |
| `sample_weights` | `Series` | undefined | Per-row weights. |
| `balance_quality_alpha` | `double` | `0.1` | Trade-off parameter of the balance-quality score. |
| `square_balance_quality` | `bool` | `true` | Square the balance-quality term. |

### `options::Confusion` — `ConfusionOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `normalize` | `ConfusionNormalize` | `None` | `None` (counts), `ByTruth` (row rates), `ByPrediction` (column rates), `Global` (share of all). |
| `label_order` | `Series` | undefined | Explicit class order. |
| `sample_weights` | `Series` | undefined | Per-row weights. |

## Model-Family Options

Every trainable family's construction options carry a short `options::` spelling;
pass either spelling to that family's factory
(`models::linear_regression(options::Linear{...})` and
`models::linear_regression(LinearFamilyOptions{...})` build the same model).

### `options::Linear` — `LinearFamilyOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `objective` | `LinearObjective` | `LeastSquares` | `LeastSquares`, `Logistic`, `Hinge`, `RobustLeast`. |
| `regularization` | `RegularizationKind` | `L2` | `None`, `L1`, `L2`, `Elastic`. |
| `regularization_strength` | `double` | `1e-6` | Penalty weight. `>= 0`; larger shrinks coefficients harder. |
| `elastic_l1_ratio` | `double` | `0.5` | Only for `Elastic`: `1.0` = pure L1, `0.0` = pure L2. |
| `robust_transition` | `double` | `1.35` | Only for `RobustLeast`: residual magnitude where loss switches quadratic → linear. |
| `fit_intercept` | `bool` | `true` | Fit a bias term. |

### `options::Tree` — `TreeFamilyOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `criterion` | `TreeCriterion` | `Gini` | `Gini`, `Entropy` (classification); `Mse`, `Mae` (regression). |
| `max_depth` | `std::int64_t` | `-1` | Depth cap. `-1` = grow until pure / leaf limits hit. |
| `min_samples_leaf` | `std::int64_t` | `1` | Minimum rows in a leaf. `>= 1`. |
| `min_samples_split` | `std::int64_t` | `2` | Minimum rows to consider splitting a node. `>= 2`. |

### `options::Ensemble` — `EnsembleFamilyOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `method` | `EnsembleMethod` | `RandomForest` | `Bagging`, `RandomForest`, `GradientBoost`, `AdaBoost`. |
| `learners` | `std::int64_t` | `100` | Number of base estimators. `> 0`. |
| `subsample` | `double` | `1.0` | Row fraction per learner. `0 < s <= 1`. |
| `learning_rate` | `double` | `0.1` | Shrinkage for the boosting methods. `> 0`. |

### `options::Neighbor` — `NeighborFamilyOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `k` | `std::int64_t` | `5` | Neighbors for `KNearest`. `> 0`. |
| `query_mode` | `NeighborQueryMode` | `KNearest` | `KNearest` or `Radius`. |
| `radius` | `double` | `1.0` | Neighborhood radius for `Radius` mode. `> 0`. |
| `radius_fallback_k` | `std::int64_t` | `1` | Neighbors to fall back to when a radius query finds none. |
| `distance_weighted` | `bool` | `false` | Weight neighbors by inverse distance. |
| `standardize` | `bool` | `true` | Standardize features before distance computation. |

### `options::Kernel` — `KernelFamilyOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `kernel` | `KernelKind` | `Rbf` | `Linear`, `Rbf`, `Polynomial`, `Sigmoid`. |
| `form` | `KernelForm` | `C` | `C` (C-SVM), `Nu` (ν-SVM), `OneClass`. |
| `c` | `double` | `1.0` | Penalty for the `C` form. `> 0`. |
| `gamma` | `double` | `0.0` | Kernel coefficient. `0` = use `1 / n_features`. |
| `epsilon` | `double` | `0.1` | ε-insensitive band for regression. `>= 0`. |
| `nu` | `double` | `0.5` | Support-vector fraction for the `Nu` / `OneClass` forms. `0 < nu <= 1`. |

### `options::Bayes` — `BayesFamilyOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `likelihood` | `BayesLikelihood` | `Gaussian` | `Gaussian`, `Bernoulli`, `Multinomial`, `Complement`. |
| `use_gaussian` | `bool` | `true` | Compatibility shim: `false` selects a discrete mode when `likelihood` is left `Gaussian`. |
| `smoothing` | `double` | `1e-9` | Additive smoothing / variance floor. `>= 0`. |

### `options::Cluster` — `ClusterFamilyOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `method` | `ClusterMethod` | `KMeans` | `KMeans`. |
| `clusters` | `std::int64_t` | `8` | Cluster count. `> 0`. |
| `max_iterations` | `std::int64_t` | `300` | Iteration cap. `> 0`. |
| `restarts` | `std::int64_t` | `4` | Random restarts; best inertia wins. `> 0`. |
| `tolerance` | `double` | `1e-4` | Centroid-shift convergence tolerance. `> 0`. |

### `options::Decompose` — `DecomposeFamilyOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `method` | `DecomposeMethod` | `Pca` | `Pca`. |
| `components` | `std::int64_t` | `-1` | Components to keep. `-1` = all. |
| `whiten` | `bool` | `false` | Scale components to unit variance. |
| `max_iterations` | `std::int64_t` | `500` | Iteration cap for the iterative solver. |
| `tolerance` | `double` | `1e-6` | Convergence tolerance. |

### `options::Detect` — `DetectFamilyOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `method` | `DetectMethod` | `Isolation` | `Isolation`. |
| `isolation` | `prep::OutlierIsolatorOptions` | `{}` | The isolation-forest settings — see [`options::OutlierIsolator`](#prep-stage-options). |

### `options::Transform` — `TransformOptions`

Not a struct — a `std::variant` that holds exactly one prep-stage `*Options`
value (default-constructed as `StandardScalerOptions`). It is how a single
transform stage carries its configuration; assign any one of the prep-stage
option structs below into it.

## Prep-Stage Options

Every `prep::stage::*` / prep wrapper factory takes its own `*Options` struct.
Ten already have a dedicated field page; the rest are tabled here in full.

### With a dedicated page

| `options::` alias | Underlying type | Key fields |
| --- | --- | --- |
| `options::StandardScaler` | [`prep::StandardScalerOptions`](prep/scaling/StandardScalerOptions.md) | `with_centering`, `with_scaling`, `variance_floor` |
| `options::MinMaxScaler` | [`prep::MinMaxScalerOptions`](prep/scaling/MinMaxScalerOptions.md) | `lower`, `upper`, `clip`, `range_floor` |
| `options::MaxAbsScaler` | [`prep::MaxAbsScalerOptions`](prep/scaling/MaxAbsScalerOptions.md) | `floor` |
| `options::RobustScaler` | [`prep::RobustScalerOptions`](prep/scaling/RobustScalerOptions.md) | `with_centering`, `with_scaling`, `lower_quantile`, `upper_quantile`, `range_floor` |
| `options::OneHotEncoder` | [`prep::OneHotEncoderOptions`](prep/encoding/OneHotEncoderOptions.md) | `max_categories`, `overflow`, `include_unknown_bucket` |
| `options::OrdinalEncoder` | [`prep::OrdinalEncoderOptions`](prep/encoding/OrdinalEncoderOptions.md) | `max_categories`, `overflow`, `include_unknown_bucket`, `one_based` |
| `options::LabelEncoder` | [`prep::LabelEncoderOptions`](prep/encoding/LabelEncoderOptions.md) | `max_categories`, `overflow`, `include_unknown_bucket`, `one_based` |
| `options::BaseNEncoder` | [`prep::BaseNEncoderOptions`](prep/encoding/BaseNEncoderOptions.md) | `base`, `max_categories`, `overflow` |
| `options::HashingEncoder` | [`prep::HashingEncoderOptions`](prep/encoding/HashingEncoderOptions.md) | `n_components`, `seed` |
| `options::FrequencyEncoder` | [`prep::FrequencyEncoderOptions`](prep/encoding/FrequencyEncoderOptions.md) | `max_categories`, `overflow`, `unknown_frequency` |

### `options::CategoryIndexer` — `prep::CategoryIndexerOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `max_categories` | `std::int64_t` | `256` | Cap on distinct categories learned per feature. |
| `overflow` | `CategoryOverflowPolicy` | `KeepMostFrequent` | `Error`, `KeepMostFrequent`, `SkipFeature` when the cap is exceeded. |
| `include_unknown_bucket` | `bool` | `true` | Reserve a code for unseen categories. |
| `one_based` | `bool` | `true` | Start codes at 1 (`false` = start at 0). |

### `options::SimpleImputer` — `prep::SimpleImputerOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `strategy` | `SimpleImputerStrategy` | `Mean` | `Mean`, `Median`, `MostFrequent`, `Constant`. |
| `constant_value` | `double` | `0.0` | Fill value when `strategy == Constant`. |
| `empty_feature_fallback` | `double` | `0.0` | Fill used when a feature is entirely missing at fit time. |

### `options::IterativeImputer` — `prep::IterativeImputerOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `rounds` | `std::int64_t` | `8` | Round-robin regression passes over the features. `> 0`. |
| `ridge` | `double` | `1e-6` | Ridge penalty on each per-feature regression. `>= 0`. |
| `empty_feature_fallback` | `double` | `0.0` | Fill for an all-missing feature. |

### `options::NeighborImputer` — `prep::NeighborImputerOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `neighbors` | `std::int64_t` | `5` | Donor rows per imputed cell. `> 0`. |
| `weighting` | `NeighborImputerWeighting` | `Distance` | `Uniform` or `Distance` (inverse-distance weighted). |
| `distance_floor` | `double` | `1e-12` | Lower clamp on distance to avoid divide-by-zero. |
| `empty_feature_fallback` | `double` | `0.0` | Fill for an all-missing feature. |

### `options::TargetEncoder` — `prep::TargetEncoderOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `smoothing` | `double` | `10.0` | Weight of the global mean in the per-category blend. `>= 0`; larger = more shrinkage toward the prior. |
| `max_categories` | `std::int64_t` | `4096` | Category cap. |
| `overflow` | `CategoryOverflowPolicy` | `KeepMostFrequent` | `Error`, `KeepMostFrequent`, `SkipFeature`. |

### `options::WeightOfEvidence` — `prep::WOEEncoderOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `regularization` | `double` | `1.0` | Laplace count added to each class tally before the log-odds. `>= 0`. |
| `max_categories` | `std::int64_t` | `4096` | Category cap. |
| `overflow` | `CategoryOverflowPolicy` | `KeepMostFrequent` | `Error`, `KeepMostFrequent`, `SkipFeature`. |

### `options::OutlierIsolator` — `prep::OutlierIsolatorOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `estimators` | `std::int64_t` | `100` | Isolation trees. `> 0`. |
| `max_samples` | `std::int64_t` | `256` | Rows sampled per tree (used when `max_samples_auto == false`). |
| `max_samples_auto` | `bool` | `true` | Auto-pick `max_samples` from the row count. |
| `max_features_ratio` | `double` | `1.0` | Feature fraction per tree. `0 < r <= 1`. |
| `bootstrap` | `bool` | `false` | Sample rows with replacement. |
| `contamination_auto` | `bool` | `true` | Derive the outlier threshold from the score distribution. |
| `contamination` | `double` | `0.1` | Expected outlier fraction when `contamination_auto == false`. `0 < c < 0.5`. |
| `random` | `RandomState` | `{seed = 0, deterministic = true}` | Forest RNG. |

### `options::FunctionTransformer` — `prep::FunctionTransformerOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `validate_input` | `bool` | `true` | Check the input frame (finite, expected columns) before calling the function. |
| `preserve_sample_count` | `bool` | `true` | Require the function to return the same row count it was given. |

### `options::PolynomialLift` — `prep::PolynomialLiftOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `degree` | `std::int64_t` | `2` | Highest polynomial degree generated. `>= 1`. |
| `interactions_only` | `bool` | `false` | Emit only cross-terms, no pure powers. |
| `include_bias` | `bool` | `false` | Add a constant `1` feature. |

### `options::PowerMap` — `prep::PowerMapOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `mode` | `PowerMapMode` | `YeoJohnson` | `YeoJohnson` (any real input) or `BoxCox` (strictly positive input). |
| `standardize` | `bool` | `true` | Standardize after the power transform. |
| `lambda` | `std::optional<double>` | `std::nullopt` | Fixed transform parameter; unset = fit it per feature. |
| `lambda_min` / `lambda_max` | `double` | `-5.0` / `5.0` | Search bounds when `lambda` is fit. |
| `lambda_grid` | `std::int64_t` | `201` | Grid resolution of the λ search. |
| `variance_floor` | `double` | `1e-12` | Lower clamp on variance during standardization. |

### `options::QuantileMap` — `prep::QuantileMapOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `output` | `QuantileMapOutput` | `Uniform` | `Uniform` (map to `[0, 1]`) or `Gaussian` (map to a normal). |
| `clip` | `double` | `1e-7` | Quantiles are clipped to `[clip, 1 - clip]` before the inverse CDF. |

### `options::ThresholdBinarizer` — `prep::ThresholdBinarizerOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `threshold` | `double` | `0.0` | Values `> threshold` become `high_value`, the rest `low_value`. |
| `low_value` | `double` | `0.0` | Output for values at or below the threshold. |
| `high_value` | `double` | `1.0` | Output for values above the threshold. |

### `options::VectorNormalizer` — `prep::VectorNormalizerOptions`

| Field | Type | Default | Meaning / values |
| --- | --- | --- | --- |
| `norm` | `VectorNorm` | `L2` | `L1`, `L2`, `Linf` — the per-row norm scaled to 1. |
| `floor` | `double` | `1e-12` | Rows with norm below this are left unchanged (no divide-by-zero). |

## Shared Enumerations

| Enum | Values |
| --- | --- |
| `ObservationLayout` | `Rows`, `Columns` |
| `MissingPolicy` | `Reject`, `Drop`, `ImputeMean`, `ImputeMedian`, `KeepNaN` |
| `UnknownCategoryPolicy` | `Error`, `MissingBucket`, `Ignore` |
| `ClassWeightMode` | `None`, `Balanced`, `Custom` |
| `ScoreCalibration` | `None`, `Platt`, `Isotonic`, `Temperature` |
| `CVPlanKind` | `Holdout`, `CrossVal`, `StratifiedCrossVal`, `GroupCrossVal`, `TSCrossVal` |
| `SearchKind` | `None`, `Grid`, `Random`, `Bayesian`, `SuccessiveHalving` |
| `ConfusionNormalize` | `None`, `ByTruth`, `ByPrediction`, `Global` |
| `CategoryOverflowPolicy` | `Error`, `KeepMostFrequent`, `SkipFeature` |
| `SimpleImputerStrategy` | `Mean`, `Median`, `MostFrequent`, `Constant` |
| `NeighborImputerWeighting` | `Uniform`, `Distance` |
| `VectorNorm` | `L1`, `L2`, `Linf` |
| `QuantileMapOutput` | `Uniform`, `Gaussian` |
| `PowerMapMode` | `YeoJohnson`, `BoxCox` |
| `WorkflowSchedule` | `Deterministic`, `Adaptive` |
| `WorkflowCache` | `Off`, `Stage` |
| `MetricKind` | classification: `Accuracy`, `BalancedAccuracy`, `Precision`, `Recall` / `Sensitivity`, `Specificity`, `F1`, `GeometricMean`, `BalanceQuality`, `RocAuc`, `LogLoss`, `Mcc`, `CohenKappa`, `Gini`, `AveragePrecision`, `Brier`; regression: `Mse`, `Rmse`, `Mae`, `R2`, `QuantileLoss` |
| `RandomState` | not an enum — `{std::uint64_t seed = 0; bool deterministic = true;}` |

## Usage

### Example 1

The cross-cutting verb options: `options::DFBatch` names the columns,
`options::Train` configures the fit, `options::Assess` picks which metrics
`assess(...)` returns.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame frame({
      {"income", Slate(pluss::make_series(
          {1.0, 1.2, 1.4, 1.1, 3.0, 3.2, 3.4, 3.1}, {8}, f64))},
      {"balance", Slate(pluss::make_series(
          {0.5, 0.7, 0.6, 0.8, 2.5, 2.7, 2.6, 2.8}, {8}, f64))},
      {"default", Slate(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  lymph::options::DFBatch batch{};
  batch.feature_names = {"income", "balance"};
  batch.label_feature = "default";

  lymph::options::Train train{};
  train.max_iterations = 250;
  train.class_weight = lymph::ClassWeightMode::Balanced;

  lymph::options::Assess assess{};
  assess.metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::F1,
                    lymph::MetricKind::Mcc};

  lymph::models::Model model = lymph::models::logistic_regression();
  model.learn_(frame, batch, train);

  const lymph::metrics::MetricReport report = model.assess(frame, batch, assess);
  std::cout << "requested metric count = " << report.requested.size() << "\n";
  std::cout << "accuracy = " << report.accuracy.item<double>() << "\n";
  std::cout << "f1       = " << report.f1.item<double>() << "\n";
  std::cout << "mcc      = " << report.mcc.item<double>() << "\n";

  return 0;
}
```

Output

```text
requested metric count = 3
accuracy = 1
f1       = 1
mcc      = 1
```

### Example 2

A model-family options struct. `options::Linear::regularization_strength`
shrinks the fitted slope toward zero — a large value pulls the prediction back
toward the target mean.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame x({
      {"x", Slate(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0}, {10}, f64))},
  });
  const Series y = pluss::make_series(
      {10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0, 100.0}, {10}, f64);
  const DataFrame probe({{"x", Slate(pluss::make_series({10.0}, {1}, f64))}});

  lymph::options::Linear light{};
  light.regularization = lymph::RegularizationKind::L2;
  light.regularization_strength = 1e-6;      // effectively ordinary least squares

  lymph::options::Linear heavy{};
  heavy.regularization = lymph::RegularizationKind::L2;
  heavy.regularization_strength = 1e3;       // strong shrinkage

  lymph::models::Model ols = lymph::models::linear_regression(light);
  ols.learn_(x, y);
  lymph::models::Model ridge = lymph::models::linear_regression(heavy);
  ridge.learn_(x, y);

  std::cout << "true value at x = 10                 : 100\n";
  std::cout << "light (strength 1e-6) prediction     : " << ols.infer(probe).item<double>() << "\n";
  std::cout << "heavy (strength 1e3)  prediction     : " << ridge.infer(probe).item<double>() << "\n";

  return 0;
}
```

Output

```text
true value at x = 10                 : 100
light (strength 1e-6) prediction     : 100
heavy (strength 1e3)  prediction     : 58.4296
```

### Example 3

A prep-stage options struct, reached through its `options::` alias.
`options::SimpleImputer::strategy` decides what fills a missing cell;
`constant_value` supplies the fill when `strategy == Constant`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <cmath>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame frame({
      {"score", Slate(pluss::make_series(
          {1.0, 2.0, std::nan(""), 4.0, 5.0}, {5}, f64))},
  });

  lymph::options::SimpleImputer median{};
  median.strategy = lymph::prep::SimpleImputerStrategy::Median;

  lymph::options::SimpleImputer constant{};
  constant.strategy = lymph::prep::SimpleImputerStrategy::Constant;
  constant.constant_value = -1.0;

  lymph::prep::SimpleImputer by_median(median);
  by_median.features({"score"});
  const DataFrame filled_median = by_median.learn_map(frame);

  lymph::prep::SimpleImputer by_constant(constant);
  by_constant.features({"score"});
  const DataFrame filled_constant = by_constant.learn_map(frame);

  std::cout << "median fill value  = " << by_median.summary().statistics << "\n";
  std::cout << "median result      = " << filled_median.features("score").as_series() << "\n";
  std::cout << "constant result    = " << filled_constant.features("score").as_series() << "\n";

  return 0;
}
```

Output

```text
median fill value  = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[3]
median result      = pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1, 2, 3, 4, 5]
constant result    = pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1, 2, -1, 4, 5]
```

## Related Pages

- [`prep`](prep.md) — most encoder / scaler / imputer options live here, several
  with a dedicated field page.
- [`models`](models.md) — the family options these factories take.
- [`selection`](selection.md) / [`workflow`](workflow.md) — `CrossVal`, `Fit`,
  `Tune`, `Split`, `Workflow`, `Time`.
- [`reports`](reports.md) — the umbrella pattern this mirrors, for report *types*
  rather than option structs.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

```
<!-- apiref:toctree:end -->

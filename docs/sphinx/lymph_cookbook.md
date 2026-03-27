# Lymph Cookbook

This cookbook is task-driven. Every recipe uses typed options and Lymph workflow semantics.
For full discrete combination enumeration, use the **Lymph Combination Atlas**.

## Training Options Reference

### API Signature

- `TrainOptions{ ... }`
- `InferOptions{ ... }`
- `AssessOptions{ ... }`
- `CrossValidationOptions{ ... }`
- `HoldoutSplitOptions{ ... }`

### Notes

These options are shared across all Lymph workflows and models.

### Inputs

- typed option structs from `pluss::ai::nodej`

### Output

- deterministic and configurable train/infer/assess behavior

### Options

`TrainOptions` fields:

- `layout`
- `missing`
- `unknown_categorical`
- `max_iterations`
- `tolerance`
- `warm_start`
- `track_diagnostics`
- `random.seed`
- `random.deterministic`
- `sample_weights`
- `class_weight`
- `class_weights`
- `calibration`
- `calibration_iterations`
- `calibration_tolerance`
- `parameter_lower_bounds`
- `parameter_upper_bounds`

`InferOptions` fields:

- `layout`
- `missing`
- `unknown_categorical`
- `positive_threshold`

`AssessOptions` fields:

- `layout`
- `metrics`
- `sample_weights`
- `positive_threshold`
- `regression_quantile`

`CrossValidationOptions` fields:

- `plan`
- `holdout_fraction`
- `folds`
- `stratified`
- `reliability_defaults`
- `shuffle`
- `random_state`
- `deterministic`

`HoldoutSplitOptions` fields:

- `test_size`
- `stratify`
- `stratify_feature`
- `random_state`
- `deterministic`
- `shuffle`

### Examples

```cpp
pluss::ai::nodej::TrainOptions train_opts{
    .max_iterations = 4000,
    .tolerance = 1e-8,
    .random = {.seed = 7, .deterministic = true},
    .track_diagnostics = true,
};

pluss::ai::nodej::AssessOptions assess_opts{
    .metrics = {
      pluss::ai::nodej::MetricKind::Rmse,
      pluss::ai::nodej::MetricKind::R2,
      pluss::ai::nodej::MetricKind::Mae,
    }
};
```

### Expected output

- same seed produces repeatable splits and learned parameters (within numeric tolerance).

## Preprocessing Recipes

### Simple Imputer For Numeric Gaps

#### API Signature

- `prep::SimpleImputer(options: SimpleImputerOptions = {}) -> SimpleImputer`
- `SimpleImputer::learn_map(frame: DataFrame, learn_options: TrainOptions = {}, infer_options: InferOptions = {}) -> DataFrame`

#### Notes

Fast baseline for missing values.

#### Inputs

- numeric or castable Series columns

#### Output

- DataFrame with imputed columns

#### Options

`SimpleImputerOptions`:

- `strategy`: `Mean`, `Median`, `MostFrequent`, `Constant`
- `constant_value`
- `empty_feature_fallback`

#### Examples

```cpp
auto imp = lym::prep::SimpleImputer({
    .strategy = lym::prep::SimpleImputerStrategy::Median,
});

const DataFrame train_imp = imp.learn_map(train_df);
const DataFrame test_imp = imp.map(test_df);
```

#### Expected output

- null/missing numeric entries are replaced using chosen strategy.

### Neighbor Imputer For Local Similarity

#### API Signature

- `prep::NeighborImputer(options: NeighborImputerOptions = {}) -> NeighborImputer`

#### Notes

Useful when local neighborhood structure matters.

#### Inputs

- dense enough feature matrix for nearest-neighbor lookup

#### Output

- imputed DataFrame

#### Options

`NeighborImputerOptions`:

- `neighbors`
- `weighting`: `Uniform`, `Distance`
- `distance_floor`
- `empty_feature_fallback`

#### Examples

```cpp
auto knn_imp = lym::prep::NeighborImputer({
    .neighbors = 7,
    .weighting = lym::prep::NeighborImputerWeighting::Distance,
});

const DataFrame x_train_i = knn_imp.learn_map(x_train);
const DataFrame x_test_i = knn_imp.map(x_test);
```

#### Expected output

- missing values are reconstructed from nearest rows.

### Iterative Imputer For Multivariate Imputation

#### API Signature

- `prep::IterativeImputer(options: IterativeImputerOptions = {}) -> IterativeImputer`

#### Notes

Useful when missingness depends on other features.

#### Inputs

- matrix with partially observed features

#### Output

- iteratively imputed DataFrame

#### Options

`IterativeImputerOptions`:

- `rounds`
- `ridge`
- `empty_feature_fallback`

#### Examples

```cpp
auto it_imp = lym::prep::IterativeImputer({
    .rounds = 10,
    .ridge = 1e-6,
});

const DataFrame x_train_i = it_imp.learn_map(x_train);
const DataFrame x_test_i = it_imp.map(x_test);
```

#### Expected output

- progressively stabilized imputations after configured rounds.

### Outlier Isolator Recipes

#### API Signature

- `prep::OutlierIsolator(options: OutlierIsolatorOptions = {}) -> OutlierIsolator`
- `prep::outlier_isolator(options: OutlierIsolatorOptions = {}) -> OutlierIsolator`

#### Notes

Isolation Forest-style anomaly detection wrapper on DataFrame series features.

#### Inputs

- DataFrame with numeric Series features

#### Output

- labels (`+1` inlier, `-1` outlier), anomaly scores, filtered DataFrame

#### Examples

Minimal (defaults):

```cpp
lym::prep::OutlierIsolator iso;   // default options
iso.learn(housing_df);            // train

auto labels = iso.infer(housing_df);         // Thread<int64>, +1 (inlier), -1 (outlier)
auto scores = iso.score_samples(housing_df); // Thread<float64>, lower => more anomalous
```

Configure options (seed, contamination, forest size):

```cpp
lym::prep::OutlierIsolatorOptions opts;
opts.estimators = 300;
opts.max_samples_auto = true;     // sklearn-style auto
opts.max_features_ratio = 0.8;
opts.bootstrap = false;
opts.contamination_auto = false;  // use explicit contamination
opts.contamination = 0.02;        // 2% outliers
opts.random.seed = 42;
opts.random.deterministic = true;

lym::prep::OutlierIsolator iso(opts);
iso.learn(housing_df);
auto labels = iso.infer(housing_df);
```

Use only selected features:

```cpp
lym::prep::OutlierIsolator iso;
iso.features({"median_income", "total_rooms", "population"})
   .learn(housing_df);

auto labels = iso.infer(housing_df);
```

One-shot train + infer:

```cpp
auto labels = lym::prep::outlier_isolator(
    lym::prep::OutlierIsolatorOptions{.estimators = 200, .max_features_ratio = 0.75}
).features({"median_income", "total_rooms"})
 .learn_infer(housing_df);
```

Train once, infer on new data:

```cpp
auto [train_df, test_df] = lym::hold_out_split(
    housing_df,
    lym::HoldoutSplitOptions{
        .test_size = 0.2,
        .random_state = 42
    });

lym::prep::OutlierIsolator iso(
    lym::prep::OutlierIsolatorOptions{.random = {.seed = 42, .deterministic = true}});
iso.learn(train_df);

auto test_scores = iso.score_samples(test_df);
auto test_labels = iso.infer(test_df);
```

Keep only inliers (or only outliers):

```cpp
lym::prep::OutlierIsolator iso;
iso.learn(housing_df);

auto clean_df = iso.filter_inliers(housing_df);                   // convenience helper
auto outlier_df = housing_df.filter(iso.infer(housing_df) == -1.0); // explicit
```

Custom threshold from raw scores:

```cpp
lym::prep::OutlierIsolator iso;
iso.learn(housing_df);

auto scores = iso.score_samples(housing_df);
// stricter than default decision boundary:
auto very_anomalous_df = housing_df.filter(scores < -0.62);
```

Inspect fitted model summary:

```cpp
auto iso = lym::prep::outlier_isolator(lym::prep::OutlierIsolatorOptions{.estimators = 256});
iso.features({"median_income", "total_rooms"}).learn(housing_df);

auto s = iso.summary();   // learned, feature_count, estimators, max_samples, max_features, contamination, offset, names
out.writeln(s);           // works with OutputEngine
```

Low-level NodeJ (without Lymph wrapper):

```cpp
#include "pluss/ai/nodej/nodej.h"

namespace nodej = pluss::ai::nodej;

// X must be a 2D numeric Series: [n_samples, n_features]
nodej::FeatureBatch batch{
    .x = X,
    .layout = nodej::ObservationLayout::Rows
};

nodej::prep::OutlierIsolator core(nodej::prep::OutlierIsolatorOptions{
    .random = {.seed = 42, .deterministic = true}
});
auto st = core.learn(batch);
if (!st.ok()) throw std::runtime_error(st.message);

auto labels_or = core.infer(batch);
if (!labels_or.ok()) throw std::runtime_error(labels_or.status().message);
auto labels = labels_or.move_value(); // Series (+1 / -1)
```

### Scaling Recipes

#### API Signature

- `prep::StandardScaler(options: StandardScalerOptions = {}) -> StandardScaler`
- `prep::MinMaxScaler(options: MinMaxScalerOptions = {}) -> MinMaxScaler`
- `prep::RobustScaler(options: RobustScalerOptions = {}) -> RobustScaler`
- `prep::MaxAbsScaler(options: MaxAbsScalerOptions = {}) -> MaxAbsScaler`

#### Notes

Choose scaler based on outlier sensitivity and feature distribution.

#### Inputs

- numeric feature columns

#### Output

- scaled DataFrame

#### Options

`StandardScalerOptions`:

- `with_centering`
- `with_scaling`
- `variance_floor`

`MinMaxScalerOptions`:

- `lower`
- `upper`
- `clip`
- `range_floor`

`RobustScalerOptions`:

- `with_centering`
- `with_scaling`
- `lower_quantile`
- `upper_quantile`
- `range_floor`

`MaxAbsScalerOptions`:

- `floor`

#### Examples

```cpp
auto scaler = lym::prep::StandardScaler({.with_centering = true, .with_scaling = true});
const DataFrame x_train_s = scaler.learn_map(x_train);
const DataFrame x_test_s = scaler.map(x_test);
```

#### Expected output

- transformed feature scales with train-test consistency.

### Encoding And Feature Mapping Recipes

#### API Signature

- `prep::OneHotEncoder(options: OneHotEncoderOptions = {}) -> OneHotEncoder`
- `prep::FrequencyEncoder(options: FrequencyEncoderOptions = {}) -> FrequencyEncoder`
- `prep::OrdinalEncoder(options: OrdinalEncoderOptions = {}) -> OrdinalEncoder`
- `prep::LabelEncoder(options: LabelEncoderOptions = {}) -> LabelEncoder`
- `prep::CategoryIndexer(options: CategoryIndexerOptions = {}) -> CategoryIndexer`
- `prep::QuantileMap(options: QuantileMapOptions = {}) -> QuantileMap`
- `prep::PowerMap(options: PowerMapOptions = {}) -> PowerMap`
- `prep::ThresholdBinarizer(options: ThresholdBinarizerOptions = {}) -> ThresholdBinarizer`
- `prep::PolynomialLift(options: PolynomialLiftOptions = {}) -> PolynomialLift`
- `prep::VectorNormalizer(options: VectorNormalizerOptions = {}) -> VectorNormalizer`

#### Notes

Use these to normalize non-Gaussian features, expand basis features, and encode categories.
For a single categorical column without a DataFrame wrapper, `lymph::prep::OneHotEncoder`
`lymph::prep::FrequencyEncoder`, `lymph::prep::OrdinalEncoder`, and
`lymph::prep::LabelEncoder` also support direct `pluss::table::Thread`
overloads (`learn/map/learn_map`).
After fitting, encoder wrappers expose fitted attrs directly:

- `feature_names_`: selected input feature names (DataFrame path)
- `categories_`: learned category labels per feature (string labels when available)
- `category_keys_`: raw numeric category keys used internally by core encoder
- `mapped_names_`: output column names in transform order
- `frequencies_`: learned category frequencies per feature (FrequencyEncoder)
- `state_`: raw serialized state tensors (advanced/debug)

For `Thread` input, if `Thread::display_name()` is set, Lymph uses it as the
base for `mapped_names_`; otherwise it falls back to `feature_0`, `feature_1`, etc.

`CategoryIndexer` specific role:

- purpose: learn/apply a stable categorical index map
- output form: integer-coded category IDs
- best for: lookup/index consistency, diagnostics, export, controlled category contracts
- not ideal alone for many linear models on nominal categories; prefer `OneHotEncoder` or `FrequencyEncoder` for model features

Encoder choice quick guide:

- nominal, low/moderate cardinality: `OneHotEncoder`
- nominal, high cardinality: `FrequencyEncoder` (or later target/WOE style encoders where appropriate)
- ordered categories: `OrdinalEncoder`
- categorical labels/targets or compact IDs: `LabelEncoder`
- need explicit stable IDs before downstream handling: `CategoryIndexer`

#### Inputs

- `OneHotEncoder`, `FrequencyEncoder`, `OrdinalEncoder`, `LabelEncoder`:
  - `DataFrame` (`learn/map/learn_map`)
  - `Thread` (`learn/map/learn_map`)
- other mappers in this section:
  - `DataFrame`

#### Output

- DataFrame path: mapped `DataFrame`
- Thread path (one-hot/frequency): mapped `Series`

#### Options

`OneHotEncoderOptions`:

- `max_categories`
- `overflow`: `Error`, `KeepMostFrequent`, `SkipFeature`
- `include_unknown_bucket`

`FrequencyEncoderOptions`:

- `max_categories`
- `overflow`
- `unknown_frequency`

`OrdinalEncoderOptions`:

- `max_categories`
- `overflow`
- `include_unknown_bucket`
- `one_based`

`LabelEncoderOptions`:

- `max_categories`
- `overflow`
- `include_unknown_bucket`
- `one_based`

`QuantileMapOptions`:

- `output`: `Uniform`, `Gaussian`
- `clip`

`PowerMapOptions`:

- `mode`: `YeoJohnson`, `BoxCox`
- `standardize`
- `lambda`
- `lambda_min`
- `lambda_max`
- `lambda_grid`
- `variance_floor`

`ThresholdBinarizerOptions`:

- `threshold`
- `low_value`
- `high_value`

`PolynomialLiftOptions`:

- `degree`
- `interactions_only`
- `include_bias`

`VectorNormalizerOptions`:

- `norm`: `L1`, `L2`, `Linf`
- `floor`

#### Examples

```cpp
// One-hot on a DataFrame subset; output is DataFrame with mapped columns.
auto cat = lym::prep::OneHotEncoder({.max_categories = 32});
cat.features({"ocean_proximity"});
DataFrame x_cat_train = cat.learn_map(train_df);
DataFrame x_cat_test = cat.map(test_df);

// Fitted attrs
const auto cat_names = cat.mapped_names_;
const auto cat_labels = cat.categories_;    // per-feature category labels
const auto cat_keys = cat.category_keys_;   // per-feature numeric keys

// Frequency encoding on DataFrame.
auto freq = lym::prep::FrequencyEncoder({
    .max_categories = 64,
    .unknown_frequency = 0.0,
});
freq.features({"ocean_proximity"});
DataFrame x_freq_train = freq.learn_map(train_df);
DataFrame x_freq_test = freq.map(test_df);

// Fitted attrs
const auto freq_names = freq.mapped_names_;
const auto freq_labels = freq.categories_;
const auto freq_keys = freq.category_keys_;
const auto freq_vals = freq.frequencies_;

// Ordinal encoding (order-preserving index map by sorted category keys).
auto ord = lym::prep::OrdinalEncoder({
    .max_categories = 64,
    .include_unknown_bucket = true,
    .one_based = true,
});
ord.features({"ocean_proximity"});
DataFrame x_ord_train = ord.learn_map(train_df);
DataFrame x_ord_test = ord.map(test_df);
const auto ord_names = ord.mapped_names_;
const auto ord_labels = ord.categories_;

// Label encoding (integer labels, default zero-based).
auto lbl = lym::prep::LabelEncoder({
    .max_categories = 64,
    .include_unknown_bucket = true,
    .one_based = false,
});
lbl.features({"ocean_proximity"});
DataFrame x_lbl_train = lbl.learn_map(train_df);
DataFrame x_lbl_test = lbl.map(test_df);
const auto lbl_names = lbl.mapped_names_;
const auto lbl_labels = lbl.categories_;

// Category indexing (grp2idx-style behavior) with explicit feature list.
auto indexer = lym::prep::CategoryIndexer({
    .max_categories = 64,
    .include_unknown_bucket = true,
    .one_based = true,
});
indexer.features({"ocean_proximity"});
DataFrame x_cat_idx_train = indexer.learn_map(train_df);
DataFrame x_cat_idx_test = indexer.map(test_df);

// Compose indexing with one-hot if you need strict ID contract + expansion.
auto idx_then_oh = lym::prep::OneHotEncoder({
    .max_categories = 64,
    .include_unknown_bucket = true,
});
idx_then_oh.features({"ocean_proximity"});
DataFrame x_cat_idx_oh_train = idx_then_oh.learn_map(x_cat_idx_train);
DataFrame x_cat_idx_oh_test = idx_then_oh.map(x_cat_idx_test);

// Single categorical Thread -> index Series
pluss::table::Thread cat_label = train_df.features("ocean_proximity");
Series y_cat_idx = indexer.learn_map(cat_label);

// Thread path for a single categorical feature.
pluss::table::Thread cat_thread = train_df.features("ocean_proximity");
auto cat_thread_enc = lym::prep::OneHotEncoder({
    .max_categories = 32,
    .overflow = lym::prep::CategoryOverflowPolicy::KeepMostFrequent,
    .include_unknown_bucket = true,
});
Series x_cat_series = cat_thread_enc.learn_map(cat_thread);

// Build named output columns from mapped_names_ if you need a new DataFrame.
std::vector<std::pair<std::string, pluss::table::Thread>> cols{};
const auto out_names = cat_thread_enc.mapped_names_;
for (std::int64_t j = 0; j < static_cast<std::int64_t>(out_names.size()); ++j) {
  Series idx = pluss::make_series(
      std::vector<std::int64_t>{j},
      {1},
      pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(x_cat_series.platform()));
  Series col = x_cat_series.index_select(1, idx).reshape({x_cat_series.sizes()[0]});
  cols.emplace_back(out_names[static_cast<std::size_t>(j)], pluss::table::Thread(col));
}
DataFrame x_cat_named(std::move(cols), train_df.cases());

// Inspect learned metadata to understand encoder mapping.
// categories_: human-readable category labels when available.
// category_keys_: numeric keys used internally by the core encoder.
// mapped_names_: transformed output feature names in emitted order.
const auto cats = cat.categories_;
const auto keys = cat.category_keys_;
const auto out_cols = cat.mapped_names_;

// Unknown-category handling at inference.
pluss::ai::nodej::InferOptions infer_opts{};
infer_opts.unknown_categorical = pluss::ai::nodej::UnknownCategoryPolicy::MissingBucket;
Series x_cat_unknown_safe = cat_thread_enc.map(test_df.features("ocean_proximity"), infer_opts);
Series x_ord_unknown_safe = ord.map(test_df.features("ocean_proximity"), infer_opts);
Series x_lbl_unknown_safe = lbl.map(test_df.features("ocean_proximity"), infer_opts);

// Continue with non-categorical feature mapping.
auto poly = lym::prep::PolynomialLift({.degree = 3, .include_bias = false});
DataFrame x3 = poly.learn_map(x_freq_train);
```

#### Expected output

- one-hot/frequency/ordinal/label categorical mappings with stable fitted metadata
- explicit unknown-category behavior via `InferOptions::unknown_categorical`
- named mapped outputs accessible from `mapped_names_` for downstream DataFrame assembly
- encoded and expanded feature matrix ready for model fitting

## Model Family Recipes

### API Signature

- `linear_factory::*`
- `tree_factory::*`
- `ensemble_factory::*`
- `neighbor_factory::*`
- `kernel_factory::*`
- `bayes_factory::*`
- `cluster_factory::*`
- `decompose_factory::*`

### Notes

Lymph factories are thin wrappers over NodeJ specs. Every family option is available.

### Inputs

- one family options struct

### Output

- `WorkflowModel`

### Options

`LinearFamilyOptions`:

- `objective`: `LeastSquares`, `Logistic`, `Hinge`
- `regularization`: `None`, `L1`, `L2`, `Elastic`
- `regularization_strength`
- `fit_intercept`

`TreeFamilyOptions`:

- `criterion`: `Gini`, `Entropy`, `Mse`, `Mae`
- `max_depth`
- `min_samples_leaf`
- `min_samples_split`

`EnsembleFamilyOptions`:

- `method`: `Bagging`, `RandomForest`, `GradientBoost`
- `learners`
- `subsample`

`NeighborFamilyOptions`:

- `k`
- `distance_weighted`
- `standardize`

`KernelFamilyOptions`:

- `kernel`: `Linear`, `Rbf`, `Polynomial`, `Sigmoid`
- `c`
- `gamma`
- `epsilon`

`BayesFamilyOptions`:

- `use_gaussian`
- `smoothing`

`ClusterFamilyOptions`:

- `method`
- `clusters`
- `max_iterations`
- `restarts`
- `tolerance`

`DecomposeFamilyOptions`:

- `method`
- `components`
- `whiten`
- `max_iterations`
- `tolerance`

### Examples

```cpp
auto lin = lym::linear_factory::linear_regression({
    .regularization = pluss::ai::nodej::RegularizationKind::L2,
    .regularization_strength = 1e-5,
    .fit_intercept = true,
});

auto krr = lym::kernel_factory::rbf_kernel_regressor({
    .c = 2.0,
    .gamma = 0.1,
    .epsilon = 0.05,
});
```

### Expected output

- model instances are immediately usable in `Workflow` or direct `WorkflowModel` calls.

## Workflow Recipes

### Recipe: Workflow stage order

#### API Signature

- `Workflow{ .model = ..., .stages = {...} }`

#### Notes

Stages execute in sequence; model consumes transformed output.

#### Inputs

- ordered stages

#### Output

- deterministic preprocessing + model fit path through one workflow

#### Options

- use `workflow::stage::*` with typed options

#### Examples

```cpp
lym::Workflow wf{
    .model = lym::ensemble_factory::random_forest_regressor({.learners = 300}),
    .stages = {
      lym::workflow::stage::simple_imputer({
          .strategy = lym::prep::SimpleImputerStrategy::Median
      }),
      lym::workflow::stage::robust_scaler(),
    },
};

wf.learn_(x_train, y_train);
auto pred = wf.infer(x_test);
auto metrics = wf.assess(x_test, y_test, pluss::ai::nodej::AssessOptions{
    .metrics = {
      pluss::ai::nodej::MetricKind::Rmse,
      pluss::ai::nodej::MetricKind::R2,
    }
});
```

#### Expected output

- train and inference use the exact same learned preprocessing state.

### Recipe: Feature-branch composition (numeric + categorical)

#### API Signature

- `workflow::feature::compose(transforms: list<TransformSpec>, feature_names: list<string>) -> Branch`
- `make_feature_transformer(feature_order: list<string>, branches: list<Branch>, remainder: FeatureRemainderPolicy = PassThrough) -> prep::FeatureTransformer`

#### Notes

Use this when different feature subsets need different transform chains.

#### Examples

```cpp
const std::vector<std::string> feature_order = x_train.feature_names();
const auto ft = lym::make_feature_transformer(
    feature_order,
    {
      lym::workflow::feature::compose(
          {lym::workflow::stage::simple_imputer({.strategy = lym::prep::SimpleImputerStrategy::Median}),
           lym::workflow::stage::standard_scaler()},
          {"median_income", "total_rooms", "population"}),
      lym::workflow::feature::compose(
          {lym::workflow::stage::one_hot_encoder({.include_unknown_bucket = true})},
          {"ocean_proximity"}),
    });
```

## Model Selection Recipes

### Recipe: K-fold shortlist and winner selection

#### API Signature

- `cross_validate(...) -> CrossValidationReport`
- `selection::tune(...) -> TuneResult`

#### Notes

Use `cross_validate` to inspect one candidate. Use `selection::tune` to compare many.

#### Inputs

- candidate workflows and objective metric

#### Output

- ranked trials and selected best workflow

#### Options

- choose objective metric + policy (`Minimize` for losses, `Maximize` for scores)

#### Examples

```cpp
std::vector<lym::selection::Candidate> candidates;
candidates.emplace_back("linear", lym::Workflow{.model = lym::linear_factory::linear_regression({})});
candidates.emplace_back("tree", lym::Workflow{.model = lym::tree_factory::decision_tree_regressor({.max_depth = 10})});
candidates.emplace_back("forest", lym::Workflow{.model = lym::ensemble_factory::random_forest_regressor({.learners = 250})});

auto tuned = lym::selection::tune(
    std::move(candidates),
    x_train,
    y_train,
    lym::CrossValidationOptions{.plan = pluss::ai::nodej::CVPlanKind::KFold, .folds = 5, .random_state = 123},
    {},
    pluss::ai::nodej::AssessOptions{.metrics = {pluss::ai::nodej::MetricKind::Rmse}},
    pluss::ai::nodej::MetricKind::Rmse,
    pluss::ai::nodej::TuneObjectivePolicy::Minimize);

auto best = std::move(tuned.best_workflow);
best.learn_(x_train, y_train);
```

#### Expected output

- `tuned.report.best_name` identifies selected model.
- `best` is a trained, ready-to-save workflow.

## Persistence Recipe

### API Signature

- `Workflow::save(...) -> void`
- `Workflow::load(path: string) -> Workflow`
- `max_abs_diff(lhs: Series, rhs: Series) -> double`

### Notes

Use prediction parity checks to validate restore integrity.

### Inputs

- trained workflow and train data

### Output

- serialized workflow bundle and restored object

### Options

- deterministic training settings strongly recommended

### Examples

```cpp
wf.save("final_model.bin", x_train, y_train, pluss::ai::nodej::TrainOptions{
    .random = {.seed = 1234, .deterministic = true}
});

auto loaded = lym::Workflow::load("final_model.bin");
const Series p1 = wf.infer(x_test);
const Series p2 = loaded.infer(x_test);
const double delta = lym::max_abs_diff(p1, p2);
```

### Expected output

- `delta` near zero for deterministic training conditions.

## Online Recipe

### API Signature

- `online_factory::linear_regressor(options: OnlineLinearOptions = {}) -> OnlineLinearRegressor`
- `OnlineLinearRegressor::learn_(...) -> OnlineLinearRegressor&`
- `OnlineLinearRegressor::advance_(...) -> OnlineLinearRegressor&`

### Notes

Online training updates the model incrementally without refitting from scratch.

### Inputs

- ordered stream chunks `(x_t, y_t)`

### Output

- continuously updated model

### Options

`OnlineLinearOptions`:

- `family`
- `learning_rate`
- `decaying_rate`

### Examples

```cpp
auto online = lym::online_factory::linear_regressor({
    .learning_rate = 0.03,
    .decaying_rate = true,
});

online.learn_(x_chunk0, y_chunk0);
online.advance_(x_chunk1, y_chunk1);
online.advance_(x_chunk2, y_chunk2);
```

### Expected output

- `online.seen_samples()` reflects total consumed sample count.

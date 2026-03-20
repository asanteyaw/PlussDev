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
- `prep::QuantileMap(options: QuantileMapOptions = {}) -> QuantileMap`
- `prep::PowerMap(options: PowerMapOptions = {}) -> PowerMap`
- `prep::ThresholdBinarizer(options: ThresholdBinarizerOptions = {}) -> ThresholdBinarizer`
- `prep::PolynomialLift(options: PolynomialLiftOptions = {}) -> PolynomialLift`
- `prep::VectorNormalizer(options: VectorNormalizerOptions = {}) -> VectorNormalizer`

#### Notes

Use these to normalize non-Gaussian features, expand basis features, and encode categories.

#### Inputs

- DataFrame with selected feature subset

#### Output

- mapped DataFrame

#### Options

`OneHotEncoderOptions`:

- `max_categories`
- `overflow`: `Error`, `KeepMostFrequent`, `SkipFeature`
- `include_unknown_bucket`

`FrequencyEncoderOptions`:

- `max_categories`
- `overflow`
- `unknown_frequency`

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
auto cat = lym::prep::OneHotEncoder({.max_categories = 32});
auto freq = lym::prep::FrequencyEncoder({.max_categories = 64});
auto poly = lym::prep::PolynomialLift({.degree = 3, .include_bias = false});

DataFrame x1 = cat.learn_map(x_train);
DataFrame x2 = freq.learn_map(x1);
DataFrame x3 = poly.learn_map(x2);
```

#### Expected output

- encoded and expanded feature matrix ready for model fitting.

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

### Recipe: sklearn-style pipeline order

#### API Signature

- `Workflow{ .model = ..., .stages = {...} }`

#### Notes

Stages execute in sequence; model consumes transformed output.

#### Inputs

- ordered stages

#### Output

- deterministic preprocessing + model fit path

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

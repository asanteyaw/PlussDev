# Lymph Combination Atlas

This page is the exhaustive option-combination map for Lymph.

Scope definition used here:

- **Exhaustive discrete coverage**: every enum/bool branch is enumerated.
- **Numeric fields**: continuous/integer ranges are infinite; these are covered with explicit sweep grids so users can fully enumerate practical search spaces.

## Global Training/Inference/Evaluation Combinations

### API Signature

- `TrainOptions{...}`
- `InferOptions{...}`
- `AssessOptions{...}`
- `CrossValidationOptions{...}`
- `HoldoutSplitOptions{...}`

### Notes

These options apply to all model families and workflows.

### Inputs

- typed options structs

### Output

- deterministic and exhaustive controllability across fitting and scoring behavior

### Options

Discrete dimensions:

`TrainOptions`

- `layout`: `{Rows, Columns}`
- `missing`: `{Reject, Drop, ImputeMean, ImputeMedian, KeepNaN}`
- `unknown_categorical`: `{Error, MissingBucket, Ignore}`
- `warm_start`: `{false, true}`
- `track_diagnostics`: `{false, true}`
- `class_weight`: `{None, Balanced, Custom}`
- `calibration`: `{None, Platt}`
- `random.deterministic`: `{false, true}`

Total discrete combinations for `TrainOptions` (excluding numeric/Series fields):

- `2 * 5 * 3 * 2 * 2 * 3 * 2 * 2 = 1440`

Numeric/Series sweep axes for completeness:

- `max_iterations`, `tolerance`
- `random.seed`
- `sample_weights`, `class_weights`
- `calibration_iterations`, `calibration_tolerance`
- `parameter_lower_bounds`, `parameter_upper_bounds`

`InferOptions`

- discrete: `layout(2) * missing(5) * unknown_categorical(3) = 30`
- numeric: `positive_threshold`

`AssessOptions`

- discrete base: `layout(2)`
- metric-set power space: all non-empty subsets of 12 metrics: `2^12 - 1 = 4095`
- threshold/quantile: numeric axes (`positive_threshold`, `regression_quantile`)

`CrossValidationOptions`

- `plan`: `{Holdout, KFold, StratifiedKFold, GroupKFold, TimeSeries}`
- `stratified`: `{false, true}`
- `reliability_defaults`: `{false, true}`
- `shuffle`: `{false, true}`
- `deterministic`: `{false, true}`

Discrete count (excluding numeric fields):

- `5 * 2 * 2 * 2 * 2 = 80`

Numeric axes: `holdout_fraction`, `folds`, `random_state`

`HoldoutSplitOptions`

- discrete: `deterministic(2) * shuffle(2) * stratify_mode(3)` where stratify mode is `{none, stratify, stratify_feature}` => `12`
- numeric: `test_size`, `random_state`

### Examples

```cpp
// Exhaustive discrete TrainOptions enumeration scaffold.
for (auto layout : {pluss::ai::nodej::ObservationLayout::Rows,
                    pluss::ai::nodej::ObservationLayout::Columns}) {
  for (auto missing : {pluss::ai::nodej::MissingPolicy::Reject,
                       pluss::ai::nodej::MissingPolicy::Drop,
                       pluss::ai::nodej::MissingPolicy::ImputeMean,
                       pluss::ai::nodej::MissingPolicy::ImputeMedian,
                       pluss::ai::nodej::MissingPolicy::KeepNaN}) {
    for (auto unknown : {pluss::ai::nodej::UnknownCategoryPolicy::Error,
                         pluss::ai::nodej::UnknownCategoryPolicy::MissingBucket,
                         pluss::ai::nodej::UnknownCategoryPolicy::Ignore}) {
      for (bool warm : {false, true}) {
        for (bool diag : {false, true}) {
          for (auto cw : {pluss::ai::nodej::ClassWeightMode::None,
                          pluss::ai::nodej::ClassWeightMode::Balanced,
                          pluss::ai::nodej::ClassWeightMode::Custom}) {
            for (auto cal : {pluss::ai::nodej::ScoreCalibration::None,
                             pluss::ai::nodej::ScoreCalibration::Platt}) {
              for (bool det : {false, true}) {
                pluss::ai::nodej::TrainOptions t{};
                t.layout = layout;
                t.missing = missing;
                t.unknown_categorical = unknown;
                t.warm_start = warm;
                t.track_diagnostics = diag;
                t.class_weight = cw;
                t.calibration = cal;
                t.random.deterministic = det;
                // add numeric sweep values for full experiment space.
              }
            }
          }
        }
      }
    }
  }
}
```

### Expected output

- iteration over every discrete branch combination for `TrainOptions`.

## Preprocessor Combination Atlas

### API Signature

- `prep::SimpleImputer(options: SimpleImputerOptions = {})`
- `prep::NeighborImputer(options: NeighborImputerOptions = {})`
- `prep::IterativeImputer(options: IterativeImputerOptions = {})`
- `prep::StandardScaler(options: StandardScalerOptions = {})`
- `prep::MinMaxScaler(options: MinMaxScalerOptions = {})`
- `prep::RobustScaler(options: RobustScalerOptions = {})`
- `prep::MaxAbsScaler(options: MaxAbsScalerOptions = {})`
- `prep::OneHotEncoder(options: OneHotEncoderOptions = {})`
- `prep::FrequencyEncoder(options: FrequencyEncoderOptions = {})`
- `prep::VectorNormalizer(options: VectorNormalizerOptions = {})`
- `prep::QuantileMap(options: QuantileMapOptions = {})`
- `prep::PowerMap(options: PowerMapOptions = {})`
- `prep::ThresholdBinarizer(options: ThresholdBinarizerOptions = {})`
- `prep::PolynomialLift(options: PolynomialLiftOptions = {})`

### Notes

All preprocessors share `learn`, `map`, and `learn_map` behavior.

### Inputs

- DataFrame

### Output

- transformed DataFrame

### Options

Discrete exhaustive counts:

- `SimpleImputerOptions`: `strategy(4)` => **4**
- `NeighborImputerOptions`: `weighting(2)` => **2** (+ numeric axes)
- `IterativeImputerOptions`: no enum/bool => **1** (+ numeric axes)
- `StandardScalerOptions`: `with_centering(2) * with_scaling(2)` => **4**
- `MinMaxScalerOptions`: `clip(2)` => **2**
- `RobustScalerOptions`: `with_centering(2) * with_scaling(2)` => **4**
- `MaxAbsScalerOptions`: no enum/bool => **1**
- `OneHotEncoderOptions`: `overflow(3) * include_unknown_bucket(2)` => **6**
- `FrequencyEncoderOptions`: `overflow(3)` => **3**
- `VectorNormalizerOptions`: `norm(3)` => **3**
- `QuantileMapOptions`: `output(2)` => **2**
- `PowerMapOptions`: `mode(2) * standardize(2) * lambda_mode(2)` => **8**
  - lambda mode: `{auto(lambda=nullopt), fixed(lambda=value)}`
- `ThresholdBinarizerOptions`: no enum/bool => **1**
- `PolynomialLiftOptions`: `interactions_only(2) * include_bias(2)` => **4**

Total discrete preprocessor combinations across all wrappers:

- **45** (excluding numeric sweep axes)

### Examples

```cpp
// Exhaustive OneHotEncoder discrete combinations (6 total)
for (auto overflow : {lym::prep::CategoryOverflowPolicy::Error,
                      lym::prep::CategoryOverflowPolicy::KeepMostFrequent,
                      lym::prep::CategoryOverflowPolicy::SkipFeature}) {
  for (bool unk : {false, true}) {
    auto enc = lym::prep::OneHotEncoder({
        .max_categories = 32,
        .overflow = overflow,
        .include_unknown_bucket = unk,
    });
    auto x_train_e = enc.learn_map(x_train);
    auto x_test_e = enc.map(x_test);
  }
}
```

### Expected output

- every discrete OneHotEncoder branch is exercised exactly once.

## Model Family Combination Atlas

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

Factories expose complete NodeJ family option structs. Discrete combinations below are exhaustive over enum/bool fields.

### Inputs

- family options structs

### Output

- `WorkflowModel` instances for each combination

### Options

Discrete exhaustive counts by family:

`LinearFamilyOptions`

- `objective(3) * regularization(4) * fit_intercept(2)` => **24**
- numeric axis: `regularization_strength`

`TreeFamilyOptions`

- `criterion(4)` => **4**
- numeric axes: `max_depth`, `min_samples_leaf`, `min_samples_split`

`EnsembleFamilyOptions`

- `method(3)` => **3**
- numeric axes: `learners`, `subsample`

`NeighborFamilyOptions`

- `distance_weighted(2) * standardize(2)` => **4**
- numeric axis: `k`

`KernelFamilyOptions`

- `kernel(4)` => **4**
- numeric axes: `c`, `gamma`, `epsilon`

`BayesFamilyOptions`

- `use_gaussian(2)` => **2**
- numeric axis: `smoothing`

`ClusterFamilyOptions`

- `method(1)` => **1**
- numeric axes: `clusters`, `max_iterations`, `restarts`, `tolerance`

`DecomposeFamilyOptions`

- `method(1) * whiten(2)` => **2**
- numeric axes: `components`, `max_iterations`, `tolerance`

Total discrete family combinations:

- **44** (excluding numeric axes)

### Examples

```cpp
// Exhaustive LinearFamilyOptions discrete combinations (24 total)
for (auto obj : {pluss::ai::nodej::LinearObjective::LeastSquares,
                 pluss::ai::nodej::LinearObjective::Logistic,
                 pluss::ai::nodej::LinearObjective::Hinge}) {
  for (auto reg : {pluss::ai::nodej::RegularizationKind::None,
                   pluss::ai::nodej::RegularizationKind::L1,
                   pluss::ai::nodej::RegularizationKind::L2,
                   pluss::ai::nodej::RegularizationKind::Elastic}) {
    for (bool fit_intercept : {false, true}) {
      pluss::ai::nodej::LinearFamilyOptions opt{};
      opt.objective = obj;
      opt.regularization = reg;
      opt.fit_intercept = fit_intercept;
      opt.regularization_strength = 1e-6;

      // choose task-specific constructor by objective
      auto model = (obj == pluss::ai::nodej::LinearObjective::LeastSquares)
                       ? lym::linear_factory::linear_regression(opt)
                       : lym::linear_factory::logistic_regression(opt);
      (void)model;
    }
  }
}
```

### Expected output

- all discrete linear objective/regularization/intercept combinations are constructed.

## Workflow Stage-Set Atlas

### API Signature

- `Workflow{ .model = WorkflowModel, .stages = vector<TransformSpec> }`
- `workflow::stage::<transform>(...) -> TransformSpec`

### Notes

Stage ordering is user-controlled and can be combinatorial. Exhaustive practical coverage is done via stage-set profiles and permutation scopes.

### Inputs

- model + ordered stage list

### Output

- trainable workflow

### Options

Supported stage types (14):

- `simple_imputer`
- `neighbor_imputer`
- `iterative_imputer`
- `standard_scaler`
- `minmax_scaler`
- `robust_scaler`
- `maxabs_scaler`
- `vector_normalizer`
- `one_hot_encoder`
- `frequency_encoder`
- `quantile_map`
- `power_map`
- `threshold_binarizer`
- `polynomial_lift`

For full permutation testing, define:

- stage subset size `k`
- subset selection
- all `k!` orderings

### Examples

```cpp
lym::Workflow wf{
    .model = lym::tree_factory::decision_tree_regressor({}),
    .stages = {
      lym::workflow::stage::simple_imputer({
        .strategy = lym::prep::SimpleImputerStrategy::Median
      }),
      lym::workflow::stage::one_hot_encoder({
        .overflow = lym::prep::CategoryOverflowPolicy::KeepMostFrequent,
        .include_unknown_bucket = true,
      }),
      lym::workflow::stage::standard_scaler(),
    },
};

wf.learn_(x_train, y_train);
```

### Expected output

- listed stages run sequentially before model fitting.

## Selection/Tuning Combination Atlas

### API Signature

- `selection::tune(...) -> TuneResult`
- `selection::tune_specs(...) -> SpecTuneResult`
- `selection::probe_specs(...) -> SpecTuneResult`

### Notes

Selection combinations are the Cartesian product of candidate set, CV settings, metric objective, and policy.

### Inputs

- candidates/specs
- CV options
- objective metric and policy

### Output

- best workflow/spec with per-trial reports

### Options

Discrete axes:

- objective metric: 12 choices
- objective policy: 3 (`Auto`, `Minimize`, `Maximize`)
- CV plan: 5
- stratified/reliability/shuffle/deterministic: 2 each

Base discrete tuning context count (excluding candidates and folds):

- `12 * 3 * 5 * 2 * 2 * 2 * 2 = 2880`

### Examples

```cpp
auto result = lym::selection::tune(
    std::move(candidates),
    x_train,
    y_train,
    lym::CrossValidationOptions{
      .plan = pluss::ai::nodej::CVPlanKind::KFold,
      .folds = 5,
      .stratified = false,
      .reliability_defaults = true,
      .shuffle = true,
      .random_state = 123,
      .deterministic = true,
    },
    train_options,
    pluss::ai::nodej::AssessOptions{.metrics = {pluss::ai::nodej::MetricKind::Rmse}},
    pluss::ai::nodej::MetricKind::Rmse,
    pluss::ai::nodej::TuneObjectivePolicy::Minimize);
```

### Expected output

- ranked trial report and one selected best workflow.

## Online Learning Combination Atlas

### API Signature

- `online_factory::linear_classifier(options: OnlineLinearOptions = {}) -> OnlineLinearClassifier`
- `online_factory::linear_regressor(options: OnlineLinearOptions = {}) -> OnlineLinearRegressor`

### Notes

Online options combine linear family settings with online update policy.

### Inputs

- `OnlineLinearOptions`

### Output

- online model instance

### Options

Discrete axes from `OnlineLinearOptions` + nested `LinearFamilyOptions`:

- `decaying_rate(2)`
- `family.objective(3)`
- `family.regularization(4)`
- `family.fit_intercept(2)`

Total discrete combinations:

- `2 * 3 * 4 * 2 = 48`

Numeric axes:

- `learning_rate`
- `family.regularization_strength`

### Examples

```cpp
for (bool decay : {false, true}) {
  for (auto obj : {pluss::ai::nodej::LinearObjective::LeastSquares,
                   pluss::ai::nodej::LinearObjective::Logistic,
                   pluss::ai::nodej::LinearObjective::Hinge}) {
    for (auto reg : {pluss::ai::nodej::RegularizationKind::None,
                     pluss::ai::nodej::RegularizationKind::L1,
                     pluss::ai::nodej::RegularizationKind::L2,
                     pluss::ai::nodej::RegularizationKind::Elastic}) {
      for (bool intercept : {false, true}) {
        lym::OnlineLinearOptions opt{};
        opt.decaying_rate = decay;
        opt.learning_rate = 0.05;
        opt.family.objective = obj;
        opt.family.regularization = reg;
        opt.family.fit_intercept = intercept;
        auto online = lym::online_factory::linear_regressor(opt);
        (void)online;
      }
    }
  }
}
```

### Expected output

- all discrete online option branches are constructible.

## Practical Full-Stack Exhaustive Strategy

1. Enumerate discrete options from this atlas for each surface.
2. Define numeric sweep grids per dataset scale.
3. Use `hold_out_split` for external holdout.
4. Use `cross_validate` inside search loops.
5. Persist top workflows and verify restore parity with `max_abs_diff`.

Expected output behavior:

- reproducible, fully auditable ML search coverage across all supported Lymph option branches.

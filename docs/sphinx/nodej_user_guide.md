# NodeJ User Guide

NodeJ is the core, full-control ML library in PlussAI. It is lower-level than Lymph and exposes direct control over model specs, data batching, workflow composition, splitting, metrics, tuning, and persistence.

## What NodeJ Is

### API Signature

- `ModelSpec(task: TaskKind, family: ModelFamily, options: FamilyOptions) -> ModelSpec`
- `make_flow(spec: ModelSpec) -> StatusOr<Flow>`
- `Flow::learn(batch: LabeledBatch|DataFrame, options: TrainOptions = {}) -> Status`
- `Flow::infer(batch: FeatureBatch|DataFrame, options: InferOptions = {}) -> StatusOr<Series>`
- `Flow::assess(batch: LabeledBatch|DataFrame, options: AssessOptions = {}) -> StatusOr<Series>`

### Notes

Use NodeJ when you need explicit control over:

- feature/label extraction policy
- transform routing and state dumps
- split plans and index-level operations
- model spec probing/sweeps

### Inputs

- typed option structs and explicit data contracts

### Output

- `Status` / `StatusOr<T>` driven execution with explicit error handling

### Options

- all options are enum/struct typed, no string-based configuration

### Examples

```cpp
using namespace pluss::ai::nodej;

ModelSpec spec{
  .task = TaskKind::Regression,
  .family = ModelFamily::Linear,
  .options = LinearFamilyOptions{},
};

auto flow_or = make_flow(spec);
if (!flow_or.ok()) {
  throw std::runtime_error(flow_or.status().message);
}
Flow flow = flow_or.move_value();
```

### Expected output

- valid `Flow` object when `ModelSpec` is consistent.

## Include And Namespace

### API Signature

- `#include <pluss/ai/nodej/nodej.h>`
- `namespace nje = pluss::ai::nodej`

### Notes

NodeJ types live under `pluss::ai::nodej`.

### Inputs

- C++20 compiler and PlussAI build

### Output

- NodeJ symbols available in translation unit

### Options

- no runtime flags needed for baseline use

### Examples

```cpp
#include <pluss/ai/nodej/nodej.h>
#include <pluss/mic.h>

namespace nje = pluss::ai::nodej;
```

### Expected output

- project compiles with NodeJ symbols resolved.

## Core Mental Model

### API Signature

- `ModelSpec(task: TaskKind, family: ModelFamily, options: FamilyOptions) -> ModelSpec`
- `Flow::add_<transform>(options: ..., feature_route: vector<int64> = {}) -> Status`
- `Flow::set_model(spec: ModelSpec) -> Status`

### Notes

NodeJ organizes ML around:

- `ModelSpec` for model identity + options
- `Flow` for ordered transforms + one model
- `FeatureBatch` / `LabeledBatch` for explicit numeric data contract

### Inputs

- model specification
- optional transform stack

### Output

- trainable, serializable flow

### Options

- `TaskKind`: `Classification`, `Regression`, `Transform`
- `ModelFamily`: `Linear`, `Tree`, `Ensemble`, `Neighbor`, `Kernel`, `Bayes`, `Cluster`, `Decompose`

### Examples

```cpp
nje::ModelSpec spec{
  .task = nje::TaskKind::Classification,
  .family = nje::ModelFamily::Kernel,
  .options = nje::KernelFamilyOptions{.kernel = nje::KernelKind::Rbf},
};

auto flow_or = nje::make_flow(spec);
if (!flow_or.ok()) throw std::runtime_error(flow_or.status().message);
auto flow = flow_or.move_value();

if (!flow.add_standard_scaler().ok()) throw std::runtime_error("add_standard_scaler failed");
if (!flow.add_simple_imputer({.strategy = nje::prep::SimpleImputerStrategy::Median}).ok()) {
  throw std::runtime_error("add_simple_imputer failed");
}
```

### Expected output

- transform stack is registered in order of `add_*` calls.

## Data Contracts And DataFrame Bridge

### API Signature

- `feature_batch_from_dataframe(frame: DataFrame, options: DataFrameBatchOptions = {}) -> StatusOr<FeatureBatch>`
- `labeled_batch_from_dataframe(frame: DataFrame, options: DataFrameBatchOptions) -> StatusOr<LabeledBatch>`

### Notes

Use DataFrame bridge when your data lives in PlussMic tables. Use explicit batch structs when already numeric.

### Inputs

- DataFrame
- `DataFrameBatchOptions`

### Output

- `FeatureBatch` or `LabeledBatch`

### Options

`DataFrameBatchOptions`:

- `feature_names`
- `label_feature`
- `sample_weight_feature`
- `layout`
- `thread_policy`: `SeriesOnly` / `CastTemporalToSeries`
- `note_policy`: `Reject` / `Drop` / `Hash`
- `note_hash_seed`

### Examples

```cpp
nje::DataFrameBatchOptions data_opts{};
data_opts.feature_names = {"x1", "x2", "x3"};
data_opts.label_feature = std::string("target");
data_opts.layout = nje::ObservationLayout::Rows;
data_opts.thread_policy = nje::DataThreadPolicy::CastTemporalToSeries;
data_opts.note_policy = nje::DataNotePolicy::Reject;

auto labeled_or = nje::labeled_batch_from_dataframe(df, data_opts);
if (!labeled_or.ok()) throw std::runtime_error(labeled_or.status().message);
const nje::LabeledBatch batch = labeled_or.value();
```

### Expected output

- numeric matrix + aligned target in a validated `LabeledBatch`.

## Training, Inference, And Assessment

### API Signature

- `Flow::learn(batch: LabeledBatch, options: TrainOptions = {}) -> Status`
- `Flow::learn(frame: DataFrame, data_options: DataFrameBatchOptions, options: TrainOptions = {}) -> Status`
- `Flow::infer(batch: FeatureBatch|DataFrame, options: InferOptions = {}) -> StatusOr<Series>`
- `Flow::infer_scores(batch: FeatureBatch|DataFrame, options: InferOptions = {}) -> StatusOr<Series>`
- `Flow::assess(batch: LabeledBatch|DataFrame, options: AssessOptions = {}) -> StatusOr<Series>`

### Notes

- `learn` fits transforms and model.
- `infer` returns predictions.
- `infer_scores` returns score/probability-like outputs where supported.
- `assess` returns metric vector in requested metric order.

### Inputs

- labeled/feature input and options

### Output

- status or series outputs

### Options

`TrainOptions` highlights:

- `max_iterations`, `tolerance`
- `warm_start`
- `random.seed`, `random.deterministic`
- `class_weight`, `class_weights`
- `calibration`, `calibration_iterations`, `calibration_tolerance`

`InferOptions` highlights:

- `positive_threshold`

`AssessOptions` highlights:

- `metrics`
- `positive_threshold`
- `regression_quantile`

### Examples

```cpp
nje::TrainOptions train_opts{};
train_opts.max_iterations = 3000;
train_opts.tolerance = 1e-8;
train_opts.random.seed = 42;
train_opts.random.deterministic = true;

if (!flow.learn(batch, train_opts).ok()) {
  throw std::runtime_error("flow.learn failed");
}

auto pred_or = flow.infer(batch.features);
if (!pred_or.ok()) throw std::runtime_error(pred_or.status().message);

nje::AssessOptions assess_opts{};
assess_opts.metrics = {nje::MetricKind::Rmse, nje::MetricKind::R2, nje::MetricKind::Mae};
auto metric_or = flow.assess(batch, assess_opts);
if (!metric_or.ok()) throw std::runtime_error(metric_or.status().message);
```

### Expected output

- trained flow produces predictions and ordered metric vector.

## Workflow Composition (Transforms + Model)

### API Signature

- `Flow::add_standard_scaler(options: prep::StandardScalerOptions = {}, feature_route: vector<int64> = {}) -> Status`
- `Flow::add_minmax_scaler(options: prep::MinMaxScalerOptions = {}, feature_route: vector<int64> = {}) -> Status`
- `Flow::add_robust_scaler(options: prep::RobustScalerOptions = {}, feature_route: vector<int64> = {}) -> Status`
- `Flow::add_simple_imputer(options: prep::SimpleImputerOptions = {}, feature_route: vector<int64> = {}) -> Status`
- `Flow::add_neighbor_imputer(options: prep::NeighborImputerOptions = {}, feature_route: vector<int64> = {}) -> Status`
- `Flow::add_iterative_imputer(options: prep::IterativeImputerOptions = {}, feature_route: vector<int64> = {}) -> Status`
- `Flow::add_one_hot_encoder(options: prep::OneHotEncoderOptions = {}, feature_route: vector<int64> = {}) -> Status`
- `Flow::add_frequency_encoder(options: prep::FrequencyEncoderOptions = {}, feature_route: vector<int64> = {}) -> Status`
- `Flow::add_ordinal_encoder(options: prep::OrdinalEncoderOptions = {}, feature_route: vector<int64> = {}) -> Status`
- `Flow::add_label_encoder(options: prep::LabelEncoderOptions = {}, feature_route: vector<int64> = {}) -> Status`
- `Flow::add_vector_normalizer(options: prep::VectorNormalizerOptions = {}, feature_route: vector<int64> = {}) -> Status`
- `Flow::add_maxabs_scaler(options: prep::MaxAbsScalerOptions = {}, feature_route: vector<int64> = {}) -> Status`
- `Flow::add_quantile_map(options: prep::QuantileMapOptions = {}, feature_route: vector<int64> = {}) -> Status`
- `Flow::add_power_map(options: prep::PowerMapOptions = {}, feature_route: vector<int64> = {}) -> Status`
- `Flow::add_threshold_binarizer(options: prep::ThresholdBinarizerOptions = {}, feature_route: vector<int64> = {}) -> Status`
- `Flow::add_polynomial_lift(options: prep::PolynomialLiftOptions = {}, feature_route: vector<int64> = {}) -> Status`

### Notes

These calls build the full pipeline directly in core NodeJ.

### Inputs

- options structs and optional feature routing indices

### Output

- transform stack stored in flow

### Options

- each transform has typed options under `nodej::prep`

### Examples

```cpp
auto flow_or = nje::make_flow(nje::ModelSpec{
    .task = nje::TaskKind::Regression,
    .family = nje::ModelFamily::Linear,
    .options = nje::LinearFamilyOptions{},
});
if (!flow_or.ok()) throw std::runtime_error(flow_or.status().message);
auto flow = flow_or.move_value();

if (!flow.add_simple_imputer({.strategy = nje::prep::SimpleImputerStrategy::Median}).ok()) {
  throw std::runtime_error("imputer add failed");
}
if (!flow.add_standard_scaler().ok()) throw std::runtime_error("scaler add failed");
```

### Expected output

- learn/infer runs with both transform stages applied before the model.

## Standalone Preprocessors

### API Signature

- `prep::SimpleImputer::learn(batch: FeatureBatch, options: TrainOptions = {}) -> Status`
- `prep::SimpleImputer::map(batch: FeatureBatch, options: InferOptions = {}) -> StatusOr<Series>`
- `prep::SimpleImputer::learn_map(batch: FeatureBatch, learn_options: TrainOptions = {}, infer_options: InferOptions = {}) -> StatusOr<Series>`
- same contract for all NodeJ preprocessors (`StandardScaler`, `MinMaxScaler`, `RobustScaler`, `NeighborImputer`, `IterativeImputer`, `OneHotEncoder`, `FrequencyEncoder`, `OrdinalEncoder`, `LabelEncoder`, `VectorNormalizer`, `MaxAbsScaler`, `QuantileMap`, `PowerMap`, `ThresholdBinarizer`, `PolynomialLift`)
- single-feature categorical convenience:
  - `prep::OneHotEncoder::learn(feature: pluss::table::Thread, ...)`
  - `prep::OneHotEncoder::map(feature: pluss::table::Thread, ...)`
  - `prep::OneHotEncoder::learn_map(feature: pluss::table::Thread, ...)`
  - same Thread overloads for `prep::FrequencyEncoder`, `prep::OrdinalEncoder`, and `prep::LabelEncoder`

### Notes

Use standalone preprocessors when you want explicit transform state management outside `Flow`.

Categorical tools have distinct intent:

- `prep::CategoryIndexer`: stable category->index mapping utility
- `prep::OrdinalEncoder`: ordered integer encoding
- `prep::LabelEncoder`: compact integer labels
- `prep::OneHotEncoder`: nominal category expansion
- `prep::FrequencyEncoder`: prevalence-based compact encoding

Practical guidance:

- use `CategoryIndexer` when you need deterministic IDs/contracts
- for nominal model features, prefer `OneHotEncoder` or `FrequencyEncoder`
- for ordered categorical features, use `OrdinalEncoder`
- for categorical label-side mapping, use `LabelEncoder`

### Inputs

- `FeatureBatch`
- `pluss::table::Thread` for single-feature categorical encoding (`OneHotEncoder`, `FrequencyEncoder`, `OrdinalEncoder`, `LabelEncoder`) across Series/Note/Timeline/TimeIndex

### Output

- transformed feature matrix (`Series`)

### Options

- each preprocessor option struct in `nodej::prep`

### Examples

```cpp
nje::prep::StandardScaler scaler({.with_centering = true, .with_scaling = true});
if (!scaler.learn(batch.features, {}).ok()) throw std::runtime_error("scaler learn failed");
auto mapped_or = scaler.map(batch.features, {});
if (!mapped_or.ok()) throw std::runtime_error(mapped_or.status().message);
const Series x_scaled = mapped_or.value();
```

### Expected output

- transformed feature matrix with learned scaler state.

## Split And Cross-Validation Utilities

### API Signature

- `holdout_split(sample_count: int64, options: SplitOptions = {}) -> StatusOr<IndexSplit>`
- `strat_holdout_split(labels: Series, options: SplitOptions = {}) -> StatusOr<IndexSplit>`
- `fold_plan(sample_count: int64, options: SplitOptions = {}) -> StatusOr<SplitPlan>`
- `strat_fold_plan(labels: Series, options: SplitOptions = {}) -> StatusOr<SplitPlan>`
- `time_series_fold_plan(sample_count: int64, options: SplitOptions = {}) -> StatusOr<SplitPlan>`
- `slice_feature_batch(in: FeatureBatch, indices: Series) -> StatusOr<FeatureBatch>`
- `slice_labeled_batch(in: LabeledBatch, indices: Series) -> StatusOr<LabeledBatch>`
- `cross_assess(spec: ModelSpec, batch: LabeledBatch, split: SplitOptions = {}, train_options: TrainOptions = {}, assess_options: AssessOptions = {}) -> StatusOr<CrossAssessReport>`

### Notes

NodeJ separates split planning from evaluation execution so you can reuse indices and plans.

### Inputs

- sample counts or labels
- split/train/assess options

### Output

- split indices/plans and CV metric reports

### Options

`SplitOptions`:

- `plan`
- `holdout_fraction`
- `folds`
- `time_gap`, `time_test_size`, `time_max_train_size`
- `stratified`, `reliability_defaults`, `shuffle`
- `random.seed`, `random.deterministic`

### Examples

```cpp
nje::SplitOptions split{};
split.plan = nje::CVPlanKind::KFold;
split.folds = 5;
split.random.seed = 19;
split.random.deterministic = true;

auto cv_or = nje::cross_assess(spec, batch, split,
                               nje::TrainOptions{},
                               nje::AssessOptions{.metrics = {nje::MetricKind::Rmse}});
if (!cv_or.ok()) throw std::runtime_error(cv_or.status().message);
const auto report = cv_or.value();
```

### Expected output

- `report.mean_metrics` and `report.std_metrics` hold per-metric aggregates.

## Hyperparameter Search (Spec-Level)

### API Signature

- `sweep(candidates: vector<ModelSpec>, batch: LabeledBatch, split: SplitOptions = {}, train_options: TrainOptions = {}, assess_options: AssessOptions = {}, objective_metric: MetricKind = Accuracy, objective_policy: TuneObjectivePolicy = Auto) -> StatusOr<TuneReport>`
- `probe(sampler: ProbeSpecSampler, trials: int64, batch: LabeledBatch, split: SplitOptions = {}, train_options: TrainOptions = {}, assess_options: AssessOptions = {}, objective_metric: MetricKind = Accuracy, objective_policy: TuneObjectivePolicy = Auto, random: RandomState = {}) -> StatusOr<TuneReport>`

### Notes

- `sweep` evaluates a fixed candidate list.
- `probe` samples specs on demand.

### Inputs

- candidates or sampler
- objective metric/policy

### Output

- trial results + `best_index`

### Options

- `TuneObjectivePolicy`: `Auto`, `Minimize`, `Maximize`

### Examples

```cpp
std::vector<nje::ModelSpec> candidates;
candidates.push_back({
  .task = nje::TaskKind::Regression,
  .family = nje::ModelFamily::Linear,
  .options = nje::LinearFamilyOptions{},
});

candidates.push_back({
  .task = nje::TaskKind::Regression,
  .family = nje::ModelFamily::Tree,
  .options = nje::TreeFamilyOptions{.max_depth = 12},
});

auto tune_or = nje::sweep(candidates,
                          batch,
                          nje::SplitOptions{.plan = nje::CVPlanKind::KFold, .folds = 5},
                          {},
                          nje::AssessOptions{.metrics = {nje::MetricKind::Rmse}},
                          nje::MetricKind::Rmse,
                          nje::TuneObjectivePolicy::Minimize);
if (!tune_or.ok()) throw std::runtime_error(tune_or.status().message);
```

### Expected output

- `TuneReport.best_index` points to best candidate under objective policy.

## Persistence And Restore

### API Signature

- `io::save(path: string, flow: Flow, train_batch: LabeledBatch, train_options: TrainOptions = {}, metadata: FlowBundleMetadata = {}) -> Status`
- `io::load(path: string) -> StatusOr<FlowBundle>`
- `io::restore(bundle: FlowBundle) -> StatusOr<Flow>`
- `io::load_and_restore(path: string) -> StatusOr<Flow>`

### Notes

NodeJ persistence stores model spec, transform spec/state, model state, train options, and train batch.

### Inputs

- trained flow + train batch

### Output

- serialized bundle and restored flow

### Options

`FlowBundleMetadata`:

- `schema_version`
- `producer`
- `producer_version`
- `created_unix_nanos`

### Examples

```cpp
if (!nje::io::save("nodej_flow.bin", flow, batch, train_opts).ok()) {
  throw std::runtime_error("save failed");
}

auto restored_or = nje::io::load_and_restore("nodej_flow.bin");
if (!restored_or.ok()) throw std::runtime_error(restored_or.status().message);
const auto restored = restored_or.value();
```

### Expected output

- restored flow can infer/assess consistently with trained flow.

## Online Learning

### API Signature

- `online::make_online_linear_classifier(options: online::OnlineLinearOptions = {}) -> StatusOr<online::OnlineLinearClassifier>`
- `online::make_online_linear_regressor(options: online::OnlineLinearOptions = {}) -> StatusOr<online::OnlineLinearRegressor>`
- `OnlineLinearClassifier::learn(batch: LabeledBatch, options: TrainOptions = {}) -> Status`
- `OnlineLinearClassifier::advance(batch: LabeledBatch, options: TrainOptions = {}) -> Status`
- `OnlineLinearClassifier::infer_scores(batch: FeatureBatch, options: InferOptions = {}) -> StatusOr<Series>`
- `OnlineLinearClassifier::infer(batch: FeatureBatch, options: InferOptions = {}) -> StatusOr<Series>`
- `OnlineLinearClassifier::assess(batch: LabeledBatch, options: AssessOptions = {}) -> StatusOr<Series>`
- `OnlineLinearRegressor::learn(batch: LabeledBatch, options: TrainOptions = {}) -> Status`
- `OnlineLinearRegressor::advance(batch: LabeledBatch, options: TrainOptions = {}) -> Status`
- `OnlineLinearRegressor::infer(batch: FeatureBatch, options: InferOptions = {}) -> StatusOr<Series>`
- `OnlineLinearRegressor::assess(batch: LabeledBatch, options: AssessOptions = {}) -> StatusOr<Series>`

### Notes

Online models support incremental update streams.

### Inputs

- chunked labeled batches

### Output

- continuously updated model state

### Options

`online::OnlineLinearOptions`:

- `family`
- `learning_rate`
- `decaying_rate`

### Examples

```cpp
auto online_or = nje::online::make_online_linear_regressor({.learning_rate = 0.05, .decaying_rate = true});
if (!online_or.ok()) throw std::runtime_error(online_or.status().message);
auto model = online_or.move_value();

if (!model.learn(batch0, {}).ok()) throw std::runtime_error("online learn failed");
if (!model.advance(batch1, {}).ok()) throw std::runtime_error("online advance failed");
```

### Expected output

- model remains trained and updates with each call.

## End-To-End Blueprint (Core NodeJ)

1. create `ModelSpec`
2. build `Flow` with `make_flow`
3. add transforms (`add_*`)
4. convert DataFrame to batch (`labeled_batch_from_dataframe`) or pass prebuilt batch
5. call `learn`
6. call `infer` and `assess`
7. call `cross_assess` or `sweep` for model selection
8. save with `io::save`, restore with `io::load_and_restore`

Expected output behavior:

- explicit, inspectable ML execution with fail-loud statuses.

## NodeJ vs Lymph

- NodeJ: maximal control, explicit `StatusOr` and batch contracts.
- Lymph: reduced boilerplate over NodeJ with convenience wrappers.

For task recipes, see **NodeJ Cookbook**.

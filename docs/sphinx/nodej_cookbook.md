# NodeJ Cookbook

This cookbook focuses on practical, minimal NodeJ patterns with full control.

## Recipe 1: Minimal Linear Regression Flow

### API Signature

- `make_flow(spec: ModelSpec) -> StatusOr<Flow>`
- `Flow::learn(batch: LabeledBatch, options: TrainOptions = {}) -> Status`
- `Flow::infer(batch: FeatureBatch, options: InferOptions = {}) -> StatusOr<Series>`
- `Flow::assess(batch: LabeledBatch, options: AssessOptions = {}) -> StatusOr<Series>`

### Notes

Use this as the baseline regression pattern in pure NodeJ.

### Inputs

- `LabeledBatch` train/test data

### Output

- predictions and metric vector

### Options

- `LinearFamilyOptions`
- `TrainOptions`
- `AssessOptions.metrics`

### Examples

```cpp
namespace nje = pluss::ai::nodej;

nje::ModelSpec spec{
  .task = nje::TaskKind::Regression,
  .family = nje::ModelFamily::Linear,
  .options = nje::LinearFamilyOptions{
      .objective = nje::LinearObjective::LeastSquares,
      .regularization = nje::RegularizationKind::L2,
      .regularization_strength = 1e-6,
      .fit_intercept = true,
  },
};

auto flow_or = nje::make_flow(spec);
if (!flow_or.ok()) throw std::runtime_error(flow_or.status().message);
auto flow = flow_or.move_value();

nje::TrainOptions train_opts{};
train_opts.max_iterations = 2500;
train_opts.tolerance = 1e-8;
train_opts.random.seed = 7;
train_opts.random.deterministic = true;
if (!flow.learn(train_batch, train_opts).ok()) throw std::runtime_error("learn failed");

auto pred_or = flow.infer(test_batch.features);
if (!pred_or.ok()) throw std::runtime_error(pred_or.status().message);

auto metric_or = flow.assess(test_batch, nje::AssessOptions{
    .metrics = {nje::MetricKind::Rmse, nje::MetricKind::R2, nje::MetricKind::Mae},
});
if (!metric_or.ok()) throw std::runtime_error(metric_or.status().message);
```

### Expected output

- trained regression model, prediction series, and metric series in requested order.

## Recipe 2: Logistic Classification With Calibration

### API Signature

- `Flow::learn(batch: LabeledBatch, options: TrainOptions = {}) -> Status`
- `Flow::infer_scores(batch: FeatureBatch, options: InferOptions = {}) -> StatusOr<Series>`

### Notes

Use logistic objective and optional score calibration.

### Inputs

- binary or multiclass classification batch

### Output

- predictions and calibrated scores

### Options

- `LinearFamilyOptions.objective = Logistic`
- `TrainOptions.calibration = Platt`

### Examples

```cpp
nje::ModelSpec spec{
  .task = nje::TaskKind::Classification,
  .family = nje::ModelFamily::Linear,
  .options = nje::LinearFamilyOptions{
      .objective = nje::LinearObjective::Logistic,
      .regularization = nje::RegularizationKind::L2,
      .regularization_strength = 1e-4,
      .fit_intercept = true,
  },
};

auto flow_or = nje::make_flow(spec);
if (!flow_or.ok()) throw std::runtime_error(flow_or.status().message);
auto flow = flow_or.move_value();

nje::TrainOptions opts{};
opts.calibration = nje::ScoreCalibration::Platt;
opts.calibration_iterations = 400;
opts.calibration_tolerance = 1e-6;
if (!flow.learn(train_batch, opts).ok()) throw std::runtime_error("learn failed");

auto yhat_or = flow.infer(test_batch.features);
auto scores_or = flow.infer_scores(test_batch.features);
if (!yhat_or.ok() || !scores_or.ok()) throw std::runtime_error("infer/infer_scores failed");
```

### Expected output

- hard predictions from `infer` and calibrated score outputs from `infer_scores`.

## Recipe 3: DataFrame To LabeledBatch (Explicit)

### API Signature

- `labeled_batch_from_dataframe(frame: DataFrame, options: DataFrameBatchOptions) -> StatusOr<LabeledBatch>`

### Notes

Use explicit mapping when dataset columns are mixed or you need strict policy behavior.

### Inputs

- DataFrame and mapping policy

### Output

- validated `LabeledBatch`

### Options

- `feature_names`, `label_feature`, `thread_policy`, `note_policy`

### Examples

```cpp
nje::DataFrameBatchOptions data_opts{};
data_opts.feature_names = {"x1", "x2", "x3", "x4"};
data_opts.label_feature = std::string("y");
data_opts.thread_policy = nje::DataThreadPolicy::CastTemporalToSeries;
data_opts.note_policy = nje::DataNotePolicy::Reject;
data_opts.layout = nje::ObservationLayout::Rows;

auto batch_or = nje::labeled_batch_from_dataframe(df, data_opts);
if (!batch_or.ok()) throw std::runtime_error(batch_or.status().message);
const auto batch = batch_or.value();
```

### Expected output

- DataFrame converted into model-ready batch with label alignment.

## Recipe 4: Flow With Multiple Preprocessing Stages

### API Signature

- `Flow::add_simple_imputer(...) -> Status`
- `Flow::add_standard_scaler(...) -> Status`
- `Flow::add_one_hot_encoder(...) -> Status`
- `Flow::add_ordinal_encoder(...) -> Status`
- `Flow::add_label_encoder(...) -> Status`

### Notes

Compose preprocessing and model in one flow. Learn runs them in sequence.

### Inputs

- flow and transform option structs

### Output

- trained, reusable preprocessing+model pipeline

### Options

- `feature_route` for targeted feature subsets

### Examples

```cpp
auto flow_or = nje::make_flow(spec);
if (!flow_or.ok()) throw std::runtime_error(flow_or.status().message);
auto flow = flow_or.move_value();

if (!flow.add_simple_imputer({.strategy = nje::prep::SimpleImputerStrategy::Median}).ok()) {
  throw std::runtime_error("simple_imputer add failed");
}
if (!flow.add_one_hot_encoder({
      .max_categories = 32,
      .overflow = nje::prep::CategoryOverflowPolicy::KeepMostFrequent,
      .include_unknown_bucket = true,
    }).ok()) {
  throw std::runtime_error("one_hot_encoder add failed");
}
if (!flow.add_standard_scaler().ok()) throw std::runtime_error("standard_scaler add failed");
```

### Expected output

- preprocessing stages are fitted and replayed consistently on inference data.

## Recipe 5: Holdout Split + Manual Slicing

### API Signature

- `holdout_split(sample_count: int64, options: SplitOptions = {}) -> StatusOr<IndexSplit>`
- `slice_labeled_batch(in: LabeledBatch, indices: Series) -> StatusOr<LabeledBatch>`

### Notes

Use index-level split when you need strict control over train/test partitioning.

### Inputs

- sample count or labels

### Output

- index split and sliced batches

### Options

- `SplitOptions.holdout_fraction`
- `SplitOptions.shuffle`
- `SplitOptions.random`

### Examples

```cpp
nje::SplitOptions split{};
split.plan = nje::CVPlanKind::Holdout;
split.holdout_fraction = 0.2;
split.shuffle = true;
split.random.seed = 42;
split.random.deterministic = true;

auto idx_or = nje::holdout_split(batch.features.sample_count(), split);
if (!idx_or.ok()) throw std::runtime_error(idx_or.status().message);

const auto idx = idx_or.value();
auto train_or = nje::slice_labeled_batch(batch, idx.train_indices);
auto test_or = nje::slice_labeled_batch(batch, idx.test_indices);
if (!train_or.ok() || !test_or.ok()) throw std::runtime_error("slice failed");
```

### Expected output

- deterministic train/test batches for fixed seed.

## Recipe 6: Stratified And Repeated Stratified Cross Assessment

### API Signature

- `cross_assess(spec: ModelSpec, batch: LabeledBatch, split: SplitOptions = {}, train_options: TrainOptions = {}, assess_options: AssessOptions = {}) -> StatusOr<CrossAssessReport>`

### Notes

Use for robust estimates across class-preserving cross-validation partitions. Set `StratifiedCrossVal` with `repeats > 1` when you want multiple shuffled stratified passes.

### Inputs

- model spec + labeled batch + split options

### Output

- fold metrics and aggregate mean/std

### Options

- `SplitOptions.plan = StratifiedCrossVal`
- `SplitOptions.folds`
- `SplitOptions.repeats`
- `AssessOptions.metrics`

### Examples

```cpp
nje::SplitOptions split{};
split.plan = nje::CVPlanKind::StratifiedCrossVal;
split.folds = 5;
split.stratified = true;
split.random.seed = 11;
split.random.deterministic = true;

auto report_or = nje::cross_assess(
    spec,
    batch,
    split,
    nje::TrainOptions{},
    nje::AssessOptions{.metrics = {nje::MetricKind::Accuracy, nje::MetricKind::F1}});
if (!report_or.ok()) throw std::runtime_error(report_or.status().message);
```

```cpp
nje::SplitOptions repeated = split;
repeated.plan = nje::CVPlanKind::StratifiedCrossVal;
repeated.repeats = 3;

auto repeated_or = nje::cross_assess(
    spec,
    batch,
    repeated,
    nje::TrainOptions{},
    nje::AssessOptions{.metrics = {nje::MetricKind::Accuracy, nje::MetricKind::F1}});
if (!repeated_or.ok()) throw std::runtime_error(repeated_or.status().message);
```

### Expected output

- stratified example: five fold reports plus aggregate mean/std vectors.
- repeated example: fifteen fold reports when `folds = 5` and `repeats = 3`.

## Recipe 7: Spec Sweep Tuning

### API Signature

- `sweep(candidates: vector<ModelSpec>, batch: LabeledBatch, split: SplitOptions = {}, train_options: TrainOptions = {}, assess_options: AssessOptions = {}, objective_metric: MetricKind = Accuracy, objective_policy: TuneObjectivePolicy = Auto) -> StatusOr<TuneReport>`

### Notes

Use fixed candidate lists when you have curated model families/options.

### Inputs

- candidate specs and tuning objective

### Output

- trial outcomes + best index

### Options

- `objective_metric`
- `objective_policy`

### Examples

```cpp
std::vector<nje::ModelSpec> candidates;
candidates.push_back({
  .task = nje::TaskKind::Regression,
  .family = nje::ModelFamily::Linear,
  .options = nje::LinearFamilyOptions{.regularization = nje::RegularizationKind::L2},
});
candidates.push_back({
  .task = nje::TaskKind::Regression,
  .family = nje::ModelFamily::Tree,
  .options = nje::TreeFamilyOptions{.max_depth = 10, .min_samples_leaf = 5},
});

auto tune_or = nje::sweep(
    candidates,
    batch,
    nje::SplitOptions{.plan = nje::CVPlanKind::CrossVal, .folds = 5},
    {},
    nje::AssessOptions{.metrics = {nje::MetricKind::Rmse}},
    nje::MetricKind::Rmse,
    nje::TuneObjectivePolicy::Minimize);
if (!tune_or.ok()) throw std::runtime_error(tune_or.status().message);
```

### Expected output

- `best_index` identifies best spec for requested objective.

## Recipe 8: Probe Tuning (Sampler-Based)

### API Signature

- `probe(sampler: ProbeSpecSampler, trials: int64, batch: LabeledBatch, split: SplitOptions = {}, train_options: TrainOptions = {}, assess_options: AssessOptions = {}, objective_metric: MetricKind = Accuracy, objective_policy: TuneObjectivePolicy = Auto, random: RandomState = {}) -> StatusOr<TuneReport>`

### Notes

Use probe for generated search spaces (randomized or programmatic).

### Inputs

- sampler callback and trial budget

### Output

- report with best sampled spec

### Options

- `trials`
- `random`

### Examples

```cpp
nje::ProbeSpecSampler sampler = [](std::int64_t trial, std::uint64_t) -> nje::StatusOr<nje::ModelSpec> {
  nje::TreeFamilyOptions opt{};
  opt.max_depth = 4 + (trial % 12);
  opt.min_samples_leaf = 1 + (trial % 8);
  return nje::ModelSpec{
      .task = nje::TaskKind::Regression,
      .family = nje::ModelFamily::Tree,
      .options = opt,
  };
};

auto probe_or = nje::probe(sampler,
                           30,
                           batch,
                           nje::SplitOptions{.plan = nje::CVPlanKind::CrossVal, .folds = 5},
                           {},
                           nje::AssessOptions{.metrics = {nje::MetricKind::Rmse}},
                           nje::MetricKind::Rmse,
                           nje::TuneObjectivePolicy::Minimize,
                           nje::RandomState{.seed = 99, .deterministic = true});
if (!probe_or.ok()) throw std::runtime_error(probe_or.status().message);
```

### Expected output

- probe evaluates generated specs and returns best sampled configuration.

## Recipe 9: Save, Load, Restore, And Parity Check

### API Signature

- `io::save(...) -> Status`
- `io::load(...) -> StatusOr<FlowBundle>`
- `io::restore(...) -> StatusOr<Flow>`
- `io::load_and_restore(...) -> StatusOr<Flow>`

### Notes

Preferred parity validation is inference on same test set before/after restore.

### Inputs

- trained flow and train batch

### Output

- restored flow

### Options

- optional `FlowBundleMetadata`

### Examples

```cpp
if (!nje::io::save("model.bin", flow, train_batch, train_opts).ok()) {
  throw std::runtime_error("save failed");
}

auto restored_or = nje::io::load_and_restore("model.bin");
if (!restored_or.ok()) throw std::runtime_error(restored_or.status().message);
const auto restored = restored_or.value();

auto p1_or = flow.infer(test_batch.features);
auto p2_or = restored.infer(test_batch.features);
if (!p1_or.ok() || !p2_or.ok()) throw std::runtime_error("infer failed");
```

### Expected output

- original and restored flows produce near-identical predictions under deterministic setup.

## Recipe 10: Unsupervised KMeans/PCA Through ModelSpec

### API Signature

- `ModelSpec(task: Transform, family: Cluster|Decompose, options: ClusterFamilyOptions|DecomposeFamilyOptions) -> ModelSpec`
- `Flow::learn_map(batch: FeatureBatch, learn_options: TrainOptions = {}, infer_options: InferOptions = {}) -> StatusOr<Series>`
- `Flow::map(batch: FeatureBatch, options: InferOptions = {}) -> StatusOr<Series>`

### Notes

Unsupervised workflows use `TaskKind::Transform` and produce transformed feature matrices.

### Inputs

- feature batches

### Output

- mapped/transformed matrices

### Options

- `ClusterFamilyOptions`
- `DecomposeFamilyOptions`

### Examples

```cpp
nje::ModelSpec pca_spec{
  .task = nje::TaskKind::Transform,
  .family = nje::ModelFamily::Decompose,
  .options = nje::DecomposeFamilyOptions{.components = 6, .whiten = true},
};

auto flow_or = nje::make_flow(pca_spec);
if (!flow_or.ok()) throw std::runtime_error(flow_or.status().message);
auto flow = flow_or.move_value();

auto z_train_or = flow.learn_map(train_features, {}, {});
if (!z_train_or.ok()) throw std::runtime_error(z_train_or.status().message);
auto z_test_or = flow.map(test_features, {});
if (!z_test_or.ok()) throw std::runtime_error(z_test_or.status().message);
```

### Expected output

- reduced-dimension transformed matrices for train/test features.

## Recipe 11: One-Class Kernel Transform Flow

### API Signature

- `ModelSpec(task: Transform, family: Kernel, options: KernelFamilyOptions{.form = OneClass}) -> ModelSpec`
- `Flow::learn_map(batch: FeatureBatch, learn_options: TrainOptions = {}, infer_options: InferOptions = {}) -> StatusOr<Series>`
- `Flow::map(batch: FeatureBatch, options: InferOptions = {}) -> StatusOr<Series>`
- `io::save(path: string, flow: Flow, train_batch: LabeledBatch = {}, train_options: TrainOptions = {}) -> Status`
- `io::load_and_restore(path: string) -> StatusOr<Flow>`

### Notes

One-class kernel workflows are transform tasks. They store learned transform state in the flow, so a fitted flow can be saved and restored without needing to retrain from a fallback batch.

### Inputs

- feature-only training batch
- kernel options with `form = KernelForm::OneClass`

### Output

- one label per sample
- restored flow that maps the same features to the same labels

### Options

- `KernelKind::Rbf` is the default kernel
- `gamma` controls kernel width
- `nu` controls the one-class acceptance fraction

### Examples

```cpp
namespace nje = pluss::ai::nodej;

nje::ModelSpec spec{
  .task = nje::TaskKind::Transform,
  .family = nje::ModelFamily::Kernel,
  .options = nje::KernelFamilyOptions{
      .kernel = nje::KernelKind::Rbf,
      .form = nje::KernelForm::OneClass,
      .gamma = 0.8,
      .nu = 0.5,
  },
};

auto flow_or = nje::make_flow(spec);
if (!flow_or.ok()) return flow_or.status();
auto flow = flow_or.move_value();

auto labels_or = flow.learn_map(train_features);
if (!labels_or.ok()) return labels_or.status();

auto save_status = nje::io::save("one_class_flow.bin", flow, nje::LabeledBatch{}, {});
if (!save_status.ok()) return save_status;

auto restored_or = nje::io::load_and_restore("one_class_flow.bin");
if (!restored_or.ok()) return restored_or.status();

auto restored_labels_or = restored_or.value().map(train_features);
if (!restored_labels_or.ok()) return restored_labels_or.status();
```

### Expected output

- `labels_or` and `restored_labels_or` have the same length and labels for the same input batch.
- inlier samples receive positive labels and outlier samples receive negative labels.

## Recipe 12: Multinomial And Complement Bayes Count Inputs

### API Signature

- `ModelSpec(task: Classification|Regression, family: Bayes, options: BayesFamilyOptions{.likelihood = Multinomial}) -> ModelSpec`
- `ModelSpec(task: Classification, family: Bayes, options: BayesFamilyOptions{.likelihood = Complement}) -> ModelSpec`
- `make_classifier(spec: ModelSpec) -> StatusOr<unique_ptr<ClassifierBase>>`
- `make_regressor(spec: ModelSpec) -> StatusOr<unique_ptr<RegressorBase>>`

### Notes

Multinomial and Complement Bayes treat features as counts. Negative feature values are rejected during learning and inference. For multinomial Bayes regression, labels must also be finite and non-negative. Complement Bayes is classifier-only.

### Inputs

- count-like feature matrix
- classification labels or non-negative regression labels

### Output

- predictions for valid non-negative count inputs
- `InvalidArgument` status for negative counts
- `NotImplemented` status when Complement likelihood is requested for Bayes regression

### Options

- `BayesLikelihood::Multinomial`
- `BayesLikelihood::Complement`
- `smoothing`

### Examples

```cpp
nje::ModelSpec spec{
  .task = nje::TaskKind::Classification,
  .family = nje::ModelFamily::Bayes,
  .options = nje::BayesFamilyOptions{
      .likelihood = nje::BayesLikelihood::Multinomial,
      .smoothing = 1e-6,
  },
};

auto model_or = nje::make_classifier(spec);
if (!model_or.ok()) return model_or.status();
auto model = model_or.move_value();

nje::Status status = model->learn(count_batch);
if (!status.ok()) return status;

auto pred_or = model->infer(count_features);
if (!pred_or.ok()) return pred_or.status();
```

```cpp
nje::ModelSpec complement_spec{
  .task = nje::TaskKind::Classification,
  .family = nje::ModelFamily::Bayes,
  .options = nje::BayesFamilyOptions{
      .likelihood = nje::BayesLikelihood::Complement,
      .smoothing = 1e-6,
  },
};

auto complement_or = nje::make_classifier(complement_spec);
if (!complement_or.ok()) return complement_or.status();
auto complement = complement_or.move_value();

auto complement_status = complement->learn(count_batch);
if (!complement_status.ok()) return complement_status;

auto complement_pred_or = complement->infer(count_features);
if (!complement_pred_or.ok()) return complement_pred_or.status();
```

### Expected output

- valid non-negative counts train and infer successfully.
- negative counts return `InvalidArgument`; they are not silently clamped to zero.
- Complement Bayes uses complement-count weights and is intended for classification.

## Recipe 13: Online Linear Updates

### API Signature

- `online::make_online_linear_regressor(options: OnlineLinearOptions = {}) -> StatusOr<OnlineLinearRegressor>`
- `OnlineLinearRegressor::learn(batch: LabeledBatch, options: TrainOptions = {}) -> Status`
- `OnlineLinearRegressor::advance(batch: LabeledBatch, options: TrainOptions = {}) -> Status`

### Notes

Use online path for stream updates without full retraining.

### Inputs

- sequential labeled chunks

### Output

- updated online model

### Options

- `OnlineLinearOptions.learning_rate`
- `OnlineLinearOptions.decaying_rate`

### Examples

```cpp
auto online_or = nje::online::make_online_linear_regressor({
    .learning_rate = 0.03,
    .decaying_rate = true,
});
if (!online_or.ok()) throw std::runtime_error(online_or.status().message);
auto online = online_or.move_value();

if (!online.learn(chunk0, {}).ok()) throw std::runtime_error("learn failed");
if (!online.advance(chunk1, {}).ok()) throw std::runtime_error("advance failed");
if (!online.advance(chunk2, {}).ok()) throw std::runtime_error("advance failed");
```

### Expected output

- online model tracks stream updates and remains inferable.

## Recipe 14: Fail-Loud Status Pattern

### API Signature

- `Status::ok() -> bool`
- `StatusOr<T>::ok() -> bool`
- `StatusOr<T>::status() -> Status`

### Notes

NodeJ APIs are intentionally explicit; always branch on `ok()` before value extraction.

### Inputs

- any NodeJ call returning `Status` / `StatusOr<T>`

### Output

- predictable error handling behavior

### Options

- none

### Examples

```cpp
auto out = flow.infer(test_batch.features);
if (!out.ok()) {
  std::cerr << "infer failed: " << out.status().message << "\n";
  return;
}
Series pred = out.value();
```

### Expected output

- controlled failure handling with actionable error messages.

## Option Coverage Reminder

- For exhaustive enum/bool branch coverage in Lymph wrappers, see `lymph_combination_atlas`.
- NodeJ uses the same underlying option structs, so the same branch-space applies at core level.

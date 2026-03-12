#include "pluss/ai/nodej/compose.h"
#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <string>
#include <vector>

namespace pluss::ai::nodej {
namespace {

[[nodiscard]] StatusOr<FeatureBatch> mapped_batch_checked(const Series& mapped,
                                                          ObservationLayout layout,
                                                          std::int64_t expected_samples,
                                                          std::int64_t stage_index,
                                                          const char* where) {
  FeatureBatch out{
      .x = mapped,
      .layout = layout,
  };
  Status check = validate_features(out);
  if (!check.ok()) {
    return StatusOr<FeatureBatch>(Status::shape_mismatch(
        std::string(where) + " stage " + std::to_string(stage_index) + " emitted invalid feature shape."));
  }
  if (out.sample_count() != expected_samples) {
    return StatusOr<FeatureBatch>(Status::shape_mismatch(
        std::string(where) + " stage " + std::to_string(stage_index) + " changed sample count."));
  }
  return StatusOr<FeatureBatch>(std::move(out));
}

[[nodiscard]] pluss::Config index_config_on(::pluss::Platform platform) {
  return pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(platform);
}

[[nodiscard]] Series make_index_series(std::vector<std::int64_t> ids, ::pluss::Platform platform) {
  return pluss::make_series(std::move(ids), {static_cast<std::int64_t>(ids.size())}, index_config_on(platform));
}

[[nodiscard]] StatusOr<std::vector<std::int64_t>> normalized_feature_route(const std::vector<std::int64_t>& route,
                                                                           std::int64_t feature_count,
                                                                           std::int64_t stage_index,
                                                                           const char* where) {
  if (route.empty()) return StatusOr<std::vector<std::int64_t>>(std::vector<std::int64_t>{});
  std::vector<std::int64_t> sorted = route;
  std::sort(sorted.begin(), sorted.end());
  if (std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end()) {
    return StatusOr<std::vector<std::int64_t>>(
        Status::invalid_argument(std::string(where) + " stage " + std::to_string(stage_index)
                                 + " route contains duplicate feature indices."));
  }
  for (std::int64_t idx : sorted) {
    if (idx < 0 || idx >= feature_count) {
      return StatusOr<std::vector<std::int64_t>>(
          Status::shape_mismatch(std::string(where) + " stage " + std::to_string(stage_index)
                                 + " route index out of range for current feature count."));
    }
  }
  return StatusOr<std::vector<std::int64_t>>(std::move(sorted));
}

[[nodiscard]] StatusOr<FeatureBatch> select_routed_features(const FeatureBatch& batch,
                                                            const std::vector<std::int64_t>& route,
                                                            std::int64_t stage_index,
                                                            const char* where) {
  Status check = validate_features(batch);
  if (!check.ok()) return StatusOr<FeatureBatch>(std::move(check));
  if (route.empty()) return StatusOr<FeatureBatch>(batch);

  FeatureBatch out = batch;
  const Series idx = make_index_series(route, batch.x.platform());
  if (batch.layout == ObservationLayout::Rows) {
    out.x = batch.x.index_select(1, idx);
  } else {
    out.x = batch.x.index_select(0, idx);
  }
  check = validate_features(out);
  if (!check.ok()) {
    return StatusOr<FeatureBatch>(
        Status::shape_mismatch(std::string(where) + " stage " + std::to_string(stage_index)
                               + " could not select routed feature subset."));
  }
  return StatusOr<FeatureBatch>(std::move(out));
}

[[nodiscard]] StatusOr<FeatureBatch> merge_routed_features(const FeatureBatch& original,
                                                           const FeatureBatch& routed_mapped,
                                                           const std::vector<std::int64_t>& route,
                                                           std::int64_t stage_index,
                                                           const char* where) {
  Status check = validate_features(original);
  if (!check.ok()) return StatusOr<FeatureBatch>(std::move(check));
  check = validate_features(routed_mapped);
  if (!check.ok()) return StatusOr<FeatureBatch>(std::move(check));
  if (original.layout != routed_mapped.layout) {
    return StatusOr<FeatureBatch>(Status::shape_mismatch(
        std::string(where) + " stage " + std::to_string(stage_index) + " route merge layout mismatch."));
  }
  if (original.sample_count() != routed_mapped.sample_count()) {
    return StatusOr<FeatureBatch>(Status::shape_mismatch(
        std::string(where) + " stage " + std::to_string(stage_index) + " route merge sample count mismatch."));
  }
  if (route.empty()) return StatusOr<FeatureBatch>(routed_mapped);

  const std::int64_t feature_count = original.feature_count();
  if (static_cast<std::int64_t>(route.size()) == feature_count) {
    return StatusOr<FeatureBatch>(routed_mapped);
  }

  std::vector<bool> selected(static_cast<std::size_t>(feature_count), false);
  for (std::int64_t idx : route) selected[static_cast<std::size_t>(idx)] = true;

  std::vector<Series> parts{};
  parts.reserve(static_cast<std::size_t>(feature_count - static_cast<std::int64_t>(route.size()) + 1));
  bool inserted = false;
  for (std::int64_t f = 0; f < feature_count; ++f) {
    if (selected[static_cast<std::size_t>(f)]) {
      if (!inserted) {
        parts.push_back(routed_mapped.x);
        inserted = true;
      }
      continue;
    }
    const Series single = make_index_series(std::vector<std::int64_t>{f}, original.x.platform());
    const Series col =
        original.layout == ObservationLayout::Rows ? original.x.index_select(1, single) : original.x.index_select(0, single);
    parts.push_back(col);
  }

  if (parts.empty()) {
    return StatusOr<FeatureBatch>(Status::internal(std::string(where) + " stage " + std::to_string(stage_index)
                                                   + " route merge produced no output parts."));
  }

  Series merged = parts.front();
  const std::int64_t axis = original.layout == ObservationLayout::Rows ? 1 : 0;
  for (std::size_t i = 1; i < parts.size(); ++i) {
    merged = merged.unite({parts[i]}, axis);
  }

  FeatureBatch out{
      .x = merged,
      .layout = original.layout,
  };
  check = validate_features(out);
  if (!check.ok()) {
    return StatusOr<FeatureBatch>(Status::shape_mismatch(std::string(where) + " stage " + std::to_string(stage_index)
                                                         + " route merge emitted invalid feature shape."));
  }
  return StatusOr<FeatureBatch>(std::move(out));
}

} // namespace

bool Flow::trained() const noexcept { return trained_; }

bool Flow::has_model() const noexcept { return static_cast<bool>(classifier_) || static_cast<bool>(regressor_); }

TaskKind Flow::task_kind() const noexcept {
  if (classifier_) return TaskKind::Classification;
  if (regressor_) return TaskKind::Regression;
  return TaskKind::Transform;
}

std::int64_t Flow::stage_count() const noexcept { return static_cast<std::int64_t>(transforms_.size()); }

bool Flow::has_model_spec() const noexcept { return model_spec_.has_value(); }

bool Flow::has_opaque_transforms() const noexcept { return has_opaque_transforms_; }

const std::optional<ModelSpec>& Flow::model_spec() const noexcept { return model_spec_; }

const std::vector<TransformSpec>& Flow::transform_specs() const noexcept { return transform_specs_; }

Status Flow::append_transform(std::unique_ptr<TransformerBase> transformer) {
  if (!transformer) return Status::invalid_argument("Flow::append_transform requires a non-null transformer.");
  transforms_.push_back(std::move(transformer));
  has_opaque_transforms_ = true;
  trained_ = false;
  return Status::success();
}

void Flow::clear_transforms() noexcept {
  transforms_.clear();
  transform_specs_.clear();
  has_opaque_transforms_ = false;
  trained_ = false;
}

Status Flow::add_standard_scaler(prep::StandardScalerOptions options, std::vector<std::int64_t> feature_route) {
  transforms_.push_back(std::make_unique<prep::StandardScaler>(options));
  transform_specs_.push_back(TransformSpec{
      .kind = TransformKind::StandardScaler,
      .options = options,
      .feature_route = std::move(feature_route),
  });
  trained_ = false;
  return Status::success();
}

Status Flow::add_minmax_scaler(prep::MinMaxScalerOptions options, std::vector<std::int64_t> feature_route) {
  transforms_.push_back(std::make_unique<prep::MinMaxScaler>(options));
  transform_specs_.push_back(TransformSpec{
      .kind = TransformKind::MinMaxScaler,
      .options = options,
      .feature_route = std::move(feature_route),
  });
  trained_ = false;
  return Status::success();
}

Status Flow::add_robust_scaler(prep::RobustScalerOptions options, std::vector<std::int64_t> feature_route) {
  transforms_.push_back(std::make_unique<prep::RobustScaler>(options));
  transform_specs_.push_back(TransformSpec{
      .kind = TransformKind::RobustScaler,
      .options = options,
      .feature_route = std::move(feature_route),
  });
  trained_ = false;
  return Status::success();
}

Status Flow::add_simple_imputer(prep::SimpleImputerOptions options, std::vector<std::int64_t> feature_route) {
  transforms_.push_back(std::make_unique<prep::SimpleImputer>(options));
  transform_specs_.push_back(TransformSpec{
      .kind = TransformKind::SimpleImputer,
      .options = options,
      .feature_route = std::move(feature_route),
  });
  trained_ = false;
  return Status::success();
}

Status Flow::add_neighbor_imputer(prep::NeighborImputerOptions options, std::vector<std::int64_t> feature_route) {
  transforms_.push_back(std::make_unique<prep::NeighborImputer>(options));
  transform_specs_.push_back(TransformSpec{
      .kind = TransformKind::NeighborImputer,
      .options = options,
      .feature_route = std::move(feature_route),
  });
  trained_ = false;
  return Status::success();
}

Status Flow::add_iterative_imputer(prep::IterativeImputerOptions options, std::vector<std::int64_t> feature_route) {
  transforms_.push_back(std::make_unique<prep::IterativeImputer>(options));
  transform_specs_.push_back(TransformSpec{
      .kind = TransformKind::IterativeImputer,
      .options = options,
      .feature_route = std::move(feature_route),
  });
  trained_ = false;
  return Status::success();
}

Status Flow::add_one_hot_encoder(prep::OneHotEncoderOptions options, std::vector<std::int64_t> feature_route) {
  transforms_.push_back(std::make_unique<prep::OneHotEncoder>(options));
  transform_specs_.push_back(TransformSpec{
      .kind = TransformKind::OneHotEncoder,
      .options = options,
      .feature_route = std::move(feature_route),
  });
  trained_ = false;
  return Status::success();
}

Status Flow::add_frequency_encoder(prep::FrequencyEncoderOptions options, std::vector<std::int64_t> feature_route) {
  transforms_.push_back(std::make_unique<prep::FrequencyEncoder>(options));
  transform_specs_.push_back(TransformSpec{
      .kind = TransformKind::FrequencyEncoder,
      .options = options,
      .feature_route = std::move(feature_route),
  });
  trained_ = false;
  return Status::success();
}

Status Flow::add_vector_normalizer(prep::VectorNormalizerOptions options, std::vector<std::int64_t> feature_route) {
  transforms_.push_back(std::make_unique<prep::VectorNormalizer>(options));
  transform_specs_.push_back(TransformSpec{
      .kind = TransformKind::VectorNormalizer,
      .options = options,
      .feature_route = std::move(feature_route),
  });
  trained_ = false;
  return Status::success();
}

Status Flow::add_maxabs_scaler(prep::MaxAbsScalerOptions options, std::vector<std::int64_t> feature_route) {
  transforms_.push_back(std::make_unique<prep::MaxAbsScaler>(options));
  transform_specs_.push_back(TransformSpec{
      .kind = TransformKind::MaxAbsScaler,
      .options = options,
      .feature_route = std::move(feature_route),
  });
  trained_ = false;
  return Status::success();
}

Status Flow::add_quantile_map(prep::QuantileMapOptions options, std::vector<std::int64_t> feature_route) {
  transforms_.push_back(std::make_unique<prep::QuantileMap>(options));
  transform_specs_.push_back(TransformSpec{
      .kind = TransformKind::QuantileMap,
      .options = options,
      .feature_route = std::move(feature_route),
  });
  trained_ = false;
  return Status::success();
}

Status Flow::add_power_map(prep::PowerMapOptions options, std::vector<std::int64_t> feature_route) {
  transforms_.push_back(std::make_unique<prep::PowerMap>(options));
  transform_specs_.push_back(TransformSpec{
      .kind = TransformKind::PowerMap,
      .options = options,
      .feature_route = std::move(feature_route),
  });
  trained_ = false;
  return Status::success();
}

Status Flow::add_threshold_binarizer(prep::ThresholdBinarizerOptions options, std::vector<std::int64_t> feature_route) {
  transforms_.push_back(std::make_unique<prep::ThresholdBinarizer>(options));
  transform_specs_.push_back(TransformSpec{
      .kind = TransformKind::ThresholdBinarizer,
      .options = options,
      .feature_route = std::move(feature_route),
  });
  trained_ = false;
  return Status::success();
}

Status Flow::add_polynomial_lift(prep::PolynomialLiftOptions options, std::vector<std::int64_t> feature_route) {
  transforms_.push_back(std::make_unique<prep::PolynomialLift>(options));
  transform_specs_.push_back(TransformSpec{
      .kind = TransformKind::PolynomialLift,
      .options = options,
      .feature_route = std::move(feature_route),
  });
  trained_ = false;
  return Status::success();
}

Status Flow::set_model(const ModelSpec& spec) {
  clear_model();
  if (spec.task == TaskKind::Classification) {
    StatusOr<std::unique_ptr<ClassifierBase>> cls_or = make_classifier(spec);
    if (!cls_or.ok()) return cls_or.status();
    classifier_ = cls_or.move_value();
    model_spec_ = spec;
    trained_ = false;
    return Status::success();
  }
  if (spec.task == TaskKind::Regression) {
    StatusOr<std::unique_ptr<RegressorBase>> reg_or = make_regressor(spec);
    if (!reg_or.ok()) return reg_or.status();
    regressor_ = reg_or.move_value();
    model_spec_ = spec;
    trained_ = false;
    return Status::success();
  }
  return Status::invalid_argument("Flow::set_model supports classification or regression ModelSpec tasks only.");
}

Status Flow::set_classifier(std::unique_ptr<ClassifierBase> model) {
  if (!model) return Status::invalid_argument("Flow::set_classifier requires a non-null model.");
  regressor_.reset();
  classifier_ = std::move(model);
  model_spec_.reset();
  trained_ = false;
  return Status::success();
}

Status Flow::set_regressor(std::unique_ptr<RegressorBase> model) {
  if (!model) return Status::invalid_argument("Flow::set_regressor requires a non-null model.");
  classifier_.reset();
  regressor_ = std::move(model);
  model_spec_.reset();
  trained_ = false;
  return Status::success();
}

void Flow::clear_model() noexcept {
  classifier_.reset();
  regressor_.reset();
  model_spec_.reset();
  trained_ = false;
}

StatusOr<FeatureBatch> Flow::apply_map(const FeatureBatch& batch, const InferOptions& options) const {
  Status input_status = validate_features(batch);
  if (!input_status.ok()) return StatusOr<FeatureBatch>(std::move(input_status));

  if (transforms_.empty()) return StatusOr<FeatureBatch>(batch);
  const std::int64_t expected_samples = batch.sample_count();
  FeatureBatch current = batch;
  for (std::size_t i = 0; i < transforms_.size(); ++i) {
    if (!transforms_[i]) {
      return StatusOr<FeatureBatch>(Status::internal("Flow contains a null transformer stage."));
    }
    const std::vector<std::int64_t> stage_route =
        i < transform_specs_.size() ? transform_specs_[i].feature_route : std::vector<std::int64_t>{};
    if (stage_route.empty()) {
      StatusOr<Series> mapped_or = transforms_[i]->map(current, options);
      if (!mapped_or.ok()) return StatusOr<FeatureBatch>(mapped_or.status());
      StatusOr<FeatureBatch> next_or =
          mapped_batch_checked(mapped_or.move_value(), current.layout, expected_samples, static_cast<std::int64_t>(i), "Flow::map");
      if (!next_or.ok()) return StatusOr<FeatureBatch>(next_or.status());
      current = next_or.move_value();
      continue;
    }

    StatusOr<std::vector<std::int64_t>> route_or = normalized_feature_route(
        stage_route, current.feature_count(), static_cast<std::int64_t>(i), "Flow::map");
    if (!route_or.ok()) return StatusOr<FeatureBatch>(route_or.status());
    const std::vector<std::int64_t> route = route_or.move_value();

    StatusOr<FeatureBatch> routed_or = select_routed_features(current, route, static_cast<std::int64_t>(i), "Flow::map");
    if (!routed_or.ok()) return StatusOr<FeatureBatch>(routed_or.status());

    StatusOr<Series> mapped_or = transforms_[i]->map(routed_or.value(), options);
    if (!mapped_or.ok()) return StatusOr<FeatureBatch>(mapped_or.status());
    StatusOr<FeatureBatch> mapped_routed_or = mapped_batch_checked(
        mapped_or.move_value(), current.layout, expected_samples, static_cast<std::int64_t>(i), "Flow::map");
    if (!mapped_routed_or.ok()) return StatusOr<FeatureBatch>(mapped_routed_or.status());

    StatusOr<FeatureBatch> merged_or = merge_routed_features(
        current, mapped_routed_or.value(), route, static_cast<std::int64_t>(i), "Flow::map");
    if (!merged_or.ok()) return StatusOr<FeatureBatch>(merged_or.status());
    current = merged_or.move_value();
  }
  return StatusOr<FeatureBatch>(std::move(current));
}

StatusOr<FeatureBatch> Flow::apply_learn_map(const FeatureBatch& batch,
                                             const TrainOptions& learn_options,
                                             const InferOptions& infer_options) {
  Status input_status = validate_features(batch);
  if (!input_status.ok()) return StatusOr<FeatureBatch>(std::move(input_status));

  if (transforms_.empty()) return StatusOr<FeatureBatch>(batch);
  const std::int64_t expected_samples = batch.sample_count();
  FeatureBatch current = batch;
  for (std::size_t i = 0; i < transforms_.size(); ++i) {
    if (!transforms_[i]) {
      return StatusOr<FeatureBatch>(Status::internal("Flow contains a null transformer stage."));
    }
    const std::vector<std::int64_t> stage_route =
        i < transform_specs_.size() ? transform_specs_[i].feature_route : std::vector<std::int64_t>{};
    if (stage_route.empty()) {
      StatusOr<Series> mapped_or = transforms_[i]->learn_map(current, learn_options, infer_options);
      if (!mapped_or.ok()) return StatusOr<FeatureBatch>(mapped_or.status());
      StatusOr<FeatureBatch> next_or = mapped_batch_checked(
          mapped_or.move_value(), current.layout, expected_samples, static_cast<std::int64_t>(i), "Flow::learn");
      if (!next_or.ok()) return StatusOr<FeatureBatch>(next_or.status());
      current = next_or.move_value();
      continue;
    }

    StatusOr<std::vector<std::int64_t>> route_or = normalized_feature_route(
        stage_route, current.feature_count(), static_cast<std::int64_t>(i), "Flow::learn");
    if (!route_or.ok()) return StatusOr<FeatureBatch>(route_or.status());
    const std::vector<std::int64_t> route = route_or.move_value();

    StatusOr<FeatureBatch> routed_or = select_routed_features(current, route, static_cast<std::int64_t>(i), "Flow::learn");
    if (!routed_or.ok()) return StatusOr<FeatureBatch>(routed_or.status());

    StatusOr<Series> mapped_or = transforms_[i]->learn_map(routed_or.value(), learn_options, infer_options);
    if (!mapped_or.ok()) return StatusOr<FeatureBatch>(mapped_or.status());
    StatusOr<FeatureBatch> mapped_routed_or = mapped_batch_checked(
        mapped_or.move_value(), current.layout, expected_samples, static_cast<std::int64_t>(i), "Flow::learn");
    if (!mapped_routed_or.ok()) return StatusOr<FeatureBatch>(mapped_routed_or.status());

    StatusOr<FeatureBatch> merged_or = merge_routed_features(
        current, mapped_routed_or.value(), route, static_cast<std::int64_t>(i), "Flow::learn");
    if (!merged_or.ok()) return StatusOr<FeatureBatch>(merged_or.status());
    current = merged_or.move_value();
  }
  return StatusOr<FeatureBatch>(std::move(current));
}

Status Flow::learn(const LabeledBatch& batch, const TrainOptions& options) {
  Status data_status = validate_labeled(batch);
  if (!data_status.ok()) return data_status;

  const InferOptions infer_options{
      .layout = batch.features.layout,
      .missing = options.missing,
      .unknown_categorical = options.unknown_categorical,
  };
  StatusOr<FeatureBatch> mapped_or = apply_learn_map(batch.features, options, infer_options);
  if (!mapped_or.ok()) return mapped_or.status();

  if (!has_model()) {
    trained_ = true;
    return Status::success();
  }

  const LabeledBatch learned_batch{
      .features = mapped_or.move_value(),
      .y = batch.y,
      .sample_weights = batch.sample_weights,
  };
  Status model_status = classifier_ ? classifier_->learn(learned_batch, options) : regressor_->learn(learned_batch, options);
  if (!model_status.ok()) return model_status;
  trained_ = true;
  return Status::success();
}

Status Flow::learn(const pluss::table::DataFrame& frame,
                   const DataFrameBatchOptions& data_options,
                   const TrainOptions& options) {
  StatusOr<LabeledBatch> batch_or = labeled_batch_from_dataframe(frame, data_options);
  if (!batch_or.ok()) {
    return batch_or.status();
  }
  return learn(batch_or.value(), options);
}

StatusOr<Series> Flow::map(const FeatureBatch& batch, const InferOptions& options) const {
  StatusOr<FeatureBatch> mapped_or = apply_map(batch, options);
  if (!mapped_or.ok()) return StatusOr<Series>(mapped_or.status());
  return StatusOr<Series>(mapped_or.move_value().x);
}

StatusOr<Series> Flow::learn_map(const FeatureBatch& batch,
                                 const TrainOptions& learn_options,
                                 const InferOptions& infer_options) {
  StatusOr<FeatureBatch> mapped_or = apply_learn_map(batch, learn_options, infer_options);
  if (!mapped_or.ok()) return StatusOr<Series>(mapped_or.status());
  if (!has_model()) {
    trained_ = true;
  } else {
    trained_ = false;
  }
  return StatusOr<Series>(mapped_or.move_value().x);
}

Status Flow::check_ready_for_infer(const char* where) const {
  if (!has_model()) {
    return Status::precondition(std::string("Flow requires a model before ") + where + "().");
  }
  if (!trained_) {
    return Status::precondition(std::string("Flow must be learned before ") + where + "().");
  }
  return Status::success();
}

StatusOr<Series> Flow::infer(const FeatureBatch& batch, const InferOptions& options) const {
  Status ready = check_ready_for_infer("infer");
  if (!ready.ok()) return StatusOr<Series>(std::move(ready));

  StatusOr<FeatureBatch> mapped_or = apply_map(batch, options);
  if (!mapped_or.ok()) return StatusOr<Series>(mapped_or.status());
  return classifier_ ? classifier_->infer(mapped_or.value(), options) : regressor_->infer(mapped_or.value(), options);
}

StatusOr<Series> Flow::infer(const pluss::table::DataFrame& frame,
                             const DataFrameBatchOptions& data_options,
                             const InferOptions& options) const {
  StatusOr<FeatureBatch> batch_or = feature_batch_from_dataframe(frame, data_options);
  if (!batch_or.ok()) {
    return StatusOr<Series>(batch_or.status());
  }
  return infer(batch_or.value(), options);
}

StatusOr<Series> Flow::infer_scores(const FeatureBatch& batch, const InferOptions& options) const {
  Status ready = check_ready_for_infer("infer_scores");
  if (!ready.ok()) return StatusOr<Series>(std::move(ready));
  if (!classifier_) {
    return StatusOr<Series>(Status::invalid_argument("Flow::infer_scores is available for classification flows only."));
  }

  StatusOr<FeatureBatch> mapped_or = apply_map(batch, options);
  if (!mapped_or.ok()) return StatusOr<Series>(mapped_or.status());
  return classifier_->infer_scores(mapped_or.value(), options);
}

StatusOr<Series> Flow::infer_scores(const pluss::table::DataFrame& frame,
                                    const DataFrameBatchOptions& data_options,
                                    const InferOptions& options) const {
  StatusOr<FeatureBatch> batch_or = feature_batch_from_dataframe(frame, data_options);
  if (!batch_or.ok()) {
    return StatusOr<Series>(batch_or.status());
  }
  return infer_scores(batch_or.value(), options);
}

StatusOr<Series> Flow::assess(const LabeledBatch& batch, const AssessOptions& options) const {
  Status ready = check_ready_for_infer("assess");
  if (!ready.ok()) return StatusOr<Series>(std::move(ready));

  Status data_status = validate_labeled(batch);
  if (!data_status.ok()) return StatusOr<Series>(std::move(data_status));

  const InferOptions infer_options{
      .layout = options.layout,
      .missing = MissingPolicy::KeepNaN,
      .unknown_categorical = UnknownCategoryPolicy::Ignore,
  };
  StatusOr<FeatureBatch> mapped_or = apply_map(batch.features, infer_options);
  if (!mapped_or.ok()) return StatusOr<Series>(mapped_or.status());

  const LabeledBatch mapped_batch{
      .features = mapped_or.move_value(),
      .y = batch.y,
      .sample_weights = batch.sample_weights,
  };
  return classifier_ ? classifier_->assess(mapped_batch, options) : regressor_->assess(mapped_batch, options);
}

StatusOr<Series> Flow::assess(const pluss::table::DataFrame& frame,
                              const DataFrameBatchOptions& data_options,
                              const AssessOptions& options) const {
  StatusOr<LabeledBatch> batch_or = labeled_batch_from_dataframe(frame, data_options);
  if (!batch_or.ok()) {
    return StatusOr<Series>(batch_or.status());
  }
  return assess(batch_or.value(), options);
}

StatusOr<std::vector<std::vector<Series>>> Flow::dump_transform_states() const {
  std::vector<std::vector<Series>> states{};
  states.reserve(transforms_.size());
  for (std::size_t i = 0; i < transforms_.size(); ++i) {
    if (!transforms_[i]) {
      return StatusOr<std::vector<std::vector<Series>>>(Status::internal("Flow contains a null transformer stage."));
    }
    StatusOr<std::vector<Series>> state_or = transforms_[i]->dump_state();
    if (!state_or.ok()) {
      return StatusOr<std::vector<std::vector<Series>>>(Status::not_implemented(
          "Flow::dump_transform_states failed at stage " + std::to_string(i) + ": " + state_or.status().message));
    }
    states.push_back(state_or.move_value());
  }
  return StatusOr<std::vector<std::vector<Series>>>(std::move(states));
}

StatusOr<std::vector<Series>> Flow::dump_model_state() const {
  if (!has_model()) return StatusOr<std::vector<Series>>(std::vector<Series>{});
  if (classifier_) return classifier_->dump_state();
  return regressor_->dump_state();
}

Status Flow::load_transform_states(const std::vector<std::vector<Series>>& states) {
  if (states.size() != transforms_.size()) {
    return Status::shape_mismatch("Flow::load_transform_states requires one state per transform stage.");
  }
  for (std::size_t i = 0; i < transforms_.size(); ++i) {
    if (!transforms_[i]) return Status::internal("Flow contains a null transformer stage.");
    Status status = transforms_[i]->load_state(states[i]);
    if (!status.ok()) {
      return Status::invalid_argument("Flow::load_transform_states failed at stage " + std::to_string(i) + ": "
                                      + status.message);
    }
  }
  return Status::success();
}

Status Flow::load_model_state(const std::vector<Series>& state) {
  if (!has_model()) {
    if (!state.empty()) {
      return Status::invalid_argument("Flow::load_model_state received model state for a transform-only flow.");
    }
    return Status::success();
  }
  if (state.empty()) {
    return Status::invalid_argument("Flow::load_model_state requires non-empty model state for model flows.");
  }
  return classifier_ ? classifier_->load_state(state) : regressor_->load_state(state);
}

void Flow::mark_trained(bool trained) noexcept { trained_ = trained; }

StatusOr<Flow> make_flow(const ModelSpec& spec) {
  Flow flow{};
  Status status = flow.set_model(spec);
  if (!status.ok()) return StatusOr<Flow>(std::move(status));
  return StatusOr<Flow>(std::move(flow));
}

} // namespace pluss::ai::nodej

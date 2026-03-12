#include "pluss/ai/nodej/compose.h"

#include <string>

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

} // namespace

bool Flow::trained() const noexcept { return trained_; }

bool Flow::has_model() const noexcept { return static_cast<bool>(classifier_) || static_cast<bool>(regressor_); }

TaskKind Flow::task_kind() const noexcept {
  if (classifier_) return TaskKind::Classification;
  if (regressor_) return TaskKind::Regression;
  return TaskKind::Transform;
}

std::int64_t Flow::stage_count() const noexcept { return static_cast<std::int64_t>(transforms_.size()); }

Status Flow::append_transform(std::unique_ptr<TransformerBase> transformer) {
  if (!transformer) return Status::invalid_argument("Flow::append_transform requires a non-null transformer.");
  transforms_.push_back(std::move(transformer));
  trained_ = false;
  return Status::success();
}

void Flow::clear_transforms() noexcept {
  transforms_.clear();
  trained_ = false;
}

Status Flow::add_standard_scaler(prep::StandardScalerOptions options) {
  return append_transform(std::make_unique<prep::StandardScaler>(options));
}

Status Flow::add_minmax_scaler(prep::MinMaxScalerOptions options) {
  return append_transform(std::make_unique<prep::MinMaxScaler>(options));
}

Status Flow::add_robust_scaler(prep::RobustScalerOptions options) {
  return append_transform(std::make_unique<prep::RobustScaler>(options));
}

Status Flow::add_simple_imputer(prep::SimpleImputerOptions options) {
  return append_transform(std::make_unique<prep::SimpleImputer>(options));
}

Status Flow::add_one_hot_encoder(prep::OneHotEncoderOptions options) {
  return append_transform(std::make_unique<prep::OneHotEncoder>(options));
}

Status Flow::add_frequency_encoder(prep::FrequencyEncoderOptions options) {
  return append_transform(std::make_unique<prep::FrequencyEncoder>(options));
}

Status Flow::set_model(const ModelSpec& spec) {
  clear_model();
  if (spec.task == TaskKind::Classification) {
    StatusOr<std::unique_ptr<ClassifierBase>> cls_or = make_classifier(spec);
    if (!cls_or.ok()) return cls_or.status();
    classifier_ = cls_or.move_value();
    trained_ = false;
    return Status::success();
  }
  if (spec.task == TaskKind::Regression) {
    StatusOr<std::unique_ptr<RegressorBase>> reg_or = make_regressor(spec);
    if (!reg_or.ok()) return reg_or.status();
    regressor_ = reg_or.move_value();
    trained_ = false;
    return Status::success();
  }
  return Status::invalid_argument("Flow::set_model supports classification or regression ModelSpec tasks only.");
}

Status Flow::set_classifier(std::unique_ptr<ClassifierBase> model) {
  if (!model) return Status::invalid_argument("Flow::set_classifier requires a non-null model.");
  regressor_.reset();
  classifier_ = std::move(model);
  trained_ = false;
  return Status::success();
}

Status Flow::set_regressor(std::unique_ptr<RegressorBase> model) {
  if (!model) return Status::invalid_argument("Flow::set_regressor requires a non-null model.");
  classifier_.reset();
  regressor_ = std::move(model);
  trained_ = false;
  return Status::success();
}

void Flow::clear_model() noexcept {
  classifier_.reset();
  regressor_.reset();
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
    StatusOr<Series> mapped_or = transforms_[i]->map(current, options);
    if (!mapped_or.ok()) return StatusOr<FeatureBatch>(mapped_or.status());
    StatusOr<FeatureBatch> next_or =
        mapped_batch_checked(mapped_or.move_value(), current.layout, expected_samples, static_cast<std::int64_t>(i), "Flow::map");
    if (!next_or.ok()) return StatusOr<FeatureBatch>(next_or.status());
    current = next_or.move_value();
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
    StatusOr<Series> mapped_or = transforms_[i]->learn_map(current, learn_options, infer_options);
    if (!mapped_or.ok()) return StatusOr<FeatureBatch>(mapped_or.status());
    StatusOr<FeatureBatch> next_or = mapped_batch_checked(
        mapped_or.move_value(), current.layout, expected_samples, static_cast<std::int64_t>(i), "Flow::learn");
    if (!next_or.ok()) return StatusOr<FeatureBatch>(next_or.status());
    current = next_or.move_value();
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

StatusOr<Flow> make_flow(const ModelSpec& spec) {
  Flow flow{};
  Status status = flow.set_model(spec);
  if (!status.ok()) return StatusOr<Flow>(std::move(status));
  return StatusOr<Flow>(std::move(flow));
}

} // namespace pluss::ai::nodej


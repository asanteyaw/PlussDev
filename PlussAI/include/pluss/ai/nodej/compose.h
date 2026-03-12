#pragma once

#include "pluss/ai/nodej/builders.h"
#include "pluss/ai/nodej/prep.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace pluss::ai::nodej {

class Flow final {
public:
  Flow() = default;
  Flow(Flow&&) noexcept = default;
  Flow& operator=(Flow&&) noexcept = default;
  Flow(const Flow&) = delete;
  Flow& operator=(const Flow&) = delete;

  [[nodiscard]] bool trained() const noexcept;
  [[nodiscard]] bool has_model() const noexcept;
  [[nodiscard]] TaskKind task_kind() const noexcept;
  [[nodiscard]] std::int64_t stage_count() const noexcept;

  Status append_transform(std::unique_ptr<TransformerBase> transformer);
  void clear_transforms() noexcept;

  Status add_standard_scaler(prep::StandardScalerOptions options = {});
  Status add_minmax_scaler(prep::MinMaxScalerOptions options = {});
  Status add_robust_scaler(prep::RobustScalerOptions options = {});
  Status add_simple_imputer(prep::SimpleImputerOptions options = {});
  Status add_one_hot_encoder(prep::OneHotEncoderOptions options = {});
  Status add_frequency_encoder(prep::FrequencyEncoderOptions options = {});

  Status set_model(const ModelSpec& spec);
  Status set_classifier(std::unique_ptr<ClassifierBase> model);
  Status set_regressor(std::unique_ptr<RegressorBase> model);
  void clear_model() noexcept;

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {});

  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const;
  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {});

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch, const InferOptions& options = {}) const;
  [[nodiscard]] StatusOr<Series> infer_scores(const FeatureBatch& batch, const InferOptions& options = {}) const;
  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch, const AssessOptions& options = {}) const;

private:
  [[nodiscard]] StatusOr<FeatureBatch> apply_map(const FeatureBatch& batch, const InferOptions& options) const;
  [[nodiscard]] StatusOr<FeatureBatch> apply_learn_map(const FeatureBatch& batch,
                                                       const TrainOptions& learn_options,
                                                       const InferOptions& infer_options);
  [[nodiscard]] Status check_ready_for_infer(const char* where) const;

  std::vector<std::unique_ptr<TransformerBase>> transforms_{};
  std::unique_ptr<ClassifierBase> classifier_{};
  std::unique_ptr<RegressorBase> regressor_{};
  bool trained_{false};
};

[[nodiscard]] StatusOr<Flow> make_flow(const ModelSpec& spec);

} // namespace pluss::ai::nodej


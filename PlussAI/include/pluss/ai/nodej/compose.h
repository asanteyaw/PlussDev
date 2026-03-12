#pragma once

#include "pluss/ai/nodej/builders.h"
#include "pluss/ai/nodej/data.h"
#include "pluss/ai/nodej/prep.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

namespace pluss::ai::nodej {

enum class TransformKind : std::uint8_t {
  StandardScaler = 0,
  MinMaxScaler = 1,
  RobustScaler = 2,
  SimpleImputer = 3,
  NeighborImputer = 4,
  IterativeImputer = 5,
  OneHotEncoder = 6,
  FrequencyEncoder = 7,
  VectorNormalizer = 8,
  MaxAbsScaler = 9,
  QuantileMap = 10,
  PowerMap = 11,
  ThresholdBinarizer = 12,
  PolynomialLift = 13,
};

using TransformOptions = std::variant<prep::StandardScalerOptions,
                                      prep::MinMaxScalerOptions,
                                      prep::RobustScalerOptions,
                                      prep::SimpleImputerOptions,
                                      prep::NeighborImputerOptions,
                                      prep::IterativeImputerOptions,
                                      prep::OneHotEncoderOptions,
                                      prep::FrequencyEncoderOptions,
                                      prep::VectorNormalizerOptions,
                                      prep::MaxAbsScalerOptions,
                                      prep::QuantileMapOptions,
                                      prep::PowerMapOptions,
                                      prep::ThresholdBinarizerOptions,
                                      prep::PolynomialLiftOptions>;

struct TransformSpec {
  TransformKind kind = TransformKind::StandardScaler;
  TransformOptions options = prep::StandardScalerOptions{};
  std::vector<std::int64_t> feature_route{};
};

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
  [[nodiscard]] bool has_model_spec() const noexcept;
  [[nodiscard]] bool has_opaque_transforms() const noexcept;
  [[nodiscard]] const std::optional<ModelSpec>& model_spec() const noexcept;
  [[nodiscard]] const std::vector<TransformSpec>& transform_specs() const noexcept;

  Status append_transform(std::unique_ptr<TransformerBase> transformer);
  void clear_transforms() noexcept;

  Status add_standard_scaler(prep::StandardScalerOptions options = {}, std::vector<std::int64_t> feature_route = {});
  Status add_minmax_scaler(prep::MinMaxScalerOptions options = {}, std::vector<std::int64_t> feature_route = {});
  Status add_robust_scaler(prep::RobustScalerOptions options = {}, std::vector<std::int64_t> feature_route = {});
  Status add_simple_imputer(prep::SimpleImputerOptions options = {}, std::vector<std::int64_t> feature_route = {});
  Status add_neighbor_imputer(prep::NeighborImputerOptions options = {}, std::vector<std::int64_t> feature_route = {});
  Status add_iterative_imputer(prep::IterativeImputerOptions options = {}, std::vector<std::int64_t> feature_route = {});
  Status add_one_hot_encoder(prep::OneHotEncoderOptions options = {}, std::vector<std::int64_t> feature_route = {});
  Status add_frequency_encoder(prep::FrequencyEncoderOptions options = {}, std::vector<std::int64_t> feature_route = {});
  Status add_vector_normalizer(prep::VectorNormalizerOptions options = {}, std::vector<std::int64_t> feature_route = {});
  Status add_maxabs_scaler(prep::MaxAbsScalerOptions options = {}, std::vector<std::int64_t> feature_route = {});
  Status add_quantile_map(prep::QuantileMapOptions options = {}, std::vector<std::int64_t> feature_route = {});
  Status add_power_map(prep::PowerMapOptions options = {}, std::vector<std::int64_t> feature_route = {});
  Status add_threshold_binarizer(prep::ThresholdBinarizerOptions options = {}, std::vector<std::int64_t> feature_route = {});
  Status add_polynomial_lift(prep::PolynomialLiftOptions options = {}, std::vector<std::int64_t> feature_route = {});

  Status set_model(const ModelSpec& spec);
  Status set_classifier(std::unique_ptr<ClassifierBase> model);
  Status set_regressor(std::unique_ptr<RegressorBase> model);
  void clear_model() noexcept;

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {});
  Status learn(const pluss::table::DataFrame& frame,
               const DataFrameBatchOptions& data_options,
               const TrainOptions& options = {});

  [[nodiscard]] StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const;
  [[nodiscard]] StatusOr<Series> learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options = {},
                                           const InferOptions& infer_options = {});

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch, const InferOptions& options = {}) const;
  [[nodiscard]] StatusOr<Series> infer(const pluss::table::DataFrame& frame,
                                       const DataFrameBatchOptions& data_options,
                                       const InferOptions& options = {}) const;
  [[nodiscard]] StatusOr<Series> infer_scores(const FeatureBatch& batch, const InferOptions& options = {}) const;
  [[nodiscard]] StatusOr<Series> infer_scores(const pluss::table::DataFrame& frame,
                                              const DataFrameBatchOptions& data_options,
                                              const InferOptions& options = {}) const;
  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch, const AssessOptions& options = {}) const;
  [[nodiscard]] StatusOr<Series> assess(const pluss::table::DataFrame& frame,
                                        const DataFrameBatchOptions& data_options,
                                        const AssessOptions& options = {}) const;

  [[nodiscard]] StatusOr<std::vector<std::vector<Series>>> dump_transform_states() const;
  [[nodiscard]] StatusOr<std::vector<Series>> dump_model_state() const;
  Status load_transform_states(const std::vector<std::vector<Series>>& states);
  Status load_model_state(const std::vector<Series>& state);
  void mark_trained(bool trained) noexcept;

private:
  [[nodiscard]] StatusOr<FeatureBatch> apply_map(const FeatureBatch& batch, const InferOptions& options) const;
  [[nodiscard]] StatusOr<FeatureBatch> apply_learn_map(const FeatureBatch& batch,
                                                       const TrainOptions& learn_options,
                                                       const InferOptions& infer_options);
  [[nodiscard]] Status check_ready_for_infer(const char* where) const;

  std::vector<std::unique_ptr<TransformerBase>> transforms_{};
  std::optional<ModelSpec> model_spec_{};
  std::vector<TransformSpec> transform_specs_{};
  bool has_opaque_transforms_{false};
  std::unique_ptr<ClassifierBase> classifier_{};
  std::unique_ptr<RegressorBase> regressor_{};
  bool trained_{false};
};

[[nodiscard]] StatusOr<Flow> make_flow(const ModelSpec& spec);
using Workflow = Flow;
[[nodiscard]] inline StatusOr<Workflow> make_workflow(const ModelSpec& spec) { return make_flow(spec); }

} // namespace pluss::ai::nodej

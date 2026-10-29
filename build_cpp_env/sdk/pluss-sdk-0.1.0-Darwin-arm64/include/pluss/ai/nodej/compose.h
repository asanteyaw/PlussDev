#pragma once

#include "pluss/ai/nodej/builders.h"
#include "pluss/ai/nodej/data.h"
#include "pluss/ai/nodej/prep.h"
#include "MSharp/RangeView.h"

#include <cstdint>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
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
  OrdinalEncoder = 14,
  LabelEncoder = 15,
  BaseNEncoder = 16,
  HashingEncoder = 17,
  TargetEncoder = 18,
  WOEEncoder = 19,
};

enum class RouteRemainderPolicy : std::uint8_t {
  PassThrough = 0,
  Drop = 1,
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
                                      prep::PolynomialLiftOptions,
                                      prep::OrdinalEncoderOptions,
                                      prep::LabelEncoderOptions,
                                      prep::BaseNEncoderOptions,
                                      prep::HashingEncoderOptions,
                                      prep::TargetEncoderOptions,
                                      prep::WOEEncoderOptions>;

struct TransformSpec {
  TransformKind kind = TransformKind::StandardScaler;
  TransformOptions options = prep::StandardScalerOptions{};
  std::vector<std::int64_t> feature_route{};
  RouteRemainderPolicy remainder = RouteRemainderPolicy::PassThrough;
  std::string stage_key{};
};

struct FlowStageInfo {
  std::int64_t index = 0;
  std::string stage_key{};
  TransformKind kind = TransformKind::StandardScaler;
  std::int64_t routed_feature_count = 0;
  RouteRemainderPolicy remainder = RouteRemainderPolicy::PassThrough;
  bool route_active = false;
};

struct FlowGraphInfo {
  TaskKind task = TaskKind::Transform;
  bool trained = false;
  bool has_model = false;
  bool has_opaque_transforms = false;
  std::vector<FlowStageInfo> stages{};
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
  [[nodiscard]] StatusOr<FlowGraphInfo> graph_info() const;
  [[nodiscard]] std::vector<std::string> nested_param_keys() const;
  [[nodiscard]] StatusOr<double> nested_param(std::string_view path) const;
  Status set_nested_param(std::string_view path, double value);
  [[nodiscard]] std::string flow_summary() const;
  [[nodiscard]] StatusOr<std::string> save_blueprint() const;
  Status load_blueprint(std::string_view text);
  [[nodiscard]] StatusOr<Flow> clone() const;

  Status add_named_transform(TransformKind kind,
                             const TransformOptions& options,
                             std::string stage_key = {},
                             pluss::msharp::RangeView::Int feature_route = {},
                             RouteRemainderPolicy remainder = RouteRemainderPolicy::PassThrough);
  Status add_column_stage(std::string stage_key,
                          TransformKind kind,
                          const TransformOptions& options,
                          pluss::msharp::RangeView::Int feature_route = {},
                          RouteRemainderPolicy remainder = RouteRemainderPolicy::PassThrough);
  Status add_column_stage(std::string stage_key,
                          TransformKind kind,
                          const TransformOptions& options,
                          std::initializer_list<std::int64_t> feature_route,
                          RouteRemainderPolicy remainder = RouteRemainderPolicy::PassThrough) {
    return add_column_stage(std::move(stage_key), kind, options, pluss::msharp::RangeView::Int(feature_route), remainder);
  }
  Status set_stage_key(std::int64_t stage_index, std::string stage_key);
  Status set_stage_route(std::int64_t stage_index, pluss::msharp::RangeView::Int feature_route);
  Status set_stage_route(std::int64_t stage_index, std::initializer_list<std::int64_t> feature_route) {
    return set_stage_route(stage_index, pluss::msharp::RangeView::Int(feature_route));
  }
  Status set_stage_remainder(std::int64_t stage_index, RouteRemainderPolicy remainder);

  Status append_transform(std::unique_ptr<TransformerBase> transformer);
  void clear_transforms() noexcept;

  Status add_standard_scaler(prep::StandardScalerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {});
  Status add_minmax_scaler(prep::MinMaxScalerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {});
  Status add_robust_scaler(prep::RobustScalerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {});
  Status add_simple_imputer(prep::SimpleImputerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {});
  Status add_neighbor_imputer(prep::NeighborImputerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {});
  Status add_iterative_imputer(prep::IterativeImputerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {});
  Status add_one_hot_encoder(prep::OneHotEncoderOptions options = {}, pluss::msharp::RangeView::Int feature_route = {});
  Status add_frequency_encoder(prep::FrequencyEncoderOptions options = {}, pluss::msharp::RangeView::Int feature_route = {});
  Status add_vector_normalizer(prep::VectorNormalizerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {});
  Status add_maxabs_scaler(prep::MaxAbsScalerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {});
  Status add_quantile_map(prep::QuantileMapOptions options = {}, pluss::msharp::RangeView::Int feature_route = {});
  Status add_power_map(prep::PowerMapOptions options = {}, pluss::msharp::RangeView::Int feature_route = {});
  Status add_threshold_binarizer(prep::ThresholdBinarizerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {});
  Status add_polynomial_lift(prep::PolynomialLiftOptions options = {}, pluss::msharp::RangeView::Int feature_route = {});
  Status add_ordinal_encoder(prep::OrdinalEncoderOptions options = {}, pluss::msharp::RangeView::Int feature_route = {});
  Status add_label_encoder(prep::LabelEncoderOptions options = {}, pluss::msharp::RangeView::Int feature_route = {});
  Status add_basen_encoder(prep::BaseNEncoderOptions options = {}, pluss::msharp::RangeView::Int feature_route = {});
  Status add_hashing_encoder(prep::HashingEncoderOptions options = {}, pluss::msharp::RangeView::Int feature_route = {});
  Status add_target_encoder(prep::TargetEncoderOptions options = {}, pluss::msharp::RangeView::Int feature_route = {});
  Status add_woe_encoder(prep::WOEEncoderOptions options = {}, pluss::msharp::RangeView::Int feature_route = {});
  Status add_function_transformer(prep::FunctionTransformer transformer);
  Status add_feature_transformer(prep::FeatureTransformer transformer);

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
  Status load_transform_states(pluss::msharp::RangeView::Of<std::vector<Series>> states);
  Status load_model_state(pluss::msharp::RangeView::Series state);
  void mark_trained(bool trained) noexcept;

private:
  [[nodiscard]] StatusOr<FeatureBatch> apply_map(const FeatureBatch& batch, const InferOptions& options) const;
  [[nodiscard]] StatusOr<FeatureBatch> apply_learn_map(const FeatureBatch& batch,
                                                       const TrainOptions& learn_options,
                                                       const InferOptions& infer_options,
                                                       const Series* target = nullptr);
  [[nodiscard]] Status check_ready_for_infer(const char* where) const;
  // Removes the TaskKind::Transform model set_model() may have pushed onto
  // transforms_ (see has_transform_model_ below), if any. No-op otherwise.
  void clear_transform_model() noexcept;

  std::vector<std::unique_ptr<TransformerBase>> transforms_{};
  std::optional<ModelSpec> model_spec_{};
  std::vector<TransformSpec> transform_specs_{};
  bool has_opaque_transforms_{false};
  std::unique_ptr<ClassifierBase> classifier_{};
  std::unique_ptr<RegressorBase> regressor_{};
  // true when transforms_.back() is the model set_model() built from a
  // TaskKind::Transform ModelSpec, rather than an ordinary transform stage.
  // Such a model carries no transform_specs_ entry (it isn't one), so it is
  // always kept at the tail: add_named_transform()/append_transform() refuse
  // to add anything after it (see their transforms_/transform_specs_ size
  // check), which is what makes "the last entry, if this flag is set" a
  // sound way to find it again for replacement or removal.
  bool has_transform_model_{false};
  bool trained_{false};
};

[[nodiscard]] StatusOr<Flow> make_flow(const ModelSpec& spec);
using Workflow = Flow;
[[nodiscard]] inline StatusOr<Workflow> make_workflow(const ModelSpec& spec) { return make_flow(spec); }

} // namespace pluss::ai::nodej

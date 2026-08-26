#pragma once

#include "pluss/ai/lymph/common.h"
#include "pluss/ai/lymph/stage.h"

namespace pluss::ai::lymph {

namespace detail {

struct FeatureSelectorOptions_ {
  std::vector<pluss::table::ThreadKind> include{};
  std::vector<pluss::table::ThreadKind> exclude{};
};

[[nodiscard]] inline std::vector<std::string> make_feature_selector_(const pluss::table::DataFrame& frame,
                                                                     FeatureSelectorOptions_ options = {}) {
  const auto names = frame.features();
  std::vector<std::string> selected{};
  selected.reserve(names.size());

  const auto has_kind = [](pluss::msharp::RangeView::Of<pluss::table::ThreadKind> kinds, pluss::table::ThreadKind kind) {
    return std::find(kinds.begin(), kinds.end(), kind) != kinds.end();
  };

  for (const auto& name : names) {
    const auto kind = frame.features(name).kind();
    const bool include_match = options.include.empty() || has_kind(options.include, kind);
    const bool exclude_match = has_kind(options.exclude, kind);
    if (include_match && !exclude_match) selected.push_back(name);
  }
  return selected;
}

[[nodiscard]] inline std::vector<std::string> temporal_features_(const pluss::table::DataFrame& frame) {
  return make_feature_selector_(frame, FeatureSelectorOptions_{
                                           .include = {pluss::table::ThreadKind::Timeline, pluss::table::ThreadKind::TimeIndex},
                                           .exclude = {},
                                       });
}

[[nodiscard]] std::vector<std::string> resolved_feature_order_for_features_(
    const pluss::table::DataFrame& frame,
    const DataFrameBatchOptions& data_options);

[[nodiscard]] std::unique_ptr<TransformerBase> make_core_transformer_from_spec_(const TransformSpec& spec);

} // namespace detail

namespace prep::branch {

struct Branch {
  std::vector<Stage> stages{};
  std::vector<std::string> feature_names{};
  std::vector<std::int64_t> feature_route{};
};

[[nodiscard]] inline Branch compose(std::vector<Stage> stages,
                                    std::vector<std::string> feature_names = {},
                                    pluss::msharp::RangeView::Int feature_route = {}) {
  return Branch{
      .stages = std::move(stages),
      .feature_names = std::move(feature_names),
      .feature_route = copy_feature_route(feature_route),
  };
}

#define PLUSS_LYMPH_PREP_BRANCH_BUILDER(FuncName, OptionsType)                                               \
  [[nodiscard]] inline Branch FuncName(OptionsType options = {},                                             \
                                       std::vector<std::string> feature_names = {},                          \
                                       pluss::msharp::RangeView::Int feature_route = {}) {                   \
    return Branch{                                                                                            \
        .stages = {stage::FuncName(options)},                                                                 \
        .feature_names = std::move(feature_names),                                                            \
        .feature_route = copy_feature_route(feature_route),                                                   \
    };                                                                                                        \
  }

PLUSS_LYMPH_PREP_BRANCH_BUILDER(standard_scaler, prep::StandardScalerOptions)
PLUSS_LYMPH_PREP_BRANCH_BUILDER(minmax_scaler, prep::MinMaxScalerOptions)
PLUSS_LYMPH_PREP_BRANCH_BUILDER(robust_scaler, prep::RobustScalerOptions)
PLUSS_LYMPH_PREP_BRANCH_BUILDER(simple_imputer, prep::SimpleImputerOptions)
PLUSS_LYMPH_PREP_BRANCH_BUILDER(neighbor_imputer, prep::NeighborImputerOptions)
PLUSS_LYMPH_PREP_BRANCH_BUILDER(iterative_imputer, prep::IterativeImputerOptions)
PLUSS_LYMPH_PREP_BRANCH_BUILDER(one_hot_encoder, prep::OneHotEncoderOptions)
PLUSS_LYMPH_PREP_BRANCH_BUILDER(frequency_encoder, prep::FrequencyEncoderOptions)
PLUSS_LYMPH_PREP_BRANCH_BUILDER(ordinal_encoder, prep::OrdinalEncoderOptions)
PLUSS_LYMPH_PREP_BRANCH_BUILDER(label_encoder, prep::LabelEncoderOptions)
PLUSS_LYMPH_PREP_BRANCH_BUILDER(vector_normalizer, prep::VectorNormalizerOptions)
PLUSS_LYMPH_PREP_BRANCH_BUILDER(maxabs_scaler, prep::MaxAbsScalerOptions)
PLUSS_LYMPH_PREP_BRANCH_BUILDER(quantile_map, prep::QuantileMapOptions)
PLUSS_LYMPH_PREP_BRANCH_BUILDER(power_map, prep::PowerMapOptions)
PLUSS_LYMPH_PREP_BRANCH_BUILDER(threshold_binarizer, prep::ThresholdBinarizerOptions)
PLUSS_LYMPH_PREP_BRANCH_BUILDER(polynomial_lift, prep::PolynomialLiftOptions)

#undef PLUSS_LYMPH_PREP_BRANCH_BUILDER

} // namespace prep::branch

namespace prep {

using Branch = branch::Branch;

// Defined in Lymph/src/workflow_dsl.cpp (real logic, not a one-line forwarder).
[[nodiscard]] ::pluss::ai::nodej::prep::FeatureTransformer feature_transformer(
    const std::vector<std::string>& feature_order,
    const std::vector<branch::Branch>& branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough);

[[nodiscard]] inline ::pluss::ai::nodej::prep::FeatureTransformer feature_transformer(
    pluss::msharp::RangeView::Of<std::string> feature_order,
    pluss::msharp::RangeView::Of<branch::Branch> branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough) {
  return feature_transformer(copy_range_values(feature_order), copy_range_values(branches), remainder);
}

[[nodiscard]] inline ::pluss::ai::nodej::prep::FeatureTransformer feature_transformer(
    std::initializer_list<std::string> feature_order,
    std::initializer_list<branch::Branch> branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough) {
  return feature_transformer(
      pluss::msharp::RangeView::Of<std::string>(feature_order),
      pluss::msharp::RangeView::Of<branch::Branch>(branches),
      remainder);
}

[[nodiscard]] inline ::pluss::ai::nodej::prep::FeatureTransformer feature_transformer(
    const pluss::table::DataFrame& frame,
    const DataFrameBatchOptions& data_options,
    const std::vector<branch::Branch>& branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough) {
  return feature_transformer(
      detail::resolved_feature_order_for_features_(frame, data_options),
      branches,
      remainder);
}

[[nodiscard]] inline ::pluss::ai::nodej::prep::FeatureTransformer feature_transformer(
    const pluss::table::DataFrame& frame,
    const DataFrameBatchOptions& data_options,
    pluss::msharp::RangeView::Of<branch::Branch> branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough) {
  return feature_transformer(frame, data_options, copy_range_values(branches), remainder);
}

[[nodiscard]] inline ::pluss::ai::nodej::prep::FeatureTransformer feature_transformer(
    const pluss::table::DataFrame& frame,
    const DataFrameBatchOptions& data_options,
    std::initializer_list<branch::Branch> branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough) {
  return feature_transformer(
      frame,
      data_options,
      pluss::msharp::RangeView::Of<branch::Branch>(branches),
      remainder);
}

[[nodiscard]] inline ::pluss::ai::nodej::prep::FeatureTransformer feature_transformer(
    const pluss::table::DataFrame& frame,
    const std::vector<branch::Branch>& branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough) {
  DataFrameBatchOptions options{};
  options.feature_names = frame.features();
  return feature_transformer(frame, options, branches, remainder);
}

[[nodiscard]] inline ::pluss::ai::nodej::prep::FeatureTransformer feature_transformer(
    const pluss::table::DataFrame& frame,
    pluss::msharp::RangeView::Of<branch::Branch> branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough) {
  DataFrameBatchOptions options{};
  options.feature_names = frame.features();
  return feature_transformer(frame, options, branches, remainder);
}

[[nodiscard]] inline ::pluss::ai::nodej::prep::FeatureTransformer feature_transformer(
    const pluss::table::DataFrame& frame,
    std::initializer_list<branch::Branch> branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough) {
  return feature_transformer(
      frame,
      pluss::msharp::RangeView::Of<branch::Branch>(branches),
      remainder);
}

} // namespace prep

} // namespace pluss::ai::lymph

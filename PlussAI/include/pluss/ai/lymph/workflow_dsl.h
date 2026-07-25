#pragma once

#include "pluss/ai/lymph/common.h"
#include "pluss/ai/lymph/prep.h"

namespace pluss::ai::lymph {

struct FeatureSelectorOptions {
  std::vector<pluss::table::ThreadKind> include{};
  std::vector<pluss::table::ThreadKind> exclude{};
};

[[nodiscard]] inline std::vector<std::string> make_feature_selector(const pluss::table::DataFrame& frame,
                                                                   FeatureSelectorOptions options = {}) {
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

[[nodiscard]] inline std::vector<std::string> temporal_features(const pluss::table::DataFrame& frame) {
  return make_feature_selector(frame, FeatureSelectorOptions{
                                         .include = {pluss::table::ThreadKind::Timeline, pluss::table::ThreadKind::TimeIndex},
                                         .exclude = {},
                                     });
}

namespace workflow {
namespace feature {

struct Branch {
  std::vector<TransformSpec> transforms{};
  std::vector<std::string> feature_names{};
  std::vector<std::int64_t> feature_route{};
};

[[nodiscard]] inline Branch compose(std::vector<TransformSpec> transforms,
                                    std::vector<std::string> feature_names = {},
                                    pluss::msharp::RangeView::Int feature_route = {}) {
  return Branch{
      .transforms = std::move(transforms),
      .feature_names = std::move(feature_names),
      .feature_route = copy_feature_route(feature_route),
  };
}

#define PLUSS_LYMPH_FEATURE_BRANCH_BUILDER(FuncName, KindName, OptionsType)                                 \
  [[nodiscard]] inline Branch FuncName(OptionsType options = {},                                             \
                                       std::vector<std::string> feature_names = {},                          \
                                       pluss::msharp::RangeView::Int feature_route = {}) {                   \
    return Branch{                                                                                            \
        .transforms = {TransformSpec{                                                                         \
            .kind = TransformKind::KindName,                                                                  \
            .options = options,                                                                               \
            .feature_route = {},                                                                              \
        }},                                                                                                   \
        .feature_names = std::move(feature_names),                                                            \
        .feature_route = copy_feature_route(feature_route),                                                   \
    };                                                                                                        \
  }

PLUSS_LYMPH_FEATURE_BRANCH_BUILDER(standard_scaler, StandardScaler, prep::StandardScalerOptions)
PLUSS_LYMPH_FEATURE_BRANCH_BUILDER(minmax_scaler, MinMaxScaler, prep::MinMaxScalerOptions)
PLUSS_LYMPH_FEATURE_BRANCH_BUILDER(robust_scaler, RobustScaler, prep::RobustScalerOptions)
PLUSS_LYMPH_FEATURE_BRANCH_BUILDER(simple_imputer, SimpleImputer, prep::SimpleImputerOptions)
PLUSS_LYMPH_FEATURE_BRANCH_BUILDER(neighbor_imputer, NeighborImputer, prep::NeighborImputerOptions)
PLUSS_LYMPH_FEATURE_BRANCH_BUILDER(iterative_imputer, IterativeImputer, prep::IterativeImputerOptions)
PLUSS_LYMPH_FEATURE_BRANCH_BUILDER(one_hot_encoder, OneHotEncoder, prep::OneHotEncoderOptions)
PLUSS_LYMPH_FEATURE_BRANCH_BUILDER(frequency_encoder, FrequencyEncoder, prep::FrequencyEncoderOptions)
PLUSS_LYMPH_FEATURE_BRANCH_BUILDER(ordinal_encoder, OrdinalEncoder, prep::OrdinalEncoderOptions)
PLUSS_LYMPH_FEATURE_BRANCH_BUILDER(label_encoder, LabelEncoder, prep::LabelEncoderOptions)
PLUSS_LYMPH_FEATURE_BRANCH_BUILDER(vector_normalizer, VectorNormalizer, prep::VectorNormalizerOptions)
PLUSS_LYMPH_FEATURE_BRANCH_BUILDER(maxabs_scaler, MaxAbsScaler, prep::MaxAbsScalerOptions)
PLUSS_LYMPH_FEATURE_BRANCH_BUILDER(quantile_map, QuantileMap, prep::QuantileMapOptions)
PLUSS_LYMPH_FEATURE_BRANCH_BUILDER(power_map, PowerMap, prep::PowerMapOptions)
PLUSS_LYMPH_FEATURE_BRANCH_BUILDER(threshold_binarizer, ThresholdBinarizer, prep::ThresholdBinarizerOptions)
PLUSS_LYMPH_FEATURE_BRANCH_BUILDER(polynomial_lift, PolynomialLift, prep::PolynomialLiftOptions)

#undef PLUSS_LYMPH_FEATURE_BRANCH_BUILDER

} // namespace feature

namespace stage {

#define PLUSS_LYMPH_STAGE_BUILDER(FuncName, KindName, OptionsType)                                 \
  [[nodiscard]] inline TransformSpec FuncName(                                                      \
      OptionsType options = {},                                                                     \
      pluss::msharp::RangeView::Int feature_route = {}) {                                           \
    return TransformSpec{                                                                           \
        .kind = TransformKind::KindName,                                                            \
        .options = options,                                                                         \
        .feature_route = copy_feature_route(feature_route),                                         \
    };                                                                                              \
  }

PLUSS_LYMPH_STAGE_BUILDER(standard_scaler, StandardScaler, prep::StandardScalerOptions)
PLUSS_LYMPH_STAGE_BUILDER(minmax_scaler, MinMaxScaler, prep::MinMaxScalerOptions)
PLUSS_LYMPH_STAGE_BUILDER(robust_scaler, RobustScaler, prep::RobustScalerOptions)
PLUSS_LYMPH_STAGE_BUILDER(simple_imputer, SimpleImputer, prep::SimpleImputerOptions)
PLUSS_LYMPH_STAGE_BUILDER(neighbor_imputer, NeighborImputer, prep::NeighborImputerOptions)
PLUSS_LYMPH_STAGE_BUILDER(iterative_imputer, IterativeImputer, prep::IterativeImputerOptions)
PLUSS_LYMPH_STAGE_BUILDER(one_hot_encoder, OneHotEncoder, prep::OneHotEncoderOptions)
PLUSS_LYMPH_STAGE_BUILDER(frequency_encoder, FrequencyEncoder, prep::FrequencyEncoderOptions)
PLUSS_LYMPH_STAGE_BUILDER(ordinal_encoder, OrdinalEncoder, prep::OrdinalEncoderOptions)
PLUSS_LYMPH_STAGE_BUILDER(label_encoder, LabelEncoder, prep::LabelEncoderOptions)
PLUSS_LYMPH_STAGE_BUILDER(vector_normalizer, VectorNormalizer, prep::VectorNormalizerOptions)
PLUSS_LYMPH_STAGE_BUILDER(maxabs_scaler, MaxAbsScaler, prep::MaxAbsScalerOptions)
PLUSS_LYMPH_STAGE_BUILDER(quantile_map, QuantileMap, prep::QuantileMapOptions)
PLUSS_LYMPH_STAGE_BUILDER(power_map, PowerMap, prep::PowerMapOptions)
PLUSS_LYMPH_STAGE_BUILDER(threshold_binarizer, ThresholdBinarizer, prep::ThresholdBinarizerOptions)
PLUSS_LYMPH_STAGE_BUILDER(polynomial_lift, PolynomialLift, prep::PolynomialLiftOptions)

#undef PLUSS_LYMPH_STAGE_BUILDER

} // namespace stage
} // namespace workflow
// Defined in Lymph/workflow_dsl.cpp (real logic, not a one-line forwarder).
[[nodiscard]] std::vector<std::string> resolved_feature_order_for_features_(
    const pluss::table::DataFrame& frame,
    const DataFrameBatchOptions& data_options);

// Defined in Lymph/workflow_dsl.cpp (real logic, not a one-line forwarder).
[[nodiscard]] std::unique_ptr<TransformerBase> make_core_transformer_from_spec_(const TransformSpec& spec);

// Defined in Lymph/workflow_dsl.cpp (real logic, not a one-line forwarder) —
// every other make_feature_transformer/make_branch_transformer/branch_transformer
// overload below is a thin forwarder onto this one.
[[nodiscard]] ::pluss::ai::nodej::prep::FeatureTransformer make_feature_transformer(
    const std::vector<std::string>& feature_order,
    const std::vector<workflow::feature::Branch>& branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough);

[[nodiscard]] inline ::pluss::ai::nodej::prep::FeatureTransformer make_feature_transformer(
    pluss::msharp::RangeView::Of<std::string> feature_order,
    pluss::msharp::RangeView::Of<workflow::feature::Branch> branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough) {
  return make_feature_transformer(copy_range_values(feature_order), copy_range_values(branches), remainder);
}

[[nodiscard]] inline ::pluss::ai::nodej::prep::FeatureTransformer make_feature_transformer(
    std::initializer_list<std::string> feature_order,
    std::initializer_list<workflow::feature::Branch> branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough) {
  return make_feature_transformer(
      pluss::msharp::RangeView::Of<std::string>(feature_order),
      pluss::msharp::RangeView::Of<workflow::feature::Branch>(branches),
      remainder);
}

[[nodiscard]] inline ::pluss::ai::nodej::prep::FeatureTransformer make_feature_transformer(
    const pluss::table::DataFrame& frame,
    const DataFrameBatchOptions& data_options,
    const std::vector<workflow::feature::Branch>& branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough) {
  return make_feature_transformer(
      resolved_feature_order_for_features_(frame, data_options),
      branches,
      remainder);
}

[[nodiscard]] inline ::pluss::ai::nodej::prep::FeatureTransformer make_feature_transformer(
    const pluss::table::DataFrame& frame,
    const DataFrameBatchOptions& data_options,
    pluss::msharp::RangeView::Of<workflow::feature::Branch> branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough) {
  return make_feature_transformer(frame, data_options, copy_range_values(branches), remainder);
}

[[nodiscard]] inline ::pluss::ai::nodej::prep::FeatureTransformer make_feature_transformer(
    const pluss::table::DataFrame& frame,
    const DataFrameBatchOptions& data_options,
    std::initializer_list<workflow::feature::Branch> branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough) {
  return make_feature_transformer(
      frame,
      data_options,
      pluss::msharp::RangeView::Of<workflow::feature::Branch>(branches),
      remainder);
}

[[nodiscard]] inline ::pluss::ai::nodej::prep::FeatureTransformer make_feature_transformer(
    const pluss::table::DataFrame& frame,
    const std::vector<workflow::feature::Branch>& branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough) {
  DataFrameBatchOptions options{};
  options.feature_names = frame.features();
  return make_feature_transformer(frame, options, branches, remainder);
}

[[nodiscard]] inline ::pluss::ai::nodej::prep::FeatureTransformer make_feature_transformer(
    const pluss::table::DataFrame& frame,
    pluss::msharp::RangeView::Of<workflow::feature::Branch> branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough) {
  DataFrameBatchOptions options{};
  options.feature_names = frame.features();
  return make_feature_transformer(frame, options, branches, remainder);
}

[[nodiscard]] inline ::pluss::ai::nodej::prep::FeatureTransformer make_feature_transformer(
    const pluss::table::DataFrame& frame,
    std::initializer_list<workflow::feature::Branch> branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough) {
  return make_feature_transformer(
      frame,
      pluss::msharp::RangeView::Of<workflow::feature::Branch>(branches),
      remainder);
}

[[nodiscard]] inline ::pluss::ai::nodej::prep::FeatureTransformer make_branch_transformer(
    pluss::msharp::RangeView::Of<std::string> feature_order,
    pluss::msharp::RangeView::Of<workflow::feature::Branch> branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough) {
  return make_feature_transformer(feature_order, branches, remainder);
}

[[nodiscard]] inline ::pluss::ai::nodej::prep::FeatureTransformer make_branch_transformer(
    std::initializer_list<std::string> feature_order,
    std::initializer_list<workflow::feature::Branch> branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough) {
  return make_feature_transformer(feature_order, branches, remainder);
}

[[nodiscard]] inline ::pluss::ai::nodej::prep::FeatureTransformer make_branch_transformer(
    const pluss::table::DataFrame& frame,
    const DataFrameBatchOptions& data_options,
    pluss::msharp::RangeView::Of<workflow::feature::Branch> branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough) {
  return make_feature_transformer(frame, data_options, branches, remainder);
}

[[nodiscard]] inline ::pluss::ai::nodej::prep::FeatureTransformer make_branch_transformer(
    const pluss::table::DataFrame& frame,
    const DataFrameBatchOptions& data_options,
    std::initializer_list<workflow::feature::Branch> branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough) {
  return make_feature_transformer(frame, data_options, branches, remainder);
}

[[nodiscard]] inline ::pluss::ai::nodej::prep::FeatureTransformer make_branch_transformer(
    const pluss::table::DataFrame& frame,
    pluss::msharp::RangeView::Of<workflow::feature::Branch> branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough) {
  return make_feature_transformer(frame, branches, remainder);
}

[[nodiscard]] inline ::pluss::ai::nodej::prep::FeatureTransformer make_branch_transformer(
    const pluss::table::DataFrame& frame,
    std::initializer_list<workflow::feature::Branch> branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough) {
  return make_feature_transformer(frame, branches, remainder);
}

[[nodiscard]] inline ::pluss::ai::nodej::prep::FeatureTransformer branch_transformer(
    pluss::msharp::RangeView::Of<std::string> feature_order,
    pluss::msharp::RangeView::Of<workflow::feature::Branch> branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough) {
  return make_branch_transformer(feature_order, branches, remainder);
}

[[nodiscard]] inline ::pluss::ai::nodej::prep::FeatureTransformer branch_transformer(
    std::initializer_list<std::string> feature_order,
    std::initializer_list<workflow::feature::Branch> branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough) {
  return make_branch_transformer(feature_order, branches, remainder);
}

[[nodiscard]] inline ::pluss::ai::nodej::prep::FeatureTransformer branch_transformer(
    const pluss::table::DataFrame& frame,
    const DataFrameBatchOptions& data_options,
    pluss::msharp::RangeView::Of<workflow::feature::Branch> branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough) {
  return make_branch_transformer(frame, data_options, branches, remainder);
}

[[nodiscard]] inline ::pluss::ai::nodej::prep::FeatureTransformer branch_transformer(
    const pluss::table::DataFrame& frame,
    const DataFrameBatchOptions& data_options,
    std::initializer_list<workflow::feature::Branch> branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough) {
  return make_branch_transformer(frame, data_options, branches, remainder);
}

[[nodiscard]] inline ::pluss::ai::nodej::prep::FeatureTransformer branch_transformer(
    const pluss::table::DataFrame& frame,
    pluss::msharp::RangeView::Of<workflow::feature::Branch> branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough) {
  return make_branch_transformer(frame, branches, remainder);
}

[[nodiscard]] inline ::pluss::ai::nodej::prep::FeatureTransformer branch_transformer(
    const pluss::table::DataFrame& frame,
    std::initializer_list<workflow::feature::Branch> branches,
    prep::FeatureRemainderPolicy remainder = prep::FeatureRemainderPolicy::PassThrough) {
  return make_branch_transformer(frame, branches, remainder);
}


} // namespace pluss::ai::lymph

#include "pluss/ai/lymph/workflow_dsl.h"

namespace pluss::ai::lymph {

namespace detail {

[[nodiscard]] std::vector<std::string> resolved_feature_order_for_features_(
    const pluss::table::DataFrame& frame,
    const DataFrameBatchOptions& data_options) {
  std::vector<std::string> names = data_options.feature_names.empty() ? frame.features() : data_options.feature_names;
  if (data_options.label_feature.has_value()) {
    names.erase(std::remove(names.begin(), names.end(), *data_options.label_feature), names.end());
  }
  if (data_options.sample_weight_feature.has_value()) {
    names.erase(std::remove(names.begin(), names.end(), *data_options.sample_weight_feature), names.end());
  }
  if (names.empty()) {
    ::pluss::throw_error("make_feature_transformer: resolved zero feature names.");
  }
  return names;
}

[[nodiscard]] std::unique_ptr<TransformerBase> make_core_transformer_from_spec_(const TransformSpec& spec) {
  switch (spec.kind) {
    case TransformKind::StandardScaler:
      return std::make_unique<::pluss::ai::nodej::prep::StandardScaler>(
          std::get<prep::StandardScalerOptions>(spec.options));
    case TransformKind::MinMaxScaler:
      return std::make_unique<::pluss::ai::nodej::prep::MinMaxScaler>(
          std::get<prep::MinMaxScalerOptions>(spec.options));
    case TransformKind::RobustScaler:
      return std::make_unique<::pluss::ai::nodej::prep::RobustScaler>(
          std::get<prep::RobustScalerOptions>(spec.options));
    case TransformKind::SimpleImputer:
      return std::make_unique<::pluss::ai::nodej::prep::SimpleImputer>(
          std::get<prep::SimpleImputerOptions>(spec.options));
    case TransformKind::NeighborImputer:
      return std::make_unique<::pluss::ai::nodej::prep::NeighborImputer>(
          std::get<prep::NeighborImputerOptions>(spec.options));
    case TransformKind::IterativeImputer:
      return std::make_unique<::pluss::ai::nodej::prep::IterativeImputer>(
          std::get<prep::IterativeImputerOptions>(spec.options));
    case TransformKind::OneHotEncoder:
      return std::make_unique<::pluss::ai::nodej::prep::OneHotEncoder>(
          std::get<prep::OneHotEncoderOptions>(spec.options));
    case TransformKind::FrequencyEncoder:
      return std::make_unique<::pluss::ai::nodej::prep::FrequencyEncoder>(
          std::get<prep::FrequencyEncoderOptions>(spec.options));
    case TransformKind::OrdinalEncoder:
      return std::make_unique<::pluss::ai::nodej::prep::OrdinalEncoder>(
          std::get<prep::OrdinalEncoderOptions>(spec.options));
    case TransformKind::LabelEncoder:
      return std::make_unique<::pluss::ai::nodej::prep::LabelEncoder>(
          std::get<prep::LabelEncoderOptions>(spec.options));
    case TransformKind::VectorNormalizer:
      return std::make_unique<::pluss::ai::nodej::prep::VectorNormalizer>(
          std::get<prep::VectorNormalizerOptions>(spec.options));
    case TransformKind::MaxAbsScaler:
      return std::make_unique<::pluss::ai::nodej::prep::MaxAbsScaler>(
          std::get<prep::MaxAbsScalerOptions>(spec.options));
    case TransformKind::QuantileMap:
      return std::make_unique<::pluss::ai::nodej::prep::QuantileMap>(
          std::get<prep::QuantileMapOptions>(spec.options));
    case TransformKind::PowerMap:
      return std::make_unique<::pluss::ai::nodej::prep::PowerMap>(
          std::get<prep::PowerMapOptions>(spec.options));
    case TransformKind::ThresholdBinarizer:
      return std::make_unique<::pluss::ai::nodej::prep::ThresholdBinarizer>(
          std::get<prep::ThresholdBinarizerOptions>(spec.options));
    case TransformKind::PolynomialLift:
      return std::make_unique<::pluss::ai::nodej::prep::PolynomialLift>(
          std::get<prep::PolynomialLiftOptions>(spec.options));
    case TransformKind::BaseNEncoder:
      return std::make_unique<::pluss::ai::nodej::prep::BaseNEncoder>(
          std::get<prep::BaseNEncoderOptions>(spec.options));
    case TransformKind::HashingEncoder:
      return std::make_unique<::pluss::ai::nodej::prep::HashingEncoder>(
          std::get<prep::HashingEncoderOptions>(spec.options));
    case TransformKind::TargetEncoder:
      return std::make_unique<::pluss::ai::nodej::prep::TargetEncoder>(
          std::get<prep::TargetEncoderOptions>(spec.options));
    case TransformKind::WOEEncoder:
      return std::make_unique<::pluss::ai::nodej::prep::WOEEncoder>(
          std::get<prep::WOEEncoderOptions>(spec.options));
  }
  ::pluss::throw_error("make_feature_transformer: unsupported transform kind.");
}

} // namespace detail

namespace prep {

[[nodiscard]] ::pluss::ai::nodej::prep::FeatureTransformer feature_transformer(
    const std::vector<std::string>& feature_order,
    const std::vector<branch::Branch>& branches,
    prep::FeatureRemainderPolicy remainder) {
  if (feature_order.empty()) {
    ::pluss::throw_error("make_feature_transformer: feature_order cannot be empty.");
  }
  if (branches.empty()) {
    ::pluss::throw_error("make_feature_transformer: requires at least one branch.");
  }

  std::unordered_map<std::string, std::int64_t> index_by_name{};
  index_by_name.reserve(feature_order.size());
  for (std::size_t i = 0; i < feature_order.size(); ++i) {
    index_by_name.emplace(feature_order[i], static_cast<std::int64_t>(i));
  }

  ::pluss::ai::nodej::prep::FeatureTransformer feature_transformer(remainder);
  for (std::size_t i = 0; i < branches.size(); ++i) {
    const auto& branch = branches[i];
    if (branch.stages.empty()) {
      ::pluss::throw_error("make_feature_transformer: branch requires at least one transform stage.");
    }
    std::vector<std::int64_t> route{};
    if (!branch.feature_names.empty() || !branch.feature_route.empty()) {
      if (!branch.feature_names.empty() && !branch.feature_route.empty()) {
        ::pluss::throw_error(
            "make_feature_transformer: branch cannot specify both feature_names and feature_route.");
      }
      if (!branch.feature_names.empty()) {
        route.reserve(branch.feature_names.size());
        for (const auto& name : branch.feature_names) {
          auto it = index_by_name.find(name);
          if (it == index_by_name.end()) {
            ::pluss::throw_error(
              "make_feature_transformer: feature '" + name + "' was not found in feature_order.");
          }
          route.push_back(it->second);
        }
      } else {
        route = branch.feature_route;
      }
    } else {
      ::pluss::throw_error("make_feature_transformer: each branch must select at least one feature.");
    }
    if (route.empty()) {
      ::pluss::throw_error("make_feature_transformer: each branch must select at least one feature.");
    }

    std::vector<std::unique_ptr<TransformerBase>> branch_chain{};
    branch_chain.reserve(branch.stages.size());
    for (const auto& stage : branch.stages) {
      const TransformSpec spec{
          .kind = stage.kind,
          .options = stage.options,
          .feature_route = {},
      };
      branch_chain.push_back(detail::make_core_transformer_from_spec_(spec));
    }
    resolve(feature_transformer.add_pipeline(std::move(branch_chain), std::move(route)),
            "make_feature_transformer.add_branch");
  }
  return feature_transformer;
}

} // namespace prep

} // namespace pluss::ai::lymph

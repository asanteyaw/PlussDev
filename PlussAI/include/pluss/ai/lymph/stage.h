#pragma once

#include "pluss/ai/lymph/common.h"
#include "pluss/ai/lymph/columnrouter.h"
#include "pluss/ai/lymph/prep.h"

#include <functional>
#include <string>
#include <type_traits>
#include <utility>

// Stage -- one preprocessing step attachable to a Model via
// Model::append(Stage, ColumnRouter). See LYMPH_REDESIGN_PLAN.md 2 for why
// this is named Stage rather than Prep: lymph::prep:: (prep.h) already owns
// a real, unrelated namespace of standalone fittable transformer objects
// (StandardScaler, OneHotEncoder, ...) with their own .learn(frame)/
// .map(frame); reusing those same factory names for a second, unrelated
// return type would have collided outright.
//
// Deliberately the lightest possible value type: TransformKind + the same
// TransformOptions variant nodej::compose.h already uses for TransformSpec
// (StandardScalerOptions/OneHotEncoderOptions/... -- the exact option
// struct types prep.h's standalone wrappers already alias, so a Stage
// factory and its prep:: standalone-object counterpart take identical
// arguments, only the return type and role differ). No route/remainder
// here -- that lives on the ColumnRouter Model::append() takes alongside a
// Stage, resolved lazily (see detail/model_core.h's pending_stages_) once a
// real DataFrame is available, since .append() itself is always called
// before any frame exists.

namespace pluss::ai::lymph {

struct TransformContract {
  bool reversible = false;
  bool inverse_available = false;
  bool lossy = true;
  bool stateful = true;
  bool target_aware = false;
  bool changes_sample_count = false;
  bool changes_feature_count = false;
  bool needs_inverse_memory = false;
};

struct Stage {
  TransformKind kind = TransformKind::StandardScaler;
  TransformOptions options = prep::StandardScalerOptions{};

  [[nodiscard]] TransformContract contract() const noexcept {
    switch (kind) {
      case TransformKind::StandardScaler:
      case TransformKind::MinMaxScaler:
      case TransformKind::RobustScaler:
      case TransformKind::MaxAbsScaler:
        return TransformContract{
            .reversible = true,
            .inverse_available = false,
            .lossy = false,
            .stateful = true,
        };
      case TransformKind::SimpleImputer:
      case TransformKind::NeighborImputer:
      case TransformKind::IterativeImputer:
        return TransformContract{
            .reversible = false,
            .inverse_available = false,
            .lossy = true,
            .stateful = true,
            .needs_inverse_memory = true,
        };
      case TransformKind::OneHotEncoder:
      case TransformKind::FrequencyEncoder:
      case TransformKind::OrdinalEncoder:
      case TransformKind::LabelEncoder:
        return TransformContract{
            .reversible = true,
            .inverse_available = false,
            .lossy = kind == TransformKind::FrequencyEncoder,
            .stateful = true,
            .changes_feature_count = kind == TransformKind::OneHotEncoder,
        };
      case TransformKind::VectorNormalizer:
      case TransformKind::ThresholdBinarizer:
        return TransformContract{
            .reversible = false,
            .inverse_available = false,
            .lossy = true,
            .stateful = false,
        };
      case TransformKind::QuantileMap:
      case TransformKind::PowerMap:
        return TransformContract{
            .reversible = true,
            .inverse_available = false,
            .lossy = false,
            .stateful = true,
        };
      case TransformKind::PolynomialLift:
        return TransformContract{
            .reversible = false,
            .inverse_available = false,
            .lossy = true,
            .stateful = false,
            .changes_feature_count = true,
        };
    }
    return TransformContract{};
  }
};

struct CustomStage {
  using SeriesMapFn = prep::FunctionTransformer::SeriesMapFn;

  std::string name{};
  SeriesMapFn map_fn{};
  prep::FunctionTransformerOptions options{};
  TransformContract contract{};
  bool persistable = false;
};

namespace stage {

[[nodiscard]] inline TransformContract callable_contract() noexcept {
  return TransformContract{
      .reversible = false,
      .inverse_available = false,
      .lossy = true,
      .stateful = false,
  };
}

template <typename Fn>
requires std::is_invocable_v<Fn&, const Series&>
[[nodiscard]] inline CustomStage custom(std::string name,
                                        Fn map_fn,
                                        TransformContract contract = callable_contract(),
                                        prep::FunctionTransformerOptions options = {}) {
  if (name.empty()) {
    ::pluss::throw_error("lymph::stage::custom: custom stage name cannot be empty.");
  }
  CustomStage::SeriesMapFn wrapped = [fn = std::move(map_fn)](const Series& input) mutable -> StatusOr<Series> {
    using Return = std::invoke_result_t<Fn&, const Series&>;
    if constexpr (std::is_same_v<std::decay_t<Return>, StatusOr<Series>>) {
      return fn(input);
    } else {
      return StatusOr<Series>(Series(fn(input)));
    }
  };
  contract.inverse_available = false;
  return CustomStage{
      .name = std::move(name),
      .map_fn = std::move(wrapped),
      .options = options,
      .contract = contract,
      .persistable = false,
  };
}

[[nodiscard]] inline Stage standard_scaler(prep::StandardScalerOptions options = {}) {
  return Stage{.kind = TransformKind::StandardScaler, .options = options};
}

[[nodiscard]] inline Stage minmax_scaler(prep::MinMaxScalerOptions options = {}) {
  return Stage{.kind = TransformKind::MinMaxScaler, .options = options};
}

[[nodiscard]] inline Stage robust_scaler(prep::RobustScalerOptions options = {}) {
  return Stage{.kind = TransformKind::RobustScaler, .options = options};
}

[[nodiscard]] inline Stage simple_imputer(prep::SimpleImputerOptions options = {}) {
  return Stage{.kind = TransformKind::SimpleImputer, .options = options};
}

[[nodiscard]] inline Stage neighbor_imputer(prep::NeighborImputerOptions options = {}) {
  return Stage{.kind = TransformKind::NeighborImputer, .options = options};
}

[[nodiscard]] inline Stage iterative_imputer(prep::IterativeImputerOptions options = {}) {
  return Stage{.kind = TransformKind::IterativeImputer, .options = options};
}

[[nodiscard]] inline Stage one_hot_encoder(prep::OneHotEncoderOptions options = {}) {
  return Stage{.kind = TransformKind::OneHotEncoder, .options = options};
}

[[nodiscard]] inline Stage frequency_encoder(prep::FrequencyEncoderOptions options = {}) {
  return Stage{.kind = TransformKind::FrequencyEncoder, .options = options};
}

[[nodiscard]] inline Stage ordinal_encoder(prep::OrdinalEncoderOptions options = {}) {
  return Stage{.kind = TransformKind::OrdinalEncoder, .options = options};
}

[[nodiscard]] inline Stage label_encoder(prep::LabelEncoderOptions options = {}) {
  return Stage{.kind = TransformKind::LabelEncoder, .options = options};
}

[[nodiscard]] inline Stage vector_normalizer(prep::VectorNormalizerOptions options = {}) {
  return Stage{.kind = TransformKind::VectorNormalizer, .options = options};
}

[[nodiscard]] inline Stage maxabs_scaler(prep::MaxAbsScalerOptions options = {}) {
  return Stage{.kind = TransformKind::MaxAbsScaler, .options = options};
}

[[nodiscard]] inline Stage quantile_map(prep::QuantileMapOptions options = {}) {
  return Stage{.kind = TransformKind::QuantileMap, .options = options};
}

[[nodiscard]] inline Stage power_map(prep::PowerMapOptions options = {}) {
  return Stage{.kind = TransformKind::PowerMap, .options = options};
}

[[nodiscard]] inline Stage threshold_binarizer(prep::ThresholdBinarizerOptions options = {}) {
  return Stage{.kind = TransformKind::ThresholdBinarizer, .options = options};
}

[[nodiscard]] inline Stage polynomial_lift(prep::PolynomialLiftOptions options = {}) {
  return Stage{.kind = TransformKind::PolynomialLift, .options = options};
}

} // namespace stage

} // namespace pluss::ai::lymph

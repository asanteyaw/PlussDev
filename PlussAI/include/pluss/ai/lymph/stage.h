#pragma once

#include "pluss/ai/lymph/common.h"
#include "pluss/ai/lymph/columnrouter.h"
#include "pluss/ai/lymph/prep.h"

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
// Stage, resolved lazily (see workflow_model.h's pending_stages_) once a
// real DataFrame is available, since .append() itself is always called
// before any frame exists.

namespace pluss::ai::lymph {

struct Stage {
  TransformKind kind = TransformKind::StandardScaler;
  TransformOptions options = prep::StandardScalerOptions{};
};

namespace stage {

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

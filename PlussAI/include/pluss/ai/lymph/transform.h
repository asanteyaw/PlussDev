#pragma once

#include "pluss/ai/lymph/common.h"

#include <functional>
#include <string>
#include <utility>

// transform.h -- TargetTransform + lymph::transform::.
//
// Kept independent of Model so detail/model_core.h can hold a TargetTransform
// field without introducing a circular include. This file depends on nothing
// but common.h, and detail/model_core.h includes it instead.
//
// A named, invertible transform applied to the label column only -- forward
// before the model ever sees a value, inverse on every prediction it
// produces, so training happens in whatever space fits the model best (e.g.
// log-space for right-skewed currency/count targets) while every
// caller-facing number stays in the original units.

namespace pluss::ai::lymph {

struct TargetTransform {
  std::string name = "target_transform";
  std::function<Series(const Series&)> forward;
  std::function<Series(const Series&)> inverse;
  // Verified once per learn_() call: inverse(forward(y)) must round-trip
  // back to y within `check_inverse_tolerance`, or Model::
  // target_transform(...) -- Model::target_transform(...) -- fails loudly
  // instead of silently training against a transform that doesn't actually
  // invert (e.g. sqrt() handed negative values, or a hand-written inverse
  // with a sign/scale bug).
  bool check_inverse = true;
  double check_inverse_tolerance = 1e-3;
};

namespace transform {

// log1p/expm1 (rather than plain log/exp) tolerate a zero-valued target,
// the common case for right-skewed currency/count labels (transaction
// amounts, claim counts).
[[nodiscard]] inline TargetTransform log1p() {
  return TargetTransform{
      .name = "log1p",
      .forward = [](const Series& y) { return y.log1p(); },
      .inverse = [](const Series& y) { return y.expm1(); },
  };
}

[[nodiscard]] inline TargetTransform sqrt() {
  return TargetTransform{
      .name = "sqrt",
      .forward = [](const Series& y) { return y.clamp_min(0.0).sqrt(); },
      .inverse = [](const Series& y) { return y.square(); },
  };
}

// Escape hatch for a hand-written forward/inverse pair -- same shape
// prep::FunctionTransformer already gives the feature side.
[[nodiscard]] inline TargetTransform custom(std::string name,
                                            std::function<Series(const Series&)> forward,
                                            std::function<Series(const Series&)> inverse,
                                            bool check_inverse = true,
                                            double check_inverse_tolerance = 1e-3) {
  return TargetTransform{
      .name = std::move(name),
      .forward = std::move(forward),
      .inverse = std::move(inverse),
      .check_inverse = check_inverse,
      .check_inverse_tolerance = check_inverse_tolerance,
  };
}

} // namespace transform

} // namespace pluss::ai::lymph

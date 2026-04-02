#pragma once

#include "pluss/ai/nodej/options.h"
#include "pluss/ai/nodej/status.h"

namespace pluss::ai::nodej {

struct FeatureBatch {
  Series x = Series::undefined();
  ObservationLayout layout = ObservationLayout::Rows;

  [[nodiscard]] bool defined() const noexcept { return x.defined(); }
  [[nodiscard]] std::int64_t feature_count() const noexcept {
    if (!x.defined() || x.dim() == 0) return 0;
    return layout == ObservationLayout::Rows ? x.sizes().back() : x.sizes().front();
  }
  [[nodiscard]] std::int64_t sample_count() const noexcept {
    if (!x.defined() || x.dim() == 0) return 0;
    return layout == ObservationLayout::Rows ? x.sizes().front() : x.sizes().back();
  }
};

struct LabeledBatch {
  FeatureBatch features{};
  Series y = Series::undefined();
  Series sample_weights = Series::undefined();

  [[nodiscard]] bool has_labels() const noexcept { return y.defined(); }
  [[nodiscard]] bool has_weights() const noexcept { return sample_weights.defined(); }
};

inline Status validate_features(const FeatureBatch& batch) {
  if (!batch.defined()) return Status::invalid_argument("FeatureBatch.x must be a defined Series.");
  if (batch.x.dim() < 2) return Status::shape_mismatch("FeatureBatch.x must be rank >= 2.");
  if (batch.sample_count() <= 0) return Status::shape_mismatch("FeatureBatch.x must contain at least one sample.");
  if (batch.feature_count() <= 0) return Status::shape_mismatch("FeatureBatch.x must contain at least one feature.");
  return Status::success();
}

inline Status validate_labeled(const LabeledBatch& batch) {
  Status s = validate_features(batch.features);
  if (!s.ok()) return s;
  if (!batch.has_labels()) return Status::invalid_argument("LabeledBatch.y must be a defined Series.");
  if (batch.y.numel() != batch.features.sample_count()) {
    return Status::shape_mismatch("LabeledBatch.y length must match sample count.");
  }
  if (batch.has_weights() && batch.sample_weights.numel() != batch.features.sample_count()) {
    return Status::shape_mismatch("LabeledBatch.sample_weights length must match sample count.");
  }
  return Status::success();
}

} // namespace pluss::ai::nodej

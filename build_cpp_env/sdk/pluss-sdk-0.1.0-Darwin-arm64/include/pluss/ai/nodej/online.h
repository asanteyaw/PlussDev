#pragma once

#include "pluss/ai/nodej/dataset.h"
#include "pluss/ai/nodej/families.h"
#include "pluss/ai/nodej/options.h"
#include "pluss/ai/nodej/status.h"

#include <cstdint>
#include <vector>

namespace pluss::ai::nodej::online {

struct OnlineLinearOptions {
  LinearFamilyOptions family{};
  double learning_rate = 5e-2;
  bool decaying_rate = true;
};

class OnlineLinearClassifier {
public:
  explicit OnlineLinearClassifier(const OnlineLinearOptions& options = {});

  [[nodiscard]] bool trained() const noexcept;
  [[nodiscard]] std::int64_t seen_samples() const noexcept;

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {});
  Status advance(const LabeledBatch& batch, const TrainOptions& options = {});

  [[nodiscard]] StatusOr<Series> infer_scores(const FeatureBatch& batch, const InferOptions& options = {}) const;
  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch, const InferOptions& options = {}) const;
  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch, const AssessOptions& options = {}) const;

private:
  OnlineLinearOptions options_{};
  bool trained_{false};
  std::int64_t seen_samples_{0};
  std::int64_t feature_count_{0};
  std::int64_t updates_{0};
  Series weights_ = Series::undefined();
  double negative_label_{0.0};
  double positive_label_{1.0};
  ScalarType label_dtype_{ScalarType::Float64};
};

class OnlineLinearRegressor {
public:
  explicit OnlineLinearRegressor(const OnlineLinearOptions& options = {});

  [[nodiscard]] bool trained() const noexcept;
  [[nodiscard]] std::int64_t seen_samples() const noexcept;

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {});
  Status advance(const LabeledBatch& batch, const TrainOptions& options = {});

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch, const InferOptions& options = {}) const;
  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch, const AssessOptions& options = {}) const;

private:
  OnlineLinearOptions options_{};
  bool trained_{false};
  std::int64_t seen_samples_{0};
  std::int64_t feature_count_{0};
  std::int64_t updates_{0};
  Series weights_ = Series::undefined();
};

StatusOr<OnlineLinearClassifier> make_online_linear_classifier(const OnlineLinearOptions& options = {});
StatusOr<OnlineLinearRegressor> make_online_linear_regressor(const OnlineLinearOptions& options = {});

} // namespace pluss::ai::nodej::online

#pragma once

#include "pluss/ai/nodej/dataset.h"

#include <cstdint>
#include <initializer_list>

namespace pluss::ai::nodej {

struct VariancePruneResult {
  FeatureBatch reduced{};
  Series selected_feature_indices = Series::undefined();
  Series feature_variances = Series::undefined();
};

enum class FeatureScoreMode : std::uint8_t {
  ClassF = 0,
  RegressionF = 1,
};

struct ScoreRankOptions {
  std::int64_t top_k = 10;
  bool absolute_score = true;
};

struct FeatureRankOptions {
  FeatureScoreMode mode = FeatureScoreMode::ClassF;
  bool absolute_score = true;
};

struct FeaturePickOptions {
  FeatureScoreMode mode = FeatureScoreMode::ClassF;
  std::int64_t top_k = 10;
  // If > 0, overrides top_k. Values in (0, 1] mean fraction of features;
  // values > 1 mean percent, so 25 means 25%.
  double fraction = 0.0;
  bool absolute_score = true;
};

struct FeatureRankReport {
  Series scores = Series::undefined();
  Series ranked_feature_indices = Series::undefined();
};

struct FeaturePickResult {
  FeatureBatch reduced{};
  Series selected_feature_indices = Series::undefined();
  Series scores = Series::undefined();
  Series ranked_feature_indices = Series::undefined();
};

StatusOr<FeatureBatch> take_features(const FeatureBatch& batch,
                                     pluss::msharp::RangeView::Int feature_indices);
inline StatusOr<FeatureBatch> take_features(const FeatureBatch& batch,
                                            std::initializer_list<std::int64_t> feature_indices) {
  return take_features(batch, pluss::msharp::RangeView::Int(feature_indices));
}

StatusOr<VariancePruneResult> variance_prune(const FeatureBatch& batch, double variance_floor = 1e-12);
StatusOr<FeatureRankReport> rank_features(const LabeledBatch& batch, const FeatureRankOptions& options = {});
StatusOr<FeaturePickResult> pick_features(const LabeledBatch& batch, const FeaturePickOptions& options = {});
// ANOVA F-score ranking for classification (binary or multiclass).
StatusOr<Series> f_score_rank(const LabeledBatch& batch, const ScoreRankOptions& options = {});
// Correlation F-score ranking for regression targets.
StatusOr<Series> f_regression_rank(const LabeledBatch& batch, const ScoreRankOptions& options = {});

} // namespace pluss::ai::nodej

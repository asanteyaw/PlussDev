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

struct ScoreRankOptions {
  std::int64_t top_k = 10;
  bool absolute_score = true;
};

StatusOr<FeatureBatch> take_features(const FeatureBatch& batch,
                                     pluss::msharp::RangeView::Int feature_indices);
inline StatusOr<FeatureBatch> take_features(const FeatureBatch& batch,
                                            std::initializer_list<std::int64_t> feature_indices) {
  return take_features(batch, pluss::msharp::RangeView::Int(feature_indices));
}

StatusOr<VariancePruneResult> variance_prune(const FeatureBatch& batch, double variance_floor = 1e-12);
// ANOVA F-score ranking for classification (binary or multiclass).
StatusOr<Series> f_score_rank(const LabeledBatch& batch, const ScoreRankOptions& options = {});
// Correlation F-score ranking for regression targets.
StatusOr<Series> f_regression_rank(const LabeledBatch& batch, const ScoreRankOptions& options = {});

} // namespace pluss::ai::nodej

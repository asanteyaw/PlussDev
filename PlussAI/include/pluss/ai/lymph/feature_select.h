#pragma once

#include "pluss/ai/lymph/common.h"

namespace pluss::ai::lymph {

namespace feature {

using VariancePrune = ::pluss::ai::nodej::VariancePruneResult;
using ScoreRankOptions = ::pluss::ai::nodej::ScoreRankOptions;
using FeatureScoreMode = ::pluss::ai::nodej::FeatureScoreMode;
using FeatureRankOptions = ::pluss::ai::nodej::FeatureRankOptions;
using FeaturePickOptions = ::pluss::ai::nodej::FeaturePickOptions;
using FeatureRankReport = ::pluss::ai::nodej::FeatureRankReport;
using Result = ::pluss::ai::nodej::FeaturePickResult;

[[nodiscard]] inline FeatureBatch take_features(const FeatureBatch& batch,
                                                pluss::msharp::RangeView::Int feature_indices) {
  return resolve(::pluss::ai::nodej::take_features(batch, feature_indices), "feature::take_features");
}

[[nodiscard]] inline FeatureBatch take_features(const FeatureBatch& batch,
                                                std::initializer_list<std::int64_t> feature_indices) {
  return resolve(::pluss::ai::nodej::take_features(batch, pluss::msharp::RangeView::Int(feature_indices)),
                 "feature::take_features");
}

[[nodiscard]] inline VariancePrune variance_prune(const FeatureBatch& batch, double variance_floor = 1e-12) {
  return resolve(::pluss::ai::nodej::variance_prune(batch, variance_floor), "feature::variance_prune");
}

[[nodiscard]] inline FeatureRankReport rank_features(const LabeledBatch& batch, const FeatureRankOptions& options = {}) {
  return resolve(::pluss::ai::nodej::rank_features(batch, options), "feature::rank_features");
}

[[nodiscard]] inline Result pick_features(const LabeledBatch& batch, const FeaturePickOptions& options = {}) {
  return resolve(::pluss::ai::nodej::pick_features(batch, options), "feature::pick_features");
}

[[nodiscard]] inline Series f_score_rank(const LabeledBatch& batch, const ScoreRankOptions& options = {}) {
  return resolve(::pluss::ai::nodej::f_score_rank(batch, options), "feature::f_score_rank");
}

[[nodiscard]] inline Series f_regression_rank(const LabeledBatch& batch, const ScoreRankOptions& options = {}) {
  return resolve(::pluss::ai::nodej::f_regression_rank(batch, options), "feature::f_regression_rank");
}

} // namespace feature

} // namespace pluss::ai::lymph

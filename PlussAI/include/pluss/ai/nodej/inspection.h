#pragma once

#include "pluss/ai/nodej/builders.h"
#include "pluss/ai/nodej/metrics.h"

#include <cstdint>

namespace pluss::ai::nodej {

struct PermuteImpactReport {
  Series mean_drop = pluss::series::undefined();
  Series std_drop = pluss::series::undefined();
  Series per_repeat_drops = pluss::series::undefined();
  MetricKind metric = MetricKind::Accuracy;
};

struct DependenceCurve {
  std::int64_t feature_index = -1;
  Series grid = pluss::series::undefined();
  Series mean_prediction = pluss::series::undefined();
};

struct DependenceSurface {
  std::int64_t feature_a = -1;
  std::int64_t feature_b = -1;
  Series grid_a = pluss::series::undefined();
  Series grid_b = pluss::series::undefined();
  // shape: [grid_a_points, grid_b_points]
  Series mean_prediction = pluss::series::undefined();
};

StatusOr<PermuteImpactReport> permutation_impact(const ModelSpec& spec,
                                                 const LabeledBatch& train,
                                                 const LabeledBatch& eval,
                                                 MetricKind metric = MetricKind::Accuracy,
                                                 std::int64_t repeats = 5,
                                                 RandomState random = {});

StatusOr<DependenceCurve> dependence_curve(const ModelSpec& spec,
                                           const LabeledBatch& train,
                                           const FeatureBatch& eval,
                                           std::int64_t feature_index,
                                           std::int64_t grid_points = 32);

StatusOr<DependenceSurface> dependence_surface(const ModelSpec& spec,
                                               const LabeledBatch& train,
                                               const FeatureBatch& eval,
                                               std::int64_t feature_a,
                                               std::int64_t feature_b,
                                               std::int64_t grid_points_a = 16,
                                               std::int64_t grid_points_b = 16);

} // namespace pluss::ai::nodej

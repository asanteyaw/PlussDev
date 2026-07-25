#pragma once

#include "pluss/ai/lymph/common.h"

namespace pluss::ai::lymph {

namespace inspection {

[[nodiscard]] inline PermuteImpactReport permutation_impact(const ModelSpec& spec,
                                                            const LabeledBatch& train,
                                                            const LabeledBatch& eval,
                                                            MetricKind metric = MetricKind::Accuracy,
                                                            std::int64_t repeats = 5,
                                                            RandomState random = {}) {
  return resolve(::pluss::ai::nodej::permutation_impact(spec, train, eval, metric, repeats, random),
                 "inspection::permutation_impact");
}

[[nodiscard]] inline DependenceCurve dependence_curve(const ModelSpec& spec,
                                                      const LabeledBatch& train,
                                                      const FeatureBatch& eval,
                                                      std::int64_t feature_index,
                                                      std::int64_t grid_points = 32) {
  return resolve(::pluss::ai::nodej::dependence_curve(spec, train, eval, feature_index, grid_points),
                 "inspection::dependence_curve");
}

[[nodiscard]] inline DependenceSurface dependence_surface(const ModelSpec& spec,
                                                          const LabeledBatch& train,
                                                          const FeatureBatch& eval,
                                                          std::int64_t feature_a,
                                                          std::int64_t feature_b,
                                                          std::int64_t grid_points_a = 16,
                                                          std::int64_t grid_points_b = 16) {
  return resolve(::pluss::ai::nodej::dependence_surface(spec,
                                                        train,
                                                        eval,
                                                        feature_a,
                                                        feature_b,
                                                        grid_points_a,
                                                        grid_points_b),
                 "inspection::dependence_surface");
}

} // namespace inspection

} // namespace pluss::ai::lymph

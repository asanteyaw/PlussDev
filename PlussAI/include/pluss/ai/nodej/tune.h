#pragma once

#include "pluss/ai/nodej/metrics.h"

#include <cstdint>
#include <functional>
#include <limits>
#include <vector>

namespace pluss::ai::nodej {

enum class TuneObjectivePolicy : std::uint8_t {
  Auto = 0,
  Minimize = 1,
  Maximize = 2,
};

struct TuneTrialResult {
  ModelSpec spec{};
  Status status = Status::success();
  CrossAssessReport assess{};
  double objective = std::numeric_limits<double>::quiet_NaN();
};

struct TuneReport {
  std::vector<TuneTrialResult> trials{};
  std::int64_t best_index = -1;
  MetricKind objective_metric = MetricKind::Accuracy;
  TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto;
};

using ProbeSpecSampler = std::function<StatusOr<ModelSpec>(std::int64_t trial_index, std::uint64_t seed)>;

StatusOr<TuneReport> sweep(const std::vector<ModelSpec>& candidates,
                           const LabeledBatch& batch,
                           const SplitOptions& split = {},
                           const TrainOptions& train_options = {},
                           const AssessOptions& assess_options = {},
                           MetricKind objective_metric = MetricKind::Accuracy,
                           TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                           std::int64_t max_parallel = 0);

StatusOr<TuneReport> probe(const ProbeSpecSampler& sampler,
                           std::int64_t trials,
                           const LabeledBatch& batch,
                           const SplitOptions& split = {},
                           const TrainOptions& train_options = {},
                           const AssessOptions& assess_options = {},
                           MetricKind objective_metric = MetricKind::Accuracy,
                           TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                           RandomState random = {},
                           std::int64_t max_parallel = 0);

} // namespace pluss::ai::nodej

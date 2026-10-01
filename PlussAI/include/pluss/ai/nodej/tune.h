#pragma once

#include "pluss/ai/nodej/metrics.h"

#include <cstdint>
#include <functional>
#include <limits>
#include <string>
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
  std::int64_t stage = 0;
};

struct TuneLedger {
  std::vector<MetricKind> metrics{};
  std::vector<std::string> encoded_param_feature_names{};
  Series objective = pluss::series::undefined();
  Series objective_rank = pluss::series::undefined();
  Series stage = pluss::series::undefined();
  Series status_code = pluss::series::undefined();
  Series is_success = pluss::series::undefined();
  Series mean_test_metrics = pluss::series::undefined();
  Series std_test_metrics = pluss::series::undefined();
  Series split_test_metrics = pluss::series::undefined();
  Series fit_time_seconds = pluss::series::undefined();
  Series score_time_seconds = pluss::series::undefined();
  Series encoded_params = pluss::series::undefined();
};

struct TuneReport {
  std::vector<TuneTrialResult> trials{};
  std::int64_t best_index = -1;
  MetricKind objective_metric = MetricKind::Accuracy;
  TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto;
  // 1-based rank where 1 is best, inspired by sklearn rank_test_score.
  // Trials with failed/non-finite objective receive the worst rank bucket.
  std::vector<std::int64_t> objective_rank{};
  TuneLedger ledger{};
};

using ProbeSpecSampler = std::function<StatusOr<ModelSpec>(std::int64_t trial_index, std::uint64_t seed)>;

StatusOr<TuneReport> sweep(pluss::msharp::RangeView::Of<ModelSpec> candidates,
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

StatusOr<TuneReport> tune(pluss::msharp::RangeView::Of<ModelSpec> candidates,
                          const LabeledBatch& batch,
                          const TuneOptions& tune_options = {},
                          const TrainOptions& train_options = {},
                          const AssessOptions& assess_options = {},
                          MetricKind objective_metric = MetricKind::Accuracy,
                          TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto);

StatusOr<TuneReport> tune(const ProbeSpecSampler& sampler,
                          const LabeledBatch& batch,
                          const TuneOptions& tune_options = {},
                          const TrainOptions& train_options = {},
                          const AssessOptions& assess_options = {},
                          MetricKind objective_metric = MetricKind::Accuracy,
                          TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto);

} // namespace pluss::ai::nodej

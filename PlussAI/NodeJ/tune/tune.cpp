#include "pluss/ai/nodej/tune.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <future>
#include <limits>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace pluss::ai::nodej {
namespace {

[[nodiscard]] bool metric_prefers_max(MetricKind metric) {
  switch (metric) {
    case MetricKind::Accuracy:
    case MetricKind::BalancedAccuracy:
    case MetricKind::Precision:
    case MetricKind::Recall:
    case MetricKind::F1:
    case MetricKind::RocAuc:
    case MetricKind::R2:
      return true;
    case MetricKind::LogLoss:
    case MetricKind::Mse:
    case MetricKind::Rmse:
    case MetricKind::Mae:
    case MetricKind::QuantileLoss:
      return false;
  }
  return true;
}

[[nodiscard]] bool maximize_objective(TuneObjectivePolicy policy, MetricKind metric) {
  if (policy == TuneObjectivePolicy::Maximize) return true;
  if (policy == TuneObjectivePolicy::Minimize) return false;
  return metric_prefers_max(metric);
}

[[nodiscard]] std::uint64_t splitmix64(std::uint64_t x) {
  x += 0x9e3779b97f4a7c15ULL;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
  return x ^ (x >> 31);
}

[[nodiscard]] std::uint64_t compose_seed(const RandomState& random) {
  if (random.deterministic) return random.seed;

  const auto now_ticks = static_cast<std::uint64_t>(
      std::chrono::high_resolution_clock::now().time_since_epoch().count());
  std::uint64_t seed = splitmix64(now_ticks);
  if (random.seed != 0) seed ^= splitmix64(random.seed);

  std::random_device rd;
  seed ^= splitmix64((static_cast<std::uint64_t>(rd()) << 32) ^ static_cast<std::uint64_t>(rd()));
  return seed;
}

[[nodiscard]] StatusOr<std::int64_t> metric_index(const CrossAssessReport& assess, MetricKind metric) {
  for (std::size_t i = 0; i < assess.metrics.size(); ++i) {
    if (assess.metrics[i] == metric) return StatusOr<std::int64_t>(static_cast<std::int64_t>(i));
  }
  return StatusOr<std::int64_t>(Status::invalid_argument(
      "tune objective metric is not present in cross_assess metrics output."));
}

[[nodiscard]] StatusOr<double> metric_value(const CrossAssessReport& assess, MetricKind metric) {
  StatusOr<std::int64_t> index_or = metric_index(assess, metric);
  if (!index_or.ok()) return StatusOr<double>(index_or.status());
  const std::int64_t i = index_or.value();
  const Series idx = pluss::arange(i, i + 1, Config{}.with_dtype(ScalarType::Int64).with_platform(assess.mean_metrics.platform()));
  const Series objective = assess.mean_metrics.reshape({assess.mean_metrics.numel()}).index_select(0, idx);
  const double value = objective.reshape({1}).to(ScalarType::Float64).item<double>();
  if (!std::isfinite(value)) {
    return StatusOr<double>(Status::numerical_failure("tune objective value is non-finite."));
  }
  return StatusOr<double>(value);
}

[[nodiscard]] bool objective_better(double candidate, double incumbent, bool maximize) {
  if (maximize) return candidate > incumbent;
  return candidate < incumbent;
}

[[nodiscard]] std::int64_t resolved_search_parallelism(std::size_t count, std::int64_t max_parallel) {
  if (count <= 1) return 1;
  if (max_parallel > 0) {
    return std::max<std::int64_t>(
        1,
        std::min<std::int64_t>(static_cast<std::int64_t>(count), max_parallel));
  }
  const std::uint32_t hw = std::thread::hardware_concurrency();
  const std::int64_t cap = hw == 0 ? 1 : static_cast<std::int64_t>(hw);
  return std::max<std::int64_t>(1, std::min<std::int64_t>(static_cast<std::int64_t>(count), cap));
}

[[nodiscard]] std::int64_t resolved_nested_fold_parallelism(std::int64_t candidate_workers) {
  if (candidate_workers <= 1) return 0;
  const std::uint32_t hw = std::thread::hardware_concurrency();
  const std::int64_t cap = hw == 0 ? candidate_workers : static_cast<std::int64_t>(hw);
  return std::max<std::int64_t>(1, cap / std::max<std::int64_t>(1, candidate_workers));
}

[[nodiscard]] TuneTrialResult make_failed_trial(const ModelSpec& spec, const Status& status) {
  TuneTrialResult out{};
  out.spec = spec;
  out.status = status;
  out.objective = std::numeric_limits<double>::quiet_NaN();
  return out;
}

void maybe_update_best(const TuneTrialResult& trial,
                       bool maximize,
                       std::int64_t trial_index,
                       bool& have_best,
                       double& best_objective,
                       std::int64_t& best_index) {
  if (!trial.status.ok() || !std::isfinite(trial.objective)) return;
  if (!have_best || objective_better(trial.objective, best_objective, maximize)) {
    have_best = true;
    best_objective = trial.objective;
    best_index = trial_index;
  }
}

} // namespace

StatusOr<TuneReport> sweep(const std::vector<ModelSpec>& candidates,
                           const LabeledBatch& batch,
                           const SplitOptions& split,
                           const TrainOptions& train_options,
                           const AssessOptions& assess_options,
                           MetricKind objective_metric,
                           TuneObjectivePolicy objective_policy,
                           std::int64_t max_parallel) {
  if (candidates.empty()) {
    return StatusOr<TuneReport>(Status::invalid_argument("sweep requires at least one candidate ModelSpec."));
  }
  Status data_status = validate_labeled(batch);
  if (!data_status.ok()) return StatusOr<TuneReport>(std::move(data_status));

  TuneReport report{};
  report.objective_metric = objective_metric;
  report.objective_policy = objective_policy;
  report.trials.reserve(candidates.size());

  const bool maximize = maximize_objective(objective_policy, objective_metric);
  bool have_best = false;
  double best_objective = maximize ? -std::numeric_limits<double>::infinity()
                                   : std::numeric_limits<double>::infinity();
  const std::int64_t workers = resolved_search_parallelism(candidates.size(), max_parallel);
  const std::int64_t fold_parallel_budget = resolved_nested_fold_parallelism(workers);

  auto eval_trial = [&](std::size_t i) -> TuneTrialResult {
    TuneTrialResult trial{};
    trial.spec = candidates[i];

    SplitOptions trial_split = split;
    if (workers > 1 && trial_split.max_parallel <= 0) {
      // Use bounded nested fold parallelism when evaluating many candidates concurrently.
      trial_split.max_parallel = fold_parallel_budget;
    }
    StatusOr<CrossAssessReport> assess_or =
        cross_assess(candidates[i], batch, trial_split, train_options, assess_options);
    if (!assess_or.ok()) {
      return make_failed_trial(candidates[i], assess_or.status());
    }

    trial.assess = assess_or.move_value();
    StatusOr<double> objective_or = metric_value(trial.assess, objective_metric);
    if (!objective_or.ok()) {
      trial.status = objective_or.status();
      trial.objective = std::numeric_limits<double>::quiet_NaN();
      return trial;
    }

    trial.status = Status::success();
    trial.objective = objective_or.value();
    return trial;
  };

  if (workers <= 1) {
    for (std::size_t i = 0; i < candidates.size(); ++i) {
      report.trials.push_back(eval_trial(i));
      maybe_update_best(report.trials.back(),
                        maximize,
                        static_cast<std::int64_t>(i),
                        have_best,
                        best_objective,
                        report.best_index);
    }
  } else {
    for (std::size_t start = 0; start < candidates.size(); start += static_cast<std::size_t>(workers)) {
      const std::size_t end = std::min(candidates.size(), start + static_cast<std::size_t>(workers));
      std::vector<std::future<TuneTrialResult>> futures{};
      futures.reserve(end - start);
      for (std::size_t i = start; i < end; ++i) {
        futures.emplace_back(std::async(std::launch::async, [&, i]() { return eval_trial(i); }));
      }
      for (std::size_t i = start; i < end; ++i) {
        report.trials.push_back(futures[i - start].get());
        maybe_update_best(report.trials.back(),
                          maximize,
                          static_cast<std::int64_t>(i),
                          have_best,
                          best_objective,
                          report.best_index);
      }
    }
  }

  return StatusOr<TuneReport>(std::move(report));
}

StatusOr<TuneReport> probe(const ProbeSpecSampler& sampler,
                           std::int64_t trials,
                           const LabeledBatch& batch,
                           const SplitOptions& split,
                           const TrainOptions& train_options,
                           const AssessOptions& assess_options,
                           MetricKind objective_metric,
                           TuneObjectivePolicy objective_policy,
                           RandomState random,
                           std::int64_t max_parallel) {
  if (!sampler) {
    return StatusOr<TuneReport>(Status::invalid_argument("probe requires a valid sampler callback."));
  }
  if (trials <= 0) {
    return StatusOr<TuneReport>(Status::invalid_argument("probe requires trials > 0."));
  }
  Status data_status = validate_labeled(batch);
  if (!data_status.ok()) return StatusOr<TuneReport>(std::move(data_status));

  TuneReport report{};
  report.objective_metric = objective_metric;
  report.objective_policy = objective_policy;
  report.trials.reserve(static_cast<std::size_t>(trials));

  const bool maximize = maximize_objective(objective_policy, objective_metric);
  bool have_best = false;
  double best_objective = maximize ? -std::numeric_limits<double>::infinity()
                                   : std::numeric_limits<double>::infinity();
  const std::int64_t workers = resolved_search_parallelism(static_cast<std::size_t>(trials), max_parallel);
  const std::int64_t fold_parallel_budget = resolved_nested_fold_parallelism(workers);

  const std::uint64_t base_seed = compose_seed(random);
  auto eval_trial = [&](std::int64_t trial_index) -> TuneTrialResult {
    const std::uint64_t trial_seed = splitmix64(base_seed + static_cast<std::uint64_t>(trial_index));
    StatusOr<ModelSpec> spec_or = sampler(trial_index, trial_seed);
    if (!spec_or.ok()) {
      TuneTrialResult failed{};
      failed.status = spec_or.status();
      failed.objective = std::numeric_limits<double>::quiet_NaN();
      return failed;
    }

    TuneTrialResult trial{};
    trial.spec = spec_or.move_value();
    SplitOptions trial_split = split;
    if (workers > 1 && trial_split.max_parallel <= 0) {
      // Use bounded nested fold parallelism when evaluating many probe trials concurrently.
      trial_split.max_parallel = fold_parallel_budget;
    }
    StatusOr<CrossAssessReport> assess_or =
        cross_assess(trial.spec, batch, trial_split, train_options, assess_options);
    if (!assess_or.ok()) {
      trial.status = assess_or.status();
      trial.objective = std::numeric_limits<double>::quiet_NaN();
      return trial;
    }

    trial.assess = assess_or.move_value();
    StatusOr<double> objective_or = metric_value(trial.assess, objective_metric);
    if (!objective_or.ok()) {
      trial.status = objective_or.status();
      trial.objective = std::numeric_limits<double>::quiet_NaN();
      return trial;
    }

    trial.status = Status::success();
    trial.objective = objective_or.value();
    return trial;
  };

  if (workers <= 1) {
    for (std::int64_t trial_index = 0; trial_index < trials; ++trial_index) {
      report.trials.push_back(eval_trial(trial_index));
      maybe_update_best(report.trials.back(),
                        maximize,
                        static_cast<std::int64_t>(report.trials.size() - 1),
                        have_best,
                        best_objective,
                        report.best_index);
    }
  } else {
    for (std::int64_t start = 0; start < trials; start += workers) {
      const std::int64_t end = std::min(trials, start + workers);
      std::vector<std::future<TuneTrialResult>> futures{};
      futures.reserve(static_cast<std::size_t>(end - start));
      for (std::int64_t i = start; i < end; ++i) {
        futures.emplace_back(std::async(std::launch::async, [&, i]() { return eval_trial(i); }));
      }
      for (std::int64_t i = start; i < end; ++i) {
        report.trials.push_back(futures[static_cast<std::size_t>(i - start)].get());
        maybe_update_best(report.trials.back(),
                          maximize,
                          static_cast<std::int64_t>(report.trials.size() - 1),
                          have_best,
                          best_objective,
                          report.best_index);
      }
    }
  }

  return StatusOr<TuneReport>(std::move(report));
}

} // namespace pluss::ai::nodej

#include "pluss/ai/nodej/tune.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <future>
#include <functional>
#include <limits>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

namespace pluss::ai::nodej {
namespace {

[[nodiscard]] Config f64_cpu() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

[[nodiscard]] Config i64_cpu() {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
}

void populate_tune_ledger(TuneReport& report);

// metric_prefers_max lives in nodej/options.h now (shared with
// sparse_runtime.cpp and inspection.cpp instead of a fourth private copy).

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
  const Series idx = pluss::series::arange(i, i + 1, Config{}.with_dtype(ScalarType::Int64).with_platform(assess.mean_metrics.platform()));
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

[[nodiscard]] bool trial_has_objective(const TuneTrialResult& trial) {
  return trial.status.ok() && std::isfinite(trial.objective);
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

void assign_objective_ranks(TuneReport& report) {
  const bool maximize = maximize_objective(report.objective_policy, report.objective_metric);
  report.objective_rank.assign(report.trials.size(), 0);
  if (report.trials.empty()) return;

  std::vector<std::int64_t> valid_indices{};
  valid_indices.reserve(report.trials.size());
  for (std::size_t i = 0; i < report.trials.size(); ++i) {
    if (trial_has_objective(report.trials[i])) valid_indices.push_back(static_cast<std::int64_t>(i));
  }
  if (valid_indices.empty()) {
    std::fill(report.objective_rank.begin(), report.objective_rank.end(), std::int64_t{1});
    return;
  }

  std::stable_sort(valid_indices.begin(),
                   valid_indices.end(),
                   [&](std::int64_t li, std::int64_t ri) {
                     const double lhs = report.trials[static_cast<std::size_t>(li)].objective;
                     const double rhs = report.trials[static_cast<std::size_t>(ri)].objective;
                     if (lhs == rhs) return false;
                     return maximize ? (lhs > rhs) : (lhs < rhs);
                   });

  std::int64_t current_rank = 1;
  double prev_value = report.trials[static_cast<std::size_t>(valid_indices[0])].objective;
  report.objective_rank[static_cast<std::size_t>(valid_indices[0])] = current_rank;
  for (std::size_t pos = 1; pos < valid_indices.size(); ++pos) {
    const double value = report.trials[static_cast<std::size_t>(valid_indices[pos])].objective;
    if (value != prev_value) {
      current_rank = static_cast<std::int64_t>(pos) + 1;
      prev_value = value;
    }
    report.objective_rank[static_cast<std::size_t>(valid_indices[pos])] = current_rank;
  }

  const std::int64_t worst_rank = static_cast<std::int64_t>(valid_indices.size()) + 1;
  for (std::size_t i = 0; i < report.trials.size(); ++i) {
    if (!trial_has_objective(report.trials[i])) {
      report.objective_rank[i] = worst_rank;
    }
  }
}

[[nodiscard]] bool trial_sort_better(const TuneTrialResult& lhs,
                                     const TuneTrialResult& rhs,
                                     bool maximize) {
  const bool lhs_ok = lhs.status.ok() && std::isfinite(lhs.objective);
  const bool rhs_ok = rhs.status.ok() && std::isfinite(rhs.objective);
  if (lhs_ok != rhs_ok) return lhs_ok;
  if (!lhs_ok) return false;
  if (lhs.objective == rhs.objective) return false;
  return maximize ? (lhs.objective > rhs.objective) : (lhs.objective < rhs.objective);
}

[[nodiscard]] std::vector<std::int64_t> top_trial_indices(const std::vector<TuneTrialResult>& trials,
                                                          std::int64_t keep_count,
                                                          bool maximize) {
  std::vector<std::int64_t> order(trials.size(), 0);
  for (std::size_t i = 0; i < trials.size(); ++i) {
    order[i] = static_cast<std::int64_t>(i);
  }
  std::stable_sort(order.begin(),
                   order.end(),
                   [&](std::int64_t li, std::int64_t ri) {
                     return trial_sort_better(trials[static_cast<std::size_t>(li)],
                                              trials[static_cast<std::size_t>(ri)],
                                              maximize);
                   });
  if (keep_count <= 0) return {};
  if (keep_count >= static_cast<std::int64_t>(order.size())) return order;
  order.resize(static_cast<std::size_t>(keep_count));
  return order;
}

[[nodiscard]] StatusOr<TuneReport> successive_halving_from_candidates(
    pluss::msharp::RangeView::Of<ModelSpec> candidates,
    const LabeledBatch& batch,
    const SplitOptions& split,
    const TrainOptions& train_options,
    const AssessOptions& assess_options,
    MetricKind objective_metric,
    TuneObjectivePolicy objective_policy,
    std::int64_t max_parallel) {
  if (candidates.empty()) {
    return StatusOr<TuneReport>(Status::invalid_argument(
        "successive_halving requires at least one candidate ModelSpec."));
  }

  const bool maximize = maximize_objective(objective_policy, objective_metric);
  TuneReport aggregate{};
  aggregate.objective_metric = objective_metric;
  aggregate.objective_policy = objective_policy;

  std::vector<ModelSpec> round_candidates(candidates.begin(), candidates.end());
  const double factor = 2.0;
  bool have_best = false;
  double best_objective = maximize ? -std::numeric_limits<double>::infinity()
                                   : std::numeric_limits<double>::infinity();
  std::int64_t stage = 0;

  while (!round_candidates.empty()) {
    StatusOr<TuneReport> round_or = sweep(round_candidates,
                                          batch,
                                          split,
                                          train_options,
                                          assess_options,
                                          objective_metric,
                                          objective_policy,
                                          max_parallel);
    if (!round_or.ok()) return StatusOr<TuneReport>(round_or.status());
    TuneReport round = round_or.move_value();

    for (auto& trial : round.trials) {
      trial.stage = stage;
      aggregate.trials.push_back(std::move(trial));
      maybe_update_best(aggregate.trials.back(),
                        maximize,
                        static_cast<std::int64_t>(aggregate.trials.size() - 1),
                        have_best,
                        best_objective,
                        aggregate.best_index);
    }

    if (round_candidates.size() <= 1) break;
    const std::int64_t keep_count = std::max<std::int64_t>(
        1,
        static_cast<std::int64_t>(
            std::ceil(static_cast<double>(round_candidates.size()) / factor)));
    const std::vector<std::int64_t> keep_indices = top_trial_indices(round.trials, keep_count, maximize);
    if (keep_indices.empty()) break;

    std::vector<ModelSpec> next_round{};
    next_round.reserve(keep_indices.size());
    for (std::int64_t idx : keep_indices) {
      next_round.push_back(round_candidates[static_cast<std::size_t>(idx)]);
    }
    if (next_round.size() == round_candidates.size()) {
      next_round.resize(next_round.size() - 1);
    }
    round_candidates = std::move(next_round);
    ++stage;
  }

  assign_objective_ranks(aggregate);
  populate_tune_ledger(aggregate);
  return StatusOr<TuneReport>(std::move(aggregate));
}

[[nodiscard]] std::vector<ModelSpec> sample_candidates_without_replacement(
    pluss::msharp::RangeView::Of<ModelSpec> candidates,
    std::int64_t take,
    RandomState random) {
  std::vector<std::int64_t> indices(candidates.size(), 0);
  for (std::size_t i = 0; i < candidates.size(); ++i) {
    indices[i] = static_cast<std::int64_t>(i);
  }
  std::mt19937_64 rng(compose_seed(random));
  std::shuffle(indices.begin(), indices.end(), rng);
  const std::int64_t count = std::max<std::int64_t>(
      0,
      std::min<std::int64_t>(take, static_cast<std::int64_t>(indices.size())));
  std::vector<ModelSpec> out{};
  out.reserve(static_cast<std::size_t>(count));
  for (std::int64_t i = 0; i < count; ++i) {
    out.push_back(candidates[static_cast<std::size_t>(indices[static_cast<std::size_t>(i)])]);
  }
  return out;
}

[[nodiscard]] double signed_log1p(double value) {
  return std::copysign(std::log1p(std::abs(value)), value);
}

[[nodiscard]] std::vector<double> encode_model_spec(const ModelSpec& spec) {
  std::vector<double> x(40, 0.0);
  x[0] = static_cast<double>(spec.task);
  x[1] = static_cast<double>(spec.family);

  std::visit(
      [&](const auto& opts) {
        using OptT = std::decay_t<decltype(opts)>;
        if constexpr (std::is_same_v<OptT, LinearFamilyOptions>) {
          x[2] = static_cast<double>(opts.objective);
          x[3] = static_cast<double>(opts.regularization);
          x[4] = signed_log1p(opts.regularization_strength);
          x[5] = opts.fit_intercept ? 1.0 : 0.0;
          x[32] = opts.elastic_l1_ratio;
          x[33] = signed_log1p(opts.robust_transition);
        } else if constexpr (std::is_same_v<OptT, TreeFamilyOptions>) {
          x[6] = static_cast<double>(opts.criterion);
          x[7] = static_cast<double>(opts.max_depth);
          x[8] = signed_log1p(static_cast<double>(opts.min_samples_leaf));
          x[9] = signed_log1p(static_cast<double>(opts.min_samples_split));
        } else if constexpr (std::is_same_v<OptT, EnsembleFamilyOptions>) {
          x[10] = static_cast<double>(opts.method);
          x[11] = signed_log1p(static_cast<double>(opts.learners));
          x[12] = opts.subsample;
        } else if constexpr (std::is_same_v<OptT, NeighborFamilyOptions>) {
          x[13] = signed_log1p(static_cast<double>(opts.k));
          x[14] = opts.distance_weighted ? 1.0 : 0.0;
          x[15] = opts.standardize ? 1.0 : 0.0;
          x[34] = static_cast<double>(opts.query_mode);
          x[35] = signed_log1p(opts.radius);
          x[36] = signed_log1p(static_cast<double>(opts.radius_fallback_k));
        } else if constexpr (std::is_same_v<OptT, KernelFamilyOptions>) {
          x[16] = static_cast<double>(opts.kernel);
          x[17] = signed_log1p(opts.c);
          x[18] = signed_log1p(opts.gamma);
          x[19] = signed_log1p(opts.epsilon);
          x[37] = static_cast<double>(opts.form);
          x[38] = opts.nu;
        } else if constexpr (std::is_same_v<OptT, BayesFamilyOptions>) {
          x[20] = opts.use_gaussian ? 1.0 : 0.0;
          x[21] = signed_log1p(opts.smoothing);
          x[39] = static_cast<double>(opts.likelihood);
        } else if constexpr (std::is_same_v<OptT, ClusterFamilyOptions>) {
          x[22] = static_cast<double>(opts.method);
          x[23] = signed_log1p(static_cast<double>(opts.clusters));
          x[24] = signed_log1p(static_cast<double>(opts.max_iterations));
          x[25] = signed_log1p(static_cast<double>(opts.restarts));
          x[26] = signed_log1p(opts.tolerance);
        } else if constexpr (std::is_same_v<OptT, DecomposeFamilyOptions>) {
          x[27] = static_cast<double>(opts.method);
          x[28] = signed_log1p(static_cast<double>(opts.components));
          x[29] = opts.whiten ? 1.0 : 0.0;
          x[30] = signed_log1p(static_cast<double>(opts.max_iterations));
          x[31] = signed_log1p(opts.tolerance);
        }
      },
      spec.options);
  return x;
}

[[nodiscard]] std::vector<std::string> encoded_features() {
  return {
      "task", "family",
      "linear.objective", "linear.regularization", "linear.regularization_strength_slog", "linear.fit_intercept",
      "tree.criterion", "tree.max_depth", "tree.min_samples_leaf_slog", "tree.min_samples_split_slog",
      "ensemble.method", "ensemble.learners_slog", "ensemble.subsample",
      "neighbor.k_slog", "neighbor.distance_weighted", "neighbor.standardize",
      "kernel.kind", "kernel.c_slog", "kernel.gamma_slog", "kernel.epsilon_slog",
      "bayes.use_gaussian", "bayes.smoothing_slog",
      "cluster.method", "cluster.clusters_slog", "cluster.max_iterations_slog", "cluster.restarts_slog", "cluster.tolerance_slog",
      "decompose.method", "decompose.components_slog", "decompose.whiten", "decompose.max_iterations_slog", "decompose.tolerance_slog",
      "linear.elastic_l1_ratio", "linear.robust_transition_slog",
      "neighbor.query_mode", "neighbor.radius_slog", "neighbor.radius_fallback_k_slog",
      "kernel.form", "kernel.nu", "bayes.likelihood"};
}

void populate_tune_ledger(TuneReport& report) {
  const std::size_t trial_count = report.trials.size();
  report.ledger = TuneLedger{};
  if (trial_count == 0) return;

  std::size_t metric_count = 0;
  std::size_t max_folds = 0;
  for (const TuneTrialResult& trial : report.trials) {
    if (!trial.status.ok()) continue;
    if (metric_count == 0 && !trial.assess.metrics.empty()) {
      metric_count = trial.assess.metrics.size();
      report.ledger.metrics = trial.assess.metrics;
    }
    max_folds = std::max<std::size_t>(max_folds, trial.assess.folds.size());
  }
  report.ledger.encoded_param_feature_names = encoded_features();

  std::vector<double> objective(trial_count, std::numeric_limits<double>::quiet_NaN());
  std::vector<std::int64_t> objective_rank(trial_count, 0);
  std::vector<std::int64_t> stage(trial_count, 0);
  std::vector<std::int64_t> status_code(trial_count, 0);
  std::vector<double> is_success(trial_count, 0.0);
  std::vector<double> mean_test(metric_count == 0 ? 0 : trial_count * metric_count, std::numeric_limits<double>::quiet_NaN());
  std::vector<double> std_test(metric_count == 0 ? 0 : trial_count * metric_count, std::numeric_limits<double>::quiet_NaN());
  std::vector<double> split_test(metric_count == 0 || max_folds == 0 ? 0 : trial_count * max_folds * metric_count,
                                 std::numeric_limits<double>::quiet_NaN());
  std::vector<double> fit_time(max_folds == 0 ? 0 : trial_count * max_folds, std::numeric_limits<double>::quiet_NaN());
  std::vector<double> score_time(max_folds == 0 ? 0 : trial_count * max_folds, std::numeric_limits<double>::quiet_NaN());
  std::vector<double> encoded(trial_count * 40, 0.0);

  for (std::size_t i = 0; i < trial_count; ++i) {
    const TuneTrialResult& trial = report.trials[i];
    objective[i] = trial.objective;
    if (i < report.objective_rank.size()) objective_rank[i] = report.objective_rank[i];
    stage[i] = trial.stage;
    status_code[i] = static_cast<std::int64_t>(trial.status.code);
    is_success[i] = trial.status.ok() ? 1.0 : 0.0;

    const std::vector<double> emb = encode_model_spec(trial.spec);
    for (std::size_t j = 0; j < emb.size(); ++j) {
      encoded[i * emb.size() + j] = emb[j];
    }

    if (!trial.status.ok() || metric_count == 0) continue;
    const std::vector<double> mean_vec = trial.assess.mean_metrics.to(pluss::pCPU, ScalarType::Float64)
                                             .reshape({static_cast<std::int64_t>(metric_count)})
                                             .to_vec<double>();
    const std::vector<double> std_vec = trial.assess.std_metrics.to(pluss::pCPU, ScalarType::Float64)
                                            .reshape({static_cast<std::int64_t>(metric_count)})
                                            .to_vec<double>();
    for (std::size_t m = 0; m < metric_count; ++m) {
      mean_test[i * metric_count + m] = mean_vec[m];
      std_test[i * metric_count + m] = std_vec[m];
    }
    for (std::size_t f = 0; f < trial.assess.folds.size(); ++f) {
      const std::vector<double> fold_vec = trial.assess.folds[f].metrics.to(pluss::pCPU, ScalarType::Float64)
                                               .reshape({static_cast<std::int64_t>(metric_count)})
                                               .to_vec<double>();
      for (std::size_t m = 0; m < metric_count; ++m) {
        split_test[i * (max_folds * metric_count) + f * metric_count + m] = fold_vec[m];
      }
    }
  }

  report.ledger.objective = pluss::make_series(std::move(objective), {static_cast<std::int64_t>(trial_count)}, f64_cpu());
  report.ledger.objective_rank = pluss::make_series(std::move(objective_rank), {static_cast<std::int64_t>(trial_count)}, i64_cpu());
  report.ledger.stage = pluss::make_series(std::move(stage), {static_cast<std::int64_t>(trial_count)}, i64_cpu());
  report.ledger.status_code = pluss::make_series(std::move(status_code), {static_cast<std::int64_t>(trial_count)}, i64_cpu());
  report.ledger.is_success = pluss::make_series(std::move(is_success), {static_cast<std::int64_t>(trial_count)}, f64_cpu());
  report.ledger.encoded_params =
      pluss::make_series(std::move(encoded), {static_cast<std::int64_t>(trial_count), 40}, f64_cpu());
  if (metric_count > 0) {
    report.ledger.mean_test_metrics =
        pluss::make_series(std::move(mean_test), {static_cast<std::int64_t>(trial_count), static_cast<std::int64_t>(metric_count)}, f64_cpu());
    report.ledger.std_test_metrics =
        pluss::make_series(std::move(std_test), {static_cast<std::int64_t>(trial_count), static_cast<std::int64_t>(metric_count)}, f64_cpu());
  }
  if (metric_count > 0 && max_folds > 0) {
    report.ledger.split_test_metrics = pluss::make_series(std::move(split_test),
                                                          {static_cast<std::int64_t>(trial_count),
                                                           static_cast<std::int64_t>(max_folds),
                                                           static_cast<std::int64_t>(metric_count)},
                                                          f64_cpu());
    report.ledger.fit_time_seconds = pluss::make_series(std::move(fit_time),
                                                        {static_cast<std::int64_t>(trial_count),
                                                         static_cast<std::int64_t>(max_folds)},
                                                        f64_cpu());
    report.ledger.score_time_seconds = pluss::make_series(std::move(score_time),
                                                          {static_cast<std::int64_t>(trial_count),
                                                           static_cast<std::int64_t>(max_folds)},
                                                          f64_cpu());
  }
}

void normalize_feature_matrix(std::vector<std::vector<double>>& features) {
  if (features.empty()) return;
  const std::size_t dims = features.front().size();
  std::vector<double> mean(dims, 0.0);
  std::vector<double> stdev(dims, 0.0);
  const double n = static_cast<double>(features.size());
  for (const auto& row : features) {
    for (std::size_t d = 0; d < dims; ++d) mean[d] += row[d];
  }
  for (double& v : mean) v /= n;
  for (const auto& row : features) {
    for (std::size_t d = 0; d < dims; ++d) {
      const double diff = row[d] - mean[d];
      stdev[d] += diff * diff;
    }
  }
  for (double& v : stdev) {
    v = std::sqrt(v / n);
    if (v < 1e-12) v = 1.0;
  }
  for (auto& row : features) {
    for (std::size_t d = 0; d < dims; ++d) {
      row[d] = (row[d] - mean[d]) / stdev[d];
    }
  }
}

[[nodiscard]] double squared_distance(pluss::msharp::RangeView::Of<double> lhs,
                                      pluss::msharp::RangeView::Of<double> rhs) {
  double s = 0.0;
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    const double d = lhs[i] - rhs[i];
    s += d * d;
  }
  return s;
}

[[nodiscard]] double rbf_kernel(pluss::msharp::RangeView::Of<double> lhs,
                                pluss::msharp::RangeView::Of<double> rhs,
                                double length_scale) {
  const double dist2 = squared_distance(lhs, rhs);
  const double denom = 2.0 * length_scale * length_scale;
  return std::exp(-dist2 / denom);
}

[[nodiscard]] StatusOr<std::vector<double>> solve_linear_system(std::vector<std::vector<double>> a,
                                                                std::vector<double> b) {
  const std::size_t n = a.size();
  if (n == 0 || b.size() != n) {
    return StatusOr<std::vector<double>>(Status::invalid_argument(
        "bayesian solver received invalid matrix/vector dimensions."));
  }
  for (std::size_t i = 0; i < n; ++i) {
    std::size_t pivot = i;
    double best = std::abs(a[i][i]);
    for (std::size_t r = i + 1; r < n; ++r) {
      const double candidate = std::abs(a[r][i]);
      if (candidate > best) {
        best = candidate;
        pivot = r;
      }
    }
    if (best < 1e-12) {
      return StatusOr<std::vector<double>>(Status::numerical_failure(
          "bayesian solver encountered near-singular matrix."));
    }
    if (pivot != i) {
      std::swap(a[i], a[pivot]);
      std::swap(b[i], b[pivot]);
    }
    const double diag = a[i][i];
    for (std::size_t c = i; c < n; ++c) a[i][c] /= diag;
    b[i] /= diag;
    for (std::size_t r = i + 1; r < n; ++r) {
      const double factor = a[r][i];
      if (factor == 0.0) continue;
      for (std::size_t c = i; c < n; ++c) {
        a[r][c] -= factor * a[i][c];
      }
      b[r] -= factor * b[i];
    }
  }

  std::vector<double> x(n, 0.0);
  for (std::size_t ri = n; ri-- > 0;) {
    double acc = b[ri];
    for (std::size_t c = ri + 1; c < n; ++c) acc -= a[ri][c] * x[c];
    x[ri] = acc;
  }
  return StatusOr<std::vector<double>>(std::move(x));
}

[[nodiscard]] StatusOr<std::pair<double, double>> gp_predict_mean_std(
    pluss::msharp::RangeView::Of<std::vector<double>> features,
    pluss::msharp::RangeView::Of<std::int64_t> observed_indices,
    pluss::msharp::RangeView::Of<double> observed_objectives,
    std::int64_t candidate_index) {
  if (observed_indices.empty()) {
    return StatusOr<std::pair<double, double>>(std::pair<double, double>{0.0, 1.0});
  }
  if (observed_indices.size() == 1) {
    return StatusOr<std::pair<double, double>>(std::pair<double, double>{observed_objectives[0], 1.0});
  }

  const std::size_t m = observed_indices.size();
  const double length_scale = std::max<double>(1.0, std::sqrt(static_cast<double>(features.front().size())));
  std::vector<std::vector<double>> k(m, std::vector<double>(m, 0.0));
  for (std::size_t i = 0; i < m; ++i) {
    for (std::size_t j = 0; j < m; ++j) {
      const auto& xi = features[static_cast<std::size_t>(observed_indices[i])];
      const auto& xj = features[static_cast<std::size_t>(observed_indices[j])];
      k[i][j] = rbf_kernel(xi, xj, length_scale);
    }
    k[i][i] += 1e-6;
  }
  std::vector<double> y(observed_objectives.begin(), observed_objectives.end());
  StatusOr<std::vector<double>> alpha_or = solve_linear_system(k, y);
  if (!alpha_or.ok()) return StatusOr<std::pair<double, double>>(alpha_or.status());
  const std::vector<double>& alpha = alpha_or.value();

  std::vector<double> k_star(m, 0.0);
  for (std::size_t i = 0; i < m; ++i) {
    const auto& xi = features[static_cast<std::size_t>(observed_indices[i])];
    const auto& x_star = features[static_cast<std::size_t>(candidate_index)];
    k_star[i] = rbf_kernel(xi, x_star, length_scale);
  }
  double mu = 0.0;
  for (std::size_t i = 0; i < m; ++i) mu += k_star[i] * alpha[i];

  StatusOr<std::vector<double>> v_or = solve_linear_system(k, k_star);
  if (!v_or.ok()) return StatusOr<std::pair<double, double>>(v_or.status());
  const std::vector<double>& v = v_or.value();
  double quad = 0.0;
  for (std::size_t i = 0; i < m; ++i) quad += k_star[i] * v[i];
  const double variance = std::max<double>(1e-12, 1.0 - quad);
  return StatusOr<std::pair<double, double>>(std::pair<double, double>{mu, std::sqrt(variance)});
}

[[nodiscard]] TuneTrialResult evaluate_trial(const ModelSpec& spec,
                                             const LabeledBatch& batch,
                                             const SplitOptions& split,
                                             const TrainOptions& train_options,
                                             const AssessOptions& assess_options,
                                             MetricKind objective_metric) {
  TuneTrialResult trial{};
  trial.spec = spec;
  StatusOr<CrossAssessReport> assess_or = cross_assess(spec, batch, split, train_options, assess_options);
  if (!assess_or.ok()) {
    return make_failed_trial(spec, assess_or.status());
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
}

[[nodiscard]] StatusOr<TuneReport> bayesian_from_candidates(
    pluss::msharp::RangeView::Of<ModelSpec> candidates,
    std::int64_t trials,
    const LabeledBatch& batch,
    const SplitOptions& split,
    const TrainOptions& train_options,
    const AssessOptions& assess_options,
    MetricKind objective_metric,
    TuneObjectivePolicy objective_policy,
    RandomState random) {
  if (candidates.empty()) {
    return StatusOr<TuneReport>(Status::invalid_argument(
        "bayesian search requires at least one candidate ModelSpec."));
  }
  if (trials <= 0) {
    return StatusOr<TuneReport>(Status::invalid_argument(
        "bayesian search requires trials > 0."));
  }

  const std::int64_t budget = std::min<std::int64_t>(trials, static_cast<std::int64_t>(candidates.size()));
  std::vector<std::vector<double>> features{};
  features.reserve(candidates.size());
  for (const auto& spec : candidates) features.push_back(encode_model_spec(spec));
  normalize_feature_matrix(features);

  TuneReport report{};
  report.objective_metric = objective_metric;
  report.objective_policy = objective_policy;
  report.trials.reserve(static_cast<std::size_t>(budget));

  const bool maximize = maximize_objective(objective_policy, objective_metric);
  bool have_best = false;
  double best_objective = maximize ? -std::numeric_limits<double>::infinity()
                                   : std::numeric_limits<double>::infinity();

  std::vector<bool> used(candidates.size(), false);
  std::vector<std::int64_t> observed_success_indices{};
  std::vector<double> observed_success_objectives{};
  observed_success_indices.reserve(static_cast<std::size_t>(budget));
  observed_success_objectives.reserve(static_cast<std::size_t>(budget));

  std::mt19937_64 rng(compose_seed(random));
  const std::int64_t warmup = std::min<std::int64_t>(
      budget,
      std::max<std::int64_t>(2, static_cast<std::int64_t>(std::ceil(std::sqrt(static_cast<double>(budget))))));
  constexpr double kappa = 2.576;

  auto random_unused = [&](void) -> std::int64_t {
    std::vector<std::int64_t> pool{};
    for (std::size_t i = 0; i < used.size(); ++i) {
      if (!used[i]) pool.push_back(static_cast<std::int64_t>(i));
    }
    std::shuffle(pool.begin(), pool.end(), rng);
    return pool.empty() ? -1 : pool.front();
  };

  for (std::int64_t step = 0; step < budget; ++step) {
    std::int64_t selected = -1;
    if (step < warmup || observed_success_indices.size() < 2) {
      selected = random_unused();
    } else {
      double best_acq = maximize ? -std::numeric_limits<double>::infinity()
                                 : std::numeric_limits<double>::infinity();
      for (std::size_t i = 0; i < candidates.size(); ++i) {
        if (used[i]) continue;
        StatusOr<std::pair<double, double>> pred_or =
            gp_predict_mean_std(features, observed_success_indices, observed_success_objectives, static_cast<std::int64_t>(i));
        if (!pred_or.ok()) continue;
        const double mu = pred_or.value().first;
        const double sigma = pred_or.value().second;
        const double acq = maximize ? (mu + kappa * sigma) : (mu - kappa * sigma);
        const bool better = (selected < 0)
                            || (maximize ? (acq > best_acq) : (acq < best_acq));
        if (better) {
          selected = static_cast<std::int64_t>(i);
          best_acq = acq;
        }
      }
      if (selected < 0) selected = random_unused();
    }
    if (selected < 0) break;
    used[static_cast<std::size_t>(selected)] = true;

    TuneTrialResult trial =
        evaluate_trial(candidates[static_cast<std::size_t>(selected)],
                       batch,
                       split,
                       train_options,
                       assess_options,
                       objective_metric);
    trial.stage = step;
    if (trial.status.ok() && std::isfinite(trial.objective)) {
      observed_success_indices.push_back(selected);
      observed_success_objectives.push_back(trial.objective);
    }
    report.trials.push_back(std::move(trial));
    maybe_update_best(report.trials.back(),
                      maximize,
                      static_cast<std::int64_t>(report.trials.size() - 1),
                      have_best,
                      best_objective,
                      report.best_index);
  }

  assign_objective_ranks(report);
  populate_tune_ledger(report);
  return StatusOr<TuneReport>(std::move(report));
}

} // namespace

StatusOr<TuneReport> sweep(pluss::msharp::RangeView::Of<ModelSpec> candidates,
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

  assign_objective_ranks(report);
  populate_tune_ledger(report);
  return StatusOr<TuneReport>(std::move(report));
}

StatusOr<TuneReport> tune(pluss::msharp::RangeView::Of<ModelSpec> candidates,
                          const LabeledBatch& batch,
                          const TuneOptions& tune_options,
                          const TrainOptions& train_options,
                          const AssessOptions& assess_options,
                          MetricKind objective_metric,
                          TuneObjectivePolicy objective_policy) {
  if (candidates.empty()) {
    return StatusOr<TuneReport>(Status::invalid_argument("tune(candidates,...) requires at least one candidate ModelSpec."));
  }
  const SplitOptions split = tune_options.split;
  if (tune_options.search == SearchKind::None || tune_options.search == SearchKind::Grid) {
    return sweep(candidates,
                 batch,
                 split,
                 train_options,
                 assess_options,
                 objective_metric,
                 objective_policy,
                 tune_options.max_parallel);
  }
  if (tune_options.search == SearchKind::SuccessiveHalving) {
    return successive_halving_from_candidates(candidates,
                                              batch,
                                              split,
                                              train_options,
                                              assess_options,
                                              objective_metric,
                                              objective_policy,
                                              tune_options.max_parallel);
  }
  if (tune_options.search == SearchKind::Random) {
    if (tune_options.trials <= 0) {
      return StatusOr<TuneReport>(Status::invalid_argument(
          "tune(candidates,...): SearchKind::Random requires TuneOptions.trials > 0."));
    }
    std::vector<ModelSpec> sampled = sample_candidates_without_replacement(candidates,
                                                                            tune_options.trials,
                                                                            tune_options.random);
    if (sampled.empty()) {
      return StatusOr<TuneReport>(Status::invalid_argument(
          "tune(candidates,...): random sampling produced zero candidates."));
    }
    return sweep(sampled,
                 batch,
                 split,
                 train_options,
                 assess_options,
                 objective_metric,
                 objective_policy,
                 tune_options.max_parallel);
  }
  if (tune_options.search == SearchKind::Bayesian) {
    if (tune_options.trials <= 0) {
      return StatusOr<TuneReport>(Status::invalid_argument(
          "tune(candidates,...): SearchKind::Bayesian requires TuneOptions.trials > 0."));
    }
    return bayesian_from_candidates(candidates,
                                    tune_options.trials,
                                    batch,
                                    split,
                                    train_options,
                                    assess_options,
                                    objective_metric,
                                    objective_policy,
                                    tune_options.random);
  }
  return StatusOr<TuneReport>(Status::invalid_argument("tune(candidates,...): unknown SearchKind."));
}

StatusOr<TuneReport> tune(const ProbeSpecSampler& sampler,
                          const LabeledBatch& batch,
                          const TuneOptions& tune_options,
                          const TrainOptions& train_options,
                          const AssessOptions& assess_options,
                          MetricKind objective_metric,
                          TuneObjectivePolicy objective_policy) {
  if (!sampler) {
    return StatusOr<TuneReport>(Status::invalid_argument("tune(sampler,...) requires a valid sampler callback."));
  }
  if (tune_options.trials <= 0) {
    return StatusOr<TuneReport>(Status::invalid_argument("tune(sampler,...) requires TuneOptions.trials > 0."));
  }
  const SplitOptions split = tune_options.split;
  if (tune_options.search == SearchKind::None
      || tune_options.search == SearchKind::Random) {
    return probe(sampler,
                 tune_options.trials,
                 batch,
                 split,
                 train_options,
                 assess_options,
                 objective_metric,
                 objective_policy,
                 tune_options.random,
                 tune_options.max_parallel);
  }
  if (tune_options.search == SearchKind::Bayesian) {
    const std::int64_t pool_size = std::max<std::int64_t>(tune_options.trials + 8, tune_options.trials * 4);
    std::vector<ModelSpec> sampled{};
    sampled.reserve(static_cast<std::size_t>(pool_size));
    const std::uint64_t base_seed = compose_seed(tune_options.random);
    for (std::int64_t i = 0; i < pool_size; ++i) {
      const std::uint64_t seed = splitmix64(base_seed + static_cast<std::uint64_t>(i));
      StatusOr<ModelSpec> spec_or = sampler(i, seed);
      if (!spec_or.ok()) return StatusOr<TuneReport>(spec_or.status());
      sampled.push_back(spec_or.move_value());
    }
    return bayesian_from_candidates(sampled,
                                    tune_options.trials,
                                    batch,
                                    split,
                                    train_options,
                                    assess_options,
                                    objective_metric,
                                    objective_policy,
                                    tune_options.random);
  }
  if (tune_options.search == SearchKind::SuccessiveHalving) {
    std::vector<ModelSpec> sampled{};
    sampled.reserve(static_cast<std::size_t>(tune_options.trials));
    const std::uint64_t base_seed = compose_seed(tune_options.random);
    for (std::int64_t i = 0; i < tune_options.trials; ++i) {
      const std::uint64_t seed = splitmix64(base_seed + static_cast<std::uint64_t>(i));
      StatusOr<ModelSpec> spec_or = sampler(i, seed);
      if (!spec_or.ok()) return StatusOr<TuneReport>(spec_or.status());
      sampled.push_back(spec_or.move_value());
    }
    return successive_halving_from_candidates(sampled,
                                              batch,
                                              split,
                                              train_options,
                                              assess_options,
                                              objective_metric,
                                              objective_policy,
                                              tune_options.max_parallel);
  }
  if (tune_options.search == SearchKind::Grid) {
    return StatusOr<TuneReport>(Status::invalid_argument(
        "tune(sampler,...): SearchKind::Grid requires the candidate-list overload."));
  }
  return StatusOr<TuneReport>(Status::invalid_argument("tune(sampler,...): unknown SearchKind."));
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

  assign_objective_ranks(report);
  populate_tune_ledger(report);
  return StatusOr<TuneReport>(std::move(report));
}

} // namespace pluss::ai::nodej

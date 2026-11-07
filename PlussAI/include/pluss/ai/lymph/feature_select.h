#pragma once

#include "pluss/ai/lymph/common.h"
#include "pluss/ai/lymph/model.h"
#include "pluss/ai/lymph/selection.h"
#include "pluss/tils/auxiliary/transmit.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace pluss::ai::lymph {

namespace features {

using VariancePrune = ::pluss::ai::nodej::VariancePruneResult;
using ScoreRankOptions = ::pluss::ai::nodej::ScoreRankOptions;
using FeatureScoreMode = ::pluss::ai::nodej::FeatureScoreMode;
using FeatureRankOptions = ::pluss::ai::nodej::FeatureRankOptions;
  using FeaturePickOptions = ::pluss::ai::nodej::FeaturePickOptions;
  using FeatureRankReport = ::pluss::ai::nodej::FeatureRankReport;
  using Result = ::pluss::ai::nodej::FeaturePickResult;

[[nodiscard]] inline FeatureBatch take_features(const FeatureBatch& batch,
                                                pluss::msharp::RangeView::Int feature_indices) {
  return resolve(::pluss::ai::nodej::take_features(batch, feature_indices), "features::take_features");
}

[[nodiscard]] inline FeatureBatch take_features(const FeatureBatch& batch,
                                                std::initializer_list<std::int64_t> feature_indices) {
  return resolve(::pluss::ai::nodej::take_features(batch, pluss::msharp::RangeView::Int(feature_indices)),
                 "features::take_features");
}

[[nodiscard]] inline VariancePrune variance_prune(const FeatureBatch& batch, double variance_floor = 1e-12) {
  return resolve(::pluss::ai::nodej::variance_prune(batch, variance_floor), "features::variance_prune");
}

[[nodiscard]] inline FeatureRankReport rank_features(const LabeledBatch& batch, const FeatureRankOptions& options = {}) {
  return resolve(::pluss::ai::nodej::rank_features(batch, options), "features::rank_features");
}

[[nodiscard]] inline Result pick_features(const LabeledBatch& batch, const FeaturePickOptions& options = {}) {
  return resolve(::pluss::ai::nodej::pick_features(batch, options), "features::pick_features");
}

[[nodiscard]] inline Series f_score_rank(const LabeledBatch& batch, const ScoreRankOptions& options = {}) {
  return resolve(::pluss::ai::nodej::f_score_rank(batch, options), "features::f_score_rank");
}

[[nodiscard]] inline Series f_regression_rank(const LabeledBatch& batch, const ScoreRankOptions& options = {}) {
  return resolve(::pluss::ai::nodej::f_regression_rank(batch, options), "features::f_regression_rank");
}

// ---------------------------------------------------------------------------
// Permutation importance -- model-agnostic feature importance from an
// ALREADY-TRAINED model, matching sklearn.inspection.permutation_importance:
// shuffle one feature column at a time, re-score, and call the drop in score
// that feature's importance. Unlike nodej::permutation_impact (which takes a
// ModelSpec and trains it internally), this takes the lymph models::Model you
// already fit, so it works uniformly across every model family without a
// per-family coef_ / feature_importances_ accessor.
// ---------------------------------------------------------------------------

struct PermutationOptions {
  std::int64_t repeats = 5;
  RandomState random = {};
};

struct PermutationImportance {
  // Mean / stdev of the per-repeat score drop, one entry per feature (column
  // order of the evaluated FeatureBatch).
  Series importances_mean = pluss::series::undefined();  // shape [n_features]
  Series importances_std = pluss::series::undefined();   // shape [n_features]
  // Raw per-(repeat, feature) drops, same layout as nodej::PermuteImpactReport.
  Series importances = pluss::series::undefined();       // shape [n_repeats, n_features]
  std::string objective_name = "accuracy";
};

namespace detail {

[[nodiscard]] inline double score_prediction_(const selection::Scorer& objective,
                                              TaskKind task,
                                              const Series& truth,
                                              const Series& prediction) {
  if (objective.is_custom()) return objective.fn(truth, prediction);
  return task == TaskKind::Classification
             ? metrics::classification::report(truth, prediction, {*objective.builtin}).values.host<double>().front()
             : metrics::regression::report(truth, prediction, {*objective.builtin}).values.host<double>().front();
}

[[nodiscard]] inline std::uint64_t splitmix64_(std::uint64_t x) {
  x += 0x9e3779b97f4a7c15ULL;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
  return x ^ (x >> 31);
}

// One independent permutation Series per (feature, repeat) cell, derived
// deterministically from the base seed so a run is reproducible when
// `random.deterministic` is set -- and safe to compute concurrently, unlike a
// single RNG instance shared across worker threads.
[[nodiscard]] inline Series random_permutation_(std::int64_t n, std::uint64_t seed, Platform platform) {
  std::vector<std::int64_t> order(static_cast<std::size_t>(n));
  std::iota(order.begin(), order.end(), std::int64_t{0});
  std::mt19937_64 rng(seed);
  std::shuffle(order.begin(), order.end(), rng);
  return pluss::make_series(order, {n}, Config{}.with_dtype(ScalarType::Int64).with_platform(platform));
}

// The feature matrix with column `feature_index` replaced by
// `x.index_select(0, perm_idx)` restricted to that column -- built from
// Series slicing + concatenation (narrow_copy/unite) rather than a host round
// trip, so the permuted probe stays on whatever platform `x` already lives on.
[[nodiscard]] inline Series permuted_feature_matrix_(const Series& x, std::int64_t feature_index, const Series& perm_idx) {
  const std::int64_t n_features = x.sizes()[1];
  const Series shuffled_col = x.narrow_copy(1, feature_index, 1).index_select(0, perm_idx);
  Series out = feature_index > 0 ? x.narrow_copy(1, 0, feature_index) : pluss::series::undefined();
  out = out.defined() ? out.unite({shuffled_col}, 1) : shuffled_col;
  if (feature_index + 1 < n_features) {
    out = out.unite({x.narrow_copy(1, feature_index + 1, n_features - feature_index - 1)}, 1);
  }
  return out;
}

} // namespace detail

// `eval` is scored as-is with the model exactly as it was fit -- pass a
// held-out split for an honest importance signal (the same way you would
// hand permutation_importance test data in sklearn), or the training batch
// when that is all you have. `objective` is a selection::Scorer: implicitly a
// MetricKind, or selection::make_scorer("name", fn) for a custom metric.
[[nodiscard]] inline PermutationImportance permutation_importance(const models::Model& model,
                                                                   const LabeledBatch& eval,
                                                                   selection::Scorer objective = MetricKind::Accuracy,
                                                                   PermutationOptions options = {}) {
  if (options.repeats <= 0) {
    ::pluss::throw_error("features::permutation_importance: repeats must be > 0.");
  }
  const std::int64_t samples = eval.features.sample_count();
  const std::int64_t n_features = eval.features.feature_count();
  if (samples <= 0 || n_features <= 0) {
    ::pluss::throw_error("features::permutation_importance: eval batch needs at least one sample and one feature.");
  }
  if (eval.features.layout != ObservationLayout::Rows) {
    ::pluss::throw_error("features::permutation_importance: only ObservationLayout::Rows is supported.");
  }

  const TaskKind task = model.task_kind();
  const bool prefers_max = objective.is_custom() ? objective.greater_is_better
                                                  : ::pluss::ai::nodej::metric_prefers_max(*objective.builtin);
  const double baseline = detail::score_prediction_(objective, task, eval.y, model.infer(eval.features));

  const Platform platform = eval.features.x.platform();
  const Series x = eval.features.x.to(platform, ScalarType::Float64);
  const std::uint64_t base_seed =
      options.random.deterministic ? options.random.seed : detail::splitmix64_(options.random.seed ^ 0xa3f1c2b5ULL);

  struct Cell {
    std::int64_t feature;
    std::int64_t repeat;
  };
  std::vector<Cell> cells;
  cells.reserve(static_cast<std::size_t>(options.repeats * n_features));
  for (std::int64_t r = 0; r < options.repeats; ++r) {
    for (std::int64_t f = 0; f < n_features; ++f) cells.push_back(Cell{f, r});
  }

  // Every cell only reads the already-fitted `model` (infer() is const) and
  // the pristine `x` / `eval.y` -- independent work, run across
  // hardware_concurrency() workers instead of one (feature, repeat) at a time.
  const auto evaluate_cell = [&](const Cell& cell) -> double {
    const std::uint64_t cell_seed = detail::splitmix64_(
        base_seed ^ (static_cast<std::uint64_t>(cell.feature) * 0x9E3779B97F4A7C15ULL) ^
        (static_cast<std::uint64_t>(cell.repeat) * 0xD1B54A32D192ED03ULL));
    const Series perm_idx = detail::random_permutation_(samples, cell_seed, platform);
    const Series permuted_x = detail::permuted_feature_matrix_(x, cell.feature, perm_idx);
    const Series prediction = model.infer(FeatureBatch{.x = permuted_x, .layout = ObservationLayout::Rows});
    const double permuted_score = detail::score_prediction_(objective, task, eval.y, prediction);
    return prefers_max ? (baseline - permuted_score) : (permuted_score - baseline);
  };

  pluss::tils::transmit_options topts{};
  topts.grain_size = 1;
  topts.exec = cells.size() > 1 ? pluss::tils::transmit_exec::par : pluss::tils::transmit_exec::seq;
  const std::vector<double> drops = pluss::tils::transmit(evaluate_cell, cells, topts);

  std::vector<double> mean(static_cast<std::size_t>(n_features), 0.0);
  std::vector<double> stdev(static_cast<std::size_t>(n_features), 0.0);
  for (std::int64_t f = 0; f < n_features; ++f) {
    double sum = 0.0;
    for (std::int64_t r = 0; r < options.repeats; ++r) {
      sum += drops[static_cast<std::size_t>(r * n_features + f)];
    }
    mean[static_cast<std::size_t>(f)] = sum / static_cast<double>(options.repeats);
    double var = 0.0;
    for (std::int64_t r = 0; r < options.repeats; ++r) {
      const double d = drops[static_cast<std::size_t>(r * n_features + f)] - mean[static_cast<std::size_t>(f)];
      var += d * d;
    }
    stdev[static_cast<std::size_t>(f)] = std::sqrt(var / static_cast<double>(options.repeats));
  }

  const Config f64_cpu = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  return PermutationImportance{
      .importances_mean = pluss::make_series(mean, {n_features}, f64_cpu),
      .importances_std = pluss::make_series(stdev, {n_features}, f64_cpu),
      .importances = pluss::make_series(drops, {options.repeats, n_features}, f64_cpu),
      .objective_name = objective.name,
  };
}

// ---------------------------------------------------------------------------
// Recursive feature elimination -- repeatedly fit, rank the surviving
// features by permutation importance, drop the least important `step` of
// them, and refit, until `n_features_to_select` remain. Mirrors sklearn's
// RFE loop (support_ / ranking_ semantics included); the one deliberate
// departure is the importance signal: lymph's models::Model spans model
// families with no uniform coef_ / feature_importances_ accessor, so RFE
// ranks by permutation_importance instead -- model-agnostic, and consistent
// with the rest of lymph's Scorer-based design.
// ---------------------------------------------------------------------------

struct RFEOptions {
  // < 0 => half the starting features (sklearn's own default), floored to 1.
  std::int64_t n_features_to_select = -1;
  // Features eliminated per round; the final round eliminates fewer if
  // `step` would overshoot n_features_to_select.
  std::int64_t step = 1;
  std::int64_t importance_repeats = 5;
  RandomState random = {};
  TrainOptions train_options = {};
};

struct RFEReport {
  // Both indexed by the ORIGINAL feature position, length = starting feature
  // count -- support[i] true means feature i survived to the final model;
  // ranking[i] is 1 for a surviving feature, and higher the earlier a
  // feature was eliminated (least important features carry the highest
  // numbers), matching sklearn's ranking_ convention.
  std::vector<bool> support{};
  std::vector<std::int64_t> ranking{};
  std::vector<std::int64_t> selected_feature_indices{};
  models::Model final_model;  // no default TrainOptions-free Model exists; always set below
  double final_score = std::numeric_limits<double>::quiet_NaN();
  std::string objective_name = "accuracy";
};

[[nodiscard]] inline RFEReport rfe(const models::Model& model,
                                   const LabeledBatch& batch,
                                   selection::Scorer objective = MetricKind::Accuracy,
                                   RFEOptions options = {}) {
  const std::int64_t n = batch.features.feature_count();
  if (n <= 0) {
    ::pluss::throw_error("features::rfe: batch must have at least one feature.");
  }
  if (options.step <= 0) {
    ::pluss::throw_error("features::rfe: RFEOptions.step must be positive.");
  }
  std::int64_t n_to_select = options.n_features_to_select;
  if (n_to_select < 0) n_to_select = std::max<std::int64_t>(1, n / 2);
  if (n_to_select < 1 || n_to_select > n) {
    ::pluss::throw_error("features::rfe: RFEOptions.n_features_to_select must be in [1, feature_count].");
  }

  std::vector<bool> support(static_cast<std::size_t>(n), true);
  std::vector<std::int64_t> ranking(static_cast<std::size_t>(n), 1);

  const auto remaining_indices = [&]() -> std::vector<std::int64_t> {
    std::vector<std::int64_t> out;
    out.reserve(static_cast<std::size_t>(n));
    for (std::int64_t i = 0; i < n; ++i) {
      if (support[static_cast<std::size_t>(i)]) out.push_back(i);
    }
    return out;
  };
  const auto count_selected = [&]() -> std::int64_t {
    return std::count(support.begin(), support.end(), true);
  };

  while (count_selected() > n_to_select) {
    const std::vector<std::int64_t> remaining = remaining_indices();
    const FeatureBatch sub_features = features::take_features(batch.features, pluss::msharp::RangeView::Int(remaining));
    const LabeledBatch sub_batch{.features = sub_features, .y = batch.y, .sample_weights = batch.sample_weights};

    const models::Model trained = model.learn(sub_batch, options.train_options);
    const PermutationImportance importance = permutation_importance(
        trained, sub_batch, objective, PermutationOptions{.repeats = options.importance_repeats, .random = options.random});
    const auto values = importance.importances_mean.to(pluss::pCPU, ScalarType::Float64)
                             .reshape({importance.importances_mean.numel()})
                             .host<double>().array().data;

    // Ascending by importance: the front of `order` is least important, in
    // local (sub_batch) column order.
    std::vector<std::int64_t> order(remaining.size());
    std::iota(order.begin(), order.end(), std::int64_t{0});
    std::stable_sort(order.begin(), order.end(),
                     [&](std::int64_t a, std::int64_t b) { return values[static_cast<std::size_t>(a)] < values[static_cast<std::size_t>(b)]; });

    const std::int64_t to_drop = std::min<std::int64_t>(options.step, count_selected() - n_to_select);
    for (std::int64_t k = 0; k < to_drop; ++k) {
      support[static_cast<std::size_t>(remaining[static_cast<std::size_t>(order[static_cast<std::size_t>(k)])])] = false;
    }
    for (std::int64_t i = 0; i < n; ++i) {
      if (!support[static_cast<std::size_t>(i)]) ++ranking[static_cast<std::size_t>(i)];
    }
  }

  const std::vector<std::int64_t> selected = remaining_indices();
  const FeatureBatch final_features = features::take_features(batch.features, pluss::msharp::RangeView::Int(selected));
  const LabeledBatch final_batch{.features = final_features, .y = batch.y, .sample_weights = batch.sample_weights};
  const models::Model final_model = model.learn(final_batch, options.train_options);
  const double final_score =
      detail::score_prediction_(objective, model.task_kind(), final_batch.y, final_model.infer(final_features));

  return RFEReport{
      .support = std::move(support),
      .ranking = std::move(ranking),
      .selected_feature_indices = selected,
      .final_model = final_model,
      .final_score = final_score,
      .objective_name = objective.name,
  };
}

} // namespace features

} // namespace pluss::ai::lymph

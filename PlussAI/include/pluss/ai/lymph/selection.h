#pragma once

#include "pluss/ai/lymph/common.h"
#include "pluss/ai/lymph/metrics.h"
#include "pluss/ai/lymph/model.h"
#include "pluss/tils/aux/transmit.h"

#include <random>

namespace pluss::ai::lymph {

namespace selection {

struct Candidate {
  std::string name{};
  // Holds the public cheap-copy Model handle, so model registries can be
  // passed into selection without std::move and without exposing the internal
  // ModelCore implementation.
  Model model;
  // nullopt (default) = use compare()'s shared train_options, matching prior
  // behavior exactly. Set this when one candidate genuinely needs different
  // training settings than the rest of the grid (e.g. calibration on a
  // logistic candidate but not on a tree candidate) -- the reason compare() used
  // to get abandoned in favor of a hand-rolled loop whenever that came up.
  std::optional<TrainOptions> train_options{};

  Candidate(std::string in_name, Model in_model, std::optional<TrainOptions> in_train_options = std::nullopt)
      : name(std::move(in_name)), model(std::move(in_model)), train_options(std::move(in_train_options)) {}
  Candidate(Candidate&&) noexcept = default;
  Candidate& operator=(Candidate&&) noexcept = default;
  Candidate(const Candidate&) noexcept = default;
  Candidate& operator=(const Candidate&) noexcept = default;
};

struct Trial {
  std::string name{};
  double objective = std::numeric_limits<double>::quiet_NaN();
  CrossValidationReport cv{};
};

// Named compare, not tune: this ranks candidates the caller has already
// fully built (a fixed name + Model + optional TrainOptions each) via
// cross-validation -- model *selection*, not hyperparameter *search*. See
// Search further down for the latter; keeping the two roles
// distinct is deliberate after this exact function briefly carried the name
// "tune" and was mistaken for a hyperparameter search entry point.
struct CompareReport {
  std::vector<Trial> trials{};
  std::int64_t best_index = -1;
  std::string best_name{};
  MetricKind objective_metric = MetricKind::Accuracy;
  TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto;
  std::vector<std::int64_t> objective_rank{};
  std::vector<std::int64_t> ranked_indices{};

  // One row per trial: name, objective, rank, and every requested metric's
  // fold mean (all trials share the same metric set within one compare() call).
  [[nodiscard]] pluss::table::DataFrame board() const;
};

struct CompareResult {
  CompareReport report{};
  // Was best_workflow: Workflow -- field renamed alongside the type change
  // Holds public Models rather than internal implementation objects. std::optional
  // keeps CompareResult default-constructible before the winner is known.
  std::optional<Model> best_model{};
  std::vector<Model> fitted{};

  [[nodiscard]] const Model& best() const {
    if (!best_model.has_value()) {
      ::pluss::throw_error("selection::CompareResult::best: no best model has been computed.");
    }
    return *best_model;
  }
  [[nodiscard]] Model& best() {
    if (!best_model.has_value()) {
      ::pluss::throw_error("selection::CompareResult::best: no best model has been computed.");
    }
    return *best_model;
  }

  [[nodiscard]] std::vector<std::pair<std::string, Model>> top(std::int64_t k) const {
    if (k < 0) {
      ::pluss::throw_error("selection::CompareResult::top: k must be non-negative.");
    }
    if (k > static_cast<std::int64_t>(fitted.size())) {
      ::pluss::throw_error("selection::CompareResult::top: requested k exceeds fitted model count; increase FitOptions.fit_top.");
    }
    std::vector<std::pair<std::string, Model>> out{};
    out.reserve(static_cast<std::size_t>(k));
    for (std::int64_t i = 0; i < k; ++i) {
      const std::int64_t trial_index = report.ranked_indices[static_cast<std::size_t>(i)];
      out.emplace_back(report.trials[static_cast<std::size_t>(trial_index)].name,
                       fitted[static_cast<std::size_t>(i)]);
    }
    return out;
  }
};

[[nodiscard]] inline pluss::table::DataFrame CompareReport::board() const {
  std::vector<std::string> names{};
  std::vector<double> objectives{};
  std::vector<std::int64_t> ranks{};
  names.reserve(trials.size());
  objectives.reserve(trials.size());
  ranks.reserve(trials.size());

  const std::vector<MetricKind> metrics = trials.empty() ? std::vector<MetricKind>{} : trials.front().cv.mean.requested;
  std::vector<std::vector<double>> metric_columns(metrics.size());
  for (auto& column : metric_columns) column.reserve(trials.size());

  for (std::size_t i = 0; i < trials.size(); ++i) {
    names.push_back(trials[i].name);
    objectives.push_back(trials[i].objective);
    ranks.push_back(i < objective_rank.size() ? objective_rank[i] : 0);
    const auto values = trials[i].cv.mean.values.to(pluss::pCPU, ScalarType::Float64)
                             .reshape({trials[i].cv.mean.values.numel()})
                             .to_vec<double>();
    for (std::size_t m = 0; m < metrics.size(); ++m) {
      metric_columns[m].push_back(m < values.size() ? values[m] : std::numeric_limits<double>::quiet_NaN());
    }
  }

  const Config f64_cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Config i64_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
  const auto n = static_cast<std::int64_t>(trials.size());

  std::vector<std::pair<std::string, pluss::table::Thread>> columns{};
  columns.reserve(3 + metrics.size());
  columns.emplace_back("name", pluss::table::Thread(pluss::make_note(names)));
  columns.emplace_back("objective", pluss::table::Thread(pluss::make_series(objectives, {n}, f64_cfg)));
  columns.emplace_back("rank", pluss::table::Thread(pluss::make_series(ranks, {n}, i64_cfg)));
  for (std::size_t m = 0; m < metrics.size(); ++m) {
    columns.emplace_back(metric_kind_name_(metrics[m]),
                         pluss::table::Thread(pluss::make_series(metric_columns[m], {n}, f64_cfg)));
  }

  return pluss::table::DataFrame(std::move(columns)).title("Candidate Comparison Ranking");
}

struct FitOptions {
  std::int64_t fit_top = 0;
};

struct SpecTuneResult {
  ::pluss::ai::nodej::TuneReport report{};
  ModelSpec best_spec{};
};

[[nodiscard]] inline bool has_ledger(const SpecTuneResult& result) {
  return result.report.ledger.objective.defined();
}

[[nodiscard]] inline const ::pluss::ai::nodej::TuneLedger& tune_ledger(const SpecTuneResult& result) {
  if (!has_ledger(result)) {
    ::pluss::throw_error("selection::tune_ledger: tune report does not include ledger artifacts.");
  }
  return result.report.ledger;
}

[[nodiscard]] inline Series objective_series(const SpecTuneResult& result) {
  return tune_ledger(result).objective;
}

[[nodiscard]] inline Series objective_rank_series(const SpecTuneResult& result) {
  return tune_ledger(result).objective_rank;
}

[[nodiscard]] inline Series encoded_param_table(const SpecTuneResult& result) {
  return tune_ledger(result).encoded_params;
}

[[nodiscard]] inline bool minimize_metric_(MetricKind metric, TuneObjectivePolicy policy) {
  if (policy == TuneObjectivePolicy::Minimize) return true;
  if (policy == TuneObjectivePolicy::Maximize) return false;
  switch (metric) {
    case MetricKind::LogLoss:
    case MetricKind::Mse:
    case MetricKind::Rmse:
    case MetricKind::Mae:
    case MetricKind::QuantileLoss:
      return true;
    default:
      return false;
  }
}

[[nodiscard]] inline double scalar_metric_(const MetricReport& report, MetricKind metric) {
  const std::size_t n = std::min(report.requested.size(), static_cast<std::size_t>(report.values.numel()));
  const auto values = report.values.to(pluss::pCPU, ScalarType::Float64).reshape({report.values.numel()}).to_vec<double>();
  for (std::size_t i = 0; i < n && i < values.size(); ++i) {
    if (report.requested[i] == metric) return values[i];
  }
  ::pluss::throw_error("selection::scalar_metric: objective metric not present in assessment metrics.");
}

inline void assign_objective_ranks_(CompareReport& report) {
  const bool minimize = minimize_metric_(report.objective_metric, report.objective_policy);
  report.objective_rank.assign(report.trials.size(), 0);
  report.ranked_indices.clear();
  report.ranked_indices.reserve(report.trials.size());
  for (std::size_t i = 0; i < report.trials.size(); ++i) {
    if (std::isfinite(report.trials[i].objective)) {
      report.ranked_indices.push_back(static_cast<std::int64_t>(i));
    }
  }
  std::stable_sort(report.ranked_indices.begin(),
                   report.ranked_indices.end(),
                   [&](std::int64_t li, std::int64_t ri) {
                     const double lhs = report.trials[static_cast<std::size_t>(li)].objective;
                     const double rhs = report.trials[static_cast<std::size_t>(ri)].objective;
                     if (lhs == rhs) return false;
                     return minimize ? (lhs < rhs) : (lhs > rhs);
                   });
  if (report.ranked_indices.empty()) {
    std::fill(report.objective_rank.begin(), report.objective_rank.end(), std::int64_t{1});
    return;
  }

  std::int64_t current_rank = 1;
  double previous = report.trials[static_cast<std::size_t>(report.ranked_indices.front())].objective;
  report.objective_rank[static_cast<std::size_t>(report.ranked_indices.front())] = current_rank;
  for (std::size_t pos = 1; pos < report.ranked_indices.size(); ++pos) {
    const std::int64_t trial_index = report.ranked_indices[pos];
    const double value = report.trials[static_cast<std::size_t>(trial_index)].objective;
    if (value != previous) {
      current_rank = static_cast<std::int64_t>(pos) + 1;
      previous = value;
    }
    report.objective_rank[static_cast<std::size_t>(trial_index)] = current_rank;
  }

  const std::int64_t worst_rank = static_cast<std::int64_t>(report.ranked_indices.size()) + 1;
  for (std::size_t i = 0; i < report.objective_rank.size(); ++i) {
    if (report.objective_rank[i] == 0) report.objective_rank[i] = worst_rank;
  }
}

// Shared body for every compare() overload below -- takes an already-built
// LabeledBatch so the (x, y), (x, Thread), and (frame, DataFrameBatchOptions)
// entry points only differ in how they construct that one batch (Reject-only
// vs. the caller's own note_policy, e.g. DataNotePolicy::Factorize) and
// otherwise share every line of the actual grid-scheduling/ranking logic.
[[nodiscard]] inline CompareResult compare_batch_(std::vector<Candidate> candidates,
                                                   const LabeledBatch& batch,
                                                   CrossValidationOptions cv,
                                                   TrainOptions train_options,
                                                   AssessOptions assess_options,
                                                   MetricKind objective_metric,
                                                   TuneObjectivePolicy objective_policy,
                                                   FitOptions fit_options) {
  if (candidates.empty()) {
    ::pluss::throw_error("selection::compare: at least one candidate is required.");
  }
  if (fit_options.fit_top < 0) {
    ::pluss::throw_error("selection::compare: FitOptions.fit_top must be non-negative.");
  }
  if (assess_options.metrics.empty()) {
    assess_options.metrics = {objective_metric};
  } else if (std::find(assess_options.metrics.begin(), assess_options.metrics.end(), objective_metric) == assess_options.metrics.end()) {
    assess_options.metrics.push_back(objective_metric);
  }

  const bool minimize = minimize_metric_(objective_metric, objective_policy);
  CompareResult result{};
  result.report.objective_metric = objective_metric;
  result.report.objective_policy = objective_policy;
  result.report.trials.reserve(candidates.size());

  const TaskKind task = candidates.front().model.task_kind();
  for (std::size_t i = 1; i < candidates.size(); ++i) {
    if (candidates[i].model.task_kind() != task) {
      ::pluss::throw_error("selection::compare: all candidates must have the same task kind.");
    }
  }
  const SplitOptions split = detail::effective_workflow_split_options_(cv, task);
  const SplitPlan plan = detail::workflow_split_plan_(batch, split, task, "selection::compare");
  const std::vector<FoldBatchPair> fold_batches = detail::materialize_fold_batches_(batch, plan, "selection::compare");

  std::int64_t best_index = -1;
  double best_objective = std::numeric_limits<double>::quiet_NaN();

  // Flatten every (candidate, fold) pair into one pluss::aux::transmit(...,
  // par) dispatch instead of nesting "parallel folds, sequential
  // candidates" (cross_validate_batches_'s own shape, used one candidate at
  // a time by the loop this replaced): on an 8+ core machine tuning 5
  // candidates x 3 folds, that shape used at most 3 threads at a time, 5
  // times in a row. This uses up to hardware_concurrency() threads across
  // the whole grid at once.
  struct GridCell_ {
    std::size_t candidate_index;
    std::size_t fold_index;
  };
  const std::size_t candidate_n = candidates.size();
  const std::size_t fold_n = fold_batches.size();
  std::vector<GridCell_> cells{};
  cells.reserve(candidate_n * fold_n);
  for (std::size_t ci = 0; ci < candidate_n; ++ci) {
    for (std::size_t fi = 0; fi < fold_n; ++fi) {
      cells.push_back(GridCell_{ci, fi});
    }
  }

  const std::int64_t metric_count = static_cast<std::int64_t>(assess_options.metrics.size());
  const auto evaluate_cell = [&](const GridCell_& cell) -> FoldAssessResult {
    const Candidate& candidate = candidates[cell.candidate_index];
    const auto& fold = plan.folds[cell.fold_index];
    const FoldBatchPair& fold_batch = fold_batches[cell.fold_index];
    const Model trained = candidate.model.learn(
        fold_batch.train, candidate.train_options.value_or(train_options));
    const Series fold_metrics =
        trained.assess_series(fold_batch.test, assess_options).to(ScalarType::Float64).reshape({metric_count});
    return FoldAssessResult{
        .metrics = fold_metrics,
        .train_indices = fold.train_indices,
        .test_indices = fold.test_indices,
    };
  };

  pluss::aux::transmit_options topts{};
  topts.grain_size = 1;
  if (split.max_parallel == 1 || cells.size() <= 1) {
    topts.exec = pluss::aux::transmit_exec::seq;
  } else {
    topts.exec = pluss::aux::transmit_exec::par;
    topts.workers = split.max_parallel > 0 ? static_cast<std::size_t>(split.max_parallel) : 0;
  }
  const std::vector<FoldAssessResult> cell_results = pluss::aux::transmit(evaluate_cell, cells, topts);

  for (std::size_t ci = 0; ci < candidate_n; ++ci) {
    CrossAssessReport report{};
    report.metrics = assess_options.metrics;
    report.folds.resize(fold_n);
    for (std::size_t fi = 0; fi < fold_n; ++fi) {
      report.folds[fi] = cell_results[ci * fold_n + fi];
    }
    CrossValidationReport cv_report = detail::cross_validation_report_from_folds_(std::move(report));
    const double objective = scalar_metric_(cv_report.mean, objective_metric);
    result.report.trials.push_back(Trial{
        .name = candidates[ci].name,
        .objective = objective,
        .cv = std::move(cv_report),
    });

    if (best_index < 0) {
      best_index = static_cast<std::int64_t>(ci);
      best_objective = objective;
      continue;
    }
    const bool better = minimize ? (objective < best_objective) : (objective > best_objective);
    if (better) {
      best_index = static_cast<std::int64_t>(ci);
      best_objective = objective;
    }
  }

  result.report.best_index = best_index;
  result.report.best_name = candidates[static_cast<std::size_t>(best_index)].name;
  assign_objective_ranks_(result.report);
  if (result.report.ranked_indices.empty()) {
    ::pluss::throw_error("selection::compare: no finite objective candidate was available to fit.");
  }

  if (fit_options.fit_top > 0) {
    const std::int64_t fit_count =
        std::min<std::int64_t>(fit_options.fit_top, static_cast<std::int64_t>(result.report.ranked_indices.size()));
    result.fitted.reserve(static_cast<std::size_t>(fit_count));
    for (std::int64_t i = 0; i < fit_count; ++i) {
      const std::int64_t candidate_index = result.report.ranked_indices[static_cast<std::size_t>(i)];
      const auto& candidate = candidates[static_cast<std::size_t>(candidate_index)];
      Model fitted = candidate.model.learn(batch, candidate.train_options.value_or(train_options));
      result.fitted.push_back(std::move(fitted));
    }
    // ranked_indices.front() == best_index here (assign_objective_ranks_ sorts
    // with the same minimize/maximize comparator used to pick best_index
    // above, both breaking ties by keeping the earliest candidate), so
    // result.fitted[0] is already a trained copy of the best candidate --
    // clone it (dump+reload trained state, not a retrain) instead of training
    // the winner a second time.
    result.best_model = result.fitted.front().clone();
    return result;
  }

  const auto& best_candidate = candidates[static_cast<std::size_t>(best_index)];
  result.best_model = best_candidate.model.learn(batch, best_candidate.train_options.value_or(train_options));
  return result;
}

// Direct LabeledBatch entry point -- every other compare() overload below
// just builds a LabeledBatch from a DataFrame (deliberately CPU-only, see
// DataFrame's own docs) and forwards to compare_batch_ regardless; this one
// skips that step entirely; so it is the one to reach for when the caller
// already has raw x/y Series (LabeledBatch is nothing but
// {FeatureBatch{x}, y, sample_weights}, no DataFrame anywhere in its
// definition -- see nodej/dataset.h). Nothing in compare_batch_/
// Model::learn_ cares which platform those Series live on: NodeJ's
// model cores dispatch on the tensor's own device, so a Series built with
// `Config{}.with_platform(pluss::pGPU)` trains/evaluates on GPU straight
// through this path with no separate "target platform" argument to pass --
// the platform is a property of the data you hand in, not a policy knob.
[[nodiscard]] inline CompareResult compare(std::vector<Candidate> candidates,
                                           const LabeledBatch& batch,
                                           CrossValidationOptions cv = {},
                                           TrainOptions train_options = {},
                                           AssessOptions assess_options = {},
                                           MetricKind objective_metric = MetricKind::Accuracy,
                                           TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                           FitOptions fit_options = {}) {
  return compare_batch_(std::move(candidates), batch, cv, train_options, assess_options,
                        objective_metric, objective_policy, fit_options);
}

[[nodiscard]] inline CompareResult compare(std::vector<Candidate> candidates,
                                           const pluss::table::DataFrame& x,
                                           const Series& y,
                                           CrossValidationOptions cv = {},
                                           TrainOptions train_options = {},
                                           AssessOptions assess_options = {},
                                           MetricKind objective_metric = MetricKind::Accuracy,
                                           TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                           FitOptions fit_options = {}) {
  const auto feature_names = cross_validate_default_feature_names_(x, "selection::compare");
  const LabeledBatch batch = cross_validate_labeled_from_xy_(x, y, feature_names, "selection::compare");
  return compare_batch_(std::move(candidates), batch, cv, train_options, assess_options,
                        objective_metric, objective_policy, fit_options);
}

// (frame, DataFrameBatchOptions) sibling of the (x, y) overload above -- see
// model-backed report builders need this sibling for the same reason: the
// (x, y) form has no
// DataFrameBatchOptions to carry a note_policy on, so it can never see a raw
// Note column; this one threads the caller's own `data_options` (e.g.
// `note_policy = DataNotePolicy::Factorize`) straight into
// `labeled_batch_from_dataframe` instead.
[[nodiscard]] inline CompareResult compare(std::vector<Candidate> candidates,
                                           const pluss::table::DataFrame& frame,
                                           const DataFrameBatchOptions& data_options,
                                           CrossValidationOptions cv = {},
                                           TrainOptions train_options = {},
                                           AssessOptions assess_options = {},
                                           MetricKind objective_metric = MetricKind::Accuracy,
                                           TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                           FitOptions fit_options = {}) {
  const LabeledBatch batch = resolve(labeled_batch_from_dataframe(frame, data_options),
                                     "selection::compare(frame,data_options).labeled_batch_from_dataframe");
  return compare_batch_(std::move(candidates), batch, cv, train_options, assess_options,
                        objective_metric, objective_policy, fit_options);
}

[[nodiscard]] inline CompareResult compare(std::vector<Candidate> candidates,
                                           const pluss::table::DataFrame& x,
                                           const pluss::table::Thread& y,
                                           CrossValidationOptions cv = {},
                                           TrainOptions train_options = {},
                                           AssessOptions assess_options = {},
                                           MetricKind objective_metric = MetricKind::Accuracy,
                                           TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                           FitOptions fit_options = {}) {
  return compare(std::move(candidates),
              x,
              as_stratify_series(y),
              cv,
              train_options,
              assess_options,
              objective_metric,
              objective_policy,
              fit_options);
}

[[nodiscard]] inline SpecTuneResult tune_specs(pluss::msharp::RangeView::Of<ModelSpec> candidates,
                                               const LabeledBatch& batch,
                                               const SplitOptions& split = {},
                                               const TrainOptions& train_options = {},
                                               const AssessOptions& assess_options = {},
                                               MetricKind objective_metric = MetricKind::Accuracy,
                                               TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto) {
  auto report = resolve(::pluss::ai::nodej::sweep(candidates,
                                                  batch,
                                                  split,
                                                  train_options,
                                                  assess_options,
                                                  objective_metric,
                                                  objective_policy),
                        "selection::tune_specs.sweep");
  if (report.best_index < 0 || static_cast<std::size_t>(report.best_index) >= report.trials.size()) {
    ::pluss::throw_error("selection::tune_specs: best_index is out of range.");
  }
  return SpecTuneResult{
      .report = report,
      .best_spec = report.trials[static_cast<std::size_t>(report.best_index)].spec,
  };
}

[[nodiscard]] inline SpecTuneResult tune_specs(pluss::msharp::RangeView::Of<ModelSpec> candidates,
                                               const LabeledBatch& batch,
                                               const TuneOptions& tune_options,
                                               const TrainOptions& train_options = {},
                                               const AssessOptions& assess_options = {},
                                               MetricKind objective_metric = MetricKind::Accuracy,
                                               TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto) {
  auto report = resolve(::pluss::ai::nodej::tune(candidates,
                                                 batch,
                                                 tune_options,
                                                 train_options,
                                                 assess_options,
                                                 objective_metric,
                                                 objective_policy),
                        "selection::tune_specs.tune");
  if (report.best_index < 0 || static_cast<std::size_t>(report.best_index) >= report.trials.size()) {
    ::pluss::throw_error("selection::tune_specs: best_index is out of range.");
  }
  return SpecTuneResult{
      .report = report,
      .best_spec = report.trials[static_cast<std::size_t>(report.best_index)].spec,
  };
}

[[nodiscard]] inline SpecTuneResult tune_specs(pluss::msharp::RangeView::Of<ModelSpec> candidates,
                                               const pluss::table::DataFrame& x,
                                               const Series& y,
                                               const SplitOptions& split = {},
                                               const TrainOptions& train_options = {},
                                               const AssessOptions& assess_options = {},
                                               MetricKind objective_metric = MetricKind::Accuracy,
                                               TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto) {
  const auto names = cross_validate_default_feature_names_(x, "selection::tune_specs(x,y)");
  const LabeledBatch batch = cross_validate_labeled_from_xy_(x, y, names, "selection::tune_specs(x,y)");
  return tune_specs(candidates, batch, split, train_options, assess_options, objective_metric, objective_policy);
}

[[nodiscard]] inline SpecTuneResult tune_specs(pluss::msharp::RangeView::Of<ModelSpec> candidates,
                                               const pluss::table::DataFrame& x,
                                               const Series& y,
                                               const TuneOptions& tune_options,
                                               const TrainOptions& train_options = {},
                                               const AssessOptions& assess_options = {},
                                               MetricKind objective_metric = MetricKind::Accuracy,
                                               TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto) {
  const auto names = cross_validate_default_feature_names_(x, "selection::tune_specs(x,y)");
  const LabeledBatch batch = cross_validate_labeled_from_xy_(x, y, names, "selection::tune_specs(x,y)");
  return tune_specs(candidates, batch, tune_options, train_options, assess_options, objective_metric, objective_policy);
}

[[nodiscard]] inline SpecTuneResult tune_specs(pluss::msharp::RangeView::Of<ModelSpec> candidates,
                                               const pluss::table::DataFrame& x,
                                               const pluss::table::Thread& y,
                                               const SplitOptions& split = {},
                                               const TrainOptions& train_options = {},
                                               const AssessOptions& assess_options = {},
                                               MetricKind objective_metric = MetricKind::Accuracy,
                                               TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto) {
  return tune_specs(candidates,
                    x,
                    as_stratify_series(y),
                    split,
                    train_options,
                    assess_options,
                    objective_metric,
                    objective_policy);
}

[[nodiscard]] inline SpecTuneResult tune_specs(pluss::msharp::RangeView::Of<ModelSpec> candidates,
                                               const pluss::table::DataFrame& x,
                                               const pluss::table::Thread& y,
                                               const TuneOptions& tune_options,
                                               const TrainOptions& train_options = {},
                                               const AssessOptions& assess_options = {},
                                               MetricKind objective_metric = MetricKind::Accuracy,
                                               TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto) {
  return tune_specs(candidates,
                    x,
                    as_stratify_series(y),
                    tune_options,
                    train_options,
                    assess_options,
                    objective_metric,
                    objective_policy);
}

[[nodiscard]] inline SpecTuneResult probe_specs(const ProbeSpecSampler& sampler,
                                                std::int64_t trials,
                                                const LabeledBatch& batch,
                                                const SplitOptions& split = {},
                                                const TrainOptions& train_options = {},
                                                const AssessOptions& assess_options = {},
                                                MetricKind objective_metric = MetricKind::Accuracy,
                                                TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                                RandomState random = {}) {
  auto report = resolve(::pluss::ai::nodej::probe(sampler,
                                                  trials,
                                                  batch,
                                                  split,
                                                  train_options,
                                                  assess_options,
                                                  objective_metric,
                                                  objective_policy,
                                                  random),
                        "selection::probe_specs.probe");
  if (report.best_index < 0 || static_cast<std::size_t>(report.best_index) >= report.trials.size()) {
    ::pluss::throw_error("selection::probe_specs: best_index is out of range.");
  }
  return SpecTuneResult{
      .report = report,
      .best_spec = report.trials[static_cast<std::size_t>(report.best_index)].spec,
  };
}

[[nodiscard]] inline SpecTuneResult probe_specs(const ProbeSpecSampler& sampler,
                                                const LabeledBatch& batch,
                                                const TuneOptions& tune_options,
                                                const TrainOptions& train_options = {},
                                                const AssessOptions& assess_options = {},
                                                MetricKind objective_metric = MetricKind::Accuracy,
                                                TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto) {
  auto report = resolve(::pluss::ai::nodej::tune(sampler,
                                                 batch,
                                                 tune_options,
                                                 train_options,
                                                 assess_options,
                                                 objective_metric,
                                                 objective_policy),
                        "selection::probe_specs.tune");
  if (report.best_index < 0 || static_cast<std::size_t>(report.best_index) >= report.trials.size()) {
    ::pluss::throw_error("selection::probe_specs: best_index is out of range.");
  }
  return SpecTuneResult{
      .report = report,
      .best_spec = report.trials[static_cast<std::size_t>(report.best_index)].spec,
  };
}

[[nodiscard]] inline SpecTuneResult probe_specs(const ProbeSpecSampler& sampler,
                                                std::int64_t trials,
                                                const pluss::table::DataFrame& x,
                                                const Series& y,
                                                const SplitOptions& split = {},
                                                const TrainOptions& train_options = {},
                                                const AssessOptions& assess_options = {},
                                                MetricKind objective_metric = MetricKind::Accuracy,
                                                TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                                RandomState random = {}) {
  const auto names = cross_validate_default_feature_names_(x, "selection::probe_specs(x,y)");
  const LabeledBatch batch = cross_validate_labeled_from_xy_(x, y, names, "selection::probe_specs(x,y)");
  return probe_specs(sampler,
                     trials,
                     batch,
                     split,
                     train_options,
                     assess_options,
                     objective_metric,
                     objective_policy,
                     random);
}

[[nodiscard]] inline SpecTuneResult probe_specs(const ProbeSpecSampler& sampler,
                                                const pluss::table::DataFrame& x,
                                                const Series& y,
                                                const TuneOptions& tune_options,
                                                const TrainOptions& train_options = {},
                                                const AssessOptions& assess_options = {},
                                                MetricKind objective_metric = MetricKind::Accuracy,
                                                TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto) {
  const auto names = cross_validate_default_feature_names_(x, "selection::probe_specs(x,y)");
  const LabeledBatch batch = cross_validate_labeled_from_xy_(x, y, names, "selection::probe_specs(x,y)");
  return probe_specs(sampler,
                     batch,
                     tune_options,
                     train_options,
                     assess_options,
                     objective_metric,
                     objective_policy);
}

[[nodiscard]] inline SpecTuneResult probe_specs(const ProbeSpecSampler& sampler,
                                                std::int64_t trials,
                                                const pluss::table::DataFrame& x,
                                                const pluss::table::Thread& y,
                                                const SplitOptions& split = {},
                                                const TrainOptions& train_options = {},
                                                const AssessOptions& assess_options = {},
                                                MetricKind objective_metric = MetricKind::Accuracy,
                                                TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                                RandomState random = {}) {
  return probe_specs(sampler,
                     trials,
                     x,
                     as_stratify_series(y),
                     split,
                     train_options,
                     assess_options,
                     objective_metric,
                     objective_policy,
                     random);
}

[[nodiscard]] inline SpecTuneResult probe_specs(const ProbeSpecSampler& sampler,
                                                const pluss::table::DataFrame& x,
                                                const pluss::table::Thread& y,
                                                const TuneOptions& tune_options,
                                                const TrainOptions& train_options = {},
                                                const AssessOptions& assess_options = {},
                                                MetricKind objective_metric = MetricKind::Accuracy,
                                                TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto) {
  return probe_specs(sampler,
                     x,
                     as_stratify_series(y),
                     tune_options,
                     train_options,
                     assess_options,
                     objective_metric,
                     objective_policy);
}

// ---------------------------------------------------------------------------
// Genuine hyperparameter tuning.
//
// Everything above this point (compare, tune_specs, probe_specs) ranks
// candidates the caller already built one at a time. Nothing in this file so
// far turns "learning_rate in {0.01, 0.05, 0.1}, max_depth in {3, 4, 5}" into
// the 9 ModelSpecs that describes -- that expansion is the actual missing
// piece, not a new cross-validation engine (probe_specs's sampler dispatch
// already is one, with the same parallel-trial shape sweep/compare use).
// GridAxis/GridPoint/tune_specs are that missing spec-level front end.
// ---------------------------------------------------------------------------

// One axis of a search space: a name (for GridPoint::at(...) lookup and for
// labeling trials) and the discrete values to sweep over it. Continuous
// ranges aren't sampled directly -- list however many points along the
// range are worth trying. A deliberate v1 scope cut: see the blueprint for
// why, and what a continuous-distribution axis would need on top of this.
struct GridAxis {
  std::string name{};
  std::vector<double> values{};
};

// One point in an expanded search space, with both positional and
// name-keyed access -- passed to a search's `make_spec` factory so its body
// can read `point.at("learning_rate")` instead of juggling a bare index
// against a separate axis list.
class GridPoint {
 public:
  GridPoint(const std::vector<GridAxis>& axes, std::vector<double> values)
      : axes_(&axes), values_(std::move(values)) {}

  [[nodiscard]] double at(std::string_view axis_name) const {
    for (std::size_t i = 0; i < axes_->size(); ++i) {
      if ((*axes_)[i].name == axis_name) return values_[i];
    }
    ::pluss::throw_error("selection::GridPoint::at: unknown axis \"" + std::string(axis_name) + "\".");
  }

  [[nodiscard]] double operator[](std::size_t index) const { return values_.at(index); }
  [[nodiscard]] std::size_t size() const noexcept { return values_.size(); }

  // "learning_rate=0.05, max_depth=4" -- the auto-generated trial name, so
  // compare()-style .board() output stays readable without the caller
  // having to name every expanded point themselves.
  [[nodiscard]] std::string label() const {
    std::string out{};
    for (std::size_t i = 0; i < axes_->size(); ++i) {
      if (i > 0) out += ", ";
      out += (*axes_)[i].name + "=" + std::to_string(values_[i]);
    }
    return out;
  }

 private:
  const std::vector<GridAxis>* axes_;
  std::vector<double> values_;
};

// Total point count across every axis -- the product every axis's size
// multiplies into, the same one ParameterGrid.__len__ computes for a dict of
// value lists. Exposed on its own so a caller can sanity-check a search
// space's size before committing to an exhaustive sweep of it.
[[nodiscard]] inline std::int64_t grid_size(const std::vector<GridAxis>& axes) {
  std::int64_t total = 1;
  for (const auto& axis : axes) {
    if (axis.values.empty()) return 0;
    total *= static_cast<std::int64_t>(axis.values.size());
  }
  return total;
}

// Mixed-radix decode: point `index`'s value on axis `a` is
// `axes[a].values[(index / stride_a) % axes[a].values.size()]`, where
// stride_a is the product of every earlier axis's size -- so the last axis
// varies fastest as `index` counts up, matching itertools.product's own
// iteration order for an equivalent nested-loop expansion. This computes one
// point directly, without ever materializing the other grid_size(axes) - 1
// points, which is what lets tune_specs(...) below draw a random handful of
// points from a space too large to enumerate in full.
[[nodiscard]] inline GridPoint grid_point_at(std::int64_t index, const std::vector<GridAxis>& axes) {
  std::vector<double> point(axes.size());
  std::int64_t stride = 1;
  for (std::size_t a = 0; a < axes.size(); ++a) {
    const auto n = static_cast<std::int64_t>(axes[a].values.size());
    point[a] = axes[a].values[static_cast<std::size_t>((index / stride) % n)];
    stride *= n;
  }
  return GridPoint(axes, std::move(point));
}

// Every point in the space, in the same cross-product order grid_point_at
// produces them one at a time. Exhaustive by construction -- only reach for
// this when every resulting candidate is actually going to be evaluated
// (the spec-level search wrapper below uses it internally for exactly that reason); a randomized
// search draws individual points via grid_point_at(...) instead, so it never
// pays to materialize points it won't use.
[[nodiscard]] inline std::vector<GridPoint> grid_points(const std::vector<GridAxis>& axes) {
  const std::int64_t total = grid_size(axes);
  std::vector<GridPoint> out{};
  out.reserve(static_cast<std::size_t>(total));
  for (std::int64_t i = 0; i < total; ++i) {
    out.push_back(grid_point_at(i, axes));
  }
  return out;
}

// Shuffle-and-take without-replacement index sample -- the same algorithm
// NodeJ's own sample_candidates_without_replacement (NodeJ/tune/tune.cpp)
// uses for its candidate-list random search, applied here to raw grid
// indices instead of already-built ModelSpecs so a huge space never needs
// materializing just to draw a small sample from it.
[[nodiscard]] inline std::vector<std::int64_t> grid_sample_indices_(std::int64_t population,
                                                                    std::int64_t sample_size,
                                                                    RandomState random) {
  std::vector<std::int64_t> indices(static_cast<std::size_t>(population));
  std::iota(indices.begin(), indices.end(), std::int64_t{0});
  std::mt19937_64 rng(random.deterministic ? random.seed : std::random_device{}());
  std::shuffle(indices.begin(), indices.end(), rng);
  const std::int64_t count = std::min(sample_size, population);
  indices.resize(static_cast<std::size_t>(count));
  std::sort(indices.begin(), indices.end());
  return indices;
}

// Shared body for tune_specs(...) below: builds a ProbeSpecSampler
// restricted to exactly the grid indices in `indices` (every index for an
// exhaustive search, a without-replacement subset for a sampled one) and
// hands off to probe_specs -- so this file still has exactly one parallel
// (trial, fold) dispatch engine, not a second one bolted on for grids.
// `make_spec` runs concurrently across trials the same way every other
// factory closure in this file already does (evaluate_cell above, sweep's
// and probe's own eval_trial); it must be safe to call from multiple threads
// at once, which holds automatically as long as it only reads `point` and
// builds a fresh ModelSpec value from it.
[[nodiscard]] inline SpecTuneResult tune_indices_(const std::vector<GridAxis>& axes,
                                                  const std::function<ModelSpec(const GridPoint&)>& make_spec,
                                                  std::vector<std::int64_t> indices,
                                                  const LabeledBatch& batch,
                                                  const SplitOptions& split,
                                                  const TrainOptions& train_options,
                                                  const AssessOptions& assess_options,
                                                  MetricKind objective_metric,
                                                  TuneObjectivePolicy objective_policy,
                                                  RandomState random) {
  const auto trials = static_cast<std::int64_t>(indices.size());
  const ProbeSpecSampler sampler = [&axes, &make_spec, &indices](std::int64_t trial_index,
                                                                 std::uint64_t) -> StatusOr<ModelSpec> {
    return StatusOr<ModelSpec>(make_spec(grid_point_at(indices[static_cast<std::size_t>(trial_index)], axes)));
  };
  return probe_specs(sampler, trials, batch, split, train_options, assess_options,
                     objective_metric, objective_policy, random);
}

// Genuine hyperparameter tuning: expands points of `axes` (the sorted-axis
// cross product grid_points/grid_point_at compute) into ModelSpecs via
// `make_spec`, then cross-validates them through the same parallel dispatch
// every other spec-level search in this file already uses. Distinct from
// compare(...) further up, which ranks candidates the caller has already
// fully built one at a time -- this one builds specs from axis definitions.
//
// `tune_options.search` picks the strategy, the same TuneOptions.search field
// tune_specs/probe_specs already dispatch on above:
//   - None/Grid -- evaluate every point of the cross product.
//   - Random    -- draw tune_options.trials distinct points without
//                  replacement (shuffle-and-take over grid_sample_indices_,
//                  seeded from tune_options.random); trials above
//                  grid_size(axes) just falls back to the exhaustive sweep,
//                  matching ParameterSampler's own
//                  n_iter = min(n_iter, grid_size) clamp. This used to be a
//                  separate public sampled-search entry point; it is now an
//                  internal branch here instead, reached only through
//                  SearchKind::Random.
//   - Bayesian/SuccessiveHalving -- not wired through this GridAxis front end
//     yet (see LYMPH_BLUEPRINT.md Sec 8/9); call probe_specs directly with a
//     custom sampler if you need those today.
[[nodiscard]] inline SpecTuneResult tune_specs(const std::vector<GridAxis>& axes,
                                               const std::function<ModelSpec(const GridPoint&)>& make_spec,
                                               const LabeledBatch& batch,
                                               const TuneOptions& tune_options = {},
                                               const TrainOptions& train_options = {},
                                               const AssessOptions& assess_options = {},
                                               MetricKind objective_metric = MetricKind::Accuracy,
                                               TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto) {
  const std::int64_t total = grid_size(axes);
  if (total <= 0) {
    ::pluss::throw_error("selection::tune_specs: search space is empty (an axis has zero values).");
  }
  if (tune_options.search == SearchKind::Random) {
    if (tune_options.trials <= 0) {
      ::pluss::throw_error("selection::tune_specs: SearchKind::Random requires TuneOptions.trials > 0.");
    }
    return tune_indices_(axes, make_spec, grid_sample_indices_(total, tune_options.trials, tune_options.random),
                         batch, tune_options.split, train_options, assess_options, objective_metric,
                         objective_policy, tune_options.random);
  }
  if (tune_options.search == SearchKind::None || tune_options.search == SearchKind::Grid) {
    std::vector<std::int64_t> indices(static_cast<std::size_t>(total));
    std::iota(indices.begin(), indices.end(), std::int64_t{0});
    return tune_indices_(axes, make_spec, std::move(indices), batch, tune_options.split, train_options,
                         assess_options, objective_metric, objective_policy, tune_options.random);
  }
  ::pluss::throw_error(
      "selection::tune_specs: only SearchKind::Grid (or None) and SearchKind::Random are supported here -- "
      "call probe_specs directly with a custom sampler for Bayesian/SuccessiveHalving.");
}

[[nodiscard]] inline SpecTuneResult tune_specs(const std::vector<GridAxis>& axes,
                                               const std::function<ModelSpec(const GridPoint&)>& make_spec,
                                               const pluss::table::DataFrame& x,
                                               const Series& y,
                                               const TuneOptions& tune_options = {},
                                               const TrainOptions& train_options = {},
                                               const AssessOptions& assess_options = {},
                                               MetricKind objective_metric = MetricKind::Accuracy,
                                               TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto) {
  const auto names = cross_validate_default_feature_names_(x, "selection::tune_specs(x,y)");
  const LabeledBatch batch = cross_validate_labeled_from_xy_(x, y, names, "selection::tune_specs(x,y)");
  return tune_specs(axes, make_spec, batch, tune_options, train_options, assess_options, objective_metric,
                    objective_policy);
}

[[nodiscard]] inline SpecTuneResult tune_specs(const std::vector<GridAxis>& axes,
                                               const std::function<ModelSpec(const GridPoint&)>& make_spec,
                                               const pluss::table::DataFrame& x,
                                               const pluss::table::Thread& y,
                                               const TuneOptions& tune_options = {},
                                               const TrainOptions& train_options = {},
                                               const AssessOptions& assess_options = {},
                                               MetricKind objective_metric = MetricKind::Accuracy,
                                               TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto) {
  return tune_specs(axes, make_spec, x, as_stratify_series(y), tune_options, train_options, assess_options,
                    objective_metric, objective_policy);
}

// (frame, DataFrameBatchOptions) sibling -- see the report-builder route in
// detail/model_core.h for why this exists alongside the (x, y) form.
[[nodiscard]] inline SpecTuneResult tune_specs(const std::vector<GridAxis>& axes,
                                               const std::function<ModelSpec(const GridPoint&)>& make_spec,
                                               const pluss::table::DataFrame& frame,
                                               const DataFrameBatchOptions& data_options,
                                               const TuneOptions& tune_options = {},
                                               const TrainOptions& train_options = {},
                                               const AssessOptions& assess_options = {},
                                               MetricKind objective_metric = MetricKind::Accuracy,
                                               TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto) {
  const LabeledBatch batch = resolve(labeled_batch_from_dataframe(frame, data_options),
                                     "selection::tune_specs(frame,data_options).labeled_batch_from_dataframe");
  return tune_specs(axes, make_spec, batch, tune_options, train_options, assess_options, objective_metric,
                    objective_policy);
}

} // namespace selection


} // namespace pluss::ai::lymph

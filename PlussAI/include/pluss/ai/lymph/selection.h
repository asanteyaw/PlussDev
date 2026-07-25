#pragma once

#include "pluss/ai/lymph/common.h"
#include "pluss/ai/lymph/metrics.h"
#include "pluss/ai/lymph/workflow.h"

namespace pluss::ai::lymph {

namespace selection {

struct Candidate {
  std::string name{};
  Workflow workflow{};

  Candidate(std::string in_name, Workflow in_workflow)
      : name(std::move(in_name)), workflow(std::move(in_workflow)) {}
  Candidate(Candidate&&) noexcept = default;
  Candidate& operator=(Candidate&&) noexcept = default;
  Candidate(const Candidate&) = delete;
  Candidate& operator=(const Candidate&) = delete;
};

struct Trial {
  std::string name{};
  double objective = std::numeric_limits<double>::quiet_NaN();
  CrossValidationReport cv{};
};

struct TuneReport {
  std::vector<Trial> trials{};
  std::int64_t best_index = -1;
  std::string best_name{};
  MetricKind objective_metric = MetricKind::Accuracy;
  TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto;
  std::vector<std::int64_t> objective_rank{};
  std::vector<std::int64_t> ranked_indices{};
};

struct TuneResult {
  TuneReport report{};
  Workflow best_workflow{};
  std::vector<Workflow> fitted{};

  [[nodiscard]] const Workflow& best() const noexcept { return best_workflow; }
  [[nodiscard]] Workflow& best() noexcept { return best_workflow; }

  [[nodiscard]] std::vector<std::reference_wrapper<Workflow>> topk(std::int64_t k) {
    if (k < 0) {
      ::pluss::throw_error("selection::TuneResult::topk: k must be non-negative.");
    }
    if (k > static_cast<std::int64_t>(fitted.size())) {
      ::pluss::throw_error("selection::TuneResult::topk: requested k exceeds fitted model count; increase FitOptions.fit_top.");
    }
    std::vector<std::reference_wrapper<Workflow>> out{};
    out.reserve(static_cast<std::size_t>(k));
    for (std::int64_t i = 0; i < k; ++i) {
      out.emplace_back(fitted[static_cast<std::size_t>(i)]);
    }
    return out;
  }

  [[nodiscard]] std::vector<std::reference_wrapper<const Workflow>> topk(std::int64_t k) const {
    if (k < 0) {
      ::pluss::throw_error("selection::TuneResult::topk: k must be non-negative.");
    }
    if (k > static_cast<std::int64_t>(fitted.size())) {
      ::pluss::throw_error("selection::TuneResult::topk: requested k exceeds fitted model count; increase FitOptions.fit_top.");
    }
    std::vector<std::reference_wrapper<const Workflow>> out{};
    out.reserve(static_cast<std::size_t>(k));
    for (std::int64_t i = 0; i < k; ++i) {
      out.emplace_back(fitted[static_cast<std::size_t>(i)]);
    }
    return out;
  }
};

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
  ::pluss::throw_error("selection::tune: objective metric not present in assessment metrics.");
}

inline void assign_objective_ranks_(TuneReport& report) {
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

[[nodiscard]] inline TuneResult tune(std::vector<Candidate> candidates,
                                     const pluss::table::DataFrame& x,
                                     const Series& y,
                                     CrossValidationOptions cv = {},
                                     TrainOptions train_options = {},
                                     AssessOptions assess_options = {},
                                     MetricKind objective_metric = MetricKind::Accuracy,
                                     TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                     FitOptions fit_options = {}) {
  if (candidates.empty()) {
    ::pluss::throw_error("selection::tune: at least one candidate is required.");
  }
  if (fit_options.fit_top < 0) {
    ::pluss::throw_error("selection::tune: FitOptions.fit_top must be non-negative.");
  }
  if (assess_options.metrics.empty()) {
    assess_options.metrics = {objective_metric};
  } else if (std::find(assess_options.metrics.begin(), assess_options.metrics.end(), objective_metric) == assess_options.metrics.end()) {
    assess_options.metrics.push_back(objective_metric);
  }

  const bool minimize = minimize_metric_(objective_metric, objective_policy);
  TuneResult result{};
  result.report.objective_metric = objective_metric;
  result.report.objective_policy = objective_policy;
  result.report.trials.reserve(candidates.size());

  const auto feature_names = cross_validate_default_feature_names_(x, "selection::tune");
  const LabeledBatch batch = cross_validate_labeled_from_xy_(x, y, feature_names, "selection::tune");
  const TaskKind task = candidates.front().workflow.task_kind();
  for (std::size_t i = 1; i < candidates.size(); ++i) {
    if (candidates[i].workflow.task_kind() != task) {
      ::pluss::throw_error("selection::tune: all workflow candidates must have the same task kind.");
    }
  }
  const SplitOptions split = effective_workflow_split_options_(cv, task);
  const SplitPlan plan = workflow_split_plan_(batch, split, task, "selection::tune");
  const std::vector<FoldBatchPair> fold_batches = materialize_fold_batches_(batch, plan, "selection::tune");

  std::int64_t best_index = -1;
  double best_objective = std::numeric_limits<double>::quiet_NaN();

  for (std::size_t i = 0; i < candidates.size(); ++i) {
    auto cv_report = cross_validate_batches_(candidates[i].workflow.workflow_model(),
                                             fold_batches,
                                             plan,
                                             split.max_parallel,
                                             train_options,
                                             assess_options,
                                             "selection::tune");
    const double objective = scalar_metric_(cv_report.mean, objective_metric);
    result.report.trials.push_back(Trial{
        .name = candidates[i].name,
        .objective = objective,
        .cv = std::move(cv_report),
    });

    if (best_index < 0) {
      best_index = static_cast<std::int64_t>(i);
      best_objective = objective;
      continue;
    }
    const bool better = minimize ? (objective < best_objective) : (objective > best_objective);
    if (better) {
      best_index = static_cast<std::int64_t>(i);
      best_objective = objective;
    }
  }

  result.report.best_index = best_index;
  result.report.best_name = candidates[static_cast<std::size_t>(best_index)].name;
  assign_objective_ranks_(result.report);
  if (result.report.ranked_indices.empty()) {
    ::pluss::throw_error("selection::tune: no finite objective candidate was available to fit.");
  }

  if (fit_options.fit_top > 0) {
    const std::int64_t fit_count =
        std::min<std::int64_t>(fit_options.fit_top, static_cast<std::int64_t>(result.report.ranked_indices.size()));
    result.fitted.reserve(static_cast<std::size_t>(fit_count));
    for (std::int64_t i = 0; i < fit_count; ++i) {
      const std::int64_t candidate_index = result.report.ranked_indices[static_cast<std::size_t>(i)];
      Workflow fitted = candidates[static_cast<std::size_t>(candidate_index)].workflow.learn(batch, train_options);
      result.fitted.push_back(std::move(fitted));
    }
    // NOTE: ranked_indices.front() == best_index here (assign_objective_ranks_
    // sorts with the same minimize/maximize comparator used to pick best_index
    // above, both breaking ties by keeping the earliest candidate), so
    // result.fitted[0] is already a trained copy of the best candidate. This
    // retrain is therefore redundant — but Workflow/WorkflowModel are
    // deliberately move-only (no copy constructor/assignment), so reusing
    // fitted[0] here isn't a free reference/copy; it would need a real clone
    // path (WorkflowModel already has one internally as
    // clone_for_out_of_place_(), which dumps+reloads trained state instead of
    // retraining, but it isn't exposed publicly). Left as a known, scoped
    // follow-up rather than adding new public clone API in this pass.
    result.best_workflow = candidates[static_cast<std::size_t>(best_index)].workflow.learn(batch, train_options);
    return result;
  }

  result.best_workflow = candidates[static_cast<std::size_t>(best_index)].workflow.learn(batch, train_options);
  return result;
}

[[nodiscard]] inline TuneResult tune(std::vector<Candidate> candidates,
                                     const pluss::table::DataFrame& x,
                                     const pluss::table::Thread& y,
                                     CrossValidationOptions cv = {},
                                     TrainOptions train_options = {},
                                     AssessOptions assess_options = {},
                                     MetricKind objective_metric = MetricKind::Accuracy,
                                     TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                     FitOptions fit_options = {}) {
  return tune(std::move(candidates),
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

} // namespace selection


} // namespace pluss::ai::lymph

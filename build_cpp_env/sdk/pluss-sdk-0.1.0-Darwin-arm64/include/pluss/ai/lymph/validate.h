#pragma once

#include "pluss/ai/lymph/builder.h"
#include "pluss/ai/lymph/model.h"
#include "pluss/ai/lymph/payload.h"
#include "pluss/ai/lymph/workflow.h"

#include <numeric>
#include <utility>
#include <vector>

namespace pluss::ai::lymph::selection::validate {

namespace detail {

[[nodiscard]] inline bool is_time_plan_(const TimeValOptions& options) noexcept {
  return options.plan == CVPlanKind::TSCrossVal;
}

inline void require_time_safe_payload_(const TimeValOptions& options,
                                       const workflow::Payload& payload,
                                       const char* where) {
  if (!is_time_plan_(options)) return;
  if (options.shuffle) {
    ::pluss::throw_error(std::string(where) + ": time validation cannot shuffle rows.");
  }
  if (!payload.has_time()) {
    ::pluss::throw_error(std::string(where) +
                         ": time validation requires Payload::time(...) so temporal intent is explicit.");
  }
  const std::string time_feature = payload.time_feature();
  if (payload.has_frame() && !time_feature.empty() && !payload.frame().get(time_feature).has_value()) {
    ::pluss::throw_error(std::string(where) + ": time feature \"" + time_feature + "\" is not present in the payload frame.");
  }
}

inline void require_workflow_time_compatible_(const TimeValOptions& options,
                                              const workflow::Workflow& workflow,
                                              const workflow::Payload& payload,
                                              const char* where) {
  if (workflow.time_aware() && !is_time_plan_(options)) {
    ::pluss::throw_error(std::string(where) +
                         ": time-aware Workflow must be evaluated with validate::time() / TSCrossVal.");
  }
  if (is_time_plan_(options)) {
    require_time_safe_payload_(options, payload, where);
    if (!workflow.time_aware()) return;
    const workflow::TimeOptions& wf_time = *workflow.time_options();
    const std::string payload_time = payload.time_feature();
    if (has_time_index(wf_time) && !payload_time.empty() && wf_time.index != payload_time) {
      ::pluss::throw_error(std::string(where) + ": Workflow time index \"" + wf_time.index +
                           "\" does not match Payload time feature \"" + payload_time + "\".");
    }
  }
}

[[nodiscard]] inline TimeValOptions effective_time_options_(TimeValOptions options,
                                                            const workflow::Workflow& workflow) {
  if (!is_time_plan_(options) || !workflow.time_aware()) return options;
  const workflow::TimeOptions& time = *workflow.time_options();
  if (options.time_gap == 0) options.time_gap = time.gap;
  if (options.time_test_size == 0) options.time_test_size = time.horizon;
  if (options.time_max_train_size == 0) options.time_max_train_size = time.lookback;
  options.shuffle = false;
  return options;
}

[[nodiscard]] inline std::int64_t metric_index_(pluss::msharp::RangeView::Of<MetricKind> metrics,
                                                MetricKind metric,
                                                const char* where) {
  for (std::int64_t i = 0; i < metrics.size(); ++i) {
    if (metrics[static_cast<std::size_t>(i)] == metric) return i;
  }
  ::pluss::throw_error(std::string(where) + ": requested metric was not present in the validation report.");
}

[[nodiscard]] inline pluss::tils::transmit_options scorer_transmit_options_(const SplitOptions& split,
                                                                           std::size_t fold_n) {
  pluss::tils::transmit_options topts{};
  topts.grain_size = 1;
  if (split.max_parallel == 1 || fold_n <= 1) {
    topts.exec = pluss::tils::transmit_exec::seq;
  } else {
    topts.exec = pluss::tils::transmit_exec::par;
    topts.workers = split.max_parallel > 0 ? static_cast<std::size_t>(split.max_parallel) : 0;
  }
  return topts;
}

[[nodiscard]] inline Series scorer_values_series_(const std::vector<double>& values) {
  return pluss::make_series(values, {static_cast<std::int64_t>(values.size())},
                            Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU));
}

// Per-fold values of a custom selection::Scorer for one model under `cv`.
// Mirrors selection::compare_batch_'s inner loop (train the fold, infer, call
// the scorer on the held-out `(truth, prediction)`) minus the ranking -- the
// built-in `MetricKind` path still runs through `Plan::run` + fold_metric_values_.
// One extra `infer` per fold, over the same fold split `Plan::run` would use.
//
// Bare-LabeledBatch form: only valid for a model with no appended prep stages
// (a nameless batch cannot route a ColumnRouter). The frame overload below
// keeps column names for staged models.
[[nodiscard]] inline Series custom_scorer_fold_values_(const models::Model& model,
                                                       const LabeledBatch& batch,
                                                       const CrossValOptions& cv,
                                                       const selection::Scorer& objective,
                                                       TrainOptions train_options,
                                                       const char* where) {
  const TaskKind task = model.task_kind();
  const SplitOptions split = ::pluss::ai::lymph::detail::effective_workflow_split_options_(cv, task);
  const SplitPlan plan = ::pluss::ai::lymph::detail::workflow_split_plan_(batch, split, task, where);
  const std::vector<metrics::FoldBatchPair> fold_batches =
      ::pluss::ai::lymph::detail::materialize_fold_batches_(batch, plan, where);
  const std::size_t fold_n = fold_batches.size();
  if (fold_n == 0) {
    ::pluss::throw_error(std::string(where) + ": split plan produced no folds.");
  }

  std::vector<std::size_t> fold_indices(fold_n);
  std::iota(fold_indices.begin(), fold_indices.end(), std::size_t{0});
  const auto evaluate_fold = [&](std::size_t fi) -> double {
    const models::Model trained = model.learn(fold_batches[fi].train, train_options);
    const Series prediction = trained.infer(fold_batches[fi].test.features);
    return objective.fn(fold_batches[fi].test.y, prediction);
  };
  const std::vector<double> values =
      pluss::tils::transmit(evaluate_fold, fold_indices, scorer_transmit_options_(split, fold_n));
  return scorer_values_series_(values);
}

// (frame, DataFrameBatchOptions) form: materializes DataFrame folds so a staged
// model's prep stages keep the column names they route against.
[[nodiscard]] inline Series custom_scorer_fold_values_(const models::Model& model,
                                                       const pluss::table::DataFrame& frame,
                                                       const DataFrameBatchOptions& data_options,
                                                       const CrossValOptions& cv,
                                                       const selection::Scorer& objective,
                                                       TrainOptions train_options,
                                                       const char* where) {
  const LabeledBatch plan_batch = resolve(labeled_batch_from_dataframe(frame, data_options), where);
  const TaskKind task = model.task_kind();
  const SplitOptions split = ::pluss::ai::lymph::detail::effective_workflow_split_options_(cv, task);
  const SplitPlan plan = ::pluss::ai::lymph::detail::workflow_split_plan_(plan_batch, split, task, where);
  const std::vector<selection::FoldFramePair_> fold_frames = selection::materialize_fold_frames_(frame, plan);
  const std::size_t fold_n = fold_frames.size();
  if (fold_n == 0) {
    ::pluss::throw_error(std::string(where) + ": split plan produced no folds.");
  }

  std::vector<std::size_t> fold_indices(fold_n);
  std::iota(fold_indices.begin(), fold_indices.end(), std::size_t{0});
  const auto evaluate_fold = [&](std::size_t fi) -> double {
    const models::Model trained = model.learn(fold_frames[fi].train, data_options, train_options);
    const Series prediction = trained.infer(fold_frames[fi].test, data_options);
    const Series y_test =
        resolve(labeled_batch_from_dataframe(fold_frames[fi].test, data_options), where).y;
    return objective.fn(y_test, prediction);
  };
  const std::vector<double> values =
      pluss::tils::transmit(evaluate_fold, fold_indices, scorer_transmit_options_(split, fold_n));
  return scorer_values_series_(values);
}

[[nodiscard]] inline Series fold_metric_values_(const metrics::CrossValidationReport& report,
                                                MetricKind metric,
                                                const char* where) {
  if (report.report.folds.empty()) {
    ::pluss::throw_error(std::string(where) + ": validation report has no folds.");
  }
  const std::int64_t index = metric_index_(report.report.metrics, metric, where);
  const std::int64_t metric_count = static_cast<std::int64_t>(report.report.metrics.size());
  const std::int64_t fold_count = static_cast<std::int64_t>(report.report.folds.size());
  Series fold_matrix = report.report.folds.front().metrics.to(ScalarType::Float64).reshape({1, metric_count});
  for (std::size_t i = 1; i < report.report.folds.size(); ++i) {
    fold_matrix = fold_matrix.unite({report.report.folds[i].metrics.to(ScalarType::Float64).reshape({1, metric_count})}, 0);
  }
  return fold_matrix.reshape({fold_count, metric_count}).narrow_copy(1, index, 1).reshape({fold_count});
}

} // namespace detail

class Plan final {
 public:
  explicit Plan(CrossValOptions options = {}) : options_(std::move(options)) {}

  [[nodiscard]] const CrossValOptions& options() const noexcept { return options_; }

  Plan& folds(std::int64_t value) {
    if (value <= 1) {
      ::pluss::throw_error("validate::Plan::folds: value must be greater than one.");
    }
    options_.folds = value;
    return *this;
  }

  Plan& repeat(std::int64_t value) {
    if (value <= 0) {
      ::pluss::throw_error("validate::Plan::repeat: value must be positive.");
    }
    options_.repeats = value;
    return *this;
  }

  Plan& shuffle(bool value = true) noexcept {
    options_.shuffle = value;
    return *this;
  }

  Plan& seed(std::uint64_t value) noexcept {
    options_.random_state = value;
    options_.deterministic = true;
    return *this;
  }

  Plan& workers(std::int64_t value) noexcept {
    options_.max_parallel = value;
    return *this;
  }

  Plan& gap(std::int64_t value) {
    if (value < 0) {
      ::pluss::throw_error("validate::Plan::gap: value cannot be negative.");
    }
    options_.time_gap = value;
    return *this;
  }

  Plan& horizon(std::int64_t value) {
    if (value < 0) {
      ::pluss::throw_error("validate::Plan::horizon: value cannot be negative.");
    }
    options_.time_test_size = value;
    return *this;
  }

  Plan& lookback(std::int64_t value) {
    if (value < 0) {
      ::pluss::throw_error("validate::Plan::lookback: value cannot be negative.");
    }
    options_.time_max_train_size = value;
    return *this;
  }

  [[nodiscard]] metrics::CrossValidationReport run(const ModelSpec& spec,
                                                   const LabeledBatch& batch,
                                                   TrainOptions train_options = {},
                                                   AssessOptions assess_options = {}) const {
    return metrics::detail::cross_validate(spec, batch, options_,
                                           std::move(train_options), std::move(assess_options));
  }

  [[nodiscard]] metrics::CrossValidationReport run(const ModelSpec& spec,
                                                   const pluss::table::DataFrame& frame,
                                                   const DataFrameBatchOptions& data_options,
                                                   TrainOptions train_options = {},
                                                   AssessOptions assess_options = {}) const {
    return metrics::detail::cross_validate(spec, frame, data_options, options_,
                                           std::move(train_options), std::move(assess_options));
  }

  [[nodiscard]] metrics::CrossValidationReport run(const ModelSpec& spec,
                                                   const pluss::table::DataFrame& x,
                                                   const Series& y,
                                                   TrainOptions train_options = {},
                                                   AssessOptions assess_options = {}) const {
    return metrics::detail::cross_validate(spec, x, y, options_,
                                           std::move(train_options), std::move(assess_options));
  }

  [[nodiscard]] metrics::CrossValidationReport run(const ModelSpec& spec,
                                                   const pluss::table::DataFrame& x,
                                                   const pluss::table::Thread& y,
                                                   TrainOptions train_options = {},
                                                   AssessOptions assess_options = {}) const {
    return run(spec, x, as_stratify_series(y), std::move(train_options), std::move(assess_options));
  }

  [[nodiscard]] metrics::CrossValidationReport run(const models::Model& model,
                                                   const LabeledBatch& batch,
                                                   TrainOptions train_options = {},
                                                   AssessOptions assess_options = {}) const {
    return ::pluss::ai::lymph::detail::cross_validate(
        ::pluss::ai::lymph::detail::model_core(model), batch, options_,
        std::move(train_options), std::move(assess_options));
  }

  [[nodiscard]] metrics::CrossValidationReport run(const models::Model& model,
                                                   const pluss::table::DataFrame& frame,
                                                   const DataFrameBatchOptions& data_options,
                                                   TrainOptions train_options = {},
                                                   AssessOptions assess_options = {}) const {
    return ::pluss::ai::lymph::detail::cross_validate(
        ::pluss::ai::lymph::detail::model_core(model), frame, data_options, options_,
        std::move(train_options), std::move(assess_options));
  }

  [[nodiscard]] metrics::CrossValidationReport run(const models::Model& model,
                                                   const pluss::table::DataFrame& x,
                                                   const Series& y,
                                                   TrainOptions train_options = {},
                                                   AssessOptions assess_options = {}) const {
    return ::pluss::ai::lymph::detail::cross_validate(
        ::pluss::ai::lymph::detail::model_core(model), x, y, options_,
        std::move(train_options), std::move(assess_options));
  }

  [[nodiscard]] metrics::CrossValidationReport run(const models::Model& model,
                                                   const pluss::table::DataFrame& x,
                                                   const pluss::table::Thread& y,
                                                   TrainOptions train_options = {},
                                                   AssessOptions assess_options = {}) const {
    return run(model, x, as_stratify_series(y), std::move(train_options), std::move(assess_options));
  }

  [[nodiscard]] metrics::CrossValidationReport run(const models::Model& model,
                                                   const workflow::Payload& payload,
                                                   TrainOptions train_options = {},
                                                   AssessOptions assess_options = {}) const {
    detail::require_time_safe_payload_(options_, payload, "validate::Plan::run(model,payload)");
    if (payload.has_frame()) {
      if (payload.has_external_weights() && !payload.has_weight_feature()) {
        ::pluss::throw_error(
            "validate::Plan::run(model,payload): DataFrame-backed Payload with Series weights must use weights(\"column\") so row filtering stays aligned.");
      }
      if (payload.has_label_feature()) {
        return run(model, payload.frame(), payload.data(), std::move(train_options), std::move(assess_options));
      }
      if (payload.has_label()) {
        return run(model, payload.frame(), payload.label(), std::move(train_options), std::move(assess_options));
      }
    }
    return run(model, payload.labeled_batch(), std::move(train_options), std::move(assess_options));
  }

  [[nodiscard]] metrics::CrossValidationReport run(const workflow::Workflow& workflow,
                                                   const workflow::Payload& payload,
                                                   TrainOptions train_options = {},
                                                   AssessOptions assess_options = {}) const {
    detail::require_workflow_time_compatible_(options_, workflow, payload, "validate::Plan::run(workflow,payload)");
    const TimeValOptions effective_options = detail::effective_time_options_(options_, workflow);
    const ::pluss::ai::lymph::detail::WorkflowLabeledInput input =
        ::pluss::ai::lymph::detail::workflow_labeled_input_(workflow, payload, "validate::Plan::run(workflow,payload)");
    if (input.frame.has_value()) {
      return ::pluss::ai::lymph::detail::cross_validate(
          ::pluss::ai::lymph::detail::model_core(workflow.model()), *input.frame, input.data_options, effective_options,
          std::move(train_options), std::move(assess_options));
    }
    return ::pluss::ai::lymph::detail::cross_validate(
        ::pluss::ai::lymph::detail::model_core(workflow.model()), *input.batch, effective_options,
        std::move(train_options), std::move(assess_options));
  }

  [[nodiscard]] selection::CompareResult run(const models::Builder& builder,
                                             const LabeledBatch& batch,
                                             TrainOptions train_options = {},
                                             AssessOptions assess_options = {},
                                             selection::Scorer objective = MetricKind::Accuracy,
                                             TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                             selection::FitOptions fit_options = {}) const {
    return builder.benchmark(batch, options_, std::move(train_options), std::move(assess_options),
                             objective, objective_policy, fit_options);
  }

  [[nodiscard]] selection::CompareResult run(const models::Builder& builder,
                                             const pluss::table::DataFrame& frame,
                                             const DataFrameBatchOptions& data_options,
                                             TrainOptions train_options = {},
                                             AssessOptions assess_options = {},
                                             selection::Scorer objective = MetricKind::Accuracy,
                                             TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                             selection::FitOptions fit_options = {}) const {
    return builder.benchmark(frame, data_options, options_, std::move(train_options), std::move(assess_options),
                             objective, objective_policy, fit_options);
  }

  [[nodiscard]] selection::CompareResult run(const models::Builder& builder,
                                             const workflow::Payload& payload,
                                             TrainOptions train_options = {},
                                             AssessOptions assess_options = {},
                                             selection::Scorer objective = MetricKind::Accuracy,
                                             TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                             selection::FitOptions fit_options = {}) const {
    detail::require_time_safe_payload_(options_, payload, "validate::Plan::run(builder,payload)");
    if (payload.has_frame()) {
      if (payload.has_external_weights() && !payload.has_weight_feature()) {
        ::pluss::throw_error(
            "validate::Plan::run(builder,payload): DataFrame-backed Payload with Series weights must use weights(\"column\") so row filtering stays aligned.");
      }
      if (payload.has_label_feature()) {
        return run(builder, payload.frame(), payload.data(), std::move(train_options), std::move(assess_options),
                   objective, objective_policy, fit_options);
      }
      if (payload.has_label()) {
        return builder.benchmark(payload.frame(), payload.label(), options_, std::move(train_options),
                                 std::move(assess_options), objective, objective_policy, fit_options);
      }
    }
    return run(builder, payload.labeled_batch(), std::move(train_options), std::move(assess_options),
               objective, objective_policy, fit_options);
  }

  [[nodiscard]] std::vector<prep::TimeSeriesSplit> splits(const pluss::table::DataFrame& frame,
                                                          std::string_view time_feature) const {
    if (!detail::is_time_plan_(options_)) {
      ::pluss::throw_error("validate::Plan::splits: use validate::time(...) for chronological splitting.");
    }
    prep::TimeSeriesSplitOptions split{};
    split.folds = options_.folds;
    split.gap = options_.time_gap;
    split.test_size = options_.time_test_size;
    split.max_train_size = options_.time_max_train_size;
    split.random_state = options_.random_state;
    split.deterministic = options_.deterministic;
    return prep::timeseries_split(frame, time_feature, split);
  }

  [[nodiscard]] prep::TimeSeriesSplit last(const pluss::table::DataFrame& frame,
                                           std::string_view time_feature) const {
    if (!detail::is_time_plan_(options_)) {
      ::pluss::throw_error("validate::Plan::last: use validate::time(...) for chronological holdout.");
    }
    prep::TimeSeriesSplitOptions split{};
    split.folds = options_.folds;
    split.gap = options_.time_gap;
    split.test_size = options_.time_test_size;
    split.max_train_size = options_.time_max_train_size;
    split.random_state = options_.random_state;
    split.deterministic = options_.deterministic;
    return prep::time_holdout(frame, time_feature, split);
  }

  [[nodiscard]] Series scores(const ModelSpec& spec,
                              const LabeledBatch& batch,
                              MetricKind metric,
                              TrainOptions train_options = {},
                              AssessOptions assess_options = {}) const {
    return metrics::detail::cross_validate_scores(
        spec, batch, metric, options_, std::move(train_options), std::move(assess_options));
  }

  [[nodiscard]] Series scores(const ModelSpec& spec,
                              const pluss::table::DataFrame& frame,
                              const DataFrameBatchOptions& data_options,
                              MetricKind metric,
                              TrainOptions train_options = {},
                              AssessOptions assess_options = {}) const {
    return metrics::detail::cross_validate_scores(
        spec, frame, data_options, metric, options_, std::move(train_options), std::move(assess_options));
  }

  [[nodiscard]] Series scores(const ModelSpec& spec,
                              const pluss::table::DataFrame& x,
                              const Series& y,
                              MetricKind metric,
                              TrainOptions train_options = {},
                              AssessOptions assess_options = {}) const {
    return metrics::detail::cross_validate_scores(
        spec, x, y, metric, options_, std::move(train_options), std::move(assess_options));
  }

  [[nodiscard]] Series scores(const ModelSpec& spec,
                              const pluss::table::DataFrame& x,
                              const pluss::table::Thread& y,
                              MetricKind metric,
                              TrainOptions train_options = {},
                              AssessOptions assess_options = {}) const {
    return scores(spec, x, as_stratify_series(y), metric, std::move(train_options), std::move(assess_options));
  }

  [[nodiscard]] double score(const ModelSpec& spec,
                             const LabeledBatch& batch,
                             MetricKind metric,
                             TrainOptions train_options = {},
                             AssessOptions assess_options = {}) const {
    return metrics::detail::cross_validate_score(
        spec, batch, metric, options_, std::move(train_options), std::move(assess_options));
  }

  [[nodiscard]] double score(const ModelSpec& spec,
                             const pluss::table::DataFrame& frame,
                             const DataFrameBatchOptions& data_options,
                             MetricKind metric,
                             TrainOptions train_options = {},
                             AssessOptions assess_options = {}) const {
    return metrics::detail::cross_validate_score(
        spec, frame, data_options, metric, options_, std::move(train_options), std::move(assess_options));
  }

  [[nodiscard]] double score(const ModelSpec& spec,
                             const pluss::table::DataFrame& x,
                             const Series& y,
                             MetricKind metric,
                             TrainOptions train_options = {},
                             AssessOptions assess_options = {}) const {
    return metrics::detail::cross_validate_score(
        spec, x, y, metric, options_, std::move(train_options), std::move(assess_options));
  }

  [[nodiscard]] double score(const ModelSpec& spec,
                             const pluss::table::DataFrame& x,
                             const pluss::table::Thread& y,
                             MetricKind metric,
                             TrainOptions train_options = {},
                             AssessOptions assess_options = {}) const {
    return score(spec, x, as_stratify_series(y), metric, std::move(train_options), std::move(assess_options));
  }

  // `objective` is a selection::Scorer -- implicitly a MetricKind (the fold
  // values come straight off `run(...)`'s report), or selection::make_scorer(
  // "name", fn) for a callable scored on each fold's held-out
  // `(truth, prediction)`. The ModelSpec overloads above stay MetricKind-only
  // (they run through the NodeJ single-metric cross-validation path).
  [[nodiscard]] Series scores(const models::Model& model,
                              const LabeledBatch& batch,
                              selection::Scorer objective,
                              TrainOptions train_options = {},
                              AssessOptions assess_options = {}) const {
    if (objective.is_custom()) {
      return detail::custom_scorer_fold_values_(model, batch, options_, objective,
                                                std::move(train_options), "validate::Plan::scores");
    }
    assess_options.metrics = {*objective.builtin};
    return detail::fold_metric_values_(run(model, batch, std::move(train_options), std::move(assess_options)),
                                       *objective.builtin, "validate::Plan::scores");
  }

  [[nodiscard]] Series scores(const models::Model& model,
                              const pluss::table::DataFrame& frame,
                              const DataFrameBatchOptions& data_options,
                              selection::Scorer objective,
                              TrainOptions train_options = {},
                              AssessOptions assess_options = {}) const {
    if (objective.is_custom()) {
      return detail::custom_scorer_fold_values_(model, frame, data_options, options_, objective,
                                                std::move(train_options), "validate::Plan::scores");
    }
    assess_options.metrics = {*objective.builtin};
    return detail::fold_metric_values_(run(model, frame, data_options, std::move(train_options), std::move(assess_options)),
                                       *objective.builtin, "validate::Plan::scores");
  }

  [[nodiscard]] double score(const models::Model& model,
                             const LabeledBatch& batch,
                             selection::Scorer objective,
                             TrainOptions train_options = {},
                             AssessOptions assess_options = {}) const {
    const Series values =
        scores(model, batch, std::move(objective), std::move(train_options), std::move(assess_options));
    return values.mean().item<double>();
  }

  [[nodiscard]] double score(const models::Model& model,
                             const pluss::table::DataFrame& frame,
                             const DataFrameBatchOptions& data_options,
                             selection::Scorer objective,
                             TrainOptions train_options = {},
                             AssessOptions assess_options = {}) const {
    const Series values = scores(model, frame, data_options, std::move(objective),
                                 std::move(train_options), std::move(assess_options));
    return values.mean().item<double>();
  }

  [[nodiscard]] Series scores(const models::Model& model,
                              const workflow::Payload& payload,
                              selection::Scorer objective,
                              TrainOptions train_options = {},
                              AssessOptions assess_options = {}) const {
    if (objective.is_custom()) {
      detail::require_time_safe_payload_(options_, payload, "validate::Plan::scores");
      if (payload.has_frame() && payload.has_label_feature()) {
        return detail::custom_scorer_fold_values_(model, payload.frame(), payload.data(), options_, objective,
                                                  std::move(train_options), "validate::Plan::scores");
      }
      return detail::custom_scorer_fold_values_(model, payload.labeled_batch(), options_, objective,
                                                std::move(train_options), "validate::Plan::scores");
    }
    assess_options.metrics = {*objective.builtin};
    return detail::fold_metric_values_(run(model, payload, std::move(train_options), std::move(assess_options)),
                                       *objective.builtin, "validate::Plan::scores");
  }

  [[nodiscard]] Series scores(const workflow::Workflow& workflow,
                              const workflow::Payload& payload,
                              selection::Scorer objective,
                              TrainOptions train_options = {},
                              AssessOptions assess_options = {}) const {
    if (objective.is_custom()) {
      detail::require_workflow_time_compatible_(options_, workflow, payload, "validate::Plan::scores");
      const CrossValOptions eff = detail::effective_time_options_(options_, workflow);
      const ::pluss::ai::lymph::detail::WorkflowLabeledInput input =
          ::pluss::ai::lymph::detail::workflow_labeled_input_(workflow, payload, "validate::Plan::scores");
      if (input.frame.has_value()) {
        return detail::custom_scorer_fold_values_(workflow.model(), *input.frame, input.data_options, eff, objective,
                                                  std::move(train_options), "validate::Plan::scores");
      }
      return detail::custom_scorer_fold_values_(workflow.model(), *input.batch, eff, objective,
                                                std::move(train_options), "validate::Plan::scores");
    }
    assess_options.metrics = {*objective.builtin};
    return detail::fold_metric_values_(run(workflow, payload, std::move(train_options), std::move(assess_options)),
                                       *objective.builtin, "validate::Plan::scores");
  }

  [[nodiscard]] double score(const models::Model& model,
                             const workflow::Payload& payload,
                             selection::Scorer objective,
                             TrainOptions train_options = {},
                             AssessOptions assess_options = {}) const {
    const Series values =
        scores(model, payload, std::move(objective), std::move(train_options), std::move(assess_options));
    return values.mean().item<double>();
  }

  [[nodiscard]] double score(const workflow::Workflow& workflow,
                             const workflow::Payload& payload,
                             selection::Scorer objective,
                             TrainOptions train_options = {},
                             AssessOptions assess_options = {}) const {
    const Series values =
        scores(workflow, payload, std::move(objective), std::move(train_options), std::move(assess_options));
    return values.mean().item<double>();
  }

 private:
  CrossValOptions options_{};
};

using HoldoutOptions = prep::HoldoutSplitOptions;

class HoldoutPlan final {
 public:
  explicit HoldoutPlan(HoldoutOptions options = {}) : options_(std::move(options)) {}

  [[nodiscard]] const HoldoutOptions& options() const noexcept { return options_; }

  HoldoutPlan& test_size(double value) {
    if (!(value > 0.0 && value < 1.0)) {
      ::pluss::throw_error("validate::HoldoutPlan::test_size: value must be between zero and one.");
    }
    options_.test_size = value;
    return *this;
  }

  HoldoutPlan& stratify(Series value) {
    options_.stratify = std::move(value);
    options_.stratify_feature.clear();
    return *this;
  }

  HoldoutPlan& stratify(std::string feature_name) {
    options_.stratify = Series::undefined();
    options_.stratify_feature = std::move(feature_name);
    return *this;
  }

  HoldoutPlan& shuffle(bool value = true) noexcept {
    options_.shuffle = value;
    return *this;
  }

  HoldoutPlan& seed(std::uint64_t value) noexcept {
    options_.random_state = value;
    options_.deterministic = true;
    return *this;
  }

  [[nodiscard]] prep::HoldoutSplit split(const pluss::table::DataFrame& frame) const {
    return prep::holdout_split(frame, options_);
  }

  [[nodiscard]] prep::HoldoutSplit split(const LabeledBatch& batch) const {
    return prep::holdout_split(batch, options_);
  }

 private:
  HoldoutOptions options_{};
};

[[nodiscard]] inline Plan cross(CrossValOptions options = {}) {
  options.plan = CVPlanKind::CrossVal;
  return Plan(std::move(options));
}

[[nodiscard]] inline Plan stratified(CrossValOptions options = {}) {
  options.plan = CVPlanKind::StratifiedCrossVal;
  options.stratified = true;
  return Plan(std::move(options));
}

[[nodiscard]] inline Plan grouped(Series groups, CrossValOptions options = {}) {
  options.plan = CVPlanKind::GroupCrossVal;
  options.groups = std::move(groups);
  return Plan(std::move(options));
}

[[nodiscard]] inline Plan time_ordered(TimeValOptions options = {}) {
  options.plan = CVPlanKind::TSCrossVal;
  options.shuffle = false;
  return Plan(std::move(options));
}

[[nodiscard]] inline Plan time(TimeValOptions options = {}) {
  return time_ordered(std::move(options));
}

[[nodiscard]] inline HoldoutPlan holdout(HoldoutOptions options = {}) {
  return HoldoutPlan(std::move(options));
}

} // namespace pluss::ai::lymph::selection::validate

namespace pluss::ai::lymph::selection {

using HoldoutPlan = validate::HoldoutPlan;
using HoldoutOptions = validate::HoldoutOptions;
using Plan = validate::Plan;
using validate::cross;
using validate::grouped;
using validate::holdout;
using validate::stratified;
using validate::time;
using validate::time_ordered;

} // namespace pluss::ai::lymph::selection

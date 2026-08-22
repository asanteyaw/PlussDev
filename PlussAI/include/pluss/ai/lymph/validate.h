#pragma once

#include "pluss/ai/lymph/builder.h"
#include "pluss/ai/lymph/model.h"
#include "pluss/ai/lymph/payload.h"
#include "pluss/ai/lymph/workflow.h"

#include <utility>

namespace pluss::ai::lymph::validate {

namespace detail {

[[nodiscard]] inline bool is_time_plan_(const metrics::ValidationOptions& options) noexcept {
  return options.plan == CVPlanKind::TSCrossVal;
}

inline void require_time_safe_payload_(const metrics::ValidationOptions& options,
                                       const Payload& payload,
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

inline void require_workflow_time_compatible_(const metrics::ValidationOptions& options,
                                              const Workflow& workflow,
                                              const Payload& payload,
                                              const char* where) {
  if (workflow.time_aware() && !is_time_plan_(options)) {
    ::pluss::throw_error(std::string(where) +
                         ": time-aware Workflow must be evaluated with validate::time() / TSCrossVal.");
  }
  if (is_time_plan_(options)) {
    require_time_safe_payload_(options, payload, where);
    if (!workflow.time_aware()) return;
    const TimeOptions& wf_time = *workflow.time_options();
    const std::string payload_time = payload.time_feature();
    if (has_time_index(wf_time) && !payload_time.empty() && wf_time.index != payload_time) {
      ::pluss::throw_error(std::string(where) + ": Workflow time index \"" + wf_time.index +
                           "\" does not match Payload time feature \"" + payload_time + "\".");
    }
  }
}

[[nodiscard]] inline metrics::ValidationOptions effective_time_options_(metrics::ValidationOptions options,
                                                                       const Workflow& workflow) {
  if (!is_time_plan_(options) || !workflow.time_aware()) return options;
  const TimeOptions& time = *workflow.time_options();
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
  explicit Plan(metrics::ValidationOptions options = {}) : options_(std::move(options)) {}

  [[nodiscard]] const metrics::ValidationOptions& options() const noexcept { return options_; }

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

  [[nodiscard]] metrics::CrossValidationReport run(const Model& model,
                                                   const LabeledBatch& batch,
                                                   TrainOptions train_options = {},
                                                   AssessOptions assess_options = {}) const {
    return ::pluss::ai::lymph::detail::cross_validate(
        ::pluss::ai::lymph::detail::model_core(model), batch, options_,
        std::move(train_options), std::move(assess_options));
  }

  [[nodiscard]] metrics::CrossValidationReport run(const Model& model,
                                                   const pluss::table::DataFrame& frame,
                                                   const DataFrameBatchOptions& data_options,
                                                   TrainOptions train_options = {},
                                                   AssessOptions assess_options = {}) const {
    return ::pluss::ai::lymph::detail::cross_validate(
        ::pluss::ai::lymph::detail::model_core(model), frame, data_options, options_,
        std::move(train_options), std::move(assess_options));
  }

  [[nodiscard]] metrics::CrossValidationReport run(const Model& model,
                                                   const pluss::table::DataFrame& x,
                                                   const Series& y,
                                                   TrainOptions train_options = {},
                                                   AssessOptions assess_options = {}) const {
    return ::pluss::ai::lymph::detail::cross_validate(
        ::pluss::ai::lymph::detail::model_core(model), x, y, options_,
        std::move(train_options), std::move(assess_options));
  }

  [[nodiscard]] metrics::CrossValidationReport run(const Model& model,
                                                   const pluss::table::DataFrame& x,
                                                   const pluss::table::Thread& y,
                                                   TrainOptions train_options = {},
                                                   AssessOptions assess_options = {}) const {
    return run(model, x, as_stratify_series(y), std::move(train_options), std::move(assess_options));
  }

  [[nodiscard]] metrics::CrossValidationReport run(const Model& model,
                                                   const Payload& payload,
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

  [[nodiscard]] metrics::CrossValidationReport run(const Workflow& workflow,
                                                   const Payload& payload,
                                                   TrainOptions train_options = {},
                                                   AssessOptions assess_options = {}) const {
    detail::require_workflow_time_compatible_(options_, workflow, payload, "validate::Plan::run(workflow,payload)");
    const metrics::ValidationOptions effective_options = detail::effective_time_options_(options_, workflow);
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

  [[nodiscard]] selection::CompareResult run(const Builder& builder,
                                             const LabeledBatch& batch,
                                             TrainOptions train_options = {},
                                             AssessOptions assess_options = {},
                                             MetricKind objective_metric = MetricKind::Accuracy,
                                             TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                             selection::FitOptions fit_options = {}) const {
    return builder.benchmark(batch, options_, std::move(train_options), std::move(assess_options),
                             objective_metric, objective_policy, fit_options);
  }

  [[nodiscard]] selection::CompareResult run(const Builder& builder,
                                             const pluss::table::DataFrame& frame,
                                             const DataFrameBatchOptions& data_options,
                                             TrainOptions train_options = {},
                                             AssessOptions assess_options = {},
                                             MetricKind objective_metric = MetricKind::Accuracy,
                                             TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                             selection::FitOptions fit_options = {}) const {
    return builder.benchmark(frame, data_options, options_, std::move(train_options), std::move(assess_options),
                             objective_metric, objective_policy, fit_options);
  }

  [[nodiscard]] selection::CompareResult run(const Builder& builder,
                                             const Payload& payload,
                                             TrainOptions train_options = {},
                                             AssessOptions assess_options = {},
                                             MetricKind objective_metric = MetricKind::Accuracy,
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
                   objective_metric, objective_policy, fit_options);
      }
      if (payload.has_label()) {
        return builder.benchmark(payload.frame(), payload.label(), options_, std::move(train_options),
                                 std::move(assess_options), objective_metric, objective_policy, fit_options);
      }
    }
    return run(builder, payload.labeled_batch(), std::move(train_options), std::move(assess_options),
               objective_metric, objective_policy, fit_options);
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

  [[nodiscard]] Series scores(const Model& model,
                              const LabeledBatch& batch,
                              MetricKind metric,
                              TrainOptions train_options = {},
                              AssessOptions assess_options = {}) const {
    assess_options.metrics = {metric};
    return detail::fold_metric_values_(run(model, batch, std::move(train_options), std::move(assess_options)),
                                       metric, "validate::Plan::scores");
  }

  [[nodiscard]] Series scores(const Model& model,
                              const pluss::table::DataFrame& frame,
                              const DataFrameBatchOptions& data_options,
                              MetricKind metric,
                              TrainOptions train_options = {},
                              AssessOptions assess_options = {}) const {
    assess_options.metrics = {metric};
    return detail::fold_metric_values_(run(model, frame, data_options, std::move(train_options), std::move(assess_options)),
                                       metric, "validate::Plan::scores");
  }

  [[nodiscard]] double score(const Model& model,
                             const LabeledBatch& batch,
                             MetricKind metric,
                             TrainOptions train_options = {},
                             AssessOptions assess_options = {}) const {
    const Series values = scores(model, batch, metric, std::move(train_options), std::move(assess_options));
    return values.mean().item<double>();
  }

  [[nodiscard]] double score(const Model& model,
                             const pluss::table::DataFrame& frame,
                             const DataFrameBatchOptions& data_options,
                             MetricKind metric,
                             TrainOptions train_options = {},
                             AssessOptions assess_options = {}) const {
    const Series values = scores(model, frame, data_options, metric, std::move(train_options), std::move(assess_options));
    return values.mean().item<double>();
  }

  [[nodiscard]] Series scores(const Model& model,
                              const Payload& payload,
                              MetricKind metric,
                              TrainOptions train_options = {},
                              AssessOptions assess_options = {}) const {
    assess_options.metrics = {metric};
    return detail::fold_metric_values_(run(model, payload, std::move(train_options), std::move(assess_options)),
                                       metric, "validate::Plan::scores");
  }

  [[nodiscard]] Series scores(const Workflow& workflow,
                              const Payload& payload,
                              MetricKind metric,
                              TrainOptions train_options = {},
                              AssessOptions assess_options = {}) const {
    assess_options.metrics = {metric};
    return detail::fold_metric_values_(run(workflow, payload, std::move(train_options), std::move(assess_options)),
                                       metric, "validate::Plan::scores");
  }

  [[nodiscard]] double score(const Model& model,
                             const Payload& payload,
                             MetricKind metric,
                             TrainOptions train_options = {},
                             AssessOptions assess_options = {}) const {
    const Series values = scores(model, payload, metric, std::move(train_options), std::move(assess_options));
    return values.mean().item<double>();
  }

  [[nodiscard]] double score(const Workflow& workflow,
                             const Payload& payload,
                             MetricKind metric,
                             TrainOptions train_options = {},
                             AssessOptions assess_options = {}) const {
    const Series values = scores(workflow, payload, metric, std::move(train_options), std::move(assess_options));
    return values.mean().item<double>();
  }

 private:
  metrics::ValidationOptions options_{};
};

class HoldoutPlan final {
 public:
  explicit HoldoutPlan(prep::HoldoutSplitOptions options = {}) : options_(std::move(options)) {}

  [[nodiscard]] const prep::HoldoutSplitOptions& options() const noexcept { return options_; }

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
  prep::HoldoutSplitOptions options_{};
};

[[nodiscard]] inline Plan cross(metrics::ValidationOptions options = {}) {
  options.plan = CVPlanKind::CrossVal;
  return Plan(std::move(options));
}

[[nodiscard]] inline Plan stratified(metrics::ValidationOptions options = {}) {
  options.plan = CVPlanKind::StratifiedCrossVal;
  options.stratified = true;
  return Plan(std::move(options));
}

[[nodiscard]] inline Plan grouped(Series groups, metrics::ValidationOptions options = {}) {
  options.plan = CVPlanKind::GroupCrossVal;
  options.groups = std::move(groups);
  return Plan(std::move(options));
}

[[nodiscard]] inline Plan time_ordered(metrics::ValidationOptions options = {}) {
  options.plan = CVPlanKind::TSCrossVal;
  options.shuffle = false;
  return Plan(std::move(options));
}

[[nodiscard]] inline Plan time(metrics::ValidationOptions options = {}) {
  return time_ordered(std::move(options));
}

[[nodiscard]] inline HoldoutPlan holdout(prep::HoldoutSplitOptions options = {}) {
  return HoldoutPlan(std::move(options));
}

} // namespace pluss::ai::lymph::validate

namespace pluss::ai::lymph {

using ValidationPlan = validate::Plan;
using HoldoutPlan = validate::HoldoutPlan;

} // namespace pluss::ai::lymph

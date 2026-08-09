#pragma once

#include "pluss/ai/lymph/builder.h"
#include "pluss/ai/lymph/model.h"

#include <utility>

namespace pluss::ai::lymph::validate {

namespace detail {

[[nodiscard]] inline std::int64_t metric_index_(pluss::msharp::RangeView::Of<MetricKind> metrics,
                                                MetricKind metric,
                                                const char* where) {
  for (std::int64_t i = 0; i < metrics.size(); ++i) {
    if (metrics[static_cast<std::size_t>(i)] == metric) return i;
  }
  ::pluss::throw_error(std::string(where) + ": requested metric was not present in the validation report.");
}

[[nodiscard]] inline Series fold_metric_values_(const CrossValidationReport& report,
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
  explicit Plan(CrossValidationOptions options = {}) : options_(std::move(options)) {}

  [[nodiscard]] const CrossValidationOptions& options() const noexcept { return options_; }

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

  [[nodiscard]] CrossValidationReport run(const Model& model,
                                          const LabeledBatch& batch,
                                          TrainOptions train_options = {},
                                          AssessOptions assess_options = {}) const {
    return ::pluss::ai::lymph::detail::cross_validate(
        ::pluss::ai::lymph::detail::model_core(model), batch, options_,
        std::move(train_options), std::move(assess_options));
  }

  [[nodiscard]] CrossValidationReport run(const Model& model,
                                          const pluss::table::DataFrame& frame,
                                          const DataFrameBatchOptions& data_options,
                                          TrainOptions train_options = {},
                                          AssessOptions assess_options = {}) const {
    return ::pluss::ai::lymph::detail::cross_validate(
        ::pluss::ai::lymph::detail::model_core(model), frame, data_options, options_,
        std::move(train_options), std::move(assess_options));
  }

  [[nodiscard]] CrossValidationReport run(const Model& model,
                                          const pluss::table::DataFrame& x,
                                          const Series& y,
                                          TrainOptions train_options = {},
                                          AssessOptions assess_options = {}) const {
    return ::pluss::ai::lymph::detail::cross_validate(
        ::pluss::ai::lymph::detail::model_core(model), x, y, options_,
        std::move(train_options), std::move(assess_options));
  }

  [[nodiscard]] CrossValidationReport run(const Model& model,
                                          const pluss::table::DataFrame& x,
                                          const pluss::table::Thread& y,
                                          TrainOptions train_options = {},
                                          AssessOptions assess_options = {}) const {
    return run(model, x, as_stratify_series(y), std::move(train_options), std::move(assess_options));
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

 private:
  CrossValidationOptions options_{};
};

class HoldoutPlan final {
 public:
  explicit HoldoutPlan(HoldoutSplitOptions options = {}) : options_(std::move(options)) {}

  [[nodiscard]] const HoldoutSplitOptions& options() const noexcept { return options_; }

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

  [[nodiscard]] FrameHoldoutSplit split(const pluss::table::DataFrame& frame) const {
    return ::pluss::ai::lymph::holdout_split(frame, options_);
  }

  [[nodiscard]] BatchHoldoutSplit split(const LabeledBatch& batch) const {
    return ::pluss::ai::lymph::holdout_split(batch, options_);
  }

 private:
  HoldoutSplitOptions options_{};
};

[[nodiscard]] inline Plan cross(CrossValidationOptions options = {}) {
  options.plan = CVPlanKind::CrossVal;
  return Plan(std::move(options));
}

[[nodiscard]] inline Plan stratified(CrossValidationOptions options = {}) {
  options.plan = CVPlanKind::StratifiedCrossVal;
  options.stratified = true;
  return Plan(std::move(options));
}

[[nodiscard]] inline Plan grouped(Series groups, CrossValidationOptions options = {}) {
  options.plan = CVPlanKind::GroupCrossVal;
  options.groups = std::move(groups);
  return Plan(std::move(options));
}

[[nodiscard]] inline Plan time_ordered(CrossValidationOptions options = {}) {
  options.plan = CVPlanKind::TSCrossVal;
  options.shuffle = false;
  return Plan(std::move(options));
}

[[nodiscard]] inline HoldoutPlan holdout(HoldoutSplitOptions options = {}) {
  return HoldoutPlan(std::move(options));
}

} // namespace pluss::ai::lymph::validate

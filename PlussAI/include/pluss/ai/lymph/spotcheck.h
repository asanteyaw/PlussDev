#pragma once

#include "pluss/ai/lymph/builder.h"
#include "pluss/ai/lymph/validate.h"
#include "pluss/ai/lymph/workflow.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace pluss::ai::lymph::selection {

namespace spotcheck {

using Result = CompareResult;

class Suite final {
 public:
  explicit Suite(const models::Builder& builder) {
    const auto names = builder.names();
    entries_.reserve(names.size());
    for (const auto& name : names) {
      entries_.emplace_back(name, builder.get(name));
    }
  }

  Suite& strategy(std::string name,
                  TrainOptions options = {}) {
    if (name.empty()) {
      ::pluss::throw_error("lymph::spotcheck::Suite::strategy: name must not be empty.");
    }
    strategies_.push_back(Strategy_{std::move(name), std::move(options)});
    return *this;
  }

  Suite& strategy(std::string name,
                  BalanceTarget balance) {
    TrainOptions options{};
    options.balance = balance;
    return strategy(std::move(name), std::move(options));
  }

  Suite& strategy(std::string name,
                  BalanceTarget balance,
                  TrainOptions options) {
    options.balance = balance;
    return strategy(std::move(name), std::move(options));
  }

  [[nodiscard]] Result run(const pluss::table::DataFrame& frame,
                           const DataFrameBatchOptions& data_options,
                           CrossValOptions cv = {},
                           AssessOptions assess_options = {},
                           MetricKind objective_metric = MetricKind::Accuracy,
                           TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                           FitOptions fit_options = {}) const {
    return compare(candidates_(), frame, data_options, cv, TrainOptions{},
                   assess_options, objective_metric, objective_policy, fit_options);
  }

  [[nodiscard]] Result run(const LabeledBatch& batch,
                           CrossValOptions cv = {},
                           AssessOptions assess_options = {},
                           MetricKind objective_metric = MetricKind::Accuracy,
                           TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                           FitOptions fit_options = {}) const {
    return compare(candidates_(), batch, cv, TrainOptions{},
                   assess_options, objective_metric, objective_policy, fit_options);
  }

  [[nodiscard]] Result run(const workflow::Workflow& design,
                           const workflow::Payload& payload,
                           CrossValOptions cv = {},
                           AssessOptions assess_options = {},
                           MetricKind objective_metric = MetricKind::Accuracy,
                           TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                           FitOptions fit_options = {}) const {
    validate::detail::require_workflow_time_compatible_(
        cv, design, payload, "lymph::selection::spotcheck::Suite::run(workflow,payload)");
    const TimeValOptions effective_cv = validate::detail::effective_time_options_(cv, design);
    const detail::WorkflowLabeledInput input =
        detail::workflow_labeled_input_(design, payload, "lymph::selection::spotcheck::Suite::run(workflow,payload)");
    if (input.frame.has_value()) {
      return compare(candidates_(design), *input.frame, input.data_options, effective_cv, TrainOptions{},
                     assess_options, objective_metric, objective_policy, fit_options);
    }
    return compare(candidates_(design), *input.batch, effective_cv, TrainOptions{},
                   assess_options, objective_metric, objective_policy, fit_options);
  }

 private:
  struct Strategy_ {
    std::string name{};
    TrainOptions train_options{};
  };

  [[nodiscard]] std::vector<Candidate> candidates_() const {
    if (entries_.empty()) {
      ::pluss::throw_error("lymph::selection::spotcheck::Suite::run: at least one model is required.");
    }
    if (strategies_.empty()) {
      ::pluss::throw_error("lymph::selection::spotcheck::Suite::run: at least one strategy is required.");
    }
    std::vector<Candidate> out{};
    out.reserve(entries_.size() * strategies_.size());
    for (const auto& strategy : strategies_) {
      for (const auto& [model_name, model] : entries_) {
        out.emplace_back(strategy.name + "__" + model_name, model, strategy.train_options);
      }
    }
    return out;
  }

  [[nodiscard]] std::vector<Candidate> candidates_(const workflow::Workflow& design) const {
    std::vector<Candidate> out = candidates_();
    for (auto& candidate : out) {
      workflow::Workflow candidate_workflow = design.with(candidate.model);
      candidate.model = candidate_workflow.model();
    }
    return out;
  }

  std::vector<std::pair<std::string, models::Model>> entries_{};
  std::vector<Strategy_> strategies_{};
};

[[nodiscard]] inline Suite suite(const models::Builder& builder) {
  return Suite(builder);
}

} // namespace spotcheck

using Suite = spotcheck::Suite;
using spotcheck::suite;

} // namespace pluss::ai::lymph::selection

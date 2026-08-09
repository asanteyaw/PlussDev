#pragma once

#include "pluss/ai/lymph/builder.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace pluss::ai::lymph {

using SpotCheckResult = selection::CompareResult;

namespace spotcheck {

class Models final {
 public:
  explicit Models(const Builder& builder) {
    const auto names = builder.names();
    entries_.reserve(names.size());
    for (const auto& name : names) {
      entries_.emplace_back(name, builder.get(name));
    }
  }

  Models& strategy(std::string name,
                   TrainOptions options = {}) {
    if (name.empty()) {
      ::pluss::throw_error("lymph::spotcheck::Models::strategy: name must not be empty.");
    }
    strategies_.push_back(Strategy_{std::move(name), std::move(options)});
    return *this;
  }

  Models& strategy(std::string name,
                   BalanceTarget balance) {
    TrainOptions options{};
    options.balance = balance;
    return strategy(std::move(name), std::move(options));
  }

  Models& strategy(std::string name,
                   BalanceTarget balance,
                   TrainOptions options) {
    options.balance = balance;
    return strategy(std::move(name), std::move(options));
  }

  [[nodiscard]] SpotCheckResult run(const pluss::table::DataFrame& frame,
                                    const DataFrameBatchOptions& data_options,
                                    CrossValidationOptions cv = {},
                                    AssessOptions assess_options = {},
                                    MetricKind objective_metric = MetricKind::Accuracy,
                                    TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                    selection::FitOptions fit_options = {}) const {
    return selection::compare(candidates_(), frame, data_options, cv, TrainOptions{},
                              assess_options, objective_metric, objective_policy, fit_options);
  }

  [[nodiscard]] SpotCheckResult run(const LabeledBatch& batch,
                                    CrossValidationOptions cv = {},
                                    AssessOptions assess_options = {},
                                    MetricKind objective_metric = MetricKind::Accuracy,
                                    TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                    selection::FitOptions fit_options = {}) const {
    return selection::compare(candidates_(), batch, cv, TrainOptions{},
                              assess_options, objective_metric, objective_policy, fit_options);
  }

 private:
  struct Strategy_ {
    std::string name{};
    TrainOptions train_options{};
  };

  [[nodiscard]] std::vector<selection::Candidate> candidates_() const {
    if (entries_.empty()) {
      ::pluss::throw_error("lymph::spotcheck::Models::run: at least one model is required.");
    }
    if (strategies_.empty()) {
      ::pluss::throw_error("lymph::spotcheck::Models::run: at least one strategy is required.");
    }
    std::vector<selection::Candidate> out{};
    out.reserve(entries_.size() * strategies_.size());
    for (const auto& strategy : strategies_) {
      for (const auto& [model_name, model] : entries_) {
        out.emplace_back(strategy.name + "__" + model_name, model, strategy.train_options);
      }
    }
    return out;
  }

  std::vector<std::pair<std::string, Model>> entries_{};
  std::vector<Strategy_> strategies_{};
};

[[nodiscard]] inline Models models(const Builder& builder) {
  return Models(builder);
}

} // namespace spotcheck

} // namespace pluss::ai::lymph

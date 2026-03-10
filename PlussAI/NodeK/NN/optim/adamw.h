#pragma once

#include "optim_base.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <stdexcept>
#include <unordered_set>
#include <vector>

namespace pluss::ai::nn::optim {

struct AdamWOptions {
  double learning_rate{1e-3};
  double beta1{0.9};
  double beta2{0.999};
  double epsilon{1e-8};
  double weight_decay{1e-2};
  bool amsgrad{false};
  bool maximize{false};
  GradientClipOptions clip{};
};

struct AdamWGroup {
  std::vector<Series*> parameters{};
  AdamWOptions options{};
};

class PLUSS_API AdamW final : public Optimizer {
public:
  explicit AdamW(std::vector<Series*> parameters, AdamWOptions options = {})
      : Optimizer(std::move(parameters)), default_options_(options) {
    validate_options_(default_options_);
    GroupState group{};
    group.options = default_options_;
    group.indices.resize(size());
    std::iota(group.indices.begin(), group.indices.end(), static_cast<std::size_t>(0));
    groups_.push_back(std::move(group));
    initialize_state_();
  }

  explicit AdamW(std::vector<AdamWGroup> groups)
      : Optimizer(flatten_groups_(groups)),
        groups_(build_group_states_(groups)),
        default_options_(groups.front().options) {
    validate_group_options_();
    initialize_state_();
  }

  explicit AdamW(Module& module, AdamWOptions options = {})
      : AdamW(module.parameters(), options) {}

  const AdamWOptions& options() const noexcept { return default_options_; }

  void set_options(const AdamWOptions& options) {
    default_options_ = options;
    validate_options_(default_options_);
    for (auto& group : groups_) {
      group.options = default_options_;
    }
  }

  void set_group_options(std::size_t group_index, const AdamWOptions& options) {
    if (group_index >= groups_.size()) {
      throw std::runtime_error("pluss::ai::nn::optim::AdamW::set_group_options: group index out of range");
    }
    validate_options_(options);
    groups_[group_index].options = options;
  }

  std::size_t group_count() const noexcept { return groups_.size(); }

private:
  struct GroupState {
    std::vector<std::size_t> indices{};
    AdamWOptions options{};
  };

  void apply_step_(const std::vector<Series>& gradients) override {
    ++step_;

    auto& params = mutable_parameters_();
    for (const auto& group : groups_) {
      const AdamWOptions& options = group.options;
      const double b1_correction = 1.0 - std::pow(options.beta1, static_cast<double>(step_));
      const double b2_correction = 1.0 - std::pow(options.beta2, static_cast<double>(step_));

      for (std::size_t i : group.indices) {
        Series* param_ptr = params[i];
        if (param_ptr == nullptr) continue;

        const Series& input_sensitivity = gradients[i];
        if (!input_sensitivity.defined()) continue;

        Series sensitivity = clip_gradient_(input_sensitivity, options.clip);
        if (options.maximize) {
          sensitivity = sensitivity.neg();
        }

        Series parameter_value = param_ptr->detach();
        if (options.weight_decay != 0.0) {
          parameter_value = parameter_value.mul(1.0 - options.learning_rate * options.weight_decay);
        }

        first_moment_[i] = first_moment_[i].mul(options.beta1).add(sensitivity.mul(1.0 - options.beta1));
        second_moment_[i] = second_moment_[i].mul(options.beta2).add(sensitivity.square().mul(1.0 - options.beta2));

        Series second_hat = second_moment_[i].div(b2_correction);
        if (options.amsgrad) {
          max_second_hat_[i] = max_second_hat_[i].maximum(second_hat);
          second_hat = max_second_hat_[i];
        }

        const Series first_hat = first_moment_[i].div(b1_correction);
        const Series denominator = second_hat.sqrt().add(options.epsilon);
        const Series delta = first_hat.div(denominator).mul(options.learning_rate);
        assign_parameter_(*param_ptr, parameter_value.sub(delta));
      }
    }
  }

  static void validate_options_(const AdamWOptions& options) {
    if (!(options.learning_rate > 0.0) || !std::isfinite(options.learning_rate)) {
      throw std::runtime_error("pluss::ai::nn::optim::AdamW: learning_rate must be positive and finite");
    }
    if (!(options.beta1 >= 0.0 && options.beta1 < 1.0) || !std::isfinite(options.beta1)) {
      throw std::runtime_error("pluss::ai::nn::optim::AdamW: beta1 must be in [0, 1)");
    }
    if (!(options.beta2 >= 0.0 && options.beta2 < 1.0) || !std::isfinite(options.beta2)) {
      throw std::runtime_error("pluss::ai::nn::optim::AdamW: beta2 must be in [0, 1)");
    }
    if (!(options.epsilon > 0.0) || !std::isfinite(options.epsilon)) {
      throw std::runtime_error("pluss::ai::nn::optim::AdamW: epsilon must be positive and finite");
    }
    if (options.weight_decay < 0.0 || !std::isfinite(options.weight_decay)) {
      throw std::runtime_error("pluss::ai::nn::optim::AdamW: weight_decay must be finite and non-negative");
    }
  }

  static std::vector<Series*> flatten_groups_(const std::vector<AdamWGroup>& groups) {
    if (groups.empty()) {
      throw std::runtime_error("pluss::ai::nn::optim::AdamW: expected non-empty parameter groups");
    }
    std::vector<Series*> flat{};
    std::unordered_set<Series*> seen{};
    for (const auto& group : groups) {
      if (group.parameters.empty()) {
        throw std::runtime_error("pluss::ai::nn::optim::AdamW: parameter group must not be empty");
      }
      for (Series* p : group.parameters) {
        if (p != nullptr && !seen.insert(p).second) {
          throw std::runtime_error("pluss::ai::nn::optim::AdamW: duplicate parameter across groups");
        }
        flat.push_back(p);
      }
    }
    return flat;
  }

  static std::vector<GroupState> build_group_states_(const std::vector<AdamWGroup>& groups) {
    std::vector<GroupState> out{};
    out.reserve(groups.size());
    std::size_t next_index = 0;
    for (const auto& group : groups) {
      GroupState state{};
      state.options = group.options;
      state.indices.reserve(group.parameters.size());
      for (std::size_t i = 0; i < group.parameters.size(); ++i) {
        state.indices.push_back(next_index++);
      }
      out.push_back(std::move(state));
    }
    return out;
  }

  void validate_group_options_() const {
    for (const auto& group : groups_) {
      validate_options_(group.options);
    }
  }

  void initialize_state_() {
    const auto& params = parameters();
    first_moment_.clear();
    second_moment_.clear();
    max_second_hat_.clear();

    first_moment_.reserve(params.size());
    second_moment_.reserve(params.size());
    max_second_hat_.reserve(params.size());

    for (const Series* p : params) {
      first_moment_.push_back(pluss::zeros_like(*p, p->scalar_type()));
      second_moment_.push_back(pluss::zeros_like(*p, p->scalar_type()));
      max_second_hat_.push_back(pluss::zeros_like(*p, p->scalar_type()));
    }
  }

  std::vector<GroupState> groups_{};
  AdamWOptions default_options_{};
  std::vector<Series> first_moment_{};
  std::vector<Series> second_moment_{};
  std::vector<Series> max_second_hat_{};
  std::int64_t step_{0};
};

} // namespace pluss::ai::nn::optim

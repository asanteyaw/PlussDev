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

struct AdamOptions {
  double learning_rate{1e-3};
  double beta1{0.9};
  double beta2{0.999};
  double epsilon{1e-8};
  double weight_decay{0.0};
  bool amsgrad{false};
  bool maximize{false};
  GradientClipOptions clip{};
};

struct AdamGroup {
  std::vector<Series*> parameters{};
  AdamOptions options{};
};

class PLUSS_API Adam final : public Optimizer {
public:
  explicit Adam(std::vector<Series*> parameters, AdamOptions options = {})
      : Optimizer(std::move(parameters)), default_options_(options) {
    validate_options_(default_options_);
    GroupState group{};
    group.options = default_options_;
    group.indices.resize(size());
    std::iota(group.indices.begin(), group.indices.end(), static_cast<std::size_t>(0));
    groups_.push_back(std::move(group));
    initialize_state_();
  }

  explicit Adam(std::vector<AdamGroup> groups)
      : Optimizer(flatten_groups_(groups)),
        groups_(build_group_states_(groups)),
        default_options_(groups.front().options) {
    validate_group_options_();
    initialize_state_();
  }

  explicit Adam(Module& module, AdamOptions options = {})
      : Adam(module.parameters(), options) {}

  const AdamOptions& options() const noexcept { return default_options_; }

  void set_options(const AdamOptions& options) {
    default_options_ = options;
    validate_options_(default_options_);
    for (auto& group : groups_) {
      group.options = default_options_;
    }
  }

  void set_group_options(std::size_t group_index, const AdamOptions& options) {
    if (group_index >= groups_.size()) {
      throw std::runtime_error("pluss::ai::nn::optim::Adam::set_group_options: group index out of range");
    }
    validate_options_(options);
    groups_[group_index].options = options;
  }

  std::size_t group_count() const noexcept { return groups_.size(); }

private:
  struct GroupState {
    std::vector<std::size_t> indices{};
    AdamOptions options{};
  };

  void apply_step_(const std::vector<Series>& gradients) override {
    ++step_;

    auto& params = mutable_parameters_();
    for (const auto& group : groups_) {
      const AdamOptions& options = group.options;
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
          sensitivity = sensitivity.add(parameter_value.mul(options.weight_decay));
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

  static void validate_options_(const AdamOptions& options) {
    if (!(options.learning_rate > 0.0) || !std::isfinite(options.learning_rate)) {
      throw std::runtime_error("pluss::ai::nn::optim::Adam: learning_rate must be positive and finite");
    }
    if (!(options.beta1 >= 0.0 && options.beta1 < 1.0) || !std::isfinite(options.beta1)) {
      throw std::runtime_error("pluss::ai::nn::optim::Adam: beta1 must be in [0, 1)");
    }
    if (!(options.beta2 >= 0.0 && options.beta2 < 1.0) || !std::isfinite(options.beta2)) {
      throw std::runtime_error("pluss::ai::nn::optim::Adam: beta2 must be in [0, 1)");
    }
    if (!(options.epsilon > 0.0) || !std::isfinite(options.epsilon)) {
      throw std::runtime_error("pluss::ai::nn::optim::Adam: epsilon must be positive and finite");
    }
    if (options.weight_decay < 0.0 || !std::isfinite(options.weight_decay)) {
      throw std::runtime_error("pluss::ai::nn::optim::Adam: weight_decay must be finite and non-negative");
    }
  }

  static std::vector<Series*> flatten_groups_(const std::vector<AdamGroup>& groups) {
    if (groups.empty()) {
      throw std::runtime_error("pluss::ai::nn::optim::Adam: expected non-empty parameter groups");
    }
    std::vector<Series*> flat{};
    std::unordered_set<Series*> seen{};
    for (const auto& group : groups) {
      if (group.parameters.empty()) {
        throw std::runtime_error("pluss::ai::nn::optim::Adam: parameter group must not be empty");
      }
      for (Series* p : group.parameters) {
        if (p != nullptr && !seen.insert(p).second) {
          throw std::runtime_error("pluss::ai::nn::optim::Adam: duplicate parameter across groups");
        }
        flat.push_back(p);
      }
    }
    return flat;
  }

  static std::vector<GroupState> build_group_states_(const std::vector<AdamGroup>& groups) {
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
  AdamOptions default_options_{};
  std::vector<Series> first_moment_{};
  std::vector<Series> second_moment_{};
  std::vector<Series> max_second_hat_{};
  std::int64_t step_{0};
};

} // namespace pluss::ai::nn::optim

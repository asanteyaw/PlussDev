#pragma once

#include "optim_base.h"

#include <cmath>
#include <cstddef>
#include <numeric>
#include <stdexcept>
#include <unordered_set>
#include <vector>

namespace pluss::ai::nn::optim {

struct RMSpropOptions {
  double learning_rate{1e-2};
  double alpha{0.99};
  double epsilon{1e-8};
  double momentum{0.0};
  double weight_decay{0.0};
  bool centered{false};
  bool maximize{false};
  GradientClipOptions clip{};
};

struct RMSpropGroup {
  std::vector<Series*> parameters{};
  RMSpropOptions options{};
};

class PLUSS_API RMSprop final : public Optimizer {
public:
  explicit RMSprop(std::vector<Series*> parameters, RMSpropOptions options = {})
      : Optimizer(std::move(parameters)), default_options_(options) {
    validate_options_(default_options_);
    GroupState group{};
    group.options = default_options_;
    group.indices.resize(size());
    std::iota(group.indices.begin(), group.indices.end(), static_cast<std::size_t>(0));
    groups_.push_back(std::move(group));
    initialize_state_();
  }

  explicit RMSprop(std::vector<RMSpropGroup> groups)
      : Optimizer(flatten_groups_(groups)),
        groups_(build_group_states_(groups)),
        default_options_(groups.front().options) {
    validate_group_options_();
    initialize_state_();
  }

  explicit RMSprop(Module& module, RMSpropOptions options = {})
      : RMSprop(module.parameters(), options) {}

  const RMSpropOptions& options() const noexcept { return default_options_; }

  void set_options(const RMSpropOptions& options) {
    default_options_ = options;
    validate_options_(default_options_);
    for (auto& group : groups_) {
      group.options = default_options_;
    }
    initialize_state_();
  }

  void set_group_options(std::size_t group_index, const RMSpropOptions& options) {
    if (group_index >= groups_.size()) {
      throw std::runtime_error("pluss::ai::nn::optim::RMSprop::set_group_options: group index out of range");
    }
    validate_options_(options);
    groups_[group_index].options = options;
    initialize_state_();
  }

  std::size_t group_count() const noexcept { return groups_.size(); }

private:
  struct GroupState {
    std::vector<std::size_t> indices{};
    RMSpropOptions options{};
  };

  void apply_step_(const std::vector<Series>& gradients) override {
    auto& params = mutable_parameters_();
    for (const auto& group : groups_) {
      const RMSpropOptions& options = group.options;
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

        square_avg_[i] = square_avg_[i].mul(options.alpha).add(sensitivity.square().mul(1.0 - options.alpha));

        Series denominator = square_avg_[i].sqrt().add(options.epsilon);
        if (options.centered) {
          grad_avg_[i] = grad_avg_[i].mul(options.alpha).add(sensitivity.mul(1.0 - options.alpha));
          denominator = square_avg_[i].sub(grad_avg_[i].square()).clamp_min(0.0).sqrt().add(options.epsilon);
        }

        Series update = sensitivity.div(denominator);
        if (options.momentum > 0.0) {
          momentum_buffer_[i] = momentum_buffer_[i].mul(options.momentum).add(update);
          update = momentum_buffer_[i];
        }

        const Series next_value = parameter_value.sub(update.mul(options.learning_rate));
        assign_parameter_(*param_ptr, next_value);
      }
    }
  }

  static void validate_options_(const RMSpropOptions& options) {
    if (!(options.learning_rate > 0.0) || !std::isfinite(options.learning_rate)) {
      throw std::runtime_error("pluss::ai::nn::optim::RMSprop: learning_rate must be positive and finite");
    }
    if (!(options.alpha >= 0.0 && options.alpha < 1.0) || !std::isfinite(options.alpha)) {
      throw std::runtime_error("pluss::ai::nn::optim::RMSprop: alpha must be in [0, 1)");
    }
    if (!(options.epsilon > 0.0) || !std::isfinite(options.epsilon)) {
      throw std::runtime_error("pluss::ai::nn::optim::RMSprop: epsilon must be positive and finite");
    }
    if (options.momentum < 0.0 || !std::isfinite(options.momentum)) {
      throw std::runtime_error("pluss::ai::nn::optim::RMSprop: momentum must be finite and non-negative");
    }
    if (options.weight_decay < 0.0 || !std::isfinite(options.weight_decay)) {
      throw std::runtime_error("pluss::ai::nn::optim::RMSprop: weight_decay must be finite and non-negative");
    }
  }

  static std::vector<Series*> flatten_groups_(const std::vector<RMSpropGroup>& groups) {
    if (groups.empty()) {
      throw std::runtime_error("pluss::ai::nn::optim::RMSprop: expected non-empty parameter groups");
    }
    std::vector<Series*> flat{};
    std::unordered_set<Series*> seen{};
    for (const auto& group : groups) {
      if (group.parameters.empty()) {
        throw std::runtime_error("pluss::ai::nn::optim::RMSprop: parameter group must not be empty");
      }
      for (Series* p : group.parameters) {
        if (p != nullptr && !seen.insert(p).second) {
          throw std::runtime_error("pluss::ai::nn::optim::RMSprop: duplicate parameter across groups");
        }
        flat.push_back(p);
      }
    }
    return flat;
  }

  static std::vector<GroupState> build_group_states_(const std::vector<RMSpropGroup>& groups) {
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
    square_avg_.clear();
    momentum_buffer_.clear();
    grad_avg_.clear();

    square_avg_.reserve(params.size());
    momentum_buffer_.reserve(params.size());
    grad_avg_.reserve(params.size());

    for (const Series* p : params) {
      square_avg_.push_back(pluss::zeros_like(*p, p->scalar_type()));
      momentum_buffer_.push_back(pluss::zeros_like(*p, p->scalar_type()));
      grad_avg_.push_back(pluss::zeros_like(*p, p->scalar_type()));
    }
  }

  std::vector<GroupState> groups_{};
  RMSpropOptions default_options_{};
  std::vector<Series> square_avg_{};
  std::vector<Series> momentum_buffer_{};
  std::vector<Series> grad_avg_{};
};

} // namespace pluss::ai::nn::optim

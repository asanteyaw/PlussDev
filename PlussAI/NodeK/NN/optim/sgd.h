#pragma once

#include "optim_base.h"

#include <cmath>
#include <cstddef>
#include <numeric>
#include <stdexcept>
#include <unordered_set>
#include <vector>

namespace pluss::ai::nn::optim {

struct SGDOptions {
  double learning_rate{1e-2};
  double momentum{0.0};
  double dampening{0.0};
  double weight_decay{0.0};
  bool nesterov{false};
  bool maximize{false};
  GradientClipOptions clip{};
};

struct SGDGroup {
  std::vector<Series*> parameters{};
  SGDOptions options{};
};

class PLUSS_API SGD final : public Optimizer {
public:
  explicit SGD(std::vector<Series*> parameters, SGDOptions options = {})
      : Optimizer(std::move(parameters)), default_options_(options) {
    validate_options_(default_options_);
    GroupState group{};
    group.options = default_options_;
    group.indices.resize(size());
    std::iota(group.indices.begin(), group.indices.end(), static_cast<std::size_t>(0));
    groups_.push_back(std::move(group));
    initialize_state_();
  }

  explicit SGD(std::vector<SGDGroup> groups)
      : Optimizer(flatten_groups_(groups)),
        groups_(build_group_states_(groups)),
        default_options_(groups.front().options) {
    validate_group_options_();
    initialize_state_();
  }

  explicit SGD(Module& module, SGDOptions options = {})
      : SGD(module.parameters(), options) {}

  const SGDOptions& options() const noexcept { return default_options_; }

  void set_options(const SGDOptions& options) {
    default_options_ = options;
    validate_options_(default_options_);
    for (auto& group : groups_) {
      group.options = default_options_;
    }
    initialize_state_();
  }

  void set_group_options(std::size_t group_index, const SGDOptions& options) {
    if (group_index >= groups_.size()) {
      throw std::runtime_error("pluss::ai::nn::optim::SGD::set_group_options: group index out of range");
    }
    validate_options_(options);
    groups_[group_index].options = options;
  }

  std::size_t group_count() const noexcept { return groups_.size(); }

private:
  struct GroupState {
    std::vector<std::size_t> indices{};
    SGDOptions options{};
  };

  void apply_step_(const std::vector<Series>& gradients) override {
    auto& params = mutable_parameters_();
    for (const auto& group : groups_) {
      const SGDOptions& options = group.options;
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

        Series update = sensitivity;
        if (options.momentum > 0.0) {
          velocity_[i] = velocity_[i].mul(options.momentum).add(sensitivity.mul(1.0 - options.dampening));
          if (options.nesterov) {
            update = sensitivity.add(velocity_[i].mul(options.momentum));
          } else {
            update = velocity_[i];
          }
        }

        const Series next_value = parameter_value.sub(update.mul(options.learning_rate));
        assign_parameter_(*param_ptr, next_value);
      }
    }
  }

  static void validate_options_(const SGDOptions& options) {
    if (!(options.learning_rate > 0.0) || !std::isfinite(options.learning_rate)) {
      throw std::runtime_error("pluss::ai::nn::optim::SGD: learning_rate must be positive and finite");
    }
    if (options.momentum < 0.0 || !std::isfinite(options.momentum)) {
      throw std::runtime_error("pluss::ai::nn::optim::SGD: momentum must be finite and non-negative");
    }
    if (options.dampening < 0.0 || !std::isfinite(options.dampening)) {
      throw std::runtime_error("pluss::ai::nn::optim::SGD: dampening must be finite and non-negative");
    }
    if (options.weight_decay < 0.0 || !std::isfinite(options.weight_decay)) {
      throw std::runtime_error("pluss::ai::nn::optim::SGD: weight_decay must be finite and non-negative");
    }
    if (options.nesterov && options.momentum <= 0.0) {
      throw std::runtime_error("pluss::ai::nn::optim::SGD: nesterov requires momentum > 0");
    }
  }

  static std::vector<Series*> flatten_groups_(const std::vector<SGDGroup>& groups) {
    if (groups.empty()) {
      throw std::runtime_error("pluss::ai::nn::optim::SGD: expected non-empty parameter groups");
    }
    std::vector<Series*> flat{};
    std::unordered_set<Series*> seen{};
    for (const auto& group : groups) {
      if (group.parameters.empty()) {
        throw std::runtime_error("pluss::ai::nn::optim::SGD: parameter group must not be empty");
      }
      for (Series* p : group.parameters) {
        if (p != nullptr && !seen.insert(p).second) {
          throw std::runtime_error("pluss::ai::nn::optim::SGD: duplicate parameter across groups");
        }
        flat.push_back(p);
      }
    }
    return flat;
  }

  static std::vector<GroupState> build_group_states_(const std::vector<SGDGroup>& groups) {
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
    velocity_.clear();
    velocity_.reserve(params.size());
    for (const Series* p : params) {
      velocity_.push_back(pluss::zeros_like(*p, p->scalar_type()));
    }
  }

  std::vector<GroupState> groups_{};
  SGDOptions default_options_{};
  std::vector<Series> velocity_{};
};

} // namespace pluss::ai::nn::optim

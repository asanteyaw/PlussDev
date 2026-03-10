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

struct NAdamOptions {
  double learning_rate{2e-3};
  double beta1{0.9};
  double beta2{0.999};
  double epsilon{1e-8};
  double weight_decay{0.0};
  double momentum_decay{4e-3};
  bool decoupled_weight_decay{false};
  bool maximize{false};
  GradientClipOptions clip{};
};

struct NAdamGroup {
  std::vector<Series*> parameters{};
  NAdamOptions options{};
};

class PLUSS_API NAdam final : public Optimizer {
public:
  explicit NAdam(std::vector<Series*> parameters, NAdamOptions options = {})
      : Optimizer(std::move(parameters)), default_options_(options) {
    validate_options_(default_options_);
    GroupState group{};
    group.options = default_options_;
    group.indices.resize(size());
    std::iota(group.indices.begin(), group.indices.end(), static_cast<std::size_t>(0));
    groups_.push_back(std::move(group));
    initialize_state_();
  }

  explicit NAdam(std::vector<NAdamGroup> groups)
      : Optimizer(flatten_groups_(groups)),
        groups_(build_group_states_(groups)),
        default_options_(groups.front().options) {
    validate_group_options_();
    initialize_state_();
  }

  explicit NAdam(Module& module, NAdamOptions options = {})
      : NAdam(module.parameters(), options) {}

  const NAdamOptions& options() const noexcept { return default_options_; }

  void set_options(const NAdamOptions& options) {
    default_options_ = options;
    validate_options_(default_options_);
    for (auto& group : groups_) {
      group.options = default_options_;
    }
  }

  void set_group_options(std::size_t group_index, const NAdamOptions& options) {
    if (group_index >= groups_.size()) {
      throw std::runtime_error("pluss::ai::nn::optim::NAdam::set_group_options: group index out of range");
    }
    validate_options_(options);
    groups_[group_index].options = options;
  }

  std::size_t group_count() const noexcept { return groups_.size(); }

private:
  struct GroupState {
    std::vector<std::size_t> indices{};
    NAdamOptions options{};
    double mu_product{1.0};
  };

  void apply_step_(const std::vector<Series>& gradients) override {
    ++step_;
    auto& params = mutable_parameters_();

    for (auto& group : groups_) {
      const NAdamOptions& options = group.options;
      const double mu_t = options.beta1 * (1.0 - 0.5 * std::pow(0.96, static_cast<double>(step_) * options.momentum_decay));
      const double mu_t_next =
          options.beta1 * (1.0 - 0.5 * std::pow(0.96, static_cast<double>(step_ + 1) * options.momentum_decay));
      group.mu_product *= mu_t;
      const double mu_product_next = group.mu_product * mu_t_next;
      const double one_minus_mu_prod = 1.0 - group.mu_product;
      const double one_minus_mu_prod_next = 1.0 - mu_product_next;
      const double one_minus_beta2_pow = 1.0 - std::pow(options.beta2, static_cast<double>(step_));

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
          if (options.decoupled_weight_decay) {
            parameter_value = parameter_value.mul(1.0 - options.learning_rate * options.weight_decay);
          } else {
            sensitivity = sensitivity.add(parameter_value.mul(options.weight_decay));
          }
        }

        exp_avg_[i] = exp_avg_[i].mul(options.beta1).add(sensitivity.mul(1.0 - options.beta1));
        exp_avg_sq_[i] = exp_avg_sq_[i].mul(options.beta2).add(sensitivity.square().mul(1.0 - options.beta2));

        const Series denom = exp_avg_sq_[i].div(one_minus_beta2_pow).sqrt().add(options.epsilon);
        const Series g_hat = sensitivity.div(one_minus_mu_prod);
        const Series m_hat = exp_avg_[i].div(one_minus_mu_prod_next);
        const Series nesterov_m = g_hat.mul(1.0 - mu_t).add(m_hat.mul(mu_t_next));
        const Series delta = nesterov_m.div(denom).mul(options.learning_rate);

        assign_parameter_(*param_ptr, parameter_value.sub(delta));
      }
    }
  }

  static void validate_options_(const NAdamOptions& options) {
    if (!(options.learning_rate > 0.0) || !std::isfinite(options.learning_rate)) {
      throw std::runtime_error("pluss::ai::nn::optim::NAdam: learning_rate must be positive and finite");
    }
    if (!(options.beta1 >= 0.0 && options.beta1 < 1.0) || !std::isfinite(options.beta1)) {
      throw std::runtime_error("pluss::ai::nn::optim::NAdam: beta1 must be in [0, 1)");
    }
    if (!(options.beta2 >= 0.0 && options.beta2 < 1.0) || !std::isfinite(options.beta2)) {
      throw std::runtime_error("pluss::ai::nn::optim::NAdam: beta2 must be in [0, 1)");
    }
    if (!(options.epsilon > 0.0) || !std::isfinite(options.epsilon)) {
      throw std::runtime_error("pluss::ai::nn::optim::NAdam: epsilon must be positive and finite");
    }
    if (options.weight_decay < 0.0 || !std::isfinite(options.weight_decay)) {
      throw std::runtime_error("pluss::ai::nn::optim::NAdam: weight_decay must be finite and non-negative");
    }
    if (options.momentum_decay < 0.0 || !std::isfinite(options.momentum_decay)) {
      throw std::runtime_error("pluss::ai::nn::optim::NAdam: momentum_decay must be finite and non-negative");
    }
  }

  static std::vector<Series*> flatten_groups_(const std::vector<NAdamGroup>& groups) {
    if (groups.empty()) {
      throw std::runtime_error("pluss::ai::nn::optim::NAdam: expected non-empty parameter groups");
    }
    std::vector<Series*> flat{};
    std::unordered_set<Series*> seen{};
    for (const auto& group : groups) {
      if (group.parameters.empty()) {
        throw std::runtime_error("pluss::ai::nn::optim::NAdam: parameter group must not be empty");
      }
      for (Series* p : group.parameters) {
        if (p != nullptr && !seen.insert(p).second) {
          throw std::runtime_error("pluss::ai::nn::optim::NAdam: duplicate parameter across groups");
        }
        flat.push_back(p);
      }
    }
    return flat;
  }

  static std::vector<GroupState> build_group_states_(const std::vector<NAdamGroup>& groups) {
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
    exp_avg_.clear();
    exp_avg_sq_.clear();
    exp_avg_.reserve(params.size());
    exp_avg_sq_.reserve(params.size());
    for (const Series* p : params) {
      exp_avg_.push_back(pluss::zeros_like(*p, p->scalar_type()));
      exp_avg_sq_.push_back(pluss::zeros_like(*p, p->scalar_type()));
    }
  }

  std::vector<GroupState> groups_{};
  NAdamOptions default_options_{};
  std::vector<Series> exp_avg_{};
  std::vector<Series> exp_avg_sq_{};
  std::int64_t step_{0};
};

} // namespace pluss::ai::nn::optim

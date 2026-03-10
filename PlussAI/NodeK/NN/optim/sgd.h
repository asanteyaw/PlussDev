#pragma once

#include "optim_base.h"

#include <cmath>
#include <stdexcept>
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

class PLUSS_API SGD final : public Optimizer {
public:
  explicit SGD(std::vector<Series*> parameters, SGDOptions options = {})
      : Optimizer(std::move(parameters)), options_(options) {
    validate_options_();
    initialize_state_();
  }

  explicit SGD(Module& module, SGDOptions options = {})
      : SGD(module.parameters(), options) {}

  const SGDOptions& options() const noexcept { return options_; }

  void set_options(const SGDOptions& options) {
    options_ = options;
    validate_options_();
    initialize_state_();
  }

private:
  void apply_step_(const std::vector<Series>& gradients) override {
    auto& params = mutable_parameters_();
    for (std::size_t i = 0; i < params.size(); ++i) {
      Series* param_ptr = params[i];
      if (param_ptr == nullptr) continue;

      const Series& input_sensitivity = gradients[i];
      if (!input_sensitivity.defined()) continue;

      Series sensitivity = clip_gradient_(input_sensitivity, options_.clip);
      if (options_.maximize) {
        sensitivity = sensitivity.neg();
      }

      Series parameter_value = param_ptr->detach();
      if (options_.weight_decay != 0.0) {
        sensitivity = sensitivity.add(parameter_value.mul(options_.weight_decay));
      }

      Series update = sensitivity;
      if (options_.momentum > 0.0) {
        velocity_[i] = velocity_[i].mul(options_.momentum).add(sensitivity.mul(1.0 - options_.dampening));
        if (options_.nesterov) {
          update = sensitivity.add(velocity_[i].mul(options_.momentum));
        } else {
          update = velocity_[i];
        }
      }

      const Series next_value = parameter_value.sub(update.mul(options_.learning_rate));
      assign_parameter_(*param_ptr, next_value);
    }
  }

  void validate_options_() const {
    if (!(options_.learning_rate > 0.0) || !std::isfinite(options_.learning_rate)) {
      throw std::runtime_error("pluss::ai::nn::optim::SGD: learning_rate must be positive and finite");
    }
    if (options_.momentum < 0.0 || !std::isfinite(options_.momentum)) {
      throw std::runtime_error("pluss::ai::nn::optim::SGD: momentum must be finite and non-negative");
    }
    if (options_.dampening < 0.0 || !std::isfinite(options_.dampening)) {
      throw std::runtime_error("pluss::ai::nn::optim::SGD: dampening must be finite and non-negative");
    }
    if (options_.weight_decay < 0.0 || !std::isfinite(options_.weight_decay)) {
      throw std::runtime_error("pluss::ai::nn::optim::SGD: weight_decay must be finite and non-negative");
    }
    if (options_.nesterov && options_.momentum <= 0.0) {
      throw std::runtime_error("pluss::ai::nn::optim::SGD: nesterov requires momentum > 0");
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

  SGDOptions options_{};
  std::vector<Series> velocity_{};
};

} // namespace pluss::ai::nn::optim

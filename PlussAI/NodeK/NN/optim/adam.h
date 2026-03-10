#pragma once

#include "optim_base.h"

#include <cmath>
#include <cstdint>
#include <stdexcept>
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

class PLUSS_API Adam final : public Optimizer {
public:
  explicit Adam(std::vector<Series*> parameters, AdamOptions options = {})
      : Optimizer(std::move(parameters)), options_(options) {
    validate_options_();
    initialize_state_();
  }

  explicit Adam(Module& module, AdamOptions options = {})
      : Adam(module.parameters(), options) {}

  const AdamOptions& options() const noexcept { return options_; }

  void set_options(const AdamOptions& options) {
    options_ = options;
    validate_options_();
  }

private:
  void apply_step_(const std::vector<Series>& gradients) override {
    ++step_;
    const double b1_correction = 1.0 - std::pow(options_.beta1, static_cast<double>(step_));
    const double b2_correction = 1.0 - std::pow(options_.beta2, static_cast<double>(step_));

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

      first_moment_[i] = first_moment_[i].mul(options_.beta1).add(sensitivity.mul(1.0 - options_.beta1));
      second_moment_[i] = second_moment_[i].mul(options_.beta2).add(sensitivity.square().mul(1.0 - options_.beta2));

      Series second_hat = second_moment_[i].div(b2_correction);
      if (options_.amsgrad) {
        max_second_hat_[i] = max_second_hat_[i].maximum(second_hat);
        second_hat = max_second_hat_[i];
      }

      const Series first_hat = first_moment_[i].div(b1_correction);
      const Series denominator = second_hat.sqrt().add(options_.epsilon);
      const Series delta = first_hat.div(denominator).mul(options_.learning_rate);

      assign_parameter_(*param_ptr, parameter_value.sub(delta));
    }
  }

  void validate_options_() const {
    if (!(options_.learning_rate > 0.0) || !std::isfinite(options_.learning_rate)) {
      throw std::runtime_error("pluss::ai::nn::optim::Adam: learning_rate must be positive and finite");
    }
    if (!(options_.beta1 >= 0.0 && options_.beta1 < 1.0) || !std::isfinite(options_.beta1)) {
      throw std::runtime_error("pluss::ai::nn::optim::Adam: beta1 must be in [0, 1)");
    }
    if (!(options_.beta2 >= 0.0 && options_.beta2 < 1.0) || !std::isfinite(options_.beta2)) {
      throw std::runtime_error("pluss::ai::nn::optim::Adam: beta2 must be in [0, 1)");
    }
    if (!(options_.epsilon > 0.0) || !std::isfinite(options_.epsilon)) {
      throw std::runtime_error("pluss::ai::nn::optim::Adam: epsilon must be positive and finite");
    }
    if (options_.weight_decay < 0.0 || !std::isfinite(options_.weight_decay)) {
      throw std::runtime_error("pluss::ai::nn::optim::Adam: weight_decay must be finite and non-negative");
    }
  }

  void initialize_state_() {
    const auto& params = parameters();
    first_moment_.clear();
    second_moment_.clear();
    max_second_hat_.clear();

    first_moment_.reserve(params.size());
    second_moment_.reserve(params.size());
    if (options_.amsgrad) {
      max_second_hat_.reserve(params.size());
    }

    for (const Series* p : params) {
      first_moment_.push_back(pluss::zeros_like(*p, p->scalar_type()));
      second_moment_.push_back(pluss::zeros_like(*p, p->scalar_type()));
      if (options_.amsgrad) {
        max_second_hat_.push_back(pluss::zeros_like(*p, p->scalar_type()));
      }
    }
  }

  AdamOptions options_{};
  std::vector<Series> first_moment_{};
  std::vector<Series> second_moment_{};
  std::vector<Series> max_second_hat_{};
  std::int64_t step_{0};
};

} // namespace pluss::ai::nn::optim

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
  ExecutionOptions execution{};
};

struct AdamWGroup {
  std::vector<Series*> parameters{};
  AdamWOptions options{};
};

class PLUSS_API AdamW final : public Optimizer {
public:
  explicit AdamW(pluss::msharp::RangeView::Of<Series*> parameters, AdamWOptions options = {})
      : Optimizer(parameters), default_options_(options) {
    validate_options_(default_options_);
    GroupState group{};
    group.options = default_options_;
    group.indices.resize(size());
    std::iota(group.indices.begin(), group.indices.end(), static_cast<std::size_t>(0));
    groups_.push_back(std::move(group));
    initialize_state_();
  }

  explicit AdamW(pluss::msharp::RangeView::Of<AdamWGroup> groups)
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
      ::pluss::throw_error("pluss::ai::nn::optim::AdamW::set_group_options: group index out of range");
    }
    validate_options_(options);
    groups_[group_index].options = options;
  }

  std::size_t group_count() const noexcept { return groups_.size(); }

  struct State {
    std::int64_t step{0};
    std::vector<AdamWOptions> group_options{};
    std::vector<Series> beta1_power{};
    std::vector<Series> beta2_power{};
    std::vector<Series> first_moment{};
    std::vector<Series> second_moment{};
    std::vector<Series> max_second_hat{};
  };

  State capture_state() const {
    State state{};
    state.step = step_;
    state.group_options.reserve(groups_.size());
    for (const auto& group : groups_) {
      state.group_options.push_back(group.options);
      state.beta1_power.push_back(group.beta1_power);
      state.beta2_power.push_back(group.beta2_power);
    }
    state.first_moment = first_moment_;
    state.second_moment = second_moment_;
    state.max_second_hat = max_second_hat_;
    return state;
  }

  void restore_state(const State& state) {
    if (state.group_options.size() != groups_.size()) {
      ::pluss::throw_error("pluss::ai::nn::optim::AdamW::restore_state: group count mismatch");
    }
    if (state.beta1_power.size() != groups_.size() || state.beta2_power.size() != groups_.size()) {
      ::pluss::throw_error("pluss::ai::nn::optim::AdamW::restore_state: capture scalar state mismatch");
    }
    validate_state_vector_(state.first_moment, "pluss::ai::nn::optim::AdamW::restore_state[first_moment]");
    validate_state_vector_(state.second_moment, "pluss::ai::nn::optim::AdamW::restore_state[second_moment]");
    validate_state_vector_(state.max_second_hat, "pluss::ai::nn::optim::AdamW::restore_state[max_second_hat]");
    for (std::size_t i = 0; i < groups_.size(); ++i) {
      validate_options_(state.group_options[i]);
      groups_[i].options = state.group_options[i];
      groups_[i].beta1_power = state.beta1_power[i];
      groups_[i].beta2_power = state.beta2_power[i];
    }
    validate_capturable_groups_();
    step_ = state.step;
    first_moment_ = state.first_moment;
    second_moment_ = state.second_moment;
    max_second_hat_ = state.max_second_hat;
  }

private:
  struct GroupState {
    std::vector<std::size_t> indices{};
    AdamWOptions options{};
    Series beta1_power{};
    Series beta2_power{};
  };

  void apply_step_(pluss::msharp::RangeView::Series gradients) override {
    ++step_;

    auto& params = mutable_parameters_();
    reset_step_lane_telemetry_();
    for (std::size_t group_index = 0; group_index < groups_.size(); ++group_index) {
      auto& group = groups_[group_index];
      const AdamWOptions& options = group.options;
      const bool batch_shape_compatible = can_batch_group_(group, params, gradients);
      const bool batched_supported =
          batch_shape_compatible && supports_batched_lane_(group.indices, params, gradients);
      const bool fused_supported =
          batch_shape_compatible && supports_fused_lane_(group.indices, params, gradients);
      const bool capturable_supported =
          supports_capturable_lane_(group.indices, params, gradients);
      const UpdateLane lane = resolve_update_lane_(
          options.execution,
          batched_supported,
          fused_supported,
          "pluss::ai::nn::optim::AdamW",
          capturable_supported);
      append_step_lane_telemetry_(
          group_index,
          options.execution.lane,
          lane,
          group.indices,
          params,
          gradients,
          batch_shape_compatible,
          batched_supported,
          fused_supported,
          capturable_supported);
      if (lane == UpdateLane::Fused) {
        apply_group_fused_(group, params, gradients);
      } else if (lane == UpdateLane::Batched || lane == UpdateLane::Foreach) {
        apply_group_batched_(group, params, gradients);
      } else {
        apply_group_loop_(group, params, gradients);
      }
    }
  }

  static void validate_options_(const AdamWOptions& options) {
    if (!(options.learning_rate > 0.0) || !std::isfinite(options.learning_rate)) {
      ::pluss::throw_error("pluss::ai::nn::optim::AdamW: learning_rate must be positive and finite");
    }
    if (!(options.beta1 >= 0.0 && options.beta1 < 1.0) || !std::isfinite(options.beta1)) {
      ::pluss::throw_error("pluss::ai::nn::optim::AdamW: beta1 must be in [0, 1)");
    }
    if (!(options.beta2 >= 0.0 && options.beta2 < 1.0) || !std::isfinite(options.beta2)) {
      ::pluss::throw_error("pluss::ai::nn::optim::AdamW: beta2 must be in [0, 1)");
    }
    if (!(options.epsilon > 0.0) || !std::isfinite(options.epsilon)) {
      ::pluss::throw_error("pluss::ai::nn::optim::AdamW: epsilon must be positive and finite");
    }
    if (options.weight_decay < 0.0 || !std::isfinite(options.weight_decay)) {
      ::pluss::throw_error("pluss::ai::nn::optim::AdamW: weight_decay must be finite and non-negative");
    }
  }

  static std::vector<Series*> flatten_groups_(pluss::msharp::RangeView::Of<AdamWGroup> groups) {
    if (groups.empty()) {
      ::pluss::throw_error("pluss::ai::nn::optim::AdamW: expected non-empty parameter groups");
    }
    std::vector<Series*> flat{};
    std::unordered_set<Series*> seen{};
    for (const auto& group : groups) {
      if (group.parameters.empty()) {
        ::pluss::throw_error("pluss::ai::nn::optim::AdamW: parameter group must not be empty");
      }
      for (Series* p : group.parameters) {
        if (p != nullptr && !seen.insert(p).second) {
          ::pluss::throw_error("pluss::ai::nn::optim::AdamW: duplicate parameter across groups");
        }
        flat.push_back(p);
      }
    }
    return flat;
  }

  static std::vector<GroupState> build_group_states_(pluss::msharp::RangeView::Of<AdamWGroup> groups) {
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
    validate_capturable_groups_();
  }

  void validate_capturable_groups_() const {
    const auto& params = parameters();
    for (const auto& group : groups_) {
      if (!group.options.execution.capturable) continue;
      if (group.indices.empty()) continue;
      const Series* anchor = params[group.indices.front()];
      if (anchor == nullptr || !anchor->defined()) {
        ::pluss::throw_error("pluss::ai::nn::optim::AdamW: capturable mode expects defined group parameters");
      }
      for (std::size_t i : group.indices) {
        const Series* p = params[i];
        if (p == nullptr || !p->defined()) continue;
        if (p->platform() != anchor->platform()) {
          ::pluss::throw_error(
              "pluss::ai::nn::optim::AdamW: capturable mode requires same platform across each group");
        }
        if (p->scalar_type() != anchor->scalar_type()) {
          ::pluss::throw_error(
              "pluss::ai::nn::optim::AdamW: capturable mode requires same dtype across each group");
        }
      }
    }
  }

  static bool same_shape_(const Series& a, const Series& b) {
    if (a.sizes().size() != b.sizes().size()) return false;
    for (std::size_t i = 0; i < a.sizes().size(); ++i) {
      if (a.sizes()[i] != b.sizes()[i]) return false;
    }
    return true;
  }

  bool can_batch_group_(const GroupState& group,
                        pluss::msharp::RangeView::Of<Series*> params,
                        pluss::msharp::RangeView::Series gradients) const {
    const Series* anchor_param = nullptr;
    for (std::size_t i : group.indices) {
      const Series* p = params[i];
      if (p == nullptr || !p->defined()) continue;
      const Series& g = gradients[i];
      if (!g.defined()) continue;
      if (anchor_param == nullptr) {
        anchor_param = p;
      } else {
        if (p->platform() != anchor_param->platform()) return false;
        if (p->scalar_type() != anchor_param->scalar_type()) return false;
        if (!same_shape_(*p, *anchor_param)) return false;
      }
      if (!same_shape_(g, *p)) return false;
    }
    return anchor_param != nullptr;
  }

  static std::vector<std::int64_t> batched_shape_(const Series& x) {
    return with_batch_axis_(x);
  }

  void advance_capturable_scalars_(GroupState& group) {
    if (!group.options.execution.capturable) return;
    group.beta1_power = group.beta1_power.mul(group.options.beta1);
    group.beta2_power = group.beta2_power.mul(group.options.beta2);
  }

  void apply_group_loop_(GroupState& group,
                         pluss::msharp::RangeView::Of<Series*> params,
                         pluss::msharp::RangeView::Series gradients) {
    const AdamWOptions& options = group.options;
    advance_capturable_scalars_(group);
    const double b1_correction = 1.0 - std::pow(options.beta1, static_cast<double>(step_));
    const double b2_correction = 1.0 - std::pow(options.beta2, static_cast<double>(step_));

    Series b1_corr_series;
    Series b2_corr_series;
    if (options.execution.capturable) {
      const Series one_b1 = scalar_on_(1.0, group.beta1_power);
      const Series one_b2 = scalar_on_(1.0, group.beta2_power);
      b1_corr_series = one_b1.sub(group.beta1_power);
      b2_corr_series = one_b2.sub(group.beta2_power);
    }

    for (std::size_t i : group.indices) {
      Series* param_ptr = params[i];
      if (param_ptr == nullptr) continue;
      const Series& input_sensitivity = gradients[i];
      if (!input_sensitivity.defined()) continue;

      Series sensitivity = clip_gradient_(input_sensitivity, options.clip);
      if (options.maximize) {
        sensitivity = sensitivity.neg();
      }

      Series parameter_value = *param_ptr;
      const Series one = scalar_on_(1.0, parameter_value);
      const Series beta1 = scalar_on_(options.beta1, parameter_value);
      const Series beta2 = scalar_on_(options.beta2, parameter_value);
      const Series one_minus_beta1 = one.sub(beta1);
      const Series one_minus_beta2 = one.sub(beta2);
      if (options.weight_decay != 0.0) {
        const Series decay = scalar_on_(options.learning_rate * options.weight_decay, parameter_value);
        parameter_value = parameter_value.mul(one.sub(decay));
      }

      first_moment_[i] = first_moment_[i].mul(beta1).add(sensitivity.mul(one_minus_beta1));
      second_moment_[i] = second_moment_[i].mul(beta2).add(sensitivity.square().mul(one_minus_beta2));

      Series second_hat = options.execution.capturable
                              ? second_moment_[i].div(b2_corr_series)
                              : second_moment_[i].div(scalar_on_(b2_correction, parameter_value));
      if (options.amsgrad) {
        max_second_hat_[i] = max_second_hat_[i].maximum(second_hat);
        second_hat = max_second_hat_[i];
      }

      const Series first_hat = options.execution.capturable
                                   ? first_moment_[i].div(b1_corr_series)
                                   : first_moment_[i].div(scalar_on_(b1_correction, parameter_value));
      const Series denominator = second_hat.sqrt().add(scalar_on_(options.epsilon, parameter_value));
      const Series delta = first_hat.div(denominator).mul(scalar_on_(options.learning_rate, parameter_value));
      assign_parameter_(*param_ptr, parameter_value.sub(delta));
    }
  }

  void apply_group_batched_(GroupState& group,
                            pluss::msharp::RangeView::Of<Series*> params,
                            pluss::msharp::RangeView::Series gradients) {
    const AdamWOptions& options = group.options;
    advance_capturable_scalars_(group);
    const double b1_correction = 1.0 - std::pow(options.beta1, static_cast<double>(step_));
    const double b2_correction = 1.0 - std::pow(options.beta2, static_cast<double>(step_));

    std::vector<std::size_t> active{};
    std::vector<Series> p_parts{};
    std::vector<Series> g_parts{};
    std::vector<Series> m1_parts{};
    std::vector<Series> m2_parts{};
    std::vector<Series> mmax_parts{};

    for (std::size_t i : group.indices) {
      Series* param_ptr = params[i];
      if (param_ptr == nullptr) continue;
      const Series& input_sensitivity = gradients[i];
      if (!input_sensitivity.defined()) continue;

      Series sensitivity = clip_gradient_(input_sensitivity, options.clip);
      if (options.maximize) sensitivity = sensitivity.neg();

      active.push_back(i);
      p_parts.push_back(param_ptr->reshape(batched_shape_(*param_ptr)));
      g_parts.push_back(sensitivity.reshape(batched_shape_(sensitivity)));
      m1_parts.push_back(first_moment_[i].reshape(batched_shape_(first_moment_[i])));
      m2_parts.push_back(second_moment_[i].reshape(batched_shape_(second_moment_[i])));
      mmax_parts.push_back(max_second_hat_[i].reshape(batched_shape_(max_second_hat_[i])));
    }
    if (active.empty()) return;

    Series p_batch = pack_batched_(p_parts, "pluss::ai::nn::optim::AdamW::apply_group_batched_");
    Series g_batch = pack_batched_(g_parts, "pluss::ai::nn::optim::AdamW::apply_group_batched_");
    Series m1_batch = pack_batched_(m1_parts, "pluss::ai::nn::optim::AdamW::apply_group_batched_");
    Series m2_batch = pack_batched_(m2_parts, "pluss::ai::nn::optim::AdamW::apply_group_batched_");
    Series mmax_batch = pack_batched_(mmax_parts, "pluss::ai::nn::optim::AdamW::apply_group_batched_");
    const Series one = scalar_on_(1.0, p_batch);
    const Series beta1 = scalar_on_(options.beta1, p_batch);
    const Series beta2 = scalar_on_(options.beta2, p_batch);
    const Series one_minus_beta1 = one.sub(beta1);
    const Series one_minus_beta2 = one.sub(beta2);

    if (options.weight_decay != 0.0) {
      const Series decay = scalar_on_(options.learning_rate * options.weight_decay, p_batch);
      p_batch = p_batch.mul(one.sub(decay));
    }
    m1_batch = m1_batch.mul(beta1).add(g_batch.mul(one_minus_beta1));
    m2_batch = m2_batch.mul(beta2).add(g_batch.square().mul(one_minus_beta2));

    Series second_hat = options.execution.capturable
                            ? m2_batch.div(scalar_on_(1.0, group.beta2_power).sub(group.beta2_power))
                            : m2_batch.div(scalar_on_(b2_correction, p_batch));
    if (options.amsgrad) {
      mmax_batch = mmax_batch.maximum(second_hat);
      second_hat = mmax_batch;
    }
    const Series first_hat = options.execution.capturable
                                 ? m1_batch.div(scalar_on_(1.0, group.beta1_power).sub(group.beta1_power))
                                 : m1_batch.div(scalar_on_(b1_correction, p_batch));
    const Series delta = first_hat.div(second_hat.sqrt().add(scalar_on_(options.epsilon, p_batch)))
                             .mul(scalar_on_(options.learning_rate, p_batch));
    const Series next_batch = p_batch.sub(delta);

    for (std::size_t j = 0; j < active.size(); ++j) {
      const std::size_t i = active[j];
      Series* param_ptr = params[i];
      Series next_value = next_batch.select(0, static_cast<std::int64_t>(j)).reshape(param_ptr->sizes());
      first_moment_[i] = m1_batch.select(0, static_cast<std::int64_t>(j)).reshape(first_moment_[i].sizes());
      second_moment_[i] = m2_batch.select(0, static_cast<std::int64_t>(j)).reshape(second_moment_[i].sizes());
      max_second_hat_[i] = mmax_batch.select(0, static_cast<std::int64_t>(j)).reshape(max_second_hat_[i].sizes());
      assign_parameter_(*param_ptr, next_value);
    }
  }

  void apply_group_fused_(GroupState& group,
                          pluss::msharp::RangeView::Of<Series*> params,
                          pluss::msharp::RangeView::Series gradients) {
    const AdamWOptions& options = group.options;
    advance_capturable_scalars_(group);
    const double b1_correction = 1.0 - std::pow(options.beta1, static_cast<double>(step_));
    const double b2_correction = 1.0 - std::pow(options.beta2, static_cast<double>(step_));

    std::vector<std::size_t> active{};
    std::vector<Series> p_parts{};
    std::vector<Series> g_parts{};
    std::vector<Series> m1_parts{};
    std::vector<Series> m2_parts{};
    std::vector<Series> mmax_parts{};

    for (std::size_t i : group.indices) {
      Series* param_ptr = params[i];
      if (param_ptr == nullptr) continue;
      const Series& input_sensitivity = gradients[i];
      if (!input_sensitivity.defined()) continue;

      Series sensitivity = clip_gradient_(input_sensitivity, options.clip);
      if (options.maximize) sensitivity = sensitivity.neg();

      active.push_back(i);
      p_parts.push_back(param_ptr->reshape(batched_shape_(*param_ptr)));
      g_parts.push_back(sensitivity.reshape(batched_shape_(sensitivity)));
      m1_parts.push_back(first_moment_[i].reshape(batched_shape_(first_moment_[i])));
      m2_parts.push_back(second_moment_[i].reshape(batched_shape_(second_moment_[i])));
      mmax_parts.push_back(max_second_hat_[i].reshape(batched_shape_(max_second_hat_[i])));
    }
    if (active.empty()) return;

    Series p_batch = pack_batched_(p_parts, "pluss::ai::nn::optim::AdamW::apply_group_fused_");
    Series g_batch = pack_batched_(g_parts, "pluss::ai::nn::optim::AdamW::apply_group_fused_");
    Series m1_batch = pack_batched_(m1_parts, "pluss::ai::nn::optim::AdamW::apply_group_fused_");
    Series m2_batch = pack_batched_(m2_parts, "pluss::ai::nn::optim::AdamW::apply_group_fused_");
    Series mmax_batch = pack_batched_(mmax_parts, "pluss::ai::nn::optim::AdamW::apply_group_fused_");
    const Series one = scalar_on_(1.0, p_batch);
    const Series beta1 = scalar_on_(options.beta1, p_batch);
    const Series beta2 = scalar_on_(options.beta2, p_batch);
    const Series one_minus_beta1 = one.sub(beta1);
    const Series one_minus_beta2 = one.sub(beta2);

    if (options.weight_decay != 0.0) {
      const Series decay = scalar_on_(options.learning_rate * options.weight_decay, p_batch);
      p_batch = p_batch.mul(one.sub(decay));
    }
    m1_batch = m1_batch.mul(beta1);
    m1_batch = m1_batch.add(g_batch.mul(one_minus_beta1));
    m2_batch = m2_batch.mul(beta2);
    m2_batch = m2_batch.add(g_batch.square().mul(one_minus_beta2));

    Series second_hat = options.execution.capturable
                            ? m2_batch.div(scalar_on_(1.0, group.beta2_power).sub(group.beta2_power))
                            : m2_batch.div(scalar_on_(b2_correction, p_batch));
    if (options.amsgrad) {
      mmax_batch = mmax_batch.maximum(second_hat);
      second_hat = mmax_batch;
    }
    const Series first_hat = options.execution.capturable
                                 ? m1_batch.div(scalar_on_(1.0, group.beta1_power).sub(group.beta1_power))
                                 : m1_batch.div(scalar_on_(b1_correction, p_batch));
    Series denominator = second_hat.sqrt();
    denominator = denominator.add(scalar_on_(options.epsilon, p_batch));
    Series delta = first_hat.div(denominator);
    delta = delta.mul(scalar_on_(options.learning_rate, p_batch));
    p_batch = p_batch.sub(delta);

    for (std::size_t j = 0; j < active.size(); ++j) {
      const std::size_t i = active[j];
      Series* param_ptr = params[i];
      Series next_value = p_batch.select(0, static_cast<std::int64_t>(j)).reshape(param_ptr->sizes());
      first_moment_[i] = m1_batch.select(0, static_cast<std::int64_t>(j)).reshape(first_moment_[i].sizes());
      second_moment_[i] = m2_batch.select(0, static_cast<std::int64_t>(j)).reshape(second_moment_[i].sizes());
      max_second_hat_[i] = mmax_batch.select(0, static_cast<std::int64_t>(j)).reshape(max_second_hat_[i].sizes());
      assign_parameter_(*param_ptr, next_value);
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
      first_moment_.push_back(state_zeros_like_(*p));
      second_moment_.push_back(state_zeros_like_(*p));
      max_second_hat_.push_back(state_zeros_like_(*p));
    }
    initialize_group_capture_state_();
  }

  void initialize_group_capture_state_() {
    const auto& params = parameters();
    for (auto& group : groups_) {
      group.beta1_power = Series{};
      group.beta2_power = Series{};
      if (!group.options.execution.capturable) continue;
      if (group.indices.empty()) continue;
      const Series* anchor = params[group.indices.front()];
      group.beta1_power = Series::full({}, 1.0, anchor->scalar_type()).to(anchor->platform());
      group.beta2_power = Series::full({}, 1.0, anchor->scalar_type()).to(anchor->platform());
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

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

struct SGDOptions {
  double learning_rate{1e-2};
  double momentum{0.0};
  double dampening{0.0};
  double weight_decay{0.0};
  bool nesterov{false};
  bool maximize{false};
  GradientClipOptions clip{};
  ExecutionOptions execution{};
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
    validate_group_options_();
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
    validate_group_options_();
    initialize_state_();
  }

  void set_group_options(std::size_t group_index, const SGDOptions& options) {
    if (group_index >= groups_.size()) {
      throw std::runtime_error("pluss::ai::nn::optim::SGD::set_group_options: group index out of range");
    }
    validate_options_(options);
    groups_[group_index].options = options;
    validate_group_options_();
    initialize_group_capture_state_();
  }

  std::size_t group_count() const noexcept { return groups_.size(); }

  struct State {
    std::vector<SGDOptions> group_options{};
    std::vector<Series> velocity{};
  };

  State capture_state() const {
    State state{};
    state.group_options.reserve(groups_.size());
    for (const auto& group : groups_) {
      state.group_options.push_back(group.options);
    }
    state.velocity = velocity_;
    return state;
  }

  void restore_state(const State& state) {
    if (state.group_options.size() != groups_.size()) {
      throw std::runtime_error("pluss::ai::nn::optim::SGD::restore_state: group count mismatch");
    }
    validate_state_vector_(state.velocity, "pluss::ai::nn::optim::SGD::restore_state");
    for (std::size_t i = 0; i < groups_.size(); ++i) {
      validate_options_(state.group_options[i]);
      groups_[i].options = state.group_options[i];
    }
    validate_group_options_();
    velocity_ = state.velocity;
    initialize_group_capture_state_();
  }

private:
  struct GroupState {
    std::vector<std::size_t> indices{};
    SGDOptions options{};
    Series one_scalar{};
    Series learning_rate_scalar{};
    Series momentum_scalar{};
    Series dampening_scalar{};
    Series weight_decay_scalar{};
  };

  struct GroupScalars {
    Series one{};
    Series learning_rate{};
    Series momentum{};
    Series dampening{};
    Series weight_decay{};
  };

  void apply_step_(const std::vector<Series>& gradients) override {
    auto& params = mutable_parameters_();
    reset_step_lane_telemetry_();
    for (std::size_t group_index = 0; group_index < groups_.size(); ++group_index) {
      const auto& group = groups_[group_index];
      const bool batch_shape_compatible = can_batch_group_(group, params, gradients);
      const bool batched_supported =
          batch_shape_compatible && supports_batched_lane_(group.indices, params, gradients);
      const bool fused_supported =
          batch_shape_compatible && supports_fused_lane_(group.indices, params, gradients);
      const bool capturable_supported =
          supports_capturable_lane_(group.indices, params, gradients);
      const UpdateLane lane = resolve_update_lane_(
          group.options.execution,
          batched_supported,
          fused_supported,
          "pluss::ai::nn::optim::SGD",
          capturable_supported);
      append_step_lane_telemetry_(
          group_index,
          group.options.execution.lane,
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

  static bool same_shape_(const Series& a, const Series& b) {
    if (a.sizes().size() != b.sizes().size()) return false;
    for (std::size_t i = 0; i < a.sizes().size(); ++i) {
      if (a.sizes()[i] != b.sizes()[i]) return false;
    }
    return true;
  }

  bool can_batch_group_(const GroupState& group,
                        const std::vector<Series*>& params,
                        const std::vector<Series>& gradients) const {
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

  GroupScalars resolve_group_scalars_(const GroupState& group, const Series& like) const {
    const bool capturable = group.options.execution.capturable;
    GroupScalars out{};
    if (capturable && group.one_scalar.defined() && group.learning_rate_scalar.defined() &&
        group.momentum_scalar.defined() && group.dampening_scalar.defined() &&
        group.weight_decay_scalar.defined()) {
      out.one = group.one_scalar;
      out.learning_rate = group.learning_rate_scalar;
      out.momentum = group.momentum_scalar;
      out.dampening = group.dampening_scalar;
      out.weight_decay = group.weight_decay_scalar;
      return out;
    }
    out.one = scalar_on_(1.0, like);
    out.learning_rate = scalar_on_(group.options.learning_rate, like);
    out.momentum = scalar_on_(group.options.momentum, like);
    out.dampening = scalar_on_(group.options.dampening, like);
    out.weight_decay = scalar_on_(group.options.weight_decay, like);
    return out;
  }

  void apply_group_loop_(const GroupState& group,
                         const std::vector<Series*>& params,
                         const std::vector<Series>& gradients) {
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

      Series parameter_value = *param_ptr;
      PLUSS_CHECK(
          parameter_value.platform() == param_ptr->platform(),
          "pluss::ai::nn::optim::SGD::apply_group_loop_: parameter/platform mismatch");
      PLUSS_CHECK(
          sensitivity.platform() == parameter_value.platform(),
          "pluss::ai::nn::optim::SGD::apply_group_loop_: sensitivity/platform mismatch");
      const GroupScalars scalars = resolve_group_scalars_(group, parameter_value);
      if (options.weight_decay != 0.0) {
        sensitivity = sensitivity.add(parameter_value.mul(scalars.weight_decay));
      }

      Series update = sensitivity;
      if (options.momentum > 0.0) {
        velocity_[i] = velocity_[i].mul(scalars.momentum).add(sensitivity.mul(scalars.one.sub(scalars.dampening)));
        if (options.nesterov) {
          update = sensitivity.add(velocity_[i].mul(scalars.momentum));
        } else {
          update = velocity_[i];
        }
      }

      const Series next_value = parameter_value.sub(update.mul(scalars.learning_rate));
      assign_parameter_(*param_ptr, next_value);
    }
  }

  void apply_group_batched_(const GroupState& group,
                            const std::vector<Series*>& params,
                            const std::vector<Series>& gradients) {
    const SGDOptions& options = group.options;

    std::vector<std::size_t> active{};
    std::vector<Series> p_parts{};
    std::vector<Series> g_parts{};
    std::vector<Series> v_parts{};

    for (std::size_t i : group.indices) {
      Series* param_ptr = params[i];
      if (param_ptr == nullptr) continue;
      const Series& input_sensitivity = gradients[i];
      if (!input_sensitivity.defined()) continue;

      Series sensitivity = clip_gradient_(input_sensitivity, options.clip);
      if (options.maximize) sensitivity = sensitivity.neg();
      Series parameter_value = *param_ptr;
      PLUSS_CHECK(
          parameter_value.platform() == param_ptr->platform(),
          "pluss::ai::nn::optim::SGD::apply_group_batched_: parameter/platform mismatch");
      PLUSS_CHECK(
          sensitivity.platform() == parameter_value.platform(),
          "pluss::ai::nn::optim::SGD::apply_group_batched_: sensitivity/platform mismatch");

      active.push_back(i);
      p_parts.push_back(parameter_value.reshape(batched_shape_(*param_ptr)));
      g_parts.push_back(sensitivity.reshape(batched_shape_(sensitivity)));
      v_parts.push_back(velocity_[i].reshape(batched_shape_(velocity_[i])));
    }
    if (active.empty()) return;

    Series p_batch = pack_batched_(p_parts, "pluss::ai::nn::optim::SGD::apply_group_batched_");
    Series g_batch = pack_batched_(g_parts, "pluss::ai::nn::optim::SGD::apply_group_batched_");
    Series v_batch = pack_batched_(v_parts, "pluss::ai::nn::optim::SGD::apply_group_batched_");
    const GroupScalars scalars = resolve_group_scalars_(group, p_batch);

    if (options.weight_decay != 0.0) {
      g_batch = g_batch.add(p_batch.mul(scalars.weight_decay));
    }

    Series update = g_batch;
    if (options.momentum > 0.0) {
      v_batch = v_batch.mul(scalars.momentum).add(g_batch.mul(scalars.one.sub(scalars.dampening)));
      update = options.nesterov ? g_batch.add(v_batch.mul(scalars.momentum)) : v_batch;
    }

    const Series next_batch = p_batch.sub(update.mul(scalars.learning_rate));
    for (std::size_t j = 0; j < active.size(); ++j) {
      const std::size_t i = active[j];
      Series* param_ptr = params[i];
      Series next_value = next_batch.select(0, static_cast<std::int64_t>(j)).reshape(param_ptr->sizes());
      velocity_[i] = v_batch.select(0, static_cast<std::int64_t>(j)).reshape(velocity_[i].sizes());
      assign_parameter_(*param_ptr, next_value);
    }
  }

  void apply_group_fused_(const GroupState& group,
                          const std::vector<Series*>& params,
                          const std::vector<Series>& gradients) {
    const SGDOptions& options = group.options;

    std::vector<std::size_t> active{};
    std::vector<Series> p_parts{};
    std::vector<Series> g_parts{};
    std::vector<Series> v_parts{};

    for (std::size_t i : group.indices) {
      Series* param_ptr = params[i];
      if (param_ptr == nullptr) continue;
      const Series& input_sensitivity = gradients[i];
      if (!input_sensitivity.defined()) continue;

      Series sensitivity = clip_gradient_(input_sensitivity, options.clip);
      if (options.maximize) sensitivity = sensitivity.neg();
      Series parameter_value = *param_ptr;

      active.push_back(i);
      p_parts.push_back(parameter_value.reshape(batched_shape_(*param_ptr)));
      g_parts.push_back(sensitivity.reshape(batched_shape_(sensitivity)));
      v_parts.push_back(velocity_[i].reshape(batched_shape_(velocity_[i])));
    }
    if (active.empty()) return;

    Series p_batch = pack_batched_(p_parts, "pluss::ai::nn::optim::SGD::apply_group_fused_");
    Series g_batch = pack_batched_(g_parts, "pluss::ai::nn::optim::SGD::apply_group_fused_");
    Series v_batch = pack_batched_(v_parts, "pluss::ai::nn::optim::SGD::apply_group_fused_");
    const GroupScalars scalars = resolve_group_scalars_(group, p_batch);

    if (options.weight_decay != 0.0) {
      g_batch = g_batch.add(p_batch.mul(scalars.weight_decay));
    }

    if (options.momentum > 0.0) {
      v_batch = v_batch.mul(scalars.momentum);
      v_batch = v_batch.add(g_batch.mul(scalars.one.sub(scalars.dampening)));
      if (options.nesterov) {
        g_batch = g_batch.add(v_batch.mul(scalars.momentum));
      } else {
        g_batch = v_batch;
      }
    }

    p_batch = p_batch.sub(g_batch.mul(scalars.learning_rate));
    for (std::size_t j = 0; j < active.size(); ++j) {
      const std::size_t i = active[j];
      Series* param_ptr = params[i];
      Series next_value = p_batch.select(0, static_cast<std::int64_t>(j)).reshape(param_ptr->sizes());
      velocity_[i] = v_batch.select(0, static_cast<std::int64_t>(j)).reshape(velocity_[i].sizes());
      assign_parameter_(*param_ptr, next_value);
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
    validate_capturable_groups_();
  }

  void validate_capturable_groups_() const {
    const auto& params = parameters();
    for (const auto& group : groups_) {
      if (!group.options.execution.capturable) continue;
      if (group.indices.empty()) continue;
      const Series* anchor = params[group.indices.front()];
      if (anchor == nullptr || !anchor->defined()) {
        throw std::runtime_error("pluss::ai::nn::optim::SGD: capturable mode expects defined group parameters");
      }
      for (std::size_t i : group.indices) {
        const Series* p = params[i];
        if (p == nullptr || !p->defined()) continue;
        if (p->platform() != anchor->platform()) {
          throw std::runtime_error(
              "pluss::ai::nn::optim::SGD: capturable mode requires same platform across each group");
        }
        if (p->scalar_type() != anchor->scalar_type()) {
          throw std::runtime_error(
              "pluss::ai::nn::optim::SGD: capturable mode requires same dtype across each group");
        }
      }
    }
  }

  void initialize_state_() {
    const auto& params = parameters();
    velocity_.clear();
    velocity_.reserve(params.size());
    for (const Series* p : params) {
      velocity_.push_back(pluss::zeros_like(*p, p->scalar_type()));
    }
    initialize_group_capture_state_();
  }

  void initialize_group_capture_state_() {
    const auto& params = parameters();
    for (auto& group : groups_) {
      group.one_scalar = Series{};
      group.learning_rate_scalar = Series{};
      group.momentum_scalar = Series{};
      group.dampening_scalar = Series{};
      group.weight_decay_scalar = Series{};
      if (!group.options.execution.capturable) continue;
      if (group.indices.empty()) continue;
      const Series* anchor = params[group.indices.front()];
      group.one_scalar = Series::full({}, 1.0, anchor->scalar_type()).to(anchor->platform());
      group.learning_rate_scalar =
          Series::full({}, group.options.learning_rate, anchor->scalar_type()).to(anchor->platform());
      group.momentum_scalar = Series::full({}, group.options.momentum, anchor->scalar_type()).to(anchor->platform());
      group.dampening_scalar =
          Series::full({}, group.options.dampening, anchor->scalar_type()).to(anchor->platform());
      group.weight_decay_scalar =
          Series::full({}, group.options.weight_decay, anchor->scalar_type()).to(anchor->platform());
    }
  }

  std::vector<GroupState> groups_{};
  SGDOptions default_options_{};
  std::vector<Series> velocity_{};
};

} // namespace pluss::ai::nn::optim

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

struct RMSpropOptions {
  double learning_rate{1e-2};
  double alpha{0.99};
  double epsilon{1e-8};
  double momentum{0.0};
  double weight_decay{0.0};
  bool centered{false};
  bool maximize{false};
  GradientClipOptions clip{};
  ExecutionOptions execution{};
};

struct RMSpropGroup {
  std::vector<Series*> parameters{};
  RMSpropOptions options{};
};

class PLUSS_API RMSprop final : public Optimizer {
public:
  explicit RMSprop(pluss::msharp::RangeView::Of<Series*> parameters, RMSpropOptions options = {})
      : Optimizer(parameters), default_options_(options) {
    validate_options_(default_options_);
    GroupState group{};
    group.options = default_options_;
    group.indices.resize(size());
    std::iota(group.indices.begin(), group.indices.end(), static_cast<std::size_t>(0));
    groups_.push_back(std::move(group));
    validate_group_options_();
    initialize_state_();
  }

  explicit RMSprop(pluss::msharp::RangeView::Of<RMSpropGroup> groups)
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
    validate_group_options_();
    initialize_state_();
  }

  void set_group_options(std::size_t group_index, const RMSpropOptions& options) {
    if (group_index >= groups_.size()) {
      ::pluss::throw_error("pluss::ai::nn::optim::RMSprop::set_group_options: group index out of range");
    }
    validate_options_(options);
    groups_[group_index].options = options;
    validate_group_options_();
    initialize_state_();
  }

  std::size_t group_count() const noexcept { return groups_.size(); }

  struct State {
    std::vector<RMSpropOptions> group_options{};
    std::vector<Series> square_avg{};
    std::vector<Series> momentum_buffer{};
    std::vector<Series> grad_avg{};
  };

  State capture_state() const {
    State state{};
    state.group_options.reserve(groups_.size());
    for (const auto& group : groups_) {
      state.group_options.push_back(group.options);
    }
    state.square_avg = square_avg_;
    state.momentum_buffer = momentum_buffer_;
    state.grad_avg = grad_avg_;
    return state;
  }

  void restore_state(const State& state) {
    if (state.group_options.size() != groups_.size()) {
      ::pluss::throw_error("pluss::ai::nn::optim::RMSprop::restore_state: group count mismatch");
    }
    validate_state_vector_(state.square_avg, "pluss::ai::nn::optim::RMSprop::restore_state[square_avg]");
    validate_state_vector_(state.momentum_buffer, "pluss::ai::nn::optim::RMSprop::restore_state[momentum_buffer]");
    validate_state_vector_(state.grad_avg, "pluss::ai::nn::optim::RMSprop::restore_state[grad_avg]");
    for (std::size_t i = 0; i < groups_.size(); ++i) {
      validate_options_(state.group_options[i]);
      groups_[i].options = state.group_options[i];
    }
    validate_group_options_();
    square_avg_ = state.square_avg;
    momentum_buffer_ = state.momentum_buffer;
    grad_avg_ = state.grad_avg;
    initialize_group_capture_state_();
  }

private:
  struct GroupState {
    std::vector<std::size_t> indices{};
    RMSpropOptions options{};
    Series one_scalar{};
    Series alpha_scalar{};
    Series one_minus_alpha_scalar{};
    Series epsilon_scalar{};
    Series momentum_scalar{};
    Series learning_rate_scalar{};
    Series weight_decay_scalar{};
  };

  struct GroupScalars {
    Series one{};
    Series alpha{};
    Series one_minus_alpha{};
    Series epsilon{};
    Series momentum{};
    Series learning_rate{};
    Series weight_decay{};
  };

  void apply_step_(pluss::msharp::RangeView::Series gradients) override {
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
          "pluss::ai::nn::optim::RMSprop",
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

  GroupScalars resolve_group_scalars_(const GroupState& group, const Series& like) const {
    const bool capturable = group.options.execution.capturable;
    GroupScalars out{};
    if (capturable && group.one_scalar.defined() && group.alpha_scalar.defined() &&
        group.one_minus_alpha_scalar.defined() && group.epsilon_scalar.defined() &&
        group.momentum_scalar.defined() && group.learning_rate_scalar.defined() &&
        group.weight_decay_scalar.defined()) {
      out.one = group.one_scalar;
      out.alpha = group.alpha_scalar;
      out.one_minus_alpha = group.one_minus_alpha_scalar;
      out.epsilon = group.epsilon_scalar;
      out.momentum = group.momentum_scalar;
      out.learning_rate = group.learning_rate_scalar;
      out.weight_decay = group.weight_decay_scalar;
      return out;
    }
    out.one = scalar_on_(1.0, like);
    out.alpha = scalar_on_(group.options.alpha, like);
    out.one_minus_alpha = out.one.sub(out.alpha);
    out.epsilon = scalar_on_(group.options.epsilon, like);
    out.momentum = scalar_on_(group.options.momentum, like);
    out.learning_rate = scalar_on_(group.options.learning_rate, like);
    out.weight_decay = scalar_on_(group.options.weight_decay, like);
    return out;
  }

  void apply_group_loop_(const GroupState& group,
                         pluss::msharp::RangeView::Of<Series*> params,
                         pluss::msharp::RangeView::Series gradients) {
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

      Series parameter_value = *param_ptr;
      {
        const GroupScalars wd_scalars = resolve_group_scalars_(group, parameter_value);
        if (options.weight_decay != 0.0) {
          sensitivity = sensitivity.add(parameter_value.mul(wd_scalars.weight_decay));
        }
      }

      // Complex parameters: view as real with a new trailing [.., 2] dim
      // (real(g)^2 and imag(g)^2 tracked independently, matching torch's
      // own `_view_as_real` in torch/optim/rmsprop.py -- capturable mode is
      // rejected for complex above). No-op for real parameters.
      const ScalarType original_dtype = parameter_value.scalar_type();
      sensitivity = view_as_real_(sensitivity);
      parameter_value = view_as_real_(parameter_value);
      Series square_avg_value = view_as_real_(square_avg_[i]);
      Series grad_avg_value = view_as_real_(grad_avg_[i]);
      Series momentum_value = view_as_real_(momentum_buffer_[i]);

      const GroupScalars scalars = resolve_group_scalars_(group, parameter_value);
      square_avg_value = square_avg_value.mul(scalars.alpha).add(sensitivity.abs2().mul(scalars.one_minus_alpha));

      Series denominator = square_avg_value.sqrt().add(scalars.epsilon);
      if (options.centered) {
        grad_avg_value = grad_avg_value.mul(scalars.alpha).add(sensitivity.mul(scalars.one_minus_alpha));
        denominator = square_avg_value.sub(grad_avg_value.abs2()).clamp_min(0.0).sqrt().add(scalars.epsilon);
      }

      Series update = sensitivity.div(denominator);
      if (options.momentum > 0.0) {
        momentum_value = momentum_value.mul(scalars.momentum).add(update);
        update = momentum_value;
      }

      const Series next_value = parameter_value.sub(update.mul(scalars.learning_rate));

      square_avg_[i] = view_as_complex_(square_avg_value, original_dtype);
      grad_avg_[i] = view_as_complex_(grad_avg_value, original_dtype);
      momentum_buffer_[i] = view_as_complex_(momentum_value, original_dtype);
      assign_parameter_(*param_ptr, view_as_complex_(next_value, original_dtype));
    }
  }

  void apply_group_batched_(const GroupState& group,
                            pluss::msharp::RangeView::Of<Series*> params,
                            pluss::msharp::RangeView::Series gradients) {
    const RMSpropOptions& options = group.options;

    std::vector<std::size_t> active{};
    std::vector<Series> p_parts{};
    std::vector<Series> g_parts{};
    std::vector<Series> square_parts{};
    std::vector<Series> momentum_parts{};
    std::vector<Series> grad_avg_parts{};

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
      square_parts.push_back(square_avg_[i].reshape(batched_shape_(square_avg_[i])));
      momentum_parts.push_back(momentum_buffer_[i].reshape(batched_shape_(momentum_buffer_[i])));
      grad_avg_parts.push_back(grad_avg_[i].reshape(batched_shape_(grad_avg_[i])));
    }
    if (active.empty()) return;

    Series p_batch = pack_batched_(p_parts, "pluss::ai::nn::optim::RMSprop::apply_group_batched_");
    Series g_batch = pack_batched_(g_parts, "pluss::ai::nn::optim::RMSprop::apply_group_batched_");
    Series square_batch = pack_batched_(square_parts, "pluss::ai::nn::optim::RMSprop::apply_group_batched_");
    Series momentum_batch = pack_batched_(momentum_parts, "pluss::ai::nn::optim::RMSprop::apply_group_batched_");
    Series grad_avg_batch = pack_batched_(grad_avg_parts, "pluss::ai::nn::optim::RMSprop::apply_group_batched_");
    const GroupScalars scalars = resolve_group_scalars_(group, p_batch);

    if (options.weight_decay != 0.0) {
      g_batch = g_batch.add(p_batch.mul(scalars.weight_decay));
    }

    square_batch = square_batch.mul(scalars.alpha).add(g_batch.abs2().mul(scalars.one_minus_alpha));
    Series denominator = square_batch.sqrt().add(scalars.epsilon);
    if (options.centered) {
      grad_avg_batch = grad_avg_batch.mul(scalars.alpha).add(g_batch.mul(scalars.one_minus_alpha));
      denominator = square_batch.sub(grad_avg_batch.abs2()).clamp_min(0.0).sqrt().add(scalars.epsilon);
    }

    Series update = g_batch.div(denominator);
    if (options.momentum > 0.0) {
      momentum_batch = momentum_batch.mul(scalars.momentum).add(update);
      update = momentum_batch;
    }

    const Series next_batch = p_batch.sub(update.mul(scalars.learning_rate));
    for (std::size_t j = 0; j < active.size(); ++j) {
      const std::size_t i = active[j];
      Series* param_ptr = params[i];
      Series next_value = next_batch.select(0, static_cast<std::int64_t>(j)).reshape(param_ptr->sizes());
      square_avg_[i] = square_batch.select(0, static_cast<std::int64_t>(j)).reshape(square_avg_[i].sizes());
      momentum_buffer_[i] =
          momentum_batch.select(0, static_cast<std::int64_t>(j)).reshape(momentum_buffer_[i].sizes());
      grad_avg_[i] = grad_avg_batch.select(0, static_cast<std::int64_t>(j)).reshape(grad_avg_[i].sizes());
      assign_parameter_(*param_ptr, next_value);
    }
  }

  void apply_group_fused_(const GroupState& group,
                          pluss::msharp::RangeView::Of<Series*> params,
                          pluss::msharp::RangeView::Series gradients) {
    const RMSpropOptions& options = group.options;

    std::vector<std::size_t> active{};
    std::vector<Series> p_parts{};
    std::vector<Series> g_parts{};
    std::vector<Series> square_parts{};
    std::vector<Series> momentum_parts{};
    std::vector<Series> grad_avg_parts{};

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
      square_parts.push_back(square_avg_[i].reshape(batched_shape_(square_avg_[i])));
      momentum_parts.push_back(momentum_buffer_[i].reshape(batched_shape_(momentum_buffer_[i])));
      grad_avg_parts.push_back(grad_avg_[i].reshape(batched_shape_(grad_avg_[i])));
    }
    if (active.empty()) return;

    Series p_batch = pack_batched_(p_parts, "pluss::ai::nn::optim::RMSprop::apply_group_fused_");
    Series g_batch = pack_batched_(g_parts, "pluss::ai::nn::optim::RMSprop::apply_group_fused_");
    Series square_batch = pack_batched_(square_parts, "pluss::ai::nn::optim::RMSprop::apply_group_fused_");
    Series momentum_batch = pack_batched_(momentum_parts, "pluss::ai::nn::optim::RMSprop::apply_group_fused_");
    Series grad_avg_batch = pack_batched_(grad_avg_parts, "pluss::ai::nn::optim::RMSprop::apply_group_fused_");
    const GroupScalars scalars = resolve_group_scalars_(group, p_batch);

    if (options.weight_decay != 0.0) {
      g_batch = g_batch.add(p_batch.mul(scalars.weight_decay));
    }

    square_batch = square_batch.mul(scalars.alpha);
    square_batch = square_batch.add(g_batch.abs2().mul(scalars.one_minus_alpha));
    Series denominator = square_batch.sqrt();
    denominator = denominator.add(scalars.epsilon);
    if (options.centered) {
      grad_avg_batch = grad_avg_batch.mul(scalars.alpha);
      grad_avg_batch = grad_avg_batch.add(g_batch.mul(scalars.one_minus_alpha));
      denominator = square_batch.sub(grad_avg_batch.abs2()).clamp_min(0.0).sqrt().add(scalars.epsilon);
    }

    g_batch = g_batch.div(denominator);
    if (options.momentum > 0.0) {
      momentum_batch = momentum_batch.mul(scalars.momentum);
      momentum_batch = momentum_batch.add(g_batch);
      g_batch = momentum_batch;
    }

    g_batch = g_batch.mul(scalars.learning_rate);
    p_batch = p_batch.sub(g_batch);
    for (std::size_t j = 0; j < active.size(); ++j) {
      const std::size_t i = active[j];
      Series* param_ptr = params[i];
      Series next_value = p_batch.select(0, static_cast<std::int64_t>(j)).reshape(param_ptr->sizes());
      square_avg_[i] = square_batch.select(0, static_cast<std::int64_t>(j)).reshape(square_avg_[i].sizes());
      momentum_buffer_[i] =
          momentum_batch.select(0, static_cast<std::int64_t>(j)).reshape(momentum_buffer_[i].sizes());
      grad_avg_[i] = grad_avg_batch.select(0, static_cast<std::int64_t>(j)).reshape(grad_avg_[i].sizes());
      assign_parameter_(*param_ptr, next_value);
    }
  }

  static void validate_options_(const RMSpropOptions& options) {
    if (!(options.learning_rate > 0.0) || !std::isfinite(options.learning_rate)) {
      ::pluss::throw_error("pluss::ai::nn::optim::RMSprop: learning_rate must be positive and finite");
    }
    if (!(options.alpha >= 0.0 && options.alpha < 1.0) || !std::isfinite(options.alpha)) {
      ::pluss::throw_error("pluss::ai::nn::optim::RMSprop: alpha must be in [0, 1)");
    }
    if (!(options.epsilon > 0.0) || !std::isfinite(options.epsilon)) {
      ::pluss::throw_error("pluss::ai::nn::optim::RMSprop: epsilon must be positive and finite");
    }
    if (options.momentum < 0.0 || !std::isfinite(options.momentum)) {
      ::pluss::throw_error("pluss::ai::nn::optim::RMSprop: momentum must be finite and non-negative");
    }
    if (options.weight_decay < 0.0 || !std::isfinite(options.weight_decay)) {
      ::pluss::throw_error("pluss::ai::nn::optim::RMSprop: weight_decay must be finite and non-negative");
    }
  }

  static std::vector<Series*> flatten_groups_(pluss::msharp::RangeView::Of<RMSpropGroup> groups) {
    if (groups.empty()) {
      ::pluss::throw_error("pluss::ai::nn::optim::RMSprop: expected non-empty parameter groups");
    }
    std::vector<Series*> flat{};
    std::unordered_set<Series*> seen{};
    for (const auto& group : groups) {
      if (group.parameters.empty()) {
        ::pluss::throw_error("pluss::ai::nn::optim::RMSprop: parameter group must not be empty");
      }
      for (Series* p : group.parameters) {
        if (p != nullptr && !seen.insert(p).second) {
          ::pluss::throw_error("pluss::ai::nn::optim::RMSprop: duplicate parameter across groups");
        }
        flat.push_back(p);
      }
    }
    return flat;
  }

  static std::vector<GroupState> build_group_states_(pluss::msharp::RangeView::Of<RMSpropGroup> groups) {
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
        ::pluss::throw_error("pluss::ai::nn::optim::RMSprop: capturable mode expects defined group parameters");
      }
      // Capturable mode is CUDA-graph-capture oriented (torch itself only
      // supports it on cuda/xpu/hpu); complex-parameter support in this pass
      // targets the ordinary Loop lane only (see view_as_real_/
      // view_as_complex_ in optim_base.h) and was never exercised against
      // capturable's own per-group bias-correction-power scalars, which are
      // typed to match the group's parameters. Reject rather than risk a
      // silently wrong interaction.
      if (pluss::codem::dtype_of(anchor->scalar_type()).is_complex()) {
        ::pluss::throw_error(
            "pluss::ai::nn::optim::RMSprop: capturable mode is not supported for complex parameters");
      }
      for (std::size_t i : group.indices) {
        const Series* p = params[i];
        if (p == nullptr || !p->defined()) continue;
        if (p->platform() != anchor->platform()) {
          ::pluss::throw_error(
              "pluss::ai::nn::optim::RMSprop: capturable mode requires same platform across each group");
        }
        if (p->scalar_type() != anchor->scalar_type()) {
          ::pluss::throw_error(
              "pluss::ai::nn::optim::RMSprop: capturable mode requires same dtype across each group");
        }
      }
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
      square_avg_.push_back(state_zeros_like_(*p));
      momentum_buffer_.push_back(state_zeros_like_(*p));
      grad_avg_.push_back(state_zeros_like_(*p));
    }
    initialize_group_capture_state_();
  }

  void initialize_group_capture_state_() {
    const auto& params = parameters();
    for (auto& group : groups_) {
      group.one_scalar = Series{};
      group.alpha_scalar = Series{};
      group.one_minus_alpha_scalar = Series{};
      group.epsilon_scalar = Series{};
      group.momentum_scalar = Series{};
      group.learning_rate_scalar = Series{};
      group.weight_decay_scalar = Series{};
      if (!group.options.execution.capturable) continue;
      if (group.indices.empty()) continue;
      const Series* anchor = params[group.indices.front()];
      group.one_scalar = Series::full({}, 1.0, anchor->scalar_type()).to(anchor->platform());
      group.alpha_scalar = Series::full({}, group.options.alpha, anchor->scalar_type()).to(anchor->platform());
      group.one_minus_alpha_scalar = Series::full({}, 1.0 - group.options.alpha, anchor->scalar_type())
                                        .to(anchor->platform());
      group.epsilon_scalar = Series::full({}, group.options.epsilon, anchor->scalar_type()).to(anchor->platform());
      group.momentum_scalar =
          Series::full({}, group.options.momentum, anchor->scalar_type()).to(anchor->platform());
      group.learning_rate_scalar =
          Series::full({}, group.options.learning_rate, anchor->scalar_type()).to(anchor->platform());
      group.weight_decay_scalar =
          Series::full({}, group.options.weight_decay, anchor->scalar_type()).to(anchor->platform());
    }
  }

  std::vector<GroupState> groups_{};
  RMSpropOptions default_options_{};
  std::vector<Series> square_avg_{};
  std::vector<Series> momentum_buffer_{};
  std::vector<Series> grad_avg_{};
};

} // namespace pluss::ai::nn::optim

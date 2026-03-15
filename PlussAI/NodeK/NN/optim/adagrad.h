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

struct AdagradOptions {
  double learning_rate{1e-2};
  double lr_decay{0.0};
  double weight_decay{0.0};
  double epsilon{1e-10};
  double initial_accumulator_value{0.0};
  bool maximize{false};
  GradientClipOptions clip{};
  ExecutionOptions execution{};
};

struct AdagradGroup {
  std::vector<Series*> parameters{};
  AdagradOptions options{};
};

class PLUSS_API Adagrad final : public Optimizer {
public:
  explicit Adagrad(std::vector<Series*> parameters, AdagradOptions options = {})
      : Optimizer(std::move(parameters)), default_options_(options) {
    validate_options_(default_options_);
    GroupState group{};
    group.options = default_options_;
    group.indices.resize(size());
    std::iota(group.indices.begin(), group.indices.end(), static_cast<std::size_t>(0));
    groups_.push_back(std::move(group));
    initialize_state_();
  }

  explicit Adagrad(std::vector<AdagradGroup> groups)
      : Optimizer(flatten_groups_(groups)),
        groups_(build_group_states_(groups)),
        default_options_(groups.front().options) {
    validate_group_options_();
    initialize_state_();
  }

  explicit Adagrad(Module& module, AdagradOptions options = {})
      : Adagrad(module.parameters(), options) {}

  const AdagradOptions& options() const noexcept { return default_options_; }

  void set_options(const AdagradOptions& options) {
    default_options_ = options;
    validate_options_(default_options_);
    for (auto& group : groups_) {
      group.options = default_options_;
    }
    initialize_state_();
  }

  void set_group_options(std::size_t group_index, const AdagradOptions& options) {
    if (group_index >= groups_.size()) {
      throw std::runtime_error("pluss::ai::nn::optim::Adagrad::set_group_options: group index out of range");
    }
    validate_options_(options);
    groups_[group_index].options = options;
    initialize_state_();
  }

  std::size_t group_count() const noexcept { return groups_.size(); }

  struct State {
    std::int64_t step{0};
    std::vector<AdagradOptions> group_options{};
    std::vector<Series> group_steps{};
    std::vector<Series> sum_square{};
  };

  State capture_state() const {
    State state{};
    state.step = step_;
    state.group_options.reserve(groups_.size());
    for (const auto& group : groups_) {
      state.group_options.push_back(group.options);
      state.group_steps.push_back(group.step_scalar);
    }
    state.sum_square = sum_square_;
    return state;
  }

  void restore_state(const State& state) {
    if (state.group_options.size() != groups_.size() || state.group_steps.size() != groups_.size()) {
      throw std::runtime_error("pluss::ai::nn::optim::Adagrad::restore_state: group count mismatch");
    }
    if (state.step < 0) {
      throw std::runtime_error("pluss::ai::nn::optim::Adagrad::restore_state: step must be non-negative");
    }
    validate_state_vector_(state.sum_square, "pluss::ai::nn::optim::Adagrad::restore_state[sum_square]");
    for (std::size_t i = 0; i < groups_.size(); ++i) {
      validate_options_(state.group_options[i]);
      groups_[i].options = state.group_options[i];
      groups_[i].step_scalar = state.group_steps[i];
    }
    validate_capturable_groups_();
    step_ = state.step;
    sum_square_ = state.sum_square;
  }

private:
  struct GroupState {
    std::vector<std::size_t> indices{};
    AdagradOptions options{};
    Series step_scalar{};
  };

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

  double current_learning_rate_(const AdagradOptions& options) const {
    if (options.lr_decay == 0.0) return options.learning_rate;
    const double t = static_cast<double>(std::max<std::int64_t>(0, step_ - 1));
    return options.learning_rate / (1.0 + t * options.lr_decay);
  }

  void advance_capturable_step_(GroupState& group) {
    if (!group.options.execution.capturable) return;
    group.step_scalar = group.step_scalar.add(scalar_on_(1.0, group.step_scalar));
  }

  Series capturable_learning_rate_(const GroupState& group, const Series& like) const {
    const Series one = scalar_on_(1.0, group.step_scalar);
    const Series t_prev = group.step_scalar.sub(one);
    const Series denom = one.add(t_prev.mul(scalar_on_(group.options.lr_decay, group.step_scalar)));
    const Series lr = scalar_on_(group.options.learning_rate, group.step_scalar).div(denom);
    return lr.to(like.platform(), like.scalar_type());
  }

  void apply_group_loop_(GroupState& group,
                         const std::vector<Series*>& params,
                         const std::vector<Series>& gradients) {
    const AdagradOptions& options = group.options;
    const double lr_value = current_learning_rate_(options);
    advance_capturable_step_(group);

    for (std::size_t i : group.indices) {
      Series* param_ptr = params[i];
      if (param_ptr == nullptr) continue;
      const Series& input_sensitivity = gradients[i];
      if (!input_sensitivity.defined()) continue;

      Series sensitivity = clip_gradient_(input_sensitivity, options.clip);
      if (options.maximize) sensitivity = sensitivity.neg();

      Series parameter_value = *param_ptr;
      const Series lr =
          options.execution.capturable ? capturable_learning_rate_(group, parameter_value)
                                       : scalar_on_(lr_value, parameter_value);
      const Series eps = scalar_on_(options.epsilon, parameter_value);
      if (options.weight_decay != 0.0) {
        sensitivity = sensitivity.add(parameter_value.mul(scalar_on_(options.weight_decay, parameter_value)));
      }

      sum_square_[i] = sum_square_[i].add(sensitivity.square());
      const Series update = sensitivity.div(sum_square_[i].sqrt().add(eps));
      assign_parameter_(*param_ptr, parameter_value.sub(update.mul(lr)));
    }
  }

  void apply_group_batched_(GroupState& group,
                            const std::vector<Series*>& params,
                            const std::vector<Series>& gradients) {
    const AdagradOptions& options = group.options;
    const double lr_value = current_learning_rate_(options);
    advance_capturable_step_(group);

    std::vector<std::size_t> active{};
    std::vector<Series> p_parts{};
    std::vector<Series> g_parts{};
    std::vector<Series> s_parts{};
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
      s_parts.push_back(sum_square_[i].reshape(batched_shape_(sum_square_[i])));
    }
    if (active.empty()) return;

    Series p_batch = pack_batched_(p_parts, "pluss::ai::nn::optim::Adagrad::apply_group_batched_");
    Series g_batch = pack_batched_(g_parts, "pluss::ai::nn::optim::Adagrad::apply_group_batched_");
    Series s_batch = pack_batched_(s_parts, "pluss::ai::nn::optim::Adagrad::apply_group_batched_");

    const Series lr =
        options.execution.capturable ? capturable_learning_rate_(group, p_batch) : scalar_on_(lr_value, p_batch);
    const Series eps = scalar_on_(options.epsilon, p_batch);
    if (options.weight_decay != 0.0) {
      g_batch = g_batch.add(p_batch.mul(scalar_on_(options.weight_decay, p_batch)));
    }

    s_batch = s_batch.add(g_batch.square());
    const Series update = g_batch.div(s_batch.sqrt().add(eps));
    const Series next_batch = p_batch.sub(update.mul(lr));

    for (std::size_t j = 0; j < active.size(); ++j) {
      const std::size_t i = active[j];
      Series* param_ptr = params[i];
      Series next_value = next_batch.select(0, static_cast<std::int64_t>(j)).reshape(param_ptr->sizes());
      sum_square_[i] = s_batch.select(0, static_cast<std::int64_t>(j)).reshape(sum_square_[i].sizes());
      assign_parameter_(*param_ptr, next_value);
    }
  }

  void apply_group_fused_(GroupState& group,
                          const std::vector<Series*>& params,
                          const std::vector<Series>& gradients) {
    const AdagradOptions& options = group.options;
    const double lr_value = current_learning_rate_(options);
    advance_capturable_step_(group);

    std::vector<std::size_t> active{};
    std::vector<Series> p_parts{};
    std::vector<Series> g_parts{};
    std::vector<Series> s_parts{};
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
      s_parts.push_back(sum_square_[i].reshape(batched_shape_(sum_square_[i])));
    }
    if (active.empty()) return;

    Series p_batch = pack_batched_(p_parts, "pluss::ai::nn::optim::Adagrad::apply_group_fused_");
    Series g_batch = pack_batched_(g_parts, "pluss::ai::nn::optim::Adagrad::apply_group_fused_");
    Series s_batch = pack_batched_(s_parts, "pluss::ai::nn::optim::Adagrad::apply_group_fused_");

    const Series lr =
        options.execution.capturable ? capturable_learning_rate_(group, p_batch) : scalar_on_(lr_value, p_batch);
    const Series eps = scalar_on_(options.epsilon, p_batch);
    if (options.weight_decay != 0.0) {
      g_batch = g_batch.add(p_batch.mul(scalar_on_(options.weight_decay, p_batch)));
    }

    s_batch = s_batch.add(g_batch.square());
    Series denom = s_batch.sqrt();
    denom = denom.add(eps);
    g_batch = g_batch.div(denom);
    g_batch = g_batch.mul(lr);
    p_batch = p_batch.sub(g_batch);

    for (std::size_t j = 0; j < active.size(); ++j) {
      const std::size_t i = active[j];
      Series* param_ptr = params[i];
      Series next_value = p_batch.select(0, static_cast<std::int64_t>(j)).reshape(param_ptr->sizes());
      sum_square_[i] = s_batch.select(0, static_cast<std::int64_t>(j)).reshape(sum_square_[i].sizes());
      assign_parameter_(*param_ptr, next_value);
    }
  }

  void apply_step_(const std::vector<Series>& gradients) override {
    ++step_;
    auto& params = mutable_parameters_();
    reset_step_lane_telemetry_();
    for (std::size_t group_index = 0; group_index < groups_.size(); ++group_index) {
      auto& group = groups_[group_index];
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
          "pluss::ai::nn::optim::Adagrad",
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
      } else if (lane == UpdateLane::Batched) {
        apply_group_batched_(group, params, gradients);
      } else {
        apply_group_loop_(group, params, gradients);
      }
    }
  }

  static void validate_options_(const AdagradOptions& options) {
    if (!(options.learning_rate > 0.0) || !std::isfinite(options.learning_rate)) {
      throw std::runtime_error("pluss::ai::nn::optim::Adagrad: learning_rate must be positive and finite");
    }
    if (options.lr_decay < 0.0 || !std::isfinite(options.lr_decay)) {
      throw std::runtime_error("pluss::ai::nn::optim::Adagrad: lr_decay must be finite and non-negative");
    }
    if (options.weight_decay < 0.0 || !std::isfinite(options.weight_decay)) {
      throw std::runtime_error("pluss::ai::nn::optim::Adagrad: weight_decay must be finite and non-negative");
    }
    if (!(options.epsilon > 0.0) || !std::isfinite(options.epsilon)) {
      throw std::runtime_error("pluss::ai::nn::optim::Adagrad: epsilon must be positive and finite");
    }
    if (options.initial_accumulator_value < 0.0 || !std::isfinite(options.initial_accumulator_value)) {
      throw std::runtime_error(
          "pluss::ai::nn::optim::Adagrad: initial_accumulator_value must be finite and non-negative");
    }
  }

  static std::vector<Series*> flatten_groups_(const std::vector<AdagradGroup>& groups) {
    if (groups.empty()) {
      throw std::runtime_error("pluss::ai::nn::optim::Adagrad: expected non-empty parameter groups");
    }
    std::vector<Series*> flat{};
    std::unordered_set<Series*> seen{};
    for (const auto& group : groups) {
      if (group.parameters.empty()) {
        throw std::runtime_error("pluss::ai::nn::optim::Adagrad: parameter group must not be empty");
      }
      for (Series* p : group.parameters) {
        if (p != nullptr && !seen.insert(p).second) {
          throw std::runtime_error("pluss::ai::nn::optim::Adagrad: duplicate parameter across groups");
        }
        flat.push_back(p);
      }
    }
    return flat;
  }

  static std::vector<GroupState> build_group_states_(const std::vector<AdagradGroup>& groups) {
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
        throw std::runtime_error("pluss::ai::nn::optim::Adagrad: capturable mode expects defined group parameters");
      }
      for (std::size_t i : group.indices) {
        const Series* p = params[i];
        if (p == nullptr || !p->defined()) continue;
        if (p->platform() != anchor->platform()) {
          throw std::runtime_error(
              "pluss::ai::nn::optim::Adagrad: capturable mode requires same platform across each group");
        }
        if (p->scalar_type() != anchor->scalar_type()) {
          throw std::runtime_error(
              "pluss::ai::nn::optim::Adagrad: capturable mode requires same dtype across each group");
        }
      }
    }
  }

  void initialize_state_() {
    const auto& params = parameters();
    sum_square_.clear();
    sum_square_.reserve(params.size());

    for (const Series* p : params) {
      sum_square_.push_back(pluss::zeros_like(*p, p->scalar_type()));
    }

    for (const auto& group : groups_) {
      if (group.options.initial_accumulator_value == 0.0) continue;
      for (std::size_t i : group.indices) {
        const Series* p = params[i];
        sum_square_[i] = pluss::full_like(*p, group.options.initial_accumulator_value, p->scalar_type());
      }
    }
    initialize_group_capture_state_();
  }

  void initialize_group_capture_state_() {
    const auto& params = parameters();
    for (auto& group : groups_) {
      group.step_scalar = Series{};
      if (!group.options.execution.capturable) continue;
      if (group.indices.empty()) continue;
      const Series* anchor = params[group.indices.front()];
      group.step_scalar = Series::full({}, 0.0, anchor->scalar_type()).to(anchor->platform());
    }
  }

  std::vector<GroupState> groups_{};
  AdagradOptions default_options_{};
  std::vector<Series> sum_square_{};
  std::int64_t step_{0};
};

} // namespace pluss::ai::nn::optim

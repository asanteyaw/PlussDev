#pragma once

#include "optim_base.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <iostream>
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
  ExecutionOptions execution{};
};

struct AdamGroup {
  std::vector<Series*> parameters{};
  AdamOptions options{};
};

class PLUSS_API Adam final : public Optimizer {
public:
  explicit Adam(pluss::msharp::RangeView::Of<Series*> parameters, AdamOptions options = {})
      : Optimizer(parameters), default_options_(options) {
    validate_options_(default_options_);
    GroupState group{};
    group.options = default_options_;
    group.indices.resize(size());
    std::iota(group.indices.begin(), group.indices.end(), static_cast<std::size_t>(0));
    groups_.push_back(std::move(group));
    initialize_state_();
  }

  explicit Adam(pluss::msharp::RangeView::Of<AdamGroup> groups)
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
      ::pluss::throw_error("pluss::ai::nn::optim::Adam::set_group_options: group index out of range");
    }
    validate_options_(options);
    groups_[group_index].options = options;
  }

  std::size_t group_count() const noexcept { return groups_.size(); }

  struct State {
    std::int64_t step{0};
    std::vector<AdamOptions> group_options{};
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
      ::pluss::throw_error("pluss::ai::nn::optim::Adam::restore_state: group count mismatch");
    }
    if (state.beta1_power.size() != groups_.size() || state.beta2_power.size() != groups_.size()) {
      ::pluss::throw_error("pluss::ai::nn::optim::Adam::restore_state: capture scalar state mismatch");
    }
    validate_state_vector_(state.first_moment, "pluss::ai::nn::optim::Adam::restore_state[first_moment]");
    validate_state_vector_(state.second_moment, "pluss::ai::nn::optim::Adam::restore_state[second_moment]");
    validate_state_vector_(state.max_second_hat, "pluss::ai::nn::optim::Adam::restore_state[max_second_hat]");
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
    AdamOptions options{};
    Series beta1_power{};
    Series beta2_power{};
  };

  void apply_step_(pluss::msharp::RangeView::Series gradients) override {
    ++step_;

    auto& params = mutable_parameters_();
    reset_step_lane_telemetry_();
    for (std::size_t group_index = 0; group_index < groups_.size(); ++group_index) {
      auto& group = groups_[group_index];
      const AdamOptions& options = group.options;
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
          "pluss::ai::nn::optim::Adam",
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

  static void validate_options_(const AdamOptions& options) {
    if (!(options.learning_rate > 0.0) || !std::isfinite(options.learning_rate)) {
      ::pluss::throw_error("pluss::ai::nn::optim::Adam: learning_rate must be positive and finite");
    }
    if (!(options.beta1 >= 0.0 && options.beta1 < 1.0) || !std::isfinite(options.beta1)) {
      ::pluss::throw_error("pluss::ai::nn::optim::Adam: beta1 must be in [0, 1)");
    }
    if (!(options.beta2 >= 0.0 && options.beta2 < 1.0) || !std::isfinite(options.beta2)) {
      ::pluss::throw_error("pluss::ai::nn::optim::Adam: beta2 must be in [0, 1)");
    }
    if (!(options.epsilon > 0.0) || !std::isfinite(options.epsilon)) {
      ::pluss::throw_error("pluss::ai::nn::optim::Adam: epsilon must be positive and finite");
    }
    if (options.weight_decay < 0.0 || !std::isfinite(options.weight_decay)) {
      ::pluss::throw_error("pluss::ai::nn::optim::Adam: weight_decay must be finite and non-negative");
    }
  }

  static std::vector<Series*> flatten_groups_(pluss::msharp::RangeView::Of<AdamGroup> groups) {
    if (groups.empty()) {
      ::pluss::throw_error("pluss::ai::nn::optim::Adam: expected non-empty parameter groups");
    }
    std::vector<Series*> flat{};
    std::unordered_set<Series*> seen{};
    for (const auto& group : groups) {
      if (group.parameters.empty()) {
        ::pluss::throw_error("pluss::ai::nn::optim::Adam: parameter group must not be empty");
      }
      for (Series* p : group.parameters) {
        if (p != nullptr && !seen.insert(p).second) {
          ::pluss::throw_error("pluss::ai::nn::optim::Adam: duplicate parameter across groups");
        }
        flat.push_back(p);
      }
    }
    return flat;
  }

  static std::vector<GroupState> build_group_states_(pluss::msharp::RangeView::Of<AdamGroup> groups) {
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
        ::pluss::throw_error("pluss::ai::nn::optim::Adam: capturable mode expects defined group parameters");
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
            "pluss::ai::nn::optim::Adam: capturable mode is not supported for complex parameters");
      }
      for (std::size_t i : group.indices) {
        const Series* p = params[i];
        if (p == nullptr || !p->defined()) continue;
        if (p->platform() != anchor->platform()) {
          ::pluss::throw_error(
              "pluss::ai::nn::optim::Adam: capturable mode requires same platform across each group");
        }
        if (p->scalar_type() != anchor->scalar_type()) {
          ::pluss::throw_error(
              "pluss::ai::nn::optim::Adam: capturable mode requires same dtype across each group");
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
    const AdamOptions& options = group.options;
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

      const bool use_capturable = options.execution.capturable;

      Series parameter_value = *param_ptr;
      Series m1_value = first_moment_[i];
      Series m2_value = second_moment_[i];
      Series mmax_value = max_second_hat_[i];
      const Series m1_before = m1_value;
      const Series m2_before = m2_value;

      if (options.weight_decay != 0.0) {
        sensitivity = sensitivity.add(parameter_value.mul(options.weight_decay));
      }

      // Complex parameters: view as real with a new trailing [.., 2] dim
      // (real(g)^2 and imag(g)^2 tracked independently, matching torch's
      // own `_view_as_real` in torch/optim/adam.py -- capturable mode is
      // rejected for complex above, so use_capturable is guaranteed false
      // whenever original_dtype is complex). No-op for real parameters.
      const ScalarType original_dtype = parameter_value.scalar_type();
      sensitivity = view_as_real_(sensitivity);
      parameter_value = view_as_real_(parameter_value);
      m1_value = view_as_real_(m1_value);
      m2_value = view_as_real_(m2_value);
      mmax_value = view_as_real_(mmax_value);

      if (use_capturable) {
        const Series one = scalar_on_(1.0, parameter_value);
        const Series beta1 = scalar_on_(options.beta1, parameter_value);
        const Series beta2 = scalar_on_(options.beta2, parameter_value);
        const Series one_minus_beta1 = one.sub(beta1);
        const Series one_minus_beta2 = one.sub(beta2);
        m1_value = m1_value.mul(beta1).add(sensitivity.mul(one_minus_beta1));
        m2_value = m2_value.mul(beta2).add(sensitivity.abs2().mul(one_minus_beta2));
      } else {
        m1_value = m1_value.mul(options.beta1).add(sensitivity.mul(1.0 - options.beta1));
        m2_value = m2_value.mul(options.beta2).add(sensitivity.abs2().mul(1.0 - options.beta2));
      }

      Series second_hat = use_capturable
                              ? m2_value.div(b2_corr_series)
                              : m2_value.div(b2_correction);
      if (options.amsgrad) {
        mmax_value = mmax_value.maximum(second_hat);
        second_hat = mmax_value;
      }

      const Series first_hat = use_capturable
                                   ? m1_value.div(b1_corr_series)
                                   : m1_value.div(b1_correction);
      const Series denominator = second_hat.sqrt().add(options.epsilon);
      const Series delta = first_hat.div(denominator).mul(options.learning_rate);
      Series next_value = parameter_value.sub(delta);

      const bool debug_adam_update = []() {
        const char* raw = std::getenv("PLUSS_DEBUG_ADAM_UPDATE");
        if (!raw) return false;
        return std::strcmp(raw, "1") == 0 || std::strcmp(raw, "true") == 0 || std::strcmp(raw, "TRUE") == 0;
      }();
      if (debug_adam_update && parameter_value.platform() == pluss::pMPS && step_ <= 3) {
        auto print_vec = [&](const char* label, const Series& s) {
          const auto v = s.reshape({s.numel()}).to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
          std::cerr << " " << label << "=";
          for (std::size_t k = 0; k < v.size(); ++k) {
            if (k) std::cerr << ",";
            std::cerr << v[k];
          }
        };
        std::cerr << "[adam-loop-debug] step=" << step_ << " param_index=" << i;
        std::cerr << " p_contig=" << (parameter_value.is_contiguous() ? 1 : 0)
                  << " g_contig=" << (sensitivity.is_contiguous() ? 1 : 0)
                  << " m1_contig=" << (m1_before.is_contiguous() ? 1 : 0)
                  << " m2_contig=" << (m2_before.is_contiguous() ? 1 : 0);
        print_vec("p", parameter_value);
        print_vec("g", sensitivity);
        print_vec("m1_before", m1_before);
        print_vec("m2_before", m2_before);
        print_vec("m1", m1_value);
        print_vec("m2", m2_value);
        print_vec("fh", first_hat);
        print_vec("den", denominator);
        print_vec("delta", delta);
        print_vec("next", next_value);
        std::cerr << "\n";
      }

      first_moment_[i] = view_as_complex_(m1_value, original_dtype);
      second_moment_[i] = view_as_complex_(m2_value, original_dtype);
      max_second_hat_[i] = view_as_complex_(mmax_value, original_dtype);
      assign_parameter_(*param_ptr, view_as_complex_(next_value, original_dtype));
    }
  }

  void apply_group_batched_(GroupState& group,
                            pluss::msharp::RangeView::Of<Series*> params,
                            pluss::msharp::RangeView::Series gradients) {
    const AdamOptions& options = group.options;
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

    Series p_batch = pack_batched_(p_parts, "pluss::ai::nn::optim::Adam::apply_group_batched_");
    Series g_batch = pack_batched_(g_parts, "pluss::ai::nn::optim::Adam::apply_group_batched_");
    Series m1_batch = pack_batched_(m1_parts, "pluss::ai::nn::optim::Adam::apply_group_batched_");
    Series m2_batch = pack_batched_(m2_parts, "pluss::ai::nn::optim::Adam::apply_group_batched_");
    Series mmax_batch = pack_batched_(mmax_parts, "pluss::ai::nn::optim::Adam::apply_group_batched_");
    const Series one = scalar_on_(1.0, p_batch);
    const Series beta1 = scalar_on_(options.beta1, p_batch);
    const Series beta2 = scalar_on_(options.beta2, p_batch);
    const Series one_minus_beta1 = one.sub(beta1);
    const Series one_minus_beta2 = one.sub(beta2);

    if (options.weight_decay != 0.0) {
      g_batch = g_batch.add(p_batch.mul(scalar_on_(options.weight_decay, p_batch)));
    }
    m1_batch = m1_batch.mul(beta1).add(g_batch.mul(one_minus_beta1));
    m2_batch = m2_batch.mul(beta2).add(g_batch.abs2().mul(one_minus_beta2));

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
    const AdamOptions& options = group.options;
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

    Series p_batch = pack_batched_(p_parts, "pluss::ai::nn::optim::Adam::apply_group_fused_");
    Series g_batch = pack_batched_(g_parts, "pluss::ai::nn::optim::Adam::apply_group_fused_");
    Series m1_batch = pack_batched_(m1_parts, "pluss::ai::nn::optim::Adam::apply_group_fused_");
    Series m2_batch = pack_batched_(m2_parts, "pluss::ai::nn::optim::Adam::apply_group_fused_");
    Series mmax_batch = pack_batched_(mmax_parts, "pluss::ai::nn::optim::Adam::apply_group_fused_");
    const Series one = scalar_on_(1.0, p_batch);
    const Series beta1 = scalar_on_(options.beta1, p_batch);
    const Series beta2 = scalar_on_(options.beta2, p_batch);
    const Series one_minus_beta1 = one.sub(beta1);
    const Series one_minus_beta2 = one.sub(beta2);

    if (options.weight_decay != 0.0) {
      g_batch = g_batch.add(p_batch.mul(scalar_on_(options.weight_decay, p_batch)));
    }
    m1_batch = m1_batch.mul(beta1);
    m1_batch = m1_batch.add(g_batch.mul(one_minus_beta1));
    m2_batch = m2_batch.mul(beta2);
    m2_batch = m2_batch.add(g_batch.abs2().mul(one_minus_beta2));

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
  AdamOptions default_options_{};
  std::vector<Series> first_moment_{};
  std::vector<Series> second_moment_{};
  std::vector<Series> max_second_hat_{};
  std::int64_t step_{0};
};

} // namespace pluss::ai::nn::optim

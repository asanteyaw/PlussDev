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
  ExecutionOptions execution{};
};

struct NAdamGroup {
  std::vector<Series*> parameters{};
  NAdamOptions options{};
};

class PLUSS_API NAdam final : public Optimizer {
public:
  explicit NAdam(pluss::msharp::RangeView::Of<Series*> parameters, NAdamOptions options = {})
      : Optimizer(parameters), default_options_(options) {
    validate_options_(default_options_);
    GroupState group{};
    group.options = default_options_;
    group.indices.resize(size());
    std::iota(group.indices.begin(), group.indices.end(), static_cast<std::size_t>(0));
    groups_.push_back(std::move(group));
    initialize_state_();
  }

  explicit NAdam(pluss::msharp::RangeView::Of<NAdamGroup> groups)
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
    validate_capturable_groups_();
    initialize_group_capture_state_();
  }

  void set_group_options(std::size_t group_index, const NAdamOptions& options) {
    if (group_index >= groups_.size()) {
      ::pluss::throw_error("pluss::ai::nn::optim::NAdam::set_group_options: group index out of range");
    }
    validate_options_(options);
    groups_[group_index].options = options;
    validate_capturable_groups_();
    initialize_group_capture_state_();
  }

  std::size_t group_count() const noexcept { return groups_.size(); }

  struct State {
    std::int64_t step{0};
    std::vector<NAdamOptions> group_options{};
    std::vector<double> mu_products{};
    std::vector<Series> mu_product_scalar{};
    std::vector<Series> beta2_power{};
    std::vector<Series> decay_power{};
    std::vector<Series> exp_avg{};
    std::vector<Series> exp_avg_sq{};
  };

  State capture_state() const {
    State state{};
    state.step = step_;
    state.group_options.reserve(groups_.size());
    state.mu_products.reserve(groups_.size());
    for (const auto& group : groups_) {
      state.group_options.push_back(group.options);
      state.mu_products.push_back(group.mu_product);
      state.mu_product_scalar.push_back(group.mu_product_scalar);
      state.beta2_power.push_back(group.beta2_power);
      state.decay_power.push_back(group.decay_power);
    }
    state.exp_avg = exp_avg_;
    state.exp_avg_sq = exp_avg_sq_;
    return state;
  }

  void restore_state(const State& state) {
    if (state.group_options.size() != groups_.size() ||
        state.mu_products.size() != groups_.size() ||
        state.mu_product_scalar.size() != groups_.size() ||
        state.beta2_power.size() != groups_.size() ||
        state.decay_power.size() != groups_.size()) {
      ::pluss::throw_error("pluss::ai::nn::optim::NAdam::restore_state: group count mismatch");
    }
    validate_state_vector_(state.exp_avg, "pluss::ai::nn::optim::NAdam::restore_state[exp_avg]");
    validate_state_vector_(state.exp_avg_sq, "pluss::ai::nn::optim::NAdam::restore_state[exp_avg_sq]");
    for (std::size_t i = 0; i < groups_.size(); ++i) {
      validate_options_(state.group_options[i]);
      groups_[i].options = state.group_options[i];
      groups_[i].mu_product = state.mu_products[i];
      groups_[i].mu_product_scalar = state.mu_product_scalar[i];
      groups_[i].beta2_power = state.beta2_power[i];
      groups_[i].decay_power = state.decay_power[i];
    }
    validate_capturable_groups_();
    step_ = state.step;
    exp_avg_ = state.exp_avg;
    exp_avg_sq_ = state.exp_avg_sq;
  }

private:
  struct GroupState {
    std::vector<std::size_t> indices{};
    NAdamOptions options{};
    double mu_product{1.0};
    Series mu_product_scalar{};
    Series beta2_power{};
    Series decay_power{};
  };

  void apply_step_(pluss::msharp::RangeView::Series gradients) override {
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
          "pluss::ai::nn::optim::NAdam",
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

  struct CapturableTerms {
    Series mu_t{};
    Series mu_t_next{};
    Series one_minus_mu_prod{};
    Series one_minus_mu_prod_next{};
    Series one_minus_beta2_pow{};
  };

  CapturableTerms advance_capturable_terms_(GroupState& group) {
    CapturableTerms terms{};
    const Series one = scalar_on_(1.0, group.mu_product_scalar);
    const Series half = scalar_on_(0.5, group.mu_product_scalar);
    const Series beta1 = scalar_on_(group.options.beta1, group.mu_product_scalar);
    const Series beta2 = scalar_on_(group.options.beta2, group.beta2_power);
    const Series decay_factor = scalar_on_(std::pow(0.96, group.options.momentum_decay), group.decay_power);

    group.decay_power = group.decay_power.mul(decay_factor);
    terms.mu_t = beta1.mul(one.sub(half.mul(group.decay_power)));
    const Series decay_next = group.decay_power.mul(decay_factor);
    terms.mu_t_next = beta1.mul(one.sub(half.mul(decay_next)));

    group.mu_product_scalar = group.mu_product_scalar.mul(terms.mu_t);
    const Series mu_product_next = group.mu_product_scalar.mul(terms.mu_t_next);
    terms.one_minus_mu_prod = one.sub(group.mu_product_scalar);
    terms.one_minus_mu_prod_next = one.sub(mu_product_next);

    group.beta2_power = group.beta2_power.mul(beta2);
    terms.one_minus_beta2_pow = one.sub(group.beta2_power);
    return terms;
  }

  void apply_group_loop_(GroupState& group,
                         pluss::msharp::RangeView::Of<Series*> params,
                         pluss::msharp::RangeView::Series gradients) {
    const NAdamOptions& options = group.options;
    const bool capturable = options.execution.capturable;
    CapturableTerms cap_terms{};
    double mu_t = 0.0;
    double mu_t_next = 0.0;
    double one_minus_mu_prod = 0.0;
    double one_minus_mu_prod_next = 0.0;
    double one_minus_beta2_pow = 0.0;
    if (capturable) {
      cap_terms = advance_capturable_terms_(group);
    } else {
      mu_t = options.beta1 * (1.0 - 0.5 * std::pow(0.96, static_cast<double>(step_) * options.momentum_decay));
      mu_t_next =
          options.beta1 * (1.0 - 0.5 * std::pow(0.96, static_cast<double>(step_ + 1) * options.momentum_decay));
      group.mu_product *= mu_t;
      const double mu_product_next = group.mu_product * mu_t_next;
      one_minus_mu_prod = 1.0 - group.mu_product;
      one_minus_mu_prod_next = 1.0 - mu_product_next;
      one_minus_beta2_pow = 1.0 - std::pow(options.beta2, static_cast<double>(step_));
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
      const Series lr = scalar_on_(options.learning_rate, parameter_value);
      const Series eps = scalar_on_(options.epsilon, parameter_value);
      if (options.weight_decay != 0.0) {
        if (options.decoupled_weight_decay) {
          const Series decay = scalar_on_(options.learning_rate * options.weight_decay, parameter_value);
          parameter_value = parameter_value.mul(one.sub(decay));
        } else {
          sensitivity = sensitivity.add(parameter_value.mul(scalar_on_(options.weight_decay, parameter_value)));
        }
      }

      exp_avg_[i] = exp_avg_[i].mul(beta1).add(sensitivity.mul(one_minus_beta1));
      exp_avg_sq_[i] = exp_avg_sq_[i].mul(beta2).add(sensitivity.square().mul(one_minus_beta2));

      const Series denom = capturable
                               ? exp_avg_sq_[i].div(cap_terms.one_minus_beta2_pow).sqrt().add(eps)
                               : exp_avg_sq_[i].div(scalar_on_(one_minus_beta2_pow, parameter_value)).sqrt().add(eps);
      const Series g_hat = capturable
                               ? sensitivity.div(cap_terms.one_minus_mu_prod)
                               : sensitivity.div(scalar_on_(one_minus_mu_prod, parameter_value));
      const Series m_hat = capturable
                               ? exp_avg_[i].div(cap_terms.one_minus_mu_prod_next)
                               : exp_avg_[i].div(scalar_on_(one_minus_mu_prod_next, parameter_value));
      const Series nesterov_m = capturable
                                    ? g_hat.mul(scalar_on_(1.0, parameter_value).sub(cap_terms.mu_t))
                                          .add(m_hat.mul(cap_terms.mu_t_next))
                                    : g_hat.mul(scalar_on_(1.0 - mu_t, parameter_value))
                                          .add(m_hat.mul(scalar_on_(mu_t_next, parameter_value)));
      const Series delta = nesterov_m.div(denom).mul(lr);

      assign_parameter_(*param_ptr, parameter_value.sub(delta));
    }
  }

  void apply_group_batched_(GroupState& group,
                            pluss::msharp::RangeView::Of<Series*> params,
                            pluss::msharp::RangeView::Series gradients) {
    const NAdamOptions& options = group.options;
    const bool capturable = options.execution.capturable;
    CapturableTerms cap_terms{};
    double mu_t = 0.0;
    double mu_t_next = 0.0;
    double one_minus_mu_prod = 0.0;
    double one_minus_mu_prod_next = 0.0;
    double one_minus_beta2_pow = 0.0;
    if (capturable) {
      cap_terms = advance_capturable_terms_(group);
    } else {
      mu_t = options.beta1 * (1.0 - 0.5 * std::pow(0.96, static_cast<double>(step_) * options.momentum_decay));
      mu_t_next =
          options.beta1 * (1.0 - 0.5 * std::pow(0.96, static_cast<double>(step_ + 1) * options.momentum_decay));
      group.mu_product *= mu_t;
      const double mu_product_next = group.mu_product * mu_t_next;
      one_minus_mu_prod = 1.0 - group.mu_product;
      one_minus_mu_prod_next = 1.0 - mu_product_next;
      one_minus_beta2_pow = 1.0 - std::pow(options.beta2, static_cast<double>(step_));
    }

    std::vector<std::size_t> active{};
    std::vector<Series> p_parts{};
    std::vector<Series> g_parts{};
    std::vector<Series> m_parts{};
    std::vector<Series> v_parts{};

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
      m_parts.push_back(exp_avg_[i].reshape(batched_shape_(exp_avg_[i])));
      v_parts.push_back(exp_avg_sq_[i].reshape(batched_shape_(exp_avg_sq_[i])));
    }
    if (active.empty()) return;

    Series p_batch = pack_batched_(p_parts, "pluss::ai::nn::optim::NAdam::apply_group_batched_");
    Series g_batch = pack_batched_(g_parts, "pluss::ai::nn::optim::NAdam::apply_group_batched_");
    Series m_batch = pack_batched_(m_parts, "pluss::ai::nn::optim::NAdam::apply_group_batched_");
    Series v_batch = pack_batched_(v_parts, "pluss::ai::nn::optim::NAdam::apply_group_batched_");
    const Series one = scalar_on_(1.0, p_batch);
    const Series beta1 = scalar_on_(options.beta1, p_batch);
    const Series beta2 = scalar_on_(options.beta2, p_batch);
    const Series one_minus_beta1 = one.sub(beta1);
    const Series one_minus_beta2 = one.sub(beta2);
    const Series lr = scalar_on_(options.learning_rate, p_batch);
    const Series eps = scalar_on_(options.epsilon, p_batch);

    if (options.weight_decay != 0.0) {
      if (options.decoupled_weight_decay) {
        const Series decay = scalar_on_(options.learning_rate * options.weight_decay, p_batch);
        p_batch = p_batch.mul(one.sub(decay));
      } else {
        g_batch = g_batch.add(p_batch.mul(scalar_on_(options.weight_decay, p_batch)));
      }
    }

    m_batch = m_batch.mul(beta1).add(g_batch.mul(one_minus_beta1));
    v_batch = v_batch.mul(beta2).add(g_batch.square().mul(one_minus_beta2));

    const Series denom = capturable
                             ? v_batch.div(cap_terms.one_minus_beta2_pow).sqrt().add(eps)
                             : v_batch.div(scalar_on_(one_minus_beta2_pow, p_batch)).sqrt().add(eps);
    const Series g_hat = capturable
                             ? g_batch.div(cap_terms.one_minus_mu_prod)
                             : g_batch.div(scalar_on_(one_minus_mu_prod, p_batch));
    const Series m_hat = capturable
                             ? m_batch.div(cap_terms.one_minus_mu_prod_next)
                             : m_batch.div(scalar_on_(one_minus_mu_prod_next, p_batch));
    const Series nesterov_m = capturable
                                  ? g_hat.mul(scalar_on_(1.0, p_batch).sub(cap_terms.mu_t))
                                        .add(m_hat.mul(cap_terms.mu_t_next))
                                  : g_hat.mul(scalar_on_(1.0 - mu_t, p_batch))
                                        .add(m_hat.mul(scalar_on_(mu_t_next, p_batch)));
    const Series delta = nesterov_m.div(denom).mul(lr);
    const Series next_batch = p_batch.sub(delta);

    for (std::size_t j = 0; j < active.size(); ++j) {
      const std::size_t i = active[j];
      Series* param_ptr = params[i];
      Series next_value = next_batch.select(0, static_cast<std::int64_t>(j)).reshape(param_ptr->sizes());
      exp_avg_[i] = m_batch.select(0, static_cast<std::int64_t>(j)).reshape(exp_avg_[i].sizes());
      exp_avg_sq_[i] = v_batch.select(0, static_cast<std::int64_t>(j)).reshape(exp_avg_sq_[i].sizes());
      assign_parameter_(*param_ptr, next_value);
    }
  }

  void apply_group_fused_(GroupState& group,
                          pluss::msharp::RangeView::Of<Series*> params,
                          pluss::msharp::RangeView::Series gradients) {
    const NAdamOptions& options = group.options;
    const bool capturable = options.execution.capturable;
    CapturableTerms cap_terms{};
    double mu_t = 0.0;
    double mu_t_next = 0.0;
    double one_minus_mu_prod = 0.0;
    double one_minus_mu_prod_next = 0.0;
    double one_minus_beta2_pow = 0.0;
    if (capturable) {
      cap_terms = advance_capturable_terms_(group);
    } else {
      mu_t = options.beta1 * (1.0 - 0.5 * std::pow(0.96, static_cast<double>(step_) * options.momentum_decay));
      mu_t_next =
          options.beta1 * (1.0 - 0.5 * std::pow(0.96, static_cast<double>(step_ + 1) * options.momentum_decay));
      group.mu_product *= mu_t;
      const double mu_product_next = group.mu_product * mu_t_next;
      one_minus_mu_prod = 1.0 - group.mu_product;
      one_minus_mu_prod_next = 1.0 - mu_product_next;
      one_minus_beta2_pow = 1.0 - std::pow(options.beta2, static_cast<double>(step_));
    }

    std::vector<std::size_t> active{};
    std::vector<Series> p_parts{};
    std::vector<Series> g_parts{};
    std::vector<Series> m_parts{};
    std::vector<Series> v_parts{};

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
      m_parts.push_back(exp_avg_[i].reshape(batched_shape_(exp_avg_[i])));
      v_parts.push_back(exp_avg_sq_[i].reshape(batched_shape_(exp_avg_sq_[i])));
    }
    if (active.empty()) return;

    Series p_batch = pack_batched_(p_parts, "pluss::ai::nn::optim::NAdam::apply_group_fused_");
    Series g_batch = pack_batched_(g_parts, "pluss::ai::nn::optim::NAdam::apply_group_fused_");
    Series m_batch = pack_batched_(m_parts, "pluss::ai::nn::optim::NAdam::apply_group_fused_");
    Series v_batch = pack_batched_(v_parts, "pluss::ai::nn::optim::NAdam::apply_group_fused_");
    const Series one = scalar_on_(1.0, p_batch);
    const Series beta1 = scalar_on_(options.beta1, p_batch);
    const Series beta2 = scalar_on_(options.beta2, p_batch);
    const Series one_minus_beta1 = one.sub(beta1);
    const Series one_minus_beta2 = one.sub(beta2);
    const Series lr = scalar_on_(options.learning_rate, p_batch);
    const Series eps = scalar_on_(options.epsilon, p_batch);

    if (options.weight_decay != 0.0) {
      if (options.decoupled_weight_decay) {
        const Series decay = scalar_on_(options.learning_rate * options.weight_decay, p_batch);
        p_batch = p_batch.mul(one.sub(decay));
      } else {
        g_batch = g_batch.add(p_batch.mul(scalar_on_(options.weight_decay, p_batch)));
      }
    }

    m_batch = m_batch.mul(beta1);
    m_batch = m_batch.add(g_batch.mul(one_minus_beta1));
    v_batch = v_batch.mul(beta2);
    v_batch = v_batch.add(g_batch.square().mul(one_minus_beta2));

    Series denom = capturable
                       ? v_batch.div(cap_terms.one_minus_beta2_pow).sqrt()
                       : v_batch.div(scalar_on_(one_minus_beta2_pow, p_batch)).sqrt();
    denom = denom.add(eps);
    Series g_hat = capturable
                       ? g_batch.div(cap_terms.one_minus_mu_prod)
                       : g_batch.div(scalar_on_(one_minus_mu_prod, p_batch));
    Series m_hat = capturable
                       ? m_batch.div(cap_terms.one_minus_mu_prod_next)
                       : m_batch.div(scalar_on_(one_minus_mu_prod_next, p_batch));
    Series nesterov_m = capturable
                            ? g_hat.mul(scalar_on_(1.0, p_batch).sub(cap_terms.mu_t))
                                  .add(m_hat.mul(cap_terms.mu_t_next))
                            : g_hat.mul(scalar_on_(1.0 - mu_t, p_batch))
                                  .add(m_hat.mul(scalar_on_(mu_t_next, p_batch)));
    Series delta = nesterov_m.div(denom);
    delta = delta.mul(lr);
    p_batch = p_batch.sub(delta);

    for (std::size_t j = 0; j < active.size(); ++j) {
      const std::size_t i = active[j];
      Series* param_ptr = params[i];
      Series next_value = p_batch.select(0, static_cast<std::int64_t>(j)).reshape(param_ptr->sizes());
      exp_avg_[i] = m_batch.select(0, static_cast<std::int64_t>(j)).reshape(exp_avg_[i].sizes());
      exp_avg_sq_[i] = v_batch.select(0, static_cast<std::int64_t>(j)).reshape(exp_avg_sq_[i].sizes());
      assign_parameter_(*param_ptr, next_value);
    }
  }

  static void validate_options_(const NAdamOptions& options) {
    if (!(options.learning_rate > 0.0) || !std::isfinite(options.learning_rate)) {
      ::pluss::throw_error("pluss::ai::nn::optim::NAdam: learning_rate must be positive and finite");
    }
    if (!(options.beta1 >= 0.0 && options.beta1 < 1.0) || !std::isfinite(options.beta1)) {
      ::pluss::throw_error("pluss::ai::nn::optim::NAdam: beta1 must be in [0, 1)");
    }
    if (!(options.beta2 >= 0.0 && options.beta2 < 1.0) || !std::isfinite(options.beta2)) {
      ::pluss::throw_error("pluss::ai::nn::optim::NAdam: beta2 must be in [0, 1)");
    }
    if (!(options.epsilon > 0.0) || !std::isfinite(options.epsilon)) {
      ::pluss::throw_error("pluss::ai::nn::optim::NAdam: epsilon must be positive and finite");
    }
    if (options.weight_decay < 0.0 || !std::isfinite(options.weight_decay)) {
      ::pluss::throw_error("pluss::ai::nn::optim::NAdam: weight_decay must be finite and non-negative");
    }
    if (options.momentum_decay < 0.0 || !std::isfinite(options.momentum_decay)) {
      ::pluss::throw_error("pluss::ai::nn::optim::NAdam: momentum_decay must be finite and non-negative");
    }
  }

  static std::vector<Series*> flatten_groups_(pluss::msharp::RangeView::Of<NAdamGroup> groups) {
    if (groups.empty()) {
      ::pluss::throw_error("pluss::ai::nn::optim::NAdam: expected non-empty parameter groups");
    }
    std::vector<Series*> flat{};
    std::unordered_set<Series*> seen{};
    for (const auto& group : groups) {
      if (group.parameters.empty()) {
        ::pluss::throw_error("pluss::ai::nn::optim::NAdam: parameter group must not be empty");
      }
      for (Series* p : group.parameters) {
        if (p != nullptr && !seen.insert(p).second) {
          ::pluss::throw_error("pluss::ai::nn::optim::NAdam: duplicate parameter across groups");
        }
        flat.push_back(p);
      }
    }
    return flat;
  }

  static std::vector<GroupState> build_group_states_(pluss::msharp::RangeView::Of<NAdamGroup> groups) {
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
        ::pluss::throw_error("pluss::ai::nn::optim::NAdam: capturable mode expects defined group parameters");
      }
      for (std::size_t i : group.indices) {
        const Series* p = params[i];
        if (p == nullptr || !p->defined()) continue;
        if (p->platform() != anchor->platform()) {
          ::pluss::throw_error(
              "pluss::ai::nn::optim::NAdam: capturable mode requires same platform across each group");
        }
        if (p->scalar_type() != anchor->scalar_type()) {
          ::pluss::throw_error(
              "pluss::ai::nn::optim::NAdam: capturable mode requires same dtype across each group");
        }
      }
    }
  }

  void initialize_state_() {
    const auto& params = parameters();
    exp_avg_.clear();
    exp_avg_sq_.clear();
    exp_avg_.reserve(params.size());
    exp_avg_sq_.reserve(params.size());
    for (const Series* p : params) {
      exp_avg_.push_back(state_zeros_like_(*p));
      exp_avg_sq_.push_back(state_zeros_like_(*p));
    }
    initialize_group_capture_state_();
  }

  void initialize_group_capture_state_() {
    const auto& params = parameters();
    for (auto& group : groups_) {
      group.mu_product = 1.0;
      group.mu_product_scalar = Series{};
      group.beta2_power = Series{};
      group.decay_power = Series{};
      if (!group.options.execution.capturable) continue;
      if (group.indices.empty()) continue;
      const Series* anchor = params[group.indices.front()];
      group.mu_product_scalar = Series::full({}, 1.0, anchor->scalar_type()).to(anchor->platform());
      group.beta2_power = Series::full({}, 1.0, anchor->scalar_type()).to(anchor->platform());
      group.decay_power = Series::full({}, 1.0, anchor->scalar_type()).to(anchor->platform());
    }
  }

  std::vector<GroupState> groups_{};
  NAdamOptions default_options_{};
  std::vector<Series> exp_avg_{};
  std::vector<Series> exp_avg_sq_{};
  std::int64_t step_{0};
};

} // namespace pluss::ai::nn::optim

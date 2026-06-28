#pragma once

#include "optim_base.h"
#include "pluss/subsp/math/diff.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace pluss::ai::nn::optim {

enum class MinimaxPrimalMethod {
  Adam,
  AdamW,
  SGD,
  RMSprop,
};

enum class MinimaxSchedule {
  Constant,
  Exponential,
  ViolationAdaptive,
};

struct MinimaxOptions {
  // Primal optimizer control.
  MinimaxPrimalMethod primal_method{MinimaxPrimalMethod::Adam};
  double learning_rate{1e-3};
  double beta1{0.9};
  double beta2{0.999};
  double epsilon{1e-8};
  double weight_decay{0.0};
  bool amsgrad{false};
  bool maximize{false};
  GradientClipOptions clip{};

  // Used for SGD and RMSprop momentum paths.
  double momentum{0.0};
  // RMSprop smoothing coefficient.
  double rmsprop_alpha{0.99};

  // InternalProtocol-inspired first-order constraint controls.
  double dual_learning_rate{1e-2};
  double equality_penalty{10.0};
  double inequality_penalty{10.0};
  bool project_bounds{true};
  bool enable_constraint_autodiff{true};
  std::int64_t nan_recovery_attempts{2};
  double nan_recovery_shrink{0.5};

  // Adaptive schedules.
  MinimaxSchedule dual_schedule{MinimaxSchedule::Constant};
  MinimaxSchedule penalty_schedule{MinimaxSchedule::Constant};
  double dual_exponential_decay{1.0};
  double penalty_exponential_growth{1.0};
  double violation_target{1e-3};
  double schedule_min_scale{0.25};
  double schedule_max_scale{4.0};
};

struct MinimaxConstraints {
  using ResidualFn = std::function<std::vector<Series>(const std::vector<Series>&)>;
  using TraceResidualFn =
      std::function<std::vector<pluss::subsp::math::diff::Trace>(const std::vector<pluss::subsp::math::diff::Trace>&)>;

  // Equality residuals c(x) = 0 and inequality residuals h(x) <= 0.
  ResidualFn equalities{};
  ResidualFn inequalities{};
  TraceResidualFn equalities_trace{};
  TraceResidualFn inequalities_trace{};

  // Optional per-parameter bounds. Empty means "no bounds".
  // If provided, size must match parameter count.
  std::vector<Series> lower_bounds{};
  std::vector<Series> upper_bounds{};
};

struct MinimaxGroup {
  std::vector<Series*> parameters{};
  MinimaxOptions options{};

  MinimaxGroup() = default;

  MinimaxGroup(pluss::msharp::RangeView::Of<Series*> params, MinimaxOptions opts = {})
      : parameters(params.begin(), params.end()),
        options(std::move(opts)) {}

  MinimaxGroup(std::initializer_list<Series*> params, MinimaxOptions opts = {})
      : parameters(params),
        options(std::move(opts)) {}
};

class PLUSS_API Minimax final : public Optimizer {
public:
  explicit Minimax(pluss::msharp::RangeView::Of<Series*> parameters,
                  MinimaxOptions options = {},
                  MinimaxConstraints constraints = {})
      : Optimizer(parameters),
        constraints_(std::move(constraints)) {
    validate_options_(options);
    GroupState group{};
    group.options = options;
    group.indices.resize(size());
    std::iota(group.indices.begin(), group.indices.end(), static_cast<std::size_t>(0));
    groups_.push_back(std::move(group));
    validate_constraints_();
    initialize_state_();
  }

  explicit Minimax(std::initializer_list<Series*> parameters,
                  MinimaxOptions options = {},
                  MinimaxConstraints constraints = {})
      : Minimax(pluss::msharp::RangeView::Of<Series*>{parameters}, std::move(options), std::move(constraints)) {}

  explicit Minimax(pluss::msharp::RangeView::Of<MinimaxGroup> groups, MinimaxConstraints constraints = {})
      : Optimizer(flatten_groups_(groups)),
        groups_(build_group_states_(groups)),
        constraints_(std::move(constraints)) {
    validate_group_options_();
    validate_constraints_();
    initialize_state_();
  }

  explicit Minimax(std::initializer_list<MinimaxGroup> groups, MinimaxConstraints constraints = {})
      : Minimax(pluss::msharp::RangeView::Of<MinimaxGroup>{groups}, std::move(constraints)) {}

  explicit Minimax(Module& module,
                  MinimaxOptions options = {},
                  MinimaxConstraints constraints = {})
      : Minimax(module.parameters(), std::move(options), std::move(constraints)) {}

  std::size_t group_count() const noexcept { return groups_.size(); }
  const MinimaxConstraints& constraints() const noexcept { return constraints_; }

  const MinimaxOptions& options() const noexcept { return groups_.front().options; }

  void set_options(const MinimaxOptions& options) {
    validate_options_(options);
    for (auto& group : groups_) {
      group.options = options;
    }
  }

  void set_group_options(std::size_t group_index, const MinimaxOptions& options) {
    if (group_index >= groups_.size()) {
      ::pluss::throw_error("pluss::ai::nn::optim::Minimax::set_group_options: group index out of range");
    }
    validate_options_(options);
    groups_[group_index].options = options;
    validate_group_options_();
  }

  void set_constraints(const MinimaxConstraints& constraints) {
    constraints_ = constraints;
    validate_constraints_();
    initialize_constraint_state_();
  }

  struct State {
    std::int64_t step{0};
    double moving_violation{0.0};
    std::vector<MinimaxOptions> group_options{};
    std::vector<Series> first_moment{};
    std::vector<Series> second_moment{};
    std::vector<Series> max_second_hat{};
    std::vector<Series> momentum_buffer{};
    std::vector<Series> rmsprop_avg{};
    std::vector<Series> eq_multipliers{};
    std::vector<Series> ineq_multipliers{};
    std::vector<Series> lower_multipliers{};
    std::vector<Series> upper_multipliers{};
  };

  State capture_state() const {
    State state{};
    state.step = step_;
    state.moving_violation = moving_violation_;
    state.group_options.reserve(groups_.size());
    for (const auto& group : groups_) {
      state.group_options.push_back(group.options);
    }
    state.first_moment = first_moment_;
    state.second_moment = second_moment_;
    state.max_second_hat = max_second_hat_;
    state.momentum_buffer = momentum_buffer_;
    state.rmsprop_avg = rmsprop_avg_;
    state.eq_multipliers = eq_multipliers_;
    state.ineq_multipliers = ineq_multipliers_;
    state.lower_multipliers = lower_multipliers_;
    state.upper_multipliers = upper_multipliers_;
    return state;
  }

  void restore_state(const State& state) {
    if (state.group_options.size() != groups_.size()) {
      ::pluss::throw_error("pluss::ai::nn::optim::Minimax::restore_state: group count mismatch");
    }
    if (state.step < 0) {
      ::pluss::throw_error("pluss::ai::nn::optim::Minimax::restore_state: step must be non-negative");
    }
    if (!std::isfinite(state.moving_violation) || state.moving_violation < 0.0) {
      ::pluss::throw_error("pluss::ai::nn::optim::Minimax::restore_state: moving_violation must be finite and >= 0");
    }
    for (const auto& options : state.group_options) {
      validate_options_(options);
    }
    for (std::size_t i = 0; i < groups_.size(); ++i) {
      groups_[i].options = state.group_options[i];
    }
    validate_group_options_();

    validate_state_vector_(state.first_moment, "pluss::ai::nn::optim::Minimax::restore_state[first_moment]");
    validate_state_vector_(state.second_moment, "pluss::ai::nn::optim::Minimax::restore_state[second_moment]");
    validate_state_vector_(state.max_second_hat, "pluss::ai::nn::optim::Minimax::restore_state[max_second_hat]");
    validate_state_vector_(state.momentum_buffer, "pluss::ai::nn::optim::Minimax::restore_state[momentum_buffer]");
    validate_state_vector_(state.rmsprop_avg, "pluss::ai::nn::optim::Minimax::restore_state[rmsprop_avg]");
    validate_constraint_state_(state.eq_multipliers, eq_multipliers_,
                               "pluss::ai::nn::optim::Minimax::restore_state[eq_multipliers]");
    validate_constraint_state_(state.ineq_multipliers, ineq_multipliers_,
                               "pluss::ai::nn::optim::Minimax::restore_state[ineq_multipliers]");
    validate_constraint_state_(state.lower_multipliers, lower_multipliers_,
                               "pluss::ai::nn::optim::Minimax::restore_state[lower_multipliers]");
    validate_constraint_state_(state.upper_multipliers, upper_multipliers_,
                               "pluss::ai::nn::optim::Minimax::restore_state[upper_multipliers]");

    first_moment_ = state.first_moment;
    second_moment_ = state.second_moment;
    max_second_hat_ = state.max_second_hat;
    momentum_buffer_ = state.momentum_buffer;
    rmsprop_avg_ = state.rmsprop_avg;
    eq_multipliers_ = state.eq_multipliers;
    ineq_multipliers_ = state.ineq_multipliers;
    lower_multipliers_ = state.lower_multipliers;
    upper_multipliers_ = state.upper_multipliers;
    step_ = state.step;
    moving_violation_ = state.moving_violation;
  }

private:
  struct GroupState {
    std::vector<std::size_t> indices{};
    MinimaxOptions options{};
  };

  static bool same_shape_(const Series& a, const Series& b) {
    const auto as = a.sizes();
    const auto bs = b.sizes();
    if (as.size() != bs.size()) return false;
    for (std::size_t i = 0; i < as.size(); ++i) {
      if (as[i] != bs[i]) return false;
    }
    return true;
  }

  static bool finite_series_(const Series& x) {
    const Series finite = x.isfinite().all();
    if (finite.platform() == pluss::pCUDA && !pluss::cuda::cpu_bridge_enabled()) {
      ::pluss::throw_error(
          "pluss::ai::nn::optim::Minimax: CUDA finite recovery checks require explicit CPU scalar "
          "materialization; enable PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 or disable nan recovery");
    }
    return finite.platform() == pluss::pCUDA ? finite.to(pluss::pCPU).item<bool>() : finite.item<bool>();
  }

  static void validate_options_(const MinimaxOptions& options) {
    if (!(options.learning_rate > 0.0) || !std::isfinite(options.learning_rate)) {
      ::pluss::throw_error("pluss::ai::nn::optim::Minimax: learning_rate must be positive and finite");
    }
    if (!(options.beta1 >= 0.0 && options.beta1 < 1.0) || !std::isfinite(options.beta1)) {
      ::pluss::throw_error("pluss::ai::nn::optim::Minimax: beta1 must be in [0, 1)");
    }
    if (!(options.beta2 >= 0.0 && options.beta2 < 1.0) || !std::isfinite(options.beta2)) {
      ::pluss::throw_error("pluss::ai::nn::optim::Minimax: beta2 must be in [0, 1)");
    }
    if (!(options.epsilon > 0.0) || !std::isfinite(options.epsilon)) {
      ::pluss::throw_error("pluss::ai::nn::optim::Minimax: epsilon must be positive and finite");
    }
    if (options.weight_decay < 0.0 || !std::isfinite(options.weight_decay)) {
      ::pluss::throw_error("pluss::ai::nn::optim::Minimax: weight_decay must be finite and non-negative");
    }
    if (!(options.momentum >= 0.0) || !std::isfinite(options.momentum)) {
      ::pluss::throw_error("pluss::ai::nn::optim::Minimax: momentum must be finite and >= 0");
    }
    if (!(options.rmsprop_alpha >= 0.0 && options.rmsprop_alpha < 1.0) || !std::isfinite(options.rmsprop_alpha)) {
      ::pluss::throw_error("pluss::ai::nn::optim::Minimax: rmsprop_alpha must be in [0,1)");
    }
    if (!(options.dual_learning_rate >= 0.0) || !std::isfinite(options.dual_learning_rate)) {
      ::pluss::throw_error("pluss::ai::nn::optim::Minimax: dual_learning_rate must be finite and >= 0");
    }
    if (!(options.equality_penalty >= 0.0) || !std::isfinite(options.equality_penalty)) {
      ::pluss::throw_error("pluss::ai::nn::optim::Minimax: equality_penalty must be finite and >= 0");
    }
    if (!(options.inequality_penalty >= 0.0) || !std::isfinite(options.inequality_penalty)) {
      ::pluss::throw_error("pluss::ai::nn::optim::Minimax: inequality_penalty must be finite and >= 0");
    }
    if (options.nan_recovery_attempts < 0) {
      ::pluss::throw_error("pluss::ai::nn::optim::Minimax: nan_recovery_attempts must be >= 0");
    }
    if (!(options.nan_recovery_shrink > 0.0 && options.nan_recovery_shrink < 1.0) ||
        !std::isfinite(options.nan_recovery_shrink)) {
      ::pluss::throw_error("pluss::ai::nn::optim::Minimax: nan_recovery_shrink must be in (0,1)");
    }
    if (!(options.dual_exponential_decay > 0.0) || !std::isfinite(options.dual_exponential_decay)) {
      ::pluss::throw_error("pluss::ai::nn::optim::Minimax: dual_exponential_decay must be finite and > 0");
    }
    if (!(options.penalty_exponential_growth > 0.0) || !std::isfinite(options.penalty_exponential_growth)) {
      ::pluss::throw_error("pluss::ai::nn::optim::Minimax: penalty_exponential_growth must be finite and > 0");
    }
    if (!(options.violation_target > 0.0) || !std::isfinite(options.violation_target)) {
      ::pluss::throw_error("pluss::ai::nn::optim::Minimax: violation_target must be finite and > 0");
    }
    if (!(options.schedule_min_scale > 0.0 && options.schedule_min_scale <= options.schedule_max_scale) ||
        !std::isfinite(options.schedule_min_scale) || !std::isfinite(options.schedule_max_scale)) {
      ::pluss::throw_error("pluss::ai::nn::optim::Minimax: invalid schedule scale bounds");
    }
  }

  static std::vector<Series*> flatten_groups_(pluss::msharp::RangeView::Of<MinimaxGroup> groups) {
    if (groups.empty()) {
      ::pluss::throw_error("pluss::ai::nn::optim::Minimax: expected non-empty parameter groups");
    }
    std::vector<Series*> flat{};
    std::unordered_set<Series*> seen{};
    for (const auto& group : groups) {
      if (group.parameters.empty()) {
        ::pluss::throw_error("pluss::ai::nn::optim::Minimax: parameter group must not be empty");
      }
      for (Series* p : group.parameters) {
        if (p != nullptr && !seen.insert(p).second) {
          ::pluss::throw_error("pluss::ai::nn::optim::Minimax: duplicate parameter across groups");
        }
        flat.push_back(p);
      }
    }
    return flat;
  }

  static std::vector<GroupState> build_group_states_(pluss::msharp::RangeView::Of<MinimaxGroup> groups) {
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

  static bool same_constraint_controls_(const MinimaxOptions& a, const MinimaxOptions& b) {
    return a.dual_learning_rate == b.dual_learning_rate &&
           a.equality_penalty == b.equality_penalty &&
           a.inequality_penalty == b.inequality_penalty &&
           a.project_bounds == b.project_bounds &&
           a.enable_constraint_autodiff == b.enable_constraint_autodiff &&
           a.dual_schedule == b.dual_schedule &&
           a.penalty_schedule == b.penalty_schedule &&
           a.dual_exponential_decay == b.dual_exponential_decay &&
           a.penalty_exponential_growth == b.penalty_exponential_growth &&
           a.violation_target == b.violation_target &&
           a.schedule_min_scale == b.schedule_min_scale &&
           a.schedule_max_scale == b.schedule_max_scale;
  }

  const MinimaxOptions& constraint_options_() const noexcept {
    return groups_.front().options;
  }

  void validate_group_options_() const {
    for (const auto& group : groups_) {
      validate_options_(group.options);
    }
    for (std::size_t i = 1; i < groups_.size(); ++i) {
      if (!same_constraint_controls_(groups_[0].options, groups_[i].options)) {
        ::pluss::throw_error(
            "pluss::ai::nn::optim::Minimax: all groups must share identical constraint control options");
      }
    }
  }

  static bool has_constraints_(const MinimaxConstraints& c) {
    return static_cast<bool>(c.equalities) ||
           static_cast<bool>(c.inequalities) ||
           !c.lower_bounds.empty() ||
           !c.upper_bounds.empty();
  }

  static void validate_constraint_state_(pluss::msharp::RangeView::Series state,
                                         pluss::msharp::RangeView::Series ref,
                                         const char* where) {
    if (state.size() != ref.size()) {
      ::pluss::throw_error(std::string(where) + ": state size mismatch");
    }
    for (std::size_t i = 0; i < state.size(); ++i) {
      const Series& s = state[i];
      const Series& r = ref[i];
      if (s.defined() != r.defined()) {
        ::pluss::throw_error(std::string(where) + ": definedness mismatch");
      }
      if (!s.defined()) continue;
      if (s.platform() != r.platform()) {
        ::pluss::throw_error(std::string(where) + ": platform mismatch");
      }
      if (!same_shape_(s, r)) {
        ::pluss::throw_error(std::string(where) + ": shape mismatch");
      }
    }
  }

  void validate_bounds_vector_(pluss::msharp::RangeView::Series bounds, const char* name) const {
    if (bounds.empty()) return;
    if (bounds.size() != size()) {
      ::pluss::throw_error(std::string("pluss::ai::nn::optim::Minimax: ") + name + " size mismatch");
    }
    const auto& params = parameters();
    for (std::size_t i = 0; i < bounds.size(); ++i) {
      if (!bounds[i].defined()) continue;
      if (bounds[i].platform() != params[i]->platform()) {
        ::pluss::throw_error(std::string("pluss::ai::nn::optim::Minimax: ") + name + " platform mismatch");
      }
      if (!same_shape_(bounds[i], *params[i]) && bounds[i].numel() != 1) {
        ::pluss::throw_error(std::string("pluss::ai::nn::optim::Minimax: ") + name + " shape mismatch");
      }
    }
  }

  void validate_constraints_() const {
    validate_bounds_vector_(constraints_.lower_bounds, "lower_bounds");
    validate_bounds_vector_(constraints_.upper_bounds, "upper_bounds");

    if (!constraints_.equalities && constraints_.equalities_trace) {
      ::pluss::throw_error(
          "pluss::ai::nn::optim::Minimax: equalities_trace requires equalities callback");
    }
    if (!constraints_.inequalities && constraints_.inequalities_trace) {
      ::pluss::throw_error(
          "pluss::ai::nn::optim::Minimax: inequalities_trace requires inequalities callback");
    }

    if (!constraint_options_().enable_constraint_autodiff) return;
    if (constraints_.equalities && !constraints_.equalities_trace) {
      ::pluss::throw_error(
          "pluss::ai::nn::optim::Minimax: equalities callback requires equalities_trace for autodiff constraints");
    }
    if (constraints_.inequalities && !constraints_.inequalities_trace) {
      ::pluss::throw_error(
          "pluss::ai::nn::optim::Minimax: inequalities callback requires inequalities_trace for autodiff constraints");
    }
  }

  void initialize_state_() {
    const auto& params = parameters();
    first_moment_.clear();
    second_moment_.clear();
    max_second_hat_.clear();
    momentum_buffer_.clear();
    rmsprop_avg_.clear();

    first_moment_.reserve(params.size());
    second_moment_.reserve(params.size());
    max_second_hat_.reserve(params.size());
    momentum_buffer_.reserve(params.size());
    rmsprop_avg_.reserve(params.size());

    for (const Series* p : params) {
      first_moment_.push_back(state_zeros_like_(*p));
      second_moment_.push_back(state_zeros_like_(*p));
      max_second_hat_.push_back(state_zeros_like_(*p));
      momentum_buffer_.push_back(state_zeros_like_(*p));
      rmsprop_avg_.push_back(state_zeros_like_(*p));
    }

    moving_violation_ = 0.0;
    initialize_constraint_state_();
  }

  void initialize_constraint_state_() {
    eq_multipliers_.clear();
    ineq_multipliers_.clear();
    lower_multipliers_.clear();
    upper_multipliers_.clear();

    const auto& params = parameters();
    lower_multipliers_.resize(params.size());
    upper_multipliers_.resize(params.size());

    for (std::size_t i = 0; i < params.size(); ++i) {
      if (!constraints_.lower_bounds.empty() && constraints_.lower_bounds[i].defined()) {
        lower_multipliers_[i] = pluss::zeros_like(*params[i], params[i]->scalar_type());
      }
      if (!constraints_.upper_bounds.empty() && constraints_.upper_bounds[i].defined()) {
        upper_multipliers_[i] = pluss::zeros_like(*params[i], params[i]->scalar_type());
      }
    }

    if (!constraints_.equalities && !constraints_.inequalities) return;
    std::vector<Series> params_value;
    params_value.reserve(params.size());
    for (const Series* p : params) params_value.push_back(p->clone());

    if (constraints_.equalities) {
      const auto residuals = constraints_.equalities(params_value);
      eq_multipliers_.reserve(residuals.size());
      for (const auto& r : residuals) {
        eq_multipliers_.push_back(pluss::zeros_like(r, r.scalar_type()));
      }
    }

    if (constraints_.inequalities) {
      const auto residuals = constraints_.inequalities(params_value);
      ineq_multipliers_.reserve(residuals.size());
      for (const auto& r : residuals) {
        ineq_multipliers_.push_back(pluss::zeros_like(r, r.scalar_type()));
      }
    }
  }

  static pluss::subsp::math::diff::Trace scalar_zero_trace_(
      const std::vector<pluss::subsp::math::diff::Trace>& vars) {
    using pluss::subsp::math::diff::sum;
    return sum(vars[0] * 0.0);
  }

  pluss::subsp::math::diff::Trace constraint_objective_trace_(
      const std::vector<pluss::subsp::math::diff::Trace>& vars,
      double equality_penalty,
      double inequality_penalty) const {
    using pluss::subsp::math::diff::Trace;
    using pluss::subsp::math::diff::relu;
    using pluss::subsp::math::diff::square;
    using pluss::subsp::math::diff::sum;

    Trace total = scalar_zero_trace_(vars);

    if (constraints_.equalities_trace) {
      const auto residuals = constraints_.equalities_trace(vars);
      for (std::size_t i = 0; i < residuals.size(); ++i) {
        const Trace c = residuals[i];
        const Trace lambda(eq_multipliers_[i], false);
        total = total + sum(c * lambda);
        if (equality_penalty > 0.0) {
          total = total + sum(square(c)) * (0.5 * equality_penalty);
        }
      }
    }

    if (constraints_.inequalities_trace) {
      const auto residuals = constraints_.inequalities_trace(vars);
      for (std::size_t i = 0; i < residuals.size(); ++i) {
        const Trace h = relu(residuals[i]);
        const Trace mu(ineq_multipliers_[i], false);
        total = total + sum(h * mu);
        if (inequality_penalty > 0.0) {
          total = total + sum(square(h)) * (0.5 * inequality_penalty);
        }
      }
    }

    for (std::size_t i = 0; i < vars.size(); ++i) {
      if (!constraints_.lower_bounds.empty() && constraints_.lower_bounds[i].defined()) {
        const Trace lb(constraints_.lower_bounds[i], false);
        const Trace h = relu(lb - vars[i]);
        const Trace mu(lower_multipliers_[i], false);
        total = total + sum(h * mu);
        if (inequality_penalty > 0.0) {
          total = total + sum(square(h)) * (0.5 * inequality_penalty);
        }
      }
      if (!constraints_.upper_bounds.empty() && constraints_.upper_bounds[i].defined()) {
        const Trace ub(constraints_.upper_bounds[i], false);
        const Trace h = relu(vars[i] - ub);
        const Trace mu(upper_multipliers_[i], false);
        total = total + sum(h * mu);
        if (inequality_penalty > 0.0) {
          total = total + sum(square(h)) * (0.5 * inequality_penalty);
        }
      }
    }

    return total;
  }

  void apply_bounds_(Series& candidate, std::size_t index) const {
    if (constraints_.lower_bounds.empty() && constraints_.upper_bounds.empty()) return;
    if (!constraint_options_().project_bounds) return;
    if (!constraints_.lower_bounds.empty() && constraints_.lower_bounds[index].defined()) {
      candidate = candidate.maximum(constraints_.lower_bounds[index]);
    }
    if (!constraints_.upper_bounds.empty() && constraints_.upper_bounds[index].defined()) {
      candidate = candidate.minimum(constraints_.upper_bounds[index]);
    }
  }

  Series nan_recover_step_(const Series& parameter_value,
                           const Series& delta,
                           std::size_t index,
                           const MinimaxOptions& options) const {
    Series candidate = parameter_value.sub(delta);
    apply_bounds_(candidate, index);
    if (finite_series_(candidate)) {
      return candidate;
    }

    Series scaled = delta;
    for (std::int64_t k = 0; k < options.nan_recovery_attempts; ++k) {
      scaled = scaled.mul(options.nan_recovery_shrink);
      candidate = parameter_value.sub(scaled);
      apply_bounds_(candidate, index);
      if (finite_series_(candidate)) {
        return candidate;
      }
    }
    return parameter_value;
  }

  static double reduction_max_abs_(const Series& x) {
    const Series reduced = x.abs().max();
    if (reduced.platform() == pluss::pCUDA && !pluss::cuda::cpu_bridge_enabled()) {
      ::pluss::throw_error(
          "pluss::ai::nn::optim::Minimax: CUDA violation scheduling requires explicit CPU scalar "
          "materialization; enable PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 or use a constant schedule");
    }
    return reduced.platform() == pluss::pCUDA ? reduced.to(pluss::pCPU).item<double>() : reduced.item<double>();
  }

  static double reduction_max_violation_(const Series& x) {
    const Series reduced = x.relu().max();
    if (reduced.platform() == pluss::pCUDA && !pluss::cuda::cpu_bridge_enabled()) {
      ::pluss::throw_error(
          "pluss::ai::nn::optim::Minimax: CUDA violation scheduling requires explicit CPU scalar "
          "materialization; enable PLUSS_CUDA_ALLOW_CPU_BRIDGE=1 or use a constant schedule");
    }
    return reduced.platform() == pluss::pCUDA ? reduced.to(pluss::pCPU).item<double>() : reduced.item<double>();
  }

  double schedule_scale_(MinimaxSchedule schedule, const MinimaxOptions& options) const {
    switch (schedule) {
      case MinimaxSchedule::Constant:
        return 1.0;
      case MinimaxSchedule::Exponential:
        return 1.0; // handled by dedicated exponential fields below
      case MinimaxSchedule::ViolationAdaptive: {
        const double ratio = moving_violation_ / std::max(options.violation_target, 1e-15);
        return std::clamp(ratio, options.schedule_min_scale, options.schedule_max_scale);
      }
    }
    return 1.0;
  }

  double effective_dual_lr_(const MinimaxOptions& options) const {
    double lr = options.dual_learning_rate;
    if (options.dual_schedule == MinimaxSchedule::Exponential) {
      lr *= std::pow(options.dual_exponential_decay, static_cast<double>(std::max<std::int64_t>(0, step_ - 1)));
    } else if (options.dual_schedule == MinimaxSchedule::ViolationAdaptive) {
      lr *= schedule_scale_(options.dual_schedule, options);
    }
    return lr;
  }

  std::pair<double, double> effective_penalties_(const MinimaxOptions& options) const {
    double eq = options.equality_penalty;
    double ineq = options.inequality_penalty;
    if (options.penalty_schedule == MinimaxSchedule::Exponential) {
      const double scale =
          std::pow(options.penalty_exponential_growth, static_cast<double>(std::max<std::int64_t>(0, step_ - 1)));
      eq *= scale;
      ineq *= scale;
    } else if (options.penalty_schedule == MinimaxSchedule::ViolationAdaptive) {
      const double scale = schedule_scale_(options.penalty_schedule, options);
      eq *= scale;
      ineq *= scale;
    }
    return {eq, ineq};
  }

  double update_constraint_multipliers_(const std::vector<Series>& params_value, double dual_lr) {
    if (!has_constraints_(constraints_)) return 0.0;
    if (dual_lr <= 0.0) return 0.0;

    double max_violation = 0.0;

    if (constraints_.equalities) {
      const auto residuals = constraints_.equalities(params_value);
      if (residuals.size() != eq_multipliers_.size()) {
        ::pluss::throw_error("pluss::ai::nn::optim::Minimax: equalities residual size changed at runtime");
      }
      for (std::size_t i = 0; i < residuals.size(); ++i) {
        if (!same_shape_(residuals[i], eq_multipliers_[i])) {
          ::pluss::throw_error("pluss::ai::nn::optim::Minimax: equality residual shape changed at runtime");
        }
        eq_multipliers_[i] = eq_multipliers_[i].add(residuals[i].mul(dual_lr));
        max_violation = std::max(max_violation, reduction_max_abs_(residuals[i]));
      }
    }

    if (constraints_.inequalities) {
      const auto residuals = constraints_.inequalities(params_value);
      if (residuals.size() != ineq_multipliers_.size()) {
        ::pluss::throw_error("pluss::ai::nn::optim::Minimax: inequalities residual size changed at runtime");
      }
      for (std::size_t i = 0; i < residuals.size(); ++i) {
        if (!same_shape_(residuals[i], ineq_multipliers_[i])) {
          ::pluss::throw_error("pluss::ai::nn::optim::Minimax: inequality residual shape changed at runtime");
        }
        const Series violation = residuals[i].relu();
        ineq_multipliers_[i] = ineq_multipliers_[i]
                                   .add(violation.mul(dual_lr))
                                   .clamp_min(0.0);
        max_violation = std::max(max_violation, reduction_max_violation_(residuals[i]));
      }
    }

    for (std::size_t i = 0; i < params_value.size(); ++i) {
      if (!constraints_.lower_bounds.empty() && constraints_.lower_bounds[i].defined()) {
        const Series violation = constraints_.lower_bounds[i].sub(params_value[i]).relu();
        lower_multipliers_[i] = lower_multipliers_[i]
                                    .add(violation.mul(dual_lr))
                                    .clamp_min(0.0);
        max_violation = std::max(max_violation, reduction_max_violation_(constraints_.lower_bounds[i].sub(params_value[i])));
      }
      if (!constraints_.upper_bounds.empty() && constraints_.upper_bounds[i].defined()) {
        const Series violation = params_value[i].sub(constraints_.upper_bounds[i]).relu();
        upper_multipliers_[i] = upper_multipliers_[i]
                                    .add(violation.mul(dual_lr))
                                    .clamp_min(0.0);
        max_violation = std::max(max_violation, reduction_max_violation_(params_value[i].sub(constraints_.upper_bounds[i])));
      }
    }

    return max_violation;
  }

  Series primal_delta_(std::size_t i,
                       const Series& sensitivity,
                       const Series& parameter_value,
                       const MinimaxOptions& options,
                       double b1_correction,
                       double b2_correction) {
    Series g = sensitivity;

    switch (options.primal_method) {
      case MinimaxPrimalMethod::Adam:
      case MinimaxPrimalMethod::AdamW: {
        const Series one = scalar_on_(1.0, parameter_value);
        const Series beta1 = scalar_on_(options.beta1, parameter_value);
        const Series beta2 = scalar_on_(options.beta2, parameter_value);
        const Series one_minus_beta1 = one.sub(beta1);
        const Series one_minus_beta2 = one.sub(beta2);
        const Series lr = scalar_on_(options.learning_rate, parameter_value);
        const Series eps = scalar_on_(options.epsilon, parameter_value);
        if (options.primal_method == MinimaxPrimalMethod::Adam && options.weight_decay != 0.0) {
          g = g.add(parameter_value.mul(scalar_on_(options.weight_decay, parameter_value)));
        }

        first_moment_[i] = first_moment_[i].mul(beta1).add(g.mul(one_minus_beta1));
        second_moment_[i] = second_moment_[i].mul(beta2).add(g.square().mul(one_minus_beta2));

        Series second_hat = second_moment_[i].div(scalar_on_(b2_correction, parameter_value));
        if (options.amsgrad) {
          max_second_hat_[i] = max_second_hat_[i].maximum(second_hat);
          second_hat = max_second_hat_[i];
        }

        const Series first_hat = first_moment_[i].div(scalar_on_(b1_correction, parameter_value));
        Series delta = first_hat.div(second_hat.sqrt().add(eps)).mul(lr);
        if (options.primal_method == MinimaxPrimalMethod::AdamW && options.weight_decay != 0.0) {
          delta = delta.add(parameter_value.mul(scalar_on_(options.learning_rate * options.weight_decay, parameter_value)));
        }
        return delta;
      }

      case MinimaxPrimalMethod::SGD: {
        const Series momentum = scalar_on_(options.momentum, parameter_value);
        const Series lr = scalar_on_(options.learning_rate, parameter_value);
        if (options.weight_decay != 0.0) {
          g = g.add(parameter_value.mul(scalar_on_(options.weight_decay, parameter_value)));
        }
        if (options.momentum > 0.0) {
          momentum_buffer_[i] = momentum_buffer_[i].mul(momentum).add(g);
          g = momentum_buffer_[i];
        }
        return g.mul(lr);
      }

      case MinimaxPrimalMethod::RMSprop: {
        const Series one = scalar_on_(1.0, parameter_value);
        const Series alpha = scalar_on_(options.rmsprop_alpha, parameter_value);
        const Series one_minus_alpha = one.sub(alpha);
        const Series eps = scalar_on_(options.epsilon, parameter_value);
        const Series momentum = scalar_on_(options.momentum, parameter_value);
        const Series lr = scalar_on_(options.learning_rate, parameter_value);
        if (options.weight_decay != 0.0) {
          g = g.add(parameter_value.mul(scalar_on_(options.weight_decay, parameter_value)));
        }
        rmsprop_avg_[i] = rmsprop_avg_[i].mul(alpha).add(g.square().mul(one_minus_alpha));
        Series update = g.div(rmsprop_avg_[i].sqrt().add(eps));
        if (options.momentum > 0.0) {
          momentum_buffer_[i] = momentum_buffer_[i].mul(momentum).add(update);
          update = momentum_buffer_[i];
        }
        return update.mul(lr);
      }
    }

    return g.mul(scalar_on_(options.learning_rate, parameter_value));
  }

  void apply_step_(pluss::msharp::RangeView::Series gradients) override {
    ++step_;
    auto& params = mutable_parameters_();

    std::vector<Series> params_value;
    params_value.reserve(params.size());
    for (Series* p : params) {
      params_value.push_back(*p);
    }

    const MinimaxOptions& copt = constraint_options_();
    const auto [eq_penalty, ineq_penalty] = effective_penalties_(copt);
    const double dual_lr = effective_dual_lr_(copt);

    std::vector<Series> total_grads(gradients.begin(), gradients.end());
    for (std::size_t i = 0; i < total_grads.size(); ++i) {
      if (!total_grads[i].defined()) continue;
      // Group-specific clipping/maximize is applied below.
    }

    if (copt.enable_constraint_autodiff && has_constraints_(constraints_)) {
      const auto [penalty_value, constraint_grads] = pluss::subsp::math::diff::value_and_grad(
          [this, eq_penalty, ineq_penalty](const std::vector<pluss::subsp::math::diff::Trace>& vars) {
            return this->constraint_objective_trace_(vars, eq_penalty, ineq_penalty);
          },
          params_value);
      (void)penalty_value;
      if (constraint_grads.size() != total_grads.size()) {
        ::pluss::throw_error("pluss::ai::nn::optim::Minimax: constraint gradient size mismatch");
      }
      for (std::size_t i = 0; i < total_grads.size(); ++i) {
        if (!constraint_grads[i].defined()) continue;
        if (total_grads[i].defined()) {
          total_grads[i] = total_grads[i].add(constraint_grads[i]);
        } else {
          total_grads[i] = constraint_grads[i];
        }
      }
    }

    for (const auto& group : groups_) {
      const MinimaxOptions& options = group.options;
      const double b1_correction = 1.0 - std::pow(options.beta1, static_cast<double>(step_));
      const double b2_correction = 1.0 - std::pow(options.beta2, static_cast<double>(step_));

      for (std::size_t i : group.indices) {
        Series* param_ptr = params[i];
        if (param_ptr == nullptr) continue;

        Series sensitivity = total_grads[i];
        if (!sensitivity.defined()) continue;
        sensitivity = clip_gradient_(sensitivity, options.clip);
        if (options.maximize) sensitivity = sensitivity.neg();

        Series parameter_value = *param_ptr;
        const Series delta = primal_delta_(i, sensitivity, parameter_value, options, b1_correction, b2_correction);
        Series next_value = nan_recover_step_(parameter_value, delta, i, options);

        assign_parameter_(*param_ptr, next_value);
        params_value[i] = next_value;
      }
    }

    const double violation = update_constraint_multipliers_(params_value, dual_lr);
    moving_violation_ = 0.9 * moving_violation_ + 0.1 * violation;
  }

  bool requires_recording_during_step_() const override {
    return constraint_options_().enable_constraint_autodiff && has_constraints_(constraints_);
  }

  std::vector<GroupState> groups_{};
  MinimaxConstraints constraints_{};

  std::vector<Series> first_moment_{};
  std::vector<Series> second_moment_{};
  std::vector<Series> max_second_hat_{};
  std::vector<Series> momentum_buffer_{};
  std::vector<Series> rmsprop_avg_{};

  std::vector<Series> eq_multipliers_{};
  std::vector<Series> ineq_multipliers_{};
  std::vector<Series> lower_multipliers_{};
  std::vector<Series> upper_multipliers_{};

  std::int64_t step_{0};
  double moving_violation_{0.0};
};

} // namespace pluss::ai::nn::optim

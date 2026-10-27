#pragma once

#include "pluss/subsp/math/diff.h"
#include "pluss/subsp/math/linalg.h"
#include "pluss/subsp/math/parameter_pack.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <complex>
#include <concepts>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <limits>
#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <iomanip>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace pluss::subsp::math::optim {

// The tape-recording types are an implementation detail: callbacks are written over plain Series.
namespace detail {
using Trace = pluss::subsp::math::diff::Trace;
using pluss::subsp::math::diff::trace;
using pluss::subsp::math::diff::fixed;
}  // namespace detail

// Compatibility re-export: existing callers can continue to use the older
// optim-facing trace helpers while new derivative-oriented code targets
// pluss::subsp::math::diff directly.
using pluss::subsp::math::diff::HvpOptions;
using GradientAuditOptions = pluss::subsp::math::diff::DerivativeAuditOptions;
using CurvatureAuditOptions = pluss::subsp::math::diff::DerivativeAuditOptions;
using GradientAuditReport = pluss::subsp::math::diff::DerivativeAuditReport;
using CurvatureAuditReport = pluss::subsp::math::diff::DerivativeAuditReport;
using pluss::subsp::math::diff::clear_sensitivities;
using pluss::subsp::math::diff::grad;
using pluss::subsp::math::diff::hvp;
using pluss::subsp::math::diff::jvp;
using pluss::subsp::math::diff::gradient_audit;
using pluss::subsp::math::diff::curvature_audit;
using pluss::subsp::math::diff::gradient_audit_report;
using pluss::subsp::math::diff::curvature_audit_report;
using pluss::subsp::math::diff::linearize;
using pluss::subsp::math::diff::nth_diff;
using pluss::subsp::math::diff::vjp;
using pluss::subsp::math::diff::value_and_grad;

namespace detail {

// A user callback over plain Series (one array, called "x", or a pack of named blocks) is adapted to the
// internal tape-recording form the engine differentiates. Callbacks take and return Series only.
template <class Fn>
std::function<detail::Trace(const ParameterPack<detail::Trace>&)> trace_pack_callback_(Fn&& fn) {
  using F = std::decay_t<Fn>;
  static_assert(!std::invocable<F&, const ParameterPack<detail::Trace>&> || std::invocable<F&, const ParameterPack<Series>&>,
                "optim: a callback must take a Series or a ParameterPack<Series>");
  static_assert(!std::invocable<F&, const detail::Trace&> || std::invocable<F&, const Series&>,
                "optim: a callback must take a Series or a ParameterPack<Series>");
  if constexpr (std::invocable<F&, const ParameterPack<Series>&>) {
    return [f = std::forward<Fn>(fn)](const ParameterPack<detail::Trace>& p) -> detail::Trace {
      std::vector<Series> values;
      values.reserve(p.size());
      for (const auto& block : p.blocks()) values.push_back(block.value.value());
      return detail::Trace(f(p.template rebind<Series>(std::move(values))));
    };
  } else {
    static_assert(std::invocable<F&, const Series&>,
                  "optim: a callback must take a Series or a ParameterPack<Series>");
    return [f = std::forward<Fn>(fn)](const ParameterPack<detail::Trace>& p) -> detail::Trace { return detail::Trace(f(p.at("x").value())); };
  }
}

}  // namespace detail

// A callback the engine records on a tape: a function over plain Series (one array "x", or a ParameterPack<Series>).
class TraceCallback : public std::function<detail::Trace(const ParameterPack<detail::Trace>&)> {
 public:
  using Base = std::function<detail::Trace(const ParameterPack<detail::Trace>&)>;
  TraceCallback() = default;
  TraceCallback(std::nullptr_t) noexcept : Base(nullptr) {}
  TraceCallback(Base fn) : Base(std::move(fn)) {}
  template <class Fn>
  requires(!std::is_same_v<std::decay_t<Fn>, TraceCallback> && !std::is_same_v<std::decay_t<Fn>, Base> &&
           !std::is_null_pointer_v<std::decay_t<Fn>>)
  TraceCallback(Fn&& fn) : Base(detail::trace_pack_callback_(std::forward<Fn>(fn))) {}
};

namespace detail {

template <class Fn>
std::pair<Series, ParameterPack<Series>> value_and_grad_trace_(Fn&& fn, const ParameterPack<Series>& params) {
  pluss::subsp::math::diff::DerivativePolicy policy{};
  policy.warn_on_disconnected = false;
  policy.keep_graph = false;
  auto [value, grads] = pluss::subsp::math::diff::value_and_grad(
      [&](const std::vector<detail::Trace>& xs) -> detail::Trace {
        return std::forward<Fn>(fn)(params.template rebind<detail::Trace>(xs));
      },
      params.values(),
      policy);
  return {value, params.template rebind<Series>(std::move(grads))};
}

template <class Fn>
std::pair<Series, ParameterPack<Series>> jacrev_trace_(Fn&& fn, const ParameterPack<Series>& params) {
  pluss::subsp::math::diff::DerivativePolicy policy{};
  policy.warn_on_disconnected = false;
  policy.keep_graph = false;
  auto [value, jacobians] = pluss::subsp::math::diff::jacrev(
      [&](const std::vector<detail::Trace>& xs) -> detail::Trace {
        return std::forward<Fn>(fn)(params.template rebind<detail::Trace>(xs));
      },
      params.values(),
      policy);
  return {value, params.template rebind<Series>(std::move(jacobians))};
}

}  // namespace detail

template <class Fn>
std::pair<Series, std::vector<Series>> value_and_sensitivity(Fn&& fn, const std::vector<Series>& params) {
  return pluss::subsp::math::diff::value_and_grad(std::forward<Fn>(fn), params);
}

template <class Fn>
std::pair<Series, ParameterPack<Series>> value_and_grad(Fn&& fn, const ParameterPack<Series>& params) {
  return detail::value_and_grad_trace_(detail::trace_pack_callback_(std::forward<Fn>(fn)), params);
}

inline detail::Trace flatten_values(const ParameterPack<detail::Trace>& params) {
  if (params.empty()) {
    ::pluss::throw_error("optim::flatten_values<detail::Trace>: empty pack");
  }
  std::vector<Series> flat;
  flat.reserve(params.size());
  for (const auto& block : params.blocks()) {
    flat.push_back(block.value.value().reshape({block.value.value().numel()}));
  }
  Series out = flat.size() == 1 ? flat.front() : flat.front().unite(std::vector<Series>(flat.begin() + 1, flat.end()), 0);
  return detail::Trace(out);
}

inline detail::Trace reshape_autodiff_(const detail::Trace& value, const std::vector<std::int64_t>& sizes) {
  return detail::Trace(value.value().reshape(sizes));
}

inline detail::Trace constant_autodiff_(const detail::Trace& like, const Series& value) {
  return pluss::subsp::math::diff::constant_like(like, value.to(like.value().platform()));
}

template <class Fn>
ParameterPack<Series> grad(Fn&& fn, const ParameterPack<Series>& params) {
  return value_and_grad(std::forward<Fn>(fn), params).second;
}

template <class Fn>
std::pair<Series, ParameterPack<Series>> jacrev(Fn&& fn, const ParameterPack<Series>& params) {
  return detail::jacrev_trace_(detail::trace_pack_callback_(std::forward<Fn>(fn)), params);
}

// Forward-mode products are available for callbacks written over plain Series: the Jacobian-vector product and
// the Jacobian come from the reverse-mode Jacobian, and the Hessian-vector product is exact, by reverse over reverse.
template <class Fn>
std::pair<Series, ParameterPack<Series>> jacfwd(Fn&& fn, const ParameterPack<Series>& params) {
  return jacrev(std::forward<Fn>(fn), params);
}

template <class Fn>
std::pair<Series, Series> jvp(Fn&& fn,
                              const ParameterPack<Series>& params,
                              const ParameterPack<Series>& tangents) {
  return pluss::subsp::math::diff::jvp(
      [&, cb = detail::trace_pack_callback_(std::forward<Fn>(fn))](const std::vector<detail::Trace>& xs) -> detail::Trace {
        return cb(params.template rebind<detail::Trace>(xs));
      },
      params.values(),
      tangents.values());
}

namespace detail {

template <class Fn>
std::pair<Series, ParameterPack<Series>> hvp_trace_(Fn&& fn,
                                                    const ParameterPack<Series>& params,
                                                    const ParameterPack<Series>& tangents,
                                                    const pluss::subsp::math::diff::HvpOptions& opt = {}) {
  pluss::subsp::math::diff::DerivativePolicy policy{};
  policy.warn_on_disconnected = false;
  policy.keep_graph = false;
  auto raw = pluss::subsp::math::diff::hvp(
      [&](const std::vector<Trace>& xs) -> Trace { return fn(params.template rebind<Trace>(xs)); },
      params.values(),
      tangents.values(),
      opt,
      policy);
  return {raw.first, params.template rebind<Series>(std::move(raw.second))};
}

}  // namespace detail

template <class Fn>
std::pair<Series, ParameterPack<Series>> hvp(Fn&& fn,
                                             const ParameterPack<Series>& params,
                                             const ParameterPack<Series>& tangents,
                                             const pluss::subsp::math::diff::HvpOptions& opt = {}) {
  return detail::hvp_trace_(detail::trace_pack_callback_(std::forward<Fn>(fn)), params, tangents, opt);
}

template <class Fn>
auto linearize(Fn&& fn, const ParameterPack<Series>& params) {
  auto [value, pushforward] = pluss::subsp::math::diff::linearize(
      [&, cb = detail::trace_pack_callback_(std::forward<Fn>(fn))](const std::vector<detail::Trace>& xs) -> detail::Trace {
        return cb(params.template rebind<detail::Trace>(xs));
      },
      params.values());

  auto pack_pushforward = [pushforward = std::move(pushforward)](const ParameterPack<Series>& tangents) mutable -> Series {
    return pushforward(tangents.values());
  };
  return std::pair{value, std::move(pack_pushforward)};
}

enum class EvalStatus : std::uint8_t {
  Ok = 0,
  NonFiniteObjective,
  NonFiniteGradient,
  NonFiniteConstraint,
  NonFiniteJacobian,
  InvalidProblem,
};

struct Bounds {
  std::optional<ParameterPack<Series>> lower{};
  std::optional<ParameterPack<Series>> upper{};

  bool empty() const noexcept { return !lower.has_value() && !upper.has_value(); }
};

struct LinearConstraints {
  std::optional<Series> Aeq{};
  std::optional<Series> beq{};
  std::optional<Series> Aineq{};
  std::optional<Series> bineq{};

  bool empty() const noexcept {
    return !Aeq.has_value() && !beq.has_value() && !Aineq.has_value() && !bineq.has_value();
  }
};

struct NonlinearConstraintEvaluation {
  Series equality = pluss::series::undefined();
  Series inequality = pluss::series::undefined();
  ParameterPack<Series> equality_jacobian{};
  ParameterPack<Series> inequality_jacobian{};
};

struct ObjectiveDefinition {
  TraceCallback autodiff{};
  std::function<std::pair<Series, ParameterPack<Series>>(const ParameterPack<Series>&)> analytic{};

  bool defined() const noexcept {
    return static_cast<bool>(autodiff) || static_cast<bool>(analytic);
  }
};

struct NonlinearConstraints {
  TraceCallback equality{};
  TraceCallback inequality{};
  std::function<NonlinearConstraintEvaluation(const ParameterPack<Series>&)> analytic{};

  bool empty() const noexcept {
    return !static_cast<bool>(equality) && !static_cast<bool>(inequality) &&
           !static_cast<bool>(analytic);
  }
};

struct ConstrainedProblem {
  ObjectiveDefinition objective{};
  NonlinearConstraints nonlinear{};
  LinearConstraints linear{};
  Bounds bounds{};
};

inline ParameterPack<Series> pack_scalars(std::initializer_list<std::pair<std::string, Series>> blocks) {
  std::vector<typename ParameterPack<Series>::Block> out;
  out.reserve(blocks.size());
  for (const auto& [name, value] : blocks) {
    detail::require_defined_(value, "pack_scalars");
    out.push_back({name, value.reshape({1})});
  }
  return ParameterPack<Series>(std::move(out));
}

inline ParameterPack<Series> pack_vector(std::initializer_list<std::pair<std::string, Series>> blocks) {
  std::vector<typename ParameterPack<Series>::Block> out;
  out.reserve(blocks.size());
  for (const auto& [name, value] : blocks) {
    detail::require_defined_(value, "pack_vector");
    out.push_back({name, value.reshape({value.numel()})});
  }
  return ParameterPack<Series>(std::move(out));
}

inline ParameterPack<Series> pack_series(const Series& values, std::string name = "x") {
  detail::require_defined_(values, "pack_series");
  return pack_vector({{std::move(name), values}});
}

class ProblemBuilder {
public:
  ProblemBuilder() = default;

  template <class Fn>
  ProblemBuilder& objective(Fn&& fn) {
    problem_.objective.autodiff = detail::trace_pack_callback_(std::forward<Fn>(fn));
    return *this;
  }

  ProblemBuilder& objective_analytic(
      std::function<std::pair<Series, ParameterPack<Series>>(const ParameterPack<Series>&)> fn) {
    problem_.objective.analytic = std::move(fn);
    return *this;
  }

  template <class Fn>
  ProblemBuilder& nonlinear_equality(Fn&& fn) {
    problem_.nonlinear.equality = detail::trace_pack_callback_(std::forward<Fn>(fn));
    return *this;
  }

  template <class Fn>
  ProblemBuilder& nonlinear_inequality(Fn&& fn) {
    problem_.nonlinear.inequality = detail::trace_pack_callback_(std::forward<Fn>(fn));
    return *this;
  }

  template <class Fn>
  ProblemBuilder& nl_eq(Fn&& fn) {
    return nonlinear_equality(std::forward<Fn>(fn));
  }

  template <class Fn>
  ProblemBuilder& nl_ineq(Fn&& fn) {
    return nonlinear_inequality(std::forward<Fn>(fn));
  }

  ProblemBuilder& nonlinear_analytic(
      std::function<NonlinearConstraintEvaluation(const ParameterPack<Series>&)> fn) {
    problem_.nonlinear.analytic = std::move(fn);
    return *this;
  }

  ProblemBuilder& lbs(const ParameterPack<Series>& lower) {
    problem_.bounds.lower = lower;
    return *this;
  }

  ProblemBuilder& lbs(const Series& lower) {
    problem_.bounds.lower = pack_series(lower);
    return *this;
  }

  ProblemBuilder& ubs(const ParameterPack<Series>& upper) {
    problem_.bounds.upper = upper;
    return *this;
  }

  ProblemBuilder& ubs(const Series& upper) {
    problem_.bounds.upper = pack_series(upper);
    return *this;
  }

  ProblemBuilder& ln_eq(const Series& Aeq, const Series& beq) {
    problem_.linear.Aeq = Aeq;
    problem_.linear.beq = beq;
    return *this;
  }

  ProblemBuilder& ln_ineq(const Series& Aineq, const Series& bineq) {
    problem_.linear.Aineq = Aineq;
    problem_.linear.bineq = bineq;
    return *this;
  }

  ConstrainedProblem build() const { return problem_; }

  operator ConstrainedProblem() const { return build(); }

private:
  ConstrainedProblem problem_{};
};

inline ProblemBuilder problem() {
  return ProblemBuilder{};
}

struct ObjectiveConstraints {
  NonlinearConstraints nonlinear{};
  LinearConstraints linear{};
  Bounds bounds{};
};

class ObjectiveConstraintsBuilder {
public:
  ObjectiveConstraintsBuilder() = default;

  template <class Fn>
  ObjectiveConstraintsBuilder& nonlinear_equality(Fn&& fn) {
    constraints_.nonlinear.equality = detail::trace_pack_callback_(std::forward<Fn>(fn));
    return *this;
  }

  template <class Fn>
  ObjectiveConstraintsBuilder& nonlinear_inequality(Fn&& fn) {
    constraints_.nonlinear.inequality = detail::trace_pack_callback_(std::forward<Fn>(fn));
    return *this;
  }

  template <class Fn>
  ObjectiveConstraintsBuilder& nl_eq(Fn&& fn) {
    return nonlinear_equality(std::forward<Fn>(fn));
  }

  template <class Fn>
  ObjectiveConstraintsBuilder& nl_ineq(Fn&& fn) {
    return nonlinear_inequality(std::forward<Fn>(fn));
  }

  ObjectiveConstraintsBuilder& nonlinear_analytic(
      std::function<NonlinearConstraintEvaluation(const ParameterPack<Series>&)> fn) {
    constraints_.nonlinear.analytic = std::move(fn);
    return *this;
  }

  ObjectiveConstraintsBuilder& lbs(const ParameterPack<Series>& lower) {
    constraints_.bounds.lower = lower;
    return *this;
  }

  ObjectiveConstraintsBuilder& lbs(const Series& lower) {
    constraints_.bounds.lower = pack_series(lower);
    return *this;
  }

  ObjectiveConstraintsBuilder& ubs(const ParameterPack<Series>& upper) {
    constraints_.bounds.upper = upper;
    return *this;
  }

  ObjectiveConstraintsBuilder& ubs(const Series& upper) {
    constraints_.bounds.upper = pack_series(upper);
    return *this;
  }

  ObjectiveConstraintsBuilder& ln_eq(const Series& Aeq, const Series& beq) {
    constraints_.linear.Aeq = Aeq;
    constraints_.linear.beq = beq;
    return *this;
  }

  ObjectiveConstraintsBuilder& ln_ineq(const Series& Aineq, const Series& bineq) {
    constraints_.linear.Aineq = Aineq;
    constraints_.linear.bineq = bineq;
    return *this;
  }

  ObjectiveConstraints build() const { return constraints_; }
  operator ObjectiveConstraints() const { return build(); }

private:
  ObjectiveConstraints constraints_{};
};

inline ObjectiveConstraintsBuilder constraints() {
  return ObjectiveConstraintsBuilder{};
}

struct ProblemEvaluation {
  EvalStatus status{EvalStatus::Ok};
  std::string message{};
  Series value = pluss::series::undefined();
  ParameterPack<Series> gradient{};
  Series nonlinear_equality_residual = pluss::series::undefined();
  Series nonlinear_inequality_residual = pluss::series::undefined();
  Series equality_residual = pluss::series::undefined();
  Series inequality_residual = pluss::series::undefined();
  ParameterPack<Series> equality_jacobian{};
  ParameterPack<Series> inequality_jacobian{};
  double max_constraint_violation{0.0};
  double first_order_opt{0.0};
  bool feasible{true};
};

struct DerivativeAuditEntry {
  bool available{false};
  bool passed{false};
  double value_abs_diff{0.0};
  double value_rel_diff{0.0};
  double derivative_abs_diff{0.0};
  double derivative_rel_diff{0.0};
  std::string message{};
};

struct DerivativeAuditReport {
  DerivativeAuditEntry objective{};
  DerivativeAuditEntry nonlinear_equalities{};
  DerivativeAuditEntry nonlinear_inequalities{};
  bool passed{true};
};

struct DerivativeAuditOptions {
  double absolute_tolerance{1e-8};
  double relative_tolerance{1e-6};
};

struct OptimaProgressSnapshot;

struct PenaltyMethodOptions {
  std::int64_t max_iterations{100};
  std::int64_t max_line_search_iterations{20};
  double initial_step_size{1e-1};
  double backtracking{0.5};
  double armijo{1e-4};
  double penalty{10.0};
  double constraint_tolerance{1e-6};
  double optimality_tolerance{1e-6};
  bool project_bounds{true};
  std::function<bool(const OptimaProgressSnapshot&)> progress_hook{};
};

struct BestFeasiblePoint {
  ParameterPack<Series> params{};
  Series value = pluss::series::undefined();
  double max_constraint_violation{0.0};
  double first_order_opt{0.0};
};

enum class OptimaProgressStage : std::uint8_t {
  Start = 0,
  Iteration,
  Done,
};

inline std::string_view optima_progress_stage_name(OptimaProgressStage stage) {
  switch (stage) {
    case OptimaProgressStage::Start:
      return "start";
    case OptimaProgressStage::Iteration:
      return "iter";
    case OptimaProgressStage::Done:
      return "done";
  }
  return "iter";
}

enum class OptimaStallCategory : std::uint8_t {
  None = 0,
  UserStop,
  InvalidCallbackContract,
  NonFiniteCallbackRegion,
  SearchDirectionCollapse,
  BoundaryLimited,
  InfeasibleStationary,
  ComplementarityStagnation,
  FeasibilityFailure,
  RecoveryFailure,
  IterationLimit,
};

inline std::string_view optima_stall_category_name(OptimaStallCategory category) {
  switch (category) {
    case OptimaStallCategory::None:
      return "none";
    case OptimaStallCategory::UserStop:
      return "user-stop";
    case OptimaStallCategory::InvalidCallbackContract:
      return "invalid-callback-contract";
    case OptimaStallCategory::NonFiniteCallbackRegion:
      return "nonfinite-callback-region";
    case OptimaStallCategory::SearchDirectionCollapse:
      return "search-direction-collapse";
    case OptimaStallCategory::BoundaryLimited:
      return "boundary-limited";
    case OptimaStallCategory::InfeasibleStationary:
      return "infeasible-stationary";
    case OptimaStallCategory::ComplementarityStagnation:
      return "complementarity-stagnation";
    case OptimaStallCategory::FeasibilityFailure:
      return "feasibility-failure";
    case OptimaStallCategory::RecoveryFailure:
      return "recovery-failure";
    case OptimaStallCategory::IterationLimit:
      return "iteration-limit";
  }
  return "none";
}

struct OptimaProgressSnapshot {
  OptimaProgressStage stage{OptimaProgressStage::Start};
  std::string algorithm{};
  ParameterPack<Series> params{};
  ProblemEvaluation evaluation{};
  std::int64_t iteration{0};
  std::int64_t function_evaluations{0};
  double step_size{0.0};
  double backend_parameter{0.0};
  double stationarity{0.0};
  double complementarity{0.0};
  std::int64_t krylov_iterations{0};
  std::string message{};
  OptimaStallCategory stall_category{OptimaStallCategory::None};
  double stall_measure{0.0};
  double stall_reference{0.0};
  std::optional<BestFeasiblePoint> best_feasible{};
};

struct ConstraintMultipliers {
  Series linear_equalities = pluss::series::undefined();
  Series nonlinear_equalities = pluss::series::undefined();
  Series linear_inequalities = pluss::series::undefined();
  Series nonlinear_inequalities = pluss::series::undefined();
  Series lower_bounds = pluss::series::undefined();
  Series upper_bounds = pluss::series::undefined();
};

struct PenaltyMethodState {
  ParameterPack<Series> params{};
  ProblemEvaluation evaluation{};
  std::int64_t iteration{0};
  std::int64_t function_evaluations{0};
  double last_step_size{0.0};
  double current_penalty{0.0};
  double penalized_value{0.0};
  bool converged{false};
  std::string message{};
  std::optional<BestFeasiblePoint> best_feasible{};
  Series hessian = pluss::series::undefined();
  Series equality_multipliers = pluss::series::undefined();
  Series inequality_multipliers = pluss::series::undefined();
  double inner_tolerance{1e-1};
  double previous_violation{std::numeric_limits<double>::infinity()};
  double stationarity{std::numeric_limits<double>::infinity()};
};

enum class OptimaLinearSystemSolver : std::uint8_t {
  Auto = 0,
  Cholesky,
  PivotedDirect,
  Inverse,
  PseudoInverse,
  ConjugateGradient,
};

enum class OptimaLinearSystemStructure : std::uint8_t {
  Auto = 0,
  Dense,
  Sparse,
};

enum class OptimaHessianApproximation : std::uint8_t {
  Bfgs = 0,
  Identity,
  Lbfgs,
  Exact,
  HessianVector,
};

// What `optima` does about the units the problem is written in.
//   None                     solve the problem as given.
//   InitialPoint             scale the variables by their size at the start.
//   ObjectiveAndConstraints  (default) solve a rescaled copy: the objective and each constraint row are multiplied by a
//                            constant chosen from their gradient at the start (the objective only ever scaled down, a
//                            function whose gradient is already between 1e-2 and 1e2 left alone), so every absolute
//                            tolerance means what it was meant to mean; the answer (value, residuals, multipliers,
//                            progress snapshots) is reported in the caller's units. Bounds are never scaled.
enum class OptimaProblemScaling : std::uint8_t {
  None = 0,
  InitialPoint,
  ObjectiveAndConstraints,
};

inline std::string_view optima_problem_scaling_name(OptimaProblemScaling value) {
  switch (value) {
    case OptimaProblemScaling::None:
      return "None";
    case OptimaProblemScaling::InitialPoint:
      return "InitialPoint";
    case OptimaProblemScaling::ObjectiveAndConstraints:
      return "ObjectiveAndConstraints";
  }
  return "Unknown";
}

struct OptimaNormalizationStats {
  double objective_scale{1.0};
  Series equality_row_scale = pluss::series::undefined();
  Series inequality_row_scale = pluss::series::undefined();
};

struct SequentialQPOptions {
  std::int64_t max_iterations{100};
  std::int64_t max_line_search_iterations{20};
  std::int64_t max_second_order_corrections{2};
  std::int64_t max_restoration_iterations{8};
  std::int64_t max_restoration_line_search_iterations{20};
  double initial_step_size{1.0};
  double backtracking{0.5};
  double armijo{1e-4};
  double initial_penalty{10.0};
  double constraint_tolerance{1e-6};
  double optimality_tolerance{1e-6};
  double step_tolerance{1e-6};
  double value_progress_tolerance{1e-6};
  double active_set_tolerance{1e-4};
  // The quadratic subproblem is solved by a primal-dual interior method whose Newton system is reduced to an
  // n x n positive-definite matrix. `linear_system_solver` / `system_structure` pick how that matrix is solved
  // (Cholesky by default, conjugate gradients when requested or when n reaches `iterative_dimension_threshold`),
  // `regularization` is the first diagonal shift tried when a factorization fails (grown tenfold per retry),
  // `pivot_threshold` is the smallest pivot a factorization accepts, and the Krylov settings control the
  // conjugate-gradient solves (the effective relative tolerance is capped at 1e-8 because the interior iteration
  // needs accurate steps; if the reduced system stalls, the subproblem is re-solved on the unreduced KKT system by a pivoted LU).
  double regularization{1e-6};
  double bfgs_curvature_tolerance{1e-8};
  double second_order_correction_tolerance{1e-4};
  double fraction_to_boundary{0.995};
  double pivot_threshold{1e-12};
  std::int64_t max_krylov_iterations{0};
  std::int64_t iterative_dimension_threshold{3000};
  double krylov_tolerance{1e-2};
  double krylov_absolute_tolerance{1e-10};
  double objective_limit{-std::numeric_limits<double>::infinity()};
  bool project_bounds{true};
  bool enable_restoration{true};
  OptimaLinearSystemStructure system_structure{OptimaLinearSystemStructure::Auto};
  OptimaLinearSystemSolver linear_system_solver{OptimaLinearSystemSolver::Auto};
  // Curvature of the quadratic model:
  //   Bfgs          damped (Powell) dense BFGS update of the Lagrangian Hessian,
  //   Lbfgs         the same update restricted to the last `lbfgs_memory` pairs, rebuilt each iteration,
  //   Exact,
  //   HessianVector the exact Hessian of the Lagrangian from forward-over-reverse products (needs the problem's
  //                 the autodiff callbacks; made positive definite by a diagonal shift where it is not),
  //   Identity      no curvature.
  OptimaHessianApproximation hessian_approximation{OptimaHessianApproximation::Bfgs};
  std::int64_t lbfgs_memory{10};
  pluss::subsp::math::diff::HvpOptions hvp_options{};
  OptimaProblemScaling problem_scaling{OptimaProblemScaling::ObjectiveAndConstraints};
  std::function<bool(const OptimaProgressSnapshot&)> progress_hook{};
};

struct SequentialQPState {
  ParameterPack<Series> params{};
  ProblemEvaluation evaluation{};
  std::int64_t iteration{0};
  std::int64_t function_evaluations{0};
  double last_step_size{0.0};
  double current_penalty{0.0};
  double merit_value{0.0};
  double kkt_residual{0.0};
  bool converged{false};
  std::string message{};
  std::optional<BestFeasiblePoint> best_feasible{};
  Series hessian = pluss::series::undefined();
  Series multipliers = pluss::series::undefined();
  Series variable_scale = pluss::series::undefined();
  OptimaNormalizationStats normalization{};
  std::int64_t krylov_iterations{0};
  double trust_radius{0.0};
  // Size of the objective gradient at the start (at least 1): stationarity is judged relative to it.
  double gradient_scale{1.0};
  // Curvature bookkeeping. The limited-memory model keeps its recent step pairs; the exact model needs the
  // multipliers of the last quadratic subproblem (one per equality row and per inequality row of the system, in
  // assembly order); `curvature_reset` makes the next step use the identity once after a failed line search.
  std::vector<Series> lbfgs_s_history{};
  std::vector<Series> lbfgs_y_history{};
  std::vector<double> qp_equality_multipliers{};
  std::vector<double> qp_inequality_multipliers{};
  bool curvature_reset{false};
};

struct WorkingSetConstraintKey {
  bool equality{false};
  std::uint8_t source_id{0};
  std::int64_t index{-1};
};

struct WorkingSetOptions : SequentialQPOptions {
  bool favor_working_set_updates{true};
  std::int64_t max_working_set_violations{8};
  double add_constraint_tolerance{1e-3};
  double drop_multiplier_tolerance{1e-8};
  double feasibility_priority_ratio{0.8};
  std::int64_t max_stalled_iterations{8};
  double multiplier_smoothing{0.5};
  double multiplier_retention_tolerance{1e-6};
};

struct WorkingSetState : SequentialQPState {
  std::vector<WorkingSetConstraintKey> active_set{};
  std::int64_t stalled_iterations{0};
};

struct InternalProtocolOptions {
  std::int64_t max_iterations{100};
  std::int64_t max_line_search_iterations{25};
  std::int64_t max_restoration_iterations{8};
  std::int64_t max_restoration_line_search_iterations{25};
  std::int64_t max_stalled_iterations{8};
  std::int64_t min_iter_to_feasibility_mode{3};
  std::int64_t min_switch_violations{2};
  double initial_step_size{1.0};
  double backtracking{0.5};
  double nonfinite_backtracking{0.25};
  double armijo{1e-4};
  double initial_barrier_parameter{1e-2};
  double minimum_barrier_parameter{1e-8};
  double barrier_decay{0.2};
  double equality_penalty{10.0};
  double feasibility_activation_factor{10.0};
  double insufficient_infeas_decrease{0.01};
  double sufficient_infeas_decrease{0.9};
  double constraint_tolerance{1e-6};
  double optimality_tolerance{1e-6};
  double step_tolerance{1e-10};
  double value_progress_tolerance{1e-10};
  double fraction_to_boundary{0.99};
  double pivot_threshold{1e-12};
  double initialization_margin{1e-4};
  double bfgs_curvature_tolerance{1e-8};
  double hessian_regularization{1e-8};
  double hessian_regularization_max{1e-2};
  double trust_region_initial_radius{1.0};
  double trust_region_min_radius{1e-8};
  double trust_region_max_radius{10.0};
  double trust_region_shrink{0.5};
  double trust_region_expand{2.0};
  double trust_region_accept_ratio{1e-4};
  double trust_region_expand_ratio{0.75};
  double trust_region_shrink_ratio{0.1};
  std::int64_t nonmonotone_window{5};
  bool enable_watchdog{true};
  double watchdog_relaxation{1e-12};
  double flat_objective_tolerance{1e-8};
  std::int64_t max_flat_objective_iterations{3};
  bool enable_soc_correction{true};
  std::int64_t soc_correction_attempts{1};
  std::int64_t lbfgs_memory{10};
  std::int64_t max_krylov_iterations{0};
  std::int64_t iterative_dimension_threshold{3000};
  double krylov_tolerance{1e-2};
  double krylov_absolute_tolerance{1e-10};
  pluss::subsp::math::diff::HvpOptions hvp_options{};
  double objective_limit{-std::numeric_limits<double>::infinity()};
  bool project_bounds{true};
  bool enable_restoration{true};
  bool enable_feasibility_phase{false};
  bool enable_nonfinite_recovery{true};
  // true: each nonlinear inequality g(x) <= 0 gets its own slack variable s > 0 with g(x) + s = 0 enforced only in
  // the limit, so iterates may violate nonlinear constraints on the way (bounds and linear rows stay strictly
  // satisfied) and a start on a curved boundary is not choked. false: s = -g(x) at every iterate, so every iterate
  // satisfies every inequality strictly; use this when the objective is undefined outside the feasible region.
  bool independent_nonlinear_slacks{true};
  OptimaLinearSystemStructure system_structure{OptimaLinearSystemStructure::Auto};
  OptimaLinearSystemSolver linear_system_solver{OptimaLinearSystemSolver::Auto};
  OptimaHessianApproximation hessian_approximation{OptimaHessianApproximation::Bfgs};
  OptimaProblemScaling problem_scaling{OptimaProblemScaling::ObjectiveAndConstraints};
  std::function<bool(const OptimaProgressSnapshot&)> progress_hook{};
};

struct InternalProtocolState {
  // Consecutive iterations that only refreshed the multipliers (zero primal step) without changing anything else.
  std::int64_t zero_step_updates{0};
  ParameterPack<Series> params{};
  ProblemEvaluation evaluation{};
  std::int64_t iteration{0};
  std::int64_t function_evaluations{0};
  double last_step_size{0.0};
  double barrier_parameter{0.0};
  double barrier_value{0.0};
  double barrier_gradient_norm{0.0};
  // Size of the objective gradient at the start (at least 1): stationarity is judged relative to it, as the
  // optimality tolerance of a gradient-scaled solver is, so a likelihood summed over thousands of terms is
  // not held to an absolute 1e-6.
  double gradient_scale{1.0};
  // Reference size (1 / inf-norm of the starting Jacobian row) of every equality row followed by every inequality
  // row, so the "violation is stationary" test is independent of the units a constraint is written in.
  std::vector<double> stationarity_row_scale{};
  bool converged{false};
  std::string message{};
  std::optional<BestFeasiblePoint> best_feasible{};
  Series hessian = pluss::series::undefined();
  Series variable_scale = pluss::series::undefined();
  OptimaNormalizationStats normalization{};
  std::int64_t krylov_iterations{0};
  std::vector<Series> lbfgs_s_history{};
  std::vector<Series> lbfgs_y_history{};
  Series inequality_slack = pluss::series::undefined();
  // Independent slack variables of the nonlinear inequality rows (g(x) + s = 0 is satisfied only in the limit), so
  // an iterate may start on the boundary of a curved constraint without being choked. Undefined means s = -g(x).
  Series nl_slack = pluss::series::undefined();
  Series inequality_dual = pluss::series::undefined();
  double primal_residual{0.0};
  double complementarity_residual{0.0};
  bool in_feasibility_phase{false};
  double feasibility_metric{0.0};
  std::int64_t stalled_iterations{0};
  std::int64_t feasibility_switch_violations{0};
  std::int64_t nonfinite_evaluations{0};
  std::int64_t flat_objective_iterations{0};
  double trust_region_radius{1.0};
  std::vector<double> barrier_history{};
  Series equality_dual = pluss::series::undefined();
  double merit_penalty{10.0};
};

enum class OptimaAlgorithm : std::uint8_t {
  MeritMethod = 0,
  SequentialQP,
  WorkingSet,
  InternalProtocol,
};

inline std::string_view optima_algorithm_name(OptimaAlgorithm value) {
  switch (value) {
    case OptimaAlgorithm::MeritMethod:
      return "MeritMethod";
    case OptimaAlgorithm::SequentialQP:
      return "SequentialQP";
    case OptimaAlgorithm::WorkingSet:
      return "WorkingSet";
    case OptimaAlgorithm::InternalProtocol:
      return "InternalProtocol";
  }
  return "Unknown";
}

enum class OptimaExitFlag : std::int32_t {
  Converged = 1,
  MaxIterations = 0,
  Stopped = -1,
  NoFeasiblePoint = -2,
  ObjectiveTargetReached = -3,
  NonFiniteEvaluation = -4,
  InvalidProblem = -5,
  NoProgress = -6,
  ConstraintStationary = -7,
};

inline std::string_view optima_exit_flag_name(OptimaExitFlag flag) {
  switch (flag) {
    case OptimaExitFlag::Converged:
      return "Converged";
    case OptimaExitFlag::MaxIterations:
      return "MaxIterations";
    case OptimaExitFlag::Stopped:
      return "Stopped";
    case OptimaExitFlag::NoFeasiblePoint:
      return "NoFeasiblePoint";
    case OptimaExitFlag::ObjectiveTargetReached:
      return "ObjectiveTargetReached";
    case OptimaExitFlag::NonFiniteEvaluation:
      return "NonFiniteEvaluation";
    case OptimaExitFlag::InvalidProblem:
      return "InvalidProblem";
    case OptimaExitFlag::NoProgress:
      return "NoProgress";
    case OptimaExitFlag::ConstraintStationary:
      return "ConstraintStationary";
  }
  return "Unknown";
}

struct OptimaOutput {
  std::string algorithm{};
  std::int64_t iterations{0};
  std::int64_t function_evaluations{0};
  double constraint_violation{0.0};
  double first_order_optimality{0.0};
  double step_size{0.0};
  double line_search_fraction{0.0};
  double backend_parameter{0.0};
  double complementarity{0.0};
  std::int64_t krylov_iterations{0};
  bool best_feasible_available{false};
  bool returned_best_feasible{false};
  double best_feasible_value{std::numeric_limits<double>::infinity()};
  double best_feasible_constraint_violation{std::numeric_limits<double>::infinity()};
  double best_feasible_first_order_optimality{std::numeric_limits<double>::infinity()};
  double multiplier_max_norm{0.0};
  std::int64_t active_multiplier_count{0};
  std::string detail_message{};
  OptimaStallCategory stall_category{OptimaStallCategory::None};
  double stall_measure{0.0};
  double stall_reference{0.0};
};

struct OptimaOptions {
  OptimaAlgorithm algorithm{OptimaAlgorithm::SequentialQP};
  PenaltyMethodOptions penalty{};
  SequentialQPOptions sequential_qp{};
  WorkingSetOptions working_set{};
  InternalProtocolOptions internal_protocol{};
  std::function<bool(const OptimaProgressSnapshot&)> progress_hook{};
};

struct OptimaState {
  OptimaAlgorithm algorithm{OptimaAlgorithm::SequentialQP};
  ParameterPack<Series> params{};
  ProblemEvaluation evaluation{};
  std::int64_t iteration{0};
  std::int64_t function_evaluations{0};
  double last_step_size{0.0};
  double backend_parameter{0.0};
  double stationarity{0.0};
  bool converged{false};
  OptimaExitFlag exit_flag{OptimaExitFlag::Stopped};
  std::string message{};
  OptimaOutput output{};
  ConstraintMultipliers multipliers{};
  std::optional<BestFeasiblePoint> best_feasible{};
};

namespace presets {

inline OptimaOptions sequential_qp() {
  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::SequentialQP;
  options.sequential_qp.max_iterations = 100;
  options.sequential_qp.max_line_search_iterations = 20;
  options.sequential_qp.constraint_tolerance = 1e-8;
  options.sequential_qp.optimality_tolerance = 1e-6;
  options.sequential_qp.problem_scaling = OptimaProblemScaling::InitialPoint;
  return options;
}

inline OptimaOptions internal_protocol() {
  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::InternalProtocol;
  options.internal_protocol.max_iterations = 100;
  options.internal_protocol.initial_barrier_parameter = 1e-2;
  options.internal_protocol.minimum_barrier_parameter = 1e-8;
  options.internal_protocol.barrier_decay = 0.2;
  options.internal_protocol.constraint_tolerance = 1e-8;
  options.internal_protocol.optimality_tolerance = 1e-6;
  options.internal_protocol.max_line_search_iterations = 12;
  options.internal_protocol.max_restoration_iterations = 4;
  options.internal_protocol.max_restoration_line_search_iterations = 10;
  options.internal_protocol.max_stalled_iterations = 4;
  // The separate feasibility phase is off: the primal-dual step already drives equality violation down, and the
  // phase made the broad parity suite slower and less reliable (it stays available as an option).
  options.internal_protocol.enable_feasibility_phase = false;
  options.internal_protocol.enable_nonfinite_recovery = true;
  options.internal_protocol.enable_watchdog = true;
  options.internal_protocol.nonmonotone_window = 5;
  options.internal_protocol.enable_soc_correction = true;
  options.internal_protocol.soc_correction_attempts = 1;
  options.internal_protocol.trust_region_initial_radius = 1.0;
  options.internal_protocol.trust_region_min_radius = 1e-8;
  options.internal_protocol.trust_region_max_radius = 10.0;
  options.internal_protocol.problem_scaling = OptimaProblemScaling::InitialPoint;
  return options;
}

inline OptimaOptions robust_internal_protocol() {
  OptimaOptions options = internal_protocol();
  options.internal_protocol.max_iterations = 160;
  options.internal_protocol.max_line_search_iterations = 16;
  options.internal_protocol.max_restoration_iterations = 6;
  options.internal_protocol.max_restoration_line_search_iterations = 12;
  options.internal_protocol.max_stalled_iterations = 5;
  options.internal_protocol.enable_restoration = true;
  options.internal_protocol.enable_feasibility_phase = false;
  options.internal_protocol.enable_nonfinite_recovery = true;
  options.internal_protocol.enable_watchdog = true;
  options.internal_protocol.nonmonotone_window = 7;
  options.internal_protocol.enable_soc_correction = true;
  options.internal_protocol.soc_correction_attempts = 2;
  options.internal_protocol.trust_region_initial_radius = 1.0;
  options.internal_protocol.trust_region_min_radius = 1e-8;
  options.internal_protocol.trust_region_max_radius = 25.0;
  options.internal_protocol.trust_region_expand = 2.0;
  options.internal_protocol.trust_region_shrink = 0.5;
  options.internal_protocol.hessian_approximation = OptimaHessianApproximation::Bfgs;
  options.internal_protocol.linear_system_solver = OptimaLinearSystemSolver::Auto;
  options.internal_protocol.system_structure = OptimaLinearSystemStructure::Auto;
  return options;
}

} // namespace presets

inline std::ostream& print_summary(std::ostream& os, const OptimaState& result) {
  os << "algorithm=" << result.output.algorithm
     << " exit=" << static_cast<int>(result.exit_flag)
     << " iterations=" << result.iteration
     << " evaluations=" << result.function_evaluations
     << " objective=";
  if (result.evaluation.value.defined() && result.evaluation.value.numel() == 1) {
    os << result.evaluation.value.item<double>();
  } else {
    os << "nan";
  }
  os << " violation=" << result.evaluation.max_constraint_violation
     << " first_order=" << result.output.first_order_optimality
     << " backend_parameter=" << result.backend_parameter
     << " complementarity=" << result.output.complementarity
     << " stall=" << optima_stall_category_name(result.output.stall_category)
     << " message=" << result.message;
  return os;
}

struct SeptimaProgressSnapshot;

namespace reports {

struct ProgressTracePreset {
  bool include_start{true};
  bool include_done{true};
  bool include_message{true};
  bool include_krylov{true};
  int precision{6};
};

inline std::function<bool(const OptimaProgressSnapshot&)> trace(std::ostream& os,
                                                                ProgressTracePreset preset = {}) {
  return [&os, preset](const OptimaProgressSnapshot& snapshot) {
    if (snapshot.stage == OptimaProgressStage::Start && !preset.include_start) return false;
    if (snapshot.stage == OptimaProgressStage::Done && !preset.include_done) return false;

    const auto old_flags = os.flags();
    const auto old_precision = os.precision();
    os.setf(std::ios::fixed, std::ios::floatfield);
    os.precision(preset.precision);

    double objective = std::numeric_limits<double>::quiet_NaN();
    if (snapshot.evaluation.value.defined() && snapshot.evaluation.value.numel() == 1) {
      objective = snapshot.evaluation.value.item<double>();
    }

    os << "iter=" << snapshot.iteration
       << " evals=" << snapshot.function_evaluations
       << " objective=" << objective
       << " viol=" << snapshot.evaluation.max_constraint_violation
       << " stationarity=" << snapshot.stationarity
       << " backend=" << snapshot.backend_parameter
       << " step=" << snapshot.step_size;
    if (preset.include_krylov) {
      os << " krylov=" << snapshot.krylov_iterations;
    }
    os << " stall=" << optima_stall_category_name(snapshot.stall_category);
    if (preset.include_message && !snapshot.message.empty()) {
      os << " msg=\"" << snapshot.message << '"';
    }
    os << '\n';

    os.flags(old_flags);
    os.precision(old_precision);
    return false;
  };
}

inline std::function<bool(const OptimaProgressSnapshot&)> iteration_only_trace(std::ostream& os,
                                                                               int precision = 6) {
  ProgressTracePreset preset{};
  preset.include_start = false;
  preset.include_done = false;
  preset.include_message = false;
  preset.include_krylov = false;
  preset.precision = precision;
  return trace(os, preset);
}

// One iteration-per-line report that works as the `progress_hook` of both `optima` and `septima`: it can be called
// with either solver's snapshot. `every` prints only every n-th iteration (the first is always printed).
class IterReport {
 public:
  IterReport(std::ostream& os, int precision, std::int64_t every)
      : os_(&os), precision_(precision), every_(every), optima_(iteration_only_trace(os, precision)) {}

  bool operator()(const OptimaProgressSnapshot& snapshot) const {
    if (!selected_(snapshot.iteration)) return false;
    return optima_(snapshot);
  }
  bool operator()(const SeptimaProgressSnapshot& snapshot) const;

 private:
  bool selected_(std::int64_t iteration) const { return every_ <= 1 || iteration == 1 || iteration % every_ == 0; }

  std::ostream* os_;
  int precision_;
  std::int64_t every_;
  std::function<bool(const OptimaProgressSnapshot&)> optima_;
};

inline IterReport iter(std::ostream& os, int precision = 6, std::int64_t every = 1) {
  return IterReport(os, precision, every);
}

inline std::ostream& compact_summary(std::ostream& os, const OptimaState& result) {
  return print_summary(os, result);
}

inline std::ostream& detailed_summary(std::ostream& os, const OptimaState& result) {
  os << "algorithm=" << result.output.algorithm << '\n';
  os << "exit=" << static_cast<int>(result.exit_flag) << '\n';
  os << "iterations=" << result.output.iterations << '\n';
  os << "evaluations=" << result.output.function_evaluations << '\n';
  os << "constraint_violation=" << result.output.constraint_violation << '\n';
  os << "first_order_optimality=" << result.output.first_order_optimality << '\n';
  os << "step_size=" << result.output.step_size << '\n';
  os << "backend_parameter=" << result.output.backend_parameter << '\n';
  os << "complementarity=" << result.output.complementarity << '\n';
  os << "krylov_iterations=" << result.output.krylov_iterations << '\n';
  os << "best_feasible_available=" << (result.output.best_feasible_available ? "true" : "false") << '\n';
  os << "returned_best_feasible=" << (result.output.returned_best_feasible ? "true" : "false") << '\n';
  os << "stall=" << optima_stall_category_name(result.output.stall_category) << '\n';
  os << "stall_measure=" << result.output.stall_measure << '\n';
  os << "stall_reference=" << result.output.stall_reference << '\n';
  os << "message=" << result.message;
  return os;
}

} // namespace reports

inline ProblemEvaluation evaluate_problem(const ConstrainedProblem& problem,
                                          const ParameterPack<Series>& params);

namespace detail {

enum class ConstraintSource : std::uint8_t {
  LinearEquality = 0,
  NonlinearEquality,
  LinearInequality,
  NonlinearInequality,
  LowerBound,
  UpperBound,
};

struct FlatConstraintRow {
  Series jacobian = pluss::series::undefined();
  double residual{0.0};
  bool equality{false};
  ConstraintSource source{ConstraintSource::LinearEquality};
  std::int64_t index{0};
  std::vector<double> dense{};  // the Jacobian row on the host when the assembly already had it (may be empty)
};

struct ConstraintScaleKey {
  bool equality{false};
  ConstraintSource source{ConstraintSource::LinearEquality};
  std::int64_t index{0};

  bool operator==(const ConstraintScaleKey& other) const noexcept {
    return equality == other.equality && source == other.source && index == other.index;
  }
};

struct ConstraintScaleKeyHash {
  std::size_t operator()(const ConstraintScaleKey& key) const noexcept {
    std::size_t h = std::hash<std::int64_t>{}(key.index);
    h ^= (static_cast<std::size_t>(key.equality) + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2));
    h ^= (static_cast<std::size_t>(static_cast<std::uint8_t>(key.source)) +
          0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2));
    return h;
  }
};

struct FlatConstraintSystem {
  std::vector<FlatConstraintRow> equalities{};
  std::vector<FlatConstraintRow> inequalities{};
};

inline Series empty_vec_like_(const ParameterPack<Series>& params,
                              ScalarType dtype = ScalarType::Float64) {
  if (params.empty()) return pluss::series::empty({0}, dtype);
  return pluss::series::empty({0}, dtype).to(params[0].value.platform());
}

inline Series as_flat_vec_(const Series& x) {
  require_defined_(x, "as_flat_vec");
  return x.reshape({x.numel()});
}

inline Series concat_flat_(const std::vector<Series>& pieces,
                           const ParameterPack<Series>& params,
                           ScalarType dtype = ScalarType::Float64) {
  std::vector<Series> flat;
  flat.reserve(pieces.size());
  pluss::codem::Platform platform =
      params.empty() ? pluss::codem::Platform::CPU : params[0].value.platform();
  for (const auto& piece : pieces) {
    if (!piece.defined() || piece.numel() == 0) continue;
    Series v = piece;
    if (v.scalar_type() != dtype) v = v.to(dtype);
    if (v.platform() != platform) v = v.to(platform);
    flat.push_back(v.reshape({v.numel()}));
  }
  if (flat.empty()) return pluss::series::empty({0}, dtype).to(platform);
  if (flat.size() == 1) return flat.front();
  return flat.front().unite(std::vector<Series>(flat.begin() + 1, flat.end()), 0);
}

inline Series dense_matvec_(const Series& matrix, const Series& vector) {
  require_defined_(matrix, "dense_matvec");
  require_defined_(vector, "dense_matvec");
  if (matrix.sizes().size() != 2) {
    ::pluss::throw_error("optim::dense_matvec_: expected a rank-2 matrix");
  }

  const std::int64_t rows = matrix.sizes()[0];
  const std::int64_t cols = matrix.sizes()[1];
  const Series flat_vector = vector.reshape({vector.numel()}).to(ScalarType::Float64);
  if (flat_vector.numel() != cols) {
    ::pluss::throw_error("optim::dense_matvec_: matrix/vector shape mismatch");
  }
  return matrix.to(ScalarType::Float64)
      .to(vector.platform())
      .reshape({rows, cols})
      .matmul(flat_vector.to(vector.platform()).reshape({cols, 1}))
      .reshape({rows});
}

inline Series dense_transpose_matvec_(const Series& matrix, const Series& vector) {
  require_defined_(matrix, "dense_transpose_matvec");
  require_defined_(vector, "dense_transpose_matvec");
  if (matrix.sizes().size() != 2) {
    ::pluss::throw_error("optim::dense_transpose_matvec_: expected a rank-2 matrix");
  }

  const std::int64_t rows = matrix.sizes()[0];
  const std::int64_t cols = matrix.sizes()[1];
  const Series flat_vector = vector.reshape({vector.numel()}).to(ScalarType::Float64);
  if (flat_vector.numel() != rows) {
    ::pluss::throw_error(
        "optim::dense_transpose_matvec_: matrix/vector shape mismatch");
  }
  // x' A, as a row vector times the matrix: no transposed copy of A is materialized (a transposed view would be
  // copied to contiguous storage on every product).
  return flat_vector.to(vector.platform())
      .reshape({1, rows})
      .matmul(matrix.to(ScalarType::Float64).to(vector.platform()).reshape({rows, cols}))
      .reshape({cols});
}

inline bool is_finite_series_(const Series& x) {
  require_defined_(x, "is_finite_series");
  return x.isfinite().all().item<bool>();
}

inline double max_abs_scalar_(const Series& x) {
  if (!x.defined() || x.numel() == 0) return 0.0;
  return x.to(ScalarType::Float64).abs().reshape({x.numel()}).max().item<double>();
}

inline double max_positive_scalar_(const Series& x) {
  if (!x.defined() || x.numel() == 0) return 0.0;
  return std::max(0.0, x.to(ScalarType::Float64).reshape({x.numel()}).max().item<double>());
}

inline std::int64_t count_active_entries_(const Series& x, double tolerance = 1e-10) {
  if (!x.defined() || x.numel() == 0) return 0;
  return x.to(ScalarType::Float64)
      .abs()
      .gt(tolerance)
      .to(ScalarType::Int64)
      .sum()
      .item<std::int64_t>();
}

inline ParameterPack<Series> zero_jacobian_pack_(const ParameterPack<Series>& params,
                                                 std::int64_t rows,
                                                 ScalarType dtype = ScalarType::Float64) {
  std::vector<typename ParameterPack<Series>::Block> out;
  out.reserve(params.size());
  for (const auto& block : params.blocks()) {
    require_defined_(block.value, "zero_jacobian_pack");
    out.push_back({block.name, pluss::series::zeros({rows, block.value.numel()}, dtype).to(block.value.platform())});
  }
  return ParameterPack<Series>(std::move(out));
}

inline std::optional<std::string> validate_scalar_output_(const Series& value,
                                                          std::string_view label) {
  if (!value.defined()) {
    return std::string(label) + " must be defined";
  }
  if (value.numel() != 1) {
    return std::string(label) + " must be scalar-valued";
  }
  if (!value.dtype().is_floating() && !value.dtype().is_complex()) {
    return std::string(label) + " must have a floating dtype";
  }
  return std::nullopt;
}

inline std::optional<std::string> validate_residual_output_(const Series& value,
                                                            std::string_view label) {
  if (!value.defined()) {
    return std::string(label) + " must be defined";
  }
  if (!value.dtype().is_floating() && !value.dtype().is_complex()) {
    return std::string(label) + " must have a floating dtype";
  }
  return std::nullopt;
}

inline std::optional<std::string> validate_pack_like_params_(const ParameterPack<Series>& params,
                                                             const ParameterPack<Series>& pack,
                                                             std::string_view label) {
  if (pack.size() != params.size()) {
    return std::string(label) + " must contain one block per parameter block";
  }
  for (const auto& block : params.blocks()) {
    if (!pack.contains(block.name)) {
      return std::string(label) + " is missing block '" + block.name + "'";
    }
    const Series& value = pack.at(block.name);
    if (!value.defined()) {
      return std::string(label) + " block '" + block.name + "' must be defined";
    }
    if (value.platform() != block.value.platform()) {
      return std::string(label) + " block '" + block.name + "' must be on the same platform as the parameter";
    }
    if (!same_shape_(value, block.value)) {
      return std::string(label) + " block '" + block.name + "' shape mismatch";
    }
    if (!value.dtype().is_floating() && !value.dtype().is_complex()) {
      return std::string(label) + " block '" + block.name + "' must have a floating dtype";
    }
  }
  return std::nullopt;
}

inline std::optional<std::string> validate_jacobian_pack_(const ParameterPack<Series>& params,
                                                          const ParameterPack<Series>& jacobian,
                                                          const Series& residual,
                                                          std::string_view label) {
  if (jacobian.size() != params.size()) {
    return std::string(label) + " must contain one block per parameter block";
  }
  const std::int64_t rows = residual.numel();
  for (const auto& block : params.blocks()) {
    if (!jacobian.contains(block.name)) {
      return std::string(label) + " is missing block '" + block.name + "'";
    }
    const Series& value = jacobian.at(block.name);
    if (!value.defined()) {
      return std::string(label) + " block '" + block.name + "' must be defined";
    }
    if (value.platform() != block.value.platform()) {
      return std::string(label) + " block '" + block.name + "' must be on the same platform as the parameter";
    }
    if (value.sizes().size() != 2 || value.sizes()[0] != rows || value.sizes()[1] != block.value.numel()) {
      return std::string(label) + " block '" + block.name + "' must have shape [" +
             std::to_string(rows) + ", " + std::to_string(block.value.numel()) + "]";
    }
    if (!value.dtype().is_floating() && !value.dtype().is_complex()) {
      return std::string(label) + " block '" + block.name + "' must have a floating dtype";
    }
  }
  return std::nullopt;
}

inline std::string describe_layout_(const ParameterPack<Series>& pack) {
  std::string out;
  for (const auto& block : pack.blocks()) {
    if (!out.empty()) out += ", ";
    out += block.name + "[";
    const auto sizes = block.value.sizes();
    for (std::size_t i = 0; i < sizes.size(); ++i) out += (i ? "x" : "") + std::to_string(sizes[i]);
    out += "]";
  }
  return out;
}

inline void validate_problem_(const ConstrainedProblem& problem, const ParameterPack<Series>& params) {
  if (!problem.objective.defined()) {
    ::pluss::throw_error("optim::evaluate_problem: objective is not defined");
  }
  if (problem.bounds.lower.has_value() && !problem.bounds.lower->same_layout(params)) {
    ::pluss::throw_error("optim::evaluate_problem: lower bounds must have the same named blocks and shapes as the "
                         "parameters (parameters: " + describe_layout_(params) + "; lower bounds: " +
                         describe_layout_(*problem.bounds.lower) + ")");
  }
  if (problem.bounds.upper.has_value() && !problem.bounds.upper->same_layout(params)) {
    ::pluss::throw_error("optim::evaluate_problem: upper bounds must have the same named blocks and shapes as the "
                         "parameters (parameters: " + describe_layout_(params) + "; upper bounds: " +
                         describe_layout_(*problem.bounds.upper) + ")");
  }
  if (problem.bounds.lower.has_value() && problem.bounds.upper.has_value()) {
    for (const auto& block : params.blocks()) {
      const Series diff = problem.bounds.lower->at(block.name).sub(problem.bounds.upper->at(block.name));
      if (max_positive_scalar_(diff) > 0.0) {
        ::pluss::throw_error("optim::evaluate_problem: lower bound exceeds upper bound");
      }
    }
  }
  const std::int64_t n = params.total_numel();
  if (problem.linear.Aeq.has_value()) {
    require_defined_(*problem.linear.Aeq, "evaluate_problem");
    if (problem.linear.Aeq->sizes().size() != 2 || problem.linear.Aeq->sizes()[1] != n) {
      ::pluss::throw_error("optim::evaluate_problem: Aeq shape mismatch");
    }
    if (problem.linear.beq.has_value() && problem.linear.beq->numel() != problem.linear.Aeq->sizes()[0]) {
      ::pluss::throw_error("optim::evaluate_problem: beq length mismatch");
    }
  } else if (problem.linear.beq.has_value() && problem.linear.beq->numel() != 0) {
    ::pluss::throw_error("optim::evaluate_problem: beq provided without Aeq");
  }
  if (problem.linear.Aineq.has_value()) {
    require_defined_(*problem.linear.Aineq, "evaluate_problem");
    if (problem.linear.Aineq->sizes().size() != 2 || problem.linear.Aineq->sizes()[1] != n) {
      ::pluss::throw_error("optim::evaluate_problem: Aineq shape mismatch");
    }
    if (problem.linear.bineq.has_value() && problem.linear.bineq->numel() != problem.linear.Aineq->sizes()[0]) {
      ::pluss::throw_error("optim::evaluate_problem: bineq length mismatch");
    }
  } else if (problem.linear.bineq.has_value() && problem.linear.bineq->numel() != 0) {
    ::pluss::throw_error("optim::evaluate_problem: bineq provided without Aineq");
  }
}

inline Series evaluate_linear_eq_(const LinearConstraints& linear, const Series& flat_params) {
  if (!linear.Aeq.has_value()) return pluss::series::empty({0}, flat_params.scalar_type()).to(flat_params.platform());
  Series residual = dense_matvec_(*linear.Aeq, flat_params).to(flat_params.scalar_type());
  if (linear.beq.has_value()) residual = residual.sub(linear.beq->reshape({linear.beq->numel()}));
  return residual;
}

inline Series evaluate_linear_ineq_(const LinearConstraints& linear, const Series& flat_params) {
  if (!linear.Aineq.has_value()) return pluss::series::empty({0}, flat_params.scalar_type()).to(flat_params.platform());
  Series residual = dense_matvec_(*linear.Aineq, flat_params).to(flat_params.scalar_type());
  if (linear.bineq.has_value()) residual = residual.sub(linear.bineq->reshape({linear.bineq->numel()}));
  return residual;
}

inline Series evaluate_lower_bound_residual_(const Bounds& bounds, const ParameterPack<Series>& params) {
  if (!bounds.lower.has_value()) return empty_vec_like_(params);
  std::vector<Series> residuals;
  residuals.reserve(params.size());
  for (const auto& block : params.blocks()) {
    const Series& lower = bounds.lower->at(block.name);
    // An infinite bound constrains nothing; report it as slack so it never counts as a violation.
    residuals.push_back(lower.clamp(-1e20, 1e20).sub(block.value));
  }
  return concat_flat_(residuals, params);
}

inline Series evaluate_upper_bound_residual_(const Bounds& bounds, const ParameterPack<Series>& params) {
  if (!bounds.upper.has_value()) return empty_vec_like_(params);
  std::vector<Series> residuals;
  residuals.reserve(params.size());
  for (const auto& block : params.blocks()) {
    const Series& upper = bounds.upper->at(block.name);
    residuals.push_back(block.value.sub(upper.clamp(-1e20, 1e20)));
  }
  return concat_flat_(residuals, params);
}

inline Series identity_matrix_(std::int64_t n,
                               pluss::codem::Platform platform);

inline Series stack_cols_series_(const std::vector<Series>& cols,
                                 std::int64_t nrows,
                                 pluss::codem::Platform platform);

template <class AD>
AD dense_matvec_autodiff_(const Series& matrix, const AD& vector) {
  require_defined_(matrix, "dense_matvec_autodiff");
  if (matrix.sizes().size() != 2) {
    ::pluss::throw_error("optim::dense_matvec_autodiff_: expected a rank-2 matrix");
  }
  const std::int64_t rows = matrix.sizes()[0];
  const std::int64_t cols = matrix.sizes()[1];
  if (vector.value().numel() != cols) {
    ::pluss::throw_error("optim::dense_matvec_autodiff_: matrix/vector shape mismatch");
  }
  const Series aligned =
      matrix.to(ScalarType::Float64).to(vector.value().platform()).reshape({rows, cols});
  return detail::Trace(aligned.matmul(vector.value().reshape({cols, 1})).reshape({rows}));
}

inline std::pair<Series, ParameterPack<Series>> objective_autodiff_value_and_grad_(
    const ConstrainedProblem& problem,
    const ParameterPack<Series>& params) {
  if (problem.objective.autodiff) {
    return detail::value_and_grad_trace_(
        [&](const ParameterPack<detail::Trace>& p) -> detail::Trace {
          detail::Trace out = problem.objective.autodiff(p);
          if (out.defined() && out.value().numel() != 1) {
            ::pluss::throw_error("optim: the objective must return a single number, but it returned " +
                                 std::to_string(out.value().numel()) +
                                 " values; reduce it with sum() or mean()");
          }
          return out;
        },
        params);
  }
  ::pluss::throw_error(
      "optim::objective_autodiff_value_and_grad_: autodiff objective callback is not defined");
}

template <class AD>
AD call_objective_autodiff_(const ConstrainedProblem& problem, const ParameterPack<AD>& params) {
  static_assert(std::is_same_v<AD, detail::Trace>);
  if (problem.objective.autodiff) return problem.objective.autodiff(params);
  ::pluss::throw_error(
      "optim::call_objective_autodiff_: autodiff objective callback is required");
}

template <class AD>
std::optional<AD> call_nonlinear_equality_autodiff_(const ConstrainedProblem& problem,
                                                    const ParameterPack<AD>& params) {
  static_assert(std::is_same_v<AD, detail::Trace>);
  if (problem.nonlinear.equality) return problem.nonlinear.equality(params);
  return std::nullopt;
}

template <class AD>
std::optional<AD> call_nonlinear_inequality_autodiff_(const ConstrainedProblem& problem,
                                                      const ParameterPack<AD>& params) {
  static_assert(std::is_same_v<AD, detail::Trace>);
  if (problem.nonlinear.inequality) return problem.nonlinear.inequality(params);
  return std::nullopt;
}

inline bool requires_dual_nonlinear_callbacks_(const ConstrainedProblem& problem) {
  return static_cast<bool>(problem.nonlinear.analytic) ||
         static_cast<bool>(problem.nonlinear.equality) ||
         static_cast<bool>(problem.nonlinear.inequality);
}

inline bool supports_exact_curvature_(const ConstrainedProblem& problem,
                                      OptimaHessianApproximation approximation) {
  if (!(approximation == OptimaHessianApproximation::Exact ||
        approximation == OptimaHessianApproximation::HessianVector)) {
    return true;
  }
  // Exact curvature differentiates the callbacks twice, so it needs them written over Series rather than
  // supplied as values and gradients.
  if (!problem.objective.autodiff) return false;
  if (problem.nonlinear.analytic && !(problem.nonlinear.equality && problem.nonlinear.inequality)) return false;
  return true;
}

inline Series slice_series_(const Series& x, std::int64_t offset, std::int64_t length);

template <class AD>
AD barrier_objective_autodiff_(const ConstrainedProblem& problem,
                               const ParameterPack<AD>& params,
                               double barrier_parameter,
                               double equality_penalty,
                               const OptimaNormalizationStats* normalization = nullptr) {
  using pluss::subsp::math::diff::log;
  using pluss::subsp::math::diff::square;
  using pluss::subsp::math::diff::sum;

  const double objective_scale = normalization ? normalization->objective_scale : 1.0;
  AD total = call_objective_autodiff_(problem, params) * objective_scale;
  const AD flat = flatten_values(params);
  const std::int64_t n_linear_eq = problem.linear.Aeq.has_value() ? problem.linear.Aeq->sizes()[0] : 0;

  if (problem.linear.Aeq.has_value()) {
    AD linear_eq = dense_matvec_autodiff_(*problem.linear.Aeq, flat);
    if (problem.linear.beq.has_value()) {
      linear_eq = linear_eq - constant_autodiff_(linear_eq, problem.linear.beq->reshape({problem.linear.beq->numel()}));
    }
    if (normalization && normalization->equality_row_scale.defined() && n_linear_eq > 0) {
      linear_eq = linear_eq * constant_autodiff_(
                                  linear_eq,
                                  slice_series_(normalization->equality_row_scale, 0, n_linear_eq));
    }
    total = total + 0.5 * equality_penalty * sum(square(linear_eq));
  }

  if (problem.linear.Aineq.has_value()) {
    AD linear_ineq = dense_matvec_autodiff_(*problem.linear.Aineq, flat);
    if (problem.linear.bineq.has_value()) {
      linear_ineq = linear_ineq - constant_autodiff_(linear_ineq, problem.linear.bineq->reshape({problem.linear.bineq->numel()}));
    }
    total = total - barrier_parameter * sum(log(-1.0 * linear_ineq));
  }

  if (problem.bounds.lower.has_value()) {
    const Series lower = flatten_values(*problem.bounds.lower).reshape({flat.value().numel()});
    total = total - barrier_parameter * sum(log(flat - constant_autodiff_(flat, lower)));
  }

  if (problem.bounds.upper.has_value()) {
    const Series upper = flatten_values(*problem.bounds.upper).reshape({flat.value().numel()});
    total = total - barrier_parameter * sum(log(constant_autodiff_(flat, upper) - flat));
  }

  if (const auto equality = call_nonlinear_equality_autodiff_(problem, params); equality.has_value()) {
    AD eq = *equality;
    if (normalization && normalization->equality_row_scale.defined() && eq.value().numel() > 0) {
      eq = eq * constant_autodiff_(
                    eq,
                    slice_series_(normalization->equality_row_scale, n_linear_eq, eq.value().numel()));
    }
    total = total + 0.5 * equality_penalty * sum(square(eq));
  }

  if (const auto inequality = call_nonlinear_inequality_autodiff_(problem, params); inequality.has_value()) {
    total = total - barrier_parameter * sum(log(-1.0 * (*inequality)));
  }

  return total;
}

inline Series assemble_hessian_from_hvp_(const ConstrainedProblem& problem,
                                         const ParameterPack<Series>& params,
                                         double barrier_parameter,
                                         double equality_penalty,
                                         const OptimaNormalizationStats& normalization,
                                         const pluss::subsp::math::diff::HvpOptions& hvp_options,
                                         bool& ok) {
  ok = false;
  const std::int64_t n = params.total_numel();
  const auto platform = params.empty() ? pluss::codem::Platform::CPU : params[0].value.platform();
  std::vector<Series> cols;
  cols.reserve(static_cast<std::size_t>(n));
  try {
    for (std::int64_t i = 0; i < n; ++i) {
      const Series basis = pluss::subsp::math::diff::detail::flat_basis_like_(
          flatten_values(params).to(ScalarType::Float64).to(platform), i);
      const auto tangent = unflatten_values(params.layout(), basis);
      auto [_, hv] = hvp_trace_(
          [&](const ParameterPack<detail::Trace>& p) -> detail::Trace {
            return barrier_objective_autodiff_(problem, p, barrier_parameter, equality_penalty, &normalization);
          },
          params,
          tangent,
          hvp_options);
      cols.push_back(flatten_values(hv).to(ScalarType::Float64).to(platform).reshape({n, 1}));
    }
  } catch (...) {
    return identity_matrix_(n, platform);
  }
  ok = static_cast<std::int64_t>(cols.size()) == n;
  return stack_cols_series_(cols, n, platform);
}

// Lagrangian f + y.h + z.g with the multipliers held constant, written over the problem's callbacks so
// that a Hessian-vector product through it gives the curvature a primal-dual step needs. Linear rows have
// no curvature and are left out. `equality_dual` is in the solver's scaled units (all equality rows, linear
// first); `inequality_dual` covers all inequality rows in assembly order.
template <class AD>
AD lagrangian_objective_autodiff_(const ConstrainedProblem& problem,
                                  const ParameterPack<AD>& params,
                                  const Series& equality_dual,
                                  const std::vector<double>& equality_scale,
                                  const Series& inequality_dual,
                                  std::int64_t linear_equality_rows,
                                  std::int64_t nonlinear_inequality_offset,
                                  const OptimaNormalizationStats& normalization) {
  using pluss::subsp::math::diff::sum;

  AD total = call_objective_autodiff_(problem, params) * normalization.objective_scale;
  if (const auto equality = call_nonlinear_equality_autodiff_(problem, params); equality.has_value()) {
    AD eq = *equality;
    const std::int64_t count = eq.value().numel();
    std::vector<double> weights(static_cast<std::size_t>(count), 0.0);
    const Series y = equality_dual.to(ScalarType::Float64).reshape({equality_dual.numel()});
    for (std::int64_t j = 0; j < count; ++j) {
      const std::int64_t row = linear_equality_rows + j;
      if (row < y.numel()) {
        weights[static_cast<std::size_t>(j)] =
            equality_scale[static_cast<std::size_t>(row)] * y.select(0, row).item<double>();
      }
    }
    total = total + sum(eq * constant_autodiff_(
                                 eq,
                                 pluss::make_series(std::move(weights), {count})
                                     .to(ScalarType::Float64)
                                     .to(eq.value().platform())));
  }
  if (const auto inequality = call_nonlinear_inequality_autodiff_(problem, params); inequality.has_value()) {
    AD ineq = *inequality;
    const std::int64_t count = ineq.value().numel();
    const Series z = inequality_dual.to(ScalarType::Float64).reshape({inequality_dual.numel()});
    total = total + sum(ineq * constant_autodiff_(ineq, slice_series_(z, nonlinear_inequality_offset, count)));
  }
  return total;
}

inline Series assemble_lagrangian_hessian_(const ConstrainedProblem& problem,
                                           const ParameterPack<Series>& params,
                                           const Series& equality_dual,
                                           const std::vector<double>& equality_scale,
                                           const Series& inequality_dual,
                                           std::int64_t linear_equality_rows,
                                           std::int64_t nonlinear_inequality_offset,
                                           const OptimaNormalizationStats& normalization,
                                           const pluss::subsp::math::diff::HvpOptions& hvp_options,
                                           bool& ok) {
  ok = false;
  const std::int64_t n = params.total_numel();
  const auto platform = params.empty() ? pluss::codem::Platform::CPU : params[0].value.platform();
  std::vector<Series> cols;
  cols.reserve(static_cast<std::size_t>(n));
  try {
    for (std::int64_t i = 0; i < n; ++i) {
      const Series basis = pluss::subsp::math::diff::detail::flat_basis_like_(
          flatten_values(params).to(ScalarType::Float64).to(platform), i);
      const auto tangent = unflatten_values(params.layout(), basis);
      auto [_, hv] = hvp_trace_(
          [&](const ParameterPack<detail::Trace>& p) -> detail::Trace {
            return lagrangian_objective_autodiff_(problem, p, equality_dual, equality_scale, inequality_dual,
                                                  linear_equality_rows, nonlinear_inequality_offset, normalization);
          },
          params,
          tangent,
          hvp_options);
      cols.push_back(flatten_values(hv).to(ScalarType::Float64).to(platform).reshape({n, 1}));
    }
  } catch (...) {
    return identity_matrix_(n, platform);
  }
  ok = static_cast<std::int64_t>(cols.size()) == n;
  return stack_cols_series_(cols, n, platform);
}

inline ParameterPack<Series> lift_flat_gradient_(const ParameterPack<Series>& params, const Series& flat_grad) {
  return unflatten_values(params.layout(), flat_grad.reshape({flat_grad.numel()}));
}

inline ParameterPack<Series> add_pack_(const ParameterPack<Series>& lhs, const ParameterPack<Series>& rhs) {
  if (lhs.size() != rhs.size()) {
    ::pluss::throw_error("optim::add_pack_: layout mismatch");
  }
  std::vector<typename ParameterPack<Series>::Block> out;
  out.reserve(lhs.size());
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    const auto& left = lhs[i];
    const auto& right = rhs[i];
    if (left.name != right.name) {
      ::pluss::throw_error("optim::add_pack_: block name mismatch at index " +
                           std::to_string(i) + " ('" + left.name + "' versus '" + right.name + "')");
    }
    if (left.value.platform() != right.value.platform()) {
      ::pluss::throw_error("optim::add_pack_: platform mismatch in block '" +
                           left.name + "'");
    }
    if (left.value.numel() != right.value.numel()) {
      ::pluss::throw_error("optim::add_pack_: element-count mismatch in block '" +
                           left.name + "'");
    }
    Series right_value = detail::same_shape_(left.value, right.value)
                             ? right.value
                             : right.value.reshape(left.value.sizes());
    Series sum = left.value.add(right_value);
    if (sum.scalar_type() != left.value.scalar_type()) {
      sum = sum.to(left.value.scalar_type());
    }
    out.push_back({left.name, std::move(sum)});
  }
  return ParameterPack<Series>(std::move(out));
}

inline ParameterPack<Series> scaled_pack_(const ParameterPack<Series>& pack, double scale) {
  std::vector<typename ParameterPack<Series>::Block> out;
  out.reserve(pack.size());
  for (const auto& block : pack.blocks()) {
    out.push_back({block.name, block.value.mul(scale)});
  }
  return ParameterPack<Series>(std::move(out));
}

inline double pack_inf_norm_(const ParameterPack<Series>& pack) {
  double best = 0.0;
  for (const auto& block : pack.blocks()) {
    best = std::max(best, max_abs_scalar_(block.value));
  }
  return best;
}

inline bool pack_finite_(const ParameterPack<Series>& pack) {
  for (const auto& block : pack.blocks()) {
    if (!block.value.defined() || !is_finite_series_(block.value)) return false;
  }
  return true;
}

inline std::string current_exception_message_(std::string_view fallback) {
  try {
    throw;
  } catch (const std::exception& ex) {
    return ex.what();
  } catch (...) {
    return std::string(fallback);
  }
}

inline std::string callback_failure_message_(std::string_view callback_name,
                                             std::string_view fallback = "callback raised an unknown error") {
  return std::string(callback_name) + " failed: " + current_exception_message_(fallback);
}

inline double max_abs_diff_series_(const Series& a, const Series& b) {
  if (!a.defined() || !b.defined()) return std::numeric_limits<double>::infinity();
  if (a.numel() != b.numel()) return std::numeric_limits<double>::infinity();
  if (a.numel() == 0) return 0.0;
  return a.to(ScalarType::Float64)
      .reshape({a.numel()})
      .sub(b.to(ScalarType::Float64))
      .reshape({a.numel()})
      .abs()
      .max()
      .item<double>();
}

inline double max_abs_value_series_(const Series& x) {
  if (!x.defined() || x.numel() == 0) return 0.0;
  return x.to(ScalarType::Float64).abs().reshape({x.numel()}).max().item<double>();
}

inline double max_rel_diff_series_(const Series& a, const Series& b) {
  const double abs_diff = max_abs_diff_series_(a, b);
  const double scale = std::max({1.0, max_abs_value_series_(a), max_abs_value_series_(b)});
  return abs_diff / scale;
}

inline double max_abs_diff_pack_(const ParameterPack<Series>& lhs, const ParameterPack<Series>& rhs) {
  if (!lhs.same_layout(rhs)) return std::numeric_limits<double>::infinity();
  double best = 0.0;
  for (const auto& block : lhs.blocks()) {
    best = std::max(best, max_abs_diff_series_(block.value, rhs.at(block.name)));
  }
  return best;
}

inline double max_abs_value_pack_(const ParameterPack<Series>& pack) {
  double best = 0.0;
  for (const auto& block : pack.blocks()) {
    best = std::max(best, max_abs_value_series_(block.value));
  }
  return best;
}

inline double max_rel_diff_pack_(const ParameterPack<Series>& lhs, const ParameterPack<Series>& rhs) {
  const double abs_diff = max_abs_diff_pack_(lhs, rhs);
  const double scale = std::max({1.0, max_abs_value_pack_(lhs), max_abs_value_pack_(rhs)});
  return abs_diff / scale;
}

inline ParameterPack<Series> project_bounds_(const ParameterPack<Series>& params, const Bounds& bounds);

inline ParameterPack<Series> lift_flat_std_(const ParameterPack<Series>& like,
                                            const std::vector<double>& flat) {
  if (flat.size() != static_cast<std::size_t>(like.total_numel())) {
    ::pluss::throw_error("optim::lift_flat_std_: size mismatch");
  }
  if (like.empty()) return like;
  Series flat_series =
      pluss::make_series(flat, {static_cast<std::int64_t>(flat.size())}).to(ScalarType::Float64);
  if (flat_series.platform() != like[0].value.platform()) {
    flat_series = flat_series.to(like[0].value.platform());
  }
  return unflatten_values(like.layout(), flat_series);
}

inline Series zeros_series_(std::int64_t n,
                            pluss::codem::Platform platform = pluss::codem::Platform::CPU) {
  return pluss::series::zeros({n}, ScalarType::Float64).to(platform);
}

inline Series ones_series_(std::int64_t n,
                           pluss::codem::Platform platform = pluss::codem::Platform::CPU) {
  return pluss::series::ones({n}, ScalarType::Float64).to(platform);
}

inline Series identity_matrix_(std::int64_t n,
                               pluss::codem::Platform platform = pluss::codem::Platform::CPU) {
  std::vector<double> out(static_cast<std::size_t>(n * n), 0.0);
  for (std::int64_t i = 0; i < n; ++i) out[static_cast<std::size_t>(i * n + i)] = 1.0;
  return pluss::make_series(std::move(out), {n, n}).to(ScalarType::Float64).to(platform);
}

inline Series slice_series_(const Series& x, std::int64_t offset, std::int64_t length);

inline double constraint_violation_value_(const ProblemEvaluation& evaluation,
                                          const OptimaNormalizationStats& normalization);

inline Series inequality_residual_series_(const FlatConstraintSystem& system,
                                          pluss::codem::Platform platform);

inline Series equality_residual_series_(const FlatConstraintSystem& system,
                                        pluss::codem::Platform platform);

inline Series stack_inequality_jacobian_(const FlatConstraintSystem& system,
                                         std::int64_t ncols,
                                         pluss::codem::Platform platform);

inline Series stack_equality_jacobian_(const FlatConstraintSystem& system,
                                       std::int64_t ncols,
                                       pluss::codem::Platform platform);

inline Series solve_spd_system_cg_series_(const Series& matrix,
                                          const Series& rhs,
                                          double regularization,
                                          std::int64_t max_iterations,
                                          double relative_tolerance,
                                          double absolute_tolerance,
                                          bool& ok,
                                          std::int64_t& iterations);

inline double series_inf_norm_(const Series& x) {
  if (!x.defined() || x.numel() == 0) return 0.0;
  return x.to(ScalarType::Float64).abs().reshape({x.numel()}).max().item<double>();
}

inline double series_dot_(const Series& a, const Series& b) {
  return a.to(ScalarType::Float64)
      .reshape({1, a.numel()})
      .matmul(b.to(ScalarType::Float64).reshape({b.numel(), 1}))
      .reshape(std::vector<std::int64_t>{})
      .item<double>();
}

inline Series series_add_scaled_(const Series& a, const Series& b, double scale_b = 1.0) {
  return a.add(b.mul(scale_b));
}

inline Series series_outer_(const Series& a, const Series& b) {
  return a.reshape({a.numel(), 1}).matmul(b.reshape({1, b.numel()}));
}

inline Series compute_variable_scale_(const ParameterPack<Series>& params,
                                      const Bounds& bounds,
                                      OptimaProblemScaling scaling) {
  const std::int64_t n = params.total_numel();
  const auto platform = params.empty() ? pluss::codem::Platform::CPU : params[0].value.platform();
  if (n == 0 || scaling == OptimaProblemScaling::None) {
    return ones_series_(n, platform);
  }

  Series scale = flatten_values(params).to(ScalarType::Float64).to(platform).abs().maximum(1.0);
  if (bounds.lower.has_value()) {
    scale = scale.maximum(flatten_values(*bounds.lower).to(ScalarType::Float64).to(platform).abs());
  }
  if (bounds.upper.has_value()) {
    scale = scale.maximum(flatten_values(*bounds.upper).to(ScalarType::Float64).to(platform).abs());
  }
  return scale.maximum(1.0);
}

inline bool normalize_objective_constraints_(OptimaProblemScaling scaling) {
  return scaling == OptimaProblemScaling::ObjectiveAndConstraints;
}

inline Series slice_series_(const Series& x, std::int64_t offset, std::int64_t length) {
  if (!x.defined() || x.numel() == 0 || length <= 0) {
    return pluss::series::empty({0}, ScalarType::Float64).to(x.defined() ? x.platform() : pluss::codem::Platform::CPU);
  }
  return x.reshape({x.numel()}).slice(0, offset, offset + length, 1).reshape({length});
}

inline Series row_scale_series_(const std::vector<FlatConstraintRow>& rows,
                                pluss::codem::Platform platform) {
  if (rows.empty()) return pluss::series::empty({0}, ScalarType::Float64).to(platform);
  std::vector<double> scales;
  scales.reserve(rows.size());
  for (const auto& row : rows) {
    const double jac_norm = series_inf_norm_(row.jacobian);
    const double denom = std::clamp(std::max(std::abs(row.residual), jac_norm), 1e-6, 1e6);
    scales.push_back(1.0 / denom);
  }
  return pluss::make_series(std::move(scales), {static_cast<std::int64_t>(rows.size())})
      .to(ScalarType::Float64)
      .to(platform);
}

inline OptimaNormalizationStats compute_problem_normalization_unexpanded_(const ProblemEvaluation& evaluation,
                                                               const FlatConstraintSystem& system,
                                                               OptimaProblemScaling scaling,
                                                               pluss::codem::Platform platform) {
  OptimaNormalizationStats out;
  if (!normalize_objective_constraints_(scaling)) {
    out.equality_row_scale = pluss::series::ones({static_cast<std::int64_t>(system.equalities.size())}, ScalarType::Float64).to(platform);
    out.inequality_row_scale = pluss::series::ones({static_cast<std::int64_t>(system.inequalities.size())}, ScalarType::Float64).to(platform);
    return out;
  }
  const auto bounded_norm = [](double value) {
    return std::clamp(value, 1e-6, 1e6);
  };
  out.objective_scale =
      1.0 / bounded_norm(std::max(std::abs(evaluation.value.item<double>()), pack_inf_norm_(evaluation.gradient)));
  out.equality_row_scale = row_scale_series_(system.equalities, platform);
  out.inequality_row_scale = row_scale_series_(system.inequalities, platform);
  return out;
}

inline OptimaNormalizationStats compute_problem_normalization_(const ProblemEvaluation& evaluation,
                                                               const FlatConstraintSystem& system,
                                                               OptimaProblemScaling scaling,
                                                               pluss::codem::Platform platform,
                                                               std::int64_t lower_total = 0,
                                                               std::int64_t upper_total = 0) {
  OptimaNormalizationStats out = compute_problem_normalization_unexpanded_(evaluation, system, scaling, platform);
  // Rows for infinite bounds are omitted from the system; give the scale one entry per evaluated residual
  // (linear, lower bounds, upper bounds, nonlinear) so it lines up with `evaluation.inequality_residual`.
  const std::int64_t full = evaluation.inequality_residual.defined() ? evaluation.inequality_residual.numel() : 0;
  if (full != static_cast<std::int64_t>(system.inequalities.size())) {
    const std::int64_t n_nl = evaluation.nonlinear_inequality_residual.defined()
                                  ? evaluation.nonlinear_inequality_residual.numel() : 0;
    const std::int64_t n_lin = full - lower_total - upper_total - n_nl;
    std::vector<double> expanded(static_cast<std::size_t>(full), 1.0);
    for (std::size_t i = 0; i < system.inequalities.size(); ++i) {
      const auto& row = system.inequalities[i];
      std::int64_t pos = row.index;
      if (row.source == ConstraintSource::LowerBound) pos += n_lin;
      else if (row.source == ConstraintSource::UpperBound) pos += n_lin + lower_total;
      else if (row.source == ConstraintSource::NonlinearInequality) pos += n_lin + lower_total + upper_total;
      expanded[static_cast<std::size_t>(pos)] = out.inequality_row_scale.select(0, static_cast<std::int64_t>(i)).item<double>();
    }
    out.inequality_row_scale = pluss::make_series(std::move(expanded), {full}).to(ScalarType::Float64).to(platform);
  }
  return out;
}

inline std::vector<FlatConstraintRow> scale_constraint_rows_(const std::vector<FlatConstraintRow>& rows,
                                                             const Series& variable_scale,
                                                             const Series& row_scale) {
  if ((!variable_scale.defined() || variable_scale.numel() == 0) &&
      (!row_scale.defined() || row_scale.numel() == 0)) {
    return rows;
  }
  std::vector<FlatConstraintRow> scaled;
  scaled.reserve(rows.size());
  const Series var = variable_scale.defined() ? variable_scale.to(ScalarType::Float64) : pluss::series::undefined();
  const Series weights = row_scale.defined() ? row_scale.to(ScalarType::Float64).reshape({row_scale.numel()}) : pluss::series::undefined();
  for (std::size_t i = 0; i < rows.size(); ++i) {
    const double w =
        (weights.defined() && static_cast<std::int64_t>(i) < weights.numel())
            ? weights.select(0, static_cast<std::int64_t>(i)).item<double>()
            : 1.0;
    Series jac = rows[i].jacobian.to(ScalarType::Float64);
    if (var.defined() && var.numel() > 0) jac = jac.to(var.platform()).mul(var);
    jac = jac.mul(w);
    scaled.push_back(FlatConstraintRow{
        std::move(jac),
        rows[i].residual * w,
        rows[i].equality,
        rows[i].source,
        rows[i].index});
  }
  return scaled;
}

inline Series active_constraint_row_scales_(const FlatConstraintSystem& system,
                                            const std::vector<FlatConstraintRow>& active_constraints,
                                            const OptimaNormalizationStats& normalization,
                                            pluss::codem::Platform platform) {
  if (active_constraints.empty()) return pluss::series::empty({0}, ScalarType::Float64).to(platform);
  const Series eq = normalization.equality_row_scale.defined()
      ? normalization.equality_row_scale.to(ScalarType::Float64).reshape({normalization.equality_row_scale.numel()})
      : pluss::series::undefined();
  const Series ineq = normalization.inequality_row_scale.defined()
      ? normalization.inequality_row_scale.to(ScalarType::Float64).reshape({normalization.inequality_row_scale.numel()})
      : pluss::series::undefined();
  std::unordered_map<ConstraintScaleKey, double, ConstraintScaleKeyHash> row_scale_lookup;
  row_scale_lookup.reserve(system.equalities.size() + system.inequalities.size());
  for (std::size_t i = 0; i < system.equalities.size(); ++i) {
    double w = 1.0;
    if (eq.defined() && static_cast<std::int64_t>(i) < eq.numel()) {
      w = eq.select(0, static_cast<std::int64_t>(i)).item<double>();
    }
    row_scale_lookup[ConstraintScaleKey{true, system.equalities[i].source, system.equalities[i].index}] = w;
  }
  for (std::size_t i = 0; i < system.inequalities.size(); ++i) {
    double w = 1.0;
    if (ineq.defined() && static_cast<std::int64_t>(i) < ineq.numel()) {
      w = ineq.select(0, static_cast<std::int64_t>(i)).item<double>();
    }
    row_scale_lookup[ConstraintScaleKey{false, system.inequalities[i].source, system.inequalities[i].index}] = w;
  }

  std::vector<double> weights;
  weights.reserve(active_constraints.size());
  for (const auto& row : active_constraints) {
    const auto it = row_scale_lookup.find(ConstraintScaleKey{row.equality, row.source, row.index});
    weights.push_back(it == row_scale_lookup.end() ? 1.0 : it->second);
  }
  return pluss::make_series(std::move(weights), {static_cast<std::int64_t>(active_constraints.size())})
      .to(ScalarType::Float64)
      .to(platform);
}

inline Series scale_gradient_(const Series& gradient, const Series& variable_scale) {
  if (!gradient.defined()) return gradient;
  if (!variable_scale.defined() || variable_scale.numel() == 0) return gradient.to(ScalarType::Float64);
  return gradient.to(ScalarType::Float64).mul(variable_scale.to(ScalarType::Float64).to(gradient.platform()));
}

inline Series unscale_step_(const Series& scaled_step, const Series& variable_scale) {
  if (!scaled_step.defined()) return scaled_step;
  if (!variable_scale.defined() || variable_scale.numel() == 0) return scaled_step.to(ScalarType::Float64);
  return scaled_step.to(ScalarType::Float64).mul(variable_scale.to(ScalarType::Float64).to(scaled_step.platform()));
}

inline Series scale_hessian_(const Series& hessian, const Series& variable_scale) {
  if (!hessian.defined() || !variable_scale.defined() || variable_scale.numel() == 0) {
    return hessian.to(ScalarType::Float64);
  }
  const Series scale = variable_scale.to(ScalarType::Float64).to(hessian.platform());
  return hessian.to(ScalarType::Float64)
      .mul(scale.reshape({scale.numel(), 1}))
      .mul(scale.reshape({1, scale.numel()}));
}

inline std::vector<FlatConstraintRow> scale_constraint_rows_(const std::vector<FlatConstraintRow>& rows,
                                                             const Series& variable_scale) {
  return scale_constraint_rows_(rows, variable_scale, pluss::series::undefined());
}

inline Series solve_dense_system_series_(const Series& matrix,
                                         const Series& rhs,
                                         double regularization,
                                         OptimaLinearSystemSolver linear_system_solver,
                                         double pivot_threshold,
                                         bool& ok) {
  ok = false;
  require_defined_(matrix, "solve_dense_system_series");
  require_defined_(rhs, "solve_dense_system_series");
  if (matrix.sizes().size() != 2 || matrix.sizes()[0] != matrix.sizes()[1]) {
    ::pluss::throw_error("optim::solve_dense_system_series_: expected square matrix");
  }
  const std::int64_t n = matrix.sizes()[0];
  const auto platform = matrix.platform();
  const bool host_dense_allowed = platform == pluss::codem::Platform::CPU;
  const Series base = matrix.to(ScalarType::Float64).to(platform);
  const Series rhs2d = rhs.to(ScalarType::Float64).to(platform).reshape({n, 1});
  const Series identity = identity_matrix_(n, platform);
  const double base_regularization = std::max(regularization, 0.0);
  const std::array<double, 5> regularization_scales{1.0, 10.0, 100.0, 1000.0, 1.0e6};

  auto try_device_cg = [&](const Series& candidate, const Series& rhs_col, Series& solution) -> bool {
    bool cg_ok = false;
    std::int64_t cg_iterations = 0;
    const Series rhs_vec = rhs_col.reshape({n});
    solution = solve_spd_system_cg_series_(
        candidate,
        rhs_vec,
        std::max(regularization, 1e-12),
        std::max<std::int64_t>(32, 2 * n),
        1e-8,
        1e-10,
        cg_ok,
        cg_iterations);
    return cg_ok && is_finite_series_(solution);
  };
  auto try_cholesky = [&](const Series& candidate, const Series& rhs_col, Series& solution) -> bool {
    if (!host_dense_allowed) return false;
    try {
      const Series factor = pluss::subsp::math::linalg::cholesky(candidate, false);
      solution = pluss::subsp::math::linalg::cholesky_solve(rhs_col, factor, false).reshape({n});
      return is_finite_series_(solution);
    } catch (...) {
      return false;
    }
  };
  auto try_inverse = [&](const Series& candidate, const Series& rhs_col, Series& solution) -> bool {
    if (!host_dense_allowed) return false;
    try {
      solution = pluss::subsp::math::linalg::inverse(candidate).matmul(rhs_col).reshape({n});
      return is_finite_series_(solution);
    } catch (...) {
      return false;
    }
  };
  auto try_pinverse = [&](const Series& candidate, const Series& rhs_col, Series& solution) -> bool {
    if (!host_dense_allowed) return false;
    try {
      solution = pluss::subsp::math::linalg::pinverse(candidate, 1e-12).matmul(rhs_col).reshape({n});
      return is_finite_series_(solution);
    } catch (...) {
      return false;
    }
  };
  auto try_pivoted_direct = [&](const Series& candidate, const Series& rhs_col, Series& solution) -> bool {
    try {
      if (!host_dense_allowed || n > 256) return false;
      (void)pivot_threshold;
      solution = pluss::subsp::math::linalg::lu_solve(rhs_col, candidate).reshape({n}).to(ScalarType::Float64).to(platform);
      return is_finite_series_(solution);
    } catch (...) {
      return false;
    }
  };

  for (double scale : regularization_scales) {
    const double shift = std::max(1e-12, base_regularization * scale);
    const Series reg = base.add(identity.mul(shift));
    const Series symmetric_reg = reg.add(reg.transpose(0, 1)).mul(0.5);
    Series solution = pluss::series::undefined();

    switch (linear_system_solver) {
      case OptimaLinearSystemSolver::Auto:
        if ((!host_dense_allowed && try_device_cg(symmetric_reg, rhs2d, solution)) ||
            try_cholesky(symmetric_reg, rhs2d, solution) ||
            try_pivoted_direct(reg, rhs2d, solution) ||
            try_inverse(reg, rhs2d, solution) ||
            try_pinverse(reg, rhs2d, solution)) {
          ok = true;
          return solution;
        }
        break;
      case OptimaLinearSystemSolver::Cholesky:
        if (try_cholesky(symmetric_reg, rhs2d, solution)) {
          ok = true;
          return solution;
        }
        break;
      case OptimaLinearSystemSolver::PivotedDirect:
        if (try_pivoted_direct(reg, rhs2d, solution)) {
          ok = true;
          return solution;
        }
        break;
      case OptimaLinearSystemSolver::Inverse:
        if (try_inverse(reg, rhs2d, solution)) {
          ok = true;
          return solution;
        }
        break;
      case OptimaLinearSystemSolver::PseudoInverse:
        if (try_pinverse(reg, rhs2d, solution)) {
          ok = true;
          return solution;
        }
        break;
      case OptimaLinearSystemSolver::ConjugateGradient:
        if (try_device_cg(symmetric_reg, rhs2d, solution)) {
          ok = true;
          return solution;
        }
        break;
    }
  }

  if (!host_dense_allowed) {
    Series solution = pluss::series::undefined();
    if (try_device_cg(base.add(identity.mul(std::max(1e-12, base_regularization))), rhs2d, solution)) {
      ok = true;
      return solution;
    }
    return zeros_series_(n, platform);
  }
  if (linear_system_solver != OptimaLinearSystemSolver::PseudoInverse) {
    Series solution = pluss::series::undefined();
    if (try_pinverse(base.add(identity.mul(std::max(1e-12, base_regularization))), rhs2d, solution)) {
      ok = true;
      return solution;
    }
  }
  return zeros_series_(n, platform);
}

inline Series solve_spd_system_cg_series_(const Series& matrix,
                                          const Series& rhs,
                                          double regularization,
                                          std::int64_t max_iterations,
                                          double relative_tolerance,
                                          double absolute_tolerance,
                                          bool& ok,
                                          std::int64_t& iterations) {
  ok = false;
  iterations = 0;
  require_defined_(matrix, "solve_spd_system_cg_series");
  require_defined_(rhs, "solve_spd_system_cg_series");
  if (matrix.sizes().size() != 2 || matrix.sizes()[0] != matrix.sizes()[1]) {
    ::pluss::throw_error("optim::solve_spd_system_cg_series_: expected square matrix");
  }
  const std::int64_t n = matrix.sizes()[0];
  const auto platform = matrix.platform();
  const Series base = matrix.to(ScalarType::Float64).to(platform);
  const Series b = rhs.to(ScalarType::Float64).to(platform).reshape({n});
  const Series identity = identity_matrix_(n, platform);
  const Series A = base.add(identity.mul(std::max(regularization, 1e-12)));

  Series x = zeros_series_(n, platform);
  Series r = b;
  const Series diag = A.diagonal(0, 0, 1).reshape({n}).abs().maximum(1e-12);
  const Series inv_diag = diag.reciprocal();
  Series z = r.mul(inv_diag);
  Series p = z;
  double rz = series_dot_(r, z);
  const double initial_residual = series_inf_norm_(r);
  const double tolerance = std::max(absolute_tolerance, relative_tolerance * initial_residual);
  if (initial_residual <= tolerance) {
    ok = true;
    return x;
  }

  const std::int64_t iter_budget = max_iterations > 0 ? max_iterations : std::max<std::int64_t>(10, 2 * n);
  for (std::int64_t iter = 0; iter < iter_budget; ++iter) {
    const Series Ap = dense_matvec_(A, p);
    const double curvature = series_dot_(p, Ap);
    if (!(curvature > 1e-18) || !std::isfinite(curvature)) {
      return x;
    }
    const double alpha = rz / curvature;
    x = x.add(p.mul(alpha));
    r = r.sub(Ap.mul(alpha));
    iterations = iter + 1;
    if (series_inf_norm_(r) <= tolerance) {
      ok = true;
      return x;
    }
    z = r.mul(inv_diag);
    const double new_rz = series_dot_(r, z);
    if (!(new_rz > 0.0) || !std::isfinite(new_rz)) {
      return x;
    }
    const double beta = new_rz / std::max(rz, 1e-18);
    p = z.add(p.mul(beta));
    rz = new_rz;
  }
  ok = series_inf_norm_(r) <= tolerance;
  return x;
}

template <class MatVecFn>
inline Series solve_spd_system_cg_operator_(MatVecFn&& matvec,
                                            const Series& rhs,
                                            const Series& diagonal_preconditioner,
                                            std::int64_t max_iterations,
                                            double relative_tolerance,
                                            double absolute_tolerance,
                                            bool& ok,
                                            std::int64_t& iterations) {
  ok = false;
  iterations = 0;
  require_defined_(rhs, "solve_spd_system_cg_operator");
  require_defined_(diagonal_preconditioner, "solve_spd_system_cg_operator");
  const std::int64_t n = rhs.numel();
  const auto platform = rhs.platform();
  const Series b = rhs.to(ScalarType::Float64).to(platform).reshape({n});
  const Series inv_diag = diagonal_preconditioner.to(ScalarType::Float64).to(platform).abs().maximum(1e-12).reciprocal();

  Series x = zeros_series_(n, platform);
  Series r = b;
  Series z = r.mul(inv_diag);
  Series p = z;
  double rz = series_dot_(r, z);
  const double initial_residual = series_inf_norm_(r);
  const double tolerance = std::max(absolute_tolerance, relative_tolerance * initial_residual);
  if (initial_residual <= tolerance) {
    ok = true;
    return x;
  }

  const std::int64_t iter_budget = max_iterations > 0 ? max_iterations : std::max<std::int64_t>(10, 2 * n);
  for (std::int64_t iter = 0; iter < iter_budget; ++iter) {
    const Series Ap = std::forward<MatVecFn>(matvec)(p);
    const double curvature = series_dot_(p, Ap);
    if (!(curvature > 1e-18) || !std::isfinite(curvature)) {
      return x;
    }
    const double alpha = rz / curvature;
    x = x.add(p.mul(alpha));
    r = r.sub(Ap.mul(alpha));
    iterations = iter + 1;
    if (series_inf_norm_(r) <= tolerance) {
      ok = true;
      return x;
    }
    z = r.mul(inv_diag);
    const double new_rz = series_dot_(r, z);
    if (!(new_rz > 0.0) || !std::isfinite(new_rz)) {
      return x;
    }
    const double beta = new_rz / std::max(rz, 1e-18);
    p = z.add(p.mul(beta));
    rz = new_rz;
  }
  ok = series_inf_norm_(r) <= tolerance;
  return x;
}

inline bool use_iterative_subproblem_solver_(OptimaLinearSystemSolver solver,
                                             OptimaLinearSystemStructure structure,
                                             std::int64_t dimension,
                                             std::int64_t iterative_dimension_threshold) {
  if (solver == OptimaLinearSystemSolver::ConjugateGradient) return true;
  if (solver != OptimaLinearSystemSolver::Auto) return false;
  if (structure == OptimaLinearSystemStructure::Sparse) return true;
  const std::int64_t threshold =
      iterative_dimension_threshold > 0 ? iterative_dimension_threshold : 16;
  return dimension >= threshold;
}

inline Series project_direction_from_active_bounds_(const ConstrainedProblem& problem,
                                                    const ParameterPack<Series>& params,
                                                    const Series& direction,
                                                    double tolerance = 1e-10) {
  if (!direction.defined() || !problem.bounds.lower.has_value() && !problem.bounds.upper.has_value()) {
    return direction;
  }
  const auto layout = params.layout();
  auto direction_pack = unflatten_values(layout, direction.to(ScalarType::Float64));
  std::vector<typename ParameterPack<Series>::Block> out_blocks;
  out_blocks.reserve(direction_pack.size());

  for (const auto& block : direction_pack.blocks()) {
    Series dir = block.value.to(ScalarType::Float64).reshape({block.value.numel()});
    const auto block_platform = dir.platform();
    const Series value = params.at(block.name).to(ScalarType::Float64).to(block_platform).reshape({block.value.numel()});
    const Series zero = pluss::series::zeros_like(dir, ScalarType::Float64).to(block_platform);

    if (problem.bounds.lower.has_value()) {
      const Series lower =
          problem.bounds.lower->at(block.name).to(ScalarType::Float64).to(block_platform).reshape({block.value.numel()});
      const Series at_lower = value.le(lower.add(tolerance));
      const Series push_out = dir.lt(0.0);
      const Series block_mask = at_lower.logical_and(push_out);
      dir = dir.where(block_mask, zero);
    }
    if (problem.bounds.upper.has_value()) {
      const Series upper =
          problem.bounds.upper->at(block.name).to(ScalarType::Float64).to(block_platform).reshape({block.value.numel()});
      const Series at_upper = value.ge(upper.sub(tolerance));
      const Series push_out = dir.gt(0.0);
      const Series block_mask = at_upper.logical_and(push_out);
      dir = dir.where(block_mask, zero);
    }

    out_blocks.push_back(
        {block.name,
         dir.reshape(block.value.sizes()).to(block.value.platform())});
  }
  return flatten_values(ParameterPack<Series>(std::move(out_blocks))).to(ScalarType::Float64).to(direction.platform());
}

inline Series stack_rows_series_(const std::vector<Series>& rows,
                                 std::int64_t ncols,
                                 pluss::codem::Platform platform) {
  if (rows.empty()) return pluss::series::empty({0, ncols}, ScalarType::Float64).to(platform);
  if (rows.size() == 1) return rows.front().reshape({1, ncols});
  return rows.front().reshape({1, ncols}).unite(
      [&]() {
        std::vector<Series> rest;
        rest.reserve(rows.size() - 1);
        for (std::size_t i = 1; i < rows.size(); ++i) rest.push_back(rows[i].reshape({1, ncols}));
        return rest;
      }(),
      0);
}

inline Series stack_cols_series_(const std::vector<Series>& cols,
                                 std::int64_t nrows,
                                 pluss::codem::Platform platform) {
  if (cols.empty()) return pluss::series::empty({nrows, 0}, ScalarType::Float64).to(platform);
  if (cols.size() == 1) return cols.front().reshape({nrows, 1});
  return cols.front().reshape({nrows, 1}).unite(
      [&]() {
        std::vector<Series> rest;
        rest.reserve(cols.size() - 1);
        for (std::size_t i = 1; i < cols.size(); ++i) rest.push_back(cols[i].reshape({nrows, 1}));
        return rest;
      }(),
      1);
}

inline Series jacobian_row_from_pack_(const ParameterPack<Series>& params,
                                      const ParameterPack<Series>& jacobian,
                                      std::int64_t row) {
  const auto platform = params.empty() ? pluss::codem::Platform::CPU : params[0].value.platform();
  std::vector<Series> row_pieces;
  row_pieces.reserve(params.size());
  for (const auto& block : params.blocks()) {
    if (!jacobian.contains(block.name)) {
      row_pieces.push_back(pluss::series::zeros({block.value.numel()}, ScalarType::Float64).to(platform));
      continue;
    }
    const Series& block_jacobian = jacobian.at(block.name);
    const std::int64_t cols = block.value.numel();
    if (block_jacobian.numel() == 0) {
      row_pieces.push_back(pluss::series::zeros({cols}, ScalarType::Float64).to(platform));
      continue;
    }
    if (block_jacobian.sizes().size() != 2 || block_jacobian.sizes()[0] <= row || block_jacobian.sizes()[1] != cols) {
      ::pluss::throw_error("optim::jacobian_row_from_pack_: shape mismatch");
    }
    row_pieces.push_back(
        block_jacobian.to(ScalarType::Float64).to(platform).reshape({block_jacobian.sizes()[0], cols}).select(0, row).reshape({cols}));
  }
  if (row_pieces.empty()) {
    return pluss::series::empty({0}, ScalarType::Float64).to(platform);
  }
  if (row_pieces.size() == 1) return row_pieces.front().to(ScalarType::Float64).to(platform);
  return row_pieces.front().unite(std::vector<Series>(row_pieces.begin() + 1, row_pieces.end()), 0)
      .to(ScalarType::Float64)
      .to(platform);
}

inline Series one_hot_jacobian_row_(std::int64_t n,
                                    std::int64_t index,
                                    double value,
                                    pluss::codem::Platform platform) {
  Series row = zeros_series_(n, platform);
  if (index < 0 || index >= n) return row;
  const Series idx = pluss::make_series(std::vector<std::int64_t>{index}, {1})
                         .to(ScalarType::Int64)
                         .to(platform);
  const Series val = pluss::make_series(std::vector<double>{value}, {1})
                         .to(ScalarType::Float64)
                         .to(platform);
  return row.index_put({idx}, val, false).to(ScalarType::Float64).to(platform);
}

// Reads a Series to host doubles in one transfer.
inline std::vector<double> host_doubles_(const Series& x) {
  if (!x.defined() || x.numel() == 0) return {};
  return x.to(ScalarType::Float64).to(pluss::codem::Platform::CPU).reshape({x.numel()}).host<double>();
}

// One constraint row as a Series (and kept as host values for the dense consumers).
inline FlatConstraintRow make_flat_row_(std::vector<double> dense, double residual, bool equality, ConstraintSource source,
                                        std::int64_t index, pluss::codem::Platform platform) {
  const std::int64_t n = static_cast<std::int64_t>(dense.size());
  Series jac = pluss::make_series(dense, {n}).to(ScalarType::Float64);
  if (platform != pluss::codem::Platform::CPU) jac = jac.to(platform);
  FlatConstraintRow row{jac, residual, equality, source, index};
  row.dense = std::move(dense);
  return row;
}

inline FlatConstraintSystem assemble_constraint_system_(const ConstrainedProblem& problem,
                                                        const ParameterPack<Series>& params,
                                                        const ProblemEvaluation& evaluation) {
  FlatConstraintSystem out;
  const std::int64_t n = params.total_numel();
  const auto platform = params.empty() ? pluss::codem::Platform::CPU : params[0].value.platform();
  const Series flat_params = flatten_values(params);
  const std::vector<double> x_host = host_doubles_(flat_params);

  // Linear rows: the matrix and the residual are read once; each row is a slice of the host copy.
  const auto linear_rows = [&](const Series& A, const Series& b, bool equality, ConstraintSource source,
                               std::vector<FlatConstraintRow>& dest) {
    const std::int64_t rows = A.sizes()[0];
    const std::vector<double> a = host_doubles_(A);
    const std::vector<double> rhs = b.defined() ? host_doubles_(b) : std::vector<double>();
    for (std::int64_t row = 0; row < rows; ++row) {
      std::vector<double> dense(a.begin() + row * n, a.begin() + (row + 1) * n);
      double residual = -(row < static_cast<std::int64_t>(rhs.size()) ? rhs[static_cast<std::size_t>(row)] : 0.0);
      for (std::int64_t j = 0; j < n; ++j) residual += dense[static_cast<std::size_t>(j)] * x_host[static_cast<std::size_t>(j)];
      dest.push_back(make_flat_row_(std::move(dense), residual, equality, source, row, platform));
    }
  };
  if (problem.linear.Aeq.has_value())
    linear_rows(problem.linear.Aeq->reshape({problem.linear.Aeq->sizes()[0], n}),
                problem.linear.beq.has_value() ? *problem.linear.beq : Series(), true, ConstraintSource::LinearEquality, out.equalities);

  // Nonlinear rows: each block of the Jacobian pack is read once.
  struct PackHost { std::vector<std::vector<double>> block; std::vector<std::int64_t> cols; std::int64_t rows{0}; };
  const auto read_pack = [&](const ParameterPack<Series>& jac, std::int64_t rows) {
    PackHost host;
    host.rows = rows;
    for (const auto& blk : params.blocks()) {
      const std::int64_t cols = blk.value.numel();
      host.cols.push_back(cols);
      if (!jac.contains(blk.name) || jac.at(blk.name).numel() == 0) {
        host.block.emplace_back();  // a block the constraint does not depend on: zeros
        continue;
      }
      const Series& bj = jac.at(blk.name);
      if (bj.sizes().size() != 2 || bj.sizes()[0] < rows || bj.sizes()[1] != cols)
        ::pluss::throw_error("optim::jacobian_row_from_pack_: shape mismatch");
      host.block.push_back(host_doubles_(bj));
    }
    return host;
  };
  const auto pack_row = [&](const PackHost& host, std::int64_t row) {
    std::vector<double> dense(static_cast<std::size_t>(n), 0.0);
    std::int64_t offset = 0;
    for (std::size_t k = 0; k < host.block.size(); ++k) {
      const std::int64_t cols = host.cols[k];
      if (!host.block[k].empty())
        std::copy(host.block[k].begin() + row * cols, host.block[k].begin() + (row + 1) * cols, dense.begin() + offset);
      offset += cols;
    }
    return dense;
  };

  const std::vector<double> nonlinear_eq = host_doubles_(evaluation.nonlinear_equality_residual);
  {
    const PackHost host = read_pack(evaluation.equality_jacobian, static_cast<std::int64_t>(nonlinear_eq.size()));
    for (std::int64_t row = 0; row < static_cast<std::int64_t>(nonlinear_eq.size()); ++row)
      out.equalities.push_back(make_flat_row_(pack_row(host, row), nonlinear_eq[static_cast<std::size_t>(row)], true,
                                              ConstraintSource::NonlinearEquality, row, platform));
  }

  if (problem.linear.Aineq.has_value())
    linear_rows(problem.linear.Aineq->reshape({problem.linear.Aineq->sizes()[0], n}),
                problem.linear.bineq.has_value() ? *problem.linear.bineq : Series(), false, ConstraintSource::LinearInequality,
                out.inequalities);

  // Bounds: one-hot rows (infinite bounds produce none).
  const auto bound_rows = [&](const std::optional<ParameterPack<Series>>& bound, bool lower) {
    if (!bound.has_value()) return;
    std::int64_t offset = 0;
    for (const auto& block : params.blocks()) {
      const std::vector<double> limit = host_doubles_(bound->at(block.name));
      const std::vector<double> value = host_doubles_(block.value);
      for (std::int64_t i = 0; i < block.value.numel(); ++i) {
        const double lim = limit[static_cast<std::size_t>(i)];
        if (std::abs(lim) >= 1e19) continue;  // infinite bound: no row
        std::vector<double> dense(static_cast<std::size_t>(n), 0.0);
        dense[static_cast<std::size_t>(offset + i)] = lower ? -1.0 : 1.0;
        const double residual = lower ? lim - value[static_cast<std::size_t>(i)] : value[static_cast<std::size_t>(i)] - lim;
        out.inequalities.push_back(make_flat_row_(std::move(dense), residual, false,
                                                  lower ? ConstraintSource::LowerBound : ConstraintSource::UpperBound,
                                                  offset + i, platform));
      }
      offset += block.value.numel();
    }
  };
  bound_rows(problem.bounds.lower, true);
  bound_rows(problem.bounds.upper, false);

  const std::vector<double> nonlinear_ineq = host_doubles_(evaluation.nonlinear_inequality_residual);
  {
    const PackHost host = read_pack(evaluation.inequality_jacobian, static_cast<std::int64_t>(nonlinear_ineq.size()));
    for (std::int64_t row = 0; row < static_cast<std::int64_t>(nonlinear_ineq.size()); ++row)
      out.inequalities.push_back(make_flat_row_(pack_row(host, row), nonlinear_ineq[static_cast<std::size_t>(row)], false,
                                                ConstraintSource::NonlinearInequality, row, platform));
  }

  return out;
}

inline std::vector<FlatConstraintRow> active_constraints_(const FlatConstraintSystem& system,
                                                          double active_set_tolerance) {
  std::vector<FlatConstraintRow> active = system.equalities;
  for (const auto& row : system.inequalities) {
    if (row.residual >= -active_set_tolerance) active.push_back(row);
  }
  return active;
}

inline bool same_constraint_(const FlatConstraintRow& lhs, const FlatConstraintRow& rhs) {
  return lhs.equality == rhs.equality && lhs.source == rhs.source && lhs.index == rhs.index;
}

inline bool same_constraint_(const WorkingSetConstraintKey& lhs, const WorkingSetConstraintKey& rhs) {
  return lhs.equality == rhs.equality && lhs.source_id == rhs.source_id && lhs.index == rhs.index;
}

inline WorkingSetConstraintKey constraint_key_(const FlatConstraintRow& row) {
  return WorkingSetConstraintKey{row.equality, static_cast<std::uint8_t>(row.source), row.index};
}

inline bool contains_constraint_(const std::vector<WorkingSetConstraintKey>& keys,
                                 const FlatConstraintRow& row) {
  const auto key = constraint_key_(row);
  return std::find_if(keys.begin(), keys.end(), [&](const WorkingSetConstraintKey& existing) {
           return same_constraint_(existing, key);
         }) != keys.end();
}

inline bool same_working_set_keys_(const std::vector<WorkingSetConstraintKey>& lhs,
                                   const std::vector<WorkingSetConstraintKey>& rhs) {
  if (lhs.size() != rhs.size()) return false;
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    if (!same_constraint_(lhs[i], rhs[i])) return false;
  }
  return true;
}

inline double series_scalar_at_or_(const Series& values, std::size_t index, double fallback = 0.0) {
  if (!values.defined() || values.numel() <= 0) return fallback;
  if (static_cast<std::int64_t>(index) >= values.numel()) return fallback;
  return values.to(ScalarType::Float64).reshape({values.numel()}).select(0, static_cast<std::int64_t>(index)).item<double>();
}

inline double multiplier_for_constraint_key_(const std::vector<WorkingSetConstraintKey>& keys,
                                             const Series& multipliers,
                                             const FlatConstraintRow& row) {
  const auto key = constraint_key_(row);
  const Series lambda = multipliers.defined() ? multipliers.to(ScalarType::Float64).reshape({multipliers.numel()}) : pluss::series::undefined();
  for (std::size_t i = 0; i < keys.size(); ++i) {
    if (same_constraint_(keys[i], key)) return series_scalar_at_or_(lambda, i, 0.0);
  }
  return 0.0;
}

inline std::vector<double> aligned_working_set_multipliers_(
    const std::vector<FlatConstraintRow>& active_constraints,
    const std::vector<WorkingSetConstraintKey>& previous_keys,
    const Series& previous_multipliers) {
  std::vector<double> aligned(active_constraints.size(), 0.0);
  if (!previous_multipliers.defined() || previous_multipliers.numel() == 0 || previous_keys.empty()) return aligned;
  for (std::size_t i = 0; i < active_constraints.size(); ++i) {
    aligned[i] = multiplier_for_constraint_key_(previous_keys, previous_multipliers, active_constraints[i]);
  }
  return aligned;
}

inline Series smooth_working_set_multipliers_(const std::vector<FlatConstraintRow>& active_constraints,
                                              const Series& multipliers,
                                              const std::vector<double>& previous_aligned,
                                              double smoothing) {
  if (!multipliers.defined() || multipliers.numel() == 0 || previous_aligned.empty()) return multipliers;
  Series lambda = multipliers.to(ScalarType::Float64).reshape({multipliers.numel()});
  Series previous = pluss::make_series(previous_aligned, {static_cast<std::int64_t>(previous_aligned.size())})
                        .to(ScalarType::Float64)
                        .to(lambda.platform())
                        .reshape({lambda.numel()});
  if (previous.numel() != lambda.numel()) return multipliers;
  const double mix = std::clamp(smoothing, 0.0, 1.0);
  std::vector<bool> ineq_mask_host(static_cast<std::size_t>(lambda.numel()), false);
  for (std::size_t i = 0; i < active_constraints.size() && static_cast<std::int64_t>(i) < lambda.numel(); ++i) {
    ineq_mask_host[i] = !active_constraints[i].equality;
  }
  const Series ineq_mask = pluss::make_series(std::move(ineq_mask_host), {lambda.numel()})
                               .to(ScalarType::Bool)
                               .to(lambda.platform());
  const Series has_history = previous.abs().gt(0.0);
  const Series apply = ineq_mask.logical_and(has_history);
  const Series blended = lambda.mul(1.0 - mix).add(previous.mul(mix));
  return apply.where(blended, lambda).to(ScalarType::Float64).to(multipliers.platform());
}

inline std::vector<FlatConstraintRow> working_set_constraints_(const FlatConstraintSystem& system,
                                                               double active_set_tolerance,
                                                               double add_constraint_tolerance,
                                                               std::int64_t max_working_set_violations);

inline std::vector<WorkingSetConstraintKey> seed_working_set_keys_(const FlatConstraintSystem& system,
                                                                   double active_set_tolerance,
                                                                   double add_constraint_tolerance,
                                                                   std::int64_t max_working_set_violations) {
  std::vector<WorkingSetConstraintKey> keys;
  keys.reserve(system.equalities.size() + system.inequalities.size());
  for (const auto& row : system.equalities) keys.push_back(constraint_key_(row));
  const auto seed_rows =
      working_set_constraints_(system, active_set_tolerance, add_constraint_tolerance, max_working_set_violations);
  for (const auto& row : seed_rows) {
    if (!contains_constraint_(keys, row)) keys.push_back(constraint_key_(row));
  }
  return keys;
}

inline std::vector<FlatConstraintRow> persistent_working_set_constraints_(
    const FlatConstraintSystem& system,
    const std::vector<WorkingSetConstraintKey>& active_keys,
    double active_set_tolerance,
    double add_constraint_tolerance,
    std::int64_t max_working_set_violations) {
  std::vector<FlatConstraintRow> active = system.equalities;
  for (const auto& row : system.inequalities) {
    if (contains_constraint_(active_keys, row)) active.push_back(row);
  }

  std::vector<FlatConstraintRow> violated;
  std::vector<FlatConstraintRow> near_active;
  violated.reserve(system.inequalities.size());
  near_active.reserve(system.inequalities.size());
  for (const auto& row : system.inequalities) {
    if (row.residual > 0.0) {
      violated.push_back(row);
    } else if (row.residual >= -std::max(active_set_tolerance, add_constraint_tolerance) &&
               !contains_constraint_(active_keys, row)) {
      near_active.push_back(row);
    }
  }
  std::sort(violated.begin(), violated.end(), [](const FlatConstraintRow& lhs, const FlatConstraintRow& rhs) {
    return lhs.residual > rhs.residual;
  });
  std::sort(near_active.begin(), near_active.end(), [](const FlatConstraintRow& lhs, const FlatConstraintRow& rhs) {
    return lhs.residual > rhs.residual;
  });
  const auto append_unique = [&](const FlatConstraintRow& row) {
    const auto it = std::find_if(active.begin(), active.end(), [&](const FlatConstraintRow& existing) {
      return same_constraint_(existing, row);
    });
    if (it == active.end()) active.push_back(row);
  };
  for (const auto& row : violated) append_unique(row);
  const std::int64_t near_active_budget =
      max_working_set_violations > 0 ? max_working_set_violations : static_cast<std::int64_t>(near_active.size());
  for (std::int64_t i = 0; i < std::min<std::int64_t>(near_active_budget, static_cast<std::int64_t>(near_active.size()));
       ++i) {
    append_unique(near_active[static_cast<std::size_t>(i)]);
  }
  return active;
}

inline std::vector<WorkingSetConstraintKey> update_working_set_keys_(
    const FlatConstraintSystem& system,
    const std::vector<FlatConstraintRow>& active_constraints,
    const Series& multipliers,
    const std::vector<WorkingSetConstraintKey>& previous_keys,
    const Series& previous_multipliers,
    double active_set_tolerance,
    double add_constraint_tolerance,
    double drop_multiplier_tolerance,
    double multiplier_retention_tolerance,
    std::int64_t max_working_set_violations) {
  std::vector<WorkingSetConstraintKey> keys;
  keys.reserve(system.equalities.size() + system.inequalities.size());
  for (const auto& row : system.equalities) keys.push_back(constraint_key_(row));

  const Series lambda = multipliers.defined() ? multipliers.to(ScalarType::Float64).reshape({multipliers.numel()}) : pluss::series::undefined();
  const double near_active_threshold = std::max(active_set_tolerance, add_constraint_tolerance);
  for (std::size_t i = 0; i < active_constraints.size(); ++i) {
    const auto& row = active_constraints[i];
    if (row.equality) continue;
    const double multiplier = series_scalar_at_or_(lambda, i, 0.0);
    const double persistent_multiplier = multiplier_for_constraint_key_(previous_keys, previous_multipliers, row);
    if (multiplier >= -drop_multiplier_tolerance || row.residual > 0.0 ||
        row.residual >= -near_active_threshold ||
        (persistent_multiplier > multiplier_retention_tolerance &&
         row.residual >= -4.0 * near_active_threshold)) {
      keys.push_back(constraint_key_(row));
    }
  }

  for (const auto& row : system.inequalities) {
    if (row.residual > 0.0 && !contains_constraint_(keys, row)) keys.push_back(constraint_key_(row));
  }

  struct RankedWorkingSetRow {
    FlatConstraintRow row{};
    double persistent_multiplier{0.0};
  };
  std::vector<RankedWorkingSetRow> near_active;
  near_active.reserve(system.inequalities.size());
  for (const auto& row : system.inequalities) {
    const double persistent_multiplier = multiplier_for_constraint_key_(previous_keys, previous_multipliers, row);
    if (row.residual <= 0.0 &&
        (row.residual >= -near_active_threshold ||
         persistent_multiplier > multiplier_retention_tolerance) &&
        !contains_constraint_(keys, row)) {
      near_active.push_back({row, persistent_multiplier});
    }
  }
  std::sort(near_active.begin(), near_active.end(), [multiplier_retention_tolerance](const RankedWorkingSetRow& lhs,
                                                                                      const RankedWorkingSetRow& rhs) {
    const bool lhs_retained = lhs.persistent_multiplier > multiplier_retention_tolerance;
    const bool rhs_retained = rhs.persistent_multiplier > multiplier_retention_tolerance;
    if (lhs_retained != rhs_retained) return lhs_retained > rhs_retained;
    if (lhs.row.residual != rhs.row.residual) return lhs.row.residual > rhs.row.residual;
    return lhs.persistent_multiplier > rhs.persistent_multiplier;
  });
  const std::int64_t near_active_budget =
      max_working_set_violations > 0 ? max_working_set_violations : static_cast<std::int64_t>(near_active.size());
  for (std::int64_t i = 0; i < std::min<std::int64_t>(near_active_budget, static_cast<std::int64_t>(near_active.size()));
       ++i) {
    keys.push_back(constraint_key_(near_active[static_cast<std::size_t>(i)].row));
  }

  std::vector<WorkingSetConstraintKey> unique_keys;
  unique_keys.reserve(keys.size());
  for (const auto& key : keys) {
    const auto it = std::find_if(unique_keys.begin(), unique_keys.end(), [&](const WorkingSetConstraintKey& existing) {
      return same_constraint_(existing, key);
    });
    if (it == unique_keys.end()) unique_keys.push_back(key);
  }
  return unique_keys;
}

inline std::vector<FlatConstraintRow> working_set_constraints_(const FlatConstraintSystem& system,
                                                               double active_set_tolerance,
                                                               double add_constraint_tolerance,
                                                               std::int64_t max_working_set_violations) {
  std::vector<FlatConstraintRow> active = system.equalities;
  std::vector<FlatConstraintRow> near_active;
  std::vector<FlatConstraintRow> violated;
  near_active.reserve(system.inequalities.size());
  violated.reserve(system.inequalities.size());
  for (const auto& row : system.inequalities) {
    if (row.residual > 0.0) {
      violated.push_back(row);
    } else if (row.residual >= -std::max(active_set_tolerance, add_constraint_tolerance)) {
      near_active.push_back(row);
    }
  }

  std::sort(violated.begin(), violated.end(), [](const FlatConstraintRow& lhs, const FlatConstraintRow& rhs) {
    return lhs.residual > rhs.residual;
  });
  std::sort(near_active.begin(), near_active.end(), [](const FlatConstraintRow& lhs, const FlatConstraintRow& rhs) {
    return lhs.residual > rhs.residual;
  });

  const auto append_unique = [&](const FlatConstraintRow& row) {
    const auto it = std::find_if(active.begin(), active.end(), [&](const FlatConstraintRow& existing) {
      return same_constraint_(existing, row);
    });
    if (it == active.end()) active.push_back(row);
  };
  for (const auto& row : violated) append_unique(row);
  const std::int64_t near_active_budget =
      max_working_set_violations > 0 ? max_working_set_violations : static_cast<std::int64_t>(near_active.size());
  for (std::int64_t i = 0; i < std::min<std::int64_t>(near_active_budget, static_cast<std::int64_t>(near_active.size())); ++i) {
    append_unique(near_active[static_cast<std::size_t>(i)]);
  }
  return active;
}

inline double max_step_to_bounds_(const ConstrainedProblem& problem,
                                  const ParameterPack<Series>& params,
                                  const Series& step_direction,
                                  double fraction_to_boundary) {
  if ((!problem.bounds.lower.has_value() && !problem.bounds.upper.has_value()) ||
      !step_direction.defined() || step_direction.numel() == 0) {
    return std::numeric_limits<double>::infinity();
  }
  const auto platform = step_direction.platform();
  const Series x = flatten_values(params).to(ScalarType::Float64).to(platform).reshape({step_direction.numel()});
  const Series d = step_direction.to(ScalarType::Float64).to(platform).reshape({step_direction.numel()});
  const Series ones = pluss::series::ones({d.numel()}, ScalarType::Float64).to(platform);
  const Series inf_fill = ones.mul(std::numeric_limits<double>::infinity());
  const double direction_noise = 1e-12 * std::max(1.0, series_inf_norm_(d));
  double max_step = std::numeric_limits<double>::infinity();
  if (problem.bounds.lower.has_value()) {
    const Series lower = flatten_values(*problem.bounds.lower).to(ScalarType::Float64).to(platform).reshape({d.numel()});
    const Series mask = d.lt(-direction_noise);
    if (mask.any().item<bool>()) {
      const Series safe_denom = mask.where(d.mul(-1.0), ones);
      Series candidate = x.sub(lower).div(safe_denom).mul(fraction_to_boundary);
      candidate = mask.where(candidate, inf_fill);
      max_step = std::min(max_step, candidate.amin(0, false).item<double>());
    }
  }
  if (problem.bounds.upper.has_value()) {
    const Series upper = flatten_values(*problem.bounds.upper).to(ScalarType::Float64).to(platform).reshape({d.numel()});
    const Series mask = d.gt(direction_noise);
    if (mask.any().item<bool>()) {
      const Series safe_denom = mask.where(d, ones);
      Series candidate = upper.sub(x).div(safe_denom).mul(fraction_to_boundary);
      candidate = mask.where(candidate, inf_fill);
      max_step = std::min(max_step, candidate.amin(0, false).item<double>());
    }
  }
  return max_step;
}

inline double max_step_to_linearized_inequalities_(const FlatConstraintSystem& system,
                                                   const Series& step_direction,
                                                   double active_set_tolerance,
                                                   double fraction_to_boundary) {
  if (!step_direction.defined() || step_direction.numel() == 0) {
    return std::numeric_limits<double>::infinity();
  }
  if (system.inequalities.empty()) {
    return std::numeric_limits<double>::infinity();
  }
  const auto platform = step_direction.platform();
  const std::int64_t n = step_direction.numel();
  const Series residual = inequality_residual_series_(system, platform).reshape({static_cast<std::int64_t>(system.inequalities.size())});
  const Series directional =
      stack_inequality_jacobian_(system, n, platform).matmul(step_direction.reshape({n, 1})).reshape({residual.numel()});
  // Components of the step that are zero up to roundoff (the QP step holds active constraints on their
  // boundary) must not cap the step length at zero.
  const double direction_noise = 1e-12 * std::max(1.0, series_inf_norm_(step_direction));
  const Series active = residual.le(active_set_tolerance);
  const Series incoming = directional.gt(direction_noise);
  const Series mask = active.logical_and(incoming);
  if (!mask.any().item<bool>()) {
    return std::numeric_limits<double>::infinity();
  }
  const Series ones = pluss::series::ones({residual.numel()}, ScalarType::Float64).to(platform);
  const Series inf_fill = ones.mul(std::numeric_limits<double>::infinity());
  const Series safe_denom = mask.where(directional, ones);
  Series candidate = residual.mul(-1.0).div(safe_denom).mul(fraction_to_boundary);
  candidate = mask.where(candidate, inf_fill);
  return candidate.amin(0, false).item<double>();
}

inline bool accept_sqp_trial_(const ProblemEvaluation& base_evaluation,
                              double base_merit,
                              const ProblemEvaluation& trial_evaluation,
                              double trial_merit,
                              double step_size,
                              double armijo_rhs,
                              double constraint_tolerance) {
  if (!std::isfinite(trial_merit) || trial_merit > armijo_rhs) return false;
  if (base_evaluation.max_constraint_violation <= constraint_tolerance) return true;
  if (trial_evaluation.max_constraint_violation <= constraint_tolerance) return true;
  const double target_violation = std::max(
      constraint_tolerance,
      base_evaluation.max_constraint_violation *
          std::max(0.1, 1.0 - 0.25 * std::max(step_size, 0.0)));
  return trial_evaluation.max_constraint_violation <= target_violation ||
         trial_merit + 1e-12 < base_merit;
}

inline bool accept_working_set_trial_(const ProblemEvaluation& base_evaluation,
                                      double base_merit,
                                      const ProblemEvaluation& trial_evaluation,
                                      double trial_merit,
                                      double step_size,
                                      double armijo_rhs,
                                      double constraint_tolerance,
                                      double feasibility_priority_ratio) {
  if (!std::isfinite(trial_merit) || trial_merit > armijo_rhs) return false;
  if (base_evaluation.max_constraint_violation <= constraint_tolerance) {
    return trial_evaluation.max_constraint_violation <= constraint_tolerance &&
           trial_merit <= base_merit + 1e-12;
  }
  if (trial_evaluation.max_constraint_violation <= constraint_tolerance) return true;

  const double target_violation = std::max(
      constraint_tolerance,
      feasibility_priority_ratio * base_evaluation.max_constraint_violation *
          std::max(0.5, 1.0 - 0.1 * std::max(step_size, 0.0)));
  if (trial_evaluation.max_constraint_violation <= target_violation) return true;

  const double violation_improvement =
      base_evaluation.max_constraint_violation - trial_evaluation.max_constraint_violation;
  return violation_improvement > std::max(1e-10, 0.02 * base_evaluation.max_constraint_violation) &&
         trial_merit <= base_merit + 1e-8;
}

inline void cleanup_working_set_multipliers_(const std::vector<FlatConstraintRow>& active_constraints,
                                             double drop_multiplier_tolerance,
                                             Series* multipliers) {
  if (multipliers == nullptr || !multipliers->defined() || multipliers->numel() == 0) return;
  Series lambda = multipliers->to(ScalarType::Float64).reshape({multipliers->numel()});
  std::vector<bool> active_ineq_host(static_cast<std::size_t>(lambda.numel()), false);
  for (std::size_t i = 0; i < active_constraints.size() && static_cast<std::int64_t>(i) < lambda.numel(); ++i) {
    active_ineq_host[i] = !active_constraints[i].equality;
  }
  const Series active_ineq = pluss::make_series(std::move(active_ineq_host), {lambda.numel()})
                                 .to(ScalarType::Bool)
                                 .to(lambda.platform());
  const Series drop_mask = active_ineq.logical_and(lambda.lt(drop_multiplier_tolerance));
  *multipliers = drop_mask.where(pluss::series::zeros_like(lambda, ScalarType::Float64).to(lambda.platform()), lambda)
                     .to(ScalarType::Float64)
                     .to(multipliers->platform());
}

inline bool prefer_candidate_(const SequentialQPState& candidate,
                              const SequentialQPState& incumbent,
                              double constraint_tolerance,
                              double value_tolerance = 1e-10) {
  if (candidate.evaluation.status != EvalStatus::Ok) return false;
  if (incumbent.evaluation.status != EvalStatus::Ok) return true;
  const bool candidate_feasible = candidate.evaluation.max_constraint_violation <= constraint_tolerance;
  const bool incumbent_feasible = incumbent.evaluation.max_constraint_violation <= constraint_tolerance;
  if (candidate_feasible != incumbent_feasible) return candidate_feasible;
  if (candidate.evaluation.max_constraint_violation + value_tolerance <
      incumbent.evaluation.max_constraint_violation) {
    return true;
  }
  if (incumbent.evaluation.max_constraint_violation + value_tolerance <
      candidate.evaluation.max_constraint_violation) {
    return false;
  }
  if (candidate.evaluation.value.defined() && candidate.evaluation.value.numel() == 1 &&
      incumbent.evaluation.value.defined() && incumbent.evaluation.value.numel() == 1) {
    const double candidate_value = candidate.evaluation.value.item<double>();
    const double incumbent_value = incumbent.evaluation.value.item<double>();
    if (candidate_value + value_tolerance < incumbent_value) return true;
    if (incumbent_value + value_tolerance < candidate_value) return false;
  }
  return candidate.kkt_residual + value_tolerance < incumbent.kkt_residual;
}

inline std::vector<FlatConstraintRow> second_order_correction_rows_(const FlatConstraintSystem& base_system,
                                                                    const FlatConstraintSystem& trial_system,
                                                                    double active_set_tolerance) {
  std::vector<FlatConstraintRow> rows = active_constraints_(base_system, active_set_tolerance);
  const std::size_t eq_count = base_system.equalities.size();
  const std::size_t ineq_count = std::min(base_system.inequalities.size(), trial_system.inequalities.size());
  for (std::size_t i = 0; i < ineq_count; ++i) {
    if (trial_system.inequalities[i].residual >= -active_set_tolerance) {
      const auto already_active = std::any_of(
          rows.begin() + static_cast<std::ptrdiff_t>(eq_count),
          rows.end(),
          [&](const FlatConstraintRow& row) {
            return row.source == trial_system.inequalities[i].source &&
                   row.index == trial_system.inequalities[i].index;
          });
      if (!already_active) rows.push_back(base_system.inequalities[i]);
    }
  }
  return rows;
}

inline Series solve_second_order_correction_(const std::vector<FlatConstraintRow>& rows,
                                             const FlatConstraintSystem& trial_system,
                                             std::int64_t n,
                                             pluss::codem::Platform platform,
                                             double regularization,
                                             OptimaLinearSystemSolver linear_system_solver,
                                             double pivot_threshold,
                                             bool& ok) {
  ok = false;
  if (rows.empty()) return zeros_series_(n, platform);
  std::vector<Series> jac_rows;
  std::vector<double> rhs;
  jac_rows.reserve(rows.size());
  rhs.reserve(rows.size());
  for (const auto& row : rows) {
    jac_rows.push_back(row.jacobian.to(platform));
    double residual = row.residual;
    const auto& source_rows = row.equality ? trial_system.equalities : trial_system.inequalities;
    for (const auto& trial_row : source_rows) {
      if (trial_row.source == row.source && trial_row.index == row.index) {
        residual = trial_row.residual;
        break;
      }
    }
    rhs.push_back(-residual);
  }
  const Series A = stack_rows_series_(jac_rows, n, platform);
  const Series AAt = A.matmul(A.transpose(0, 1));
  const Series rhs_series = pluss::make_series(std::move(rhs), {static_cast<std::int64_t>(jac_rows.size())})
                                .to(ScalarType::Float64)
                                .to(platform);
  const Series lambda = solve_dense_system_series_(
      AAt,
      rhs_series,
      regularization,
      linear_system_solver == OptimaLinearSystemSolver::ConjugateGradient
          ? OptimaLinearSystemSolver::Auto
          : linear_system_solver,
      pivot_threshold,
      ok);
  if (!ok) return zeros_series_(n, platform);
  return A.transpose(0, 1).matmul(lambda.reshape({lambda.numel(), 1})).reshape({n});
}

inline bool feasible_with_tolerance_(const ProblemEvaluation& evaluation, double constraint_tolerance) {
  return evaluation.status == EvalStatus::Ok &&
         evaluation.max_constraint_violation <= std::max(0.0, constraint_tolerance);
}

inline void maybe_update_best_feasible_(SequentialQPState& state,
                                        double constraint_tolerance = 0.0) {
  if (!feasible_with_tolerance_(state.evaluation, constraint_tolerance) ||
      !state.evaluation.value.defined() || state.evaluation.value.numel() != 1) {
    return;
  }
  if (!state.best_feasible.has_value() ||
      state.evaluation.value.item<double>() <= state.best_feasible->value.item<double>()) {
    state.best_feasible = BestFeasiblePoint{
        state.params,
        state.evaluation.value,
        state.evaluation.max_constraint_violation,
        state.evaluation.first_order_opt};
  }
}

inline double inequality_margin_(const FlatConstraintSystem& system) {
  if (system.inequalities.empty()) return std::numeric_limits<double>::infinity();
  const Series residuals = inequality_residual_series_(system, pluss::codem::Platform::CPU);
  return residuals.mul(-1.0).amin(0, false).item<double>();
}

inline double equality_residual_norm_(const FlatConstraintSystem& system) {
  if (system.equalities.empty()) return 0.0;
  const Series residuals = equality_residual_series_(system, pluss::codem::Platform::CPU);
  return residuals.square().sum().sqrt().item<double>();
}

inline double barrier_value_(const ProblemEvaluation& evaluation,
                             const FlatConstraintSystem& system,
                             double barrier_parameter,
                             double equality_penalty) {
  if (!(barrier_parameter > 0.0) || !std::isfinite(barrier_parameter)) {
    ::pluss::throw_error("optim::barrier_value_: barrier parameter must be positive");
  }

  double value = evaluation.value.item<double>();
  for (const auto& row : system.equalities) {
    value += 0.5 * equality_penalty * row.residual * row.residual;
  }
  for (const auto& row : system.inequalities) {
    if (!(row.residual < 0.0)) return std::numeric_limits<double>::infinity();
    value -= barrier_parameter * std::log(-row.residual);
  }
  return value;
}

inline double barrier_value_(const ProblemEvaluation& evaluation,
                             const FlatConstraintSystem& system,
                             double barrier_parameter,
                             double equality_penalty,
                             const OptimaNormalizationStats& normalization) {
  if (!(barrier_parameter > 0.0) || !std::isfinite(barrier_parameter)) {
    ::pluss::throw_error("optim::barrier_value_: barrier parameter must be positive");
  }
  double value = normalization.objective_scale * evaluation.value.item<double>();
  const Series eq_scales = normalization.equality_row_scale.defined()
      ? normalization.equality_row_scale.to(ScalarType::Float64).reshape({normalization.equality_row_scale.numel()})
      : pluss::series::undefined();
  for (std::size_t i = 0; i < system.equalities.size(); ++i) {
    const double scale =
        (eq_scales.defined() && static_cast<std::int64_t>(i) < eq_scales.numel())
            ? eq_scales.select(0, static_cast<std::int64_t>(i)).item<double>()
            : 1.0;
    value += 0.5 * equality_penalty * std::pow(scale * system.equalities[i].residual, 2);
  }
  for (const auto& row : system.inequalities) {
    if (!(row.residual < 0.0)) return std::numeric_limits<double>::infinity();
    value -= barrier_parameter * std::log(-row.residual);
  }
  return value;
}

inline Series inequality_slack_(const FlatConstraintSystem& system,
                                pluss::codem::Platform platform) {
  if (system.inequalities.empty()) return pluss::series::empty({0}, ScalarType::Float64).to(platform);
  std::vector<double> slack(system.inequalities.size(), 0.0);
  for (std::size_t i = 0; i < system.inequalities.size(); ++i) {
    slack[i] = std::max(-system.inequalities[i].residual, 1e-12);
  }
  return pluss::make_series(std::move(slack), {static_cast<std::int64_t>(system.inequalities.size())})
      .to(ScalarType::Float64)
      .to(platform);
}

inline std::vector<double> series_cpu_vec_(const Series& s) {
  if (!s.defined() || s.numel() == 0) return {};
  return s.to(ScalarType::Float64).to(pluss::codem::Platform::CPU).reshape({s.numel()}).host<double>();
}

inline std::int64_t count_nonlinear_rows_(const FlatConstraintSystem& system) {
  std::int64_t n = 0;
  for (const auto& row : system.inequalities) if (row.source == ConstraintSource::NonlinearInequality) ++n;
  return n;
}

// Slack vector of the internal-protocol iteration: -g(x) for bound and linear rows, which stay strictly
// satisfied, and the independent slack variable for nonlinear inequality rows.
inline Series ip_slack_(const FlatConstraintSystem& system, const Series& nl_slack, pluss::codem::Platform platform) {
  Series base = inequality_slack_(system, platform);
  const std::int64_t n_nl = count_nonlinear_rows_(system);
  if (n_nl == 0 || !nl_slack.defined() || nl_slack.numel() != n_nl) return base;
  std::vector<double> s = series_cpu_vec_(base);
  const std::vector<double> nl = series_cpu_vec_(nl_slack);
  for (std::size_t i = 0; i < system.inequalities.size(); ++i) {
    const auto& row = system.inequalities[i];
    if (row.source == ConstraintSource::NonlinearInequality) s[i] = std::max(nl[static_cast<std::size_t>(row.index)], 1e-12);
  }
  return pluss::make_series(std::move(s), {static_cast<std::int64_t>(system.inequalities.size())})
      .to(ScalarType::Float64).to(platform);
}

// g(x) + s for the nonlinear rows (zero elsewhere): the residual of the slack equation.
inline Series ip_gap_(const FlatConstraintSystem& system, const Series& slack, pluss::codem::Platform platform) {
  std::vector<double> gap(system.inequalities.size(), 0.0);
  if (!slack.defined() || slack.numel() != static_cast<std::int64_t>(gap.size())) {
    return pluss::make_series(std::move(gap), {static_cast<std::int64_t>(system.inequalities.size())}).to(ScalarType::Float64).to(platform);
  }
  const std::vector<double> s = series_cpu_vec_(slack);
  for (std::size_t i = 0; i < gap.size(); ++i) {
    if (system.inequalities[i].source == ConstraintSource::NonlinearInequality) gap[i] = system.inequalities[i].residual + s[i];
  }
  return pluss::make_series(std::move(gap), {static_cast<std::int64_t>(system.inequalities.size())}).to(ScalarType::Float64).to(platform);
}

// Nonlinear-row slack variables after a step ds (given for all rows) of length alpha.
inline Series nl_slack_after_(const FlatConstraintSystem& system, const Series& slack, const Series& slack_step,
                              double alpha, pluss::codem::Platform platform) {
  const std::int64_t n_nl = count_nonlinear_rows_(system);
  std::vector<double> out(static_cast<std::size_t>(n_nl), 1e-12);
  const std::vector<double> s = series_cpu_vec_(slack);
  const std::vector<double> ds = series_cpu_vec_(slack_step);
  for (std::size_t i = 0; i < system.inequalities.size(); ++i) {
    const auto& row = system.inequalities[i];
    if (row.source == ConstraintSource::NonlinearInequality)
      out[static_cast<std::size_t>(row.index)] = std::max(s[i] + alpha * ds[i], 1e-12);
  }
  return pluss::make_series(std::move(out), {n_nl}).to(ScalarType::Float64).to(platform);
}

// Slack resetting: wherever the constraint is still satisfied at the trial point the slack is set to -g(x), so
// the iteration behaves like a strictly interior one; only a row the step would push to or past its limit keeps
// the linearized slack and its gap, which the merit function then penalizes instead of rejecting the step.
inline Series nl_slack_reset_(const FlatConstraintSystem& system, const Series& nl_slack, pluss::codem::Platform platform) {
  if (!nl_slack.defined() || nl_slack.numel() == 0) return nl_slack;
  std::vector<double> out = series_cpu_vec_(nl_slack);
  for (const auto& row : system.inequalities)
    if (row.source == ConstraintSource::NonlinearInequality && static_cast<std::size_t>(row.index) < out.size())
      if (row.residual < 0.0) out[static_cast<std::size_t>(row.index)] = -row.residual;  // satisfied: no gap
  return pluss::make_series(std::move(out), {nl_slack.numel()}).to(ScalarType::Float64).to(platform);
}

// Initial independent slacks: -g(x) when the row is strictly satisfied, otherwise a small positive value.
inline Series initial_nl_slack_(const FlatConstraintSystem& system, double floor_value, pluss::codem::Platform platform) {
  const std::int64_t n_nl = count_nonlinear_rows_(system);
  if (n_nl == 0) return pluss::series::undefined();
  std::vector<double> out(static_cast<std::size_t>(n_nl), floor_value);
  for (const auto& row : system.inequalities)
    if (row.source == ConstraintSource::NonlinearInequality)
      out[static_cast<std::size_t>(row.index)] = std::max(-row.residual, floor_value);
  return pluss::make_series(std::move(out), {n_nl}).to(ScalarType::Float64).to(platform);
}

inline Series inequality_residual_series_(const FlatConstraintSystem& system,
                                          pluss::codem::Platform platform) {
  if (system.inequalities.empty()) return pluss::series::empty({0}, ScalarType::Float64).to(platform);
  std::vector<double> residuals(system.inequalities.size(), 0.0);
  for (std::size_t i = 0; i < system.inequalities.size(); ++i) {
    residuals[i] = system.inequalities[i].residual;
  }
  return pluss::make_series(std::move(residuals), {static_cast<std::int64_t>(system.inequalities.size())})
      .to(ScalarType::Float64)
      .to(platform);
}

inline Series equality_residual_series_(const FlatConstraintSystem& system,
                                        pluss::codem::Platform platform) {
  if (system.equalities.empty()) return pluss::series::empty({0}, ScalarType::Float64).to(platform);
  std::vector<double> residuals(system.equalities.size(), 0.0);
  for (std::size_t i = 0; i < system.equalities.size(); ++i) {
    residuals[i] = system.equalities[i].residual;
  }
  return pluss::make_series(std::move(residuals), {static_cast<std::int64_t>(system.equalities.size())})
      .to(ScalarType::Float64)
      .to(platform);
}

inline Series inequality_dual_(const Series& slack,
                               double barrier_parameter) {
  if (!slack.defined() || slack.numel() == 0) return slack;
  return slack.reciprocal().mul(std::max(barrier_parameter, 1e-12));
}

inline Series positive_series_(const Series& x,
                               double floor = 1e-12) {
  if (!x.defined() || x.numel() == 0) return x;
  return x.maximum(floor);
}

inline Series current_or_barrier_dual_(const Series& current_dual,
                                       const Series& slack,
                                       double barrier_parameter) {
  if (slack.numel() == 0) return slack;
  if (current_dual.defined() && current_dual.numel() == slack.numel()) {
    return positive_series_(current_dual.to(ScalarType::Float64).to(slack.platform()));
  }
  return inequality_dual_(slack, barrier_parameter);
}

inline Series update_dual_step_(const Series& dual,
                                const Series& dual_step,
                                double step_size) {
  if (!dual.defined() || dual.numel() == 0) return dual;
  if (!dual_step.defined() || dual_step.numel() != dual.numel()) return positive_series_(dual);
  return positive_series_(dual.add(dual_step.mul(step_size)));
}

inline Series blend_dual_toward_centrality_(const Series& dual,
                                            const Series& slack,
                                            double barrier_parameter,
                                            double blend_fraction) {
  if (!dual.defined() || dual.numel() == 0 || !slack.defined() || slack.numel() != dual.numel()) {
    return dual;
  }
  const double eta = std::clamp(blend_fraction, 0.0, 1.0);
  if (!(eta > 0.0)) return positive_series_(dual);
  const Series central_dual = inequality_dual_(slack, barrier_parameter);
  return positive_series_(dual.mul(1.0 - eta).add(central_dual.mul(eta)));
}

inline double complementarity_residual_(const Series& slack,
                                        const Series& dual,
                                        double barrier_parameter) {
  if (!slack.defined() || slack.numel() == 0 || !dual.defined() || dual.numel() == 0) return 0.0;
  return slack.mul(dual).sub(std::max(barrier_parameter, 0.0)).abs().max().item<double>();
}

inline Series stack_inequality_jacobian_(const FlatConstraintSystem& system,
                                         std::int64_t ncols,
                                         pluss::codem::Platform platform) {
  std::vector<Series> rows;
  rows.reserve(system.inequalities.size());
  for (const auto& row : system.inequalities) rows.push_back(row.jacobian.to(platform));
  return stack_rows_series_(rows, ncols, platform);
}

inline Series stack_equality_jacobian_(const FlatConstraintSystem& system,
                                       std::int64_t ncols,
                                       pluss::codem::Platform platform) {
  std::vector<Series> rows;
  rows.reserve(system.equalities.size());
  for (const auto& row : system.equalities) rows.push_back(row.jacobian.to(platform));
  return stack_rows_series_(rows, ncols, platform);
}

inline Series add_weighted_row_jacobians_(const Series& base,
                                          const std::vector<FlatConstraintRow>& rows,
                                          const Series& weights,
                                          std::int64_t row_limit = -1) {
  Series out = base.to(ScalarType::Float64).reshape({base.numel()});
  if (rows.empty() || !weights.defined() || weights.numel() == 0) return out;
  const auto platform = out.platform();
  const Series w = weights.to(ScalarType::Float64).to(platform).reshape({weights.numel()});
  const std::int64_t max_rows = row_limit > 0 ? row_limit : static_cast<std::int64_t>(rows.size());
  const std::int64_t m = std::min<std::int64_t>(std::min<std::int64_t>(static_cast<std::int64_t>(rows.size()), w.numel()),
                                                max_rows);
  if (m <= 0) return out;

  std::vector<Series> jac_rows;
  jac_rows.reserve(static_cast<std::size_t>(m));
  for (std::int64_t i = 0; i < m; ++i) {
    jac_rows.push_back(rows[static_cast<std::size_t>(i)].jacobian.to(ScalarType::Float64).to(platform).reshape({out.numel()}));
  }
  const Series J = stack_rows_series_(jac_rows, out.numel(), platform);
  const Series w_used = (m == w.numel()) ? w : w.slice(0, 0, m, 1).reshape({m});
  const Series contribution = J.transpose(0, 1).matmul(w_used.reshape({m, 1})).reshape({out.numel()});
  return out.add(contribution);
}

inline Series quadratic_rows_matvec_(const std::vector<FlatConstraintRow>& rows,
                                     const std::vector<double>& weights,
                                     const Series& direction,
                                     std::int64_t n,
                                     pluss::codem::Platform platform) {
  Series out = zeros_series_(n, platform);
  for (std::size_t i = 0; i < rows.size() && i < weights.size(); ++i) {
    const double weight = weights[i];
    if (!(weight > 0.0) || !std::isfinite(weight)) continue;
    const Series jac = rows[i].jacobian.to(ScalarType::Float64).to(platform).reshape({n});
    const double coeff = weight * series_dot_(jac, direction);
    if (std::abs(coeff) <= 0.0) continue;
    out = out.add(jac.mul(coeff));
  }
  return out;
}

inline Series quadratic_rows_diagonal_(const std::vector<FlatConstraintRow>& rows,
                                       const std::vector<double>& weights,
                                       std::int64_t n,
                                       pluss::codem::Platform platform) {
  Series diag = zeros_series_(n, platform);
  for (std::size_t i = 0; i < rows.size() && i < weights.size(); ++i) {
    const double weight = weights[i];
    if (!(weight > 0.0) || !std::isfinite(weight)) continue;
    const Series jac = rows[i].jacobian.to(ScalarType::Float64).to(platform).reshape({n});
    diag = diag.add(jac.square().mul(weight));
  }
  return diag;
}

inline std::vector<double> equality_quadratic_weights_(const FlatConstraintSystem& system,
                                                       double equality_penalty,
                                                       const OptimaNormalizationStats& normalization) {
  std::vector<double> weights(system.equalities.size(), std::max(0.0, equality_penalty));
  const Series eq_scales = normalization.equality_row_scale.defined()
      ? normalization.equality_row_scale.to(ScalarType::Float64).reshape({normalization.equality_row_scale.numel()})
      : pluss::series::undefined();
  for (std::size_t i = 0; i < weights.size(); ++i) {
    const double scale =
        (eq_scales.defined() && static_cast<std::int64_t>(i) < eq_scales.numel())
            ? eq_scales.select(0, static_cast<std::int64_t>(i)).item<double>()
            : 1.0;
    weights[i] *= scale * scale;
  }
  return weights;
}

inline std::vector<double> inequality_quadratic_weights_(const Series& slack,
                                                         const Series& dual) {
  const Series slack_flat = slack.defined() ? slack.to(ScalarType::Float64).reshape({slack.numel()}) : pluss::series::undefined();
  const Series dual_flat = dual.defined() ? dual.to(ScalarType::Float64).reshape({dual.numel()}) : pluss::series::undefined();
  const std::size_t m = std::min(
      slack_flat.defined() ? static_cast<std::size_t>(slack_flat.numel()) : std::size_t{0},
      dual_flat.defined() ? static_cast<std::size_t>(dual_flat.numel()) : std::size_t{0});
  std::vector<double> weights(m, 0.0);
  for (std::size_t i = 0; i < weights.size(); ++i) {
    const double si = std::max(slack_flat.select(0, static_cast<std::int64_t>(i)).item<double>(), 1e-12);
    const double zi = std::max(dual_flat.select(0, static_cast<std::int64_t>(i)).item<double>(), 0.0);
    weights[i] = zi / si;
  }
  return weights;
}

inline std::vector<double> equality_row_scales_(const FlatConstraintSystem& system,
                                                const OptimaNormalizationStats& normalization) {
  std::vector<double> scales(system.equalities.size(), 1.0);
  if (!normalization.equality_row_scale.defined()) return scales;
  const Series host =
      normalization.equality_row_scale.to(ScalarType::Float64).reshape({normalization.equality_row_scale.numel()});
  for (std::size_t i = 0; i < scales.size() && static_cast<std::int64_t>(i) < host.numel(); ++i) {
    scales[i] = host.select(0, static_cast<std::int64_t>(i)).item<double>();
  }
  return scales;
}

inline Series equality_dual_or_zero_(const Series& current, std::int64_t count, pluss::codem::Platform platform) {
  if (current.defined() && current.numel() == count) {
    return current.to(ScalarType::Float64).to(platform).reshape({count});
  }
  return pluss::series::zeros({count}, ScalarType::Float64).to(platform);
}

// Gradient of the Lagrangian f + y.h + z.g in the solver's scaled units: the objective is multiplied by
// its scale, equality row i by its row scale, and y is expressed against those scaled rows.
inline Series lagrangian_gradient_(const ProblemEvaluation& evaluation,
                                   const FlatConstraintSystem& system,
                                   const Series& equality_dual,
                                   const Series& inequality_dual,
                                   const OptimaNormalizationStats& normalization) {
  Series grad = flatten_values(evaluation.gradient).to(ScalarType::Float64).mul(normalization.objective_scale);
  grad = grad.reshape({grad.numel()});
  const auto platform = grad.platform();
  if (!system.equalities.empty() && equality_dual.defined() && equality_dual.numel() > 0) {
    const auto scales = equality_row_scales_(system, normalization);
    const Series y = equality_dual.to(ScalarType::Float64).reshape({equality_dual.numel()});
    std::vector<double> weights(system.equalities.size(), 0.0);
    for (std::size_t i = 0; i < weights.size() && static_cast<std::int64_t>(i) < y.numel(); ++i) {
      weights[i] = scales[i] * y.select(0, static_cast<std::int64_t>(i)).item<double>();
    }
    grad = add_weighted_row_jacobians_(
        grad,
        system.equalities,
        pluss::make_series(std::move(weights), {static_cast<std::int64_t>(system.equalities.size())})
            .to(ScalarType::Float64)
            .to(platform));
  }
  if (!system.inequalities.empty() && inequality_dual.defined() && inequality_dual.numel() > 0) {
    grad = add_weighted_row_jacobians_(
        grad, system.inequalities,
        inequality_dual.to(ScalarType::Float64).to(platform).reshape({inequality_dual.numel()}));
  }
  return grad;
}

inline double scaled_equality_l1_(const FlatConstraintSystem& system, const OptimaNormalizationStats& normalization) {
  const auto scales = equality_row_scales_(system, normalization);
  double total = 0.0;
  for (std::size_t i = 0; i < system.equalities.size(); ++i) {
    total += std::abs(scales[i] * system.equalities[i].residual);
  }
  return total;
}

// l1 exact-penalty barrier merit: f - mu * sum(log(-g_i)) + nu * sum |h_j|. Infinite when any
// inequality is not strictly satisfied, so the line search rejects a step that leaves the interior.
inline double merit_value_(const ProblemEvaluation& evaluation,
                           const FlatConstraintSystem& system,
                           double barrier_parameter,
                           const OptimaNormalizationStats& normalization,
                           double merit_penalty,
                           const Series& nl_slack = Series()) {
  double value = normalization.objective_scale * evaluation.value.item<double>();
  const bool use_slack = nl_slack.defined() && nl_slack.numel() == count_nonlinear_rows_(system) && nl_slack.numel() > 0;
  const std::vector<double> nl = use_slack ? series_cpu_vec_(nl_slack) : std::vector<double>{};
  double gap_l1 = 0.0;
  for (const auto& row : system.inequalities) {
    if (use_slack && row.source == ConstraintSource::NonlinearInequality) {
      const double s = nl[static_cast<std::size_t>(row.index)];
      if (!(s > 0.0)) return std::numeric_limits<double>::infinity();
      value -= barrier_parameter * std::log(s);
      gap_l1 += std::abs(row.residual + s);
      continue;
    }
    if (!(row.residual < 0.0)) return std::numeric_limits<double>::infinity();
    value -= barrier_parameter * std::log(-row.residual);
  }
  return value + merit_penalty * (scaled_equality_l1_(system, normalization) + gap_l1);
}

// Keeps each inequality multiplier within a factor kappa of its central value mu / s, so a stale
// multiplier can neither collapse to zero nor explode relative to its slack.
inline Series safeguard_inequality_dual_(const Series& dual, const Series& slack, double barrier_parameter) {
  if (!dual.defined() || dual.numel() == 0 || !slack.defined() || slack.numel() != dual.numel()) return dual;
  constexpr double kappa = 1e10;
  const Series central = slack.reciprocal().mul(std::max(barrier_parameter, 1e-12));
  return dual.to(ScalarType::Float64).minimum(central.mul(kappa)).maximum(central.div(kappa));
}

inline void update_primal_dual_metrics_(InternalProtocolState& state,
                                        const FlatConstraintSystem& system,
                                        double barrier_parameter,
                                        const OptimaNormalizationStats& normalization) {
  const auto platform =
      state.params.empty() ? pluss::codem::Platform::CPU : state.params[0].value.platform();
  state.inequality_slack = ip_slack_(system, state.nl_slack, platform);
  state.inequality_dual = safeguard_inequality_dual_(
      current_or_barrier_dual_(state.inequality_dual, state.inequality_slack, barrier_parameter),
      state.inequality_slack,
      barrier_parameter);
  state.equality_dual =
      equality_dual_or_zero_(state.equality_dual, static_cast<std::int64_t>(system.equalities.size()), platform);
  state.primal_residual = std::max(
      equality_residual_norm_(system),
      state.inequality_slack.defined() && state.inequality_slack.numel() > 0
          ? state.inequality_slack.add(inequality_residual_series_(system, platform)).abs().max().item<double>()
          : 0.0);
  state.complementarity_residual =
      complementarity_residual_(state.inequality_slack, state.inequality_dual, barrier_parameter);
  state.barrier_gradient_norm = series_inf_norm_(lagrangian_gradient_(
      state.evaluation, system, state.equality_dual, state.inequality_dual, normalization)) /
      std::max(1.0, state.gradient_scale);
  state.feasibility_metric = constraint_violation_value_(state.evaluation, normalization);
}

inline Series barrier_gradient_flat_(const ProblemEvaluation& evaluation,
                                     const FlatConstraintSystem& system,
                                     double barrier_parameter,
                                     double equality_penalty) {
  const Series flat_grad = flatten_values(evaluation.gradient).to(ScalarType::Float64);
  Series grad = flat_grad.reshape({flat_grad.numel()});
  const auto platform = grad.platform();
  if (!system.equalities.empty() && equality_penalty != 0.0) {
    const Series eq_weights = equality_residual_series_(system, platform).mul(equality_penalty);
    grad = add_weighted_row_jacobians_(grad, system.equalities, eq_weights);
  }
  if (!system.inequalities.empty()) {
    const Series ineq_residual = inequality_residual_series_(system, platform);
    if (ineq_residual.ge(0.0).any().item<bool>()) {
      return pluss::series::full({grad.numel()}, std::numeric_limits<double>::quiet_NaN(), ScalarType::Float64)
          .to(platform);
    }
    const Series ineq_weights = ineq_residual.reciprocal().mul(-barrier_parameter);
    grad = add_weighted_row_jacobians_(grad, system.inequalities, ineq_weights);
  }
  return grad.reshape({grad.numel()});
}

inline Series barrier_gradient_flat_(const ProblemEvaluation& evaluation,
                                     const FlatConstraintSystem& system,
                                     double barrier_parameter,
                                     double equality_penalty,
                                     const OptimaNormalizationStats& normalization) {
  Series grad = flatten_values(evaluation.gradient).to(ScalarType::Float64).mul(normalization.objective_scale);
  const auto platform = grad.platform();
  const Series eq_scales = normalization.equality_row_scale.defined()
      ? normalization.equality_row_scale.to(ScalarType::Float64).to(platform).reshape({normalization.equality_row_scale.numel()})
      : pluss::series::undefined();
  if (!system.equalities.empty() && equality_penalty != 0.0) {
    const Series eq_residual = equality_residual_series_(system, platform);
    std::vector<double> eq_weight_host(system.equalities.size(), 0.0);
    for (std::size_t i = 0; i < system.equalities.size(); ++i) {
      const double scale =
          (eq_scales.defined() && static_cast<std::int64_t>(i) < eq_scales.numel())
              ? eq_scales.select(0, static_cast<std::int64_t>(i)).item<double>()
              : 1.0;
      eq_weight_host[i] = equality_penalty * scale * scale *
                          eq_residual.select(0, static_cast<std::int64_t>(i)).item<double>();
    }
    const Series eq_weights =
        pluss::make_series(std::move(eq_weight_host), {static_cast<std::int64_t>(system.equalities.size())})
            .to(ScalarType::Float64)
            .to(platform);
    grad = add_weighted_row_jacobians_(grad, system.equalities, eq_weights);
  }
  if (!system.inequalities.empty()) {
    const Series ineq_residual = inequality_residual_series_(system, platform);
    if (ineq_residual.ge(0.0).any().item<bool>()) {
      return pluss::series::full({grad.numel()}, std::numeric_limits<double>::quiet_NaN(), ScalarType::Float64)
          .to(platform);
    }
    const Series ineq_weights = ineq_residual.reciprocal().mul(-barrier_parameter);
    grad = add_weighted_row_jacobians_(grad, system.inequalities, ineq_weights);
  }
  return grad.reshape({grad.numel()});
}

inline double merit_violation_(const FlatConstraintSystem& system, double margin) {
  double total = 0.0;
  for (const auto& row : system.inequalities) {
    total += std::max(row.residual + margin, 0.0);
  }
  return total;
}

inline ParameterPack<Series> make_strictly_interior_(const ConstrainedProblem& problem,
                                                     ParameterPack<Series> params,
                                                     double margin,
                                                     bool project_bounds,
                                                     std::int64_t& eval_count,
                                                     bool& ok,
                                                     bool skip_nonlinear = false) {
  ok = true;
  // With `skip_nonlinear`, nonlinear inequality rows are left alone: the caller gives them independent slacks.
  const auto row_counts = [skip_nonlinear](const FlatConstraintRow& row) {
    return !(skip_nonlinear && row.source == ConstraintSource::NonlinearInequality);
  };
  if (project_bounds) params = project_bounds_(params, problem.bounds);

  for (std::int64_t iter = 0; iter < 60; ++iter) {
    auto evaluation = evaluate_problem(problem, params);
    ++eval_count;
    if (evaluation.status != EvalStatus::Ok) {
      ok = false;
      return params;
    }
    const auto system = assemble_constraint_system_(problem, params, evaluation);
    double min_margin = std::numeric_limits<double>::infinity();
    for (const auto& row : system.inequalities) if (row_counts(row)) min_margin = std::min(min_margin, -row.residual);
    if (system.inequalities.empty() || min_margin >= margin * (1.0 - 1e-9)) {
      return params;
    }

    // Gauss-Newton phase-one step: the shortest move that removes the linearized violation of every
    // inequality that is not yet at least `margin` inside its boundary.
    const auto platform = params.empty() ? pluss::codem::Platform::CPU : params[0].value.platform();
    std::vector<Series> violated_rows;
    std::vector<double> violations;
    // Rows closer to their boundary than `margin` are moved to a more comfortable distance inside it, so
    // the first barrier steps are not choked by a slack of exactly `margin`.
    const double push = std::max(margin, 1e-2);
    for (const auto& row : system.inequalities) {
      if (!row_counts(row)) continue;
      if (!(row.residual + margin > 0.0)) continue;
      violated_rows.push_back(row.jacobian.to(ScalarType::Float64).to(platform));
      violations.push_back(row.residual + push);
    }
    const std::int64_t k = static_cast<std::int64_t>(violations.size());
    const std::int64_t n_vars = params.total_numel();
    const Series J = stack_rows_series_(violated_rows, n_vars, platform);
    const Series v = pluss::make_series(violations, {k}).to(ScalarType::Float64).to(platform);
    bool solved = false;
    Series multipliers = solve_dense_system_series_(
        J.matmul(J.transpose(0, 1)), v, 1e-12, OptimaLinearSystemSolver::Auto, 1e-12, solved);
    Series correction = solved ? J.transpose(0, 1).matmul(multipliers.reshape({k, 1})).reshape({n_vars})
                               : zeros_series_(n_vars, platform);
    if (!solved || series_inf_norm_(correction) <= 1e-12) {
      // Fall back to the gradient of the squared violation when the rows are degenerate.
      correction = zeros_series_(n_vars, platform);
      for (std::int64_t i = 0; i < k; ++i) {
        correction = correction.add(violated_rows[static_cast<std::size_t>(i)].mul(violations[static_cast<std::size_t>(i)]));
      }
    }
    if (series_inf_norm_(correction) <= 1e-12) {
      ok = false;
      return params;
    }

    const Series base_flat = flatten_values(params).to(ScalarType::Float64);
    const Series step_dir = correction.mul(-1.0);
    const auto violation_of = [&](const FlatConstraintSystem& s) {
      double total = 0.0;
      for (const auto& row : s.inequalities) if (row_counts(row)) total += std::max(row.residual + margin, 0.0);
      return total;
    };
    const double base_violation = violation_of(system);
    bool accepted = false;
    double step = 1.0;
    for (std::int64_t ls = 0; ls < 30; ++ls) {
      auto trial = unflatten_values(params.layout(), base_flat.add(step_dir.mul(step)));
      if (project_bounds) trial = project_bounds_(trial, problem.bounds);
      auto trial_eval = evaluate_problem(problem, trial);
      ++eval_count;
      if (trial_eval.status == EvalStatus::Ok) {
        const auto trial_system = assemble_constraint_system_(problem, trial, trial_eval);
        if (violation_of(trial_system) < base_violation) {
          params = std::move(trial);
          accepted = true;
          break;
        }
      }
      step *= 0.5;
    }
    if (!accepted) {
      ok = false;
      return params;
    }
  }

  ok = false;
  return params;
}

inline bool is_nonfinite_eval_status_(EvalStatus status) {
  return status == EvalStatus::NonFiniteObjective || status == EvalStatus::NonFiniteGradient ||
         status == EvalStatus::NonFiniteConstraint || status == EvalStatus::NonFiniteJacobian;
}

inline bool recover_nonfinite_initial_point_(const ConstrainedProblem& problem,
                                             const ParameterPack<Series>& seed_params,
                                             const InternalProtocolOptions& options,
                                             std::int64_t& eval_count,
                                             ParameterPack<Series>& recovered_params,
                                             ProblemEvaluation& recovered_eval) {
  if (seed_params.empty()) return false;
  const auto layout = seed_params.layout();
  ParameterPack<Series> seed = options.project_bounds ? project_bounds_(seed_params, problem.bounds) : seed_params;
  const Series seed_flat = flatten_values(seed).to(ScalarType::Float64);
  const Series zero_flat = pluss::series::zeros_like(seed_flat);
  const std::array<double, 14> scales = {
      1.0, 0.5, 0.25, 0.125, 0.0625, 0.03125, 0.015625,
      0.0078125, 0.00390625, 0.001953125, 1e-3, 1e-4, 1e-5, 1e-6};

  for (double scale : scales) {
    const Series trial_flat = seed_flat.mul(scale).add(zero_flat.mul(1.0 - scale));
    auto trial = unflatten_values(layout, trial_flat);
    if (options.project_bounds) trial = project_bounds_(trial, problem.bounds);
    auto trial_eval = evaluate_problem(problem, trial);
    ++eval_count;
    if (trial_eval.status == EvalStatus::Ok) {
      recovered_params = std::move(trial);
      recovered_eval = std::move(trial_eval);
      return true;
    }
  }
  return false;
}

inline void maybe_update_best_feasible_(InternalProtocolState& state,
                                        double constraint_tolerance = 0.0) {
  if (!feasible_with_tolerance_(state.evaluation, constraint_tolerance) ||
      !state.evaluation.value.defined() || state.evaluation.value.numel() != 1) {
    return;
  }
  if (!state.best_feasible.has_value() ||
      state.evaluation.value.item<double>() <= state.best_feasible->value.item<double>()) {
    state.best_feasible = BestFeasiblePoint{
        state.params,
        state.evaluation.value,
        state.evaluation.max_constraint_violation,
        state.evaluation.first_order_opt};
  }
}

inline bool should_enter_feasibility_phase_(const InternalProtocolState& state,
                                            const InternalProtocolOptions& options);

inline void push_barrier_history_(std::vector<double>& history,
                                  double barrier_value,
                                  std::size_t max_keep = 32);

inline void reduce_barrier_state_(InternalProtocolState& state,
                                  const FlatConstraintSystem& system,
                                  const InternalProtocolOptions& options) {
  if (!(state.barrier_parameter > options.minimum_barrier_parameter)) return;

  // Superlinear barrier schedule: shrink by barrier_decay, or faster once mu is small
  // (mu^1.5), but never below the minimum. The curvature model and the multipliers describe the
  // problem itself rather than one barrier level, so both are kept.
  state.barrier_parameter = std::max(
      options.minimum_barrier_parameter,
      std::min(state.barrier_parameter * options.barrier_decay, std::pow(state.barrier_parameter, 1.5)));
  state.inequality_dual = safeguard_inequality_dual_(
      current_or_barrier_dual_(state.inequality_dual, state.inequality_slack, state.barrier_parameter),
      state.inequality_slack,
      state.barrier_parameter);
  state.barrier_value = merit_value_(
      state.evaluation, system, state.barrier_parameter, state.normalization, state.merit_penalty, state.nl_slack);
  state.barrier_history.clear();
  push_barrier_history_(state.barrier_history, state.barrier_value);
  update_primal_dual_metrics_(state, system, state.barrier_parameter, state.normalization);
  state.in_feasibility_phase = should_enter_feasibility_phase_(state, options);
  state.message = "barrier reduced";
}

inline Series bfgs_hessian_update_(const Series& hessian,
                                   const Series& s,
                                   const Series& y,
                                                double curvature_tolerance) {
  const std::int64_t n = s.numel();
  if (!hessian.defined() || hessian.numel() != n * n || y.numel() != n) {
    ::pluss::throw_error("optim::bfgs_hessian_update_: shape mismatch");
  }

  const double sy = series_dot_(s, y);
  const double s_norm = s.to(ScalarType::Float64).square().sum().sqrt().item<double>();
  const double y_norm = y.to(ScalarType::Float64).square().sum().sqrt().item<double>();
  // Scale-free tests: an absolute floor (max(1, .)) would silently skip every
  // update once steps shrink below ~sqrt(tolerance), freezing the Hessian.
  if (!(sy > curvature_tolerance * s_norm * y_norm) || !(s_norm > 0.0) || !(y_norm > 0.0)) {
    return hessian;
  }

  const Series Bs = hessian.matmul(s.reshape({n, 1})).reshape({n});
  const double sBs = series_dot_(s, Bs);
  const double Bs_norm = Bs.to(ScalarType::Float64).square().sum().sqrt().item<double>();
  if (!(sBs > curvature_tolerance * s_norm * Bs_norm)) {
    return hessian;
  }

  return hessian.sub(series_outer_(Bs, Bs).div(sBs)).add(series_outer_(y, y).div(sy));
}

inline bool lbfgs_curvature_ok_(const Series& s,
                                const Series& y,
                                double curvature_tolerance) {
  const double sy = series_dot_(s, y);
  const double s_norm = s.to(ScalarType::Float64).square().sum().sqrt().item<double>();
  const double y_norm = y.to(ScalarType::Float64).square().sum().sqrt().item<double>();
  return sy > curvature_tolerance * s_norm * y_norm && s_norm > 0.0 && y_norm > 0.0;
}

inline void update_lbfgs_history_(std::vector<Series>& s_history,
                                  std::vector<Series>& y_history,
                                  const Series& s,
                                  const Series& y,
                                  std::int64_t memory,
                                  double curvature_tolerance) {
  if (memory <= 0) return;
  if (!lbfgs_curvature_ok_(s, y, curvature_tolerance)) return;
  if (static_cast<std::int64_t>(s_history.size()) >= memory) {
    s_history.erase(s_history.begin());
    y_history.erase(y_history.begin());
  }
  s_history.push_back(s.to(ScalarType::Float64));
  y_history.push_back(y.to(ScalarType::Float64));
}

inline Series lbfgs_direction_(const Series& gradient,
                               const std::vector<Series>& s_history,
                               const std::vector<Series>& y_history) {
  Series q = gradient.to(ScalarType::Float64);
  if (s_history.empty() || y_history.empty()) {
    return q.mul(-1.0);
  }

  const std::size_t m = std::min(s_history.size(), y_history.size());
  std::vector<double> alpha(m, 0.0);
  std::vector<double> rho(m, 0.0);

  for (std::size_t k = m; k-- > 0;) {
    const double sy = std::max(series_dot_(s_history[k], y_history[k]), 1e-18);
    rho[k] = 1.0 / sy;
    alpha[k] = rho[k] * series_dot_(s_history[k], q);
    q = q.sub(y_history[k].mul(alpha[k]));
  }

  const Series& s_last = s_history.back();
  const Series& y_last = y_history.back();
  const double yy = std::max(series_dot_(y_last, y_last), 1e-18);
  const double sy = std::max(series_dot_(s_last, y_last), 1e-18);
  Series r = q.mul(sy / yy);

  for (std::size_t k = 0; k < m; ++k) {
    const double beta = rho[k] * series_dot_(y_history[k], r);
    r = r.add(s_history[k].mul(alpha[k] - beta));
  }

  return r.mul(-1.0);
}

inline Series update_hessian_(
    const Series& current_hessian,
    const Series& s,
    const Series& y,
    double curvature_tolerance,
    OptimaHessianApproximation approximation,
    std::int64_t n,
    pluss::codem::Platform platform) {
  switch (approximation) {
    case OptimaHessianApproximation::Bfgs:
      return bfgs_hessian_update_(current_hessian, s, y, curvature_tolerance);
    case OptimaHessianApproximation::Identity:
      return identity_matrix_(n, platform);
    case OptimaHessianApproximation::Lbfgs:
      return current_hessian.defined() ? current_hessian : identity_matrix_(n, platform);
    case OptimaHessianApproximation::Exact:
    case OptimaHessianApproximation::HessianVector:
      return current_hessian.defined() ? current_hessian : identity_matrix_(n, platform);
  }
  return identity_matrix_(n, platform);
}

// Dense limited-memory BFGS approximation: start from the usual scaled identity and apply the stored
// pairs oldest to newest. The primal-dual step needs the Hessian approximation itself (it is combined
// with constraint terms), not just its inverse applied to a vector.
inline Series lbfgs_dense_hessian_(const std::vector<Series>& s_history,
                                   const std::vector<Series>& y_history,
                                   std::int64_t n,
                                   pluss::codem::Platform platform) {
  Series B = identity_matrix_(n, platform);
  const std::size_t m = std::min(s_history.size(), y_history.size());
  if (m == 0) return B;
  const double yy = series_dot_(y_history[m - 1], y_history[m - 1]);
  const double sy = series_dot_(s_history[m - 1], y_history[m - 1]);
  if (yy > 0.0 && sy > 0.0) B = B.mul(yy / sy);
  for (std::size_t k = 0; k < m; ++k) {
    B = bfgs_hessian_update_(B, s_history[k], y_history[k], 1e-12);
  }
  return B;
}

// BFGS update for a Lagrangian Hessian, which can be indefinite: when the curvature along the step is
// poor, blend the gradient change toward B s (Powell damping) so the update keeps B positive definite.
inline Series damped_bfgs_update_(const Series& hessian,
                                  const Series& s,
                                  const Series& y,
                                  double curvature_tolerance) {
  const std::int64_t n = s.numel();
  const Series Bs = hessian.matmul(s.reshape({n, 1})).reshape({n});
  const double sBs = series_dot_(s, Bs);
  const double sy = series_dot_(s, y);
  Series y_used = y.to(ScalarType::Float64);
  if (sBs > 0.0 && sy < 0.2 * sBs) {
    const double theta = 0.8 * sBs / (sBs - sy);
    y_used = y_used.mul(theta).add(Bs.mul(1.0 - theta));
  }
  return bfgs_hessian_update_(hessian, s, y_used, curvature_tolerance);
}

// Solves min 0.5 d'H d + g'd subject to J d = -c exactly through the indefinite KKT system
// [H + shift*I, J'; J, 0] [d; y] = [-g; -c] with a pivoted LU factorization. The shift is applied to the
// Hessian block only, so the constraint rows stay exact; returns false when the system is singular.
inline bool solve_equality_kkt_dense_(const Series& hessian,
                                      const Series& gradient,
                                      const std::vector<FlatConstraintRow>& rows,
                                      double shift,
                                      Series& step,
                                      Series& multipliers) {
  const std::int64_t n = gradient.numel();
  const std::int64_t m = static_cast<std::int64_t>(rows.size());
  const auto platform = gradient.platform();
  if (platform != pluss::codem::Platform::CPU || n + m > 512) return false;
  try {
    const Series Hreg = hessian.add(identity_matrix_(n, platform).mul(std::max(shift, 0.0)));
    std::vector<Series> jac_rows;
    jac_rows.reserve(rows.size());
    for (const auto& row : rows) jac_rows.push_back(row.jacobian.to(platform));
    const Series A = stack_rows_series_(jac_rows, n, platform);
    const Series top = Hreg.unite({A.transpose(0, 1)}, 1);
    const Series bottom = A.unite({pluss::series::zeros({m, m}, ScalarType::Float64).to(platform)}, 1);
    const Series KKT = top.unite({bottom}, 0);
    std::vector<double> tail(static_cast<std::size_t>(m), 0.0);
    for (std::int64_t i = 0; i < m; ++i) tail[static_cast<std::size_t>(i)] = -rows[static_cast<std::size_t>(i)].residual;
    const Series rhs = gradient.mul(-1.0).unite(
        {pluss::make_series(std::move(tail), {m}).to(ScalarType::Float64).to(platform)}, 0);
    const Series solution =
        pluss::subsp::math::linalg::lu_solve(rhs.reshape({n + m, 1}), KKT).reshape({n + m}).to(ScalarType::Float64);
    if (!is_finite_series_(solution)) return false;
    step = solution.slice(0, 0, n, 1).reshape({n});
    multipliers = solution.slice(0, n, n + m, 1).reshape({m});
    return true;
  } catch (...) {
    return false;
  }
}

inline std::pair<Series, Series> solve_sqp_kkt_(
    const Series& hessian,
    const Series& gradient,
    const std::vector<FlatConstraintRow>& active_constraints,
    double regularization,
    OptimaLinearSystemSolver linear_system_solver,
    std::int64_t max_krylov_iterations,
    double krylov_tolerance,
    double krylov_absolute_tolerance,
    double pivot_threshold,
    std::int64_t& krylov_iterations,
    bool& ok) {
  const std::int64_t n = gradient.numel();
  const std::int64_t m = static_cast<std::int64_t>(active_constraints.size());
  const auto platform = gradient.platform();
  const Series Hreg = hessian.add(identity_matrix_(n, platform).mul(regularization));
  krylov_iterations = 0;

  if (m == 0) {
    Series step = pluss::series::undefined();
    if (linear_system_solver == OptimaLinearSystemSolver::ConjugateGradient) {
      step = solve_spd_system_cg_series_(
          Hreg,
          gradient.mul(-1.0),
          regularization,
          max_krylov_iterations,
          krylov_tolerance,
          krylov_absolute_tolerance,
          ok,
          krylov_iterations);
    } else {
      step =
          solve_dense_system_series_(
              Hreg, gradient.mul(-1.0), regularization, linear_system_solver, pivot_threshold, ok);
    }
    return {step, zeros_series_(0, platform)};
  }

  if (linear_system_solver != OptimaLinearSystemSolver::ConjugateGradient) {
    // Exact KKT solve: the regularization above lives on the Hessian block only, so the constraint
    // rows are satisfied to machine precision instead of to the size of the shift.
    Series exact_step = pluss::series::undefined();
    Series exact_multipliers = pluss::series::undefined();
    if (solve_equality_kkt_dense_(Hreg, gradient, active_constraints, 0.0, exact_step, exact_multipliers)) {
      ok = true;
      return {exact_step, exact_multipliers};
    }
  }

  std::vector<Series> rows;
  rows.reserve(active_constraints.size());
  for (const auto& row : active_constraints) rows.push_back(row.jacobian.to(platform));
  const Series A = stack_rows_series_(rows, n, platform);
  const Series top = Hreg.unite({A.transpose(0, 1)}, 1);
  const Series bottom = A.unite({pluss::series::zeros({m, m}, ScalarType::Float64).to(platform)}, 1);
  const Series KKT = top.unite({bottom}, 0);

  std::vector<double> rhs_tail(static_cast<std::size_t>(m), 0.0);
  for (std::int64_t i = 0; i < m; ++i) rhs_tail[static_cast<std::size_t>(i)] = -active_constraints[static_cast<std::size_t>(i)].residual;
  Series rhs = gradient.mul(-1.0).unite(
      {pluss::make_series(std::move(rhs_tail), {m}).to(ScalarType::Float64).to(platform)},
      0);

  Series solution = pluss::series::undefined();
  if (linear_system_solver == OptimaLinearSystemSolver::ConjugateGradient) {
    const Series normal_rhs = dense_transpose_matvec_(KKT, rhs);
    const Series normal_diag =
        KKT.abs().sum(0).reshape({n + m}).maximum(1e-8).square().add(std::max(regularization, 1e-8));
    auto normal_matvec = [&](const Series& direction) -> Series {
      return dense_transpose_matvec_(KKT, dense_matvec_(KKT, direction)).add(direction.mul(std::max(regularization, 1e-8)));
    };
    solution = solve_spd_system_cg_operator_(
        normal_matvec,
        normal_rhs,
        normal_diag,
        max_krylov_iterations,
        krylov_tolerance,
        krylov_absolute_tolerance,
        ok,
        krylov_iterations);
    if (!ok) {
      // The normal equations square the conditioning, and conjugate gradients may not reach the tolerance within
      // the budget. A direct solve of the same system is the fallback, so choosing conjugate gradients can never
      // turn a solvable step into a failed one (which would otherwise degrade to steepest descent).
      solution = solve_dense_system_series_(KKT, rhs, regularization, OptimaLinearSystemSolver::Auto, pivot_threshold, ok);
    }
  } else {
    solution =
        solve_dense_system_series_(KKT, rhs, regularization, linear_system_solver, pivot_threshold, ok);
  }
  if (!ok) {
    return {zeros_series_(n, platform), zeros_series_(m, platform)};
  }
  return {
      solution.index({pluss::series::arange(0, n, 1, ScalarType::Int64).to(platform)}).reshape({n}),
      solution.index({pluss::series::arange(n, n + m, 1, ScalarType::Int64).to(platform)}).reshape({m})};
}

inline double kkt_residual_(const Series& gradient,
                            const std::vector<FlatConstraintRow>& active_constraints,
                            const Series& multipliers) {
  Series lag_grad = gradient.to(ScalarType::Float64).reshape({gradient.numel()});
  double multiplier_violation = 0.0;
  const Series lambda = multipliers.defined() ? multipliers.to(ScalarType::Float64).reshape({multipliers.numel()}) : pluss::series::undefined();
  if (!active_constraints.empty() && lambda.defined() && lambda.numel() > 0) {
    const std::int64_t m = std::min<std::int64_t>(static_cast<std::int64_t>(active_constraints.size()), lambda.numel());
    if (m > 0) {
      const Series lambda_used = lambda.slice(0, 0, m, 1).reshape({m});
      lag_grad = add_weighted_row_jacobians_(lag_grad, active_constraints, lambda_used, m);

      std::vector<bool> ineq_mask_host(static_cast<std::size_t>(m), false);
      for (std::int64_t i = 0; i < m; ++i) {
        ineq_mask_host[static_cast<std::size_t>(i)] =
            !active_constraints[static_cast<std::size_t>(i)].equality;
      }
      const Series ineq_mask = pluss::make_series(std::move(ineq_mask_host), {m})
                                   .to(ScalarType::Bool)
                                   .to(lambda_used.platform());
      const Series zeros = pluss::series::zeros_like(lambda_used, ScalarType::Float64).to(lambda_used.platform());
      const Series ineq_lambdas = ineq_mask.where(lambda_used, zeros);
      multiplier_violation = std::max(0.0, -ineq_lambdas.amin(0, false).item<double>());
    }
  }
  return std::max(series_inf_norm_(lag_grad), multiplier_violation);
}

inline std::pair<double, std::size_t> most_negative_inequality_multiplier_(
    const std::vector<FlatConstraintRow>& active_constraints,
    const Series& multipliers) {
  double most_negative = 0.0;
  std::size_t most_negative_index = active_constraints.size();
  const Series lambda = multipliers.defined() ? multipliers.to(ScalarType::Float64).reshape({multipliers.numel()}) : pluss::series::undefined();
  for (std::size_t i = 0; i < active_constraints.size(); ++i) {
    if (active_constraints[i].equality) continue;
    const double li = series_scalar_at_or_(lambda, i, 0.0);
    if (li < most_negative) {
      most_negative = li;
      most_negative_index = i;
    }
  }
  return {most_negative, most_negative_index};
}

inline Series sized_multiplier_series_(std::int64_t n, pluss::codem::Platform platform) {
  return n > 0 ? zeros_series_(n, platform) : pluss::series::empty({0}, ScalarType::Float64).to(platform);
}

inline ConstraintMultipliers zero_constraint_multipliers_(const ConstrainedProblem& problem,
                                                          const ParameterPack<Series>& params,
                                                          const ProblemEvaluation& evaluation) {
  const auto platform =
      params.empty() ? pluss::codem::Platform::CPU : params[0].value.platform();
  const std::int64_t n_flat = params.total_numel();
  const std::int64_t n_nonlinear_eq =
      evaluation.nonlinear_equality_residual.defined() ? evaluation.nonlinear_equality_residual.numel() : 0;
  const std::int64_t n_nonlinear_ineq =
      evaluation.nonlinear_inequality_residual.defined() ? evaluation.nonlinear_inequality_residual.numel() : 0;
  return ConstraintMultipliers{
      sized_multiplier_series_(problem.linear.Aeq.has_value() ? problem.linear.Aeq->sizes()[0] : 0, platform),
      sized_multiplier_series_(n_nonlinear_eq, platform),
      sized_multiplier_series_(problem.linear.Aineq.has_value() ? problem.linear.Aineq->sizes()[0] : 0, platform),
      sized_multiplier_series_(n_nonlinear_ineq, platform),
      sized_multiplier_series_(problem.bounds.lower.has_value() ? n_flat : 0, platform),
      sized_multiplier_series_(problem.bounds.upper.has_value() ? n_flat : 0, platform)};
}

inline void scatter_multiplier_value_(Series* target, std::int64_t index, double value) {
  if (target == nullptr || !target->defined() || target->numel() == 0) return;
  if (index < 0 || index >= target->numel()) return;
  const auto platform = target->platform();
  const Series idx = pluss::make_series(std::vector<std::int64_t>{index}, {1})
                         .to(ScalarType::Int64)
                         .to(platform);
  const Series val = pluss::make_series(std::vector<double>{value}, {1})
                         .to(ScalarType::Float64)
                         .to(platform);
  *target = target->index_put({idx}, val, false).to(ScalarType::Float64).to(platform);
}

inline ConstraintMultipliers gather_constraint_multipliers_(
    const ConstrainedProblem& problem,
    const ParameterPack<Series>& params,
    const ProblemEvaluation& evaluation,
    const std::vector<FlatConstraintRow>& active_constraints,
    const Series& active_multipliers) {
  ConstraintMultipliers out = zero_constraint_multipliers_(problem, params, evaluation);
  if (!active_multipliers.defined() || active_multipliers.numel() == 0) return out;
  const Series lambda = active_multipliers.to(ScalarType::Float64).reshape({active_multipliers.numel()});

  for (std::size_t i = 0; i < active_constraints.size(); ++i) {
    const auto& row = active_constraints[i];
    const double value = series_scalar_at_or_(lambda, i, 0.0);
    switch (row.source) {
      case ConstraintSource::LinearEquality:
        scatter_multiplier_value_(&out.linear_equalities, row.index, value);
        break;
      case ConstraintSource::NonlinearEquality:
        scatter_multiplier_value_(&out.nonlinear_equalities, row.index, value);
        break;
      case ConstraintSource::LinearInequality:
        scatter_multiplier_value_(&out.linear_inequalities, row.index, std::max(0.0, value));
        break;
      case ConstraintSource::NonlinearInequality:
        scatter_multiplier_value_(&out.nonlinear_inequalities, row.index, std::max(0.0, value));
        break;
      case ConstraintSource::LowerBound:
        scatter_multiplier_value_(&out.lower_bounds, row.index, std::max(0.0, value));
        break;
      case ConstraintSource::UpperBound:
        scatter_multiplier_value_(&out.upper_bounds, row.index, std::max(0.0, value));
        break;
    }
  }
  return out;
}

inline ConstraintMultipliers estimate_barrier_multipliers_(
    const ConstrainedProblem& problem,
    const ParameterPack<Series>& params,
    const ProblemEvaluation& evaluation,
    const FlatConstraintSystem& system,
    double barrier_parameter) {
  ConstraintMultipliers out = zero_constraint_multipliers_(problem, params, evaluation);
  for (const auto& row : system.inequalities) {
    if (!(row.residual < 0.0)) continue;
    const double value = std::max(0.0, barrier_parameter / (-row.residual));
    switch (row.source) {
      case ConstraintSource::LinearInequality:
        scatter_multiplier_value_(&out.linear_inequalities, row.index, value);
        break;
      case ConstraintSource::NonlinearInequality:
        scatter_multiplier_value_(&out.nonlinear_inequalities, row.index, value);
        break;
      case ConstraintSource::LowerBound:
        scatter_multiplier_value_(&out.lower_bounds, row.index, value);
        break;
      case ConstraintSource::UpperBound:
        scatter_multiplier_value_(&out.upper_bounds, row.index, value);
        break;
      case ConstraintSource::LinearEquality:
      case ConstraintSource::NonlinearEquality:
        break;
    }
  }
  return out;
}

inline ConstraintMultipliers gather_primal_dual_inequality_multipliers_(
    const ConstrainedProblem& problem,
    const ParameterPack<Series>& params,
    const ProblemEvaluation& evaluation,
    const FlatConstraintSystem& system,
    const Series& inequality_dual) {
  ConstraintMultipliers out = zero_constraint_multipliers_(problem, params, evaluation);
  if (!inequality_dual.defined() || inequality_dual.numel() == 0) return out;
  const Series dual = inequality_dual.to(ScalarType::Float64).reshape({inequality_dual.numel()});

  for (std::size_t i = 0; i < system.inequalities.size(); ++i) {
    const auto& row = system.inequalities[i];
    const double value = std::max(0.0, series_scalar_at_or_(dual, i, 0.0));
    switch (row.source) {
      case ConstraintSource::LinearInequality:
        scatter_multiplier_value_(&out.linear_inequalities, row.index, value);
        break;
      case ConstraintSource::NonlinearInequality:
        scatter_multiplier_value_(&out.nonlinear_inequalities, row.index, value);
        break;
      case ConstraintSource::LowerBound:
        scatter_multiplier_value_(&out.lower_bounds, row.index, value);
        break;
      case ConstraintSource::UpperBound:
        scatter_multiplier_value_(&out.upper_bounds, row.index, value);
        break;
      case ConstraintSource::LinearEquality:
      case ConstraintSource::NonlinearEquality:
        break;
    }
  }
  return out;
}

inline Series estimate_kkt_multipliers_(const Series& gradient,
                                        const std::vector<FlatConstraintRow>& active_constraints) {
  const auto platform = gradient.platform();
  const std::int64_t m = static_cast<std::int64_t>(active_constraints.size());
  if (m == 0) return sized_multiplier_series_(0, platform);

  std::vector<Series> rows;
  rows.reserve(active_constraints.size());
  for (const auto& row : active_constraints) rows.push_back(row.jacobian.to(platform));
  const Series A = stack_rows_series_(rows, gradient.numel(), platform);
  const Series AAt = A.matmul(A.transpose(0, 1));
  const Series rhs = A.matmul(gradient.reshape({gradient.numel(), 1})).reshape({m}).mul(-1.0);

  bool ok = false;
  Series lambda = solve_dense_system_series_(
      AAt,
      rhs,
      1e-8,
      OptimaLinearSystemSolver::Auto,
      1e-12,
      ok);
  if (!ok) return sized_multiplier_series_(m, platform);
  return lambda;
}

inline const char* algorithm_name_(OptimaAlgorithm algorithm) {
  switch (algorithm) {
    case OptimaAlgorithm::MeritMethod:
      return "merit-method";
    case OptimaAlgorithm::SequentialQP:
      return "sqp";
    case OptimaAlgorithm::WorkingSet:
      return "working-set";
    case OptimaAlgorithm::InternalProtocol:
      return "internal-protocol";
  }
  return "unknown";
}

inline bool run_progress_hook_(const std::function<bool(const OptimaProgressSnapshot&)>& progress_hook,
                               const OptimaProgressSnapshot& snapshot,
                               std::string* failure_message = nullptr) {
  if (!progress_hook) return false;
  try {
    return progress_hook(snapshot);
  } catch (...) {
    if (failure_message != nullptr) {
      *failure_message = callback_failure_message_("progress hook");
    }
    return true;
  }
}

inline bool feasible_objective_target_met_(const ProblemEvaluation& evaluation, double objective_limit) {
  if (!std::isfinite(objective_limit) || !evaluation.feasible || !evaluation.value.defined() ||
      evaluation.status != EvalStatus::Ok) {
    return false;
  }
  const double value = evaluation.value.item<double>();
  return std::isfinite(value) && value <= objective_limit;
}

struct OptimaStallDiagnostic {
  OptimaStallCategory category{OptimaStallCategory::None};
  double measure{0.0};
  double reference{0.0};
};

inline OptimaStallDiagnostic classify_stall_(OptimaAlgorithm algorithm,
                                             EvalStatus status,
                                             const std::string& message,
                                             double step_size,
                                             double stationarity,
                                             double complementarity) {
  auto make = [](OptimaStallCategory category, double measure, double reference) {
    return OptimaStallDiagnostic{category, measure, reference};
  };

  if (status == EvalStatus::InvalidProblem) {
    return make(OptimaStallCategory::InvalidCallbackContract, 0.0, 0.0);
  }
  if (status == EvalStatus::NonFiniteObjective || status == EvalStatus::NonFiniteGradient ||
      status == EvalStatus::NonFiniteConstraint || status == EvalStatus::NonFiniteJacobian) {
    return make(OptimaStallCategory::NonFiniteCallbackRegion, 0.0, 0.0);
  }
  if (message.find("progress hook") != std::string::npos) {
    return make(OptimaStallCategory::UserStop, 0.0, 0.0);
  }
  if (message.find("constraint violation locally stationary") != std::string::npos) {
    return make(OptimaStallCategory::InfeasibleStationary, stationarity, 0.0);
  }
  if (message.find("feasibility phase failed") != std::string::npos ||
      message.find("feasibility phase reached a stationary violation point") != std::string::npos) {
    return make(OptimaStallCategory::FeasibilityFailure, stationarity, 0.0);
  }
  if (message.find("restoration failed") != std::string::npos ||
      message.find("barrier recovery failed") != std::string::npos ||
      message.find("recovery requires") != std::string::npos ||
      message.find("line search failed to produce an acceptable SQP step after restoration") != std::string::npos ||
      message.find("line search failed to produce an acceptable internal-protocol step after recovery") !=
          std::string::npos) {
    return make(OptimaStallCategory::RecoveryFailure, step_size, 0.0);
  }
  if (message.find("bound-feasible region") != std::string::npos ||
      message.find("step hit the boundary") != std::string::npos) {
    return make(OptimaStallCategory::BoundaryLimited, std::abs(step_size), 0.0);
  }
  if (message.find("line search failed") != std::string::npos) {
    return make(OptimaStallCategory::SearchDirectionCollapse, std::abs(step_size), 0.0);
  }
  if (message.find("maximum iterations reached") != std::string::npos) {
    if (algorithm == OptimaAlgorithm::InternalProtocol &&
        complementarity > std::max(1e-12, 1e-2 * std::max(stationarity, 1e-12))) {
      return make(OptimaStallCategory::ComplementarityStagnation, complementarity,
                  std::max(stationarity, 1e-12));
    }
    return make(OptimaStallCategory::IterationLimit, stationarity, std::abs(step_size));
  }
  return {};
}

inline std::string append_stall_detail_(std::string detail_message,
                                        const OptimaStallDiagnostic& diagnostic) {
  if (diagnostic.category == OptimaStallCategory::None) return detail_message;

  std::ostringstream oss;
  oss.setf(std::ios::scientific);
  oss.precision(3);
  oss << " diagnostic=" << optima_stall_category_name(diagnostic.category)
      << " measure=" << diagnostic.measure
      << " reference=" << diagnostic.reference << ".";
  const std::string suffix = oss.str();
  if (detail_message.find(suffix) != std::string::npos) return detail_message;
  if (detail_message.empty()) return suffix.substr(1);
  detail_message += suffix;
  return detail_message;
}

inline OptimaProgressSnapshot make_progress_snapshot_(OptimaAlgorithm algorithm,
                                                      OptimaProgressStage stage,
                                                      const PenaltyMethodState& state) {
  const auto diagnostic =
      classify_stall_(algorithm, state.evaluation.status, state.message, state.last_step_size,
                      pack_inf_norm_(state.evaluation.gradient), 0.0);
  return OptimaProgressSnapshot{
      stage,
      algorithm_name_(algorithm),
      state.params,
      state.evaluation,
      state.iteration,
      state.function_evaluations,
      state.last_step_size,
      state.current_penalty,
      std::isfinite(state.stationarity) ? state.stationarity : pack_inf_norm_(state.evaluation.gradient),
      0.0,
      0,
      state.message,
      diagnostic.category,
      diagnostic.measure,
      diagnostic.reference,
      state.best_feasible};
}

inline OptimaProgressSnapshot make_progress_snapshot_(OptimaAlgorithm algorithm,
                                                      OptimaProgressStage stage,
                                                      const SequentialQPState& state) {
  const auto diagnostic =
      classify_stall_(algorithm, state.evaluation.status, state.message, state.last_step_size,
                      state.kkt_residual, 0.0);
  return OptimaProgressSnapshot{
      stage,
      algorithm_name_(algorithm),
      state.params,
      state.evaluation,
      state.iteration,
      state.function_evaluations,
      state.last_step_size,
      state.current_penalty,
      state.kkt_residual,
      0.0,
      state.krylov_iterations,
      state.message,
      diagnostic.category,
      diagnostic.measure,
      diagnostic.reference,
      state.best_feasible};
}

inline OptimaProgressSnapshot make_progress_snapshot_(OptimaAlgorithm algorithm,
                                                      OptimaProgressStage stage,
                                                      const InternalProtocolState& state) {
  const double reported_complementarity =
      state.iteration <= 1
          ? std::max(state.complementarity_residual, std::max(state.barrier_parameter, 0.0))
          : state.complementarity_residual;
  const auto diagnostic =
      classify_stall_(algorithm, state.evaluation.status, state.message, state.last_step_size,
                      state.barrier_gradient_norm, reported_complementarity);
  return OptimaProgressSnapshot{
      stage,
      algorithm_name_(algorithm),
      state.params,
      state.evaluation,
      state.iteration,
      state.function_evaluations,
      state.last_step_size,
      state.barrier_parameter,
      state.barrier_gradient_norm,
      reported_complementarity,
      state.krylov_iterations,
      state.message,
      diagnostic.category,
      diagnostic.measure,
      diagnostic.reference,
      state.best_feasible};
}

inline OptimaExitFlag exit_flag_from_eval_status_(EvalStatus status) {
  switch (status) {
    case EvalStatus::Ok:
      return OptimaExitFlag::Stopped;
    case EvalStatus::NonFiniteObjective:
    case EvalStatus::NonFiniteGradient:
    case EvalStatus::NonFiniteConstraint:
    case EvalStatus::NonFiniteJacobian:
      return OptimaExitFlag::NonFiniteEvaluation;
    case EvalStatus::InvalidProblem:
      return OptimaExitFlag::InvalidProblem;
  }
  return OptimaExitFlag::InvalidProblem;
}

inline OptimaExitFlag exit_flag_from_state_(bool converged,
                                            const std::string& message,
                                            EvalStatus eval_status) {
  if (eval_status != EvalStatus::Ok) {
    return exit_flag_from_eval_status_(eval_status);
  }
  if (message.find("progress hook") != std::string::npos) {
    return OptimaExitFlag::Stopped;
  }
  if (converged) {
    if (message.find("objective limit reached") != std::string::npos) {
      return OptimaExitFlag::ObjectiveTargetReached;
    }
    if (message.find("maximum iterations reached") != std::string::npos) {
      return OptimaExitFlag::MaxIterations;
    }
    if (message.find("constraint violation locally stationary") != std::string::npos) {
      return OptimaExitFlag::ConstraintStationary;
    }
    if (message.find("failed to initialize a strict internal protocol") != std::string::npos) {
      return OptimaExitFlag::NoFeasiblePoint;
    }
    if (message.find("restoration failed") != std::string::npos ||
        message.find("barrier recovery failed") != std::string::npos ||
        message.find("recovery requires") != std::string::npos ||
        message.find("feasibility phase failed") != std::string::npos ||
        message.find("feasibility phase reached a stationary violation point") != std::string::npos ||
        message.find("working set failed to recover a feasible step") != std::string::npos) {
      return OptimaExitFlag::NoProgress;
    }
    if (message.find("line search failed") != std::string::npos ||
        message.find("step hit the boundary") != std::string::npos ||
        message.find("bound-feasible region") != std::string::npos ||
        message.find("no progress") != std::string::npos ||
        message.find("working set cannot produce a bound-feasible step") != std::string::npos) {
      return OptimaExitFlag::NoProgress;
    }
    return OptimaExitFlag::Converged;
  }
  return OptimaExitFlag::Stopped;
}

inline double multiplier_max_norm_(const ConstraintMultipliers& multipliers) {
  return std::max({max_abs_scalar_(multipliers.linear_equalities),
                   max_abs_scalar_(multipliers.nonlinear_equalities),
                   max_abs_scalar_(multipliers.linear_inequalities),
                   max_abs_scalar_(multipliers.nonlinear_inequalities),
                   max_abs_scalar_(multipliers.lower_bounds),
                   max_abs_scalar_(multipliers.upper_bounds)});
}

inline std::int64_t active_multiplier_count_(const ConstraintMultipliers& multipliers,
                                             double tolerance = 1e-10) {
  return count_active_entries_(multipliers.linear_equalities, tolerance) +
         count_active_entries_(multipliers.nonlinear_equalities, tolerance) +
         count_active_entries_(multipliers.linear_inequalities, tolerance) +
         count_active_entries_(multipliers.nonlinear_inequalities, tolerance) +
         count_active_entries_(multipliers.lower_bounds, tolerance) +
         count_active_entries_(multipliers.upper_bounds, tolerance);
}

inline bool returned_best_feasible_(const ProblemEvaluation& evaluation,
                                    const std::optional<BestFeasiblePoint>& best_feasible,
                                    double value_tolerance = 1e-9,
                                    double violation_tolerance = 1e-12) {
  if (!best_feasible.has_value() || evaluation.status != EvalStatus::Ok || !evaluation.value.defined() ||
      evaluation.value.numel() != 1) {
    return false;
  }
  const double current_value = evaluation.value.item<double>();
  return std::abs(current_value - best_feasible->value.item<double>()) <= value_tolerance &&
         evaluation.max_constraint_violation <= best_feasible->max_constraint_violation + violation_tolerance &&
         std::abs(evaluation.max_constraint_violation - best_feasible->max_constraint_violation) <=
             std::max(violation_tolerance, 1e-9);
}

inline std::string augment_detail_message_(std::string detail_message,
                                           const ProblemEvaluation& evaluation,
                                           const std::optional<BestFeasiblePoint>& best_feasible,
                                           bool returned_best_feasible) {
  if (!best_feasible.has_value()) return detail_message;
  if (returned_best_feasible) return detail_message;

  const std::string suffix =
      " best feasible point retained separately from the returned iterate.";
  if (detail_message.find(suffix) != std::string::npos) return detail_message;
  if (detail_message.empty()) return "best feasible point retained separately from the returned iterate.";
  detail_message += suffix;
  return detail_message;
}

inline OptimaOutput make_output_(OptimaAlgorithm algorithm,
                                 OptimaExitFlag exit_flag,
                                 const ProblemEvaluation& evaluation,
                                 std::int64_t iteration,
                                 std::int64_t function_evaluations,
                                 double last_step_size,
                                 double backend_parameter,
                                 double stationarity,
                                 double complementarity,
                                 const ConstraintMultipliers& multipliers,
                                 const std::optional<BestFeasiblePoint>& best_feasible,
                                 const std::string& detail_message,
                                 std::int64_t krylov_iterations = 0) {
  const bool returned_best_feasible = returned_best_feasible_(evaluation, best_feasible);
  const auto diagnostic =
      classify_stall_(algorithm, evaluation.status, detail_message, last_step_size, stationarity, complementarity);
  std::string public_detail_message =
      augment_detail_message_(detail_message, evaluation, best_feasible, returned_best_feasible);
  if (exit_flag != OptimaExitFlag::Converged && exit_flag != OptimaExitFlag::ObjectiveTargetReached) {
    public_detail_message = append_stall_detail_(std::move(public_detail_message), diagnostic);
  }
  return OptimaOutput{
      algorithm_name_(algorithm),
      iteration,
      function_evaluations,
      evaluation.max_constraint_violation,
      stationarity,
      last_step_size,
      last_step_size,
      backend_parameter,
      complementarity,
      krylov_iterations,
      best_feasible.has_value(),
      returned_best_feasible,
      best_feasible.has_value() ? best_feasible->value.item<double>() : std::numeric_limits<double>::infinity(),
      best_feasible.has_value() ? best_feasible->max_constraint_violation
                                : std::numeric_limits<double>::infinity(),
      best_feasible.has_value() ? best_feasible->first_order_opt : std::numeric_limits<double>::infinity(),
      multiplier_max_norm_(multipliers),
      active_multiplier_count_(multipliers),
      public_detail_message,
      diagnostic.category,
      diagnostic.measure,
      diagnostic.reference};
}

inline ParameterPack<Series> project_bounds_(const ParameterPack<Series>& params, const Bounds& bounds) {
  if (bounds.empty()) return params;
  std::vector<typename ParameterPack<Series>::Block> out;
  out.reserve(params.size());
  for (const auto& block : params.blocks()) {
    Series value = block.value.detach();
    if (bounds.lower.has_value()) {
      value = value.maximum(bounds.lower->at(block.name));
    }
    if (bounds.upper.has_value()) {
      value = value.minimum(bounds.upper->at(block.name));
    }
    out.push_back({block.name, std::move(value)});
  }
  return ParameterPack<Series>(std::move(out));
}

inline ParameterPack<Series> combine_constraint_gradient_(
    const ParameterPack<Series>& params,
    const ParameterPack<Series>& jacobian,
    const Series& residual,
    bool equality_term) {
  (void)equality_term;
  if (jacobian.empty() || !residual.defined() || residual.numel() == 0) {
    return params.zeros_like();
  }
  const Series r = residual.to(ScalarType::Float64).reshape({residual.numel(), 1});
  std::vector<typename ParameterPack<Series>::Block> out;
  out.reserve(params.size());
  for (const auto& param_block : params.blocks()) {
    Series contribution = pluss::series::zeros_like(param_block.value, param_block.value.scalar_type());
    if (jacobian.contains(param_block.name)) {
      const Series& block_jacobian = jacobian.at(param_block.name);
      if (block_jacobian.defined() && block_jacobian.numel() > 0) {
        const std::int64_t cols = param_block.value.numel();
        const std::int64_t rows = residual.numel();
        if (block_jacobian.numel() != rows * cols) {
          ::pluss::throw_error(
              "optim::combine_constraint_gradient_: jacobian/residual shape mismatch");
        }
        const auto platform = param_block.value.platform();
        contribution = block_jacobian.to(ScalarType::Float64)
                           .to(platform)
                           .reshape({rows, cols})
                           .transpose(0, 1)
                           .matmul(r.to(platform))
                           .reshape(param_block.value.sizes())
                           .to(param_block.value.scalar_type());
      }
    }
    if (contribution.platform() != param_block.value.platform()) {
      contribution = contribution.to(param_block.value.platform());
    }
    out.push_back({param_block.name, std::move(contribution)});
  }
  return ParameterPack<Series>(std::move(out));
}

inline double squared_l2_(const Series& x) {
  if (!x.defined() || x.numel() == 0) return 0.0;
  return x.to(ScalarType::Float64).square().sum().item<double>();
}

inline double penalized_value_(const ProblemEvaluation& eval, double penalty) {
  const double base = eval.value.item<double>();
  const double eq_term = 0.5 * squared_l2_(eval.equality_residual);
  const Series ineq_pos = eval.inequality_residual.numel() == 0
                              ? eval.inequality_residual
                              : eval.inequality_residual.clamp_min(0.0);
  const double ineq_term = 0.5 * squared_l2_(ineq_pos);
  return base + penalty * (eq_term + ineq_term);
}

inline double penalized_value_(const ProblemEvaluation& eval,
                               double penalty,
                               const OptimaNormalizationStats& normalization) {
  const double base = normalization.objective_scale * eval.value.item<double>();
  const Series eq_scaled =
      eval.equality_residual.numel() == 0 || !normalization.equality_row_scale.defined()
          ? eval.equality_residual
          : eval.equality_residual.to(ScalarType::Float64).mul(normalization.equality_row_scale.to(eval.equality_residual.platform()));
  const Series ineq_pos = eval.inequality_residual.numel() == 0
                              ? eval.inequality_residual
                              : eval.inequality_residual.clamp_min(0.0);
  const Series ineq_scaled =
      ineq_pos.numel() == 0 || !normalization.inequality_row_scale.defined()
          ? ineq_pos
          : ineq_pos.to(ScalarType::Float64).mul(normalization.inequality_row_scale.to(ineq_pos.platform()));
  const double eq_term = 0.5 * squared_l2_(eq_scaled);
  const double ineq_term = 0.5 * squared_l2_(ineq_scaled);
  return base + penalty * (eq_term + ineq_term);
}

inline void maybe_update_best_feasible_(PenaltyMethodState& state,
                                        double constraint_tolerance = 0.0) {
  if (!feasible_with_tolerance_(state.evaluation, constraint_tolerance) ||
      !state.evaluation.value.defined() || state.evaluation.value.numel() != 1) {
    return;
  }
  if (!state.best_feasible.has_value() ||
      state.evaluation.value.item<double>() <= state.best_feasible->value.item<double>()) {
    state.best_feasible = BestFeasiblePoint{
        state.params,
        state.evaluation.value,
        state.evaluation.max_constraint_violation,
        state.evaluation.first_order_opt};
  }
}

} // namespace detail

inline ProblemEvaluation evaluate_problem(const ConstrainedProblem& problem,
                                          const ParameterPack<Series>& incoming_params) {
  // Evaluate at the values only. A parameter that carries history from earlier arithmetic (for example
  // a point built as x + step) would otherwise tie its derivatives to that history and lose the
  // derivative with respect to itself.
  std::vector<typename ParameterPack<Series>::Block> detached_blocks;
  detached_blocks.reserve(incoming_params.size());
  for (const auto& block : incoming_params.blocks()) {
    detached_blocks.push_back({block.name, block.value.detach()});
  }
  const ParameterPack<Series> params(std::move(detached_blocks));
  detail::validate_problem_(problem, params);

  ProblemEvaluation out;

  if (problem.objective.analytic) {
    try {
      auto [value, grad_pack] = problem.objective.analytic(params);
      out.value = std::move(value);
      out.gradient = std::move(grad_pack);
      // One number is stored as a rank-zero Series whatever shape the callback gave it.
      if (out.value.defined() && out.value.numel() == 1 && out.value.dim() != 0) out.value = out.value.reshape(std::vector<std::int64_t>{});
    } catch (...) {
      out.status = EvalStatus::InvalidProblem;
      out.message = detail::callback_failure_message_("objective analytic callback");
      return out;
    }
  } else {
    try {
      auto [value, grad_pack] = detail::objective_autodiff_value_and_grad_(problem, params);
      out.value = std::move(value);
      out.gradient = std::move(grad_pack);
      // One number is stored as a rank-zero Series whatever shape the callback gave it.
      if (out.value.defined() && out.value.numel() == 1 && out.value.dim() != 0) out.value = out.value.reshape(std::vector<std::int64_t>{});
    } catch (...) {
      out.status = EvalStatus::InvalidProblem;
      out.message = detail::callback_failure_message_("objective autodiff callback");
      return out;
    }
  }

  if (const auto error = detail::validate_scalar_output_(out.value, "objective value")) {
    out.status = EvalStatus::InvalidProblem;
    out.message = *error;
    return out;
  }
  if (const auto error = detail::validate_pack_like_params_(params, out.gradient, "objective gradient")) {
    out.status = EvalStatus::InvalidProblem;
    out.message = *error;
    return out;
  }

  if (!detail::is_finite_series_(out.value)) {
    out.status = EvalStatus::NonFiniteObjective;
    out.message = "objective evaluation returned non-finite values";
    return out;
  }
  if (detail::pack_inf_norm_(out.gradient) != detail::pack_inf_norm_(out.gradient)) {
    out.status = EvalStatus::NonFiniteGradient;
    out.message = "objective gradient evaluation returned non-finite values";
    return out;
  }

  NonlinearConstraintEvaluation nonlinear_eval;
  if (problem.nonlinear.analytic) {
    try {
      nonlinear_eval = problem.nonlinear.analytic(params);
    } catch (...) {
      out.status = EvalStatus::InvalidProblem;
      out.message = detail::callback_failure_message_("nonlinear analytic callback");
      return out;
    }
    // A residual the callback leaves undefined means "no constraints of that kind", as for the autodiff form.
    if (!nonlinear_eval.equality.defined()) {
      nonlinear_eval.equality = detail::empty_vec_like_(params);
      nonlinear_eval.equality_jacobian = detail::zero_jacobian_pack_(params, 0);
    }
    if (!nonlinear_eval.inequality.defined()) {
      nonlinear_eval.inequality = detail::empty_vec_like_(params);
      nonlinear_eval.inequality_jacobian = detail::zero_jacobian_pack_(params, 0);
    }
  } else {
    if (problem.nonlinear.equality) {
      try {
        auto [value, jac] = detail::jacrev_trace_(
            [&](const ParameterPack<detail::Trace>& p) -> detail::Trace { return problem.nonlinear.equality(p); },
            params);
        nonlinear_eval.equality = std::move(value);
        nonlinear_eval.equality_jacobian = std::move(jac);
      } catch (...) {
        out.status = EvalStatus::InvalidProblem;
        out.message = detail::callback_failure_message_("nonlinear equality autodiff callback");
        return out;
      }
    } else {
      nonlinear_eval.equality = detail::empty_vec_like_(params);
      nonlinear_eval.equality_jacobian = detail::zero_jacobian_pack_(params, 0);
    }
    if (problem.nonlinear.inequality) {
      try {
        auto [value, jac] = detail::jacrev_trace_(
            [&](const ParameterPack<detail::Trace>& p) -> detail::Trace { return problem.nonlinear.inequality(p); },
            params);
        nonlinear_eval.inequality = std::move(value);
        nonlinear_eval.inequality_jacobian = std::move(jac);
      } catch (...) {
        out.status = EvalStatus::InvalidProblem;
        out.message = detail::callback_failure_message_("nonlinear inequality autodiff callback");
        return out;
      }
    } else {
      nonlinear_eval.inequality = detail::empty_vec_like_(params);
      nonlinear_eval.inequality_jacobian = detail::zero_jacobian_pack_(params, 0);
    }
  }

  if (const auto error =
          detail::validate_residual_output_(nonlinear_eval.equality, "nonlinear equality residual")) {
    out.status = EvalStatus::InvalidProblem;
    out.message = *error;
    return out;
  }
  if (const auto error =
          detail::validate_residual_output_(nonlinear_eval.inequality, "nonlinear inequality residual")) {
    out.status = EvalStatus::InvalidProblem;
    out.message = *error;
    return out;
  }
  if (const auto error = detail::validate_jacobian_pack_(
          params, nonlinear_eval.equality_jacobian, nonlinear_eval.equality, "nonlinear equality jacobian")) {
    out.status = EvalStatus::InvalidProblem;
    out.message = *error;
    return out;
  }
  if (const auto error = detail::validate_jacobian_pack_(
          params,
          nonlinear_eval.inequality_jacobian,
          nonlinear_eval.inequality,
          "nonlinear inequality jacobian")) {
    out.status = EvalStatus::InvalidProblem;
    out.message = *error;
    return out;
  }

  const Series flat_params = flatten_values(params);
  const Series linear_eq = detail::evaluate_linear_eq_(problem.linear, flat_params);
  const Series linear_ineq = detail::evaluate_linear_ineq_(problem.linear, flat_params);
  const Series lower_ineq = detail::evaluate_lower_bound_residual_(problem.bounds, params);
  const Series upper_ineq = detail::evaluate_upper_bound_residual_(problem.bounds, params);

  out.nonlinear_equality_residual = nonlinear_eval.equality;
  out.nonlinear_inequality_residual = nonlinear_eval.inequality;
  out.equality_residual = detail::concat_flat_({linear_eq, nonlinear_eval.equality}, params);
  out.inequality_residual =
      detail::concat_flat_({linear_ineq, lower_ineq, upper_ineq, nonlinear_eval.inequality}, params);
  out.equality_jacobian = nonlinear_eval.equality_jacobian.empty()
                              ? detail::zero_jacobian_pack_(params, out.equality_residual.numel())
                              : std::move(nonlinear_eval.equality_jacobian);
  out.inequality_jacobian = nonlinear_eval.inequality_jacobian.empty()
                                ? detail::zero_jacobian_pack_(params, nonlinear_eval.inequality.numel())
                                : std::move(nonlinear_eval.inequality_jacobian);

  if (!detail::is_finite_series_(out.equality_residual) || !detail::is_finite_series_(out.inequality_residual)) {
    out.status = EvalStatus::NonFiniteConstraint;
    out.message = "constraint evaluation returned non-finite values";
    return out;
  }
  if (!detail::pack_finite_(out.equality_jacobian) || !detail::pack_finite_(out.inequality_jacobian)) {
    out.status = EvalStatus::NonFiniteJacobian;
    out.message = "constraint jacobian evaluation returned non-finite values";
    return out;
  }

  const Series flat_grad = flatten_values(out.gradient);
  const Series linear_eq_grad_flat =
      problem.linear.Aeq.has_value() && out.equality_residual.numel() > 0
          ? detail::dense_transpose_matvec_(
                *problem.linear.Aeq,
                detail::evaluate_linear_eq_(problem.linear, flat_params).to(ScalarType::Float64))
                : pluss::series::zeros({flat_params.numel()}, ScalarType::Float64).to(flat_params.platform());
  const Series linear_ineq_pos = linear_ineq.numel() == 0 ? linear_ineq : linear_ineq.clamp_min(0.0);
  const Series linear_ineq_grad_flat =
      problem.linear.Aineq.has_value() && linear_ineq_pos.numel() > 0
          ? detail::dense_transpose_matvec_(*problem.linear.Aineq, linear_ineq_pos.to(ScalarType::Float64))
          : pluss::series::zeros({flat_params.numel()}, ScalarType::Float64).to(flat_params.platform());

  Series bound_grad_flat = pluss::series::zeros({flat_params.numel()}, ScalarType::Float64).to(flat_params.platform());
  if (problem.bounds.lower.has_value()) {
    bound_grad_flat = bound_grad_flat.sub(lower_ineq.clamp_min(0.0).to(ScalarType::Float64));
  }
  if (problem.bounds.upper.has_value()) {
    bound_grad_flat = bound_grad_flat.add(upper_ineq.clamp_min(0.0).to(ScalarType::Float64));
  }

  auto eq_grad_pack = detail::add_pack_(
      detail::lift_flat_gradient_(params, linear_eq_grad_flat),
      detail::combine_constraint_gradient_(params, out.equality_jacobian, nonlinear_eval.equality, true));
  auto ineq_grad_pack = detail::add_pack_(
      detail::add_pack_(
          detail::lift_flat_gradient_(params, linear_ineq_grad_flat),
          detail::lift_flat_gradient_(params, bound_grad_flat)),
      detail::combine_constraint_gradient_(
          params,
          out.inequality_jacobian,
          nonlinear_eval.inequality.numel() == 0 ? nonlinear_eval.inequality
                                                 : nonlinear_eval.inequality.clamp_min(0.0),
          false));

  const double eq_violation = detail::max_abs_scalar_(out.equality_residual);
  const double ineq_violation = detail::max_positive_scalar_(out.inequality_residual);
  out.max_constraint_violation = std::max(eq_violation, ineq_violation);
  out.first_order_opt = detail::pack_inf_norm_(out.gradient);
  out.feasible = out.max_constraint_violation <= 0.0;
  out.message = "ok";

  (void)flat_grad;
  (void)eq_grad_pack;
  (void)ineq_grad_pack;
  return out;
}

inline DerivativeAuditReport audit_optima_derivatives(
    const ConstrainedProblem& problem,
    const ParameterPack<Series>& params,
    const DerivativeAuditOptions& options = {}) {
  detail::validate_problem_(problem, params);

  auto make_entry = [&](bool available,
                        double value_abs_diff,
                        double value_rel_diff,
                        double derivative_abs_diff,
                        double derivative_rel_diff,
                        std::string message) {
    const bool passed = available &&
                        value_abs_diff <= options.absolute_tolerance &&
                        value_rel_diff <= options.relative_tolerance &&
                        derivative_abs_diff <= options.absolute_tolerance &&
                        derivative_rel_diff <= options.relative_tolerance;
    return DerivativeAuditEntry{
        available,
        passed,
        value_abs_diff,
        value_rel_diff,
        derivative_abs_diff,
        derivative_rel_diff,
        std::move(message)};
  };

  DerivativeAuditReport report;

  if (problem.objective.analytic && problem.objective.autodiff) {
    auto [analytic_value, analytic_gradient] = problem.objective.analytic(params);
    auto [autodiff_value, autodiff_gradient] = detail::objective_autodiff_value_and_grad_(problem, params);
    const double value_abs_diff = detail::max_abs_diff_series_(analytic_value, autodiff_value);
    const double value_rel_diff = detail::max_rel_diff_series_(analytic_value, autodiff_value);
    const double gradient_abs_diff = detail::max_abs_diff_pack_(analytic_gradient, autodiff_gradient);
    const double gradient_rel_diff = detail::max_rel_diff_pack_(analytic_gradient, autodiff_gradient);
    report.objective = make_entry(
        true,
        value_abs_diff,
        value_rel_diff,
        gradient_abs_diff,
        gradient_rel_diff,
        "objective analytic vs autodiff");
  } else {
    report.objective.message = "requires analytic objective and autodiff objective definition";
  }

  if (problem.nonlinear.analytic && problem.nonlinear.equality) {
    const auto analytic_eval = problem.nonlinear.analytic(params);
    auto [autodiff_value, autodiff_jacobian] = detail::jacrev_trace_(
        [&](const ParameterPack<detail::Trace>& p) -> detail::Trace { return problem.nonlinear.equality(p); },
        params);
    const double value_abs_diff = detail::max_abs_diff_series_(analytic_eval.equality, autodiff_value);
    const double value_rel_diff = detail::max_rel_diff_series_(analytic_eval.equality, autodiff_value);
    const double jac_abs_diff =
        detail::max_abs_diff_pack_(analytic_eval.equality_jacobian, autodiff_jacobian);
    const double jac_rel_diff =
        detail::max_rel_diff_pack_(analytic_eval.equality_jacobian, autodiff_jacobian);
    report.nonlinear_equalities = make_entry(
        true,
        value_abs_diff,
        value_rel_diff,
        jac_abs_diff,
        jac_rel_diff,
        "nonlinear equality analytic vs autodiff");
  } else {
    report.nonlinear_equalities.message =
        "requires analytic nonlinear constraints and autodiff equality definition";
  }

  if (problem.nonlinear.analytic && problem.nonlinear.inequality) {
    const auto analytic_eval = problem.nonlinear.analytic(params);
    auto [autodiff_value, autodiff_jacobian] = detail::jacrev_trace_(
        [&](const ParameterPack<detail::Trace>& p) -> detail::Trace { return problem.nonlinear.inequality(p); },
        params);
    const double value_abs_diff = detail::max_abs_diff_series_(analytic_eval.inequality, autodiff_value);
    const double value_rel_diff = detail::max_rel_diff_series_(analytic_eval.inequality, autodiff_value);
    const double jac_abs_diff =
        detail::max_abs_diff_pack_(analytic_eval.inequality_jacobian, autodiff_jacobian);
    const double jac_rel_diff =
        detail::max_rel_diff_pack_(analytic_eval.inequality_jacobian, autodiff_jacobian);
    report.nonlinear_inequalities = make_entry(
        true,
        value_abs_diff,
        value_rel_diff,
        jac_abs_diff,
        jac_rel_diff,
        "nonlinear inequality analytic vs autodiff");
  } else {
    report.nonlinear_inequalities.message =
        "requires analytic nonlinear constraints and autodiff inequality definition";
  }

  report.passed = (!report.objective.available || report.objective.passed) &&
                  (!report.nonlinear_equalities.available || report.nonlinear_equalities.passed) &&
                  (!report.nonlinear_inequalities.available || report.nonlinear_inequalities.passed);
  return report;
}

inline ParameterPack<Series> penalty_gradient(const ConstrainedProblem& problem,
                                              const ParameterPack<Series>& params,
                                              const ProblemEvaluation& evaluation,
                                              double penalty) {
  (void)problem;
  auto total = evaluation.gradient;

  const Series flat_params = flatten_values(params);
  Series linear_eq = detail::evaluate_linear_eq_(problem.linear, flat_params);
  Series linear_ineq = detail::evaluate_linear_ineq_(problem.linear, flat_params);
  Series lower_ineq = detail::evaluate_lower_bound_residual_(problem.bounds, params);
  Series upper_ineq = detail::evaluate_upper_bound_residual_(problem.bounds, params);

  Series linear_eq_grad_flat =
      problem.linear.Aeq.has_value() && linear_eq.numel() > 0
          ? detail::dense_transpose_matvec_(*problem.linear.Aeq, linear_eq.to(ScalarType::Float64))
          : pluss::series::zeros({flat_params.numel()}, ScalarType::Float64).to(flat_params.platform());
  Series linear_ineq_grad_flat =
      problem.linear.Aineq.has_value() && linear_ineq.numel() > 0
          ? detail::dense_transpose_matvec_(*problem.linear.Aineq, linear_ineq.clamp_min(0.0).to(ScalarType::Float64))
          : pluss::series::zeros({flat_params.numel()}, ScalarType::Float64).to(flat_params.platform());
  Series bound_grad_flat = pluss::series::zeros({flat_params.numel()}, ScalarType::Float64).to(flat_params.platform());
  if (lower_ineq.numel() > 0) bound_grad_flat = bound_grad_flat.sub(lower_ineq.clamp_min(0.0).to(ScalarType::Float64));
  if (upper_ineq.numel() > 0) bound_grad_flat = bound_grad_flat.add(upper_ineq.clamp_min(0.0).to(ScalarType::Float64));

  auto penalty_pack = detail::add_pack_(
      detail::add_pack_(
          detail::lift_flat_gradient_(params, linear_eq_grad_flat),
          detail::lift_flat_gradient_(params, linear_ineq_grad_flat.add(bound_grad_flat))),
      detail::add_pack_(
          detail::combine_constraint_gradient_(
              params,
              evaluation.equality_jacobian,
              evaluation.nonlinear_equality_residual,
              true),
          detail::combine_constraint_gradient_(
              params,
              evaluation.inequality_jacobian,
              evaluation.nonlinear_inequality_residual.numel() == 0
                  ? evaluation.nonlinear_inequality_residual
                  : evaluation.nonlinear_inequality_residual.clamp_min(0.0),
              false)));

  total = detail::add_pack_(total, detail::scaled_pack_(penalty_pack, penalty));
  return total;
}

inline ParameterPack<Series> normalized_penalty_gradient_(const ConstrainedProblem& problem,
                                                          const ParameterPack<Series>& params,
                                                          const ProblemEvaluation& evaluation,
                                                          double penalty,
                                                          const OptimaNormalizationStats& normalization) {
  auto total = detail::scaled_pack_(evaluation.gradient, normalization.objective_scale);

  const Series flat_params = flatten_values(params);
  const Series linear_eq = detail::evaluate_linear_eq_(problem.linear, flat_params);
  const Series linear_ineq = detail::evaluate_linear_ineq_(problem.linear, flat_params);
  const Series lower_ineq = detail::evaluate_lower_bound_residual_(problem.bounds, params);
  const Series upper_ineq = detail::evaluate_upper_bound_residual_(problem.bounds, params);

  const std::int64_t n_linear_eq = problem.linear.Aeq.has_value() ? problem.linear.Aeq->sizes()[0] : 0;
  const std::int64_t n_linear_ineq = problem.linear.Aineq.has_value() ? problem.linear.Aineq->sizes()[0] : 0;
  const std::int64_t n_lower = problem.bounds.lower.has_value() ? params.total_numel() : 0;
  const std::int64_t n_upper = problem.bounds.upper.has_value() ? params.total_numel() : 0;

  const Series eq_scale = normalization.equality_row_scale.defined()
                              ? normalization.equality_row_scale.to(flat_params.platform())
                              : pluss::series::ones({evaluation.equality_residual.numel()}, ScalarType::Float64).to(flat_params.platform());
  const Series ineq_scale = normalization.inequality_row_scale.defined()
                                ? normalization.inequality_row_scale.to(flat_params.platform())
                                : pluss::series::ones({evaluation.inequality_residual.numel()}, ScalarType::Float64).to(flat_params.platform());

  const Series linear_eq_weight =
      linear_eq.numel() == 0 ? linear_eq
                             : linear_eq.to(ScalarType::Float64)
                                   .mul(detail::slice_series_(eq_scale, 0, n_linear_eq).square());
  const Series nonlinear_eq_weight =
      evaluation.nonlinear_equality_residual.numel() == 0
          ? evaluation.nonlinear_equality_residual
          : evaluation.nonlinear_equality_residual.to(ScalarType::Float64)
                .mul(detail::slice_series_(eq_scale, n_linear_eq, evaluation.nonlinear_equality_residual.numel()).square());

  const Series linear_ineq_weight =
      linear_ineq.numel() == 0 ? linear_ineq
                               : linear_ineq.clamp_min(0.0).to(ScalarType::Float64)
                                     .mul(detail::slice_series_(ineq_scale, 0, n_linear_ineq).square());
  const Series lower_weight =
      lower_ineq.numel() == 0 ? lower_ineq
                              : lower_ineq.clamp_min(0.0).to(ScalarType::Float64)
                                    .mul(detail::slice_series_(ineq_scale, n_linear_ineq, n_lower).square());
  const Series upper_weight =
      upper_ineq.numel() == 0 ? upper_ineq
                              : upper_ineq.clamp_min(0.0).to(ScalarType::Float64)
                                    .mul(detail::slice_series_(ineq_scale, n_linear_ineq + n_lower, n_upper).square());
  const Series nonlinear_ineq_weight =
      evaluation.nonlinear_inequality_residual.numel() == 0
          ? evaluation.nonlinear_inequality_residual
          : evaluation.nonlinear_inequality_residual.clamp_min(0.0).to(ScalarType::Float64)
                .mul(detail::slice_series_(ineq_scale,
                                           n_linear_ineq + n_lower + n_upper,
                                           evaluation.nonlinear_inequality_residual.numel()).square());

  Series linear_eq_grad_flat =
      problem.linear.Aeq.has_value() && linear_eq_weight.numel() > 0
          ? detail::dense_transpose_matvec_(*problem.linear.Aeq, linear_eq_weight)
          : pluss::series::zeros({flat_params.numel()}, ScalarType::Float64).to(flat_params.platform());
  Series linear_ineq_grad_flat =
      problem.linear.Aineq.has_value() && linear_ineq_weight.numel() > 0
          ? detail::dense_transpose_matvec_(*problem.linear.Aineq, linear_ineq_weight)
          : pluss::series::zeros({flat_params.numel()}, ScalarType::Float64).to(flat_params.platform());
  Series bound_grad_flat = pluss::series::zeros({flat_params.numel()}, ScalarType::Float64).to(flat_params.platform());
  if (lower_weight.numel() > 0) bound_grad_flat = bound_grad_flat.sub(lower_weight);
  if (upper_weight.numel() > 0) bound_grad_flat = bound_grad_flat.add(upper_weight);

  auto penalty_pack = detail::add_pack_(
      detail::add_pack_(
          detail::lift_flat_gradient_(params, linear_eq_grad_flat),
          detail::lift_flat_gradient_(params, linear_ineq_grad_flat.add(bound_grad_flat))),
      detail::add_pack_(
          detail::combine_constraint_gradient_(params, evaluation.equality_jacobian, nonlinear_eq_weight, true),
          detail::combine_constraint_gradient_(params, evaluation.inequality_jacobian, nonlinear_ineq_weight, false)));

  total = detail::add_pack_(total, detail::scaled_pack_(penalty_pack, penalty));
  return total;
}

namespace detail {

struct RecoveryResult {
  ParameterPack<Series> params{};
  ProblemEvaluation evaluation{};
  double objective_value{std::numeric_limits<double>::infinity()};
  double last_step_size{0.0};
  std::int64_t function_evaluations{0};
  std::int64_t iterations{0};
  bool success{false};
  std::string message{};
};

inline double constraint_violation_value_(const ProblemEvaluation& evaluation) {
  const double eq_term = 0.5 * squared_l2_(evaluation.equality_residual);
  const Series ineq_pos = evaluation.inequality_residual.numel() == 0
                              ? evaluation.inequality_residual
                              : evaluation.inequality_residual.clamp_min(0.0);
  const double ineq_term = 0.5 * squared_l2_(ineq_pos);
  return eq_term + ineq_term;
}

inline double constraint_violation_value_(const ProblemEvaluation& evaluation,
                                          const OptimaNormalizationStats& normalization) {
  const Series eq_scaled =
      evaluation.equality_residual.numel() == 0 || !normalization.equality_row_scale.defined()
          ? evaluation.equality_residual
          : evaluation.equality_residual.to(ScalarType::Float64).mul(normalization.equality_row_scale.to(evaluation.equality_residual.platform()));
  const Series ineq_pos = evaluation.inequality_residual.numel() == 0
                              ? evaluation.inequality_residual
                              : evaluation.inequality_residual.clamp_min(0.0);
  const Series ineq_scaled =
      ineq_pos.numel() == 0 || !normalization.inequality_row_scale.defined()
          ? ineq_pos
          : ineq_pos.to(ScalarType::Float64).mul(normalization.inequality_row_scale.to(ineq_pos.platform()));
  return 0.5 * squared_l2_(eq_scaled) + 0.5 * squared_l2_(ineq_scaled);
}

inline ParameterPack<Series> constraint_violation_gradient_(const ConstrainedProblem& problem,
                                                            const ParameterPack<Series>& params,
                                                            const ProblemEvaluation& evaluation) {
  return add_pack_(
      penalty_gradient(problem, params, evaluation, 1.0),
      scaled_pack_(evaluation.gradient, -1.0));
}

inline ParameterPack<Series> constraint_violation_gradient_(const ConstrainedProblem& problem,
                                                            const ParameterPack<Series>& params,
                                                            const ProblemEvaluation& evaluation,
                                                            const OptimaNormalizationStats& normalization) {
  return add_pack_(
      normalized_penalty_gradient_(problem, params, evaluation, 1.0, normalization),
      scaled_pack_(evaluation.gradient, -normalization.objective_scale));
}

inline RecoveryResult restore_feasibility_(const ConstrainedProblem& problem,
                                           const ParameterPack<Series>& start_params,
                                           const ProblemEvaluation& start_evaluation,
                                           const OptimaNormalizationStats& normalization,
                                           const SequentialQPOptions& options) {
  RecoveryResult result;
  result.params = start_params;
  result.evaluation = start_evaluation;
  result.objective_value = constraint_violation_value_(start_evaluation, normalization);
  result.message = "restoration failed to reduce constraint violation";

  if (start_evaluation.status != EvalStatus::Ok) {
    result.message = "restoration requires a valid starting evaluation";
    return result;
  }

  auto current_params = start_params;
  auto current_evaluation = start_evaluation;
  double current_value = result.objective_value;
  const auto current_layout = current_params.layout();
  Series current_flat = flatten_values(current_params).to(ScalarType::Float64);
  const double start_violation = start_evaluation.max_constraint_violation;

  for (std::int64_t iter = 0; iter < options.max_restoration_iterations; ++iter) {
    const auto grad_pack = constraint_violation_gradient_(problem, current_params, current_evaluation, normalization);
    const Series gradient = flatten_values(grad_pack).to(ScalarType::Float64);
    if (series_inf_norm_(gradient) <= 1e-12) break;

    const Series direction = gradient.mul(-1.0);
    double directional_derivative = series_dot_(gradient, direction);
    if (!(directional_derivative < 0.0)) {
      directional_derivative = -std::max(1.0, series_dot_(direction, direction));
    }

    double step_size = options.initial_step_size;
    bool accepted = false;
    for (std::int64_t ls = 0; ls < options.max_restoration_line_search_iterations; ++ls) {
      if (step_size * series_inf_norm_(direction) <= std::max(1e-12, options.step_tolerance)) break;
      auto trial = unflatten_values(current_layout, current_flat.add(direction.mul(step_size)));
      if (options.project_bounds) trial = project_bounds_(trial, problem.bounds);

      auto trial_evaluation = evaluate_problem(problem, trial);
      ++result.function_evaluations;
      if (trial_evaluation.status == EvalStatus::Ok) {
        const double trial_value = constraint_violation_value_(trial_evaluation, normalization);
        if (std::isfinite(trial_value) &&
            trial_value <= current_value + options.armijo * step_size * directional_derivative) {
          current_params = std::move(trial);
          current_evaluation = std::move(trial_evaluation);
          current_value = trial_value;
          current_flat = flatten_values(current_params).to(ScalarType::Float64);
          result.last_step_size = step_size;
          ++result.iterations;
          accepted = true;
          break;
        }
      }
      step_size *= options.backtracking;
    }

    if (!accepted) break;
    if (current_evaluation.max_constraint_violation <= options.constraint_tolerance) break;
  }

  result.params = std::move(current_params);
  result.evaluation = std::move(current_evaluation);
  result.objective_value = current_value;
  result.success =
      result.evaluation.max_constraint_violation + 1e-10 < start_violation ||
      result.objective_value + 1e-12 < constraint_violation_value_(start_evaluation, normalization) ||
      result.evaluation.max_constraint_violation <= options.constraint_tolerance;
  if (result.success) {
    result.message = "restoration reduced constraint violation";
  }
  return result;
}

inline RecoveryResult recover_barrier_descent_(const ConstrainedProblem& problem,
                                               const ParameterPack<Series>& start_params,
                                               const ProblemEvaluation& start_evaluation,
                                               const OptimaNormalizationStats& normalization,
                                               double barrier_parameter,
                                               const InternalProtocolOptions& options) {
  RecoveryResult result;
  result.params = start_params;
  result.evaluation = start_evaluation;

  if (start_evaluation.status != EvalStatus::Ok) {
    result.message = "barrier recovery requires a valid starting evaluation";
    return result;
  }

  auto current_params = start_params;
  auto current_evaluation = start_evaluation;
  const auto current_layout = current_params.layout();
  Series current_flat = flatten_values(current_params).to(ScalarType::Float64);
  auto current_system = assemble_constraint_system_(problem, current_params, current_evaluation);
  // The barrier is finite for any strictly positive margin; demanding the full initialization margin made a
  // legitimately near-boundary iterate (a parameter close to its bound) impossible to recover.
  if (!(inequality_margin_(current_system) > 0.0)) {
    result.message = "barrier recovery requires a strict interior starting point";
    return result;
  }

  double current_value =
      barrier_value_(current_evaluation, current_system, barrier_parameter, options.equality_penalty, normalization);
  result.objective_value = current_value;
  result.message = "barrier recovery failed to reduce the barrier objective";
  if (!std::isfinite(current_value)) return result;

  for (std::int64_t iter = 0; iter < options.max_restoration_iterations; ++iter) {
    const auto barrier_gradient =
        barrier_gradient_flat_(current_evaluation, current_system, barrier_parameter, options.equality_penalty, normalization);
    const std::int64_t n = current_params.total_numel();
    Series gradient = barrier_gradient.to(ScalarType::Float64).to(current_params[0].value.platform()).reshape({n});
    if (!is_finite_series_(gradient) || series_inf_norm_(gradient) <= 1e-12) break;

    const Series direction = gradient.mul(-1.0);
    double max_step = options.initial_step_size;
    for (const auto& row : current_system.inequalities) {
      const double directional = series_dot_(row.jacobian.to(direction.platform()), direction);
      if (directional > 0.0) {
        max_step = std::min(
            max_step,
            options.fraction_to_boundary * (-options.initialization_margin - row.residual) / directional);
      }
    }
    if (!(max_step > 0.0) || !std::isfinite(max_step)) break;

    double directional_derivative = series_dot_(gradient, direction);
    if (!(directional_derivative < 0.0)) {
      directional_derivative = -std::max(1.0, series_dot_(direction, direction));
    }

    double step_size = max_step;
    bool accepted = false;
    for (std::int64_t ls = 0; ls < options.max_restoration_line_search_iterations; ++ls) {
      if (step_size * series_inf_norm_(direction) <= std::max(1e-12, options.step_tolerance)) break;
      auto trial = unflatten_values(current_layout, current_flat.add(direction.mul(step_size)));
      if (options.project_bounds) trial = project_bounds_(trial, problem.bounds);

      auto trial_evaluation = evaluate_problem(problem, trial);
      ++result.function_evaluations;
      if (trial_evaluation.status == EvalStatus::Ok) {
        const auto trial_system = assemble_constraint_system_(problem, trial, trial_evaluation);
        if (inequality_margin_(trial_system) > options.initialization_margin) {
          const double trial_value = barrier_value_(
              trial_evaluation,
              trial_system,
              barrier_parameter,
              options.equality_penalty,
              normalization);
          if (std::isfinite(trial_value) &&
              trial_value <= current_value + options.armijo * step_size * directional_derivative) {
            current_params = std::move(trial);
            current_evaluation = std::move(trial_evaluation);
            current_system = std::move(trial_system);
            current_value = trial_value;
            current_flat = flatten_values(current_params).to(ScalarType::Float64);
            result.last_step_size = step_size;
            ++result.iterations;
            accepted = true;
            break;
          }
        }
      }
      step_size *= options.backtracking;
    }

    if (!accepted) break;
  }

  result.params = std::move(current_params);
  result.evaluation = std::move(current_evaluation);
  result.objective_value = current_value;
  const double start_value =
      barrier_value_(start_evaluation,
                     assemble_constraint_system_(problem, start_params, start_evaluation),
                     barrier_parameter,
                     options.equality_penalty,
                     normalization);
  const double stationary_tolerance = std::max(1e-12, 1e-10 * std::max(1.0, std::abs(start_value)));
  const bool nonworse_objective =
      std::isfinite(result.objective_value) && result.objective_value <= start_value + stationary_tolerance;
  const bool improved_objective = nonworse_objective && result.objective_value + stationary_tolerance < start_value;
  const Series start_flat = flatten_values(start_params).to(ScalarType::Float64);
  const Series end_flat = flatten_values(result.params).to(ScalarType::Float64);
  const double start_scale = std::max(1.0, series_inf_norm_(start_flat));
  const double step_magnitude = series_inf_norm_(end_flat.sub(start_flat));
  const bool meaningful_step = step_magnitude > std::max(1e-14, 1e-10 * start_scale);
  const bool improved_violation =
      result.evaluation.max_constraint_violation + 1e-12 < start_evaluation.max_constraint_violation;
  const bool stationary_feasible = result.evaluation.status == EvalStatus::Ok &&
                                   result.evaluation.max_constraint_violation <= options.constraint_tolerance;
  // Recovery must make actual progress. Treating a zero-step feasible point as
  // "success" causes repeated no-op major iterations with step=0.
  result.success = nonworse_objective && result.iterations > 0 &&
                   (improved_objective || meaningful_step || improved_violation);
  if (result.success) {
    result.message = "barrier recovery reduced the barrier objective";
  } else if (stationary_feasible && !improved_objective) {
    result.message = "barrier recovery made no progress at a feasible point";
  }
  return result;
}

inline bool should_enter_feasibility_phase_(const InternalProtocolState& state,
                                            const InternalProtocolOptions& options) {
  if (!options.enable_feasibility_phase) return false;
  if (state.evaluation.status != EvalStatus::Ok) return false;
  const double threshold =
      std::max(options.constraint_tolerance, options.feasibility_activation_factor * state.barrier_parameter);
  return state.evaluation.max_constraint_violation > threshold;
}

inline bool near_stationary_feasible_(const InternalProtocolState& state,
                                      const InternalProtocolOptions& options) {
  if (state.evaluation.status != EvalStatus::Ok) return false;
  // Practical convergence gate: mirrors TolFun/TolX-style stopping behavior
  // once feasibility is strong and barrier terms are sufficiently small.
  const double barrier_gate =
      std::max(100.0 * options.minimum_barrier_parameter, options.minimum_barrier_parameter);
  if (!(state.barrier_parameter <= barrier_gate)) return false;

  const double stationarity_gate = std::max(10.0 * options.optimality_tolerance, 1e-12);
  const double feasibility_gate = std::max(options.constraint_tolerance, 1e-12);
  const double complementarity_gate = std::max(10.0 * options.optimality_tolerance, 1e-12);

  return state.evaluation.max_constraint_violation <= feasibility_gate &&
         state.complementarity_residual <= complementarity_gate &&
         state.barrier_gradient_norm <= stationarity_gate;
}

inline bool should_reduce_barrier_(const InternalProtocolState& state,
                                   const InternalProtocolOptions& options) {
  if (!(state.barrier_parameter > options.minimum_barrier_parameter)) return false;
  return state.barrier_gradient_norm <=
             std::max(10.0 * options.optimality_tolerance, 10.0 * state.barrier_parameter) &&
         state.evaluation.max_constraint_violation <=
             std::max(options.constraint_tolerance, 10.0 * state.barrier_parameter) &&
         state.complementarity_residual <=
             std::max(options.optimality_tolerance, 10.0 * state.barrier_parameter);
}

inline void push_barrier_history_(std::vector<double>& history,
                                  double barrier_value,
                                  std::size_t max_keep) {
  if (!std::isfinite(barrier_value)) return;
  history.push_back(barrier_value);
  if (history.size() > max_keep) {
    const std::size_t drop = history.size() - max_keep;
    history.erase(history.begin(), history.begin() + static_cast<long>(drop));
  }
}

inline double nonmonotone_reference_(const std::vector<double>& history,
                                     std::int64_t window,
                                     double fallback) {
  double reference = fallback;
  if (history.empty()) return reference;
  const std::size_t w = static_cast<std::size_t>(std::max<std::int64_t>(1, window));
  const std::size_t start = history.size() > w ? (history.size() - w) : 0;
  for (std::size_t i = start; i < history.size(); ++i) {
    if (std::isfinite(history[i])) reference = std::max(reference, history[i]);
  }
  return reference;
}

inline double update_trust_radius_(double current_radius,
                                   double step_size,
                                   double ratio,
                                   const InternalProtocolOptions& options) {
  double radius = std::clamp(
      current_radius,
      std::max(1e-12, options.trust_region_min_radius),
      std::max(options.trust_region_min_radius, options.trust_region_max_radius));
  if (std::isfinite(ratio) && ratio >= options.trust_region_expand_ratio &&
      step_size >= 0.8 * radius) {
    radius = std::min(
        std::max(options.trust_region_min_radius, options.trust_region_max_radius),
        radius * std::max(1.0, options.trust_region_expand));
  } else if (!std::isfinite(ratio) || ratio < options.trust_region_shrink_ratio) {
    radius = std::max(
        std::max(1e-12, options.trust_region_min_radius),
        radius * std::clamp(options.trust_region_shrink, 1e-3, 1.0));
  }
  return radius;
}

inline RecoveryResult advance_feasibility_phase_(const ConstrainedProblem& problem,
                                                 const ParameterPack<Series>& start_params,
                                                 const ProblemEvaluation& start_evaluation,
                                                 const OptimaNormalizationStats& normalization,
                                                 const InternalProtocolOptions& options) {
  RecoveryResult result;
  result.params = start_params;
  result.evaluation = start_evaluation;
  result.objective_value = constraint_violation_value_(start_evaluation, normalization);
  result.message = "feasibility phase failed to reduce constraint violation";

  if (start_evaluation.status != EvalStatus::Ok) {
    result.message = "feasibility phase requires a valid starting evaluation";
    return result;
  }

  const double start_violation = start_evaluation.max_constraint_violation;
  auto current_params = start_params;
  auto current_evaluation = start_evaluation;
  double current_value = result.objective_value;
  const auto current_layout = current_params.layout();
  Series current_flat = flatten_values(current_params).to(ScalarType::Float64);

  for (std::int64_t iter = 0; iter < options.max_restoration_iterations; ++iter) {
    const auto grad_pack = constraint_violation_gradient_(problem, current_params, current_evaluation, normalization);
    const Series gradient = flatten_values(grad_pack).to(ScalarType::Float64);
    if (series_inf_norm_(gradient) <= 1e-12) {
      result.message = "feasibility phase reached a stationary violation point";
      break;
    }

    const Series direction = gradient.mul(-1.0);
    double directional_derivative = series_dot_(gradient, direction);
    if (!(directional_derivative < 0.0)) {
      directional_derivative = -std::max(1.0, series_dot_(direction, direction));
    }

    double step_size = options.initial_step_size;
    bool accepted = false;
    for (std::int64_t ls = 0; ls < options.max_restoration_line_search_iterations; ++ls) {
      if (step_size * series_inf_norm_(direction) <= std::max(1e-12, options.step_tolerance)) break;
      auto trial = unflatten_values(current_layout, current_flat.add(direction.mul(step_size)));
      if (options.project_bounds) trial = project_bounds_(trial, problem.bounds);

      auto trial_evaluation = evaluate_problem(problem, trial);
      ++result.function_evaluations;
      if (trial_evaluation.status == EvalStatus::Ok) {
        const double trial_value = constraint_violation_value_(trial_evaluation, normalization);
        if (std::isfinite(trial_value) &&
            trial_value <= current_value + options.armijo * step_size * directional_derivative) {
          current_params = std::move(trial);
          current_evaluation = std::move(trial_evaluation);
          current_value = trial_value;
          current_flat = flatten_values(current_params).to(ScalarType::Float64);
          result.last_step_size = step_size;
          ++result.iterations;
          accepted = true;
          break;
        }
      }
      step_size *= options.backtracking;
    }

    if (!accepted) break;

    if (current_evaluation.max_constraint_violation <=
        std::max(options.constraint_tolerance, 0.5 * start_violation)) {
      result.success = true;
      result.params = current_params;
      result.evaluation = current_evaluation;
      result.objective_value = current_value;
      result.message = "feasibility phase reduced constraint violation";
      return result;
    }
  }

  if (current_evaluation.max_constraint_violation + 1e-12 < start_violation) {
    result.success = true;
    result.params = current_params;
    result.evaluation = current_evaluation;
    result.objective_value = current_value;
    result.message = "feasibility phase improved constraint violation";
  }
  return result;
}

} // namespace detail

namespace detail {

inline std::pair<Series, Series> flat_bound_vectors_(const ConstrainedProblem& problem,
                                                     const ParameterPack<Series>& params) {
  const std::int64_t n = params.total_numel();
  const auto platform = params.empty() ? pluss::codem::Platform::CPU : params[0].value.platform();
  const double inf = std::numeric_limits<double>::infinity();
  Series lower = pluss::series::full({n}, -inf, ScalarType::Float64).to(platform);
  Series upper = pluss::series::full({n}, inf, ScalarType::Float64).to(platform);
  if (problem.bounds.lower.has_value()) {
    lower = flatten_values(*problem.bounds.lower).to(ScalarType::Float64).to(platform).reshape({n});
  }
  if (problem.bounds.upper.has_value()) {
    upper = flatten_values(*problem.bounds.upper).to(ScalarType::Float64).to(platform).reshape({n});
  }
  return {lower, upper};
}

inline std::vector<FlatConstraintRow> non_bound_inequality_rows_(const FlatConstraintSystem& system) {
  std::vector<FlatConstraintRow> rows;
  for (const auto& row : system.inequalities) {
    if (row.source == ConstraintSource::LowerBound || row.source == ConstraintSource::UpperBound) continue;
    rows.push_back(row);
  }
  return rows;
}

inline Series multiplier_or_zero_(const Series& current, std::int64_t count, pluss::codem::Platform platform) {
  if (current.defined() && current.numel() == count) {
    return current.to(ScalarType::Float64).to(platform).reshape({count});
  }
  return pluss::series::zeros({count}, ScalarType::Float64).to(platform);
}

struct AugmentedLagrangian {
  double value{0.0};
  Series gradient = pluss::series::undefined();
  std::vector<double> equality_weight{};
  std::vector<double> inequality_weight{};
};

// Augmented Lagrangian (Powell-Hestenes-Rockafellar form) over the equality rows and the non-bound
// inequality rows; bounds are handled separately by projection.
//   f + sum(l_j h_j + rho/2 h_j^2) + sum((max(0, m_i + rho g_i)^2 - m_i^2) / (2 rho))
inline AugmentedLagrangian augmented_lagrangian_(const ProblemEvaluation& evaluation,
                                                 const std::vector<FlatConstraintRow>& equalities,
                                                 const std::vector<FlatConstraintRow>& inequalities,
                                                 const Series& equality_multiplier,
                                                 const Series& inequality_multiplier,
                                                 double rho) {
  AugmentedLagrangian out;
  out.value = evaluation.value.item<double>();
  Series grad = flatten_values(evaluation.gradient).to(ScalarType::Float64);
  grad = grad.reshape({grad.numel()});
  const auto platform = grad.platform();
  const Series lambda = equality_multiplier.to(ScalarType::Float64).reshape({equality_multiplier.numel()});
  const Series mu = inequality_multiplier.to(ScalarType::Float64).reshape({inequality_multiplier.numel()});
  out.equality_weight.assign(equalities.size(), 0.0);
  for (std::size_t j = 0; j < equalities.size(); ++j) {
    const double l = lambda.select(0, static_cast<std::int64_t>(j)).item<double>();
    const double h = equalities[j].residual;
    out.value += l * h + 0.5 * rho * h * h;
    out.equality_weight[j] = l + rho * h;
  }
  out.inequality_weight.assign(inequalities.size(), 0.0);
  for (std::size_t i = 0; i < inequalities.size(); ++i) {
    const double m = mu.select(0, static_cast<std::int64_t>(i)).item<double>();
    const double t = m + rho * inequalities[i].residual;
    if (t > 0.0) {
      out.value += (t * t - m * m) / (2.0 * rho);
      out.inequality_weight[i] = t;
    } else {
      out.value -= m * m / (2.0 * rho);
    }
  }
  auto weights_series = [&](const std::vector<double>& w) {
    return pluss::make_series(w, {static_cast<std::int64_t>(w.size())}).to(ScalarType::Float64).to(platform);
  };
  if (!equalities.empty()) grad = add_weighted_row_jacobians_(grad, equalities, weights_series(out.equality_weight));
  if (!inequalities.empty()) grad = add_weighted_row_jacobians_(grad, inequalities, weights_series(out.inequality_weight));
  out.gradient = grad;
  return out;
}

// Largest component of x - P(x - g), the standard first-order measure for a bound-constrained minimizer.
inline double projected_gradient_norm_(const Series& x,
                                       const Series& gradient,
                                       const Series& lower,
                                       const Series& upper) {
  return series_inf_norm_(x.sub(x.sub(gradient).minimum(upper).maximum(lower)));
}

} // namespace detail

inline PenaltyMethodState init_penalty_method_state(const ConstrainedProblem& problem,
                                                    const ParameterPack<Series>& initial_params,
                                                    const PenaltyMethodOptions& options = {}) {
  PenaltyMethodState state;
  state.params = options.project_bounds ? detail::project_bounds_(initial_params, problem.bounds) : initial_params;
  state.evaluation = evaluate_problem(problem, state.params);
  state.function_evaluations = 1;
  state.current_penalty = std::max(options.penalty, 1e-8);
  state.converged = state.evaluation.status != EvalStatus::Ok;
  state.message = state.converged ? state.evaluation.message : "initialized";
  const auto platform = state.params.empty() ? pluss::codem::Platform::CPU : state.params[0].value.platform();
  state.hessian = detail::identity_matrix_(state.params.total_numel(), platform);
  if (state.evaluation.status == EvalStatus::Ok) {
    const auto system = detail::assemble_constraint_system_(problem, state.params, state.evaluation);
    const auto inequalities = detail::non_bound_inequality_rows_(system);
    state.equality_multipliers = detail::multiplier_or_zero_(Series(), static_cast<std::int64_t>(system.equalities.size()), platform);
    state.inequality_multipliers = detail::multiplier_or_zero_(Series(), static_cast<std::int64_t>(inequalities.size()), platform);
    state.penalized_value = detail::augmented_lagrangian_(
                                state.evaluation, system.equalities, inequalities,
                                state.equality_multipliers, state.inequality_multipliers, state.current_penalty)
                                .value;
  } else {
    state.penalized_value = std::numeric_limits<double>::infinity();
  }
  detail::maybe_update_best_feasible_(state, options.constraint_tolerance);
  return state;
}

// One iteration of an augmented-Lagrangian method. Inner iterations minimize the augmented Lagrangian over
// the bounds with a projected BFGS step and an Armijo line search along the projected path. When the inner
// problem is solved to its tolerance, an outer iteration updates the multipliers and, if the constraint
// violation has not fallen enough, raises the penalty weight.
inline PenaltyMethodState penalty_method_step(const ConstrainedProblem& problem,
                                              const PenaltyMethodState& state,
                                              const PenaltyMethodOptions& options = {}) {
  PenaltyMethodState next = state;
  if (state.converged || state.evaluation.status != EvalStatus::Ok) return next;

  const auto platform = state.params.empty() ? pluss::codem::Platform::CPU : state.params[0].value.platform();
  const std::int64_t n = state.params.total_numel();
  const auto system = detail::assemble_constraint_system_(problem, state.params, state.evaluation);
  const auto inequalities = detail::non_bound_inequality_rows_(system);
  const Series equality_multiplier = detail::multiplier_or_zero_(
      state.equality_multipliers, static_cast<std::int64_t>(system.equalities.size()), platform);
  const Series inequality_multiplier = detail::multiplier_or_zero_(
      state.inequality_multipliers, static_cast<std::int64_t>(inequalities.size()), platform);
  const double rho = state.current_penalty;
  const auto current = detail::augmented_lagrangian_(
      state.evaluation, system.equalities, inequalities, equality_multiplier, inequality_multiplier, rho);
  const auto [lower, upper] = detail::flat_bound_vectors_(problem, state.params);
  const Series x = flatten_values(state.params).to(ScalarType::Float64).reshape({n});
  const double projected_norm = detail::projected_gradient_norm_(x, current.gradient, lower, upper);
  const double violation = state.evaluation.max_constraint_violation;

  next.equality_multipliers = equality_multiplier;
  next.inequality_multipliers = inequality_multiplier;
  next.penalized_value = current.value;
  next.stationarity = projected_norm;
  if (violation <= options.constraint_tolerance && projected_norm <= options.optimality_tolerance) {
    next.converged = true;
    next.message = "first-order and constraint tolerances satisfied";
    return next;
  }

  if (projected_norm <= state.inner_tolerance) {
    // Outer iteration: first-order multiplier update, then decide whether the penalty must grow.
    std::vector<double> new_equality(system.equalities.size(), 0.0);
    const Series old_equality = equality_multiplier.to(ScalarType::Float64);
    for (std::size_t j = 0; j < new_equality.size(); ++j) {
      new_equality[j] = old_equality.select(0, static_cast<std::int64_t>(j)).item<double>() +
                        rho * system.equalities[j].residual;
    }
    std::vector<double> new_inequality(inequalities.size(), 0.0);
    const Series old_inequality = inequality_multiplier.to(ScalarType::Float64);
    for (std::size_t i = 0; i < new_inequality.size(); ++i) {
      new_inequality[i] = std::max(0.0, old_inequality.select(0, static_cast<std::int64_t>(i)).item<double>() +
                                            rho * inequalities[i].residual);
    }
    next.equality_multipliers =
        pluss::make_series(new_equality, {static_cast<std::int64_t>(new_equality.size())}).to(ScalarType::Float64).to(platform);
    next.inequality_multipliers =
        pluss::make_series(new_inequality, {static_cast<std::int64_t>(new_inequality.size())}).to(ScalarType::Float64).to(platform);
    constexpr double penalty_cap = 1e12;
    const bool stalled_violation = violation > options.constraint_tolerance && violation > 0.25 * state.previous_violation;
    if (stalled_violation) next.current_penalty = std::min(10.0 * rho, penalty_cap);
    next.inner_tolerance = std::max(options.optimality_tolerance, 0.1 * state.inner_tolerance);
    next.previous_violation = violation;
    next.iteration += 1;
    next.last_step_size = 0.0;
    next.message = "multipliers updated";
    const auto updated = detail::augmented_lagrangian_(
        state.evaluation, system.equalities, inequalities, next.equality_multipliers,
        next.inequality_multipliers, next.current_penalty);
    next.penalized_value = updated.value;
    if (violation > options.constraint_tolerance && rho >= penalty_cap &&
        violation >= 0.99 * state.previous_violation) {
      next.converged = true;
      next.message = "constraint violation locally stationary";
    } else if (next.iteration >= options.max_iterations) {
      next.converged = true;
      next.message = "maximum iterations reached";
    }
    return next;
  }

  // Inner iteration: projected quasi-Newton direction on the variables that are free to move.
  const Series blocked_lower = x.le(lower.add(1e-12)).logical_and(current.gradient.gt(0.0));
  const Series blocked_upper = x.ge(upper.sub(1e-12)).logical_and(current.gradient.lt(0.0));
  const Series free_mask = blocked_lower.logical_or(blocked_upper).logical_not().to(ScalarType::Float64);
  const Series free_gradient = current.gradient.mul(free_mask);
  // `hessian` is the BFGS approximation of the Hessian; solve it restricted to the free variables.
  Series hessian = state.hessian.defined() ? state.hessian.to(ScalarType::Float64) : detail::identity_matrix_(n, platform);
  const Series row_mask = free_mask.reshape({n, 1});
  const Series column_mask = free_mask.reshape({1, n});
  const Series restricted = hessian.mul(row_mask).mul(column_mask)
                                .add(detail::identity_matrix_(n, platform).mul(free_mask.mul(-1.0).add(1.0).reshape({n, 1})));
  bool solve_ok = false;
  Series direction = detail::solve_dense_system_series_(
      restricted, free_gradient.mul(-1.0), 1e-10, OptimaLinearSystemSolver::Auto, 1e-12, solve_ok);
  if (solve_ok && direction.defined()) direction = direction.mul(free_mask);
  if (!solve_ok || !direction.defined() || !(detail::series_dot_(current.gradient, direction) < 0.0)) {
    hessian = detail::identity_matrix_(n, platform);
    direction = free_gradient.mul(-1.0);
  }
  if (!(detail::series_dot_(current.gradient, direction) < 0.0) || !std::isfinite(detail::series_inf_norm_(direction))) {
    next.converged = true;
    next.message = "line search failed to produce an acceptable step";
    return next;
  }

  const auto layout = state.params.layout();
  double step = state.iteration == 0 || !state.hessian.defined()
                    ? std::min(options.initial_step_size, 1.0 / std::max(1.0, detail::series_inf_norm_(direction)))
                    : 1.0;
  for (std::int64_t ls_iter = 0; ls_iter < options.max_line_search_iterations; ++ls_iter) {
    Series candidate = x.add(direction.mul(step));
    if (options.project_bounds) candidate = candidate.minimum(upper).maximum(lower);
    auto trial = unflatten_values(layout, candidate);
    auto trial_eval = evaluate_problem(problem, trial);
    next.function_evaluations += 1;
    if (trial_eval.status == EvalStatus::Ok) {
      const auto trial_system = detail::assemble_constraint_system_(problem, trial, trial_eval);
      const auto trial_inequalities = detail::non_bound_inequality_rows_(trial_system);
      const auto trial_al = detail::augmented_lagrangian_(
          trial_eval, trial_system.equalities, trial_inequalities, equality_multiplier, inequality_multiplier, rho);
      const Series moved = candidate.sub(x);
      if (std::isfinite(trial_al.value) &&
          trial_al.value <= current.value + options.armijo * detail::series_dot_(current.gradient, moved)) {
        next.params = std::move(trial);
        next.evaluation = std::move(trial_eval);
        next.iteration += 1;
        next.last_step_size = step;
        next.penalized_value = trial_al.value;
        next.stationarity = detail::projected_gradient_norm_(candidate, trial_al.gradient, lower, upper);
        next.message = "step accepted";
        const Series gradient_change = trial_al.gradient.sub(current.gradient);
        next.hessian = detail::bfgs_hessian_update_(hessian, moved, gradient_change, 1e-8);
        detail::maybe_update_best_feasible_(next, options.constraint_tolerance);
        if (next.iteration >= options.max_iterations) {
          next.converged = true;
          next.message = "maximum iterations reached";
        }
        return next;
      }
    }
    step *= options.backtracking;
  }

  next.hessian = detail::identity_matrix_(n, platform);
  if (state.hessian.defined() && detail::series_inf_norm_(state.hessian.sub(next.hessian)) > 0.0) {
    // The quasi-Newton model may be the problem: retry once from steepest descent.
    next.message = "line search restarted from steepest descent";
    next.iteration += 1;
    return next;
  }
  next.converged = true;
  next.message = "line search failed to produce an acceptable step";
  return next;
}

inline PenaltyMethodState solve_penalty_method(const ConstrainedProblem& problem,
                                               const ParameterPack<Series>& initial_params,
                                               const PenaltyMethodOptions& options = {}) {
  PenaltyMethodState state = init_penalty_method_state(problem, initial_params, options);
  std::string progress_message;
  if (detail::run_progress_hook_(
          options.progress_hook,
          detail::make_progress_snapshot_(OptimaAlgorithm::MeritMethod, OptimaProgressStage::Start, state),
          &progress_message)) {
    state.converged = true;
    state.message = progress_message.empty() ? "stopped by progress hook" : progress_message;
  }
  while (!state.converged && state.iteration < options.max_iterations) {
    PenaltyMethodState next = penalty_method_step(problem, state, options);
    if (next.iteration == state.iteration && next.message == state.message && next.converged == state.converged) {
      break;
    }
    state = std::move(next);
    progress_message.clear();
    if (detail::run_progress_hook_(
            options.progress_hook,
            detail::make_progress_snapshot_(OptimaAlgorithm::MeritMethod, OptimaProgressStage::Iteration, state),
            &progress_message)) {
      state.converged = true;
      state.message = progress_message.empty() ? "stopped by progress hook" : progress_message;
      break;
    }
  }
  if (!state.converged && state.iteration >= options.max_iterations) {
    state.converged = true;
    state.message = "maximum iterations reached";
  }
  detail::run_progress_hook_(
      options.progress_hook,
      detail::make_progress_snapshot_(OptimaAlgorithm::MeritMethod, OptimaProgressStage::Done, state));
  return state;
}

inline SequentialQPState init_sequential_qp_state(const ConstrainedProblem& problem,
                                                  const ParameterPack<Series>& initial_params,
                                                  const SequentialQPOptions& options = {}) {
  SequentialQPState state;
  if (!detail::supports_exact_curvature_(problem, options.hessian_approximation)) {
    state.params = initial_params;
    state.evaluation.status = EvalStatus::InvalidProblem;
    state.converged = true;
    state.message =
        "exact curvature requires an autodiff objective and autodiff nonlinear callbacks (hessian_approximation is Exact "
        "or HessianVector)";
    return state;
  }
  state.params = options.project_bounds ? detail::project_bounds_(initial_params, problem.bounds) : initial_params;
  state.evaluation = evaluate_problem(problem, state.params);
  state.function_evaluations = 1;
  state.current_penalty = std::max(options.initial_penalty, 1e-8);
  state.converged = state.evaluation.status != EvalStatus::Ok;
  state.message = state.converged ? state.evaluation.message : "initialized";
  state.hessian = detail::identity_matrix_(
      state.params.total_numel(),
      state.params.empty() ? pluss::codem::Platform::CPU : state.params[0].value.platform());
  state.variable_scale = detail::compute_variable_scale_(state.params, problem.bounds, options.problem_scaling);
  if (state.evaluation.status == EvalStatus::Ok) {
    const auto system = detail::assemble_constraint_system_(problem, state.params, state.evaluation);
    state.normalization = detail::compute_problem_normalization_(
        state.evaluation,
        system,
        options.problem_scaling,
        state.params.empty() ? pluss::codem::Platform::CPU : state.params[0].value.platform(),
        problem.bounds.lower.has_value() ? state.params.total_numel() : 0,
        problem.bounds.upper.has_value() ? state.params.total_numel() : 0);
    {
      const double g0 = detail::pack_inf_norm_(state.evaluation.gradient);
      state.gradient_scale = std::isfinite(g0) ? std::max(1.0, g0) : 1.0;
    }
    state.merit_value = detail::penalized_value_(state.evaluation, state.current_penalty, state.normalization);
    const auto active = detail::active_constraints_(system, options.active_set_tolerance);
    const auto normalized_active = detail::scale_constraint_rows_(
        active,
        pluss::series::undefined(),
        detail::active_constraint_row_scales_(system, active, state.normalization, state.params[0].value.platform()));
    state.kkt_residual = detail::kkt_residual_(
        flatten_values(state.evaluation.gradient).to(ScalarType::Float64).mul(state.normalization.objective_scale),
        normalized_active,
        state.multipliers);
  } else {
    state.merit_value = std::numeric_limits<double>::infinity();
    state.kkt_residual = std::numeric_limits<double>::infinity();
  }
  state.krylov_iterations = 0;
  detail::maybe_update_best_feasible_(state, options.constraint_tolerance);
  if (!state.converged && detail::feasible_objective_target_met_(state.evaluation, options.objective_limit)) {
    state.converged = true;
    state.message = "objective limit reached at a feasible point";
  }
  return state;
}

inline WorkingSetState init_working_set_state(const ConstrainedProblem& problem,
                                              const ParameterPack<Series>& initial_params,
                                              const WorkingSetOptions& options = {}) {
  SequentialQPState base = init_sequential_qp_state(problem, initial_params, options);
  WorkingSetState state;
  static_cast<SequentialQPState&>(state) = std::move(base);
  if (state.evaluation.status == EvalStatus::Ok) {
    const auto system = detail::assemble_constraint_system_(problem, state.params, state.evaluation);
    state.active_set = detail::seed_working_set_keys_(
        system,
        options.active_set_tolerance,
        options.add_constraint_tolerance,
        options.max_working_set_violations);
  }
  return state;
}

namespace detail {

// Dense linear solve with partial pivoting (in place on a copy). Returns false when singular.
inline bool dense_solve_(std::vector<double> A, std::vector<double>& b, std::int64_t n) {
  for (std::int64_t k = 0; k < n; ++k) {
    std::int64_t piv = k;
    double best = std::abs(A[static_cast<std::size_t>(k * n + k)]);
    for (std::int64_t i = k + 1; i < n; ++i) {
      const double v = std::abs(A[static_cast<std::size_t>(i * n + k)]);
      if (v > best) { best = v; piv = i; }
    }
    if (!(best > 1e-300)) return false;
    if (piv != k) {
      for (std::int64_t j = 0; j < n; ++j) std::swap(A[static_cast<std::size_t>(k * n + j)], A[static_cast<std::size_t>(piv * n + j)]);
      std::swap(b[static_cast<std::size_t>(k)], b[static_cast<std::size_t>(piv)]);
    }
    const double d = A[static_cast<std::size_t>(k * n + k)];
    for (std::int64_t i = k + 1; i < n; ++i) {
      const double f = A[static_cast<std::size_t>(i * n + k)] / d;
      if (f == 0.0) continue;
      for (std::int64_t j = k; j < n; ++j) A[static_cast<std::size_t>(i * n + j)] -= f * A[static_cast<std::size_t>(k * n + j)];
      b[static_cast<std::size_t>(i)] -= f * b[static_cast<std::size_t>(k)];
    }
  }
  for (std::int64_t i = n - 1; i >= 0; --i) {
    double s = b[static_cast<std::size_t>(i)];
    for (std::int64_t j = i + 1; j < n; ++j) s -= A[static_cast<std::size_t>(i * n + j)] * b[static_cast<std::size_t>(j)];
    b[static_cast<std::size_t>(i)] = s / A[static_cast<std::size_t>(i * n + i)];
  }
  for (double v : b) if (!std::isfinite(v)) return false;
  return true;
}

// Linear-algebra settings for the quadratic subproblem's Newton systems. The subproblem is reduced to an n x n
// symmetric positive-definite system (n = number of variables), so the same solver choices as the rest of Optima
// apply: a Cholesky factorization, a pivoted direct solve, an explicit (pseudo)inverse, or preconditioned
// conjugate gradients that never form the matrix.
struct QpLinearSolverSettings {
  OptimaLinearSystemSolver solver{OptimaLinearSystemSolver::Auto};
  OptimaLinearSystemStructure structure{OptimaLinearSystemStructure::Auto};
  double regularization{1e-6};  // first diagonal shift tried when a factorization fails; grown tenfold per retry
  double pivot_threshold{1e-12};
  std::int64_t max_krylov_iterations{0};
  std::int64_t iterative_dimension_threshold{3000};
  double krylov_tolerance{1e-2};
  double krylov_absolute_tolerance{1e-10};
  // Solve the unreduced (nz + me) KKT system by a pivoted LU instead of reducing it. Slower, but not squared in
  // the conditioning: the fallback when the reduced system is too ill-conditioned for the interior iteration.
  bool full_system{false};
  // Start the interior iteration on the central path (every complementarity product equal to 1) instead of with all
  // multipliers at 1. Used as a fallback: the default start can cycle on a subproblem whose bounds are far from the
  // iterate, and the centred start does not.
  bool centered_start{false};
};

// In-place Cholesky M = L L' (lower triangle of `a`); false when a pivot is not positive, or is smaller than
// `pivot_rel` times the diagonal entry it started as (a pivot that has cancelled to rounding level).
inline bool cholesky_factor_(std::vector<double>& a, std::int64_t n, double pivot_rel) {
  for (std::int64_t j = 0; j < n; ++j) {
    const double original = a[static_cast<std::size_t>(j * n + j)];
    const double pivot_floor = pivot_rel * std::abs(original);
    double d = original;
    for (std::int64_t k = 0; k < j; ++k) d -= a[static_cast<std::size_t>(j * n + k)] * a[static_cast<std::size_t>(j * n + k)];
    if (!(d > pivot_floor) || !std::isfinite(d)) return false;
    d = std::sqrt(d);
    a[static_cast<std::size_t>(j * n + j)] = d;
    for (std::int64_t i = j + 1; i < n; ++i) {
      double v = a[static_cast<std::size_t>(i * n + j)];
      for (std::int64_t k = 0; k < j; ++k) v -= a[static_cast<std::size_t>(i * n + k)] * a[static_cast<std::size_t>(j * n + k)];
      a[static_cast<std::size_t>(i * n + j)] = v / d;
    }
  }
  return true;
}

inline void cholesky_solve_(const std::vector<double>& l, std::int64_t n, std::vector<double>& b) {
  for (std::int64_t i = 0; i < n; ++i) {
    double v = b[static_cast<std::size_t>(i)];
    for (std::int64_t k = 0; k < i; ++k) v -= l[static_cast<std::size_t>(i * n + k)] * b[static_cast<std::size_t>(k)];
    b[static_cast<std::size_t>(i)] = v / l[static_cast<std::size_t>(i * n + i)];
  }
  for (std::int64_t i = n - 1; i >= 0; --i) {
    double v = b[static_cast<std::size_t>(i)];
    for (std::int64_t k = i + 1; k < n; ++k) v -= l[static_cast<std::size_t>(k * n + i)] * b[static_cast<std::size_t>(k)];
    b[static_cast<std::size_t>(i)] = v / l[static_cast<std::size_t>(i * n + i)];
  }
}

// Jacobi-preconditioned conjugate gradients for M x = b with M available only through `matvec`.
// Stops at max(absolute, relative * |b|_inf) in the infinity norm; false when it does not get there.
template <class MatVec>
inline bool pcg_solve_(MatVec&& matvec, const std::vector<double>& diag, const std::vector<double>& b,
                       std::vector<double>& x, std::int64_t max_iterations, double relative_tolerance,
                       double absolute_tolerance, std::int64_t& iterations) {
  const std::size_t n = b.size();
  x.assign(n, 0.0);
  std::vector<double> r = b, z(n), p(n), Ap(n);
  const auto inf_norm = [](const std::vector<double>& v) {
    double m = 0.0;
    for (double e : v) m = std::max(m, std::abs(e));
    return m;
  };
  const auto dot = [n](const std::vector<double>& a, const std::vector<double>& c) {
    double v = 0.0;
    for (std::size_t i = 0; i < n; ++i) v += a[i] * c[i];
    return v;
  };
  const double tol = std::max(absolute_tolerance, relative_tolerance * inf_norm(r));
  if (inf_norm(r) <= tol) return true;
  for (std::size_t i = 0; i < n; ++i) z[i] = r[i] / std::max(std::abs(diag[i]), 1e-12);
  p = z;
  double rz = dot(r, z);
  const std::int64_t budget = max_iterations > 0 ? max_iterations : std::max<std::int64_t>(10, 2 * static_cast<std::int64_t>(n));
  for (std::int64_t it = 0; it < budget; ++it) {
    matvec(p, Ap);
    const double curvature = dot(p, Ap);
    if (!(curvature > 1e-18) || !std::isfinite(curvature)) return false;
    const double alpha = rz / curvature;
    for (std::size_t i = 0; i < n; ++i) {
      x[i] += alpha * p[i];
      r[i] -= alpha * Ap[i];
    }
    iterations = it + 1;
    if (inf_norm(r) <= tol) return true;
    for (std::size_t i = 0; i < n; ++i) z[i] = r[i] / std::max(std::abs(diag[i]), 1e-12);
    const double new_rz = dot(r, z);
    if (!(new_rz > 0.0) || !std::isfinite(new_rz)) return false;
    const double beta = new_rz / std::max(rz, 1e-18);
    for (std::size_t i = 0; i < n; ++i) p[i] = z[i] + beta * p[i];
    rz = new_rz;
  }
  return false;
}

struct ElasticQpResult {
  bool ok{false};
  std::vector<double> d{};
  std::vector<double> eq_multipliers{};
  std::vector<double> ineq_multipliers{};
  double elastic_sum{0.0};
  double elastic_max{0.0};  // largest single elastic variable (the sum grows with the number of rows times mu)
  std::int64_t krylov_iterations{0};
};

// Solves   min 0.5 d'B d + g'd + rho*(sum p + sum n + sum t)
//          s.t. Ae d + h = p - n,   Ai d + c <= t,   p, n, t >= 0
// with a primal-dual interior method (Mehrotra predictor-corrector). The elastic variables make the
// subproblem feasible for any linearization, which is what lets the outer SQP iteration recover from
// infeasible or inconsistent constraint models. Rows are dense and row-major.
inline ElasticQpResult solve_elastic_qp_core_(const std::vector<double>& B,
                                         const std::vector<double>& g,
                                         const std::vector<double>& Ae, const std::vector<double>& h,
                                         const std::vector<double>& Ai, const std::vector<double>& c,
                                         double rho, const QpLinearSolverSettings& ls) {
  const std::int64_t n = static_cast<std::int64_t>(g.size());
  const std::int64_t me = static_cast<std::int64_t>(h.size());
  const std::int64_t mi = static_cast<std::int64_t>(c.size());
  const std::int64_t nz = n + 2 * me + mi;           // d, p, n, t
  const std::int64_t mg = mi + (2 * me + mi);        // Ai d - t <= -c ; -p,-n,-t <= 0
  ElasticQpResult res;

  // Q (dense nz x nz, only the d-block nonzero) and q.
  std::vector<double> q(static_cast<std::size_t>(nz), 0.0);
  for (std::int64_t i = 0; i < n; ++i) q[static_cast<std::size_t>(i)] = g[static_cast<std::size_t>(i)];
  for (std::int64_t i = n; i < nz; ++i) q[static_cast<std::size_t>(i)] = rho;
  // E z = r : Ae d - p + n = -h      and      G z <= w : Ai d - t <= -c ; -p, -n, -t <= 0.
  // Neither matrix is ever formed: both act through the rows of Ae and Ai and a few identity blocks, so the cost
  // of every product is the number of nonzeros of the constraint data, not (rows x variables).
  std::vector<double> r(static_cast<std::size_t>(me), 0.0), w(static_cast<std::size_t>(mg), 0.0);
  for (std::int64_t i = 0; i < me; ++i) r[static_cast<std::size_t>(i)] = -h[static_cast<std::size_t>(i)];
  for (std::int64_t i = 0; i < mi; ++i) w[static_cast<std::size_t>(i)] = -c[static_cast<std::size_t>(i)];
  // Nonzero columns of every inequality / equality row, fixed for the whole solve.
  const auto row_pattern = [n](const std::vector<double>& A, std::int64_t rows) {
    std::vector<std::vector<std::int64_t>> out(static_cast<std::size_t>(rows));
    for (std::int64_t k = 0; k < rows; ++k)
      for (std::int64_t j = 0; j < n; ++j)
        if (A[static_cast<std::size_t>(k * n + j)] != 0.0) out[static_cast<std::size_t>(k)].push_back(j);
    return out;
  };
  const auto pat_i = row_pattern(Ai, mi);
  const auto pat_e = row_pattern(Ae, me);
  const std::int64_t off_p = n, off_n = n + me, off_t = n + 2 * me;
  const auto Gmul = [&](const std::vector<double>& zz, std::vector<double>& out) {
    out.assign(static_cast<std::size_t>(mg), 0.0);
    for (std::int64_t i = 0; i < mi; ++i) {
      double v = 0.0;
      for (auto j : pat_i[static_cast<std::size_t>(i)]) v += Ai[static_cast<std::size_t>(i * n + j)] * zz[static_cast<std::size_t>(j)];
      out[static_cast<std::size_t>(i)] = v - zz[static_cast<std::size_t>(off_t + i)];
      out[static_cast<std::size_t>(mi + 2 * me + i)] = -zz[static_cast<std::size_t>(off_t + i)];
    }
    for (std::int64_t k = 0; k < me; ++k) {
      out[static_cast<std::size_t>(mi + k)] = -zz[static_cast<std::size_t>(off_p + k)];
      out[static_cast<std::size_t>(mi + me + k)] = -zz[static_cast<std::size_t>(off_n + k)];
    }
  };
  const auto Gt_add = [&](const std::vector<double>& lv, std::vector<double>& oz) {
    for (std::int64_t i = 0; i < mi; ++i) {
      const double l = lv[static_cast<std::size_t>(i)];
      if (l != 0.0)
        for (auto j : pat_i[static_cast<std::size_t>(i)]) oz[static_cast<std::size_t>(j)] += Ai[static_cast<std::size_t>(i * n + j)] * l;
      oz[static_cast<std::size_t>(off_t + i)] -= l + lv[static_cast<std::size_t>(mi + 2 * me + i)];
    }
    for (std::int64_t k = 0; k < me; ++k) {
      oz[static_cast<std::size_t>(off_p + k)] -= lv[static_cast<std::size_t>(mi + k)];
      oz[static_cast<std::size_t>(off_n + k)] -= lv[static_cast<std::size_t>(mi + me + k)];
    }
  };
  const auto Emul = [&](const std::vector<double>& zz, std::vector<double>& out) {
    out.assign(static_cast<std::size_t>(me), 0.0);
    for (std::int64_t k = 0; k < me; ++k) {
      double v = 0.0;
      for (auto j : pat_e[static_cast<std::size_t>(k)]) v += Ae[static_cast<std::size_t>(k * n + j)] * zz[static_cast<std::size_t>(j)];
      out[static_cast<std::size_t>(k)] = v - zz[static_cast<std::size_t>(off_p + k)] + zz[static_cast<std::size_t>(off_n + k)];
    }
  };
  const auto Et_add = [&](const std::vector<double>& yv, std::vector<double>& oz) {
    for (std::int64_t k = 0; k < me; ++k) {
      const double yk = yv[static_cast<std::size_t>(k)];
      for (auto j : pat_e[static_cast<std::size_t>(k)]) oz[static_cast<std::size_t>(j)] += Ae[static_cast<std::size_t>(k * n + j)] * yk;
      oz[static_cast<std::size_t>(off_p + k)] -= yk;
      oz[static_cast<std::size_t>(off_n + k)] += yk;
    }
  };

  std::vector<double> z(static_cast<std::size_t>(nz), 0.0), y(static_cast<std::size_t>(me), 0.0);
  for (std::int64_t i = 0; i < me; ++i) {
    z[static_cast<std::size_t>(n + i)] = std::abs(h[static_cast<std::size_t>(i)]) + 1.0;
    z[static_cast<std::size_t>(n + me + i)] = 1.0;
  }
  for (std::int64_t i = 0; i < mi; ++i) z[static_cast<std::size_t>(n + 2 * me + i)] = std::max(0.0, c[static_cast<std::size_t>(i)]) + 1.0;
  std::vector<double> s(static_cast<std::size_t>(mg), 1.0), lam(static_cast<std::size_t>(mg), 1.0);
  {
    std::vector<double> gz;
    Gmul(z, gz);
    for (std::int64_t i = 0; i < mg; ++i) s[static_cast<std::size_t>(i)] = std::max(1.0, w[static_cast<std::size_t>(i)] - gz[static_cast<std::size_t>(i)]);
    // With all multipliers at 1 a row whose slack is large (a bound far from the iterate) starts with a complementarity
    // product as large as its slack, and the predictor-corrector iteration can cycle without ever reducing it. The
    // centred start puts every product at 1.
    if (ls.centered_start)
      for (std::int64_t i = 0; i < mg; ++i) lam[static_cast<std::size_t>(i)] = 1.0 / s[static_cast<std::size_t>(i)];
  }

  const double reg = 1e-9;
  const bool use_cg = use_iterative_subproblem_solver_(ls.solver, ls.structure, n, ls.iterative_dimension_threshold);
  std::int64_t krylov_total = 0;
  double best_merit = std::numeric_limits<double>::infinity(), best_rn = 0.0, best_mu = 0.0;
  std::vector<double> best_z = z, best_y = y, best_s = s, best_lam = lam;
  bool converged_flag = false;
  for (int iter = 0; iter < 80; ++iter) {
    // residuals
    std::vector<double> rd(static_cast<std::size_t>(nz), 0.0), re(static_cast<std::size_t>(me), 0.0), rg(static_cast<std::size_t>(mg), 0.0);
    for (std::int64_t i = 0; i < n; ++i) {
      double v = q[static_cast<std::size_t>(i)];
      for (std::int64_t j = 0; j < n; ++j) v += B[static_cast<std::size_t>(i * n + j)] * z[static_cast<std::size_t>(j)];
      rd[static_cast<std::size_t>(i)] = v;
    }
    for (std::int64_t i = n; i < nz; ++i) rd[static_cast<std::size_t>(i)] = q[static_cast<std::size_t>(i)];
    Et_add(y, rd);
    Gt_add(lam, rd);
    {
      std::vector<double> ez, gz;
      Emul(z, ez);
      Gmul(z, gz);
      for (std::int64_t k = 0; k < me; ++k) re[static_cast<std::size_t>(k)] = ez[static_cast<std::size_t>(k)] - r[static_cast<std::size_t>(k)];
      for (std::int64_t k = 0; k < mg; ++k) rg[static_cast<std::size_t>(k)] = gz[static_cast<std::size_t>(k)] + s[static_cast<std::size_t>(k)] - w[static_cast<std::size_t>(k)];
    }
    double mu = 0.0;
    for (std::int64_t k = 0; k < mg; ++k) mu += s[static_cast<std::size_t>(k)] * lam[static_cast<std::size_t>(k)];
    mu = mg > 0 ? mu / static_cast<double>(mg) : 0.0;
    double rn = 0.0;
    for (double v : rd) rn = std::max(rn, std::abs(v));
    for (double v : re) rn = std::max(rn, std::abs(v));
    for (double v : rg) rn = std::max(rn, std::abs(v));
    if (rn + mu < best_merit) {
      best_merit = rn + mu; best_rn = rn; best_mu = mu;
      best_z = z; best_y = y; best_s = s; best_lam = lam;
    }
    if (rn < 1e-9 && mu < 1e-10) { converged_flag = true; break; }

    // Reduced Newton system. Eliminating the elastic variables p, n, t and the equality multiplier from the
    // KKT system leaves the n x n symmetric positive-definite matrix
    //     M = B + reg I + Ai' W Ai + Ae' C^-1 Ae,
    // W = sA sT / (sA + sT + reg) per inequality row, C = 1/sP + 1/sN + reg per equality row (sA, sT, sP, sN are
    // the barrier weights lam/s of the rows  Ai d - t <= -c,  -t <= 0,  -p <= 0,  -n <= 0).
    std::vector<double> sA(static_cast<std::size_t>(mi)), Htt(static_cast<std::size_t>(mi)), Wd(static_cast<std::size_t>(mi));
    std::vector<double> Hpp(static_cast<std::size_t>(me)), Hnn(static_cast<std::size_t>(me)), Cinv(static_cast<std::size_t>(me));
    for (std::int64_t k = 0; k < mi; ++k) {
      const double sa = lam[static_cast<std::size_t>(k)] / s[static_cast<std::size_t>(k)];
      const double st = lam[static_cast<std::size_t>(mi + 2 * me + k)] / s[static_cast<std::size_t>(mi + 2 * me + k)];
      sA[static_cast<std::size_t>(k)] = sa;
      Htt[static_cast<std::size_t>(k)] = sa + st + reg;
      Wd[static_cast<std::size_t>(k)] = sa * (st + reg) / (sa + st + reg);
    }
    for (std::int64_t k = 0; k < me; ++k) {
      Hpp[static_cast<std::size_t>(k)] = lam[static_cast<std::size_t>(mi + k)] / s[static_cast<std::size_t>(mi + k)] + reg;
      Hnn[static_cast<std::size_t>(k)] = lam[static_cast<std::size_t>(mi + me + k)] / s[static_cast<std::size_t>(mi + me + k)] + reg;
      Cinv[static_cast<std::size_t>(k)] = 1.0 / (1.0 / Hpp[static_cast<std::size_t>(k)] + 1.0 / Hnn[static_cast<std::size_t>(k)] + reg);
    }
    // y = M v without forming M (conjugate gradients) and the dense M (factorizations).
    const auto apply_M = [&](const std::vector<double>& v, std::vector<double>& out) {
      for (std::int64_t i = 0; i < n; ++i) {
        double acc = reg * v[static_cast<std::size_t>(i)];
        for (std::int64_t j = 0; j < n; ++j) acc += B[static_cast<std::size_t>(i * n + j)] * v[static_cast<std::size_t>(j)];
        out[static_cast<std::size_t>(i)] = acc;
      }
      for (std::int64_t k = 0; k < mi; ++k) {
        double dot = 0.0;
        for (auto j : pat_i[static_cast<std::size_t>(k)]) dot += Ai[static_cast<std::size_t>(k * n + j)] * v[static_cast<std::size_t>(j)];
        dot *= Wd[static_cast<std::size_t>(k)];
        for (auto j : pat_i[static_cast<std::size_t>(k)]) out[static_cast<std::size_t>(j)] += Ai[static_cast<std::size_t>(k * n + j)] * dot;
      }
      for (std::int64_t k = 0; k < me; ++k) {
        double dot = 0.0;
        for (auto j : pat_e[static_cast<std::size_t>(k)]) dot += Ae[static_cast<std::size_t>(k * n + j)] * v[static_cast<std::size_t>(j)];
        dot *= Cinv[static_cast<std::size_t>(k)];
        for (auto j : pat_e[static_cast<std::size_t>(k)]) out[static_cast<std::size_t>(j)] += Ae[static_cast<std::size_t>(k * n + j)] * dot;
      }
    };
    std::vector<double> Md(static_cast<std::size_t>(n * n), 0.0), Mdiag(static_cast<std::size_t>(n), 0.0);
    for (std::int64_t i = 0; i < n; ++i)
      for (std::int64_t j = 0; j < n; ++j) Md[static_cast<std::size_t>(i * n + j)] = B[static_cast<std::size_t>(i * n + j)];
    for (std::int64_t i = 0; i < n; ++i) Md[static_cast<std::size_t>(i * n + i)] += reg;
    for (std::int64_t k = 0; k < mi; ++k)
      for (auto a_ : pat_i[static_cast<std::size_t>(k)]) for (auto b_ : pat_i[static_cast<std::size_t>(k)])
        Md[static_cast<std::size_t>(a_ * n + b_)] += Wd[static_cast<std::size_t>(k)] * Ai[static_cast<std::size_t>(k * n + a_)] * Ai[static_cast<std::size_t>(k * n + b_)];
    for (std::int64_t k = 0; k < me; ++k)
      for (auto a_ : pat_e[static_cast<std::size_t>(k)]) for (auto b_ : pat_e[static_cast<std::size_t>(k)])
        Md[static_cast<std::size_t>(a_ * n + b_)] += Cinv[static_cast<std::size_t>(k)] * Ae[static_cast<std::size_t>(k * n + a_)] * Ae[static_cast<std::size_t>(k * n + b_)];
    for (std::int64_t i = 0; i < n; ++i) Mdiag[static_cast<std::size_t>(i)] = Md[static_cast<std::size_t>(i * n + i)];

    // Factor once per interior iteration; the predictor and corrector share it.
    // The matrix is equilibrated (D M D with D = diag(M)^-1/2) before it is factored: the barrier weights make the
    // diagonal span many orders of magnitude, and the symmetric scaling removes that from the conditioning at the
    // cost of O(n^2). The solution is mapped back through D.
    std::vector<double> chol, equilibration(static_cast<std::size_t>(n), 1.0);
    bool chol_ok = false;
    const bool wants_cholesky = !use_cg && (ls.solver == OptimaLinearSystemSolver::Auto || ls.solver == OptimaLinearSystemSolver::Cholesky);
    if (wants_cholesky && n > 0) {
      for (std::int64_t i = 0; i < n; ++i)
        if (Mdiag[static_cast<std::size_t>(i)] > 0.0) equilibration[static_cast<std::size_t>(i)] = 1.0 / std::sqrt(Mdiag[static_cast<std::size_t>(i)]);
      const double floor = std::max(ls.pivot_threshold, 0.0);
      double shift = 0.0;
      for (int attempt = 0; attempt < 12 && !chol_ok; ++attempt) {
        chol = Md;
        for (std::int64_t i = 0; i < n; ++i)
          for (std::int64_t j = 0; j < n; ++j)
            chol[static_cast<std::size_t>(i * n + j)] *= equilibration[static_cast<std::size_t>(i)] * equilibration[static_cast<std::size_t>(j)];
        for (std::int64_t i = 0; i < n; ++i) chol[static_cast<std::size_t>(i * n + i)] += shift;
        if (n >= 64) {
          // Beyond a few dozen variables the factorization is done by the library's blocked LAPACK routine (an order
          // of magnitude faster than the scalar loops); a matrix that is not positive definite makes it throw.
          try {
            const Series factor = pluss::subsp::math::linalg::cholesky(
                pluss::make_series(chol, {n, n}).to(ScalarType::Float64), false);
            std::vector<double> l = factor.to(pluss::codem::Platform::CPU).host<double>().array().data;
            chol_ok = static_cast<std::int64_t>(l.size()) == n * n;
            for (std::int64_t i = 0; i < n && chol_ok; ++i) {
              const double dgl = l[static_cast<std::size_t>(i * n + i)];
              if (!(dgl > 0.0) || !std::isfinite(dgl)) chol_ok = false;
            }
            if (chol_ok) chol = std::move(l);
          } catch (...) {
            chol_ok = false;
          }
        } else {
          chol_ok = cholesky_factor_(chol, n, floor);
        }
        shift = shift == 0.0 ? std::max(ls.regularization, 1e-12) : shift * 10.0;
      }
    }

    // Solves M x = rhs by the configured method; false only if every route fails.
    const auto solve_reduced = [&](std::vector<double> rhs_red, std::vector<double>& x) {
      if (n == 0) { x.clear(); return true; }
      if (use_cg) {
        std::int64_t its = 0;
        // The interior iteration drives its residuals to ~1e-9, so a loose inner tolerance would stall it; cap it
        // the same way the interior-point solver does.
        const bool ok = pcg_solve_(apply_M, Mdiag, rhs_red, x, ls.max_krylov_iterations,
                                   std::min(ls.krylov_tolerance, 1e-8), std::min(ls.krylov_absolute_tolerance, 1e-12), its);
        krylov_total += its;
        if (ok) return true;
      } else if (chol_ok) {
        x = rhs_red;
        for (std::int64_t i = 0; i < n; ++i) x[static_cast<std::size_t>(i)] *= equilibration[static_cast<std::size_t>(i)];
        cholesky_solve_(chol, n, x);
        for (std::int64_t i = 0; i < n; ++i) x[static_cast<std::size_t>(i)] *= equilibration[static_cast<std::size_t>(i)];
        return true;
      } else if (!wants_cholesky) {
        bool ok = false;
        const Series sol = solve_dense_system_series_(pluss::make_series(Md, {n, n}).to(ScalarType::Float64),
                                                       pluss::make_series(rhs_red, {n}).to(ScalarType::Float64),
                                                       ls.regularization, ls.solver, ls.pivot_threshold, ok);
        if (ok) { x = sol.to(pluss::codem::Platform::CPU).host<double>().array().data; return true; }
      }
      x = rhs_red;
      std::vector<double> A = Md;
      return dense_solve_(A, x, n);
    };

    // Solves the full Newton system  [H E'; E -reg][dz; dy] = rhs  through the reduced n x n matrix.
    // Dense unreduced KKT matrix, built only when the full-system path is requested.
    std::vector<double> Kfull;
    if (ls.full_system) {
      const std::int64_t kn = nz + me;
      Kfull.assign(static_cast<std::size_t>(kn * kn), 0.0);
      for (std::int64_t i = 0; i < n; ++i)
        for (std::int64_t j = 0; j < n; ++j) Kfull[static_cast<std::size_t>(i * kn + j)] = B[static_cast<std::size_t>(i * n + j)];
      for (std::int64_t i = 0; i < nz; ++i) Kfull[static_cast<std::size_t>(i * kn + i)] += reg;
      for (std::int64_t k = 0; k < mi; ++k) {
        const double sa = sA[static_cast<std::size_t>(k)];
        const double st = lam[static_cast<std::size_t>(mi + 2 * me + k)] / s[static_cast<std::size_t>(mi + 2 * me + k)];
        for (auto a_ : pat_i[static_cast<std::size_t>(k)]) {
          for (auto b_ : pat_i[static_cast<std::size_t>(k)])
            Kfull[static_cast<std::size_t>(a_ * kn + b_)] += sa * Ai[static_cast<std::size_t>(k * n + a_)] * Ai[static_cast<std::size_t>(k * n + b_)];
          Kfull[static_cast<std::size_t>(a_ * kn + off_t + k)] -= sa * Ai[static_cast<std::size_t>(k * n + a_)];
          Kfull[static_cast<std::size_t>((off_t + k) * kn + a_)] -= sa * Ai[static_cast<std::size_t>(k * n + a_)];
        }
        Kfull[static_cast<std::size_t>((off_t + k) * kn + off_t + k)] += sa + st;
      }
      for (std::int64_t k = 0; k < me; ++k) {
        Kfull[static_cast<std::size_t>((off_p + k) * kn + off_p + k)] += lam[static_cast<std::size_t>(mi + k)] / s[static_cast<std::size_t>(mi + k)];
        Kfull[static_cast<std::size_t>((off_n + k) * kn + off_n + k)] += lam[static_cast<std::size_t>(mi + me + k)] / s[static_cast<std::size_t>(mi + me + k)];
        for (auto j : pat_e[static_cast<std::size_t>(k)]) {
          const double v = Ae[static_cast<std::size_t>(k * n + j)];
          Kfull[static_cast<std::size_t>(j * kn + nz + k)] = v;
          Kfull[static_cast<std::size_t>((nz + k) * kn + j)] = v;
        }
        Kfull[static_cast<std::size_t>(off_p + k) * static_cast<std::size_t>(kn) + static_cast<std::size_t>(nz + k)] = -1.0;
        Kfull[static_cast<std::size_t>(nz + k) * static_cast<std::size_t>(kn) + static_cast<std::size_t>(off_p + k)] = -1.0;
        Kfull[static_cast<std::size_t>(off_n + k) * static_cast<std::size_t>(kn) + static_cast<std::size_t>(nz + k)] = 1.0;
        Kfull[static_cast<std::size_t>(nz + k) * static_cast<std::size_t>(kn) + static_cast<std::size_t>(off_n + k)] = 1.0;
        Kfull[static_cast<std::size_t>((nz + k) * kn + nz + k)] = -reg;
      }
    }
    const auto newton_step = [&](const std::vector<double>& rhs, std::vector<double>& dz, std::vector<double>& dy) {
      if (ls.full_system) {
        const std::int64_t kn = nz + me;
        std::vector<double> sol = rhs;
        if (!dense_solve_(Kfull, sol, kn)) return false;
        dz.assign(sol.begin(), sol.begin() + nz);
        dy.assign(sol.begin() + nz, sol.end());
        return true;
      }
      // Condense onto d.
      std::vector<double> red(rhs.begin(), rhs.begin() + n);
      std::vector<double> ryp(static_cast<std::size_t>(me));
      for (std::int64_t k = 0; k < mi; ++k) {
        const double t_term = sA[static_cast<std::size_t>(k)] * rhs[static_cast<std::size_t>(n + 2 * me + k)] / Htt[static_cast<std::size_t>(k)];
        for (auto j : pat_i[static_cast<std::size_t>(k)]) red[static_cast<std::size_t>(j)] += Ai[static_cast<std::size_t>(k * n + j)] * t_term;
      }
      for (std::int64_t k = 0; k < me; ++k) {
        ryp[static_cast<std::size_t>(k)] = rhs[static_cast<std::size_t>(nz + k)] + rhs[static_cast<std::size_t>(n + k)] / Hpp[static_cast<std::size_t>(k)] -
                                           rhs[static_cast<std::size_t>(n + me + k)] / Hnn[static_cast<std::size_t>(k)];
        const double e_term = Cinv[static_cast<std::size_t>(k)] * ryp[static_cast<std::size_t>(k)];
        for (auto j : pat_e[static_cast<std::size_t>(k)]) red[static_cast<std::size_t>(j)] += Ae[static_cast<std::size_t>(k * n + j)] * e_term;
      }
      std::vector<double> dd;
      if (!solve_reduced(red, dd)) return false;
      for (double v : dd) if (!std::isfinite(v)) return false;
      // Expand to (d, p, n, t) and the equality multiplier.
      dz.assign(static_cast<std::size_t>(nz), 0.0);
      dy.assign(static_cast<std::size_t>(me), 0.0);
      for (std::int64_t j = 0; j < n; ++j) dz[static_cast<std::size_t>(j)] = dd[static_cast<std::size_t>(j)];
      for (std::int64_t k = 0; k < me; ++k) {
        double ad = 0.0;
        for (auto j : pat_e[static_cast<std::size_t>(k)]) ad += Ae[static_cast<std::size_t>(k * n + j)] * dd[static_cast<std::size_t>(j)];
        const double dyk = Cinv[static_cast<std::size_t>(k)] * (ad - ryp[static_cast<std::size_t>(k)]);
        dy[static_cast<std::size_t>(k)] = dyk;
        dz[static_cast<std::size_t>(n + k)] = (rhs[static_cast<std::size_t>(n + k)] + dyk) / Hpp[static_cast<std::size_t>(k)];
        dz[static_cast<std::size_t>(n + me + k)] = (rhs[static_cast<std::size_t>(n + me + k)] - dyk) / Hnn[static_cast<std::size_t>(k)];
      }
      for (std::int64_t k = 0; k < mi; ++k) {
        double ad = 0.0;
        for (auto j : pat_i[static_cast<std::size_t>(k)]) ad += Ai[static_cast<std::size_t>(k * n + j)] * dd[static_cast<std::size_t>(j)];
        dz[static_cast<std::size_t>(n + 2 * me + k)] = (rhs[static_cast<std::size_t>(n + 2 * me + k)] + sA[static_cast<std::size_t>(k)] * ad) / Htt[static_cast<std::size_t>(k)];
      }
      return true;
    };
    // Residual of the unreduced Newton system, used to refine the step: the reduced matrix squares the
    // conditioning of the late interior iterations, and one or two refinement passes against the full equations
    // restore the accuracy of the original augmented system.
    const auto apply_K = [&](const std::vector<double>& dz, const std::vector<double>& dy, std::vector<double>& oz, std::vector<double>& oy) {
      oz.assign(static_cast<std::size_t>(nz), 0.0);
      for (std::int64_t i = 0; i < n; ++i) {
        double acc = 0.0;
        for (std::int64_t j = 0; j < n; ++j) acc += B[static_cast<std::size_t>(i * n + j)] * dz[static_cast<std::size_t>(j)];
        oz[static_cast<std::size_t>(i)] = acc;
      }
      for (std::int64_t j = 0; j < nz; ++j) oz[static_cast<std::size_t>(j)] += reg * dz[static_cast<std::size_t>(j)];
      std::vector<double> gv;
      Gmul(dz, gv);
      for (std::int64_t k = 0; k < mg; ++k) gv[static_cast<std::size_t>(k)] *= lam[static_cast<std::size_t>(k)] / s[static_cast<std::size_t>(k)];
      Gt_add(gv, oz);
      Et_add(dy, oz);
      Emul(dz, oy);
      for (std::int64_t k = 0; k < me; ++k) oy[static_cast<std::size_t>(k)] -= reg * dy[static_cast<std::size_t>(k)];
    };
    const auto refined_step = [&](const std::vector<double>& rhs, std::vector<double>& dz, std::vector<double>& dy) {
      if (!newton_step(rhs, dz, dy)) return false;
      double rhs_norm = 0.0;
      for (double v : rhs) rhs_norm = std::max(rhs_norm, std::abs(v));
      std::vector<double> oz, oy, res(rhs.size()), cz, cy;
      for (int pass = 0; pass < 2; ++pass) {
        apply_K(dz, dy, oz, oy);
        double rn = 0.0;
        for (std::int64_t j = 0; j < nz; ++j) { res[static_cast<std::size_t>(j)] = rhs[static_cast<std::size_t>(j)] - oz[static_cast<std::size_t>(j)]; rn = std::max(rn, std::abs(res[static_cast<std::size_t>(j)])); }
        for (std::int64_t k = 0; k < me; ++k) { res[static_cast<std::size_t>(nz + k)] = rhs[static_cast<std::size_t>(nz + k)] - oy[static_cast<std::size_t>(k)]; rn = std::max(rn, std::abs(res[static_cast<std::size_t>(nz + k)])); }
        if (!(rn > 1e-13 * std::max(rhs_norm, 1e-300))) break;
        if (!newton_step(res, cz, cy)) break;
        for (std::int64_t j = 0; j < nz; ++j) dz[static_cast<std::size_t>(j)] += cz[static_cast<std::size_t>(j)];
        for (std::int64_t k = 0; k < me; ++k) dy[static_cast<std::size_t>(k)] += cy[static_cast<std::size_t>(k)];
      }
      return true;
    };

    auto solve_dir = [&](const std::vector<double>& rc, std::vector<double>& dz, std::vector<double>& dy,
                         std::vector<double>& ds, std::vector<double>& dl) {
      // Full-system right-hand side, as before: rhs_z = -rd - G'((-rc + lam*rg)/s), rhs_y = -re.
      std::vector<double> rhs(static_cast<std::size_t>(nz + me), 0.0);
      for (std::int64_t j = 0; j < nz; ++j) rhs[static_cast<std::size_t>(j)] = -rd[static_cast<std::size_t>(j)];
      {
        std::vector<double> coef(static_cast<std::size_t>(mg)), gt(static_cast<std::size_t>(nz), 0.0);
        for (std::int64_t k = 0; k < mg; ++k)
          coef[static_cast<std::size_t>(k)] = (-rc[static_cast<std::size_t>(k)] + lam[static_cast<std::size_t>(k)] * rg[static_cast<std::size_t>(k)]) / s[static_cast<std::size_t>(k)];
        Gt_add(coef, gt);
        for (std::int64_t j = 0; j < nz; ++j) rhs[static_cast<std::size_t>(j)] -= gt[static_cast<std::size_t>(j)];
      }
      for (std::int64_t k = 0; k < me; ++k) rhs[static_cast<std::size_t>(nz + k)] = -re[static_cast<std::size_t>(k)];
      if (!refined_step(rhs, dz, dy)) return false;
      ds.assign(static_cast<std::size_t>(mg), 0.0);
      dl.assign(static_cast<std::size_t>(mg), 0.0);
      std::vector<double> gdz;
      Gmul(dz, gdz);
      for (std::int64_t k = 0; k < mg; ++k) {
        ds[static_cast<std::size_t>(k)] = -rg[static_cast<std::size_t>(k)] - gdz[static_cast<std::size_t>(k)];
        dl[static_cast<std::size_t>(k)] = (-rc[static_cast<std::size_t>(k)] - lam[static_cast<std::size_t>(k)] * ds[static_cast<std::size_t>(k)]) / s[static_cast<std::size_t>(k)];
      }
      return true;
    };
    auto max_step = [&](const std::vector<double>& ds, const std::vector<double>& dl) {
      double a = 1.0;
      for (std::int64_t k = 0; k < mg; ++k) {
        if (ds[static_cast<std::size_t>(k)] < 0.0) a = std::min(a, -s[static_cast<std::size_t>(k)] / ds[static_cast<std::size_t>(k)]);
        if (dl[static_cast<std::size_t>(k)] < 0.0) a = std::min(a, -lam[static_cast<std::size_t>(k)] / dl[static_cast<std::size_t>(k)]);
      }
      return a;
    };

    std::vector<double> rc(static_cast<std::size_t>(mg)), dz, dy, ds, dl;
    for (std::int64_t k = 0; k < mg; ++k) rc[static_cast<std::size_t>(k)] = s[static_cast<std::size_t>(k)] * lam[static_cast<std::size_t>(k)];
    if (!solve_dir(rc, dz, dy, ds, dl)) return res;
    const double a_aff = max_step(ds, dl);
    double mu_aff = 0.0;
    for (std::int64_t k = 0; k < mg; ++k)
      mu_aff += (s[static_cast<std::size_t>(k)] + a_aff * ds[static_cast<std::size_t>(k)]) * (lam[static_cast<std::size_t>(k)] + a_aff * dl[static_cast<std::size_t>(k)]);
    mu_aff = mg > 0 ? mu_aff / static_cast<double>(mg) : 0.0;
    const double sigma = mu > 0.0 ? std::pow(mu_aff / mu, 3) : 0.0;
    for (std::int64_t k = 0; k < mg; ++k)
      rc[static_cast<std::size_t>(k)] = s[static_cast<std::size_t>(k)] * lam[static_cast<std::size_t>(k)] + ds[static_cast<std::size_t>(k)] * dl[static_cast<std::size_t>(k)] - sigma * mu;
    if (!solve_dir(rc, dz, dy, ds, dl)) return res;
    const double alpha = std::min(1.0, 0.995 * max_step(ds, dl));
    if (alpha < 1e-10) break;  // no further progress is possible at this precision
    for (std::int64_t j = 0; j < nz; ++j) z[static_cast<std::size_t>(j)] += alpha * dz[static_cast<std::size_t>(j)];
    for (std::int64_t k = 0; k < me; ++k) y[static_cast<std::size_t>(k)] += alpha * dy[static_cast<std::size_t>(k)];
    for (std::int64_t k = 0; k < mg; ++k) {
      s[static_cast<std::size_t>(k)] += alpha * ds[static_cast<std::size_t>(k)];
      lam[static_cast<std::size_t>(k)] += alpha * dl[static_cast<std::size_t>(k)];
    }
  }
  if (!converged_flag) { z = best_z; y = best_y; s = best_s; lam = best_lam; }
  res.krylov_iterations = krylov_total;
  // If the iteration stalled short of the strict test (the late Newton systems carry rounding noise of size about
  // eps times the largest barrier weight), the best iterate it passed through is still an accurate step.
  res.ok = converged_flag || (best_rn < 1e-7 && best_mu < 1e-6);
  res.d.assign(z.begin(), z.begin() + n);
  res.eq_multipliers = y;
  res.ineq_multipliers.assign(lam.begin(), lam.begin() + mi);
  for (std::int64_t i = n; i < nz; ++i) {
    res.elastic_sum += z[static_cast<std::size_t>(i)];
    res.elastic_max = std::max(res.elastic_max, z[static_cast<std::size_t>(i)]);
  }
  for (double v : res.d) if (!std::isfinite(v)) res.ok = false;
  return res;
}

inline ElasticQpResult solve_elastic_qp_(const std::vector<double>& B,
                                         const std::vector<double>& g,
                                         const std::vector<double>& Ae, const std::vector<double>& h,
                                         const std::vector<double>& Ai, const std::vector<double>& c,
                                         double rho, const QpLinearSolverSettings& ls = {}) {
  // Normalize the objective so multipliers are O(1); this keeps the interior iteration well conditioned
  // when the gradient or curvature is many orders of magnitude from the constraint data.
  double sc = std::max(1.0, rho);
  for (double v : g) sc = std::max(sc, std::abs(v));
  for (double v : B) sc = std::max(sc, std::abs(v));
  std::vector<double> Bs(B), gs(g);
  for (double& v : Bs) v /= sc;
  for (double& v : gs) v /= sc;
  // Give every constraint row unit infinity norm so rows of wildly different magnitude condition the
  // interior iteration equally; multipliers are mapped back to the original row scaling afterwards.
  const std::size_t n = g.size();
  const auto normalize_rows = [n](const std::vector<double>& A, const std::vector<double>& r,
                                  std::vector<double>& As, std::vector<double>& rs, std::vector<double>& scale) {
    const std::size_t m = r.size();
    As = A; rs = r; scale.assign(m, 1.0);
    for (std::size_t k = 0; k < m; ++k) {
      double nrm = 0.0;
      for (std::size_t j = 0; j < n; ++j) nrm = std::max(nrm, std::abs(A[k * n + j]));
      if (nrm > 0.0 && std::isfinite(nrm)) {
        scale[k] = nrm;
        for (std::size_t j = 0; j < n; ++j) As[k * n + j] /= nrm;
        rs[k] /= nrm;
      }
    }
  };
  std::vector<double> Aes, hs, es, Ais, cs, is;
  normalize_rows(Ae, h, Aes, hs, es);
  normalize_rows(Ai, c, Ais, cs, is);
  ElasticQpResult res = solve_elastic_qp_core_(Bs, gs, Aes, hs, Ais, cs, rho / sc, ls);
  if (!res.ok && !ls.full_system && static_cast<std::int64_t>(g.size() + 3 * h.size() + c.size()) <= 700) {
    // The reduced system squares the conditioning of the late interior iterations (and conjugate gradients can
    // stall on it). The unreduced system, solved by a pivoted LU, is the fallback, so the linear-solver option
    // never turns a solvable subproblem into a failure.
    QpLinearSolverSettings direct = ls;
    direct.solver = OptimaLinearSystemSolver::Cholesky;
    direct.structure = OptimaLinearSystemStructure::Dense;
    direct.full_system = true;
    const std::int64_t spent = res.krylov_iterations;
    res = solve_elastic_qp_core_(Bs, gs, Aes, hs, Ais, cs, rho / sc, direct);
    res.krylov_iterations += spent;
  }
  if (!res.ok) {
    // The iteration stalled short of convergence (complementarity cycling, not conditioning). Restart it from the
    // centred point; subproblems that already solved are never affected.
    QpLinearSolverSettings centred = ls;
    centred.centered_start = true;
    const std::int64_t spent = res.krylov_iterations;
    res = solve_elastic_qp_core_(Bs, gs, Aes, hs, Ais, cs, rho / sc, centred);
    res.krylov_iterations += spent;
  }
  for (std::size_t k = 0; k < res.eq_multipliers.size(); ++k) res.eq_multipliers[k] *= sc / es[k];
  for (std::size_t k = 0; k < res.ineq_multipliers.size(); ++k) res.ineq_multipliers[k] *= sc / is[k];
  return res;
}

inline std::vector<double> series_to_doubles_(const Series& s) {
  if (!s.defined() || s.numel() == 0) return {};
  return s.to(ScalarType::Float64).to(pluss::codem::Platform::CPU).reshape({s.numel()}).host<double>();
}

inline double l1_violation_(const ProblemEvaluation& e) {
  double v = 0.0;
  for (double x : series_to_doubles_(e.equality_residual)) v += std::abs(x);
  for (double x : series_to_doubles_(e.inequality_residual)) v += std::max(0.0, x);
  return v;
}


// Curvature models shared by the quadratic-model algorithms (sequential QP and the working set).
//
// Exact: the Hessian of the Lagrangian  f + sum lambda_i c_i  at the current point, with the multipliers of the
// previous subproblem held fixed, by forward-over-reverse Hessian-vector products against the unit vectors.
// It is symmetrized and, where the subproblem would not be convex, a multiple of the identity is added (Nocedal and
// Wright, Section 18.5). What the subproblem needs is positive curvature on the directions the active constraints
// leave free, not on the whole space, so the test is made on  H + rho sum a_i a_i'  over the active rows (for a large
// enough rho this is positive definite exactly when H is positive definite on the null space of the rows): the
// smallest shift found by trying Cholesky factorizations of that matrix with a growing shift is added to H. A full
// Newton step is kept whenever the reduced curvature is already positive.
inline Series exact_lagrangian_model_(const ConstrainedProblem& problem,
                                      const ParameterPack<Series>& params,
                                      const FlatConstraintSystem& system,
                                      const std::vector<double>& equality_multipliers,
                                      const std::vector<double>& inequality_multipliers,
                                      const std::vector<const FlatConstraintRow*>& active_rows,
                                      const pluss::subsp::math::diff::HvpOptions& hvp_options,
                                      bool& ok) {
  const std::int64_t n = params.total_numel();
  const auto platform = params.empty() ? pluss::codem::Platform::CPU : params[0].value.platform();
  std::int64_t linear_equality_rows = 0;
  for (const auto& row : system.equalities)
    if (row.source == ConstraintSource::LinearEquality) ++linear_equality_rows;
  std::int64_t nonlinear_offset = 0;
  for (const auto& row : system.inequalities) {
    if (row.source == ConstraintSource::NonlinearInequality) break;
    ++nonlinear_offset;
  }
  std::vector<double> eq(equality_multipliers), ineq(inequality_multipliers);
  eq.resize(system.equalities.size(), 0.0);
  ineq.resize(system.inequalities.size(), 0.0);
  const std::vector<double> eq_scale(system.equalities.size(), 1.0);
  const Series eq_dual = pluss::make_series(eq, {static_cast<std::int64_t>(eq.size())}).to(ScalarType::Float64).to(platform);
  const Series ineq_dual = pluss::make_series(ineq, {static_cast<std::int64_t>(ineq.size())}).to(ScalarType::Float64).to(platform);
  const OptimaNormalizationStats unscaled{};
  Series H = assemble_lagrangian_hessian_(problem, params, eq_dual, eq_scale, ineq_dual, linear_equality_rows,
                                          nonlinear_offset, unscaled, hvp_options, ok);
  if (!ok) return H;
  H = H.add(H.transpose(0, 1)).mul(0.5);
  std::vector<double> a = series_to_doubles_(H);
  double scale = 0.0;
  for (std::int64_t i = 0; i < n; ++i) scale = std::max(scale, std::abs(a[static_cast<std::size_t>(i * n + i)]));
  for (double v : a) if (!std::isfinite(v)) { ok = false; return H; }
  // H + rho sum a a' with unit-norm rows: the reduced-curvature test matrix.
  std::vector<double> test = a;
  const double rho = 1e3 * std::max(scale, 1.0);
  for (const FlatConstraintRow* row : active_rows) {
    const std::vector<double> r = series_to_doubles_(row->jacobian);
    double nrm = 0.0;
    for (double v : r) nrm += v * v;
    nrm = std::sqrt(nrm);
    if (!(nrm > 1e-14) || static_cast<std::int64_t>(r.size()) != n) continue;
    for (std::int64_t i = 0; i < n; ++i)
      for (std::int64_t j = 0; j < n; ++j)
        test[static_cast<std::size_t>(i * n + j)] += rho * (r[static_cast<std::size_t>(i)] / nrm) * (r[static_cast<std::size_t>(j)] / nrm);
  }
  double shift = 0.0;
  const double first_shift = 1e-6 * std::max(scale, 1.0);
  bool pd = false;
  for (int attempt = 0; attempt < 16 && !pd; ++attempt) {
    std::vector<double> trial = test;
    for (std::int64_t i = 0; i < n; ++i) trial[static_cast<std::size_t>(i * n + i)] += shift;
    pd = cholesky_factor_(trial, n, 1e-12);
    if (!pd) shift = shift == 0.0 ? first_shift : shift * 10.0;
  }
  if (shift > 0.0) H = H.add(identity_matrix_(n, platform).mul(shift));
  return H;
}

// Limited-memory update. The new pair is damped with the current limited-memory matrix (Powell, as in the dense
// update) so that the curvature condition holds, pushed onto the history (oldest dropped past `memory`), and the
// dense matrix is rebuilt from the history: B = BFGS applied to theta I with the stored pairs, oldest to newest,
// which is the compact representation of Byrd, Nocedal and Schnabel (1994).
inline Series lbfgs_model_update_(std::vector<Series>& s_history, std::vector<Series>& y_history,
                                  const Series& current, const Series& s, const Series& y, std::int64_t memory,
                                  double curvature_tolerance, std::int64_t n, pluss::codem::Platform platform) {
  Series y_used = y.to(ScalarType::Float64);
  if (current.defined() && current.numel() == n * n) {
    const Series Bs = current.matmul(s.reshape({n, 1})).reshape({n});
    const double sBs = series_dot_(s, Bs);
    const double sy = series_dot_(s, y_used);
    if (sBs > 0.0 && sy < 0.2 * sBs) {
      const double theta = 0.8 * sBs / (sBs - sy);
      y_used = y_used.mul(theta).add(Bs.mul(1.0 - theta));
    }
  }
  update_lbfgs_history_(s_history, y_history, s, y_used, memory, curvature_tolerance);
  return lbfgs_dense_hessian_(s_history, y_history, n, platform);
}

// Multipliers of the active rows of the working set (aligned with `active`), spread over the rows of the full
// constraint system.
inline void scatter_active_multipliers_(const FlatConstraintSystem& system,
                                        const std::vector<FlatConstraintRow>& active,
                                        const std::vector<double>& aligned,
                                        std::vector<double>& equality,
                                        std::vector<double>& inequality) {
  equality.assign(system.equalities.size(), 0.0);
  inequality.assign(system.inequalities.size(), 0.0);
  for (std::size_t k = 0; k < active.size() && k < aligned.size(); ++k) {
    const auto& rows = active[k].equality ? system.equalities : system.inequalities;
    auto& dest = active[k].equality ? equality : inequality;
    for (std::size_t i = 0; i < rows.size(); ++i) {
      if (rows[i].source == active[k].source && rows[i].index == active[k].index) {
        dest[i] = aligned[k];
        break;
      }
    }
  }
}

} // namespace detail

inline SequentialQPState sequential_qp_step(const ConstrainedProblem& problem,
                                            const SequentialQPState& state,
                                            const SequentialQPOptions& options = {}) {
  SequentialQPState next = state;
  if (state.converged || state.evaluation.status != EvalStatus::Ok) return next;

  const auto platform = state.params.empty() ? pluss::codem::Platform::CPU : state.params[0].value.platform();
  const std::int64_t n = state.params.total_numel();
  const auto system = detail::assemble_constraint_system_(problem, state.params, state.evaluation);
  const std::vector<double> grad = detail::series_to_doubles_(flatten_values(state.evaluation.gradient));
  double grad_inf = 0.0;
  for (double v : grad) grad_inf = std::max(grad_inf, std::abs(v));

  const auto pack_rows = [&](const std::vector<detail::FlatConstraintRow>& rows, const detail::FlatConstraintSystem& sys,
                             std::vector<double>& J, std::vector<double>& res) {
    (void)sys;
    J.assign(rows.size() * static_cast<std::size_t>(n), 0.0);
    res.assign(rows.size(), 0.0);
    for (std::size_t i = 0; i < rows.size(); ++i) {
      const std::vector<double> row_owned = rows[i].dense.empty() ? detail::series_to_doubles_(rows[i].jacobian) : std::vector<double>();
      const std::vector<double>& row = rows[i].dense.empty() ? row_owned : rows[i].dense;
      for (std::int64_t j = 0; j < n && j < static_cast<std::int64_t>(row.size()); ++j)
        J[i * static_cast<std::size_t>(n) + static_cast<std::size_t>(j)] = row[static_cast<std::size_t>(j)];
      res[i] = rows[i].residual;
    }
  };
  std::vector<double> Ae, h, Ai, c;
  pack_rows(system.equalities, system, Ae, h);
  pack_rows(system.inequalities, system, Ai, c);

  const bool exact_model = options.hessian_approximation == OptimaHessianApproximation::Exact ||
                           options.hessian_approximation == OptimaHessianApproximation::HessianVector;
  Series model_hessian = state.hessian;
  if (exact_model && !state.curvature_reset) {
    bool exact_ok = false;
    std::vector<const detail::FlatConstraintRow*> active;
    for (const auto& row : system.equalities) active.push_back(&row);
    for (std::size_t i = 0; i < system.inequalities.size(); ++i) {
      const double mult = i < state.qp_inequality_multipliers.size() ? state.qp_inequality_multipliers[i] : 0.0;
      if (mult > 1e-8 || system.inequalities[i].residual >= -1e-6) active.push_back(&system.inequalities[i]);
    }
    const Series exact = detail::exact_lagrangian_model_(problem, state.params, system, state.qp_equality_multipliers,
                                                         state.qp_inequality_multipliers, active, options.hvp_options, exact_ok);
    if (exact_ok) model_hessian = exact;
  }
  std::vector<double> B = model_hessian.defined() ? detail::series_to_doubles_(model_hessian)
                                                  : std::vector<double>();
  if (static_cast<std::int64_t>(B.size()) != n * n) {
    B.assign(static_cast<std::size_t>(n * n), 0.0);
    for (std::int64_t i = 0; i < n; ++i) B[static_cast<std::size_t>(i * n + i)] = 1.0;
  }

  const double viol0 = detail::l1_violation_(state.evaluation);
  double rho = std::max({state.current_penalty, 10.0, 10.0 * grad_inf});
  double x_inf0 = 0.0;
  for (double v : detail::series_to_doubles_(flatten_values(state.params))) x_inf0 = std::max(x_inf0, std::abs(v));
  const double radius = state.trust_radius > 0.0 ? state.trust_radius : std::max(10.0, 10.0 * x_inf0);
  const std::size_t mi_real = c.size();
  // Trust-region box |d_j| <= radius, appended after the real inequality rows.
  for (std::int64_t j = 0; j < n; ++j) {
    for (double sgn : {1.0, -1.0}) {
      Ai.resize(Ai.size() + static_cast<std::size_t>(n), 0.0);
      Ai[Ai.size() - static_cast<std::size_t>(n) + static_cast<std::size_t>(j)] = sgn;
      c.push_back(-radius);
    }
  }
  detail::QpLinearSolverSettings qp_settings;
  qp_settings.solver = options.linear_system_solver;
  qp_settings.structure = options.system_structure;
  qp_settings.regularization = options.regularization;
  qp_settings.pivot_threshold = options.pivot_threshold;
  qp_settings.max_krylov_iterations = options.max_krylov_iterations;
  qp_settings.iterative_dimension_threshold = options.iterative_dimension_threshold;
  qp_settings.krylov_tolerance = options.krylov_tolerance;
  qp_settings.krylov_absolute_tolerance = options.krylov_absolute_tolerance;
  detail::ElasticQpResult qp;
  for (int attempt = 0; attempt < 6; ++attempt) {
    qp = detail::solve_elastic_qp_(B, grad, Ae, h, Ai, c, rho, qp_settings);
    next.krylov_iterations += qp.krylov_iterations;
    if (!qp.ok) break;
    // Elastic activity at a linearization that could be satisfied means the penalty was too small.
    if (qp.elastic_max <= 1e-7 * (1.0 + viol0) || rho >= 1e8) break;
    rho *= 10.0;
  }
  if (!qp.ok) {
    next.converged = true;
    next.message = "line search failed: the quadratic subproblem could not be solved";
    return next;
  }

  // Stationarity of the Lagrangian at x with the subproblem multipliers.
  std::vector<double> lag = grad;
  const std::size_t me = h.size(), mi = mi_real;
  for (std::size_t k = 0; k < me; ++k)
    for (std::int64_t j = 0; j < n; ++j) lag[static_cast<std::size_t>(j)] += qp.eq_multipliers[k] * Ae[k * static_cast<std::size_t>(n) + static_cast<std::size_t>(j)];
  for (std::size_t k = 0; k < mi; ++k)
    for (std::int64_t j = 0; j < n; ++j) lag[static_cast<std::size_t>(j)] += qp.ineq_multipliers[k] * Ai[k * static_cast<std::size_t>(n) + static_cast<std::size_t>(j)];
  double stationarity = 0.0;
  for (double v : lag) stationarity = std::max(stationarity, std::abs(v));
  double complementarity = 0.0;
  for (std::size_t k = 0; k < mi; ++k) complementarity = std::max(complementarity, qp.ineq_multipliers[k] * std::max(0.0, -c[k]));
  double dual_inf = 0.0;
  for (std::size_t k = 0; k < mi; ++k) dual_inf = std::max(dual_inf, std::max(0.0, -qp.ineq_multipliers[k]));
  double mult_inf = 0.0;
  for (double v : qp.eq_multipliers) mult_inf = std::max(mult_inf, std::abs(v));
  for (double v : qp.ineq_multipliers) mult_inf = std::max(mult_inf, std::abs(v));
  next.kkt_residual = stationarity / std::max(1.0, state.gradient_scale);

  double d_inf = 0.0;
  for (double v : qp.d) d_inf = std::max(d_inf, std::abs(v));
  double x_inf = 0.0;
  for (double v : detail::series_to_doubles_(flatten_values(state.params))) x_inf = std::max(x_inf, std::abs(v));

  const bool trust_active = d_inf >= 0.99 * radius;
  const bool feasible = state.evaluation.max_constraint_violation <= options.constraint_tolerance;
  if (!trust_active && feasible && next.kkt_residual <= options.optimality_tolerance &&
      complementarity <= options.optimality_tolerance * std::max(1.0, mult_inf)) {
    next.converged = true;
    next.message = "optimality and feasibility tolerances satisfied";
    return next;
  }
  if (!trust_active && !feasible && qp.elastic_max > 1e-6 * (1.0 + viol0) && d_inf <= 1e-9 * (1.0 + x_inf)) {
    next.converged = true;
    next.message = "constraint violation locally stationary";
    return next;
  }

  // l1 merit with a penalty above the multiplier magnitude.
  const double nu = std::max(1.1 * mult_inf + 1e-2, 0.5 * state.current_penalty);
  const auto merit = [&](const ProblemEvaluation& e) {
    return e.value.item<double>() + nu * detail::l1_violation_(e);
  };
  const double phi0 = merit(state.evaluation);
  double lin_viol = 0.0;
  for (std::size_t k = 0; k < me; ++k) {
    double v = h[k];
    for (std::int64_t j = 0; j < n; ++j) v += Ae[k * static_cast<std::size_t>(n) + static_cast<std::size_t>(j)] * qp.d[static_cast<std::size_t>(j)];
    lin_viol += std::abs(v);
  }
  for (std::size_t k = 0; k < mi; ++k) {
    double v = c[k];
    for (std::int64_t j = 0; j < n; ++j) v += Ai[k * static_cast<std::size_t>(n) + static_cast<std::size_t>(j)] * qp.d[static_cast<std::size_t>(j)];
    lin_viol += std::max(0.0, v);
  }
  double gd = 0.0;
  for (std::int64_t j = 0; j < n; ++j) gd += grad[static_cast<std::size_t>(j)] * qp.d[static_cast<std::size_t>(j)];
  double dir_deriv = gd - nu * (viol0 - lin_viol);
  if (!(dir_deriv < 0.0)) dir_deriv = -1e-12 - 1e-8 * d_inf * d_inf;

  // A feasible point where the model predicts no useful decrease and the Lagrangian is nearly stationary is the
  // converged answer. Waiting for the line search to fail on it first would spend up to max_line_search_iterations
  // objective evaluations on rounding noise (two thirds of all evaluations of a typical run).
  if (feasible && !trust_active && next.kkt_residual <= 100.0 * options.optimality_tolerance && d_inf <= 1e-5 * (1.0 + x_inf) &&
      -dir_deriv <= options.value_progress_tolerance * (1.0 + std::abs(state.evaluation.value.item<double>()))) {
    next.converged = true;
    next.message = "predicted objective improvement below tolerance at a feasible point";
    return next;
  }

  const auto layout = state.params.layout();
  const std::vector<double> x0 = detail::series_to_doubles_(flatten_values(state.params));
  const auto make_trial = [&](const std::vector<double>& d, double alpha) {
    std::vector<double> xt(x0);
    for (std::int64_t j = 0; j < n; ++j) xt[static_cast<std::size_t>(j)] += alpha * d[static_cast<std::size_t>(j)];
    auto flat = pluss::make_series(std::move(xt), {n}).to(ScalarType::Float64).to(platform);
    auto trial = unflatten_values(layout, flat);
    if (options.project_bounds) trial = detail::project_bounds_(trial, problem.bounds);
    return trial;
  };

  bool accepted = false;
  ParameterPack<Series> accepted_params;
  ProblemEvaluation accepted_eval;
  double accepted_alpha = 0.0;
  std::vector<double> step_used = qp.d;
  double alpha = options.initial_step_size;
  // An amplified initial step (> 1) must not leave the bounds: cap it with the usual fraction to the boundary.
  if (alpha > 1.0) {
    Series dir = pluss::make_series(std::vector<double>(qp.d), {n}).to(ScalarType::Float64).to(platform);
    const double cap = detail::max_step_to_bounds_(problem, state.params, dir, options.fraction_to_boundary);
    if (cap > 0.0 && std::isfinite(cap)) alpha = std::min(alpha, cap);
  }
  for (std::int64_t ls = 0; ls < options.max_line_search_iterations && !accepted; ++ls) {
    auto trial = make_trial(step_used, alpha);
    auto trial_eval = evaluate_problem(problem, trial);
    next.function_evaluations += 1;
    if (trial_eval.status == EvalStatus::Ok) {
      const double phi = merit(trial_eval);
      if (phi <= phi0 + options.armijo * alpha * dir_deriv) {
        accepted = true;
        accepted_params = std::move(trial);
        accepted_eval = std::move(trial_eval);
        accepted_alpha = alpha;
        break;
      }
      // Maratos effect: a full step that raises the violation is retried once with a second-order correction.
      if (ls == 0 && options.max_second_order_corrections > 0 && me + mi > 0 &&
          detail::l1_violation_(trial_eval) > viol0) {
        const auto trial_system = detail::assemble_constraint_system_(problem, trial, trial_eval);
        if (trial_system.equalities.size() == me && trial_system.inequalities.size() == mi) {
          std::vector<double> h2(me), c2(c);
          // Total step D from x must satisfy A D + (c(x+d) - A d) <= 0: the constraint values at the rejected
          // trial, corrected by what the linearization already predicted for that step.
          const auto predicted = [&](const std::vector<double>& J, std::size_t k) {
            double v = 0.0;
            for (std::int64_t j = 0; j < n; ++j) v += J[k * static_cast<std::size_t>(n) + static_cast<std::size_t>(j)] * qp.d[static_cast<std::size_t>(j)];
            return v;
          };
          for (std::size_t k = 0; k < me; ++k) h2[k] = trial_system.equalities[k].residual - predicted(Ae, k);
          for (std::size_t k = 0; k < mi; ++k) c2[k] = trial_system.inequalities[k].residual - predicted(Ai, k);
          const auto soc = detail::solve_elastic_qp_(B, grad, Ae, h2, Ai, c2, rho, qp_settings);
          next.krylov_iterations += soc.krylov_iterations;
          if (soc.ok) {
            auto soc_trial = make_trial(soc.d, 1.0);
            auto soc_eval = evaluate_problem(problem, soc_trial);
            next.function_evaluations += 1;
            if (soc_eval.status == EvalStatus::Ok && merit(soc_eval) <= phi0 + options.armijo * dir_deriv) {
              accepted = true;
              accepted_params = std::move(soc_trial);
              accepted_eval = std::move(soc_eval);
              accepted_alpha = 1.0;
              step_used = soc.d;
              break;
            }
          }
        }
      }
    }
    // At a feasible, nearly stationary point where the model predicts no useful decrease, a line search that has
    // already failed a few times is failing on rounding noise: stop here, where the check after the loop declares
    // convergence, rather than halving the step down to the evaluation limit.
    if (ls >= 3 && feasible && !trust_active && next.kkt_residual <= 1e-3 &&
        -dir_deriv <= options.value_progress_tolerance * (1.0 + std::abs(state.evaluation.value.item<double>()))) {
      break;
    }
    alpha *= options.backtracking;
  }

  if (accepted) {
    const std::vector<double> x1 = detail::series_to_doubles_(flatten_values(accepted_params));
    double move = 0.0;
    for (std::int64_t j = 0; j < n; ++j) move = std::max(move, std::abs(x1[static_cast<std::size_t>(j)] - x0[static_cast<std::size_t>(j)]));
    // A vanishing step while the Lagrangian is still not stationary is a failure, not convergence.
    if (move <= 1e-14 * (1.0 + x_inf)) {
      next.converged = true;
      next.message = (feasible && next.kkt_residual <= 1e-3 && -dir_deriv <= options.value_progress_tolerance * (1.0 + std::abs(state.evaluation.value.item<double>())))
                         ? "predicted objective improvement below tolerance at a feasible point"
                         : "no progress: the step vanished before first-order optimality was reached";
      return next;
    }

    // Damped BFGS on the Lagrangian gradient difference with the subproblem multipliers.
    const auto system1 = detail::assemble_constraint_system_(problem, accepted_params, accepted_eval);
    std::vector<double> lag1 = detail::series_to_doubles_(flatten_values(accepted_eval.gradient));
    std::vector<double> lag0 = grad;
    if (system1.equalities.size() == me && system1.inequalities.size() == mi) {
      const auto add_rows = [&](const std::vector<detail::FlatConstraintRow>& rows, const std::vector<double>& mult,
                                const std::vector<double>& J0, std::vector<double>& out0, std::vector<double>& out1) {
        for (std::size_t k = 0; k < rows.size(); ++k) {
          const auto r1 = detail::series_to_doubles_(rows[k].jacobian);
          for (std::int64_t j = 0; j < n; ++j) {
            out1[static_cast<std::size_t>(j)] += mult[k] * r1[static_cast<std::size_t>(j)];
            out0[static_cast<std::size_t>(j)] += mult[k] * J0[k * static_cast<std::size_t>(n) + static_cast<std::size_t>(j)];
          }
        }
      };
      add_rows(system1.equalities, qp.eq_multipliers, Ae, lag0, lag1);
      add_rows(system1.inequalities, qp.ineq_multipliers, Ai, lag0, lag1);
    }
    std::vector<double> sv(static_cast<std::size_t>(n)), yv(static_cast<std::size_t>(n));
    for (std::int64_t j = 0; j < n; ++j) {
      sv[static_cast<std::size_t>(j)] = x1[static_cast<std::size_t>(j)] - x0[static_cast<std::size_t>(j)];
      yv[static_cast<std::size_t>(j)] = lag1[static_cast<std::size_t>(j)] - lag0[static_cast<std::size_t>(j)];
    }
    const Series s_ser = pluss::make_series(std::move(sv), {n}).to(ScalarType::Float64).to(platform);
    const Series y_ser = pluss::make_series(std::move(yv), {n}).to(ScalarType::Float64).to(platform);
    switch (options.hessian_approximation) {
      case OptimaHessianApproximation::Identity:
        next.hessian = detail::identity_matrix_(n, platform);
        break;
      case OptimaHessianApproximation::Lbfgs:
        next.lbfgs_s_history = state.lbfgs_s_history;
        next.lbfgs_y_history = state.lbfgs_y_history;
        next.hessian = detail::lbfgs_model_update_(next.lbfgs_s_history, next.lbfgs_y_history, model_hessian, s_ser, y_ser,
                                                   options.lbfgs_memory, options.bfgs_curvature_tolerance, n, platform);
        break;
      case OptimaHessianApproximation::Exact:
      case OptimaHessianApproximation::HessianVector:
        next.hessian = state.hessian;  // recomputed at the next point from the new multipliers
        break;
      case OptimaHessianApproximation::Bfgs:
        next.hessian = detail::damped_bfgs_update_(
            state.hessian.defined() ? state.hessian : detail::identity_matrix_(n, platform),
            s_ser, y_ser, options.bfgs_curvature_tolerance);
        break;
    }
    next.curvature_reset = false;
    next.qp_equality_multipliers.assign(qp.eq_multipliers.begin(), qp.eq_multipliers.end());
    next.qp_inequality_multipliers.assign(qp.ineq_multipliers.begin(),
                                          qp.ineq_multipliers.begin() + static_cast<std::ptrdiff_t>(std::min(mi_real, qp.ineq_multipliers.size())));

    next.trust_radius = accepted_alpha < 1.0 ? std::max({accepted_alpha * d_inf, 0.25 * radius, 1e-8})
                        : (d_inf >= 0.5 * radius ? 2.0 * radius : radius);
    next.params = std::move(accepted_params);
    next.evaluation = std::move(accepted_eval);
    next.iteration += 1;
    next.last_step_size = accepted_alpha;
    next.current_penalty = nu;
    next.merit_value = merit(next.evaluation);
    next.message = "step accepted";
    detail::maybe_update_best_feasible_(next, options.constraint_tolerance);
    if (next.iteration >= options.max_iterations) {
      next.converged = true;
      next.message = "maximum iterations reached";
    }
    if (detail::feasible_objective_target_met_(next.evaluation, options.objective_limit)) {
      next.converged = true;
      next.message = "objective limit reached at a feasible point";
    }
    return next;
  }

  // At a feasible point where the model predicts no further useful decrease, the line search can only fail
  // on rounding noise; that is convergence to the attainable accuracy, not a failure.
  // Tiny predicted decrease only counts as convergence near a stationary point: a badly scaled curvature
  // matrix also makes the predicted decrease tiny while the gradient is still large.
  if (feasible && -dir_deriv <= options.value_progress_tolerance * (1.0 + std::abs(state.evaluation.value.item<double>())) &&
      !trust_active && next.kkt_residual <= 1e-3) {
    next.converged = true;
    next.message = "predicted objective improvement below tolerance at a feasible point";
    return next;
  }

  // The model was misleading: fall back to the identity curvature once before giving up on the line search.
  bool identity_already = true;
  for (std::int64_t i = 0; i < n && identity_already; ++i)
    for (std::int64_t j = 0; j < n; ++j)
      if (std::abs(B[static_cast<std::size_t>(i * n + j)] - (i == j ? 1.0 : 0.0)) > 1e-14) { identity_already = false; break; }
  if (!identity_already) {
    next.hessian = detail::identity_matrix_(n, platform);
    next.lbfgs_s_history.clear();
    next.lbfgs_y_history.clear();
    next.curvature_reset = true;
    next.iteration += 0;
    next.message = "curvature reset";
    return next;
  }

  if (options.enable_restoration && viol0 > options.constraint_tolerance) {
    const auto recovery = detail::restore_feasibility_(problem, state.params, state.evaluation, state.normalization, options);
    next.function_evaluations += recovery.function_evaluations;
    if (recovery.success) {
      next.params = recovery.params;
      next.evaluation = recovery.evaluation;
      next.iteration += 1;
      next.last_step_size = recovery.last_step_size;
      next.current_penalty = state.current_penalty;
      next.merit_value = merit(next.evaluation);
      next.hessian = detail::identity_matrix_(n, platform);
      next.message = "restoration step accepted";
      detail::maybe_update_best_feasible_(next, options.constraint_tolerance);
      if (next.iteration >= options.max_iterations) {
        next.converged = true;
        next.message = "maximum iterations reached";
      }
      return next;
    }
    next.message = recovery.message.empty() ? "line search failed to produce an acceptable SQP step after restoration"
                                            : recovery.message;
    next.converged = true;
    return next;
  }
  next.converged = true;
  next.message = "line search failed to produce an acceptable SQP step";
  return next;
}

inline WorkingSetState working_set_step(const ConstrainedProblem& problem,
                                        const WorkingSetState& state,
                                        const WorkingSetOptions& options = {}) {
  WorkingSetState next = state;
  if (state.converged || state.evaluation.status != EvalStatus::Ok) return next;

  const auto constraint_system = detail::assemble_constraint_system_(problem, state.params, state.evaluation);
  if (state.active_set.empty()) {
    next.active_set = detail::seed_working_set_keys_(
        constraint_system,
        options.active_set_tolerance,
        options.add_constraint_tolerance,
        options.max_working_set_violations);
  }
  auto active_constraints = detail::persistent_working_set_constraints_(
      constraint_system,
      next.active_set.empty() ? state.active_set : next.active_set,
      options.active_set_tolerance,
      options.add_constraint_tolerance,
      options.max_working_set_violations);
  const auto previous_multiplier_hint =
      detail::aligned_working_set_multipliers_(active_constraints, state.active_set, state.multipliers);
  const Series flat_gradient = flatten_values(state.evaluation.gradient).to(ScalarType::Float64);
  const Series variable_scale = state.variable_scale.defined()
                                    ? state.variable_scale.to(ScalarType::Float64).to(flat_gradient.platform())
                                    : detail::ones_series_(flat_gradient.numel(), flat_gradient.platform());

  // Curvature of the quadratic model: the stored model, or for the exact models the Hessian of the Lagrangian at
  // this point with the multipliers of the active rows from the previous step.
  const bool exact_model = options.hessian_approximation == OptimaHessianApproximation::Exact ||
                           options.hessian_approximation == OptimaHessianApproximation::HessianVector;
  Series model_hessian = state.hessian;
  if (exact_model) {
    std::vector<double> eq_mult, ineq_mult;
    detail::scatter_active_multipliers_(constraint_system, active_constraints, previous_multiplier_hint, eq_mult, ineq_mult);
    bool exact_ok = false;
    std::vector<const detail::FlatConstraintRow*> active;
    for (const auto& row : active_constraints) active.push_back(&row);
    const Series exact = detail::exact_lagrangian_model_(problem, state.params, constraint_system, eq_mult, ineq_mult,
                                                         active, options.hvp_options, exact_ok);
    if (exact_ok) model_hessian = exact;
  }

  bool kkt_ok = false;
  Series step_direction = pluss::series::undefined();
  Series multipliers = pluss::series::undefined();
  std::int64_t krylov_iterations = 0;
  const OptimaLinearSystemSolver linear_solver =
      detail::use_iterative_subproblem_solver_(
          options.linear_system_solver,
          options.system_structure,
          flat_gradient.numel() + static_cast<std::int64_t>(active_constraints.size()),
          options.iterative_dimension_threshold)
          ? OptimaLinearSystemSolver::ConjugateGradient
          : options.linear_system_solver;
  const double krylov_tolerance =
      linear_solver == OptimaLinearSystemSolver::ConjugateGradient
          ? std::min(options.krylov_tolerance, 1e-8)
          : options.krylov_tolerance;
  const double krylov_absolute_tolerance =
      linear_solver == OptimaLinearSystemSolver::ConjugateGradient
          ? std::min(options.krylov_absolute_tolerance, 1e-12)
          : options.krylov_absolute_tolerance;

  // Dropping the last active constraint must be followed by one more solve of
  // the reduced system; bound the retries by the ORIGINAL count (the loop used to
  // re-read the shrinking size, so it exited without re-solving and the step and
  // multipliers still described the system that included the dropped row).
  const std::size_t max_drop_attempts = active_constraints.size();
  for (std::size_t attempt = 0; attempt <= max_drop_attempts; ++attempt) {
    const Series active_row_scale = detail::active_constraint_row_scales_(
        constraint_system, active_constraints, state.normalization, flat_gradient.platform());
    const auto scaled_constraints = detail::scale_constraint_rows_(active_constraints, variable_scale, active_row_scale);
    std::tie(step_direction, multipliers) = detail::solve_sqp_kkt_(
        detail::scale_hessian_(model_hessian, variable_scale),
        detail::scale_gradient_(flat_gradient.mul(state.normalization.objective_scale), variable_scale),
        scaled_constraints,
        options.regularization,
        linear_solver,
        options.max_krylov_iterations,
        krylov_tolerance,
        krylov_absolute_tolerance,
        options.pivot_threshold,
        krylov_iterations,
        kkt_ok);
    step_direction = detail::unscale_step_(step_direction, variable_scale);
    if (!kkt_ok) break;

    multipliers = detail::smooth_working_set_multipliers_(
        active_constraints, multipliers, previous_multiplier_hint, options.multiplier_smoothing);
    detail::cleanup_working_set_multipliers_(active_constraints, options.drop_multiplier_tolerance, &multipliers);
    const auto [most_negative, most_negative_index] =
        detail::most_negative_inequality_multiplier_(active_constraints, multipliers);
    if (most_negative_index == active_constraints.size() ||
        most_negative >= -options.drop_multiplier_tolerance) {
      break;
    }
    active_constraints.erase(active_constraints.begin() + static_cast<std::ptrdiff_t>(most_negative_index));
  }

  const auto merit_gradient_pack =
      normalized_penalty_gradient_(problem, state.params, state.evaluation, state.current_penalty, state.normalization);
  const Series merit_gradient = flatten_values(merit_gradient_pack).to(ScalarType::Float64);
  const Series scaled_merit_gradient = detail::scale_gradient_(merit_gradient, variable_scale);

  if (!kkt_ok || detail::series_dot_(merit_gradient, step_direction) >= 0.0) {
    step_direction = detail::unscale_step_(scaled_merit_gradient.mul(-1.0), variable_scale);
    multipliers = detail::zeros_series_(
        static_cast<std::int64_t>(active_constraints.size()),
        merit_gradient.platform());
  }

  const double step_inf_norm = detail::series_inf_norm_(step_direction);
  const Series accepted_active_row_scale = detail::active_constraint_row_scales_(
      constraint_system, active_constraints, state.normalization, flat_gradient.platform());
  next.kkt_residual = detail::kkt_residual_(
      flat_gradient.mul(state.normalization.objective_scale),
      detail::scale_constraint_rows_(active_constraints, pluss::series::undefined(), accepted_active_row_scale),
      multipliers);
  next.krylov_iterations = linear_solver == OptimaLinearSystemSolver::ConjugateGradient ? krylov_iterations : 0;
  if (step_inf_norm <= options.optimality_tolerance &&
      next.kkt_residual <= options.optimality_tolerance &&
      state.evaluation.max_constraint_violation <= options.constraint_tolerance) {
    next.converged = true;
    next.message = "working set satisfies optimality and feasibility tolerances";
    return next;
  }

  double step_size = options.initial_step_size;
  step_size = std::min(step_size,
                       detail::max_step_to_bounds_(
                           problem, state.params, step_direction, options.fraction_to_boundary));
  step_size = std::min(step_size,
                       detail::max_step_to_linearized_inequalities_(
                           constraint_system,
                           step_direction,
                           options.add_constraint_tolerance,
                           options.fraction_to_boundary));
  const double base_merit = state.merit_value;
  double directional_derivative = detail::series_dot_(merit_gradient, step_direction);
  if (!(directional_derivative < 0.0)) {
    directional_derivative = -std::max(1.0, detail::series_dot_(step_direction, step_direction));
  }
  if (!(step_size > 0.0) || !std::isfinite(step_size)) {
    const Series fallback_direction =
        detail::project_direction_from_active_bounds_(problem, state.params, step_direction, 1e-10);
    if (detail::series_inf_norm_(fallback_direction) > options.optimality_tolerance &&
        detail::series_dot_(merit_gradient, fallback_direction) < 0.0) {
      step_direction = fallback_direction;
      step_size = options.initial_step_size;
      step_size = std::min(step_size,
                           detail::max_step_to_bounds_(
                               problem, state.params, step_direction, options.fraction_to_boundary));
      step_size = std::min(step_size,
                           detail::max_step_to_linearized_inequalities_(
                               constraint_system,
                               step_direction,
                               options.add_constraint_tolerance,
                               options.fraction_to_boundary));
      directional_derivative = detail::series_dot_(merit_gradient, step_direction);
      if (!(directional_derivative < 0.0)) {
        directional_derivative = -std::max(1.0, detail::series_dot_(step_direction, step_direction));
      }
    }
    if (!(step_size > 0.0) || !std::isfinite(step_size)) {
      next.converged = true;
      next.message = "working set cannot produce a bound-feasible step";
      return next;
    }
  }
  const auto state_layout = state.params.layout();
  const Series state_flat64 = flatten_values(state.params).to(ScalarType::Float64);

  for (std::int64_t ls_iter = 0; ls_iter < options.max_line_search_iterations; ++ls_iter) {
    if (step_size * step_inf_norm <= std::max(1e-12, options.step_tolerance)) break;
    const Series trial_flat = state_flat64.add(step_direction.mul(step_size));
    auto trial = unflatten_values(state_layout, trial_flat);
    if (options.project_bounds) trial = detail::project_bounds_(trial, problem.bounds);

    auto trial_eval = evaluate_problem(problem, trial);
    next.function_evaluations += 1;
    if (trial_eval.status == EvalStatus::Ok) {
      double trial_penalty = state.current_penalty;
      if (trial_eval.max_constraint_violation >
          std::max(options.constraint_tolerance,
                   options.feasibility_priority_ratio * state.evaluation.max_constraint_violation)) {
        trial_penalty *= 2.0;
      }
      const double trial_merit = detail::penalized_value_(trial_eval, trial_penalty, state.normalization);
      const double armijo_rhs = base_merit + options.armijo * step_size * directional_derivative;
      if (detail::accept_working_set_trial_(
              state.evaluation,
              base_merit,
              trial_eval,
              trial_merit,
              step_size,
              armijo_rhs,
              options.constraint_tolerance,
              options.feasibility_priority_ratio)) {
        next.params = std::move(trial);
        next.evaluation = std::move(trial_eval);
        next.iteration += 1;
        next.last_step_size = step_size;
        next.current_penalty = trial_penalty;
        next.merit_value = trial_merit;
        next.multipliers = std::move(multipliers);
        detail::cleanup_working_set_multipliers_(active_constraints, options.drop_multiplier_tolerance, &next.multipliers);
        next.message = "working-set step accepted";

        const Series x_prev = state_flat64;
        const Series x_next = flatten_values(next.params).to(ScalarType::Float64);
        const Series s = x_next.sub(x_prev);
        if (state.current_penalty == next.current_penalty) {
          const Series next_merit_gradient =
              flatten_values(normalized_penalty_gradient_(
                                 problem, next.params, next.evaluation, next.current_penalty, state.normalization))
                  .to(ScalarType::Float64);
          const Series y = next_merit_gradient.sub(merit_gradient);
          if (options.hessian_approximation == OptimaHessianApproximation::Lbfgs) {
            next.lbfgs_s_history = state.lbfgs_s_history;
            next.lbfgs_y_history = state.lbfgs_y_history;
            next.hessian = detail::lbfgs_model_update_(
                next.lbfgs_s_history, next.lbfgs_y_history, model_hessian, s, y, options.lbfgs_memory,
                options.bfgs_curvature_tolerance, next.params.total_numel(), next.params[0].value.platform());
          } else if (exact_model) {
            next.hessian = state.hessian;  // recomputed at the next point
          } else {
            next.hessian = detail::update_hessian_(
                state.hessian,
                s,
                y,
                options.bfgs_curvature_tolerance,
                options.hessian_approximation,
                next.params.total_numel(),
                next.params[0].value.platform());
          }
        } else {
          next.hessian = detail::identity_matrix_(next.params.total_numel(), next.params[0].value.platform());
          next.lbfgs_s_history.clear();
          next.lbfgs_y_history.clear();
        }

        const auto next_constraints = detail::assemble_constraint_system_(problem, next.params, next.evaluation);
        const auto next_active = detail::persistent_working_set_constraints_(
            next_constraints,
            state.active_set,
            options.active_set_tolerance,
            options.add_constraint_tolerance,
            options.max_working_set_violations);
        const Series next_flat_gradient = flatten_values(next.evaluation.gradient).to(ScalarType::Float64);
        next.kkt_residual = detail::kkt_residual_(
            next_flat_gradient.mul(state.normalization.objective_scale),
            detail::scale_constraint_rows_(
                next_active,
                pluss::series::undefined(),
                detail::active_constraint_row_scales_(next_constraints, next_active, state.normalization, next_flat_gradient.platform())),
            next.multipliers);
        next.variable_scale = state.variable_scale;
        next.normalization = state.normalization;
        next.active_set = detail::update_working_set_keys_(
            next_constraints,
            next_active,
            next.multipliers,
            state.active_set,
            state.multipliers,
            options.active_set_tolerance,
            options.add_constraint_tolerance,
            options.drop_multiplier_tolerance,
            options.multiplier_retention_tolerance,
            options.max_working_set_violations);
        const double objective_change =
            (state.evaluation.value.defined() && state.evaluation.value.numel() == 1 &&
             next.evaluation.value.defined() && next.evaluation.value.numel() == 1)
                ? std::abs(next.evaluation.value.item<double>() - state.evaluation.value.item<double>())
                : std::numeric_limits<double>::infinity();
        const bool stable_active_set = detail::same_working_set_keys_(next.active_set, state.active_set);
        const double relative_objective_change =
            objective_change /
            std::max(1.0, std::abs(state.evaluation.value.defined() && state.evaluation.value.numel() == 1
                                       ? state.evaluation.value.item<double>()
                                       : 1.0));
        const bool feasible_but_stalling =
            next.evaluation.max_constraint_violation <= options.constraint_tolerance &&
            stable_active_set &&
            next.kkt_residual > 10.0 * options.optimality_tolerance &&
            relative_objective_change <= 1e-3 &&
            next.iteration >= std::max<std::int64_t>(8, next.params.total_numel() / 4);
        const bool weak_progress =
            next.evaluation.max_constraint_violation >=
                std::max(0.0, 0.95 * state.evaluation.max_constraint_violation) &&
            next.kkt_residual >= 0.9 * state.kkt_residual &&
            objective_change <= options.value_progress_tolerance;
        const bool stalled_progress = weak_progress || feasible_but_stalling;
        next.stalled_iterations = stalled_progress ? (state.stalled_iterations + 1) : 0;

        detail::maybe_update_best_feasible_(next, options.constraint_tolerance);
        if (next.iteration >= options.max_iterations) {
          next.converged = true;
          next.message = "maximum iterations reached";
        }
        if (next.kkt_residual <= options.optimality_tolerance &&
            next.evaluation.max_constraint_violation <= options.constraint_tolerance) {
          next.converged = true;
          next.message = "working set satisfies optimality and feasibility tolerances";
        }
        if (detail::feasible_objective_target_met_(next.evaluation, options.objective_limit)) {
          next.converged = true;
          next.message = "objective limit reached at a feasible working-set point";
        }
        if (options.favor_working_set_updates) {
          SequentialQPOptions sqp_safeguard = options;
          sqp_safeguard.progress_hook = {};
          WorkingSetState safeguard = next;
          static_cast<SequentialQPState&>(safeguard) = sequential_qp_step(problem, state, sqp_safeguard);
          if (safeguard.evaluation.status == EvalStatus::Ok) {
            const auto safeguard_constraints =
                detail::assemble_constraint_system_(problem, safeguard.params, safeguard.evaluation);
            safeguard.active_set = detail::seed_working_set_keys_(
                safeguard_constraints,
                options.active_set_tolerance,
                options.add_constraint_tolerance,
                options.max_working_set_violations);
          } else {
            safeguard.active_set.clear();
          }
          if (detail::prefer_candidate_(safeguard, next, options.constraint_tolerance)) {
            safeguard.message = safeguard.message.find("working set") != std::string::npos
                                    ? safeguard.message
                                    : "working set fallback: " + safeguard.message;
            safeguard.stalled_iterations = 0;
            return safeguard;
          }
        }
        return next;
      }
    }
    step_size *= options.backtracking;
  }

  if (options.enable_restoration) {
    const auto recovery = detail::restore_feasibility_(problem, state.params, state.evaluation, state.normalization, options);
    next.function_evaluations += recovery.function_evaluations;
    if (recovery.success) {
      next.params = recovery.params;
      next.evaluation = recovery.evaluation;
      next.iteration += 1;
      next.last_step_size = recovery.last_step_size;
      next.current_penalty = state.current_penalty;
      if (next.evaluation.max_constraint_violation >
          std::max(options.constraint_tolerance,
                   options.feasibility_priority_ratio * state.evaluation.max_constraint_violation)) {
        next.current_penalty *= 2.0;
      }
      next.merit_value = detail::penalized_value_(next.evaluation, next.current_penalty, state.normalization);
      next.hessian = detail::identity_matrix_(
          next.params.total_numel(),
          next.params.empty() ? pluss::codem::Platform::CPU : next.params[0].value.platform());
      next.multipliers = detail::zeros_series_(
          static_cast<std::int64_t>(active_constraints.size()),
          next.hessian.platform());
      const auto next_constraints = detail::assemble_constraint_system_(problem, next.params, next.evaluation);
      const auto next_active = detail::persistent_working_set_constraints_(
          next_constraints,
          state.active_set,
          options.active_set_tolerance,
          options.add_constraint_tolerance,
          options.max_working_set_violations);
      next.kkt_residual = detail::kkt_residual_(
          flatten_values(next.evaluation.gradient).to(ScalarType::Float64).mul(state.normalization.objective_scale),
          detail::scale_constraint_rows_(
              next_active,
              pluss::series::undefined(),
              detail::active_constraint_row_scales_(next_constraints, next_active, state.normalization, flat_gradient.platform())),
          next.multipliers);
      next.message = "working-set restoration step accepted";
      next.variable_scale = state.variable_scale;
      next.normalization = state.normalization;
      next.active_set = detail::update_working_set_keys_(
          next_constraints,
          next_active,
          next.multipliers,
          state.active_set,
          state.multipliers,
          options.active_set_tolerance,
          options.add_constraint_tolerance,
          options.drop_multiplier_tolerance,
          options.multiplier_retention_tolerance,
          options.max_working_set_violations);
      next.stalled_iterations = 0;
      detail::maybe_update_best_feasible_(next, options.constraint_tolerance);
      if (next.iteration >= options.max_iterations) {
        next.converged = true;
        next.message = "maximum iterations reached";
      }
      if (next.kkt_residual <= options.optimality_tolerance &&
          next.evaluation.max_constraint_violation <= options.constraint_tolerance) {
        next.converged = true;
        next.message = "working set satisfies optimality and feasibility tolerances";
      }
      if (detail::feasible_objective_target_met_(next.evaluation, options.objective_limit)) {
        next.converged = true;
        next.message = "objective limit reached at a feasible working-set point";
      }
      if (options.favor_working_set_updates) {
        SequentialQPOptions sqp_safeguard = options;
        sqp_safeguard.progress_hook = {};
        WorkingSetState safeguard = next;
        static_cast<SequentialQPState&>(safeguard) = sequential_qp_step(problem, state, sqp_safeguard);
        if (safeguard.evaluation.status == EvalStatus::Ok) {
          const auto safeguard_constraints =
              detail::assemble_constraint_system_(problem, safeguard.params, safeguard.evaluation);
          safeguard.active_set = detail::seed_working_set_keys_(
              safeguard_constraints,
              options.active_set_tolerance,
              options.add_constraint_tolerance,
              options.max_working_set_violations);
        } else {
          safeguard.active_set.clear();
        }
        if (detail::prefer_candidate_(safeguard, next, options.constraint_tolerance)) {
          safeguard.message = safeguard.message.find("working set") != std::string::npos
                                  ? safeguard.message
                                  : "working set fallback: " + safeguard.message;
          safeguard.stalled_iterations = 0;
          return safeguard;
        }
      }
      return next;
    }
    next.message = recovery.message.empty()
                       ? "working set failed to recover a feasible step"
                       : recovery.message;
  }

  next.stalled_iterations = state.stalled_iterations + 1;
  if (options.favor_working_set_updates && next.stalled_iterations >= options.max_stalled_iterations) {
    SequentialQPOptions sqp_fallback = options;
    sqp_fallback.progress_hook = {};
    WorkingSetState fallback = state;
    static_cast<SequentialQPState&>(fallback) = sequential_qp_step(problem, state, sqp_fallback);
    if (fallback.evaluation.status == EvalStatus::Ok) {
      const auto fallback_constraints =
          detail::assemble_constraint_system_(problem, fallback.params, fallback.evaluation);
      fallback.active_set = detail::seed_working_set_keys_(
          fallback_constraints,
          options.active_set_tolerance,
          options.add_constraint_tolerance,
          options.max_working_set_violations);
    } else {
      fallback.active_set.clear();
    }
    fallback.stalled_iterations = 0;
    if (detail::prefer_candidate_(fallback, next, options.constraint_tolerance) ||
        fallback.iteration > state.iteration) {
      fallback.message = fallback.message.find("working set") != std::string::npos
                             ? fallback.message
                             : "working set reset: " + fallback.message;
      return fallback;
    }
  }

  SequentialQPOptions sqp_fallback = options;
  sqp_fallback.progress_hook = {};
  WorkingSetState fallback = state;
  static_cast<SequentialQPState&>(fallback) = sequential_qp_step(problem, state, sqp_fallback);
  if (fallback.evaluation.status == EvalStatus::Ok) {
    const auto fallback_constraints =
        detail::assemble_constraint_system_(problem, fallback.params, fallback.evaluation);
    fallback.active_set = detail::seed_working_set_keys_(
        fallback_constraints,
        options.active_set_tolerance,
        options.add_constraint_tolerance,
        options.max_working_set_violations);
  } else {
    fallback.active_set.clear();
  }
  fallback.stalled_iterations = 0;
  if (fallback.iteration > state.iteration ||
      (fallback.evaluation.status == EvalStatus::Ok &&
       fallback.evaluation.max_constraint_violation + 1e-12 < state.evaluation.max_constraint_violation)) {
    if (fallback.message.find("working set") == std::string::npos) {
      fallback.message = "working set fallback: " + fallback.message;
    }
    return fallback;
  }

  next.converged = true;
  if (next.message.empty()) next.message = "working set line search failed to produce an acceptable step";
  return next;
}

inline SequentialQPState solve_sequential_qp(const ConstrainedProblem& problem,
                                             const ParameterPack<Series>& initial_params,
                                             const SequentialQPOptions& options = {}) {
  SequentialQPState state = init_sequential_qp_state(problem, initial_params, options);
  // A run can end on a fallback direction that carries no multipliers, leaving a stale stationarity.
  // Once the run has ended, re-estimate it at the returned point so every later report agrees.
  const auto refresh_final_stationarity = [&](SequentialQPState& s) {
    if (!s.converged || s.evaluation.status != EvalStatus::Ok) return;
    const auto system = detail::assemble_constraint_system_(problem, s.params, s.evaluation);
    const auto active = detail::active_constraints_(system, 1e-4);
    const Series gradient = flatten_values(s.evaluation.gradient).to(ScalarType::Float64);
    const Series lambda = detail::estimate_kkt_multipliers_(gradient, active);
    s.kkt_residual = std::min(s.kkt_residual, detail::kkt_residual_(gradient, active, lambda));
  };
  std::string progress_message;
  if (detail::run_progress_hook_(
          options.progress_hook,
          detail::make_progress_snapshot_(OptimaAlgorithm::SequentialQP, OptimaProgressStage::Start, state),
          &progress_message)) {
    state.converged = true;
    state.message = progress_message.empty() ? "stopped by progress hook" : progress_message;
  }
  while (!state.converged && state.iteration < options.max_iterations) {
    SequentialQPState next = sequential_qp_step(problem, state, options);
    if (next.iteration == state.iteration && next.message == state.message &&
        next.converged == state.converged) {
      break;
    }
    state = std::move(next);
    refresh_final_stationarity(state);
    progress_message.clear();
    if (detail::run_progress_hook_(
            options.progress_hook,
            detail::make_progress_snapshot_(OptimaAlgorithm::SequentialQP, OptimaProgressStage::Iteration, state),
            &progress_message)) {
      state.converged = true;
      state.message = progress_message.empty() ? "stopped by progress hook" : progress_message;
      break;
    }
  }
  detail::run_progress_hook_(
      options.progress_hook,
      detail::make_progress_snapshot_(OptimaAlgorithm::SequentialQP, OptimaProgressStage::Done, state));
  return state;
}

inline WorkingSetState solve_working_set(const ConstrainedProblem& problem,
                                         const ParameterPack<Series>& initial_params,
                                         const WorkingSetOptions& options = {}) {
  const auto qualify_working_set_message = [](std::string message) {
    if (message.empty() || message.find("working set") != std::string::npos) return message;
    return std::string("working set: ") + message;
  };
  WorkingSetOptions working_set_options = options;
  if (options.favor_working_set_updates) {
    working_set_options.max_second_order_corrections =
        std::max<std::int64_t>(working_set_options.max_second_order_corrections, 1);
    working_set_options.active_set_tolerance =
        std::min(working_set_options.active_set_tolerance, 1e-5);
    working_set_options.add_constraint_tolerance =
        std::max(working_set_options.add_constraint_tolerance, 10.0 * working_set_options.active_set_tolerance);
    working_set_options.max_working_set_violations =
        std::max<std::int64_t>(working_set_options.max_working_set_violations,
                               std::max<std::int64_t>(8, initial_params.total_numel() / 2));
    working_set_options.max_stalled_iterations =
        std::min<std::int64_t>(working_set_options.max_stalled_iterations, 3);
  }

  WorkingSetState state = init_working_set_state(problem, initial_params, working_set_options);
  bool continuation_used_ = false;
  std::string progress_message;
  if (detail::run_progress_hook_(
          options.progress_hook,
          detail::make_progress_snapshot_(OptimaAlgorithm::WorkingSet, OptimaProgressStage::Start, state),
          &progress_message)) {
    state.converged = true;
    state.message = qualify_working_set_message(progress_message.empty() ? "stopped by progress hook" : progress_message);
  }
  while (!state.converged && state.iteration < working_set_options.max_iterations) {
    WorkingSetState next = working_set_step(problem, state, working_set_options);
    if (next.iteration == state.iteration && next.message == state.message &&
        next.converged == state.converged) {
      break;
    }
    next.message = qualify_working_set_message(std::move(next.message));
    state = std::move(next);
    // Hand the run to the sequential QP phase when the working-set phase stalls, runs past its own
    // budget, or ends on a failure that the QP phase may still be able to recover from.
    const bool failed_step =
        state.converged &&
        detail::exit_flag_from_state_(true, state.message, EvalStatus::Ok) == OptimaExitFlag::NoProgress;
    const bool over_budget =
        !state.converged &&
        state.iteration >= std::min<std::int64_t>(working_set_options.max_iterations,
                                                  50 + 10 * initial_params.total_numel());
    const bool stalled =
        !state.converged && working_set_options.favor_working_set_updates &&
        state.stalled_iterations >= working_set_options.max_stalled_iterations;
    if ((failed_step || over_budget || stalled) && state.iteration < working_set_options.max_iterations &&
        !continuation_used_) {
      continuation_used_ = true;
      SequentialQPOptions continuation_options = working_set_options;
      continuation_options.progress_hook = {};
      continuation_options.max_iterations =
          std::max<std::int64_t>(1, working_set_options.max_iterations - state.iteration);
      const std::int64_t iterations_so_far = state.iteration;
      const std::int64_t evaluations_so_far = state.function_evaluations;
      SequentialQPState continuation = solve_sequential_qp(problem, state.params, continuation_options);
      continuation.iteration += iterations_so_far;
      continuation.function_evaluations += evaluations_so_far;
      static_cast<SequentialQPState&>(state) = std::move(continuation);
      if (state.evaluation.status == EvalStatus::Ok) {
        const auto continuation_constraints =
            detail::assemble_constraint_system_(problem, state.params, state.evaluation);
        state.active_set = detail::seed_working_set_keys_(
            continuation_constraints,
            working_set_options.active_set_tolerance,
            working_set_options.add_constraint_tolerance,
            working_set_options.max_working_set_violations);
      } else {
        state.active_set.clear();
      }
      state.stalled_iterations = 0;
      state.message = "working set continuation: " + state.message;
    }
    progress_message.clear();
    if (detail::run_progress_hook_(
            options.progress_hook,
            detail::make_progress_snapshot_(OptimaAlgorithm::WorkingSet, OptimaProgressStage::Iteration, state),
            &progress_message)) {
      state.converged = true;
      state.message = qualify_working_set_message(progress_message.empty() ? "stopped by progress hook" : progress_message);
      break;
    }
  }
  state.message = qualify_working_set_message(std::move(state.message));
  detail::run_progress_hook_(
      options.progress_hook,
      detail::make_progress_snapshot_(OptimaAlgorithm::WorkingSet, OptimaProgressStage::Done, state));
  return state;
}

inline InternalProtocolState init_internal_protocol_state(const ConstrainedProblem& problem,
                                                    const ParameterPack<Series>& initial_params,
                                                    const InternalProtocolOptions& options = {}) {
  InternalProtocolState state;
  if (!detail::supports_exact_curvature_(problem, options.hessian_approximation)) {
    state.params = initial_params;
    state.evaluation.status = EvalStatus::InvalidProblem;
    state.converged = true;
    state.message =
        "exact internal-protocol curvature requires an autodiff objective and autodiff nonlinear callbacks";
    return state;
  }
  std::int64_t eval_count = 0;
  bool init_ok = true;
  bool recovered_nonfinite_init = false;
  state.params = detail::make_strictly_interior_(
      problem,
      initial_params,
      options.initialization_margin,
      options.project_bounds,
      eval_count,
      init_ok,
      /*skip_nonlinear=*/options.independent_nonlinear_slacks);
  state.function_evaluations = eval_count;
  state.evaluation = evaluate_problem(problem, state.params);
  state.function_evaluations += 1;
  if (state.evaluation.status != EvalStatus::Ok &&
      options.enable_nonfinite_recovery &&
      detail::is_nonfinite_eval_status_(state.evaluation.status)) {
    ParameterPack<Series> recovered_params = state.params;
    ProblemEvaluation recovered_eval = state.evaluation;
    if (detail::recover_nonfinite_initial_point_(
            problem,
            state.params,
            options,
            state.function_evaluations,
            recovered_params,
            recovered_eval)) {
      bool reinited_ok = true;
      state.params = detail::make_strictly_interior_(
          problem,
          recovered_params,
          options.initialization_margin,
          options.project_bounds,
          state.function_evaluations,
          reinited_ok,
          /*skip_nonlinear=*/options.independent_nonlinear_slacks);
      state.evaluation = evaluate_problem(problem, state.params);
      state.function_evaluations += 1;
      init_ok = init_ok && reinited_ok;
      if (state.evaluation.status == EvalStatus::Ok) {
        recovered_nonfinite_init = true;
      }
    }
  }
  state.barrier_parameter = std::max(options.initial_barrier_parameter, options.minimum_barrier_parameter);
  if (state.evaluation.status == EvalStatus::Ok) {
    const auto system = detail::assemble_constraint_system_(problem, state.params, state.evaluation);
    state.normalization = detail::compute_problem_normalization_(
        state.evaluation,
        system,
        options.problem_scaling,
        state.params.empty() ? pluss::codem::Platform::CPU : state.params[0].value.platform(),
        problem.bounds.lower.has_value() ? state.params.total_numel() : 0,
        problem.bounds.upper.has_value() ? state.params.total_numel() : 0);
    state.merit_penalty = options.equality_penalty;
    for (const auto* rows : {&system.equalities, &system.inequalities}) {
      for (const auto& row : *rows) {
        const double jn = detail::series_inf_norm_(row.jacobian.to(ScalarType::Float64));
        state.stationarity_row_scale.push_back(jn > 1e-12 ? 1.0 / jn : 1.0);
      }
    }
    if (options.independent_nonlinear_slacks) state.nl_slack = detail::initial_nl_slack_(
        system, std::max(options.initialization_margin, 1e-2),
        state.params.empty() ? pluss::codem::Platform::CPU : state.params[0].value.platform());
    state.barrier_value = detail::merit_value_(
        state.evaluation,
        system,
        state.barrier_parameter,
        state.normalization,
        state.merit_penalty,
        state.nl_slack);
    detail::update_primal_dual_metrics_(
        state,
        system,
        state.barrier_parameter,
        state.normalization);
    // Judge stationarity relative to the starting Lagrangian gradient, in the solver's own (normalized) units.
    if (std::isfinite(state.barrier_gradient_norm)) {
      state.gradient_scale = std::max(1.0, state.barrier_gradient_norm);
      state.barrier_gradient_norm /= state.gradient_scale;
    }
  } else {
    state.barrier_value = std::numeric_limits<double>::infinity();
    state.barrier_gradient_norm = std::numeric_limits<double>::infinity();
    state.inequality_slack = pluss::series::empty({0}, ScalarType::Float64).to(
        state.params.empty() ? pluss::codem::Platform::CPU : state.params[0].value.platform());
    state.inequality_dual = state.inequality_slack;
    state.primal_residual = std::numeric_limits<double>::infinity();
    state.complementarity_residual = std::numeric_limits<double>::infinity();
  }
  state.converged = !init_ok || state.evaluation.status != EvalStatus::Ok || !std::isfinite(state.barrier_value);
  state.message = state.converged ? "failed to initialize a strict internal protocol"
                                  : (recovered_nonfinite_init ? "initialized after non-finite recovery"
                                                              : "initialized");
  if (state.evaluation.status != EvalStatus::Ok) state.message = state.evaluation.message;
  state.hessian = detail::identity_matrix_(
      state.params.total_numel(),
      state.params.empty() ? pluss::codem::Platform::CPU : state.params[0].value.platform());
  state.variable_scale = detail::compute_variable_scale_(state.params, problem.bounds, options.problem_scaling);
  state.krylov_iterations = 0;
  state.trust_region_radius = std::clamp(
      std::max(options.trust_region_initial_radius, options.initial_step_size),
      std::max(1e-12, options.trust_region_min_radius),
      std::max(options.trust_region_min_radius, options.trust_region_max_radius));
  state.barrier_history.clear();
  detail::push_barrier_history_(state.barrier_history, state.barrier_value);
  state.lbfgs_s_history.clear();
  state.lbfgs_y_history.clear();
  state.in_feasibility_phase = detail::should_enter_feasibility_phase_(state, options);
  detail::maybe_update_best_feasible_(state, options.constraint_tolerance);
  if (!state.converged && detail::feasible_objective_target_met_(state.evaluation, options.objective_limit)) {
    state.converged = true;
    state.message = "objective limit reached at a feasible point";
  }
  return state;
}

inline InternalProtocolState internal_protocol_step(const ConstrainedProblem& problem,
                                              const InternalProtocolState& state,
                                              const InternalProtocolOptions& options = {}) {
  InternalProtocolState next = state;
  if (state.converged || state.evaluation.status != EvalStatus::Ok) return next;

  if (state.in_feasibility_phase) {
    const auto phase = detail::advance_feasibility_phase_(
        problem, state.params, state.evaluation, state.normalization, options);
    next.function_evaluations += phase.function_evaluations;
    if (phase.success) {
      next.params = phase.params;
      next.nl_slack = pluss::series::undefined();
      next.evaluation = phase.evaluation;
      // Count one major internal-protocol iteration; feasibility substeps are
      // tracked separately in function-evaluation counts.
      next.iteration += 1;
      next.last_step_size = phase.last_step_size;
      next.message = phase.message;
      next.hessian = detail::identity_matrix_(
          next.params.total_numel(),
          next.params.empty() ? pluss::codem::Platform::CPU : next.params[0].value.platform());
      next.krylov_iterations = state.krylov_iterations;
      next.lbfgs_s_history.clear();
      next.lbfgs_y_history.clear();
      const auto next_system = detail::assemble_constraint_system_(problem, next.params, next.evaluation);
      next.barrier_value = detail::merit_value_(
          next.evaluation,
          next_system,
          next.barrier_parameter,
          next.normalization,
          next.merit_penalty);
      next.barrier_history.clear();
      detail::update_primal_dual_metrics_(
          next,
          next_system,
          next.barrier_parameter,
          next.normalization);
      if (detail::should_reduce_barrier_(next, options)) {
        detail::reduce_barrier_state_(next, next_system, options);
      }
      next.trust_region_radius = detail::update_trust_radius_(
          state.trust_region_radius,
          std::max(phase.last_step_size, 0.0),
          1.0,
          options);
      detail::push_barrier_history_(next.barrier_history, next.barrier_value);
      next.stalled_iterations = 0;
      next.feasibility_switch_violations = 0;
      next.in_feasibility_phase = detail::should_enter_feasibility_phase_(next, options);
      if (!next.in_feasibility_phase &&
          next.evaluation.max_constraint_violation > options.constraint_tolerance &&
          next.feasibility_metric >
              std::max(options.constraint_tolerance, state.feasibility_metric) *
                  (1.0 - std::clamp(options.sufficient_infeas_decrease, 0.0, 1.0))) {
        next.in_feasibility_phase = true;
      }
      detail::maybe_update_best_feasible_(next, options.constraint_tolerance);
      if (next.iteration >= options.max_iterations && !next.converged) {
        next.converged = true;
        next.message = "maximum iterations reached";
      }
      return next;
    }
    next.converged = true;
    next.message = phase.message;
    return next;
  }

  const auto system = detail::assemble_constraint_system_(problem, state.params, state.evaluation);
  const auto platform = state.hessian.platform();
  const std::int64_t n = state.params.total_numel();
  const std::int64_t m_eq = static_cast<std::int64_t>(system.equalities.size());
  const double mu = state.barrier_parameter;
  // Nonlinear inequality rows carry independent slacks; (re)arm them when a recovery step moved the iterate.
  Series nl_slack_cur = state.nl_slack;
  bool slack_rearmed = false;
  {
    const std::int64_t n_nl = detail::count_nonlinear_rows_(system);
    if (options.independent_nonlinear_slacks && n_nl > 0 && !(nl_slack_cur.defined() && nl_slack_cur.numel() == n_nl)) {
      nl_slack_cur = detail::initial_nl_slack_(system, std::max(options.initialization_margin, 1e-2), platform);
      slack_rearmed = true;
    }
  }
  next.nl_slack = nl_slack_cur;
  const Series slack = detail::ip_slack_(system, nl_slack_cur, platform);
  const Series gap = detail::ip_gap_(system, slack, platform);
  const Series dual = detail::safeguard_inequality_dual_(
      detail::current_or_barrier_dual_(state.inequality_dual, slack, mu), slack, mu);
  const Series eq_dual = detail::equality_dual_or_zero_(state.equality_dual, m_eq, platform);
  const Series lagrangian_gradient =
      detail::lagrangian_gradient_(state.evaluation, system, eq_dual, dual, state.normalization);
  const double barrier_grad_abs = detail::series_inf_norm_(lagrangian_gradient);
  const double barrier_grad_norm = barrier_grad_abs / std::max(1.0, state.gradient_scale);
  const double complementarity = detail::complementarity_residual_(slack, dual, mu);
  next.inequality_slack = slack;
  next.inequality_dual = dual;
  next.equality_dual = eq_dual;
  next.primal_residual = std::max(detail::equality_residual_norm_(system), 0.0);
  next.complementarity_residual = complementarity;
  next.barrier_gradient_norm = barrier_grad_norm;

  if (barrier_grad_norm <= options.optimality_tolerance &&
      complementarity <= std::max(options.optimality_tolerance, state.barrier_parameter)) {
    if (state.evaluation.max_constraint_violation <= options.constraint_tolerance &&
        state.barrier_parameter <= options.minimum_barrier_parameter) {
      next.converged = true;
      next.message = "barrier optimality and feasibility tolerances satisfied";
      return next;
    }
    if (state.barrier_parameter > options.minimum_barrier_parameter) {
      next.iteration += 1;
      detail::reduce_barrier_state_(next, system, options);
      detail::maybe_update_best_feasible_(next, options.constraint_tolerance);
      if (next.iteration >= options.max_iterations && !next.converged) {
        next.converged = true;
        next.message = "maximum iterations reached";
      }
      return next;
    }
    if (options.enable_feasibility_phase &&
        state.evaluation.max_constraint_violation > options.constraint_tolerance) {
      next.iteration += 1;
      next.in_feasibility_phase = true;
      next.message = "switching to feasibility phase";
      if (next.iteration >= options.max_iterations && !next.converged) {
        next.converged = true;
        next.message = "maximum iterations reached";
      }
      return next;
    }
    // Declaring the violation stationary needs the gradient small in absolute terms: the relative measure
    // used for convergence is too forgiving for a point that is still infeasible.
    if (barrier_grad_abs <= options.optimality_tolerance) {
      next.converged = true;
      next.message = "constraint violation locally stationary";
      return next;
    }
  }

  // An infeasible point at which the violated constraints cannot be reduced any further (the gradient of their
  // squared violation vanishes) is a stationary point of the violation: no step can restore feasibility. The
  // violated rows are the equalities and the nonlinear inequalities that are positive; each row is weighted by a
  // reference scale taken at the start so the test does not depend on the units a constraint is written in.
  if (state.evaluation.max_constraint_violation > options.constraint_tolerance) {
    const std::size_t m_ineq_rows = system.inequalities.size();
    const bool have_scales = state.stationarity_row_scale.size() == system.equalities.size() + m_ineq_rows;
    Series violation_gradient = detail::zeros_series_(n, platform);
    double residual_l1 = 0.0;
    auto accumulate = [&](const detail::FlatConstraintRow& row, double scale, double residual) {
      violation_gradient = violation_gradient.add(row.jacobian.to(ScalarType::Float64).to(platform).mul(scale * scale * residual));
      residual_l1 += scale * std::abs(residual);
    };
    for (std::size_t i = 0; i < system.equalities.size(); ++i) {
      const double scale = have_scales ? state.stationarity_row_scale[i] : 1.0;
      accumulate(system.equalities[i], scale, system.equalities[i].residual);
    }
    for (std::size_t i = 0; i < m_ineq_rows; ++i) {
      const auto& row = system.inequalities[i];
      if (row.source != detail::ConstraintSource::NonlinearInequality || !(row.residual > 0.0)) continue;
      const double scale = have_scales ? state.stationarity_row_scale[system.equalities.size() + i] : 1.0;
      accumulate(row, scale, row.residual);
    }
    // The gradient must vanish relative to the residual itself: a small residual with a proportionally small
    // gradient is just a nearly feasible point that still has a descent direction.
    if (residual_l1 > 0.0 && detail::series_inf_norm_(violation_gradient) <=
        std::min(options.optimality_tolerance * std::max(1.0, residual_l1) * 1e-2, 1e-4 * residual_l1)) {
      next.converged = true;
      next.message = "constraint violation locally stationary";
      return next;
    }
  }

  // ---- Primal-dual Newton step on the barrier problem ------------------------------------------------
  // With slacks s = -g(x) > 0 and multipliers y (equalities) and z (inequalities), the Newton step for
  //   grad f + J_E' y + J_I' z = 0,   h = 0,   s z = mu
  // reduces, after eliminating ds and dz, to the equality-constrained quadratic program
  //   (W + J_I' diag(z/s) J_I) dx + J_E' y+ = -(grad f + J_I' (mu/s)),   J_E dx = -h,
  // where W approximates the Hessian of the Lagrangian. Equalities enter the KKT system exactly, so they
  // are met to tolerance rather than only penalized.
  const Series variable_scale = state.variable_scale.defined()
                                    ? state.variable_scale.to(ScalarType::Float64).to(platform)
                                    : detail::ones_series_(n, platform);
  const auto equality_scales = detail::equality_row_scales_(system, state.normalization);
  const std::int64_t linear_equality_rows =
      problem.linear.Aeq.has_value() ? problem.linear.Aeq->sizes()[0] : 0;
  std::int64_t nonlinear_inequality_offset = static_cast<std::int64_t>(system.inequalities.size());
  if (const auto count = state.evaluation.nonlinear_inequality_residual; count.defined()) {
    nonlinear_inequality_offset -= count.numel();
  }

  Series barrier_gradient = flatten_values(state.evaluation.gradient)
                                .to(ScalarType::Float64)
                                .mul(state.normalization.objective_scale)
                                .reshape({n});
  if (slack.defined() && slack.numel() > 0) {
    barrier_gradient = detail::add_weighted_row_jacobians_(barrier_gradient, system.inequalities,
                                                           slack.reciprocal().mul(mu));
  }

  // Newton right-hand side with the slack-equation residual folded in: the stationarity row picks up
  // J' diag(z/s) (g + s) because ds = -(g + s) - J dx.
  double gap_l1 = 0.0;
  double gap_barrier_term = 0.0;  // sum_i (mu / s_i) (g_i + s_i)
  Series newton_gradient = barrier_gradient;
  if (slack.defined() && slack.numel() > 0 && gap.abs().max().item<double>() > 0.0) {
    newton_gradient = detail::add_weighted_row_jacobians_(barrier_gradient, system.inequalities, dual.div(slack).mul(gap));
    gap_l1 = gap.abs().sum().item<double>();
    gap_barrier_term = slack.reciprocal().mul(mu).mul(gap).sum().item<double>();
  }

  // Curvature of the Lagrangian.
  const bool use_exact_curvature =
      options.hessian_approximation == OptimaHessianApproximation::Exact ||
      options.hessian_approximation == OptimaHessianApproximation::HessianVector;
  Series lagrangian_hessian = state.hessian.to(ScalarType::Float64).to(platform);
  if (options.hessian_approximation == OptimaHessianApproximation::Lbfgs) {
    lagrangian_hessian = detail::lbfgs_dense_hessian_(state.lbfgs_s_history, state.lbfgs_y_history, n, platform);
  } else if (use_exact_curvature) {
    bool exact_ok = false;
    const Series exact = detail::assemble_lagrangian_hessian_(
        problem, state.params, eq_dual, equality_scales, dual, linear_equality_rows,
        nonlinear_inequality_offset, state.normalization, options.hvp_options, exact_ok);
    if (exact_ok) lagrangian_hessian = exact;
  }
  Series condensed_hessian = lagrangian_hessian;
  if (slack.defined() && slack.numel() > 0) {
    const Series jacobian_ineq = detail::stack_inequality_jacobian_(system, n, platform);
    const std::int64_t m_ineq = slack.numel();
    const Series weights = dual.div(slack).reshape({m_ineq, 1});
    condensed_hessian = condensed_hessian.add(jacobian_ineq.transpose(0, 1).matmul(jacobian_ineq.mul(weights)));
  }

  // Equality rows in the solver's scaled units.
  std::vector<detail::FlatConstraintRow> scaled_equalities = system.equalities;
  double equality_l1 = 0.0;
  for (std::size_t i = 0; i < scaled_equalities.size(); ++i) {
    scaled_equalities[i].jacobian = scaled_equalities[i].jacobian.to(ScalarType::Float64).mul(equality_scales[i]);
    scaled_equalities[i].residual *= equality_scales[i];
    equality_l1 += std::abs(scaled_equalities[i].residual);
  }
  const auto kkt_equalities = detail::scale_constraint_rows_(scaled_equalities, variable_scale);
  const Series scaled_gradient = detail::scale_gradient_(newton_gradient, variable_scale);
  const Series scaled_hessian = detail::scale_hessian_(condensed_hessian, variable_scale);

  OptimaLinearSystemSolver kkt_solver = options.linear_system_solver;
  if (detail::use_iterative_subproblem_solver_(
          options.linear_system_solver, options.system_structure, n, options.iterative_dimension_threshold)) {
    kkt_solver = OptimaLinearSystemSolver::ConjugateGradient;
  } else if (m_eq > 0 && kkt_solver == OptimaLinearSystemSolver::Cholesky) {
    kkt_solver = OptimaLinearSystemSolver::PivotedDirect;
  }

  // Diagonal shift added to the Hessian block only. Start at zero so a well-conditioned step is exact,
  // and grow it when the step is not a descent direction for the merit function.
  const double shift_floor = std::max(options.hessian_regularization, 1e-12);
  double regularization = 0.0;

  Series step_direction = pluss::series::undefined();
  Series eq_dual_target = eq_dual;
  double merit_penalty = std::max(state.merit_penalty, options.equality_penalty);
  double directional_derivative = 0.0;
  bool direction_ok = false;
  std::int64_t krylov_iterations = 0;
  // Increase the diagonal shift until the step is a descent direction for the merit function, as when
  // the Lagrangian curvature is not positive definite on the constraint null space.
  for (int attempt = 0; attempt < 8 && !direction_ok; ++attempt) {
    bool solve_ok = false;
    std::int64_t attempt_krylov = 0;
    Series scaled_step = pluss::series::undefined();
    Series multipliers = pluss::series::undefined();
    if (m_eq > 0 && kkt_solver != OptimaLinearSystemSolver::ConjugateGradient) {
      solve_ok = detail::solve_equality_kkt_dense_(
          scaled_hessian, scaled_gradient, kkt_equalities, regularization, scaled_step, multipliers);
    }
    if (!solve_ok) {
      auto fallback = detail::solve_sqp_kkt_(
          scaled_hessian,
          scaled_gradient,
          kkt_equalities,
          std::max(regularization, shift_floor),
          kkt_solver,
          options.max_krylov_iterations,
          // A primal-dual step must be accurate to the interior iteration's own precision: a loose inner tolerance
          // gives inexact steps that stall the iteration short of the solution. Cap it as sequential QP does.
          std::min(options.krylov_tolerance, 1e-8),
          std::min(options.krylov_absolute_tolerance, 1e-12),
          options.pivot_threshold,
          attempt_krylov,
          solve_ok);
      scaled_step = std::move(fallback.first);
      multipliers = std::move(fallback.second);
    }
    regularization = std::min(std::max(100.0 * regularization, 10.0 * shift_floor), 1e8);
    if (!solve_ok || !scaled_step.defined()) continue;
    const Series candidate = detail::unscale_step_(scaled_step, variable_scale);
    if (!std::isfinite(detail::series_inf_norm_(candidate))) continue;
    double penalty = merit_penalty;
    if (m_eq > 0 && multipliers.defined() && multipliers.numel() == m_eq) {
      penalty = std::max(penalty, 1.1 * detail::series_inf_norm_(multipliers) + 1e-3);
    }
    // The slack-equation residual g + s is penalized like an equality: its weight must exceed the size of the
    // inequality multipliers, or the iterate may drift away from g + s = 0 to lower the barrier.
    if (gap_l1 > 0.0 && dual.defined() && dual.numel() > 0) {
      penalty = std::max(penalty, 1.1 * detail::series_inf_norm_(dual) + 1e-3);
    }
    const double slope = detail::series_dot_(barrier_gradient, candidate) + gap_barrier_term - penalty * (equality_l1 + gap_l1);
    const double candidate_norm = detail::series_inf_norm_(candidate);
    const double position_scale = std::max(1.0, detail::series_inf_norm_(flatten_values(state.params)));
    const bool primal_step_is_zero = candidate_norm <= 1e-12 * position_scale;
    if (primal_step_is_zero || slope < -1e-14 * std::max(1.0, detail::series_dot_(candidate, candidate))) {
      step_direction = candidate;
      directional_derivative = slope;
      merit_penalty = penalty;
      if (m_eq > 0 && multipliers.defined() && multipliers.numel() == m_eq) {
        eq_dual_target = multipliers.to(ScalarType::Float64).to(platform).reshape({m_eq});
      }
      krylov_iterations = attempt_krylov;
      direction_ok = true;
    }
  }
  next.krylov_iterations = std::max(state.krylov_iterations, krylov_iterations);
  if (!direction_ok) {
    step_direction = barrier_gradient.mul(-1.0);
    directional_derivative = -detail::series_dot_(barrier_gradient, barrier_gradient) - merit_penalty * (equality_l1 + gap_l1);
    next.krylov_iterations = state.krylov_iterations;
  }

  // Slack and multiplier steps implied by the primal step, and the largest steps that keep every slack
  // and every inequality multiplier positive (fraction to the boundary).
  const double trust_radius = std::clamp(
      state.trust_region_radius,
      std::max(1e-12, options.trust_region_min_radius),
      std::max(options.trust_region_min_radius, options.trust_region_max_radius));
  double max_step = std::min(options.initial_step_size, trust_radius);
  double dual_step_limit = 1.0;
  Series dual_step = pluss::series::empty({0}, ScalarType::Float64).to(platform);
  Series slack_step = pluss::series::empty({0}, ScalarType::Float64).to(platform);
  if (slack.defined() && slack.numel() > 0) {
    const Series Jdx = detail::stack_inequality_jacobian_(system, n, platform)
                           .matmul(step_direction.reshape({n, 1}))
                           .reshape({slack.numel()});
    slack_step = Jdx.add(gap).mul(-1.0);
    dual_step = slack.reciprocal().mul(mu).sub(dual).add(dual.div(slack).mul(Jdx.add(gap)));
    const Series ones = pluss::series::ones({slack.numel()}, ScalarType::Float64).to(platform);
    const Series inf_fill = ones.mul(std::numeric_limits<double>::infinity());
    const Series slack_mask = slack_step.lt(0.0);
    if (slack_mask.any().item<bool>()) {
      Series candidate = slack.div(slack_mask.where(slack_step.mul(-1.0), ones)).mul(options.fraction_to_boundary);
      max_step = std::min(max_step, slack_mask.where(candidate, inf_fill).amin(0, false).item<double>());
    }
    const Series dual_mask = dual_step.lt(0.0);
    if (dual_mask.any().item<bool>()) {
      Series candidate = dual.div(dual_mask.where(dual_step.mul(-1.0), ones)).mul(options.fraction_to_boundary);
      dual_step_limit = std::min(1.0, dual_mask.where(candidate, inf_fill).amin(0, false).item<double>());
    }
  }
  if (!(max_step > 0.0) || !std::isfinite(max_step)) {
    next.converged = true;
    next.message = "internal-protocol step hit the boundary";
    return next;
  }

  // A zero primal step is legitimate when the point already solves the primal equations and only the
  // multipliers are out of date: apply the multiplier update without a line search.
  if (direction_ok &&
      detail::series_inf_norm_(step_direction) <=
          1e-12 * std::max(1.0, detail::series_inf_norm_(flatten_values(state.params)))) {
    next.iteration += 1;
    next.last_step_size = 0.0;
    next.merit_penalty = merit_penalty;
    next.equality_dual = m_eq > 0 ? eq_dual_target : eq_dual;
    next.inequality_dual = detail::safeguard_inequality_dual_(
        detail::positive_series_(
            dual.numel() > 0 ? detail::update_dual_step_(dual, dual_step, std::min(dual_step_limit, 1.0)) : dual),
        slack,
        mu);
    next.message = "multiplier update";
    next.zero_step_updates = state.message == "multiplier update" ? state.zero_step_updates + 1 : 1;
    if (next.zero_step_updates >= 4) {
      // Several multiplier-only iterations in a row change nothing: the point solves the primal equations but the
      // complementarity measure cannot be brought within tolerance (typically a rounding-level slack). Stop and say
      // so, instead of repeating the same update until the iteration limit.
      next.converged = true;
      next.message = "no progress: repeated multiplier updates no longer change the iterate";
      return next;
    }
    if (nl_slack_cur.defined() && slack_step.numel() == slack.numel() && detail::count_nonlinear_rows_(system) > 0) {
      next.nl_slack = detail::nl_slack_reset_(system, detail::nl_slack_after_(system, slack, slack_step, std::min(1.0, max_step), platform), platform);
      next.barrier_value = detail::merit_value_(state.evaluation, system, mu, state.normalization, merit_penalty, next.nl_slack);
    }
    detail::update_primal_dual_metrics_(next, system, next.barrier_parameter, state.normalization);
    for (int reductions = 0; reductions < 4 && detail::should_reduce_barrier_(next, options); ++reductions) {
      detail::reduce_barrier_state_(next, system, options);
    }
    if (next.iteration >= options.max_iterations && !next.converged) {
      next.converged = true;
      next.message = "maximum iterations reached";
    }
    if (next.barrier_gradient_norm <= options.optimality_tolerance &&
        next.evaluation.max_constraint_violation <= options.constraint_tolerance &&
        next.barrier_parameter <= options.minimum_barrier_parameter) {
      next.converged = true;
      next.message = "barrier optimality and feasibility tolerances satisfied";
    }
    return next;
  }

  double step_size = max_step;
  const bool penalty_changed = merit_penalty != state.merit_penalty || slack_rearmed;
  const double base_value =
      penalty_changed ? detail::merit_value_(state.evaluation, system, mu, state.normalization, merit_penalty, nl_slack_cur)
                      : state.barrier_value;
  const std::vector<double> reference_history =
      penalty_changed ? std::vector<double>{base_value} : state.barrier_history;
  const double watchdog_reference =
      options.enable_watchdog
          ? detail::nonmonotone_reference_(reference_history, options.nonmonotone_window, base_value)
          : base_value;
  bool saw_nonfinite_trial = false;
  const auto state_layout = state.params.layout();
  const Series state_flat64 = flatten_values(state.params).to(ScalarType::Float64);

  for (std::int64_t ls_iter = 0; ls_iter < options.max_line_search_iterations; ++ls_iter) {
    if (step_size * detail::series_inf_norm_(step_direction) <=
        std::max(1e-12, options.step_tolerance)) break;
    auto trial = unflatten_values(
        state_layout,
        state_flat64.add(step_direction.mul(step_size)));
    if (options.project_bounds) trial = detail::project_bounds_(trial, problem.bounds);

    auto trial_eval = evaluate_problem(problem, trial);
    next.function_evaluations += 1;
    if (trial_eval.status == EvalStatus::Ok) {
      auto trial_system = detail::assemble_constraint_system_(problem, trial, trial_eval);
      const Series nl_trial = nl_slack_cur.defined() && detail::count_nonlinear_rows_(trial_system) > 0 && slack_step.numel() == slack.numel()
                                  ? detail::nl_slack_reset_(trial_system, detail::nl_slack_after_(system, slack, slack_step, step_size, platform), platform)
                                  : pluss::series::undefined();
      const double trial_barrier =
          detail::merit_value_(trial_eval, trial_system, mu, state.normalization, merit_penalty, nl_trial);
      const bool current_feasible =
          state.evaluation.max_constraint_violation <= options.constraint_tolerance;
      const bool trial_feasible =
          trial_eval.max_constraint_violation <= options.constraint_tolerance;
      // Near a feasible point, accept a step that both endpoints keep feasible and that still lowers the
      // merit, even if the model predicted a different decrease.
      const bool allow_feasible_progress =
          current_feasible && trial_feasible &&
          std::isfinite(trial_barrier) && trial_barrier < base_value;
      const double predicted_decrease = std::max(1e-14, -step_size * directional_derivative);
      const double actual_decrease = std::isfinite(trial_barrier) ? (base_value - trial_barrier) : -std::numeric_limits<double>::infinity();
      const double trust_ratio = actual_decrease / predicted_decrease;
      const double armijo_rhs =
          watchdog_reference + options.armijo * step_size * directional_derivative + options.watchdog_relaxation;
      const bool armijo_ok = std::isfinite(trial_barrier) && trial_barrier <= armijo_rhs;
      const bool trust_ok =
          std::isfinite(trust_ratio) &&
          (trust_ratio >= options.trust_region_accept_ratio ||
           (allow_feasible_progress && actual_decrease >= -options.watchdog_relaxation));
      if (armijo_ok && trust_ok) {
        next.params = std::move(trial);
        next.evaluation = std::move(trial_eval);
        next.iteration += 1;
        next.last_step_size = step_size;
        next.barrier_parameter = state.barrier_parameter;
        next.barrier_value = trial_barrier;
        next.merit_penalty = merit_penalty;
        next.message = "step accepted";
        if (penalty_changed) next.barrier_history = {base_value};

        const Series x_prev = state_flat64;
        const Series x_next = flatten_values(next.params).to(ScalarType::Float64);
        const Series s = x_next.sub(x_prev);
        const double x_scale = std::max(1.0, detail::series_inf_norm_(x_prev));
        const double step_magnitude = detail::series_inf_norm_(s);
        const double step_tolerance = std::max(1e-14, 1e-10 * x_scale);
        const double barrier_decrease = base_value - trial_barrier;
        const double barrier_decrease_tolerance =
            std::max(1e-12, 1e-10 * std::max(1.0, std::abs(base_value)));
        const bool no_measurable_progress =
            step_magnitude <= step_tolerance && barrier_decrease <= barrier_decrease_tolerance;
        auto next_system = std::move(trial_system);
        next.nl_slack = nl_trial;
        next.inequality_slack = detail::ip_slack_(next_system, next.nl_slack, platform);
        next.inequality_dual = detail::safeguard_inequality_dual_(
            detail::positive_series_(
                dual.numel() > 0 ? detail::update_dual_step_(dual, dual_step, std::min(dual_step_limit, 1.0)) : dual),
            next.inequality_slack,
            next.barrier_parameter);
        next.equality_dual = m_eq > 0 ? eq_dual.add(eq_dual_target.sub(eq_dual).mul(step_size)) : eq_dual;

        // Curvature pair from the Lagrangian gradient at fixed (updated) multipliers.
        const Series gradient_next = detail::lagrangian_gradient_(
            next.evaluation, next_system, next.equality_dual, next.inequality_dual, state.normalization);
        const Series gradient_prev = detail::lagrangian_gradient_(
            state.evaluation, system, next.equality_dual, next.inequality_dual, state.normalization);
        const Series y_pair = gradient_next.sub(gradient_prev).reshape({n});
        if (use_exact_curvature) {
          next.hessian = state.hessian;
          next.lbfgs_s_history.clear();
          next.lbfgs_y_history.clear();
        } else if (options.hessian_approximation == OptimaHessianApproximation::Lbfgs) {
          next.hessian = state.hessian;
          next.lbfgs_s_history = state.lbfgs_s_history;
          next.lbfgs_y_history = state.lbfgs_y_history;
          detail::update_lbfgs_history_(
              next.lbfgs_s_history,
              next.lbfgs_y_history,
              s,
              y_pair,
              options.lbfgs_memory,
              options.bfgs_curvature_tolerance);
        } else if (options.hessian_approximation == OptimaHessianApproximation::Bfgs) {
          next.hessian = detail::damped_bfgs_update_(state.hessian, s, y_pair, options.bfgs_curvature_tolerance);
          next.lbfgs_s_history = state.lbfgs_s_history;
          next.lbfgs_y_history = state.lbfgs_y_history;
        } else {
          next.hessian = detail::identity_matrix_(n, platform);
          next.lbfgs_s_history = state.lbfgs_s_history;
          next.lbfgs_y_history = state.lbfgs_y_history;
        }
        next.trust_region_radius = detail::update_trust_radius_(
            state.trust_region_radius,
            step_size,
            trust_ratio,
            options);
        detail::push_barrier_history_(next.barrier_history, next.barrier_value);
        next.variable_scale = state.variable_scale;
        next.normalization = state.normalization;
        detail::update_primal_dual_metrics_(
            next,
            next_system,
            next.barrier_parameter,
            state.normalization);
        next.in_feasibility_phase = detail::should_enter_feasibility_phase_(next, options);
        const double prev_feas = std::max(state.feasibility_metric, state.evaluation.max_constraint_violation);
        const double next_feas = std::max(next.feasibility_metric, next.evaluation.max_constraint_violation);
        const double feas_drop_ratio =
            (prev_feas - next_feas) / std::max(prev_feas, 1e-12);
        const bool weak_feas_progress =
            next_feas > options.constraint_tolerance &&
            feas_drop_ratio < std::max(0.0, options.insufficient_infeas_decrease);
        next.feasibility_switch_violations =
            (options.enable_feasibility_phase &&
             next.iteration >= options.min_iter_to_feasibility_mode &&
             weak_feas_progress)
                ? (state.feasibility_switch_violations + 1)
                : 0;
        const double obj_curr = state.evaluation.value.item<double>();
        const double obj_next = next.evaluation.value.item<double>();
        const double obj_scale =
            std::max({1.0, std::abs(obj_curr), std::abs(obj_next)});
        const double flat_tol =
            std::max(options.flat_objective_tolerance, options.value_progress_tolerance * obj_scale);
        const bool flat_objective =
            std::abs(obj_next - obj_curr) <= flat_tol &&
            next.evaluation.max_constraint_violation <= options.constraint_tolerance;
        next.flat_objective_iterations =
            flat_objective ? (state.flat_objective_iterations + 1) : 0;

        // Reduce the barrier parameter for as long as the current barrier problem is solved to
        // within its own tolerance.
        for (int reductions = 0; reductions < 4 && detail::should_reduce_barrier_(next, options); ++reductions) {
          detail::reduce_barrier_state_(next, next_system, options);
        }

        detail::maybe_update_best_feasible_(next, options.constraint_tolerance);
        if (detail::near_stationary_feasible_(next, options)) {
          next.converged = true;
          next.message = "barrier progress below tolerance at a feasible point";
        }
        if (next.iteration >= options.max_iterations && !next.converged) {
          next.converged = true;
          next.message = "maximum iterations reached";
        }
        if (next.barrier_gradient_norm <= options.optimality_tolerance &&
            next.evaluation.max_constraint_violation <= options.constraint_tolerance &&
            next.barrier_parameter <= options.minimum_barrier_parameter) {
          next.converged = true;
          next.message = "barrier optimality and feasibility tolerances satisfied";
        }
        if (detail::feasible_objective_target_met_(next.evaluation, options.objective_limit)) {
          next.converged = true;
          next.message = "objective limit reached at a feasible point";
        }
        if (!next.converged &&
            next.flat_objective_iterations >= options.max_flat_objective_iterations &&
            next.evaluation.max_constraint_violation <= options.constraint_tolerance &&
            next.barrier_parameter <= std::max(1e-3, 100.0 * options.minimum_barrier_parameter) &&
            // a run of tiny objective changes is only convergence if the point is also nearly stationary
            next.barrier_gradient_norm <= 1e-3 * std::max(1.0, detail::pack_inf_norm_(next.evaluation.gradient))) {
          next.converged = true;
          next.message = "step tolerance reached at a feasible point";
        }
        const bool weak_barrier_progress =
            std::abs(barrier_decrease) <= std::max(options.value_progress_tolerance, barrier_decrease_tolerance);
        const double stationarity_drop = state.barrier_gradient_norm - next.barrier_gradient_norm;
        const bool weak_stationarity_progress =
            stationarity_drop <= std::max(options.value_progress_tolerance, 1e-12);
        const bool weak_step_progress =
            step_magnitude <= std::max(options.step_tolerance, step_tolerance);
        const bool stalled_major = no_measurable_progress ||
                                   (weak_step_progress && weak_barrier_progress && weak_stationarity_progress);
        next.stalled_iterations = stalled_major ? (state.stalled_iterations + 1) : 0;
        if (!next.converged &&
            options.enable_feasibility_phase &&
            next.feasibility_switch_violations >= options.min_switch_violations) {
          next.in_feasibility_phase = true;
          next.stalled_iterations = 0;
          next.message = "switching to feasibility phase after insufficient infeasibility decrease";
        }
        if (!next.converged &&
            next.stalled_iterations >= options.max_stalled_iterations) {
          if (options.enable_feasibility_phase &&
              next.evaluation.max_constraint_violation > options.constraint_tolerance) {
            next.in_feasibility_phase = true;
            next.stalled_iterations = 0;
            next.message = "switching to feasibility phase after stalled internal-protocol progress";
          } else if (next.evaluation.max_constraint_violation <= options.constraint_tolerance &&
                     next.barrier_gradient_norm <=
                         std::max(10.0 * options.optimality_tolerance, 1e-12)) {
            next.converged = true;
            next.message = "step tolerance reached at a feasible point";
          } else {
            next.converged = true;
            next.message = "internal-protocol no progress across major iterations";
          }
        }
        if (!next.converged && no_measurable_progress) {
          if (next.evaluation.max_constraint_violation <= options.constraint_tolerance &&
              next.barrier_gradient_norm <=
                  std::max(10.0 * options.optimality_tolerance, 1e-12)) {
            next.converged = true;
            next.message = "step tolerance reached at a feasible point";
          } else if (options.enable_feasibility_phase &&
                     next.evaluation.max_constraint_violation > options.constraint_tolerance) {
            next.in_feasibility_phase = true;
            next.stalled_iterations = 0;
            next.message = "switching to feasibility phase after no measurable progress";
          } else if (next.stalled_iterations >= options.max_stalled_iterations) {
            next.converged = true;
            next.message = "internal-protocol no progress across major iterations";
          } else {
            next.message = "accepted step had no measurable progress; continuing with safeguards";
          }
        }
        return next;
      }
    } else if (options.enable_nonfinite_recovery &&
               detail::is_nonfinite_eval_status_(trial_eval.status)) {
      saw_nonfinite_trial = true;
      next.nonfinite_evaluations = state.nonfinite_evaluations + 1;
      step_size *= std::min(options.backtracking, options.nonfinite_backtracking);
      continue;
    }
    step_size *= options.backtracking;
  }

  if (options.enable_soc_correction &&
      options.soc_correction_attempts > 0 &&
      state.evaluation.status == EvalStatus::Ok) {
    ParameterPack<Series> soc_params = state.params;
    ProblemEvaluation soc_eval = state.evaluation;
    for (std::int64_t attempt = 0; attempt < options.soc_correction_attempts; ++attempt) {
      const auto soc = detail::advance_feasibility_phase_(
          problem,
          soc_params,
          soc_eval,
          state.normalization,
          options);
      next.function_evaluations += soc.function_evaluations;
      if (!soc.success || soc.evaluation.status != EvalStatus::Ok) {
        break;
      }
      const auto soc_system = detail::assemble_constraint_system_(problem, soc.params, soc.evaluation);
      const double soc_barrier = detail::merit_value_(
          soc.evaluation,
          soc_system,
          state.barrier_parameter,
          state.normalization,
          merit_penalty);
      const bool improved_feasibility =
          soc.evaluation.max_constraint_violation + 1e-12 < soc_eval.max_constraint_violation;
      const bool improved_barrier =
          std::isfinite(soc_barrier) && soc_barrier + options.watchdog_relaxation < base_value;
      if (improved_feasibility || improved_barrier) {
        next.params = soc.params;
        next.nl_slack = pluss::series::undefined();
        next.evaluation = soc.evaluation;
        next.iteration += 1;
        next.last_step_size = soc.last_step_size;
        next.barrier_parameter = state.barrier_parameter;
        next.barrier_value = soc_barrier;
        next.merit_penalty = merit_penalty;
        next.barrier_history.clear();
        next.message = "second-order correction accepted";
        next.hessian = detail::identity_matrix_(
            next.params.total_numel(),
            next.params.empty() ? pluss::codem::Platform::CPU : next.params[0].value.platform());
        next.variable_scale = state.variable_scale;
        next.normalization = state.normalization;
        next.krylov_iterations = state.krylov_iterations;
        next.lbfgs_s_history.clear();
        next.lbfgs_y_history.clear();
        detail::update_primal_dual_metrics_(
            next,
            soc_system,
            next.barrier_parameter,
            state.normalization);
        {
          const double obj_curr = state.evaluation.value.item<double>();
          const double obj_next = next.evaluation.value.item<double>();
          const double obj_scale = std::max({1.0, std::abs(obj_curr), std::abs(obj_next)});
          const double flat_tol =
              std::max(options.flat_objective_tolerance, options.value_progress_tolerance * obj_scale);
          const bool flat_objective =
              std::abs(obj_next - obj_curr) <= flat_tol &&
              next.evaluation.max_constraint_violation <= options.constraint_tolerance;
          next.flat_objective_iterations =
              flat_objective ? (state.flat_objective_iterations + 1) : 0;
        }
        if (detail::should_reduce_barrier_(next, options)) {
          detail::reduce_barrier_state_(next, soc_system, options);
        }
        next.trust_region_radius = std::max(
            std::max(1e-12, options.trust_region_min_radius),
            std::clamp(
                state.trust_region_radius * std::clamp(options.trust_region_shrink, 1e-3, 1.0),
                options.trust_region_min_radius,
                std::max(options.trust_region_min_radius, options.trust_region_max_radius)));
        detail::push_barrier_history_(next.barrier_history, next.barrier_value);
        next.in_feasibility_phase = detail::should_enter_feasibility_phase_(next, options);
        detail::maybe_update_best_feasible_(next, options.constraint_tolerance);
        if (next.iteration >= options.max_iterations && !next.converged) {
          next.converged = true;
          next.message = "maximum iterations reached";
        }
        if (next.barrier_gradient_norm <= options.optimality_tolerance &&
            next.evaluation.max_constraint_violation <= options.constraint_tolerance &&
            next.barrier_parameter <= options.minimum_barrier_parameter) {
          next.converged = true;
          next.message = "barrier optimality and feasibility tolerances satisfied";
        }
        return next;
      }
      soc_params = soc.params;
      soc_eval = soc.evaluation;
    }
  }

  if (options.enable_restoration) {
    const auto recovery = detail::recover_barrier_descent_(
        problem,
        state.params,
        state.evaluation,
        state.normalization,
        state.barrier_parameter,
        options);
    next.function_evaluations += recovery.function_evaluations;
    if (recovery.success) {
      next.params = recovery.params;
      next.nl_slack = pluss::series::undefined();
      next.evaluation = recovery.evaluation;
      next.iteration += 1;
      next.last_step_size = recovery.last_step_size;
      next.barrier_parameter = state.barrier_parameter;
      const auto next_system = detail::assemble_constraint_system_(problem, next.params, next.evaluation);
      next.barrier_value = detail::merit_value_(
          next.evaluation,
          next_system,
          next.barrier_parameter,
          state.normalization,
          next.merit_penalty);
      next.barrier_history.clear();
      next.hessian = detail::identity_matrix_(
          next.params.total_numel(),
          next.params.empty() ? pluss::codem::Platform::CPU : next.params[0].value.platform());
      next.message = "recovery step accepted";
      next.variable_scale = state.variable_scale;
      next.normalization = state.normalization;
      next.krylov_iterations = state.krylov_iterations;
      next.lbfgs_s_history.clear();
      next.lbfgs_y_history.clear();
      next.inequality_dual = detail::blend_dual_toward_centrality_(
          detail::current_or_barrier_dual_(next.inequality_dual, next.inequality_slack, next.barrier_parameter),
          next.inequality_slack,
          next.barrier_parameter,
          1.0);
      detail::update_primal_dual_metrics_(
          next,
          next_system,
          next.barrier_parameter,
          state.normalization);
      {
        const double obj_curr = state.evaluation.value.item<double>();
        const double obj_next = next.evaluation.value.item<double>();
        const double obj_scale = std::max({1.0, std::abs(obj_curr), std::abs(obj_next)});
        const double flat_tol =
            std::max(options.flat_objective_tolerance, options.value_progress_tolerance * obj_scale);
        const bool flat_objective =
            std::abs(obj_next - obj_curr) <= flat_tol &&
            next.evaluation.max_constraint_violation <= options.constraint_tolerance;
        next.flat_objective_iterations =
            flat_objective ? (state.flat_objective_iterations + 1) : 0;
      }
      next.trust_region_radius = detail::update_trust_radius_(
          state.trust_region_radius,
          std::max(recovery.last_step_size, 0.0),
          0.5,
          options);
      detail::push_barrier_history_(next.barrier_history, next.barrier_value);
      const double recovery_step_magnitude = detail::series_inf_norm_(
          flatten_values(next.params).to(ScalarType::Float64).sub(
              flatten_values(state.params).to(ScalarType::Float64)));
      const double recovery_scale = std::max(1.0, detail::series_inf_norm_(flatten_values(state.params).to(ScalarType::Float64)));
      const double recovery_tol = std::max(options.step_tolerance, 1e-10 * recovery_scale);
      next.stalled_iterations =
          (recovery_step_magnitude <= recovery_tol) ? (state.stalled_iterations + 1) : 0;
      next.feasibility_switch_violations = 0;
      if (detail::should_reduce_barrier_(next, options)) {
        detail::reduce_barrier_state_(next, next_system, options);
        // Keep recovery-provenance messaging even if we also reduce mu.
        next.message = "recovery step accepted";
      }
      next.in_feasibility_phase = detail::should_enter_feasibility_phase_(next, options);
      detail::maybe_update_best_feasible_(next, options.constraint_tolerance);
      if (next.iteration >= options.max_iterations && !next.converged) {
        next.converged = true;
        next.message = "maximum iterations reached";
      }
      if (next.barrier_gradient_norm <= options.optimality_tolerance &&
          next.evaluation.max_constraint_violation <= options.constraint_tolerance &&
          next.barrier_parameter <= options.minimum_barrier_parameter) {
        next.converged = true;
        next.message = "barrier optimality and feasibility tolerances satisfied";
      }
      if (detail::feasible_objective_target_met_(next.evaluation, options.objective_limit)) {
        next.converged = true;
        next.message = "objective limit reached at a feasible point";
      }
      if (!next.converged && next.stalled_iterations >= options.max_stalled_iterations) {
        next.converged = true;
        next.message = "internal-protocol no progress after repeated recovery steps";
      }
      return next;
    }
    // Feasible-stall rescue: if barrier restoration cannot progress at an
    // already-feasible point, attempt a conservative objective-descent step.
    if (state.evaluation.max_constraint_violation <= options.constraint_tolerance &&
        state.evaluation.status == EvalStatus::Ok) {
      const Series x_prev = flatten_values(state.params).to(ScalarType::Float64);
      const Series obj_grad = flatten_values(state.evaluation.gradient).to(ScalarType::Float64);
      const double grad_norm = detail::series_inf_norm_(obj_grad);
      if (std::isfinite(grad_norm) && grad_norm > 1e-12) {
        Series direction = obj_grad.mul(-1.0);
        const double directional = detail::series_dot_(obj_grad, direction);
        double fallback_step = std::min(std::max(max_step, 0.0), std::max(options.initial_step_size, 1e-6));
        bool fallback_accepted = false;
        for (std::int64_t ls = 0; ls < options.max_restoration_line_search_iterations; ++ls) {
          if (fallback_step * detail::series_inf_norm_(direction) <=
              std::max(1e-12, options.step_tolerance)) break;
          auto trial = unflatten_values(
              state.params.layout(),
              x_prev.add(direction.mul(fallback_step)));
          if (options.project_bounds) trial = detail::project_bounds_(trial, problem.bounds);

          auto trial_eval = evaluate_problem(problem, trial);
          next.function_evaluations += 1;
          if (trial_eval.status == EvalStatus::Ok) {
            const double base_obj = state.evaluation.value.item<double>();
            const double trial_obj = trial_eval.value.item<double>();
            const auto trial_system = detail::assemble_constraint_system_(problem, trial, trial_eval);
            const double trial_barrier = detail::merit_value_(
                trial_eval,
                trial_system,
                state.barrier_parameter,
                state.normalization,
                merit_penalty);
            if (std::isfinite(trial_obj) &&
                std::isfinite(trial_barrier) &&
                trial_obj <= base_obj + options.armijo * fallback_step * directional) {
              next.params = std::move(trial);
              next.nl_slack = pluss::series::undefined();
              next.evaluation = std::move(trial_eval);
              next.iteration += 1;
              next.last_step_size = fallback_step;
              next.barrier_parameter = state.barrier_parameter;
              next.barrier_value = trial_barrier;
              next.merit_penalty = merit_penalty;
              next.barrier_history.clear();
              next.message = "fallback objective-descent step accepted";
              next.hessian = detail::identity_matrix_(
                  next.params.total_numel(),
                  next.params.empty() ? pluss::codem::Platform::CPU : next.params[0].value.platform());
              next.variable_scale = state.variable_scale;
              next.normalization = state.normalization;
              next.krylov_iterations = state.krylov_iterations;
              next.lbfgs_s_history.clear();
              next.lbfgs_y_history.clear();
              next.feasibility_switch_violations = 0;
              const auto next_system = detail::assemble_constraint_system_(problem, next.params, next.evaluation);
              detail::update_primal_dual_metrics_(
                  next,
                  next_system,
                  next.barrier_parameter,
                  state.normalization);
              {
                const double obj_curr = state.evaluation.value.item<double>();
                const double obj_next = next.evaluation.value.item<double>();
                const double obj_scale = std::max({1.0, std::abs(obj_curr), std::abs(obj_next)});
                const double flat_tol =
                    std::max(options.flat_objective_tolerance, options.value_progress_tolerance * obj_scale);
                const bool flat_objective =
                    std::abs(obj_next - obj_curr) <= flat_tol &&
                    next.evaluation.max_constraint_violation <= options.constraint_tolerance;
                next.flat_objective_iterations =
                    flat_objective ? (state.flat_objective_iterations + 1) : 0;
              }
              next.trust_region_radius = detail::update_trust_radius_(
                  state.trust_region_radius,
                  std::max(fallback_step, 0.0),
                  0.5,
                  options);
              detail::push_barrier_history_(next.barrier_history, next.barrier_value);
              if (detail::should_reduce_barrier_(next, options)) {
                detail::reduce_barrier_state_(next, next_system, options);
                next.message = "fallback objective-descent step accepted";
              }
              next.in_feasibility_phase = detail::should_enter_feasibility_phase_(next, options);
              detail::maybe_update_best_feasible_(next, options.constraint_tolerance);
              if (next.iteration >= options.max_iterations && !next.converged) {
                next.converged = true;
                next.message = "maximum iterations reached";
              }
              if (next.barrier_gradient_norm <= options.optimality_tolerance &&
                  next.evaluation.max_constraint_violation <= options.constraint_tolerance &&
                  next.barrier_parameter <= options.minimum_barrier_parameter) {
                next.converged = true;
                next.message = "barrier optimality and feasibility tolerances satisfied";
              }
              fallback_accepted = true;
              break;
            }
          }
          if (trial_eval.status != EvalStatus::Ok &&
              options.enable_nonfinite_recovery &&
              detail::is_nonfinite_eval_status_(trial_eval.status)) {
            saw_nonfinite_trial = true;
            next.nonfinite_evaluations = state.nonfinite_evaluations + 1;
            fallback_step *= std::min(options.backtracking, options.nonfinite_backtracking);
          } else {
            fallback_step *= options.backtracking;
          }
        }
        if (fallback_accepted) {
          return next;
        }
      }
    }
    if (options.enable_feasibility_phase) {
      if (next.evaluation.max_constraint_violation > options.constraint_tolerance) {
        next.in_feasibility_phase = true;
        next.message = recovery.message.empty()
                           ? "barrier recovery failed; switching to feasibility phase"
                           : recovery.message + "; switching to feasibility phase";
        if (saw_nonfinite_trial) {
          next.message += " after non-finite trial evaluations";
        }
        return next;
      }
    }
    const bool feasible_recovery_stall =
        state.evaluation.max_constraint_violation <= options.constraint_tolerance &&
        (recovery.message.find("made no progress at a feasible point") != std::string::npos ||
         recovery.message.find("failed to reduce the barrier objective") != std::string::npos);
    if (feasible_recovery_stall && state.barrier_parameter > options.minimum_barrier_parameter) {
      next.iteration += 1;
      next.barrier_parameter = std::max(
          options.minimum_barrier_parameter,
          state.barrier_parameter * std::clamp(options.barrier_decay, 0.0, 1.0));
      next.trust_region_radius = std::max(
          std::max(1e-12, options.trust_region_min_radius),
          std::clamp(
              state.trust_region_radius * std::clamp(options.trust_region_shrink, 1e-3, 1.0),
              options.trust_region_min_radius,
              std::max(options.trust_region_min_radius, options.trust_region_max_radius)));
      next.stalled_iterations = state.stalled_iterations + 1;
      next.flat_objective_iterations = state.flat_objective_iterations + 1;
      next.message = "recovery stalled at feasible point; reduced barrier and retrying";
      if (next.stalled_iterations >= options.max_stalled_iterations ||
          next.flat_objective_iterations >= options.max_flat_objective_iterations ||
          next.iteration >= options.max_iterations) {
        next.converged = true;
        next.message = state.barrier_gradient_norm <= std::max(10.0 * options.optimality_tolerance, 1e-12)
                           ? "step tolerance reached at a feasible point"
                           : "line search failed to produce an acceptable internal-protocol step after recovery";
      }
      return next;
    }
    next.message = recovery.message.empty()
                       ? "line search failed to produce an acceptable internal-protocol step after recovery"
                       : recovery.message;
    if (saw_nonfinite_trial) {
      next.message += "; non-finite trial evaluations encountered";
    }
    next.converged = true;
    return next;
  }

  if (options.enable_feasibility_phase) {
    if (next.evaluation.max_constraint_violation > options.constraint_tolerance) {
      next.in_feasibility_phase = true;
      next.message = "switching to feasibility phase";
      if (saw_nonfinite_trial) {
        next.message += " and non-finite trial evaluations";
      }
      return next;
    }
  }

  next.converged = true;
  next.message = "line search failed to produce an acceptable internal-protocol step";
  if (saw_nonfinite_trial) {
    next.message += "; non-finite trial evaluations encountered";
  }
  return next;
}

inline InternalProtocolState solve_internal_protocol(const ConstrainedProblem& problem,
                                               const ParameterPack<Series>& initial_params,
                                               const InternalProtocolOptions& options = {}) {
  InternalProtocolState state = init_internal_protocol_state(problem, initial_params, options);
  std::string progress_message;
  if (detail::run_progress_hook_(
          options.progress_hook,
          detail::make_progress_snapshot_(OptimaAlgorithm::InternalProtocol, OptimaProgressStage::Start, state),
          &progress_message)) {
    state.converged = true;
    state.message = progress_message.empty() ? "stopped by progress hook" : progress_message;
  }
  while (!state.converged && state.iteration < options.max_iterations) {
    InternalProtocolState next = internal_protocol_step(problem, state, options);
    if (next.iteration == state.iteration && next.message == state.message &&
        next.converged == state.converged) {
      break;
    }
    state = std::move(next);
    progress_message.clear();
    if (detail::run_progress_hook_(
            options.progress_hook,
            detail::make_progress_snapshot_(OptimaAlgorithm::InternalProtocol, OptimaProgressStage::Iteration, state),
            &progress_message)) {
      state.converged = true;
      state.message = progress_message.empty() ? "stopped by progress hook" : progress_message;
      break;
    }
  }
  if (!state.converged && state.iteration >= options.max_iterations &&
      detail::near_stationary_feasible_(state, options)) {
    state.converged = true;
    state.message = "iteration budget reached near-stationary feasible point";
  }
  if (!state.converged && state.best_feasible.has_value()) {
    auto best_evaluation = evaluate_problem(problem, state.best_feasible->params);
    ++state.function_evaluations;
    if (best_evaluation.status == EvalStatus::Ok) {
      const auto best_system = detail::assemble_constraint_system_(problem, state.best_feasible->params, best_evaluation);
      InternalProtocolState best_state = state;
      best_state.params = state.best_feasible->params;
      best_state.evaluation = std::move(best_evaluation);
      detail::update_primal_dual_metrics_(
          best_state,
          best_system,
          best_state.barrier_parameter,
          best_state.normalization);
      if (best_state.evaluation.max_constraint_violation <= options.constraint_tolerance &&
          (best_state.barrier_gradient_norm <= options.optimality_tolerance ||
           detail::near_stationary_feasible_(best_state, options))) {
        best_state.converged = true;
        best_state.message = detail::near_stationary_feasible_(best_state, options)
                                 ? "returned near-stationary best feasible point"
                                 : "returned best feasible point satisfying tolerances";
        state = std::move(best_state);
      }
    }
  }
  detail::run_progress_hook_(
      options.progress_hook,
      detail::make_progress_snapshot_(OptimaAlgorithm::InternalProtocol, OptimaProgressStage::Done, state));
  return state;
}

inline OptimaState to_optima_state(const ConstrainedProblem& problem, const PenaltyMethodState& state) {
  ConstraintMultipliers multipliers = detail::zero_constraint_multipliers_(problem, state.params, state.evaluation);
  double stationarity = detail::pack_inf_norm_(state.evaluation.gradient);
  if (state.evaluation.status == EvalStatus::Ok && state.evaluation.nonlinear_equality_residual.defined() &&
      state.evaluation.nonlinear_inequality_residual.defined()) {
    const auto platform = state.params.empty() ? pluss::codem::Platform::CPU : state.params[0].value.platform();
    const std::int64_t n = state.params.total_numel();
    const auto system = detail::assemble_constraint_system_(problem, state.params, state.evaluation);
    const auto inequalities = detail::non_bound_inequality_rows_(system);
    const Series lambda = detail::multiplier_or_zero_(state.equality_multipliers, static_cast<std::int64_t>(system.equalities.size()), platform);
    const Series mu = detail::multiplier_or_zero_(state.inequality_multipliers, static_cast<std::int64_t>(inequalities.size()), platform);
    const auto al = detail::augmented_lagrangian_(
        state.evaluation, system.equalities, inequalities, lambda, mu, state.current_penalty);
    // Multipliers of the Lagrangian at the returned point: the stored estimate plus the current penalty
    // contribution, which is the quantity the gradient above was built from.
    std::vector<double> equality_weights(al.equality_weight);
    std::vector<double> inequality_weights(al.inequality_weight);
    const auto split = [&](const std::vector<double>& values, const std::vector<detail::FlatConstraintRow>& rows,
                           detail::ConstraintSource first, Series& first_out, Series& second_out) {
      std::vector<double> a, b;
      for (std::size_t i = 0; i < rows.size(); ++i) (rows[i].source == first ? a : b).push_back(values[i]);
      auto make = [&](const std::vector<double>& v, Series& out) {
        if (out.defined() && out.numel() == static_cast<std::int64_t>(v.size()) && !v.empty()) {
          out = pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(ScalarType::Float64).to(platform);
        }
      };
      make(a, first_out);
      make(b, second_out);
    };
    split(equality_weights, system.equalities, detail::ConstraintSource::LinearEquality,
          multipliers.linear_equalities, multipliers.nonlinear_equalities);
    split(inequality_weights, inequalities, detail::ConstraintSource::LinearInequality,
          multipliers.linear_inequalities, multipliers.nonlinear_inequalities);
    const auto [lower, upper] = detail::flat_bound_vectors_(problem, state.params);
    const Series x = flatten_values(state.params).to(ScalarType::Float64).reshape({n});
    stationarity = detail::projected_gradient_norm_(x, al.gradient, lower, upper);
    if (multipliers.lower_bounds.defined() && multipliers.lower_bounds.numel() == n) {
      multipliers.lower_bounds =
          x.le(lower.add(1e-9)).where(al.gradient.clamp_min(0.0), pluss::series::zeros_like(al.gradient));
    }
    if (multipliers.upper_bounds.defined() && multipliers.upper_bounds.numel() == n) {
      multipliers.upper_bounds =
          x.ge(upper.sub(1e-9)).where(al.gradient.mul(-1.0).clamp_min(0.0), pluss::series::zeros_like(al.gradient));
    }
  }
  OptimaState out{
      OptimaAlgorithm::MeritMethod,
      state.params,
      state.evaluation,
      state.iteration,
      state.function_evaluations,
      state.last_step_size,
      state.current_penalty,
      stationarity,
      state.converged,
      detail::exit_flag_from_state_(state.converged, state.message, state.evaluation.status),
      state.message,
      {},
      multipliers,
      state.best_feasible};
  out.output = detail::make_output_(
      out.algorithm,
      out.exit_flag,
      out.evaluation,
      out.iteration,
      out.function_evaluations,
      out.last_step_size,
      out.backend_parameter,
      out.stationarity,
      0.0,
      out.multipliers,
      out.best_feasible,
      out.message);
  out.message = out.output.detail_message;
  return out;
}

inline OptimaState to_optima_state(const ConstrainedProblem& problem,
                                   const SequentialQPState& state,
                                   OptimaAlgorithm algorithm) {
  OptimaState out{
      algorithm,
      state.params,
      state.evaluation,
      state.iteration,
      state.function_evaluations,
      state.last_step_size,
      state.current_penalty,
      state.kkt_residual,
      state.converged,
      detail::exit_flag_from_state_(state.converged, state.message, state.evaluation.status),
      state.message,
      {},
      {},
      state.best_feasible};
  if (state.evaluation.status == EvalStatus::Ok) {
    const auto system = detail::assemble_constraint_system_(problem, state.params, state.evaluation);
    const auto active = detail::active_constraints_(system, 1e-4);
    const Series lambda = detail::estimate_kkt_multipliers_(
        flatten_values(state.evaluation.gradient).to(ScalarType::Float64),
        active);
    out.multipliers =
        detail::gather_constraint_multipliers_(problem, state.params, state.evaluation, active, lambda);
    // Report first-order optimality at the returned point from its own multiplier estimate, so it
    // is meaningful even when the last iteration ended on a fallback direction.
    out.stationarity = detail::kkt_residual_(
        flatten_values(state.evaluation.gradient).to(ScalarType::Float64), active, lambda);
  } else {
    out.multipliers = detail::zero_constraint_multipliers_(problem, state.params, state.evaluation);
  }
  out.output = detail::make_output_(
      out.algorithm,
      out.exit_flag,
      out.evaluation,
      out.iteration,
      out.function_evaluations,
      out.last_step_size,
      out.backend_parameter,
      out.stationarity,
      0.0,
      out.multipliers,
      out.best_feasible,
      out.message,
      state.krylov_iterations);
  out.message = out.output.detail_message;
  return out;
}

inline OptimaState to_optima_state(const ConstrainedProblem& problem, const SequentialQPState& state) {
  return to_optima_state(problem, state, OptimaAlgorithm::SequentialQP);
}

inline OptimaState to_optima_state(const ConstrainedProblem& problem, const InternalProtocolState& state) {
  const double reported_complementarity =
      state.iteration <= 1
          ? std::max(state.complementarity_residual, std::max(state.barrier_parameter, 0.0))
          : state.complementarity_residual;
  OptimaState out{
      OptimaAlgorithm::InternalProtocol,
      state.params,
      state.evaluation,
      state.iteration,
      state.function_evaluations,
      state.last_step_size,
      state.barrier_parameter,
      state.barrier_gradient_norm,
      state.converged,
      detail::exit_flag_from_state_(state.converged, state.message, state.evaluation.status),
      state.message,
      {},
      {},
      state.best_feasible};
  if (state.evaluation.status == EvalStatus::Ok) {
    const auto system = detail::assemble_constraint_system_(problem, state.params, state.evaluation);
    out.multipliers = state.inequality_dual.defined() && state.inequality_dual.numel() == static_cast<std::int64_t>(system.inequalities.size())
                          ? detail::gather_primal_dual_inequality_multipliers_(
                                problem, state.params, state.evaluation, system, state.inequality_dual)
                          : detail::estimate_barrier_multipliers_(
                                problem, state.params, state.evaluation, system, state.barrier_parameter);
  } else {
    out.multipliers = detail::zero_constraint_multipliers_(problem, state.params, state.evaluation);
  }
  out.output = detail::make_output_(
      out.algorithm,
      out.exit_flag,
      out.evaluation,
      out.iteration,
      out.function_evaluations,
      out.last_step_size,
      out.backend_parameter,
      out.stationarity,
      reported_complementarity,
      out.multipliers,
      out.best_feasible,
      out.message,
      state.krylov_iterations);
  out.message = out.output.detail_message;
  return out;
}

namespace detail {

// Automatic scaling of the objective and the constraint rows, done once at the start by gradient size. A problem
// written in extreme units (an objective in cents, a constraint in billions) makes every absolute tolerance
// mean something different from what the caller intended, and every algorithm then stops early, stalls, or
// cannot meet the tolerance. The cure is to solve the problem the caller meant: multiply the objective and each
// constraint row by a constant so that its gradient at the start has a reasonable size, run the algorithm on that,
// and map the answer (value, residuals, multipliers) back. Bounds are not scaled (they are exact), and a function
// whose gradient is already between 1e-2 and 1e2 is left alone; the objective is only ever scaled down.
struct ProblemScaling {
  double objective{1.0};
  std::vector<double> linear_equality;
  std::vector<double> nonlinear_equality;
  std::vector<double> linear_inequality;
  std::vector<double> nonlinear_inequality;
  bool active{false};
};

inline double gradient_balance_(double gradient_norm, bool allow_scale_up) {
  if (!std::isfinite(gradient_norm) || !(gradient_norm > 1e-8)) return 1.0;
  if (gradient_norm > 100.0) return 1.0 / gradient_norm;
  if (allow_scale_up && gradient_norm < 1e-2) return 1.0 / gradient_norm;
  return 1.0;
}

inline ProblemScaling compute_gradient_scaling_(const ConstrainedProblem& problem,
                                                const ParameterPack<Series>& params) {
  ProblemScaling out;
  const ProblemEvaluation evaluation = evaluate_problem(problem, params);
  if (evaluation.status != EvalStatus::Ok) return out;
  const FlatConstraintSystem system = assemble_constraint_system_(problem, params, evaluation);
  const std::int64_t n_lin_eq = problem.linear.Aeq.has_value() ? problem.linear.Aeq->sizes()[0] : 0;
  const std::int64_t n_nl_eq = evaluation.nonlinear_equality_residual.defined() ? evaluation.nonlinear_equality_residual.numel() : 0;
  const std::int64_t n_lin_ineq = problem.linear.Aineq.has_value() ? problem.linear.Aineq->sizes()[0] : 0;
  const std::int64_t n_nl_ineq = evaluation.nonlinear_inequality_residual.defined() ? evaluation.nonlinear_inequality_residual.numel() : 0;
  out.linear_equality.assign(static_cast<std::size_t>(n_lin_eq), 1.0);
  out.nonlinear_equality.assign(static_cast<std::size_t>(n_nl_eq), 1.0);
  out.linear_inequality.assign(static_cast<std::size_t>(n_lin_ineq), 1.0);
  out.nonlinear_inequality.assign(static_cast<std::size_t>(n_nl_ineq), 1.0);
  out.objective = gradient_balance_(pack_inf_norm_(evaluation.gradient), /*allow_scale_up=*/false);
  const auto assign = [&](const std::vector<FlatConstraintRow>& rows) {
    for (const auto& row : rows) {
      const double w = gradient_balance_(series_inf_norm_(row.jacobian), /*allow_scale_up=*/true);
      switch (row.source) {
        case ConstraintSource::LinearEquality: out.linear_equality[static_cast<std::size_t>(row.index)] = w; break;
        case ConstraintSource::NonlinearEquality: out.nonlinear_equality[static_cast<std::size_t>(row.index)] = w; break;
        case ConstraintSource::LinearInequality: out.linear_inequality[static_cast<std::size_t>(row.index)] = w; break;
        case ConstraintSource::NonlinearInequality: out.nonlinear_inequality[static_cast<std::size_t>(row.index)] = w; break;
        default: break;  // bounds are exact and stay unscaled
      }
    }
  };
  assign(system.equalities);
  assign(system.inequalities);
  const auto any_scaled = [](const std::vector<double>& v) {
    return std::any_of(v.begin(), v.end(), [](double w) { return w != 1.0; });
  };
  out.active = out.objective != 1.0 || any_scaled(out.linear_equality) || any_scaled(out.nonlinear_equality) ||
               any_scaled(out.linear_inequality) || any_scaled(out.nonlinear_inequality);
  return out;
}

inline Series scale_weights_series_(const std::vector<double>& w) {
  return pluss::make_series(w, {static_cast<std::int64_t>(w.size())}).to(ScalarType::Float64);
}

inline Series scale_jacobian_rows_(const Series& value, const std::vector<double>& w) {
  const std::int64_t m = static_cast<std::int64_t>(w.size());
  if (m == 0 || value.numel() == 0) return value;
  const std::int64_t cols = value.numel() / m;
  return value.reshape({m, cols}).mul(scale_weights_series_(w).to(value.platform()).reshape({m, 1})).reshape(value.sizes());
}

inline ParameterPack<Series> scale_jacobian_pack_(const ParameterPack<Series>& pack, const std::vector<double>& w) {
  std::vector<ParameterPack<Series>::Block> blocks;
  for (const auto& b : pack.blocks()) blocks.push_back({b.name, scale_jacobian_rows_(b.value, w)});
  return ParameterPack<Series>(std::move(blocks));
}

inline ConstrainedProblem apply_problem_scaling_(const ConstrainedProblem& problem, const ProblemScaling& sc) {
  ConstrainedProblem out = problem;
  const double sf = sc.objective;
  const auto any = [](const std::vector<double>& v) {
    return std::any_of(v.begin(), v.end(), [](double w) { return w != 1.0; });
  };
  if (sf != 1.0) {
    if (problem.objective.autodiff)
      out.objective.autodiff = TraceCallback::Base([f = problem.objective.autodiff, sf](const ParameterPack<detail::Trace>& p) { return f(p) * sf; });
    if (problem.objective.analytic)
      out.objective.analytic = [f = problem.objective.analytic, sf](const ParameterPack<Series>& p) {
        auto [value, grad] = f(p);
        return std::pair<Series, ParameterPack<Series>>{value.mul(sf), scaled_pack_(grad, sf)};
      };
  }
  if (any(sc.linear_equality) && problem.linear.Aeq.has_value()) {
    const std::int64_t m = static_cast<std::int64_t>(sc.linear_equality.size());
    const Series w = scale_weights_series_(sc.linear_equality).to(problem.linear.Aeq->platform());
    out.linear.Aeq = problem.linear.Aeq->reshape({m, problem.linear.Aeq->numel() / m}).mul(w.reshape({m, 1}));
    if (problem.linear.beq.has_value()) out.linear.beq = problem.linear.beq->reshape({m}).mul(w);
  }
  if (any(sc.linear_inequality) && problem.linear.Aineq.has_value()) {
    const std::int64_t m = static_cast<std::int64_t>(sc.linear_inequality.size());
    const Series w = scale_weights_series_(sc.linear_inequality).to(problem.linear.Aineq->platform());
    out.linear.Aineq = problem.linear.Aineq->reshape({m, problem.linear.Aineq->numel() / m}).mul(w.reshape({m, 1}));
    if (problem.linear.bineq.has_value()) out.linear.bineq = problem.linear.bineq->reshape({m}).mul(w);
  }
  const bool scale_eq = any(sc.nonlinear_equality);
  const bool scale_ineq = any(sc.nonlinear_inequality);
  if (scale_eq) {
    const Series w = scale_weights_series_(sc.nonlinear_equality);
    if (problem.nonlinear.equality)
      out.nonlinear.equality = TraceCallback::Base([f = problem.nonlinear.equality, w](const ParameterPack<detail::Trace>& p) {
        detail::Trace v = f(p);
        return v * constant_autodiff_(v, w.to(v.value().platform()).reshape({v.value().numel()}));
      });
  }
  if (scale_ineq) {
    const Series w = scale_weights_series_(sc.nonlinear_inequality);
    if (problem.nonlinear.inequality)
      out.nonlinear.inequality = TraceCallback::Base([f = problem.nonlinear.inequality, w](const ParameterPack<detail::Trace>& p) {
        detail::Trace v = f(p);
        return v * constant_autodiff_(v, w.to(v.value().platform()).reshape({v.value().numel()}));
      });
  }
  if ((scale_eq || scale_ineq) && problem.nonlinear.analytic) {
    out.nonlinear.analytic = [f = problem.nonlinear.analytic, we = sc.nonlinear_equality, wi = sc.nonlinear_inequality](
                                 const ParameterPack<Series>& p) {
      NonlinearConstraintEvaluation e = f(p);
      if (e.equality.defined() && !we.empty()) {
        e.equality = e.equality.mul(scale_weights_series_(we).to(e.equality.platform()).reshape({e.equality.numel()}));
        e.equality_jacobian = scale_jacobian_pack_(e.equality_jacobian, we);
      }
      if (e.inequality.defined() && !wi.empty()) {
        e.inequality = e.inequality.mul(scale_weights_series_(wi).to(e.inequality.platform()).reshape({e.inequality.numel()}));
        e.inequality_jacobian = scale_jacobian_pack_(e.inequality_jacobian, wi);
      }
      return e;
    };
  }
  return out;
}

// Multipliers of the scaled problem, in the units of the original one: the Lagrangian of the scaled problem is
// s_f f + sum (lambda~ s_i) c_i, so lambda_i = lambda~_i s_i / s_f, and a bound multiplier is divided by s_f.
inline Series unscale_multiplier_(const Series& m, const std::vector<double>& w, double sf) {
  if (!m.defined() || m.numel() == 0) return m;
  std::vector<double> factor(static_cast<std::size_t>(m.numel()), 1.0 / sf);
  for (std::size_t i = 0; i < w.size() && i < factor.size(); ++i) factor[i] = w[i] / sf;
  return m.to(ScalarType::Float64).reshape({m.numel()}).mul(scale_weights_series_(factor).to(m.platform()));
}

inline void unscale_optima_state_(OptimaState& state, const ConstrainedProblem& original, const ProblemScaling& sc) {
  const ProblemEvaluation e = evaluate_problem(original, state.params);
  if (e.status == EvalStatus::Ok) {
    state.evaluation = e;
    state.output.constraint_violation = e.max_constraint_violation;
  }
  const double sf = sc.objective;
  state.multipliers.linear_equalities = unscale_multiplier_(state.multipliers.linear_equalities, sc.linear_equality, sf);
  state.multipliers.nonlinear_equalities = unscale_multiplier_(state.multipliers.nonlinear_equalities, sc.nonlinear_equality, sf);
  state.multipliers.linear_inequalities = unscale_multiplier_(state.multipliers.linear_inequalities, sc.linear_inequality, sf);
  state.multipliers.nonlinear_inequalities = unscale_multiplier_(state.multipliers.nonlinear_inequalities, sc.nonlinear_inequality, sf);
  state.multipliers.lower_bounds = unscale_multiplier_(state.multipliers.lower_bounds, {}, sf);
  state.multipliers.upper_bounds = unscale_multiplier_(state.multipliers.upper_bounds, {}, sf);
  state.output.multiplier_max_norm = multiplier_max_norm_(state.multipliers);
  if (state.best_feasible.has_value()) {
    const ProblemEvaluation b = evaluate_problem(original, state.best_feasible->params);
    if (b.status == EvalStatus::Ok) {
      state.best_feasible->value = b.value;
      state.best_feasible->max_constraint_violation = b.max_constraint_violation;
      state.output.best_feasible_value = b.value.item<double>();
      state.output.best_feasible_constraint_violation = b.max_constraint_violation;
    }
  }
}

}  // namespace detail

inline OptimaState optima(const ConstrainedProblem& problem_in,
                          const ParameterPack<Series>& initial_params,
                          const OptimaOptions& options = {}) {
  // Infinite bounds mean "unbounded": clamp them to a huge finite value so every algorithm sees ordinary,
  // permanently inactive rows.
  ConstrainedProblem problem = problem_in;
  const auto clamp_pack = [](std::optional<ParameterPack<Series>>& pack) {
    if (!pack.has_value()) return;
    std::vector<ParameterPack<Series>::Block> blocks;
    for (const auto& b : pack->blocks()) blocks.push_back({b.name, b.value.clamp(-1e20, 1e20)});
    pack = ParameterPack<Series>(std::move(blocks));
  };
  clamp_pack(problem.bounds.lower);
  clamp_pack(problem.bounds.upper);
  detail::validate_problem_(problem, initial_params);
  // Automatic scaling (OptimaProblemScaling::ObjectiveAndConstraints): solve a rescaled copy of the problem, with
  // the algorithm's own scaling switched off, and map the answer back to the caller's units.
  const OptimaProblemScaling requested_scaling =
      options.algorithm == OptimaAlgorithm::SequentialQP ? options.sequential_qp.problem_scaling
      : options.algorithm == OptimaAlgorithm::WorkingSet ? options.working_set.problem_scaling
      : options.algorithm == OptimaAlgorithm::InternalProtocol ? options.internal_protocol.problem_scaling
                                                               : OptimaProblemScaling::None;
  if (requested_scaling == OptimaProblemScaling::ObjectiveAndConstraints) {
    const detail::ProblemScaling scaling = detail::compute_gradient_scaling_(problem, initial_params);
    OptimaOptions inner = options;
    inner.sequential_qp.problem_scaling = OptimaProblemScaling::None;
    inner.working_set.problem_scaling = OptimaProblemScaling::None;
    inner.internal_protocol.problem_scaling = OptimaProblemScaling::None;
    if (!scaling.active) return optima(problem, initial_params, inner);
    const auto user_hook = [&]() {
      if (options.algorithm == OptimaAlgorithm::SequentialQP && options.sequential_qp.progress_hook) return options.sequential_qp.progress_hook;
      if (options.algorithm == OptimaAlgorithm::WorkingSet && options.working_set.progress_hook) return options.working_set.progress_hook;
      if (options.algorithm == OptimaAlgorithm::InternalProtocol && options.internal_protocol.progress_hook) return options.internal_protocol.progress_hook;
      return options.progress_hook;
    }();
    if (user_hook) {
      // Progress snapshots describe the caller's problem: re-evaluate it at the snapshot's point.
      auto wrapped = [user_hook, original = problem](const OptimaProgressSnapshot& snapshot) {
        OptimaProgressSnapshot out = snapshot;
        const ProblemEvaluation e = evaluate_problem(original, snapshot.params);
        if (e.status == EvalStatus::Ok) out.evaluation = e;
        return user_hook(out);
      };
      inner.progress_hook = wrapped;
      inner.sequential_qp.progress_hook = wrapped;
      inner.working_set.progress_hook = wrapped;
      inner.internal_protocol.progress_hook = wrapped;
    }
    OptimaState state = optima(detail::apply_problem_scaling_(problem, scaling), initial_params, inner);
    detail::unscale_optima_state_(state, problem, scaling);
    return state;
  }
  // `converged` reports success, not mere termination: it is true only for the Converged exit flag.
  const auto finish = [](OptimaState state) {
    state.converged = state.exit_flag == OptimaExitFlag::Converged;
    return state;
  };
  switch (options.algorithm) {
    case OptimaAlgorithm::MeritMethod: {
      auto penalty_options = options.penalty;
      if (!penalty_options.progress_hook) penalty_options.progress_hook = options.progress_hook;
      return finish(to_optima_state(problem, solve_penalty_method(problem, initial_params, penalty_options)));
    }
    case OptimaAlgorithm::SequentialQP: {
      auto sequential_qp_options = options.sequential_qp;
      if (!sequential_qp_options.progress_hook) sequential_qp_options.progress_hook = options.progress_hook;
      return finish(to_optima_state(problem, solve_sequential_qp(problem, initial_params, sequential_qp_options)));
    }
    case OptimaAlgorithm::WorkingSet: {
      auto working_set_options = options.working_set;
      if (!working_set_options.progress_hook) working_set_options.progress_hook = options.progress_hook;
      return finish(to_optima_state(problem,
                                    solve_working_set(problem, initial_params, working_set_options),
                                    OptimaAlgorithm::WorkingSet));
    }
    case OptimaAlgorithm::InternalProtocol: {
      auto internal_protocol_options = options.internal_protocol;
      if (!internal_protocol_options.progress_hook) internal_protocol_options.progress_hook = options.progress_hook;
      return finish(to_optima_state(problem, solve_internal_protocol(problem, initial_params, internal_protocol_options)));
    }
  }

  ::pluss::throw_error("optim::optima: unknown algorithm");
}

inline OptimaState optima(const ConstrainedProblem& problem,
                          const Series& initial_params,
                          const OptimaOptions& options = {}) {
  return optima(problem, pack_series(initial_params), options);
}

// Objectives over plain Series: the callback receives the parameters as Series and returns a Series (a single
// number); the engine differentiates it through the Series tape.
inline OptimaState optima(
    std::function<Series(const Series&)> objective_series,
    const Series& initial_params,
    const OptimaOptions& options = {},
    const ObjectiveConstraints& constraints = {}) {
  ConstrainedProblem problem;
  problem.objective.autodiff = detail::trace_pack_callback_(std::move(objective_series));
  problem.nonlinear = constraints.nonlinear;
  problem.linear = constraints.linear;
  problem.bounds = constraints.bounds;
  return optima(problem, pack_series(initial_params), options);
}

inline OptimaState optima(
    std::function<Series(const ParameterPack<Series>&)> objective_series,
    const ParameterPack<Series>& initial_params,
    const OptimaOptions& options = {},
    const ObjectiveConstraints& constraints = {}) {
  ConstrainedProblem problem;
  problem.objective.autodiff = detail::trace_pack_callback_(std::move(objective_series));
  problem.nonlinear = constraints.nonlinear;
  problem.linear = constraints.linear;
  problem.bounds = constraints.bounds;
  return optima(problem, initial_params, options);
}

inline OptimaState optima(
    std::function<std::pair<Series, ParameterPack<Series>>(const ParameterPack<Series>&)> objective_analytic,
    const ParameterPack<Series>& initial_params,
    const OptimaOptions& options = {},
    const ObjectiveConstraints& constraints = {}) {
  ConstrainedProblem problem;
  problem.objective.analytic = std::move(objective_analytic);
  problem.nonlinear = constraints.nonlinear;
  problem.linear = constraints.linear;
  problem.bounds = constraints.bounds;
  return optima(problem, initial_params, options);
}

inline OptimaState optima(
    std::function<std::pair<Series, Series>(const Series&)> objective_analytic,
    const Series& initial_params,
    const OptimaOptions& options = {},
    const ObjectiveConstraints& constraints = {}) {
  ConstrainedProblem problem;
  problem.objective.analytic =
      [f = std::move(objective_analytic)](const ParameterPack<Series>& p) -> std::pair<Series, ParameterPack<Series>> {
    auto [value, grad] = f(p.at("x").reshape({p.at("x").numel()}));
    return {std::move(value), pack_series(grad)};
  };
  problem.nonlinear = constraints.nonlinear;
  problem.linear = constraints.linear;
  problem.bounds = constraints.bounds;
  return optima(problem, pack_series(initial_params), options);
}

namespace detail {

inline bool is_complex_(const Series& x) {
  return x.defined() && x.dtype().is_complex();
}

// Pluss's counterpart to torch's `torch.view_as_real`: reinterpret a
// complex Series as a real one with a NEW trailing dimension of size 2
// (index 0 = real part, index 1 = imaginary part), so `adam_step_inplace`'s
// existing (already real-dtype-correct) formula runs completely unchanged
// and applies independently to the real and imaginary channels -- torch's
// own approach for complex parameters (`_view_as_real` in
// torch/optim/optimizer.py, applied to param/grad/exp_avg/exp_avg_sq/
// max_exp_avg_sqs before the ordinary real update in torch/optim/adam.py):
// a complex gradient's second-moment estimate needs real(g)^2 and imag(g)^2
// tracked SEPARATELY -- not |g|^2, which mixes them, and not g^2 (what
// `adam_step_inplace` computed pre-fix), which is itself complex and does
// not match torch's actual numeric convention. Mirrors
// `pluss::ai::nn::optim::Optimizer::view_as_real_` in
// PlussAI/NodeK/NN/optim/optim_base.h exactly (same architecture,
// oracle-verified there against live torch numbers) -- see that file's own
// comment for the full derivation, including why no single intermediate
// fix (e.g. `.square()` -> `.abs2()`) can replicate this: complex DIVISION
// mixes real/imaginary components, so the whole formula must run twice.
//
// Real (non-complex) input passes through unchanged, so call sites don't
// need to branch. UNLIKE torch's `view_as_real` (a true zero-copy view),
// this is a COPY -- CodeM has no reinterpret-cast-dtype view primitive
// (same known, deliberately out-of-scope limitation noted in optim_base.h).
inline Series view_as_real_(const Series& x) {
  if (!is_complex_(x)) return x;
  const Series re = x.real();
  const Series im = x.imag();
  return re.pile(std::vector<Series>{im}, re.dim());
}

// Inverse of view_as_real_: given a real Series shaped [..., 2] (as
// produced by view_as_real_) and the ORIGINAL complex dtype, reconstruct
// the complex Series. `original_dtype` not being complex means the value
// was never converted in the first place -- returns it unchanged. An
// undefined input (e.g. an AMSGrad max_v buffer nobody has written to yet
// on a fresh, never-stepped state) passes through unchanged too, matching
// view_as_real_'s own symmetry.
inline Series view_as_complex_(const Series& real_view, ScalarType original_dtype) {
  if (!real_view.defined()) return real_view;
  if (!pluss::codem::dtype_of(original_dtype).is_complex()) return real_view;
  const std::int64_t last = real_view.dim() - 1;
  const Series re = real_view.select(last, 0);
  const Series im = real_view.select(last, 1);
  const Series i_unit =
      pluss::make_series(std::vector<std::complex<double>>{{0.0, 1.0}}, {})
          .to(original_dtype)
          .to(re.platform());
  return re.to(original_dtype).add(im.to(original_dtype).mul(i_unit));
}

} // namespace detail

struct AdamOptions {
  double learning_rate{1e-3};
  double beta1{0.9};
  double beta2{0.999};
  double eps{1e-8};
  double weight_decay{0.0};
  bool amsgrad{false};
  bool maximize{false};
  bool decoupled_weight_decay{false};
};

struct AdamState {
  std::vector<Series> m{};
  std::vector<Series> v{};
  std::vector<Series> max_v{};
  std::int64_t step{0};
};

inline AdamState adam_state_like(const std::vector<Series>& params) {
  AdamState st;
  st.m.reserve(params.size());
  st.v.reserve(params.size());
  st.max_v.reserve(params.size());
  for (const auto& p : params) {
    st.m.push_back(pluss::series::zeros_like(p, p.scalar_type()));
    st.v.push_back(pluss::series::zeros_like(p, p.scalar_type()));
    st.max_v.push_back(pluss::series::zeros_like(p, p.scalar_type()));
  }
  return st;
}

inline void adam_step_inplace(std::vector<Series>& params,
                              const std::vector<Series>& sensitivities,
                              AdamState& state,
                              const AdamOptions& opt = {}) {
  if (params.size() != sensitivities.size()) {
    ::pluss::throw_error("optim::adam_step_inplace: params/sensitivities size mismatch");
  }
  if (state.m.size() != params.size() || state.v.size() != params.size() || state.max_v.size() != params.size()) {
    state = adam_state_like(params);
  }

  ++state.step;
  const double b1_correction = 1.0 - std::pow(opt.beta1, static_cast<double>(state.step));
  const double b2_correction = 1.0 - std::pow(opt.beta2, static_cast<double>(state.step));

  for (std::size_t i = 0; i < params.size(); ++i) {
    detail::require_defined_(params[i], "adam_step_inplace");
    detail::require_defined_(sensitivities[i], "adam_step_inplace");
    detail::require_same_platform_(params[i], sensitivities[i], "adam_step_inplace");
    detail::require_same_platform_(params[i], state.m[i], "adam_step_inplace");
    detail::require_same_platform_(params[i], state.v[i], "adam_step_inplace");
    detail::require_same_platform_(params[i], state.max_v[i], "adam_step_inplace");
    detail::require_float_dtype_(params[i], "adam_step_inplace");

    // Keep optimizer updates outside autodiff recording.
    Series p = params[i].detach();
    Series g = sensitivities[i];
    if (!detail::same_shape_(p, g)) {
      if (p.numel() == 1 && g.numel() == 1) {
        g = g.sum();
      } else {
        ::pluss::throw_error("optim::adam_step_inplace: sensitivity shape mismatch");
      }
    }

    if (opt.maximize) {
      g = g.mul(-1.0);
    }

    if (opt.weight_decay != 0.0) {
      if (opt.decoupled_weight_decay) {
        p = p.mul(1.0 - opt.learning_rate * opt.weight_decay);
      } else {
        g = g.add(p.mul(opt.weight_decay));
      }
    }

    // Complex parameters: view as real with a new trailing [.., 2] dim so
    // the real-valued formula below (unchanged) applies independently to
    // the real and imaginary channels -- see detail::view_as_real_ above
    // for the full rationale (mirrors PlussAI/NodeK/NN/optim/adam.h's
    // apply_group_loop_ exactly, oracle-verified there against live torch).
    // No-op for real (non-complex) parameters. Order matches that file too:
    // maximize/weight_decay applied in complex space FIRST (scaling by a
    // real scalar is unambiguous either way), only THEN view-as-real, since
    // that's specifically where real/imaginary independence matters.
    const ScalarType original_dtype = p.scalar_type();
    p = detail::view_as_real_(p);
    g = detail::view_as_real_(g);
    state.m[i] = detail::view_as_real_(state.m[i]);
    state.v[i] = detail::view_as_real_(state.v[i]);
    state.max_v[i] = detail::view_as_real_(state.max_v[i]);

    state.m[i] = state.m[i].mul(opt.beta1).add(g.mul(1.0 - opt.beta1));
    state.v[i] = state.v[i].mul(opt.beta2).add(g.mul(g).mul(1.0 - opt.beta2));

    const Series mhat = state.m[i].div(b1_correction);
    Series vhat = state.v[i].div(b2_correction);
    if (opt.amsgrad) {
      // Post view-as-real, this is an ordinary elementwise real max over
      // the real/imaginary channels independently -- the same "no total
      // order on ℂ" concern that (correctly) blocks Minimax's box
      // constraints from running on a complex PARAMETER directly does not
      // apply here, since by this point `state.max_v[i]`/`state.v[i]` are
      // both already real-valued views, not complex values.
      state.max_v[i] = state.max_v[i].maximum(state.v[i]);
      vhat = state.max_v[i].div(b2_correction);
    }
    const Series update = mhat.div(vhat.sqrt().add(opt.eps)).mul(opt.learning_rate);
    const Series next_value = p.sub(update);

    state.m[i] = detail::view_as_complex_(state.m[i], original_dtype);
    state.v[i] = detail::view_as_complex_(state.v[i], original_dtype);
    state.max_v[i] = detail::view_as_complex_(state.max_v[i], original_dtype);
    params[i] = detail::view_as_complex_(next_value, original_dtype);
  }
}

inline std::vector<Series> adam_step(std::vector<Series> params,
                                     const std::vector<Series>& sensitivities,
                                     AdamState& state,
                                     const AdamOptions& opt = {}) {
  adam_step_inplace(params, sensitivities, state, opt);
  return params;
}

using SeptimaObjectiveTraceFn = std::function<detail::Trace(const std::vector<detail::Trace>&)>;
using SeptimaObjectiveSeriesFn = std::function<Series(const std::vector<Series>&)>;

struct SeptimaOptions {
  enum class Optimizer : std::uint8_t {
    Adam = 0,
    Lbfgs = 1,
  };

  enum class LbfgsLineSearch : std::uint8_t {
    None = 0,
    StrongWolfe = 1,
  };

  struct LbfgsOptions {
    double learning_rate{1.0};
    std::int64_t history_size{20};
    double curvature_tolerance{1e-10};
    std::int64_t max_line_search_iterations{25};
    double tolerance_change{1e-9};
    double wolfe_c1{1e-4};
    // Kept for API/ABI compatibility; no longer read by the StrongWolfe line search itself
    // (Armijo alone is now the acceptance criterion -- see the long comment at that call site
    // for why backtracking cannot reliably enforce the strong-Wolfe curvature condition, and why
    // it isn't needed here: update_lbfgs_history_ already applies its own independent curvature
    // skip, lbfgs_curvature_ok_, before adding any pair to the LBFGS history).
    double wolfe_c2{0.9};
    LbfgsLineSearch line_search{LbfgsLineSearch::None};
  };

  Optimizer optimizer{Optimizer::Adam};
  AdamOptions adam{};
  LbfgsOptions lbfgs{};
  // Step of the multiplier update, lambda += rate * residual. 0 (the default) means "the penalty weight", the step of
  // the classical method of multipliers, which converges when the augmented problem is solved accurately between
  // updates; a positive value overrides it.
  double dual_learning_rate{0.0};
  double equality_penalty{10.0};
  double inequality_penalty{10.0};
  // Multiplier schedule. The multipliers change the function being minimized, so they are updated only once the
  // augmented problem has been solved well enough: when its stationarity is at most `dual_update_ratio` times the
  // current constraint violation (never asking for less than `dual_update_floor`), or after `dual_update_max_steps`
  // steps, whichever comes first.
  double dual_update_ratio{0.1};
  double dual_update_floor{1e-10};
  std::int64_t dual_update_max_steps{50};
  // Penalty schedule. When a multiplier update leaves the violation larger than `penalty_growth_trigger` times what it was
  // at the previous update, the penalty weights are multiplied by `penalty_growth`, up to `max_penalty`. A penalty that is
  // too small for the problem (a nonlinear constraint can need a large one before the augmented problem is well behaved)
  // is thereby raised by the run itself. `penalty_growth = 1` keeps the penalties fixed.
  double penalty_growth{10.0};
  double penalty_growth_trigger{0.25};
  double max_penalty{1e8};
  bool project_bounds{true};
  bool enable_constraint_autodiff{true};
  std::int64_t nan_recovery_attempts{2};
  double nan_recovery_shrink{0.5};
};

using SeptimaOptimizer = SeptimaOptions::Optimizer;
using SeptimaLbfgsLineSearch = SeptimaOptions::LbfgsLineSearch;
using SeptimaLbfgsOptions = SeptimaOptions::LbfgsOptions;

struct SeptimaConstraints {
  using ResidualFn = std::function<std::vector<Series>(const std::vector<Series>&)>;

  ResidualFn equalities{};
  ResidualFn inequalities{};
  std::vector<Series> lbs{};
  std::vector<Series> ubs{};

  bool empty() const noexcept {
    return !equalities && !inequalities && lbs.empty() && ubs.empty();
  }
};

struct SeptimaState {
  AdamState adam{};
  std::vector<Series> lbfgs_s_history{};
  std::vector<Series> lbfgs_y_history{};
  Series lbfgs_prev_grad_flat = pluss::series::undefined();
  Series lbfgs_prev_step_flat = pluss::series::undefined();
  std::vector<Series> lambda_equalities{};
  std::vector<Series> lambda_inequalities{};
  double constraint_violation{0.0};
  // Largest component of the gradient of the augmented Lagrangian at the point the last step started from, with the
  // components that an active bound blocks removed. On a problem without constraints it equals the objective gradient.
  double stationarity{std::numeric_limits<double>::infinity()};
  std::int64_t steps_since_dual_update{0};
  std::int64_t dual_updates{0};
  double penalty_scale{1.0};  // the penalty weights in force are the options' weights times this
  double last_update_violation{std::numeric_limits<double>::infinity()};
  std::int64_t step{0};
};

enum class SeptimaExitFlag : std::int32_t {
  Converged = 1,
  MaxIterations = 0,
  Stopped = -1,
  NoProgress = -2,
  NonFiniteEvaluation = -3,
  InvalidProblem = -4,
};

inline std::string_view septima_exit_flag_name(SeptimaExitFlag value) {
  switch (value) {
    case SeptimaExitFlag::Converged:
      return "Converged";
    case SeptimaExitFlag::MaxIterations:
      return "MaxIterations";
    case SeptimaExitFlag::Stopped:
      return "Stopped";
    case SeptimaExitFlag::NoProgress:
      return "NoProgress";
    case SeptimaExitFlag::NonFiniteEvaluation:
      return "NonFiniteEvaluation";
    case SeptimaExitFlag::InvalidProblem:
      return "InvalidProblem";
  }
  return "Unknown";
}

struct SeptimaProgressSnapshot {
  std::int64_t iteration{0};
  std::int64_t function_evaluations{0};
  double objective{std::numeric_limits<double>::infinity()};
  double gradient_inf_norm{std::numeric_limits<double>::infinity()};
  double stationarity{std::numeric_limits<double>::infinity()};
  double step_inf_norm{0.0};
  double constraint_violation{0.0};
  std::string message{};
};

struct SeptimaSolveOptions {
  SeptimaOptions step{};
  std::int64_t max_iterations{200};
  std::int64_t max_no_progress_iterations{25};
  double objective_tolerance{1e-12};
  double gradient_tolerance{1e-6};
  double step_tolerance{1e-10};
  double constraint_tolerance{1e-6};
  bool stop_on_nonfinite_objective{true};
  std::function<bool(const SeptimaProgressSnapshot&)> progress_hook{};
};

namespace reports {

inline bool IterReport::operator()(const SeptimaProgressSnapshot& snapshot) const {
  if (selected_(snapshot.iteration)) {
    const auto flags = os_->flags();
    const auto old_precision = os_->precision();
    *os_ << "iter " << snapshot.iteration << "  objective " << std::scientific << std::setprecision(precision_) << snapshot.objective << "  gradient "
         << snapshot.gradient_inf_norm << "  step " << snapshot.step_inf_norm << '\n';
    os_->flags(flags);
    os_->precision(old_precision);
  }
  return true;
}

}  // namespace reports

struct SeptimaResult {
  std::vector<Series> params{};
  SeptimaState state{};
  Series objective = pluss::series::undefined();
  std::int64_t iteration{0};
  std::int64_t function_evaluations{0};
  double objective_value{std::numeric_limits<double>::infinity()};
  double gradient_inf_norm{std::numeric_limits<double>::infinity()};  // objective gradient alone
  double stationarity{std::numeric_limits<double>::infinity()};       // augmented-Lagrangian gradient, bounds projected
  double step_inf_norm{0.0};
  double constraint_violation{std::numeric_limits<double>::infinity()};
  bool converged{false};
  SeptimaExitFlag exit_flag{SeptimaExitFlag::MaxIterations};
  std::string message{};
};

namespace detail {

inline bool septima_finite_(const Series& x) {
  // CodeM's `isfinite()` predicate has no complex-dtype implementation
  // (throws "predicate unary: dtype not implemented") -- go through
  // view_as_real_ first so a complex candidate is checked component-wise
  // (both the real and imaginary channels must be finite), matching
  // PlussAI's Minimax::finite_series_ fix for the identical gap. A no-op
  // for non-complex input.
  return view_as_real_(x).isfinite().all().item<bool>();
}

inline bool septima_finite_(const std::vector<Series>& xs) {
  return std::all_of(xs.begin(), xs.end(), [](const Series& x) { return septima_finite_(x); });
}

inline double septima_max_abs_(const Series& x) {
  if (!x.defined() || x.numel() == 0) return 0.0;
  return x.abs().max().item<double>();
}

inline void septima_validate_options_(const SeptimaOptions& options) {
  if (options.optimizer == SeptimaOptions::Optimizer::Adam) {
    if (!(options.adam.learning_rate > 0.0) || !std::isfinite(options.adam.learning_rate)) {
      ::pluss::throw_error("optim::septima_step_inplace: adam.learning_rate must be positive and finite");
    }
    if (!(options.adam.beta1 >= 0.0 && options.adam.beta1 < 1.0) || !std::isfinite(options.adam.beta1)) {
      ::pluss::throw_error("optim::septima_step_inplace: adam.beta1 must be in [0,1)");
    }
    if (!(options.adam.beta2 >= 0.0 && options.adam.beta2 < 1.0) || !std::isfinite(options.adam.beta2)) {
      ::pluss::throw_error("optim::septima_step_inplace: adam.beta2 must be in [0,1)");
    }
    if (!(options.adam.eps > 0.0) || !std::isfinite(options.adam.eps)) {
      ::pluss::throw_error("optim::septima_step_inplace: adam.eps must be positive and finite");
    }
    if (options.adam.weight_decay < 0.0 || !std::isfinite(options.adam.weight_decay)) {
      ::pluss::throw_error("optim::septima_step_inplace: adam.weight_decay must be finite and >= 0");
    }
  } else if (options.optimizer == SeptimaOptions::Optimizer::Lbfgs) {
    if (!(options.lbfgs.learning_rate > 0.0) || !std::isfinite(options.lbfgs.learning_rate)) {
      ::pluss::throw_error("optim::septima_step_inplace: lbfgs.learning_rate must be positive and finite");
    }
    if (options.lbfgs.history_size < 0) {
      ::pluss::throw_error("optim::septima_step_inplace: lbfgs.history_size must be >= 0");
    }
    if (!(options.lbfgs.curvature_tolerance >= 0.0) || !std::isfinite(options.lbfgs.curvature_tolerance)) {
      ::pluss::throw_error(
          "optim::septima_step_inplace: lbfgs.curvature_tolerance must be finite and >= 0");
    }
    if (options.lbfgs.max_line_search_iterations < 0) {
      ::pluss::throw_error("optim::septima_step_inplace: lbfgs.max_line_search_iterations must be >= 0");
    }
    if (!(options.lbfgs.tolerance_change >= 0.0) || !std::isfinite(options.lbfgs.tolerance_change)) {
      ::pluss::throw_error(
          "optim::septima_step_inplace: lbfgs.tolerance_change must be finite and >= 0");
    }
    if (!(options.lbfgs.wolfe_c1 > 0.0 && options.lbfgs.wolfe_c1 < 1.0) || !std::isfinite(options.lbfgs.wolfe_c1)) {
      ::pluss::throw_error("optim::septima_step_inplace: lbfgs.wolfe_c1 must be in (0,1)");
    }
    if (!(options.lbfgs.wolfe_c2 > options.lbfgs.wolfe_c1 && options.lbfgs.wolfe_c2 < 1.0) ||
        !std::isfinite(options.lbfgs.wolfe_c2)) {
      ::pluss::throw_error("optim::septima_step_inplace: lbfgs.wolfe_c2 must be in (wolfe_c1,1)");
    }
  } else {
    ::pluss::throw_error("optim::septima_step_inplace: unsupported septima optimizer");
  }
  if (options.dual_learning_rate < 0.0 || !std::isfinite(options.dual_learning_rate)) {
    ::pluss::throw_error("optim::septima_step_inplace: dual_learning_rate must be finite and >= 0");
  }
  if (options.equality_penalty < 0.0 || !std::isfinite(options.equality_penalty)) {
    ::pluss::throw_error("optim::septima_step_inplace: equality_penalty must be finite and >= 0");
  }
  if (options.inequality_penalty < 0.0 || !std::isfinite(options.inequality_penalty)) {
    ::pluss::throw_error("optim::septima_step_inplace: inequality_penalty must be finite and >= 0");
  }
  if (!(options.penalty_growth >= 1.0) || !std::isfinite(options.penalty_growth)) {
    ::pluss::throw_error("optim::septima_step_inplace: penalty_growth must be finite and >= 1");
  }
  if (!(options.penalty_growth_trigger > 0.0) || !std::isfinite(options.penalty_growth_trigger)) {
    ::pluss::throw_error("optim::septima_step_inplace: penalty_growth_trigger must be positive and finite");
  }
  if (!(options.max_penalty > 0.0)) {
    ::pluss::throw_error("optim::septima_step_inplace: max_penalty must be positive");
  }
  if (!(options.dual_update_ratio > 0.0) || !std::isfinite(options.dual_update_ratio)) {
    ::pluss::throw_error("optim::septima_step_inplace: dual_update_ratio must be positive and finite");
  }
  if (options.dual_update_floor < 0.0 || !std::isfinite(options.dual_update_floor)) {
    ::pluss::throw_error("optim::septima_step_inplace: dual_update_floor must be finite and >= 0");
  }
  if (options.dual_update_max_steps < 1) {
    ::pluss::throw_error("optim::septima_step_inplace: dual_update_max_steps must be >= 1");
  }
  if (options.nan_recovery_attempts < 0) {
    ::pluss::throw_error("optim::septima_step_inplace: nan_recovery_attempts must be >= 0");
  }
  if (!(options.nan_recovery_shrink > 0.0 && options.nan_recovery_shrink < 1.0) ||
      !std::isfinite(options.nan_recovery_shrink)) {
    ::pluss::throw_error("optim::septima_step_inplace: nan_recovery_shrink must be in (0,1)");
  }
}

inline void septima_validate_solve_options_(const SeptimaSolveOptions& options) {
  if (options.max_iterations <= 0) {
    ::pluss::throw_error("optim::septima: max_iterations must be > 0");
  }
  if (options.max_no_progress_iterations < 0) {
    ::pluss::throw_error("optim::septima: max_no_progress_iterations must be >= 0");
  }
  if (!(options.objective_tolerance >= 0.0) || !std::isfinite(options.objective_tolerance)) {
    ::pluss::throw_error("optim::septima: objective_tolerance must be finite and >= 0");
  }
  if (!(options.gradient_tolerance >= 0.0) || !std::isfinite(options.gradient_tolerance)) {
    ::pluss::throw_error("optim::septima: gradient_tolerance must be finite and >= 0");
  }
  if (!(options.step_tolerance >= 0.0) || !std::isfinite(options.step_tolerance)) {
    ::pluss::throw_error("optim::septima: step_tolerance must be finite and >= 0");
  }
  if (!(options.constraint_tolerance >= 0.0) || !std::isfinite(options.constraint_tolerance)) {
    ::pluss::throw_error("optim::septima: constraint_tolerance must be finite and >= 0");
  }
}

inline void septima_validate_params_(const std::vector<Series>& params) {
  if (params.empty()) {
    ::pluss::throw_error("optim::septima_step_inplace: expected non-empty params");
  }
  for (const auto& p : params) {
    require_defined_(p, "septima_step_inplace");
    require_float_dtype_(p, "septima_step_inplace");
  }
}

// Forward declaration: septima_validate_constraints_ below needs to
// evaluate residuals to check their dtype (an inequality/equality
// constraint's residual can require realness even when the PARAMETER
// itself is complex -- see the checks below), but the definition lives
// further down this file, alongside the rest of the residual-evaluation
// helpers.
inline std::vector<Series> septima_evaluate_residuals_(
    const std::vector<Series>& params,
    const SeptimaConstraints::ResidualFn& fn,
    const char* name);

inline void septima_validate_constraints_(const std::vector<Series>& params,
                                          const SeptimaConstraints& constraints,
                                          bool require_trace_callbacks) {
  if (!constraints.lbs.empty() && constraints.lbs.size() != params.size()) {
    ::pluss::throw_error("optim::septima_step_inplace: lbs size mismatch");
  }
  if (!constraints.ubs.empty() && constraints.ubs.size() != params.size()) {
    ::pluss::throw_error("optim::septima_step_inplace: ubs size mismatch");
  }

  for (std::size_t i = 1; i < params.size(); ++i) {
    if (params[i].platform() != params[0].platform()) {
      ::pluss::throw_error(
          "optim::septima_step_inplace: constrained updates require params on one platform");
    }
  }


  auto check_bound = [&](const std::vector<Series>& bounds, const char* name) {
    for (std::size_t i = 0; i < bounds.size(); ++i) {
      if (!bounds[i].defined()) continue;
      require_same_platform_(params[i], bounds[i], "septima_step_inplace");
      if (!same_shape_(params[i], bounds[i])) {
        if (!(params[i].numel() == 1 && bounds[i].numel() == 1)) {
          ::pluss::throw_error(std::string("optim::septima_step_inplace: ") + name +
                                   " shape mismatch");
        }
      }
      // Box constraints project the PARAMETER's own value directly via
      // .maximum()/.minimum() (septima_project_bounds_) -- no total order
      // over complex numbers exists, so a complex parameter cannot carry a
      // box constraint. Mirrors PlussAI::Minimax's identical
      // validate_bounds_vector_ guard, and for the same reason.
      if (is_complex_(params[i])) {
        ::pluss::throw_error(std::string("optim::septima_step_inplace: ") + name +
                                 " box constraints are not supported for a complex parameter (no "
                                 "defined ordering convention) -- omit the bound for this parameter, "
                                 "or clip a real projection of it yourself before the step");
      }
    }
  };
  check_bound(constraints.lbs, "lbs");
  check_bound(constraints.ubs, "ubs");

  // Inequality residuals feed the dual-ascent multiplier update's
  // `.maximum(0.0)` (septima_step_inplace_'s lambda_inequalities update)
  // UNCONDITIONALLY, regardless of enable_constraint_autodiff -- an
  // ordering operation with no meaning over complex numbers. Checked on
  // the RESIDUAL's dtype, not the parameter's: a complex parameter with a
  // real-valued inequality residual (e.g. h(x) = |x|^2 - 1 <= 0) is
  // perfectly well-posed. Mirrors PlussAI::Minimax's identical, precise
  // inequality-residual guard.
  if (constraints.inequalities) {
    const auto residuals = septima_evaluate_residuals_(params, constraints.inequalities, "inequalities");
    for (const auto& r : residuals) {
      if (is_complex_(r)) {
        ::pluss::throw_error(
            "optim::septima_step_inplace: inequality residuals must be real-valued (the "
            "dual multiplier's non-negativity projection has no meaning over complex numbers); return "
            "a real residual from the inequalities callback even if the parameters themselves are "
            "complex");
      }
    }
  }

  // Equality residuals only need realness when the constraint-augmented
  // objective Trace runs (require_trace_callbacks, i.e.
  // options.enable_constraint_autodiff): its quadratic penalty term
  // `square(residual)` (septima_constraint_objective_trace_) needs a REAL
  // reduction to stay a well-defined scalar loss -- the combined objective
  // is reduced via `.item<double>()` throughout septima()'s main loop and
  // the LBFGS line search's Armijo check, which would fail to extract a
  // double from a complex value. The plain dual-ascent multiplier update
  // itself (`lambda_equalities[i].add(residual.mul(dual_lr))`) is pure
  // add/mul with no ordering, so with enable_constraint_autodiff == false
  // a complex equality residual is fine -- same as PlussAI::Minimax's
  // equality constraints, which have no autodiff-objective path at all.
  if (require_trace_callbacks && constraints.equalities) {
    const auto residuals = septima_evaluate_residuals_(params, constraints.equalities, "equalities");
    for (const auto& r : residuals) {
      if (is_complex_(r)) {
        ::pluss::throw_error(
            "optim::septima_step_inplace: equality residuals must be real-valued when "
            "enable_constraint_autodiff is true (the augmented-Lagrangian penalty term squares the "
            "residual to build a real scalar loss); return a real residual, or set "
            "enable_constraint_autodiff = false to use only the dual-ascent multiplier update, which "
            "has no such restriction");
      }
    }
  }
}

inline std::vector<Series> septima_evaluate_residuals_(
    const std::vector<Series>& params,
    const SeptimaConstraints::ResidualFn& fn,
    const char* name) {
  if (!fn) return {};
  const auto out = fn(params);
  for (const auto& r : out) {
    require_defined_(r, "septima_step_inplace");
    require_same_platform_(params.front(), r, "septima_step_inplace");
    require_float_dtype_(r, "septima_step_inplace");
  }
  if (out.empty()) {
    ::pluss::throw_error(std::string("optim::septima_step_inplace: empty ") + name +
                             " residual vector");
  }
  return out;
}

inline std::vector<detail::Trace> septima_evaluate_trace_residuals_(
    const std::vector<detail::Trace>& vars,
    const SeptimaConstraints::ResidualFn& series_fn,
    const char* name) {
  if (series_fn) {
    std::vector<Series> values;
    values.reserve(vars.size());
    for (const auto& v : vars) values.push_back(v.value());
    const auto residual_values = septima_evaluate_residuals_(values, series_fn, name);
    std::vector<detail::Trace> out;
    out.reserve(residual_values.size());
    for (const auto& r : residual_values) out.emplace_back(r);
    return out;
  }

  return {};
}

inline std::vector<Series> septima_zero_like_(const std::vector<Series>& residuals) {
  std::vector<Series> lambda;
  lambda.reserve(residuals.size());
  for (const auto& r : residuals) {
    lambda.push_back(pluss::series::zeros_like(r, r.scalar_type()));
  }
  return lambda;
}

inline void septima_align_multipliers_(std::vector<Series>& lambda, const std::vector<Series>& residuals) {
  if (lambda.size() != residuals.size()) {
    lambda = septima_zero_like_(residuals);
    return;
  }
  for (std::size_t i = 0; i < residuals.size(); ++i) {
    if (!same_shape_(lambda[i], residuals[i]) ||
        lambda[i].platform() != residuals[i].platform() ||
        lambda[i].scalar_type() != residuals[i].scalar_type()) {
      lambda[i] = pluss::series::zeros_like(residuals[i], residuals[i].scalar_type());
    }
  }
}

struct SeptimaVectorBlockLayout {
  std::vector<std::int64_t> sizes{};
  std::int64_t numel{0};
  ScalarType scalar_type{ScalarType::Float32};
  pluss::codem::Platform platform{pluss::codem::Platform::CPU};
};

inline std::vector<SeptimaVectorBlockLayout> septima_make_layout_(const std::vector<Series>& values) {
  std::vector<SeptimaVectorBlockLayout> layout;
  layout.reserve(values.size());
  for (const auto& value : values) {
    detail::require_defined_(value, "septima_make_layout_");
    const auto sizes = value.sizes();
    layout.push_back(SeptimaVectorBlockLayout{
        std::vector<std::int64_t>(sizes.begin(), sizes.end()),
        value.numel(),
        value.scalar_type(),
        value.platform()});
  }
  return layout;
}

inline Series septima_flatten_values_(const std::vector<Series>& values) {
  if (values.empty()) {
    ::pluss::throw_error("optim::septima_flatten_values_: expected non-empty values");
  }
  std::vector<Series> flat;
  flat.reserve(values.size());
  for (const auto& value : values) {
    detail::require_defined_(value, "septima_flatten_values_");
    // Complex blocks: view as real first (a new trailing [.., 2] dim --
    // real and imaginary channels appended, see detail::view_as_real_
    // above) so every downstream LBFGS primitive (series_dot_,
    // lbfgs_direction_, update_lbfgs_history_, and the Armijo/curvature/
    // Wolfe checks in the line search below) operates on an ordinary real
    // Float64 vector without any of them needing complex-aware logic of
    // their own -- exactly the standard treatment of optimizing a
    // real-valued loss over a complex variable (view the problem as a
    // 2n-dimensional real optimization; matches the same architecture
    // adam_step_inplace uses, just applied once at the flatten boundary
    // instead of per moment-buffer, since LBFGS already flattens
    // everything into one vector regardless). A no-op for a real
    // (non-complex) block, so this function's real-dtype behavior is
    // unchanged.
    const Series real_view = view_as_real_(value);
    flat.push_back(real_view.reshape({real_view.numel()}));
  }
  if (flat.size() == 1) return flat.front().to(ScalarType::Float64);
  return flat.front().unite(std::vector<Series>(flat.begin() + 1, flat.end()), 0).to(ScalarType::Float64);
}

inline std::vector<Series> septima_unflatten_values_(const Series& flat_values,
                                                     const std::vector<SeptimaVectorBlockLayout>& layout) {
  detail::require_defined_(flat_values, "septima_unflatten_values_");
  Series flat = flat_values.reshape({flat_values.numel()});
  // A complex block contributes 2x its own (complex) numel to the
  // flattened real vector -- see septima_flatten_values_'s view_as_real_
  // above -- so slicing must count real elements, not complex ones.
  auto real_element_count = [](const SeptimaVectorBlockLayout& block) -> std::int64_t {
    return pluss::codem::dtype_of(block.scalar_type).is_complex() ? block.numel * 2 : block.numel;
  };
  std::int64_t total_real_numel = 0;
  for (const auto& block : layout) total_real_numel += real_element_count(block);
  if (flat.numel() != total_real_numel) {
    ::pluss::throw_error("optim::septima_unflatten_values_: flat size mismatch");
  }

  std::vector<Series> out;
  out.reserve(layout.size());
  std::int64_t offset = 0;
  for (const auto& block : layout) {
    const std::int64_t count = real_element_count(block);
    Series piece = flat.slice(0, offset, offset + count, 1);
    if (!piece.is_contiguous()) piece = piece.contiguous();
    if (pluss::codem::dtype_of(block.scalar_type).is_complex()) {
      // Reshape the sliced real elements back into the [..,2] view_as_real_
      // shape (original sizes plus a trailing dim of 2), then reconstruct
      // the complex value -- the exact inverse of what
      // septima_flatten_values_ did to this block going in.
      std::vector<std::int64_t> real_view_shape = block.sizes;
      real_view_shape.push_back(2);
      piece = piece.reshape(real_view_shape);
      piece = view_as_complex_(piece, block.scalar_type);
    } else {
      piece = block.sizes.empty() ? piece.reshape({}) : piece.reshape(block.sizes);
    }
    if (piece.scalar_type() != block.scalar_type) piece = piece.to(block.scalar_type);
    if (piece.platform() != block.platform) piece = piece.to(block.platform);
    // `piece` is a slice/reshape view chained off `flat_values` (itself
    // typically a fresh arithmetic result, e.g. flat_params + direction *
    // step_size in the line search below) -- it still carries that
    // computation's own source link. Every caller of this function goes on
    // to treat each returned piece as an INDEPENDENT leaf input to a brand
    // new value_and_grad/Trace evaluation of the objective; without
    // severing that residual link first, the autodiff engine has to
    // reconcile "this is a fresh tracked leaf for this evaluation" against
    // "this value's own graph says it's derived from something else",
    // which resulted in every such evaluation coming back with an
    // undefined/disconnected gradient for that block (confirmed by
    // instrumenting the strong-Wolfe line search directly: every trial
    // step, from the initial step size down through 24 halvings, reported
    // grads_defined=0 while the objective VALUE was perfectly finite).
    // Minimax's own assign_parameter_ established the same fix for the
    // same reason ("Optimizer outputs become fresh leaf parameters for the
    // next iteration").
    pluss::codem::series_tape::clear_source_link(piece);
    out.push_back(std::move(piece));
    offset += count;
  }
  return out;
}

inline void septima_project_bounds_(std::vector<Series>& params, const SeptimaConstraints& constraints) {
  for (std::size_t i = 0; i < params.size(); ++i) {
    if (!constraints.lbs.empty() && constraints.lbs[i].defined()) {
      params[i] = params[i].maximum(constraints.lbs[i]);
    }
    if (!constraints.ubs.empty() && constraints.ubs[i].defined()) {
      params[i] = params[i].minimum(constraints.ubs[i]);
    }
  }
}

inline double septima_constraint_violation_(const std::vector<Series>& params,
                                            const SeptimaConstraints& constraints) {
  double violation = 0.0;
  if (constraints.equalities) {
    const auto residuals = septima_evaluate_residuals_(params, constraints.equalities, "equalities");
    for (const auto& r : residuals) violation = std::max(violation, septima_max_abs_(r));
  }
  if (constraints.inequalities) {
    const auto residuals = septima_evaluate_residuals_(params, constraints.inequalities, "inequalities");
    for (const auto& r : residuals) {
      violation = std::max(violation, septima_max_abs_(r.maximum(0.0)));
    }
  }
  if (!constraints.lbs.empty()) {
    for (std::size_t i = 0; i < params.size(); ++i) {
      if (!constraints.lbs[i].defined()) continue;
      violation = std::max(violation, septima_max_abs_((constraints.lbs[i] - params[i]).maximum(0.0)));
    }
  }
  if (!constraints.ubs.empty()) {
    for (std::size_t i = 0; i < params.size(); ++i) {
      if (!constraints.ubs[i].defined()) continue;
      violation = std::max(violation, septima_max_abs_((params[i] - constraints.ubs[i]).maximum(0.0)));
    }
  }
  return violation;
}

inline detail::Trace septima_constraint_objective_trace_(const std::vector<detail::Trace>& vars,
                                                 const SeptimaState& state,
                                                 const SeptimaConstraints& constraints,
                                                 const SeptimaOptions& options) {
  detail::Trace objective = sum(vars.front() * 0.0);

  if (constraints.equalities) {
    const auto residuals = septima_evaluate_trace_residuals_(
        vars, constraints.equalities, "equalities");
    if (residuals.size() != state.lambda_equalities.size()) {
      ::pluss::throw_error("optim::septima_step_inplace: equality multiplier size mismatch");
    }
    for (std::size_t i = 0; i < residuals.size(); ++i) {
      objective = objective +
                  sum(detail::fixed(state.lambda_equalities[i]) * residuals[i]) +
                  0.5 * options.equality_penalty * state.penalty_scale * sum(square(residuals[i]));
    }
  }

  if (constraints.inequalities) {
    const auto residuals = septima_evaluate_trace_residuals_(
        vars, constraints.inequalities, "inequalities");
    if (residuals.size() != state.lambda_inequalities.size()) {
      ::pluss::throw_error("optim::septima_step_inplace: inequality multiplier size mismatch");
    }
    for (std::size_t i = 0; i < residuals.size(); ++i) {
      const double inequality_penalty = options.inequality_penalty * state.penalty_scale;
      if (inequality_penalty > 0.0) {
        // Augmented-Lagrangian term for h <= 0:  (max(0, lambda + rho h)^2 - lambda^2) / (2 rho). Its gradient,
        // max(0, lambda + rho h) grad h, shrinks as the constraint becomes slack, so a multiplier that has grown
        // too large is pushed back down by the update instead of pinning the iterate inside the feasible set.
        const detail::Trace shifted = relu(detail::fixed(state.lambda_inequalities[i]) + inequality_penalty * residuals[i]);
        objective = objective +
                    (sum(square(shifted)) - sum(detail::fixed(state.lambda_inequalities[i] * state.lambda_inequalities[i]))) *
                        (0.5 / inequality_penalty);
      } else {
        objective = objective + sum(detail::fixed(state.lambda_inequalities[i]) * relu(residuals[i]));
      }
    }
  }

  return objective;
}

// Evaluates a Trace-based objective's VALUE only, skipping the reverse
// (backward) pass entirely. Building `vars`/calling `fn` still goes through
// the ordinary Trace forward machinery, but no sensitivity is ever
// requested or computed. Used where only the scalar objective is needed --
// e.g. the LBFGS line search's base-point evaluation below -- so callers
// don't pay for a full backward pass just to discard the gradient.
template <class Fn>
inline Series septima_value_only_(Fn&& fn, const std::vector<Series>& params) {
  std::vector<detail::Trace> vars;
  vars.reserve(params.size());
  for (const auto& p : params) {
    vars.emplace_back(p);
  }
  detail::Trace loss = std::forward<Fn>(fn)(vars);
  if (!loss.defined()) {
    ::pluss::throw_error("optim::septima_step_inplace: objective returned undefined detail::Trace");
  }
  Series value = loss.value();
  if (value.numel() == 1 && value.dim() != 0) value = value.reshape(std::vector<std::int64_t>{});
  return value;
}

// Constraint-augmented counterpart of septima_augmented_value_grad_ that
// returns only the (possibly constraint-augmented) scalar value -- see
// septima_value_only_ above for why this avoids a wasted backward pass.
inline Series septima_augmented_value_only_(const SeptimaObjectiveTraceFn& objective_trace,
                                            const std::vector<Series>& params,
                                            const SeptimaState& state,
                                            const SeptimaOptions& options,
                                            const SeptimaConstraints& constraints) {
  if (!objective_trace) {
    ::pluss::throw_error("optim::septima_step_inplace: objective callback is required for line search");
  }
  if (options.enable_constraint_autodiff && (constraints.equalities || constraints.inequalities)) {
    return septima_value_only_(
        [&](const std::vector<detail::Trace>& vars) -> detail::Trace {
          return objective_trace(vars) + septima_constraint_objective_trace_(vars, state, constraints, options);
        },
        params);
  }
  return septima_value_only_(objective_trace, params);
}

inline std::pair<Series, std::vector<Series>> septima_augmented_value_grad_(
    const SeptimaObjectiveTraceFn& objective_trace,
    const std::vector<Series>& params,
    const SeptimaState& state,
    const SeptimaOptions& options,
    const SeptimaConstraints& constraints) {
  if (!objective_trace) {
    ::pluss::throw_error("optim::septima_step_inplace: objective callback is required for line search");
  }

  // A line-search trial point is exactly where a wild step (e.g. saturating
  // a sigmoid/softplus-transformed parameter) is most likely to disconnect
  // the backward graph from an input. The library default silently
  // materializes a *zero* sensitivity for a disconnected input
  // (DisconnectedSensitivity::Zero), which is indistinguishable from a
  // genuinely near-zero gradient to every caller below -- Undefined lets
  // the line search (see its call site) tell the two apart and reject the
  // trial instead of accepting a fabricated "this looks converged" step.
  pluss::subsp::math::diff::DerivativePolicy policy{};
  policy.disconnected_sensitivity = pluss::subsp::math::diff::DisconnectedSensitivity::Undefined;
  policy.warn_on_disconnected = false;  // septima reports this via rejection + retry, not a log warning
  // Each line-search trial's (value, grad) pair is extracted and used
  // immediately below, then never touched again -- there is no legitimate
  // reason to retain this trial's backward graph afterward. The library
  // default (keep_graph=true) was previously masked by the false-convergence
  // bug this function also fixes (septima used to stop after ~2 iterations,
  // never running long enough for retained graphs to matter); once that bug
  // is fixed and septima runs its full iteration budget with a real
  // multi-trial line search, retaining every rejected trial's graph by
  // default accumulates unboundedly and can exhaust memory.
  policy.keep_graph = false;

  if (options.enable_constraint_autodiff && (constraints.equalities || constraints.inequalities)) {
    return value_and_grad(
        [&](const std::vector<detail::Trace>& vars) -> detail::Trace {
          return objective_trace(vars) + septima_constraint_objective_trace_(vars, state, constraints, options);
        },
        params, policy);
  }
  return value_and_grad(objective_trace, params, policy);
}

inline SeptimaState septima_state_like_(const std::vector<Series>& params,
                                        const SeptimaConstraints& constraints = {}) {
  septima_validate_params_(params);
  septima_validate_constraints_(params, constraints, false);

  SeptimaState state;
  state.adam = adam_state_like(params);

  if (constraints.equalities) {
    const auto residuals = septima_evaluate_residuals_(params, constraints.equalities, "equalities");
    state.lambda_equalities = septima_zero_like_(residuals);
  }
  if (constraints.inequalities) {
    const auto residuals = septima_evaluate_residuals_(params, constraints.inequalities, "inequalities");
    state.lambda_inequalities = septima_zero_like_(residuals);
  }
  state.constraint_violation = septima_constraint_violation_(params, constraints);
  return state;
}

// Largest component of `grads` after removing those a bound blocks: at a lower bound a positive gradient points
// outward (the step would go below the bound), at an upper bound a negative one does.
inline double septima_projected_gradient_norm_(const std::vector<Series>& params,
                                               const std::vector<Series>& grads,
                                               const SeptimaConstraints& constraints) {
  double norm = 0.0;
  for (std::size_t i = 0; i < params.size(); ++i) {
    Series g = grads[i];
    const bool has_lower = !constraints.lbs.empty() && constraints.lbs[i].defined();
    const bool has_upper = !constraints.ubs.empty() && constraints.ubs[i].defined();
    if (has_lower || has_upper) {
      Series blocked = pluss::series::zeros_like(g, ScalarType::Bool).to(g.platform());
      if (has_lower) blocked = blocked.logical_or(params[i].le(constraints.lbs[i]).logical_and(g.gt(0.0)));
      if (has_upper) blocked = blocked.logical_or(params[i].ge(constraints.ubs[i]).logical_and(g.lt(0.0)));
      g = g.mul(blocked.logical_not().to(g.scalar_type()));
    }
    norm = std::max(norm, septima_max_abs_(g));
  }
  return norm;
}

inline double septima_step_inplace_(std::vector<Series>& params,
                                    const std::vector<Series>& sensitivities,
                                    SeptimaState& state,
                                    const SeptimaOptions& options = {},
                                    const SeptimaConstraints& constraints = {},
                                    const SeptimaObjectiveTraceFn* objective_trace = nullptr,
                                    std::int64_t* function_evaluations = nullptr) {
  septima_validate_options_(options);
  septima_validate_params_(params);
  septima_validate_constraints_(params, constraints, options.enable_constraint_autodiff);

  if (sensitivities.size() != params.size()) {
    ::pluss::throw_error("optim::septima_step_inplace: params/sensitivities size mismatch");
  }
  if (state.adam.m.size() != params.size() || state.adam.v.size() != params.size()) {
    state = septima_state_like_(params, constraints);
  }

  std::vector<Series> grads;
  grads.reserve(params.size());
  for (std::size_t i = 0; i < params.size(); ++i) {
    require_defined_(sensitivities[i], "septima_step_inplace");
    require_same_platform_(params[i], sensitivities[i], "septima_step_inplace");
    require_float_dtype_(sensitivities[i], "septima_step_inplace");

    Series g = sensitivities[i];
    if (!same_shape_(params[i], g)) {
      if (params[i].numel() == 1 && g.numel() == 1) {
        g = g.sum();
      } else {
        ::pluss::throw_error("optim::septima_step_inplace: sensitivity shape mismatch");
      }
    }
    grads.push_back(std::move(g));
  }

  if (constraints.equalities) {
    const auto residuals = septima_evaluate_residuals_(params, constraints.equalities, "equalities");
    septima_align_multipliers_(state.lambda_equalities, residuals);
  }
  if (constraints.inequalities) {
    const auto residuals = septima_evaluate_residuals_(params, constraints.inequalities, "inequalities");
    septima_align_multipliers_(state.lambda_inequalities, residuals);
  }

  if (options.enable_constraint_autodiff && (constraints.equalities || constraints.inequalities)) {
    const auto [constraint_loss, constraint_grads] = value_and_grad(
        [&](const std::vector<detail::Trace>& vars) -> detail::Trace {
          return septima_constraint_objective_trace_(vars, state, constraints, options);
        },
        params);
    (void)constraint_loss;

    if (constraint_grads.size() != grads.size()) {
      ::pluss::throw_error("optim::septima_step_inplace: constraint gradient size mismatch");
    }
    for (std::size_t i = 0; i < grads.size(); ++i) {
      grads[i] = grads[i].add(constraint_grads[i]);
    }
  }
  state.stationarity = septima_projected_gradient_norm_(params, grads, constraints);

  bool accepted = false;
  double step_inf_norm = 0.0;

  if (options.optimizer == SeptimaOptions::Optimizer::Adam) {
    const auto params_backup = params;
    const AdamState adam_backup = state.adam;
    for (std::int64_t attempt = 0; attempt <= options.nan_recovery_attempts; ++attempt) {
      params = params_backup;
      state.adam = adam_backup;

      AdamOptions step_options = options.adam;
      if (attempt > 0) {
        step_options.learning_rate *= std::pow(options.nan_recovery_shrink, static_cast<double>(attempt));
      }

      adam_step_inplace(params, grads, state.adam, step_options);
      if (options.project_bounds) {
        septima_project_bounds_(params, constraints);
      }

      if (septima_finite_(params) &&
          septima_finite_(state.adam.m) &&
          septima_finite_(state.adam.v) &&
          septima_finite_(state.adam.max_v)) {
        accepted = true;
        break;
      }
    }
    if (!accepted) {
      params = params_backup;
      state.adam = adam_backup;
      state.constraint_violation = septima_constraint_violation_(params, constraints);
      return 0.0;
    }

    for (std::size_t i = 0; i < params.size(); ++i) {
      step_inf_norm = std::max(step_inf_norm, septima_max_abs_(params[i] - params_backup[i]));
    }
  } else {
    const auto params_layout = septima_make_layout_(params);
    const Series flat_params = septima_flatten_values_(params);
    const Series flat_grad = septima_flatten_values_(grads).to(flat_params.platform());

    std::vector<Series> next_s_history = state.lbfgs_s_history;
    std::vector<Series> next_y_history = state.lbfgs_y_history;
    if (state.lbfgs_prev_grad_flat.defined() &&
        state.lbfgs_prev_step_flat.defined() &&
        state.lbfgs_prev_grad_flat.numel() == flat_grad.numel() &&
        state.lbfgs_prev_step_flat.numel() == flat_grad.numel()) {
      const Series s = state.lbfgs_prev_step_flat.to(flat_grad.platform()).to(ScalarType::Float64);
      const Series y = flat_grad.sub(state.lbfgs_prev_grad_flat.to(flat_grad.platform()).to(ScalarType::Float64));
      update_lbfgs_history_(
          next_s_history,
          next_y_history,
          s,
          y,
          options.lbfgs.history_size,
          options.lbfgs.curvature_tolerance);
    }
    Series direction = lbfgs_direction_(flat_grad, next_s_history, next_y_history);
    if (!septima_finite_(direction) || !(series_dot_(flat_grad, direction) < 0.0)) {
      direction = flat_grad.mul(-1.0);
    }

    const double gtd = series_dot_(flat_grad, direction);
    // No descent direction means the (augmented) problem is solved as far as this method can tell. That is not the end of
    // the run when there are constraints: the multiplier update below still has to run, because it is what changes the
    // problem and lets the violation shrink. So the step is skipped, not the function.
    //
    // With the constraint terms in the gradient the inner problem has to be solved far more finely than the absolute
    // `tolerance_change` (a threshold on the squared gradient) allows: after a multiplier update the gradient is tiny, the
    // optimizer must still move to answer it, or the multipliers would be updated again from a stale point. Any strict
    // descent direction is therefore accepted there, and the sufficient-decrease test below keeps the steps safe. Those
    // penalty terms are also not smooth enough (the inequality term has a kink) for unguarded full steps, so a
    // backtracking search on the augmented value is used whenever they are present.
    const bool penalized = options.enable_constraint_autodiff && (constraints.equalities || constraints.inequalities) &&
                           objective_trace != nullptr;
    const double descent_floor = penalized ? 0.0 : options.lbfgs.tolerance_change;
    const bool no_descent = !(gtd < -descent_floor);
    if (no_descent) {
      accepted = true;
      step_inf_norm = 0.0;
    }

    if (no_descent) {
      // nothing to do: the point stays where it is
    } else if (options.lbfgs.line_search == SeptimaOptions::LbfgsLineSearch::StrongWolfe || penalized) {
      if (!objective_trace) {
        ::pluss::throw_error(
            "optim::septima_step_inplace: LBFGS strong-wolfe line search requires objective callback");
      }

      // Only the scalar value is needed here (for the Armijo condition
      // below) -- previously this called septima_augmented_value_grad_ and
      // discarded the gradient half, paying for a full backward pass every
      // outer iteration for nothing.
      const Series base_value = septima_augmented_value_only_(
          *objective_trace,
          params,
          state,
          options,
          constraints);
      if (function_evaluations) ++(*function_evaluations);

      const double base_objective = base_value.item<double>();
      double step_size = options.lbfgs.learning_rate;
      const std::int64_t ls_iters = std::max<std::int64_t>(1, options.lbfgs.max_line_search_iterations);

      // Accept on the Armijo (sufficient-decrease) condition alone, NOT the full strong-Wolfe
      // curvature condition -- a plain backtracking search cannot reliably enforce curvature,
      // since as step_size shrinks toward 0 the trial gradient's directional derivative gtd_new
      // converges TO gtd itself, making |gtd_new| <= -wolfe_c2*gtd (wolfe_c2<1) HARDER to
      // satisfy, not easier. This previously caused septima's LBFGS to report "no progress" and
      // stall after just a few iterations on realistic problems (confirmed directly on a
      // 5-parameter GARCH QMLE: 28 iterations, gamma/lambda barely moved from their initial
      // values) -- the exact same structural failure mode found and fixed in this SDK's L-SR1
      // trust-region optimizer (optim_lsr1.h). Dropping the line search's own curvature check is
      // safe for LBFGS specifically because `update_lbfgs_history_` already applies an
      // independent curvature skip (`lbfgs_curvature_ok_`) before adding any (s,y) pair to
      // history -- the line search enforcing curvature too was redundant "belt and suspenders",
      // not the only thing keeping the LBFGS approximation well-defined.
      //
      // Also track the best (smallest-objective) finite trial seen even when it misses Armijo's
      // margin: once step_size shrinks far enough, the *required* decrease
      // (wolfe_c1*step_size*gtd) can fall below the floating-point noise floor of evaluating the
      // objective, so a genuinely improving trial can still fail the strict inequality. Accepting
      // it anyway (rather than discarding real, if small, progress) lets septima's own outer
      // iteration continue instead of reporting a false "no progress".
      bool have_best = false;
      double best_objective = base_objective;
      std::vector<Series> best_params;

      for (std::int64_t ls = 0; ls < ls_iters; ++ls) {
        const Series trial_flat = flat_params.add(direction.mul(step_size));
        auto trial_params = septima_unflatten_values_(trial_flat, params_layout);
        if (options.project_bounds) {
          septima_project_bounds_(trial_params, constraints);
        }
        if (!septima_finite_(trial_params)) {
          step_size *= options.nan_recovery_shrink;
          continue;
        }

        const auto [trial_value, trial_grads] = septima_augmented_value_grad_(
            *objective_trace,
            trial_params,
            state,
            options,
            constraints);
        if (function_evaluations) ++(*function_evaluations);
        // Check definedness before finiteness: septima_augmented_value_grad_
        // now asks for Undefined (not Zero) disconnected sensitivities, so a
        // disconnected trial point comes back with an undefined Series here
        // -- calling septima_finite_ on that would itself fail, and more
        // importantly a disconnected gradient must be rejected exactly like
        // a non-finite one, not treated as "gradient is legitimately zero".
        const bool trial_grads_defined =
            std::all_of(trial_grads.begin(), trial_grads.end(), [](const Series& g) { return g.defined(); });
        if (!septima_finite_(trial_value) || !trial_grads_defined || !septima_finite_(trial_grads)) {
          step_size *= options.nan_recovery_shrink;
          continue;
        }

        const double trial_objective = trial_value.item<double>();

        const bool armijo_ok =
            trial_objective <= base_objective + options.lbfgs.wolfe_c1 * step_size * gtd;

        if (armijo_ok) {
          params = std::move(trial_params);
          state.lbfgs_s_history = next_s_history;
          state.lbfgs_y_history = next_y_history;
          state.lbfgs_prev_grad_flat = flat_grad;
          const Series next_flat = septima_flatten_values_(params).to(flat_params.platform());
          state.lbfgs_prev_step_flat = next_flat.sub(flat_params);
          step_inf_norm = septima_max_abs_(state.lbfgs_prev_step_flat);
          accepted = true;
          break;
        }

        if (trial_objective < best_objective) {
          have_best = true;
          best_objective = trial_objective;
          best_params = trial_params;
        }

        step_size *= options.nan_recovery_shrink;
        if (step_size <= options.lbfgs.tolerance_change) break;
      }
      if (!accepted && have_best) {
        params = std::move(best_params);
        state.lbfgs_s_history = next_s_history;
        state.lbfgs_y_history = next_y_history;
        state.lbfgs_prev_grad_flat = flat_grad;
        const Series next_flat = septima_flatten_values_(params).to(flat_params.platform());
        state.lbfgs_prev_step_flat = next_flat.sub(flat_params);
        step_inf_norm = septima_max_abs_(state.lbfgs_prev_step_flat);
        accepted = true;
      }
    } else {
      for (std::int64_t attempt = 0; attempt <= options.nan_recovery_attempts; ++attempt) {
        double step_size = options.lbfgs.learning_rate;
        if (attempt > 0) {
          step_size *= std::pow(options.nan_recovery_shrink, static_cast<double>(attempt));
        }
        const Series trial_flat = flat_params.add(direction.mul(step_size));
        auto trial_params = septima_unflatten_values_(trial_flat, params_layout);
        if (options.project_bounds) {
          septima_project_bounds_(trial_params, constraints);
        }
        if (!septima_finite_(trial_params)) continue;

        params = std::move(trial_params);
        state.lbfgs_s_history = next_s_history;
        state.lbfgs_y_history = next_y_history;
        state.lbfgs_prev_grad_flat = flat_grad;
        const Series next_flat = septima_flatten_values_(params).to(flat_params.platform());
        state.lbfgs_prev_step_flat = next_flat.sub(flat_params);
        step_inf_norm = septima_max_abs_(state.lbfgs_prev_step_flat);
        accepted = true;
        break;
      }
    }
    if (!accepted) {
      // Every trial step failed. With constraints the multiplier update can still change the problem; without them
      // there is nothing left to do.
      if (!(constraints.equalities || constraints.inequalities)) {
        state.constraint_violation = septima_constraint_violation_(params, constraints);
        return 0.0;
      }
      step_inf_norm = 0.0;
    }
  }

  double violation = 0.0;
  std::vector<Series> equality_residuals, inequality_residuals;
  if (constraints.equalities) {
    equality_residuals = septima_evaluate_residuals_(params, constraints.equalities, "equalities");
    septima_align_multipliers_(state.lambda_equalities, equality_residuals);
    for (const auto& r : equality_residuals) violation = std::max(violation, septima_max_abs_(r));
  }
  if (constraints.inequalities) {
    inequality_residuals = septima_evaluate_residuals_(params, constraints.inequalities, "inequalities");
    septima_align_multipliers_(state.lambda_inequalities, inequality_residuals);
    for (const auto& r : inequality_residuals) violation = std::max(violation, septima_max_abs_(r.maximum(0.0)));
  }

  if (!constraints.lbs.empty()) {
    for (std::size_t i = 0; i < params.size(); ++i) {
      if (!constraints.lbs[i].defined()) continue;
      violation = std::max(violation, septima_max_abs_((constraints.lbs[i] - params[i]).maximum(0.0)));
    }
  }
  if (!constraints.ubs.empty()) {
    for (std::size_t i = 0; i < params.size(); ++i) {
      if (!constraints.ubs[i].defined()) continue;
      violation = std::max(violation, septima_max_abs_((params[i] - constraints.ubs[i]).maximum(0.0)));
    }
  }

  if (constraints.equalities || constraints.inequalities) {
    ++state.steps_since_dual_update;
    // How far the multipliers are from complementarity: an inequality with a positive multiplier and slack to spare
    // still has a multiplier that wants to come down, even though nothing is violated.
    double pending = violation;
    for (std::size_t i = 0; i < inequality_residuals.size(); ++i) {
      pending = std::max(pending, septima_max_abs_(state.lambda_inequalities[i].minimum(inequality_residuals[i].mul(-1.0).maximum(0.0))));
    }
    // Without the penalty term in the gradient (enable_constraint_autodiff == false) the multiplier update is the only
    // thing acting on the constraints, so it runs every step, as a plain dual ascent. Otherwise it waits until the
    // augmented problem is solved to a tolerance tied to the current infeasibility.
    const bool inner_solved =
        state.stationarity <= std::max(options.dual_update_floor, options.dual_update_ratio * pending);
    // An optimizer that can no longer move (no descent direction, or every trial step rejected) has solved the augmented
    // problem as far as it can; waiting for a tolerance it cannot reach would only burn the step cap.
    const bool inner_stalled = step_inf_norm <= 0.0;
    const bool update_duals = !options.enable_constraint_autodiff || inner_solved || inner_stalled ||
                              state.steps_since_dual_update >= options.dual_update_max_steps;
    if (update_duals) {
      const double legacy_rate = options.enable_constraint_autodiff ? 0.0 : 1e-2;
      const double equality_rate = options.dual_learning_rate > 0.0 ? options.dual_learning_rate
                                   : (legacy_rate > 0.0 ? legacy_rate : options.equality_penalty * state.penalty_scale);
      const double inequality_rate = options.dual_learning_rate > 0.0 ? options.dual_learning_rate
                                     : (legacy_rate > 0.0 ? legacy_rate : options.inequality_penalty * state.penalty_scale);
      for (std::size_t i = 0; i < equality_residuals.size(); ++i) {
        state.lambda_equalities[i] = state.lambda_equalities[i].add(equality_residuals[i].mul(equality_rate));
      }
      for (std::size_t i = 0; i < inequality_residuals.size(); ++i) {
        // Signed residual: the multiplier grows while the constraint is violated and relaxes while it is slack.
        state.lambda_inequalities[i] =
            state.lambda_inequalities[i].add(inequality_residuals[i].mul(inequality_rate)).maximum(0.0);
      }
      state.steps_since_dual_update = 0;
      ++state.dual_updates;
      // Only an update that followed a solved inner problem says anything about the penalty: when the step cap forced the
      // update, the violation is still shrinking for lack of work, not for lack of weight.
      if (options.enable_constraint_autodiff && options.penalty_growth > 1.0 && (inner_solved || inner_stalled) &&
          violation > options.penalty_growth_trigger * state.last_update_violation) {
        const double largest = std::max(options.equality_penalty, options.inequality_penalty);
        if (largest > 0.0) {
          state.penalty_scale = std::min(state.penalty_scale * options.penalty_growth, options.max_penalty / largest);
        }
      }
      state.last_update_violation = violation;
      // The next gradient belongs to a different function: do not form a curvature pair across the change.
      state.lbfgs_prev_grad_flat = pluss::series::undefined();
      state.lbfgs_prev_step_flat = pluss::series::undefined();
    }
  }
  state.constraint_violation = violation;

  ++state.step;
  return step_inf_norm;
}

inline void septima_step_inplace_from_tape_(std::vector<Series>& params,
                                            SeptimaState& state,
                                            const SeptimaOptions& options = {},
                                            const SeptimaConstraints& constraints = {}) {
  std::vector<Series> sensitivities;
  sensitivities.reserve(params.size());
  for (const auto& p : params) {
    const Series g = p.sensitivity();
    if (!g.defined()) {
      ::pluss::throw_error("optim::septima_step_inplace: undefined sensitivity in params");
    }
    sensitivities.push_back(g);
  }
  (void)septima_step_inplace_(params, sensitivities, state, options, constraints);
}

} // namespace detail

namespace detail {

inline SeptimaResult septima_trace_(const SeptimaObjectiveTraceFn& objective_trace,
                             std::vector<Series> initial_params,
                             const SeptimaSolveOptions& solve_options = {},
                             const SeptimaConstraints& constraints = {}) {
  if (!objective_trace) {
    ::pluss::throw_error("optim::septima: objective callback is required");
  }
  detail::septima_validate_solve_options_(solve_options);
  detail::septima_validate_options_(solve_options.step);
  detail::septima_validate_params_(initial_params);
  detail::septima_validate_constraints_(
      initial_params, constraints, solve_options.step.enable_constraint_autodiff);

  SeptimaResult out;
  out.params = std::move(initial_params);
  out.state = detail::septima_state_like_(out.params, constraints);
  out.exit_flag = SeptimaExitFlag::MaxIterations;

  double prev_value = std::numeric_limits<double>::infinity();
  std::int64_t no_progress_count = 0;

  for (std::int64_t iter = 1; iter <= solve_options.max_iterations; ++iter) {
    // Same rationale as septima_augmented_value_grad_'s internal calls:
    // request Undefined (not the library default Zero) for disconnected
    // inputs, so a disconnected gradient at the CURRENT accepted point can
    // be told apart from a genuinely near-zero one below, instead of
    // silently reporting gradient_inf_norm == 0.0 (a fabricated
    // "converged" signal) when the true gradient is simply unavailable.
    pluss::subsp::math::diff::DerivativePolicy grad_policy{};
    grad_policy.disconnected_sensitivity = pluss::subsp::math::diff::DisconnectedSensitivity::Undefined;
    grad_policy.warn_on_disconnected = false;
    // See septima_augmented_value_grad_'s identical keep_graph rationale:
    // this iteration's (value, grads) are used immediately below and never
    // revisited, so there is nothing to gain from retaining the graph and,
    // over solve_options.max_iterations, real memory to lose by doing so.
    grad_policy.keep_graph = false;
    const auto [value, grads] = value_and_grad(objective_trace, out.params, grad_policy);
    ++out.function_evaluations;
    out.objective = value;

    if (!value.defined() || value.numel() != 1) {
      ::pluss::throw_error("optim::septima: objective must return a scalar detail::Trace");
    }
    out.objective_value = value.item<double>();

    const bool grads_defined = std::all_of(grads.begin(), grads.end(), [](const Series& g) { return g.defined(); });
    const bool grads_finite = grads_defined && detail::septima_finite_(grads);

    if (!std::isfinite(out.objective_value) || !grads_defined || !grads_finite) {
      out.iteration = iter;
      out.message = !grads_defined
                        ? "gradient disconnected from one or more inputs at this point"
                        : (!grads_finite ? "gradient became non-finite" : "objective became non-finite");
      out.exit_flag = SeptimaExitFlag::NonFiniteEvaluation;
      if (solve_options.stop_on_nonfinite_objective) {
        break;
      }
    }

    // If the caller opted out of stopping above, substitute zeros for any
    // undefined block so the step machinery below (which assumes every
    // block is a real Series) doesn't itself fail on a value we already
    // flagged as invalid.
    std::vector<Series> grads_for_step = grads;
    if (!grads_defined) {
      for (std::size_t i = 0; i < grads_for_step.size(); ++i) {
        if (!grads_for_step[i].defined()) {
          grads_for_step[i] = pluss::series::zeros_like(out.params[i], out.params[i].scalar_type());
        }
      }
    }

    out.gradient_inf_norm = grads_defined ? 0.0 : std::numeric_limits<double>::infinity();
    if (grads_defined) {
      for (const auto& g : grads) {
        out.gradient_inf_norm = std::max(out.gradient_inf_norm, detail::septima_max_abs_(g));
      }
    }

    out.step_inf_norm = detail::septima_step_inplace_(
        out.params,
        grads_for_step,
        out.state,
        solve_options.step,
        constraints,
        &objective_trace,
        &out.function_evaluations);
    out.constraint_violation = out.state.constraint_violation;
    out.stationarity = out.state.stationarity;
    out.iteration = iter;

    // Stationarity, not the objective gradient alone: at a constrained optimum the objective gradient is balanced
    // by the constraint terms and does not vanish. Without constraints the two are the same number.
    const bool converged =
        out.stationarity <= solve_options.gradient_tolerance &&
        out.step_inf_norm <= solve_options.step_tolerance &&
        out.constraint_violation <= solve_options.constraint_tolerance;
    if (converged) {
      out.converged = true;
      out.exit_flag = SeptimaExitFlag::Converged;
      out.message = "converged";
    } else {
      const bool objective_stalled =
          std::isfinite(prev_value) &&
          std::fabs(out.objective_value - prev_value) <= solve_options.objective_tolerance;
      const bool step_stalled = out.step_inf_norm <= solve_options.step_tolerance;
      // While the constraints are still violated the multipliers are still working, so a flat objective is not
      // evidence that the run has stopped making progress.
      if (objective_stalled && step_stalled && out.constraint_violation <= solve_options.constraint_tolerance) {
        ++no_progress_count;
      } else {
        no_progress_count = 0;
      }
      if (solve_options.max_no_progress_iterations > 0 &&
          no_progress_count >= solve_options.max_no_progress_iterations) {
        out.exit_flag = SeptimaExitFlag::NoProgress;
        out.message = "no progress";
      }
    }

    SeptimaProgressSnapshot snapshot{};
    snapshot.iteration = out.iteration;
    snapshot.function_evaluations = out.function_evaluations;
    snapshot.objective = out.objective_value;
    snapshot.gradient_inf_norm = out.gradient_inf_norm;
    snapshot.stationarity = out.stationarity;
    snapshot.step_inf_norm = out.step_inf_norm;
    snapshot.constraint_violation = out.constraint_violation;
    snapshot.message = out.message;
    if (solve_options.progress_hook && !solve_options.progress_hook(snapshot)) {
      out.exit_flag = SeptimaExitFlag::Stopped;
      out.message = "stopped by progress hook";
    }

    prev_value = out.objective_value;
    if (out.exit_flag != SeptimaExitFlag::MaxIterations) break;
  }

  if (out.exit_flag == SeptimaExitFlag::MaxIterations) {
    out.message = "max iterations reached";
  }
  return out;
}

}  // namespace detail

inline SeptimaResult septima(const SeptimaObjectiveSeriesFn& objective_series,
                             std::vector<Series> initial_params,
                             const SeptimaSolveOptions& solve_options = {},
                             const SeptimaConstraints& constraints = {}) {
  if (!objective_series) {
    ::pluss::throw_error("optim::septima: objective callback is required");
  }
  return detail::septima_trace_(
      [f = objective_series](const std::vector<detail::Trace>& vars) -> detail::Trace {
        std::vector<Series> values;
        values.reserve(vars.size());
        for (const auto& v : vars) values.push_back(v.value());
        return detail::Trace(f(values));
      },
      std::move(initial_params),
      solve_options,
      constraints);
}

inline SeptimaResult septima(const std::function<Series(const ParameterPack<Series>&)>& objective_series,
                             const ParameterPack<Series>& initial_params,
                             const SeptimaSolveOptions& solve_options = {},
                             const SeptimaConstraints& constraints = {}) {
  if (!objective_series) {
    ::pluss::throw_error("optim::septima: objective callback is required");
  }
  std::vector<Series> init_values;
  std::vector<std::string> names;
  init_values.reserve(initial_params.size());
  names.reserve(initial_params.size());
  for (const auto& b : initial_params.blocks()) {
    names.push_back(b.name);
    init_values.push_back(b.value);
  }

  return detail::septima_trace_(
      [f = objective_series, names = std::move(names)](const std::vector<detail::Trace>& vars) -> detail::Trace {
        if (vars.size() != names.size()) {
          ::pluss::throw_error("optim::septima: objective parameter block count mismatch");
        }
        std::vector<typename ParameterPack<Series>::Block> blocks;
        blocks.reserve(vars.size());
        for (std::size_t i = 0; i < vars.size(); ++i) {
          blocks.push_back({names[i], vars[i].value()});
        }
        return detail::Trace(f(ParameterPack<Series>(std::move(blocks))));
      },
      std::move(init_values),
      solve_options,
      constraints);
}

inline SeptimaResult septima(const std::function<Series(const Series&)>& objective_series,
                             const Series& initial_param,
                             const SeptimaSolveOptions& solve_options = {},
                             const SeptimaConstraints& constraints = {}) {
  if (!objective_series) {
    ::pluss::throw_error("optim::septima: objective callback is required");
  }
  return detail::septima_trace_(
      [&](const std::vector<detail::Trace>& vars) -> detail::Trace {
        if (vars.size() != 1) {
          ::pluss::throw_error("optim::septima: expected one parameter block");
        }
        return detail::Trace(objective_series(vars[0].value()));
      },
      std::vector<Series>{initial_param},
      solve_options,
      constraints);
}

inline SeptimaResult septima(const std::function<Series(const Series&)>& objective_series,
                             const ParameterPack<Series>& initial_params,
                             const SeptimaSolveOptions& solve_options,
                             const SeptimaConstraints& constraints = {}) {
  if (initial_params.size() != 1) {
    ::pluss::throw_error("optim::septima: single-series objective requires one parameter block");
  }
  const auto& first = initial_params.blocks().front();
  return septima(objective_series, initial_params.at(first.name), solve_options, constraints);
}

} // namespace pluss::subsp::math::optim

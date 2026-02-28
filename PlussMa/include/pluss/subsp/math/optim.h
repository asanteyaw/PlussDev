#pragma once

#include "pluss/subsp/math/diff.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace pluss::subsp::math::optim {

namespace detail {

inline bool same_shape_(const Series& a, const Series& b) {
  if (!a.defined() || !b.defined()) return false;
  if (a.sizes().size() != b.sizes().size()) return false;
  for (std::size_t i = 0; i < a.sizes().size(); ++i) {
    if (a.sizes()[i] != b.sizes()[i]) return false;
  }
  return true;
}

inline void require_defined_(const Series& x, const char* op) {
  if (!x.defined()) {
    throw std::runtime_error(std::string("subsp::math::optim::") + op + ": expected defined Series");
  }
}

inline void require_same_platform_(const Series& a, const Series& b, const char* op) {
  if (a.platform() != b.platform()) {
    throw std::runtime_error(std::string("subsp::math::optim::") + op + ": expected matching platforms");
  }
}

inline void require_float_dtype_(const Series& x, const char* op) {
  if (!x.dtype().is_floating() && !x.dtype().is_complex()) {
    throw std::runtime_error(std::string("subsp::math::optim::") + op +
                             ": expected floating or complex dtype");
  }
}

} // namespace detail

// Compatibility re-export: existing callers can continue to use the older
// optim-facing trace helpers while new derivative-oriented code targets
// pluss::subsp::math::diff directly.
using pluss::subsp::math::diff::Trace;
using pluss::subsp::math::diff::DualTrace;
using pluss::subsp::math::diff::trace;
using pluss::subsp::math::diff::fixed;
using pluss::subsp::math::diff::add;
using pluss::subsp::math::diff::sub;
using pluss::subsp::math::diff::mul;
using pluss::subsp::math::diff::div;
using pluss::subsp::math::diff::neg;
using pluss::subsp::math::diff::pow;
using pluss::subsp::math::diff::exp;
using pluss::subsp::math::diff::log;
using pluss::subsp::math::diff::sqrt;
using pluss::subsp::math::diff::sin;
using pluss::subsp::math::diff::cos;
using pluss::subsp::math::diff::tanh;
using pluss::subsp::math::diff::square;
using pluss::subsp::math::diff::rsqrt;
using pluss::subsp::math::diff::relu;
using pluss::subsp::math::diff::sigmoid;
using pluss::subsp::math::diff::softplus;
using pluss::subsp::math::diff::clamp;
using pluss::subsp::math::diff::sum;
using pluss::subsp::math::diff::mean;
using pluss::subsp::math::diff::matmul;
using pluss::subsp::math::diff::clear_sensitivities;
using pluss::subsp::math::diff::grad;
using pluss::subsp::math::diff::hvp;
using pluss::subsp::math::diff::jvp;
using pluss::subsp::math::diff::linearize;
using pluss::subsp::math::diff::nth_derivative;
using pluss::subsp::math::diff::vjp;
using pluss::subsp::math::diff::value_and_grad;

struct ParameterBlockLayout {
  std::string name;
  std::vector<std::int64_t> sizes;
  std::int64_t numel{0};
  ScalarType scalar_type{ScalarType::Float32};
  pluss::codem::Platform platform{pluss::codem::Platform::CPU};
};

class ParameterLayout {
public:
  ParameterLayout() = default;
  explicit ParameterLayout(std::vector<ParameterBlockLayout> blocks) : blocks_(std::move(blocks)) {}

  const std::vector<ParameterBlockLayout>& blocks() const noexcept { return blocks_; }
  std::size_t size() const noexcept { return blocks_.size(); }
  bool empty() const noexcept { return blocks_.empty(); }

  std::int64_t total_numel() const noexcept {
    std::int64_t total = 0;
    for (const auto& block : blocks_) total += block.numel;
    return total;
  }

  bool contains(std::string_view name) const noexcept {
    return std::any_of(
        blocks_.begin(), blocks_.end(), [&](const auto& block) { return block.name == name; });
  }

  const ParameterBlockLayout& at(std::string_view name) const {
    for (const auto& block : blocks_) {
      if (block.name == name) return block;
    }
    throw std::runtime_error("subsp::math::optim::ParameterLayout::at: unknown parameter block");
  }

private:
  std::vector<ParameterBlockLayout> blocks_{};
};

template <class Fn>
std::pair<Series, std::vector<Series>> value_and_sensitivity(Fn&& fn, const std::vector<Series>& params) {
  return pluss::subsp::math::diff::value_and_grad(std::forward<Fn>(fn), params);
}

template <class T>
class ParameterPack {
public:
  struct Block {
    std::string name;
    T value;
  };

  ParameterPack() = default;
  explicit ParameterPack(std::vector<Block> blocks) : blocks_(std::move(blocks)) {
    validate_names_();
  }

  std::size_t size() const noexcept { return blocks_.size(); }
  bool empty() const noexcept { return blocks_.empty(); }

  const Block& operator[](std::size_t index) const { return blocks_.at(index); }
  Block& operator[](std::size_t index) { return blocks_.at(index); }

  const std::vector<Block>& blocks() const noexcept { return blocks_; }
  std::int64_t total_numel() const noexcept {
    std::int64_t total = 0;
    for (const auto& block : blocks_) total += block.value.numel();
    return total;
  }

  bool contains(std::string_view name) const noexcept {
    return std::any_of(
        blocks_.begin(), blocks_.end(), [&](const Block& block) { return block.name == name; });
  }

  const T& at(std::string_view name) const {
    for (const auto& block : blocks_) {
      if (block.name == name) return block.value;
    }
    throw std::runtime_error("subsp::math::optim::ParameterPack::at: unknown parameter block");
  }

  T& at(std::string_view name) {
    for (auto& block : blocks_) {
      if (block.name == name) return block.value;
    }
    throw std::runtime_error("subsp::math::optim::ParameterPack::at: unknown parameter block");
  }

  std::vector<T> values() const {
    std::vector<T> out;
    out.reserve(blocks_.size());
    for (const auto& block : blocks_) {
      out.push_back(block.value);
    }
    return out;
  }

  template <class U>
  ParameterPack<U> rebind(std::vector<U> values) const {
    if (values.size() != blocks_.size()) {
      throw std::runtime_error("subsp::math::optim::ParameterPack::rebind: size mismatch");
    }

    std::vector<typename ParameterPack<U>::Block> rebound;
    rebound.reserve(blocks_.size());
    for (std::size_t i = 0; i < blocks_.size(); ++i) {
      rebound.push_back(typename ParameterPack<U>::Block{blocks_[i].name, std::move(values[i])});
    }
    return ParameterPack<U>(std::move(rebound));
  }

  ParameterLayout layout() const requires std::is_same_v<T, Series> {
    std::vector<ParameterBlockLayout> out;
    out.reserve(blocks_.size());
    for (const auto& block : blocks_) {
      detail::require_defined_(block.value, "ParameterPack::layout");
      const auto sizes = block.value.sizes();
      out.push_back(ParameterBlockLayout{
          block.name,
          std::vector<std::int64_t>(sizes.begin(), sizes.end()),
          block.value.numel(),
          block.value.scalar_type(),
          block.value.platform()});
    }
    return ParameterLayout(std::move(out));
  }

  bool same_layout(const ParameterPack<Series>& other) const requires std::is_same_v<T, Series> {
    if (blocks_.size() != other.blocks_.size()) return false;
    for (std::size_t i = 0; i < blocks_.size(); ++i) {
      const auto& lhs = blocks_[i];
      const auto& rhs = other.blocks_[i];
      if (lhs.name != rhs.name) return false;
      if (lhs.value.scalar_type() != rhs.value.scalar_type()) return false;
      if (lhs.value.platform() != rhs.value.platform()) return false;
      if (!detail::same_shape_(lhs.value, rhs.value)) return false;
    }
    return true;
  }

  Series flatten_values() const requires std::is_same_v<T, Series> {
    if (blocks_.empty()) {
      throw std::runtime_error("subsp::math::optim::ParameterPack::flatten_values: empty pack");
    }

    std::vector<Series> flat;
    flat.reserve(blocks_.size());
    const auto platform = blocks_.front().value.platform();
    const auto scalar_type = blocks_.front().value.scalar_type();
    for (const auto& block : blocks_) {
      detail::require_defined_(block.value, "ParameterPack::flatten_values");
      detail::require_same_platform_(blocks_.front().value, block.value, "ParameterPack::flatten_values");
      if (block.value.scalar_type() != scalar_type) {
        throw std::runtime_error(
            "subsp::math::optim::ParameterPack::flatten_values: expected matching dtypes");
      }
      (void)platform;
      flat.push_back(block.value.reshape({block.value.numel()}));
    }

    if (flat.size() == 1) return flat.front();
    return flat.front().unite(std::vector<Series>(flat.begin() + 1, flat.end()), 0);
  }

  ParameterPack<Series> zeros_like() const requires std::is_same_v<T, Series> {
    std::vector<typename ParameterPack<Series>::Block> out;
    out.reserve(blocks_.size());
    for (const auto& block : blocks_) {
      detail::require_defined_(block.value, "ParameterPack::zeros_like");
      out.push_back({block.name, pluss::zeros_like(block.value, block.value.scalar_type())});
    }
    return ParameterPack<Series>(std::move(out));
  }

private:
  void validate_names_() const {
    std::unordered_set<std::string> seen;
    seen.reserve(blocks_.size());
    for (const auto& block : blocks_) {
      if (block.name.empty()) {
        throw std::runtime_error("subsp::math::optim::ParameterPack: block names must be non-empty");
      }
      if (!seen.insert(block.name).second) {
        throw std::runtime_error("subsp::math::optim::ParameterPack: duplicate block name");
      }
    }
  }

  std::vector<Block> blocks_{};
};

template <class Fn>
std::pair<Series, ParameterPack<Series>> value_and_grad(Fn&& fn, const ParameterPack<Series>& params) {
  auto [value, grads] = pluss::subsp::math::diff::value_and_grad(
      [&](const std::vector<Trace>& xs) -> Trace {
        return std::forward<Fn>(fn)(params.template rebind<Trace>(xs));
      },
      params.values());
  return {value, params.template rebind<Series>(std::move(grads))};
}

inline ParameterPack<Series> unflatten_values(const ParameterLayout& layout, const Series& flat_values) {
  detail::require_defined_(flat_values, "unflatten_values");

  Series flat = flat_values.reshape({flat_values.numel()});
  if (flat.numel() != layout.total_numel()) {
    throw std::runtime_error("subsp::math::optim::unflatten_values: flat size mismatch");
  }

  std::vector<typename ParameterPack<Series>::Block> blocks;
  blocks.reserve(layout.size());

  std::int64_t offset = 0;
  for (const auto& block : layout.blocks()) {
    const Series idx = pluss::arange(offset, offset + block.numel, 1, ScalarType::Int64).to(flat.platform());
    Series piece = flat.index({idx}).contiguous();
    if (block.sizes.empty()) {
      piece = piece.reshape({});
    } else {
      piece = piece.reshape(block.sizes);
    }
    if (piece.scalar_type() != block.scalar_type) piece = piece.to(block.scalar_type);
    if (piece.platform() != block.platform) piece = piece.to(block.platform);
    blocks.push_back({block.name, std::move(piece)});
    offset += block.numel;
  }

  return ParameterPack<Series>(std::move(blocks));
}

inline Series flatten_values(const ParameterPack<Series>& params) {
  return params.flatten_values();
}

template <class Fn>
ParameterPack<Series> grad(Fn&& fn, const ParameterPack<Series>& params) {
  return value_and_grad(std::forward<Fn>(fn), params).second;
}

template <class Fn>
std::pair<Series, ParameterPack<Series>> jacrev(Fn&& fn, const ParameterPack<Series>& params) {
  auto [value, jacobians] = pluss::subsp::math::diff::jacrev(
      [&](const std::vector<Trace>& xs) -> Trace {
        return std::forward<Fn>(fn)(params.template rebind<Trace>(xs));
      },
      params.values());
  return {value, params.template rebind<Series>(std::move(jacobians))};
}

template <class Fn>
std::pair<Series, ParameterPack<Series>> jacfwd(Fn&& fn, const ParameterPack<Series>& params) {
  auto [value, jacobians] = pluss::subsp::math::diff::jacfwd(
      [&](const std::vector<DualTrace>& xs) -> DualTrace {
        return std::forward<Fn>(fn)(params.template rebind<DualTrace>(xs));
      },
      params.values());
  return {value, params.template rebind<Series>(std::move(jacobians))};
}

template <class Fn>
std::pair<Series, Series> jvp(Fn&& fn,
                              const ParameterPack<Series>& params,
                              const ParameterPack<Series>& tangents) {
  return pluss::subsp::math::diff::jvp(
      [&](const std::vector<DualTrace>& xs) -> DualTrace {
        return std::forward<Fn>(fn)(params.template rebind<DualTrace>(xs));
      },
      params.values(),
      tangents.values());
}

template <class Fn>
std::pair<Series, ParameterPack<Series>> hvp(Fn&& fn,
                                             const ParameterPack<Series>& params,
                                             const ParameterPack<Series>& tangents,
                                             const pluss::subsp::math::diff::HvpOptions& opt = {}) {
  using FnType = std::decay_t<Fn>;
  auto params_values = params.values();
  auto tangent_values = tangents.values();

  std::pair<Series, std::vector<Series>> raw{};
  if constexpr (std::is_invocable_v<FnType&, const ParameterPack<DualTrace>&>) {
    FnType fn_copy(std::forward<Fn>(fn));
    raw = pluss::subsp::math::diff::hvp(
        [&](const std::vector<DualTrace>& xs) -> DualTrace {
          return fn_copy(params.template rebind<DualTrace>(xs));
        },
        params_values,
        tangent_values,
        opt);
  } else {
    raw = pluss::subsp::math::diff::hvp(
        [&](const std::vector<Trace>& xs) -> Trace {
          return std::forward<Fn>(fn)(params.template rebind<Trace>(xs));
        },
        params_values,
        tangent_values,
        opt);
  }
  auto& [value, hv] = raw;
  return {value, params.template rebind<Series>(std::move(hv))};
}

template <class Fn>
auto linearize(Fn&& fn, const ParameterPack<Series>& params) {
  auto [value, pushforward] = pluss::subsp::math::diff::linearize(
      [&](const std::vector<DualTrace>& xs) -> DualTrace {
        return std::forward<Fn>(fn)(params.template rebind<DualTrace>(xs));
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
  Series equality{};
  Series inequality{};
  ParameterPack<Series> equality_jacobian{};
  ParameterPack<Series> inequality_jacobian{};
};

struct ObjectiveDefinition {
  std::function<Trace(const ParameterPack<Trace>&)> autodiff{};
  std::function<std::pair<Series, ParameterPack<Series>>(const ParameterPack<Series>&)> analytic{};

  bool defined() const noexcept {
    return static_cast<bool>(autodiff) || static_cast<bool>(analytic);
  }
};

struct NonlinearConstraints {
  std::function<Trace(const ParameterPack<Trace>&)> equality{};
  std::function<Trace(const ParameterPack<Trace>&)> inequality{};
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

struct ProblemEvaluation {
  EvalStatus status{EvalStatus::Ok};
  std::string message{};
  Series value{};
  ParameterPack<Series> gradient{};
  Series nonlinear_equality_residual{};
  Series nonlinear_inequality_residual{};
  Series equality_residual{};
  Series inequality_residual{};
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
  Series value{};
  double max_constraint_violation{0.0};
  double first_order_opt{0.0};
};

enum class OptimaProgressStage : std::uint8_t {
  Start = 0,
  Iteration,
  Done,
};

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
  std::string message{};
  std::optional<BestFeasiblePoint> best_feasible{};
};

struct ConstraintMultipliers {
  Series linear_equalities{};
  Series nonlinear_equalities{};
  Series linear_inequalities{};
  Series nonlinear_inequalities{};
  Series lower_bounds{};
  Series upper_bounds{};
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
};

enum class OptimaLinearSystemSolver : std::uint8_t {
  Auto = 0,
  Cholesky,
  Inverse,
  PseudoInverse,
};

enum class OptimaHessianApproximation : std::uint8_t {
  Bfgs = 0,
  Identity,
};

struct SequentialQPOptions {
  std::int64_t max_iterations{100};
  std::int64_t max_line_search_iterations{20};
  std::int64_t max_restoration_iterations{8};
  std::int64_t max_restoration_line_search_iterations{20};
  double initial_step_size{1.0};
  double backtracking{0.5};
  double armijo{1e-4};
  double initial_penalty{10.0};
  double constraint_tolerance{1e-6};
  double optimality_tolerance{1e-6};
  double active_set_tolerance{1e-4};
  double regularization{1e-6};
  double bfgs_curvature_tolerance{1e-8};
  double objective_limit{-std::numeric_limits<double>::infinity()};
  bool project_bounds{true};
  bool enable_restoration{true};
  OptimaLinearSystemSolver linear_system_solver{OptimaLinearSystemSolver::Auto};
  OptimaHessianApproximation hessian_approximation{OptimaHessianApproximation::Bfgs};
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
  Series hessian{};
  Series multipliers{};
};

struct InteriorPointOptions {
  std::int64_t max_iterations{100};
  std::int64_t max_line_search_iterations{25};
  std::int64_t max_restoration_iterations{8};
  std::int64_t max_restoration_line_search_iterations{25};
  double initial_step_size{1.0};
  double backtracking{0.5};
  double armijo{1e-4};
  double initial_barrier_parameter{1e-1};
  double minimum_barrier_parameter{1e-8};
  double barrier_decay{0.2};
  double equality_penalty{10.0};
  double constraint_tolerance{1e-6};
  double optimality_tolerance{1e-6};
  double fraction_to_boundary{0.99};
  double initialization_margin{1e-4};
  double bfgs_curvature_tolerance{1e-8};
  double objective_limit{-std::numeric_limits<double>::infinity()};
  bool project_bounds{true};
  bool enable_restoration{true};
  OptimaLinearSystemSolver linear_system_solver{OptimaLinearSystemSolver::Auto};
  OptimaHessianApproximation hessian_approximation{OptimaHessianApproximation::Bfgs};
  std::function<bool(const OptimaProgressSnapshot&)> progress_hook{};
};

struct InteriorPointState {
  ParameterPack<Series> params{};
  ProblemEvaluation evaluation{};
  std::int64_t iteration{0};
  std::int64_t function_evaluations{0};
  double last_step_size{0.0};
  double barrier_parameter{0.0};
  double barrier_value{0.0};
  double barrier_gradient_norm{0.0};
  bool converged{false};
  std::string message{};
  std::optional<BestFeasiblePoint> best_feasible{};
  Series hessian{};
};

enum class OptimaAlgorithm : std::uint8_t {
  Penalty = 0,
  SequentialQP,
  InteriorPoint,
};

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

struct OptimaOutput {
  std::string algorithm{};
  std::int64_t iterations{0};
  std::int64_t function_evaluations{0};
  double constraint_violation{0.0};
  double first_order_optimality{0.0};
  double step_size{0.0};
  double line_search_fraction{0.0};
  double backend_parameter{0.0};
  std::int64_t krylov_iterations{0};
  bool best_feasible_available{false};
  std::string detail_message{};
};

struct OptimaOptions {
  OptimaAlgorithm algorithm{OptimaAlgorithm::SequentialQP};
  PenaltyMethodOptions penalty{};
  SequentialQPOptions sequential_qp{};
  InteriorPointOptions interior_point{};
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
  Series jacobian{};
  double residual{0.0};
  bool equality{false};
  ConstraintSource source{ConstraintSource::LinearEquality};
  std::int64_t index{0};
};

struct FlatConstraintSystem {
  std::vector<FlatConstraintRow> equalities{};
  std::vector<FlatConstraintRow> inequalities{};
};

inline Series empty_vec_like_(const ParameterPack<Series>& params,
                              ScalarType dtype = ScalarType::Float64) {
  if (params.empty()) return pluss::empty({0}, dtype);
  return pluss::empty({0}, dtype).to(params[0].value.platform());
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
  if (flat.empty()) return pluss::empty({0}, dtype).to(platform);
  if (flat.size() == 1) return flat.front();
  return flat.front().unite(std::vector<Series>(flat.begin() + 1, flat.end()), 0);
}

inline Series dense_matvec_(const Series& matrix, const Series& vector) {
  require_defined_(matrix, "dense_matvec");
  require_defined_(vector, "dense_matvec");
  if (matrix.sizes().size() != 2) {
    throw std::runtime_error("subsp::math::optim::dense_matvec_: expected a rank-2 matrix");
  }

  const std::int64_t rows = matrix.sizes()[0];
  const std::int64_t cols = matrix.sizes()[1];
  const Series flat_vector = vector.reshape({vector.numel()}).to(ScalarType::Float64);
  if (flat_vector.numel() != cols) {
    throw std::runtime_error("subsp::math::optim::dense_matvec_: matrix/vector shape mismatch");
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
    throw std::runtime_error("subsp::math::optim::dense_transpose_matvec_: expected a rank-2 matrix");
  }

  const std::int64_t rows = matrix.sizes()[0];
  const std::int64_t cols = matrix.sizes()[1];
  const Series flat_vector = vector.reshape({vector.numel()}).to(ScalarType::Float64);
  if (flat_vector.numel() != rows) {
    throw std::runtime_error(
        "subsp::math::optim::dense_transpose_matvec_: matrix/vector shape mismatch");
  }
  return matrix.to(ScalarType::Float64)
      .to(vector.platform())
      .reshape({rows, cols})
      .transpose(0, 1)
      .matmul(flat_vector.to(vector.platform()).reshape({rows, 1}))
      .reshape({cols});
}

inline bool is_finite_series_(const Series& x) {
  require_defined_(x, "is_finite_series");
  return x.isfinite().all().item<bool>();
}

inline double max_abs_scalar_(const Series& x) {
  if (!x.defined() || x.numel() == 0) return 0.0;
  const auto vals = x.to(ScalarType::Float64).abs().reshape({x.numel()}).to_vec<double>();
  double best = 0.0;
  for (double v : vals) best = std::max(best, std::abs(v));
  return best;
}

inline double max_positive_scalar_(const Series& x) {
  if (!x.defined() || x.numel() == 0) return 0.0;
  const auto vals = x.to(ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
  double best = 0.0;
  for (double v : vals) best = std::max(best, v);
  return best;
}

inline ParameterPack<Series> zero_jacobian_pack_(const ParameterPack<Series>& params,
                                                 std::int64_t rows,
                                                 ScalarType dtype = ScalarType::Float64) {
  std::vector<typename ParameterPack<Series>::Block> out;
  out.reserve(params.size());
  for (const auto& block : params.blocks()) {
    require_defined_(block.value, "zero_jacobian_pack");
    out.push_back({block.name, pluss::zeros({rows, block.value.numel()}, dtype).to(block.value.platform())});
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

inline void validate_problem_(const ConstrainedProblem& problem, const ParameterPack<Series>& params) {
  if (!problem.objective.defined()) {
    throw std::runtime_error("subsp::math::optim::evaluate_problem: objective is not defined");
  }
  if (problem.bounds.lower.has_value() && !problem.bounds.lower->same_layout(params)) {
    throw std::runtime_error("subsp::math::optim::evaluate_problem: lower bounds layout mismatch");
  }
  if (problem.bounds.upper.has_value() && !problem.bounds.upper->same_layout(params)) {
    throw std::runtime_error("subsp::math::optim::evaluate_problem: upper bounds layout mismatch");
  }
  if (problem.bounds.lower.has_value() && problem.bounds.upper.has_value()) {
    for (const auto& block : params.blocks()) {
      const Series diff = problem.bounds.lower->at(block.name).sub(problem.bounds.upper->at(block.name));
      if (max_positive_scalar_(diff) > 0.0) {
        throw std::runtime_error("subsp::math::optim::evaluate_problem: lower bound exceeds upper bound");
      }
    }
  }
  const std::int64_t n = params.total_numel();
  if (problem.linear.Aeq.has_value()) {
    require_defined_(*problem.linear.Aeq, "evaluate_problem");
    if (problem.linear.Aeq->sizes().size() != 2 || problem.linear.Aeq->sizes()[1] != n) {
      throw std::runtime_error("subsp::math::optim::evaluate_problem: Aeq shape mismatch");
    }
    if (problem.linear.beq.has_value() && problem.linear.beq->numel() != problem.linear.Aeq->sizes()[0]) {
      throw std::runtime_error("subsp::math::optim::evaluate_problem: beq length mismatch");
    }
  } else if (problem.linear.beq.has_value() && problem.linear.beq->numel() != 0) {
    throw std::runtime_error("subsp::math::optim::evaluate_problem: beq provided without Aeq");
  }
  if (problem.linear.Aineq.has_value()) {
    require_defined_(*problem.linear.Aineq, "evaluate_problem");
    if (problem.linear.Aineq->sizes().size() != 2 || problem.linear.Aineq->sizes()[1] != n) {
      throw std::runtime_error("subsp::math::optim::evaluate_problem: Aineq shape mismatch");
    }
    if (problem.linear.bineq.has_value() && problem.linear.bineq->numel() != problem.linear.Aineq->sizes()[0]) {
      throw std::runtime_error("subsp::math::optim::evaluate_problem: bineq length mismatch");
    }
  } else if (problem.linear.bineq.has_value() && problem.linear.bineq->numel() != 0) {
    throw std::runtime_error("subsp::math::optim::evaluate_problem: bineq provided without Aineq");
  }
}

inline Series evaluate_linear_eq_(const LinearConstraints& linear, const Series& flat_params) {
  if (!linear.Aeq.has_value()) return pluss::empty({0}, flat_params.scalar_type()).to(flat_params.platform());
  Series residual = dense_matvec_(*linear.Aeq, flat_params).to(flat_params.scalar_type());
  if (linear.beq.has_value()) residual = residual.sub(linear.beq->reshape({linear.beq->numel()}));
  return residual;
}

inline Series evaluate_linear_ineq_(const LinearConstraints& linear, const Series& flat_params) {
  if (!linear.Aineq.has_value()) return pluss::empty({0}, flat_params.scalar_type()).to(flat_params.platform());
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
    residuals.push_back(lower.sub(block.value));
  }
  return concat_flat_(residuals, params);
}

inline Series evaluate_upper_bound_residual_(const Bounds& bounds, const ParameterPack<Series>& params) {
  if (!bounds.upper.has_value()) return empty_vec_like_(params);
  std::vector<Series> residuals;
  residuals.reserve(params.size());
  for (const auto& block : params.blocks()) {
    const Series& upper = bounds.upper->at(block.name);
    residuals.push_back(block.value.sub(upper));
  }
  return concat_flat_(residuals, params);
}

inline ParameterPack<Series> lift_flat_gradient_(const ParameterPack<Series>& params, const Series& flat_grad) {
  return unflatten_values(params.layout(), flat_grad.reshape({flat_grad.numel()}));
}

inline ParameterPack<Series> add_pack_(const ParameterPack<Series>& lhs, const ParameterPack<Series>& rhs) {
  if (!lhs.same_layout(rhs)) {
    throw std::runtime_error("subsp::math::optim::add_pack_: layout mismatch");
  }
  std::vector<typename ParameterPack<Series>::Block> out;
  out.reserve(lhs.size());
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    out.push_back({lhs[i].name, lhs[i].value.add(rhs[i].value)});
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

inline std::vector<double> flat_series_to_std_(const Series& x) {
  if (!x.defined() || x.numel() == 0) return {};
  return x.to(ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
}

inline std::vector<double> flat_pack_to_std_(const ParameterPack<Series>& pack) {
  if (pack.empty()) return {};
  return flat_series_to_std_(flatten_values(pack));
}

inline ParameterPack<Series> lift_flat_std_(const ParameterPack<Series>& like,
                                            const std::vector<double>& flat) {
  if (flat.size() != static_cast<std::size_t>(like.total_numel())) {
    throw std::runtime_error("subsp::math::optim::lift_flat_std_: size mismatch");
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
  return pluss::zeros({n}, ScalarType::Float64).to(platform);
}

inline Series identity_matrix_(std::int64_t n,
                               pluss::codem::Platform platform = pluss::codem::Platform::CPU) {
  std::vector<double> out(static_cast<std::size_t>(n * n), 0.0);
  for (std::int64_t i = 0; i < n; ++i) out[static_cast<std::size_t>(i * n + i)] = 1.0;
  return pluss::make_series(std::move(out), {n, n}).to(ScalarType::Float64).to(platform);
}

inline double dot_(const std::vector<double>& a, const std::vector<double>& b) {
  if (a.size() != b.size()) {
    throw std::runtime_error("subsp::math::optim::dot_: size mismatch");
  }
  double out = 0.0;
  for (std::size_t i = 0; i < a.size(); ++i) out += a[i] * b[i];
  return out;
}

inline double inf_norm_(const std::vector<double>& x) {
  double best = 0.0;
  for (double v : x) best = std::max(best, std::abs(v));
  return best;
}

inline double series_inf_norm_(const Series& x) {
  if (!x.defined() || x.numel() == 0) return 0.0;
  return x.to(ScalarType::Float64).abs().reshape({x.numel()}).max().item<double>();
}

inline double series_dot_(const Series& a, const Series& b) {
  return a.to(ScalarType::Float64)
      .reshape({1, a.numel()})
      .matmul(b.to(ScalarType::Float64).reshape({b.numel(), 1}))
      .item<double>();
}

inline Series series_add_scaled_(const Series& a, const Series& b, double scale_b = 1.0) {
  return a.add(b.mul(scale_b));
}

inline Series series_outer_(const Series& a, const Series& b) {
  return a.reshape({a.numel(), 1}).matmul(b.reshape({1, b.numel()}));
}

inline Series solve_dense_system_series_(const Series& matrix,
                                         const Series& rhs,
                                         double regularization,
                                         OptimaLinearSystemSolver linear_system_solver,
                                         bool& ok) {
  ok = false;
  require_defined_(matrix, "solve_dense_system_series");
  require_defined_(rhs, "solve_dense_system_series");
  if (matrix.sizes().size() != 2 || matrix.sizes()[0] != matrix.sizes()[1]) {
    throw std::runtime_error("subsp::math::optim::solve_dense_system_series_: expected square matrix");
  }
  const std::int64_t n = matrix.sizes()[0];
  const auto platform = matrix.platform();
  const Series base = matrix.to(ScalarType::Float64).to(platform);
  const Series rhs2d = rhs.to(ScalarType::Float64).to(platform).reshape({n, 1});
  const Series identity = identity_matrix_(n, platform);
  const double base_regularization = std::max(regularization, 0.0);
  const std::array<double, 5> regularization_scales{1.0, 10.0, 100.0, 1000.0, 1.0e6};

  auto try_cholesky = [&](const Series& candidate, const Series& rhs_col, Series& solution) -> bool {
    try {
      const Series factor = pluss::subsp::math::linalg::cholesky(candidate, false);
      solution = pluss::subsp::math::linalg::cholesky_solve(rhs_col, factor, false).reshape({n});
      return is_finite_series_(solution);
    } catch (...) {
      return false;
    }
  };
  auto try_inverse = [&](const Series& candidate, const Series& rhs_col, Series& solution) -> bool {
    try {
      solution = pluss::subsp::math::linalg::inverse(candidate).matmul(rhs_col).reshape({n});
      return is_finite_series_(solution);
    } catch (...) {
      return false;
    }
  };
  auto try_pinverse = [&](const Series& candidate, const Series& rhs_col, Series& solution) -> bool {
    try {
      solution = pluss::subsp::math::linalg::pinverse(candidate, 1e-12).matmul(rhs_col).reshape({n});
      return is_finite_series_(solution);
    } catch (...) {
      return false;
    }
  };

  for (double scale : regularization_scales) {
    const double shift = std::max(1e-12, base_regularization * scale);
    const Series reg = base.add(identity.mul(shift));
    const Series symmetric_reg = reg.add(reg.transpose(0, 1)).mul(0.5);
    Series solution;

    switch (linear_system_solver) {
      case OptimaLinearSystemSolver::Auto:
        if (try_cholesky(symmetric_reg, rhs2d, solution) ||
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
    }
  }

  if (linear_system_solver != OptimaLinearSystemSolver::PseudoInverse) {
    Series solution;
    if (try_pinverse(base.add(identity.mul(std::max(1e-12, base_regularization))), rhs2d, solution)) {
      ok = true;
      return solution;
    }
  }
  return zeros_series_(n, platform);
}

inline Series stack_rows_series_(const std::vector<Series>& rows,
                                 std::int64_t ncols,
                                 pluss::codem::Platform platform) {
  if (rows.empty()) return pluss::empty({0, ncols}, ScalarType::Float64).to(platform);
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

inline double l2_norm_(const std::vector<double>& x) {
  return std::sqrt(std::max(0.0, dot_(x, x)));
}

inline std::vector<double> add_vec_(const std::vector<double>& a,
                                    const std::vector<double>& b,
                                    double scale_b = 1.0) {
  if (a.size() != b.size()) {
    throw std::runtime_error("subsp::math::optim::add_vec_: size mismatch");
  }
  std::vector<double> out(a.size(), 0.0);
  for (std::size_t i = 0; i < a.size(); ++i) out[i] = a[i] + scale_b * b[i];
  return out;
}

inline std::vector<double> scale_vec_(const std::vector<double>& x, double scale) {
  std::vector<double> out(x.size(), 0.0);
  for (std::size_t i = 0; i < x.size(); ++i) out[i] = scale * x[i];
  return out;
}

inline std::vector<double> matvec_(const std::vector<double>& matrix,
                                   std::size_t rows,
                                   std::size_t cols,
                                   const std::vector<double>& x) {
  if (matrix.size() != rows * cols || x.size() != cols) {
    throw std::runtime_error("subsp::math::optim::matvec_: shape mismatch");
  }
  std::vector<double> out(rows, 0.0);
  for (std::size_t row = 0; row < rows; ++row) {
    double acc = 0.0;
    for (std::size_t col = 0; col < cols; ++col) {
      acc += matrix[row * cols + col] * x[col];
    }
    out[row] = acc;
  }
  return out;
}

inline std::vector<double> solve_linear_system_(std::vector<double> matrix,
                                                std::vector<double> rhs,
                                                std::size_t n,
                                                bool& ok) {
  ok = false;
  if (matrix.size() != n * n || rhs.size() != n) {
    throw std::runtime_error("subsp::math::optim::solve_linear_system_: shape mismatch");
  }

  for (std::size_t k = 0; k < n; ++k) {
    std::size_t pivot = k;
    double pivot_abs = std::abs(matrix[k * n + k]);
    for (std::size_t row = k + 1; row < n; ++row) {
      const double cand = std::abs(matrix[row * n + k]);
      if (cand > pivot_abs) {
        pivot = row;
        pivot_abs = cand;
      }
    }
    if (pivot_abs <= 1e-14) {
      return std::vector<double>(n, 0.0);
    }
    if (pivot != k) {
      for (std::size_t col = k; col < n; ++col) {
        std::swap(matrix[k * n + col], matrix[pivot * n + col]);
      }
      std::swap(rhs[k], rhs[pivot]);
    }

    const double diag = matrix[k * n + k];
    for (std::size_t row = k + 1; row < n; ++row) {
      const double factor = matrix[row * n + k] / diag;
      if (factor == 0.0) continue;
      matrix[row * n + k] = 0.0;
      for (std::size_t col = k + 1; col < n; ++col) {
        matrix[row * n + col] -= factor * matrix[k * n + col];
      }
      rhs[row] -= factor * rhs[k];
    }
  }

  std::vector<double> x(n, 0.0);
  for (std::size_t i = n; i-- > 0;) {
    double acc = rhs[i];
    for (std::size_t col = i + 1; col < n; ++col) {
      acc -= matrix[i * n + col] * x[col];
    }
    const double diag = matrix[i * n + i];
    if (std::abs(diag) <= 1e-14) {
      return std::vector<double>(n, 0.0);
    }
    x[i] = acc / diag;
  }

  ok = true;
  return x;
}

inline Series jacobian_row_from_pack_(const ParameterPack<Series>& params,
                                      const ParameterPack<Series>& jacobian,
                                      std::int64_t row) {
  std::vector<double> out;
  out.reserve(static_cast<std::size_t>(params.total_numel()));
  for (const auto& block : params.blocks()) {
    if (!jacobian.contains(block.name)) {
      out.insert(out.end(), static_cast<std::size_t>(block.value.numel()), 0.0);
      continue;
    }
    const Series& block_jacobian = jacobian.at(block.name);
    const std::int64_t cols = block.value.numel();
    if (block_jacobian.numel() == 0) {
      out.insert(out.end(), static_cast<std::size_t>(cols), 0.0);
      continue;
    }
    if (block_jacobian.sizes().size() != 2 || block_jacobian.sizes()[0] <= row || block_jacobian.sizes()[1] != cols) {
      throw std::runtime_error("subsp::math::optim::jacobian_row_from_pack_: shape mismatch");
    }
    const auto data =
        block_jacobian.to(ScalarType::Float64).reshape({block_jacobian.numel()}).to_vec<double>();
    const std::size_t offset = static_cast<std::size_t>(row * cols);
    out.insert(out.end(), data.begin() + static_cast<std::ptrdiff_t>(offset),
               data.begin() + static_cast<std::ptrdiff_t>(offset + cols));
  }
  return pluss::make_series(std::move(out), {params.total_numel()}).to(ScalarType::Float64);
}

inline FlatConstraintSystem assemble_constraint_system_(const ConstrainedProblem& problem,
                                                        const ParameterPack<Series>& params,
                                                        const ProblemEvaluation& evaluation) {
  FlatConstraintSystem out;
  const std::vector<double> flat_params = flat_pack_to_std_(params);
  const std::int64_t n = static_cast<std::int64_t>(flat_params.size());

  const Series linear_eq = evaluate_linear_eq_(problem.linear, flatten_values(params));
  const Series linear_ineq = evaluate_linear_ineq_(problem.linear, flatten_values(params));

  if (problem.linear.Aeq.has_value()) {
    const auto residuals = flat_series_to_std_(linear_eq);
    const auto Aeq =
        problem.linear.Aeq->to(ScalarType::Float64).reshape({problem.linear.Aeq->numel()}).to_vec<double>();
    const std::int64_t rows = problem.linear.Aeq->sizes()[0];
    for (std::int64_t row = 0; row < rows; ++row) {
      std::vector<double> jac(n, 0.0);
      for (std::int64_t col = 0; col < n; ++col) jac[static_cast<std::size_t>(col)] = Aeq[static_cast<std::size_t>(row * n + col)];
      out.equalities.push_back(
          {pluss::make_series(std::move(jac), {n}).to(ScalarType::Float64),
           residuals[static_cast<std::size_t>(row)],
           true,
           ConstraintSource::LinearEquality,
           row});
    }
  }

  const auto nonlinear_eq = flat_series_to_std_(evaluation.nonlinear_equality_residual);
  for (std::int64_t row = 0; row < evaluation.nonlinear_equality_residual.numel(); ++row) {
    out.equalities.push_back(
        {jacobian_row_from_pack_(params, evaluation.equality_jacobian, row),
         nonlinear_eq[static_cast<std::size_t>(row)],
         true,
         ConstraintSource::NonlinearEquality,
         row});
  }

  if (problem.linear.Aineq.has_value()) {
    const auto residuals = flat_series_to_std_(linear_ineq);
    const auto Aineq =
        problem.linear.Aineq->to(ScalarType::Float64).reshape({problem.linear.Aineq->numel()}).to_vec<double>();
    const std::int64_t rows = problem.linear.Aineq->sizes()[0];
    for (std::int64_t row = 0; row < rows; ++row) {
      std::vector<double> jac(n, 0.0);
      for (std::int64_t col = 0; col < n; ++col) jac[static_cast<std::size_t>(col)] = Aineq[static_cast<std::size_t>(row * n + col)];
      out.inequalities.push_back(
          {pluss::make_series(std::move(jac), {n}).to(ScalarType::Float64),
           residuals[static_cast<std::size_t>(row)],
           false,
           ConstraintSource::LinearInequality,
           row});
    }
  }

  if (problem.bounds.lower.has_value()) {
    std::int64_t offset = 0;
    for (const auto& block : params.blocks()) {
      const auto lower = flat_series_to_std_(problem.bounds.lower->at(block.name));
      const auto value = flat_series_to_std_(block.value);
      for (std::int64_t i = 0; i < block.value.numel(); ++i) {
        std::vector<double> jac(static_cast<std::size_t>(n), 0.0);
        jac[static_cast<std::size_t>(offset + i)] = -1.0;
        out.inequalities.push_back(
            {pluss::make_series(std::move(jac), {n}).to(ScalarType::Float64),
             lower[static_cast<std::size_t>(i)] - value[static_cast<std::size_t>(i)],
             false,
             ConstraintSource::LowerBound,
             offset + i});
      }
      offset += block.value.numel();
    }
  }

  if (problem.bounds.upper.has_value()) {
    std::int64_t offset = 0;
    for (const auto& block : params.blocks()) {
      const auto upper = flat_series_to_std_(problem.bounds.upper->at(block.name));
      const auto value = flat_series_to_std_(block.value);
      for (std::int64_t i = 0; i < block.value.numel(); ++i) {
        std::vector<double> jac(static_cast<std::size_t>(n), 0.0);
        jac[static_cast<std::size_t>(offset + i)] = 1.0;
        out.inequalities.push_back(
            {pluss::make_series(std::move(jac), {n}).to(ScalarType::Float64),
             value[static_cast<std::size_t>(i)] - upper[static_cast<std::size_t>(i)],
             false,
             ConstraintSource::UpperBound,
             offset + i});
      }
      offset += block.value.numel();
    }
  }

  const auto nonlinear_ineq = flat_series_to_std_(evaluation.nonlinear_inequality_residual);
  for (std::int64_t row = 0; row < evaluation.nonlinear_inequality_residual.numel(); ++row) {
    out.inequalities.push_back(
        {jacobian_row_from_pack_(params, evaluation.inequality_jacobian, row),
         nonlinear_ineq[static_cast<std::size_t>(row)],
         false,
         ConstraintSource::NonlinearInequality,
         row});
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

inline void maybe_update_best_feasible_(SequentialQPState& state) {
  if (!state.evaluation.feasible) return;
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
  double margin = std::numeric_limits<double>::infinity();
  for (const auto& row : system.inequalities) {
    margin = std::min(margin, -row.residual);
  }
  return std::isfinite(margin) ? margin : std::numeric_limits<double>::infinity();
}

inline double equality_residual_norm_(const FlatConstraintSystem& system) {
  double sumsq = 0.0;
  for (const auto& row : system.equalities) {
    sumsq += row.residual * row.residual;
  }
  return std::sqrt(std::max(0.0, sumsq));
}

inline double barrier_value_(const ProblemEvaluation& evaluation,
                             const FlatConstraintSystem& system,
                             double barrier_parameter,
                             double equality_penalty) {
  if (!(barrier_parameter > 0.0) || !std::isfinite(barrier_parameter)) {
    throw std::runtime_error("subsp::math::optim::barrier_value_: barrier parameter must be positive");
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

inline std::vector<double> barrier_gradient_flat_(const ProblemEvaluation& evaluation,
                                                  const FlatConstraintSystem& system,
                                                  double barrier_parameter,
                                                  double equality_penalty) {
  Series grad = flatten_values(evaluation.gradient).to(ScalarType::Float64);
  for (const auto& row : system.equalities) {
    grad = grad.add(row.jacobian.mul(equality_penalty * row.residual));
  }
  for (const auto& row : system.inequalities) {
    if (!(row.residual < 0.0)) {
      return std::vector<double>(
          static_cast<std::size_t>(grad.numel()), std::numeric_limits<double>::quiet_NaN());
    }
    const double coeff = -barrier_parameter / row.residual;
    grad = grad.add(row.jacobian.mul(coeff));
  }
  return grad.to_vec<double>();
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
                                                     bool& ok) {
  ok = true;
  if (project_bounds) params = project_bounds_(params, problem.bounds);

  for (std::int64_t iter = 0; iter < 25; ++iter) {
    auto evaluation = evaluate_problem(problem, params);
    ++eval_count;
    if (evaluation.status != EvalStatus::Ok) {
      ok = false;
      return params;
    }
    const auto system = assemble_constraint_system_(problem, params, evaluation);
    if (system.inequalities.empty() || inequality_margin_(system) > margin) {
      return params;
    }

    Series correction = zeros_series_(params.total_numel(),
                                      params.empty() ? pluss::codem::Platform::CPU : params[0].value.platform());
    for (const auto& row : system.inequalities) {
      const double violation = std::max(row.residual + margin, 0.0);
      if (violation == 0.0) continue;
      correction = correction.add(row.jacobian.to(correction.platform()).mul(violation));
    }

    if (series_inf_norm_(correction) <= 1e-12) {
      ok = false;
      return params;
    }

    const Series base_flat = flatten_values(params).to(ScalarType::Float64);
    const Series step_dir = correction.mul(-1.0);
    const double base_violation = merit_violation_(system, margin);
    bool accepted = false;
    double step = 1.0;
    for (std::int64_t ls = 0; ls < 20; ++ls) {
      auto trial = unflatten_values(params.layout(), base_flat.add(step_dir.mul(step)));
      if (project_bounds) trial = project_bounds_(trial, problem.bounds);
      auto trial_eval = evaluate_problem(problem, trial);
      ++eval_count;
      if (trial_eval.status == EvalStatus::Ok) {
        const auto trial_system = assemble_constraint_system_(problem, trial, trial_eval);
        if (merit_violation_(trial_system, margin) < base_violation) {
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

inline void maybe_update_best_feasible_(InteriorPointState& state) {
  if (!state.evaluation.feasible) return;
  if (!state.best_feasible.has_value() ||
      state.evaluation.value.item<double>() <= state.best_feasible->value.item<double>()) {
    state.best_feasible = BestFeasiblePoint{
        state.params,
        state.evaluation.value,
        state.evaluation.max_constraint_violation,
        state.evaluation.first_order_opt};
  }
}

inline Series bfgs_hessian_update_(const Series& hessian,
                                   const Series& s,
                                   const Series& y,
                                                double curvature_tolerance) {
  const std::int64_t n = s.numel();
  if (!hessian.defined() || hessian.numel() != n * n || y.numel() != n) {
    throw std::runtime_error("subsp::math::optim::bfgs_hessian_update_: shape mismatch");
  }

  const double sy = series_dot_(s, y);
  const double s_norm = s.to(ScalarType::Float64).square().sum().sqrt().item<double>();
  const double y_norm = y.to(ScalarType::Float64).square().sum().sqrt().item<double>();
  if (!(sy > curvature_tolerance * std::max(1.0, s_norm * y_norm))) {
    return hessian;
  }

  const Series Bs = hessian.matmul(s.reshape({n, 1})).reshape({n});
  const double sBs = series_dot_(s, Bs);
  if (!(sBs > curvature_tolerance * std::max(1.0, s_norm * s_norm))) {
    return hessian;
  }

  return hessian.sub(series_outer_(Bs, Bs).div(sBs)).add(series_outer_(y, y).div(sy));
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
  }
  return identity_matrix_(n, platform);
}

inline std::pair<Series, Series> solve_sqp_kkt_(
    const Series& hessian,
    const Series& gradient,
    const std::vector<FlatConstraintRow>& active_constraints,
    double regularization,
    OptimaLinearSystemSolver linear_system_solver,
    bool& ok) {
  const std::int64_t n = gradient.numel();
  const std::int64_t m = static_cast<std::int64_t>(active_constraints.size());
  const auto platform = gradient.platform();
  const Series Hreg = hessian.add(identity_matrix_(n, platform).mul(regularization));

  if (m == 0) {
    const Series step =
        solve_dense_system_series_(Hreg, gradient.mul(-1.0), regularization, linear_system_solver, ok);
    return {step, zeros_series_(0, platform)};
  }

  std::vector<Series> rows;
  rows.reserve(active_constraints.size());
  for (const auto& row : active_constraints) rows.push_back(row.jacobian.to(platform));
  const Series A = stack_rows_series_(rows, n, platform);
  const Series top = Hreg.unite({A.transpose(0, 1)}, 1);
  const Series bottom = A.unite({pluss::zeros({m, m}, ScalarType::Float64).to(platform)}, 1);
  const Series KKT = top.unite({bottom}, 0);

  std::vector<double> rhs_tail(static_cast<std::size_t>(m), 0.0);
  for (std::int64_t i = 0; i < m; ++i) rhs_tail[static_cast<std::size_t>(i)] = -active_constraints[static_cast<std::size_t>(i)].residual;
  Series rhs = gradient.mul(-1.0).unite(
      {pluss::make_series(std::move(rhs_tail), {m}).to(ScalarType::Float64).to(platform)},
      0);

  const Series solution = solve_dense_system_series_(KKT, rhs, regularization, linear_system_solver, ok);
  if (!ok) {
    return {zeros_series_(n, platform), zeros_series_(m, platform)};
  }
  return {
      solution.index({pluss::arange(0, n, 1, ScalarType::Int64).to(platform)}).reshape({n}),
      solution.index({pluss::arange(n, n + m, 1, ScalarType::Int64).to(platform)}).reshape({m})};
}

inline double kkt_residual_(const Series& gradient,
                            const std::vector<FlatConstraintRow>& active_constraints,
                            const Series& multipliers) {
  Series lag_grad = gradient.to(ScalarType::Float64);
  double multiplier_violation = 0.0;
  const auto lambda = multipliers.defined() ? multipliers.to_vec<double>() : std::vector<double>{};
  for (std::size_t i = 0; i < active_constraints.size() && i < lambda.size(); ++i) {
    lag_grad = lag_grad.add(active_constraints[i].jacobian.mul(lambda[i]));
    if (!active_constraints[i].equality) {
      multiplier_violation = std::max(multiplier_violation, std::max(0.0, -lambda[i]));
    }
  }
  return std::max(series_inf_norm_(lag_grad), multiplier_violation);
}

inline Series sized_multiplier_series_(std::int64_t n, pluss::codem::Platform platform) {
  return n > 0 ? zeros_series_(n, platform) : pluss::empty({0}, ScalarType::Float64).to(platform);
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

inline ConstraintMultipliers gather_constraint_multipliers_(
    const ConstrainedProblem& problem,
    const ParameterPack<Series>& params,
    const ProblemEvaluation& evaluation,
    const std::vector<FlatConstraintRow>& active_constraints,
    const Series& active_multipliers) {
  ConstraintMultipliers out = zero_constraint_multipliers_(problem, params, evaluation);
  if (!active_multipliers.defined() || active_multipliers.numel() == 0) return out;

  auto linear_eq = out.linear_equalities.to_vec<double>();
  auto nonlinear_eq = out.nonlinear_equalities.to_vec<double>();
  auto linear_ineq = out.linear_inequalities.to_vec<double>();
  auto nonlinear_ineq = out.nonlinear_inequalities.to_vec<double>();
  auto lower = out.lower_bounds.to_vec<double>();
  auto upper = out.upper_bounds.to_vec<double>();
  const auto lambda = active_multipliers.to_vec<double>();

  for (std::size_t i = 0; i < active_constraints.size() && i < lambda.size(); ++i) {
    const auto& row = active_constraints[i];
    const double value = lambda[i];
    switch (row.source) {
      case ConstraintSource::LinearEquality:
        if (row.index >= 0 && static_cast<std::size_t>(row.index) < linear_eq.size()) {
          linear_eq[static_cast<std::size_t>(row.index)] = value;
        }
        break;
      case ConstraintSource::NonlinearEquality:
        if (row.index >= 0 && static_cast<std::size_t>(row.index) < nonlinear_eq.size()) {
          nonlinear_eq[static_cast<std::size_t>(row.index)] = value;
        }
        break;
      case ConstraintSource::LinearInequality:
        if (row.index >= 0 && static_cast<std::size_t>(row.index) < linear_ineq.size()) {
          linear_ineq[static_cast<std::size_t>(row.index)] = std::max(0.0, value);
        }
        break;
      case ConstraintSource::NonlinearInequality:
        if (row.index >= 0 && static_cast<std::size_t>(row.index) < nonlinear_ineq.size()) {
          nonlinear_ineq[static_cast<std::size_t>(row.index)] = std::max(0.0, value);
        }
        break;
      case ConstraintSource::LowerBound:
        if (row.index >= 0 && static_cast<std::size_t>(row.index) < lower.size()) {
          lower[static_cast<std::size_t>(row.index)] = std::max(0.0, value);
        }
        break;
      case ConstraintSource::UpperBound:
        if (row.index >= 0 && static_cast<std::size_t>(row.index) < upper.size()) {
          upper[static_cast<std::size_t>(row.index)] = std::max(0.0, value);
        }
        break;
    }
  }

  const auto platform =
      params.empty() ? pluss::codem::Platform::CPU : params[0].value.platform();
  out.linear_equalities = pluss::make_series(std::move(linear_eq), {out.linear_equalities.numel()})
                              .to(ScalarType::Float64)
                              .to(platform);
  out.nonlinear_equalities =
      pluss::make_series(std::move(nonlinear_eq), {out.nonlinear_equalities.numel()})
          .to(ScalarType::Float64)
          .to(platform);
  out.linear_inequalities = pluss::make_series(std::move(linear_ineq), {out.linear_inequalities.numel()})
                                .to(ScalarType::Float64)
                                .to(platform);
  out.nonlinear_inequalities =
      pluss::make_series(std::move(nonlinear_ineq), {out.nonlinear_inequalities.numel()})
          .to(ScalarType::Float64)
          .to(platform);
  out.lower_bounds = pluss::make_series(std::move(lower), {out.lower_bounds.numel()})
                         .to(ScalarType::Float64)
                         .to(platform);
  out.upper_bounds = pluss::make_series(std::move(upper), {out.upper_bounds.numel()})
                         .to(ScalarType::Float64)
                         .to(platform);
  return out;
}

inline ConstraintMultipliers estimate_barrier_multipliers_(
    const ConstrainedProblem& problem,
    const ParameterPack<Series>& params,
    const ProblemEvaluation& evaluation,
    const FlatConstraintSystem& system,
    double barrier_parameter) {
  ConstraintMultipliers out = zero_constraint_multipliers_(problem, params, evaluation);
  const auto platform =
      params.empty() ? pluss::codem::Platform::CPU : params[0].value.platform();
  auto linear_ineq = out.linear_inequalities.to_vec<double>();
  auto nonlinear_ineq = out.nonlinear_inequalities.to_vec<double>();
  auto lower = out.lower_bounds.to_vec<double>();
  auto upper = out.upper_bounds.to_vec<double>();

  for (const auto& row : system.inequalities) {
    if (!(row.residual < 0.0)) continue;
    const double value = std::max(0.0, barrier_parameter / (-row.residual));
    switch (row.source) {
      case ConstraintSource::LinearInequality:
        if (row.index >= 0 && static_cast<std::size_t>(row.index) < linear_ineq.size()) {
          linear_ineq[static_cast<std::size_t>(row.index)] = value;
        }
        break;
      case ConstraintSource::NonlinearInequality:
        if (row.index >= 0 && static_cast<std::size_t>(row.index) < nonlinear_ineq.size()) {
          nonlinear_ineq[static_cast<std::size_t>(row.index)] = value;
        }
        break;
      case ConstraintSource::LowerBound:
        if (row.index >= 0 && static_cast<std::size_t>(row.index) < lower.size()) {
          lower[static_cast<std::size_t>(row.index)] = value;
        }
        break;
      case ConstraintSource::UpperBound:
        if (row.index >= 0 && static_cast<std::size_t>(row.index) < upper.size()) {
          upper[static_cast<std::size_t>(row.index)] = value;
        }
        break;
      case ConstraintSource::LinearEquality:
      case ConstraintSource::NonlinearEquality:
        break;
    }
  }

  out.linear_inequalities = pluss::make_series(std::move(linear_ineq), {out.linear_inequalities.numel()})
                                .to(ScalarType::Float64)
                                .to(platform);
  out.nonlinear_inequalities =
      pluss::make_series(std::move(nonlinear_ineq), {out.nonlinear_inequalities.numel()})
          .to(ScalarType::Float64)
          .to(platform);
  out.lower_bounds = pluss::make_series(std::move(lower), {out.lower_bounds.numel()})
                         .to(ScalarType::Float64)
                         .to(platform);
  out.upper_bounds = pluss::make_series(std::move(upper), {out.upper_bounds.numel()})
                         .to(ScalarType::Float64)
                         .to(platform);
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
      ok);
  if (!ok) return sized_multiplier_series_(m, platform);
  return lambda;
}

inline const char* algorithm_name_(OptimaAlgorithm algorithm) {
  switch (algorithm) {
    case OptimaAlgorithm::Penalty:
      return "penalty";
    case OptimaAlgorithm::SequentialQP:
      return "sqp";
    case OptimaAlgorithm::InteriorPoint:
      return "interior-point";
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

inline OptimaProgressSnapshot make_progress_snapshot_(OptimaAlgorithm algorithm,
                                                      OptimaProgressStage stage,
                                                      const PenaltyMethodState& state) {
  return OptimaProgressSnapshot{
      stage,
      algorithm_name_(algorithm),
      state.params,
      state.evaluation,
      state.iteration,
      state.function_evaluations,
      state.last_step_size,
      state.current_penalty,
      pack_inf_norm_(state.evaluation.gradient),
      state.message,
      state.best_feasible};
}

inline OptimaProgressSnapshot make_progress_snapshot_(OptimaAlgorithm algorithm,
                                                      OptimaProgressStage stage,
                                                      const SequentialQPState& state) {
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
      state.message,
      state.best_feasible};
}

inline OptimaProgressSnapshot make_progress_snapshot_(OptimaAlgorithm algorithm,
                                                      OptimaProgressStage stage,
                                                      const InteriorPointState& state) {
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
      state.message,
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
    if (message.find("failed to initialize a strict interior point") != std::string::npos) {
      return OptimaExitFlag::NoFeasiblePoint;
    }
    if (message.find("line search failed") != std::string::npos ||
        message.find("step hit the boundary") != std::string::npos) {
      return OptimaExitFlag::NoProgress;
    }
    return OptimaExitFlag::Converged;
  }
  return OptimaExitFlag::Stopped;
}

inline OptimaOutput make_output_(OptimaAlgorithm algorithm,
                                 const ProblemEvaluation& evaluation,
                                 std::int64_t iteration,
                                 std::int64_t function_evaluations,
                                 double last_step_size,
                                 double backend_parameter,
                                 double stationarity,
                                 const std::optional<BestFeasiblePoint>& best_feasible,
                                 const std::string& detail_message,
                                 std::int64_t krylov_iterations = 0) {
  return OptimaOutput{
      algorithm_name_(algorithm),
      iteration,
      function_evaluations,
      evaluation.max_constraint_violation,
      stationarity,
      last_step_size,
      last_step_size,
      backend_parameter,
      krylov_iterations,
      best_feasible.has_value(),
      detail_message};
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
  if (jacobian.empty() || !residual.defined() || residual.numel() == 0) {
    return params.zeros_like();
  }
  const Series r = residual.to(ScalarType::Float64).reshape({residual.numel(), 1});
  std::vector<typename ParameterPack<Series>::Block> out;
  out.reserve(params.size());
  const auto r_values = r.reshape({r.numel()}).to_vec<double>();
  for (const auto& param_block : params.blocks()) {
    Series contribution = pluss::zeros_like(param_block.value, param_block.value.scalar_type());
    if (jacobian.contains(param_block.name)) {
      const Series& block_jacobian = jacobian.at(param_block.name);
      if (block_jacobian.defined() && block_jacobian.numel() > 0) {
        const std::int64_t cols = param_block.value.numel();
        const std::int64_t rows = residual.numel();
        if (block_jacobian.numel() != rows * cols) {
          throw std::runtime_error(
              "subsp::math::optim::combine_constraint_gradient_: jacobian/residual shape mismatch");
        }

        const auto jac_values =
            block_jacobian.to(ScalarType::Float64).reshape({rows * cols}).to_vec<double>();
        std::vector<double> grad_values(static_cast<std::size_t>(cols), 0.0);
        for (std::int64_t row = 0; row < rows; ++row) {
          const double weight = r_values[static_cast<std::size_t>(row)];
          for (std::int64_t col = 0; col < cols; ++col) {
            grad_values[static_cast<std::size_t>(col)] +=
                jac_values[static_cast<std::size_t>(row * cols + col)] * weight;
          }
        }

        contribution =
            pluss::make_series(std::move(grad_values), {cols}).reshape(param_block.value.sizes());
        if (!equality_term) {
          contribution = contribution.to(param_block.value.scalar_type());
        } else {
          contribution = contribution.to(param_block.value.scalar_type());
        }
        if (contribution.platform() != param_block.value.platform()) {
          contribution = contribution.to(param_block.value.platform());
        }
      }
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

inline void maybe_update_best_feasible_(PenaltyMethodState& state) {
  if (!state.evaluation.feasible) return;
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
                                          const ParameterPack<Series>& params) {
  detail::validate_problem_(problem, params);

  ProblemEvaluation out;

  if (problem.objective.analytic) {
    try {
      auto [value, grad_pack] = problem.objective.analytic(params);
      out.value = std::move(value);
      out.gradient = std::move(grad_pack);
    } catch (...) {
      out.status = EvalStatus::InvalidProblem;
      out.message = detail::callback_failure_message_("objective analytic callback");
      return out;
    }
  } else {
    try {
      auto [value, grad_pack] =
          value_and_grad([&](const ParameterPack<Trace>& p) -> Trace { return problem.objective.autodiff(p); }, params);
      out.value = std::move(value);
      out.gradient = std::move(grad_pack);
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
  } else {
    if (problem.nonlinear.equality) {
      try {
        auto [value, jac] = jacrev(
            [&](const ParameterPack<Trace>& p) -> Trace { return problem.nonlinear.equality(p); },
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
        auto [value, jac] = jacrev(
            [&](const ParameterPack<Trace>& p) -> Trace { return problem.nonlinear.inequality(p); },
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
                : pluss::zeros({flat_params.numel()}, ScalarType::Float64).to(flat_params.platform());
  const Series linear_ineq_pos = linear_ineq.numel() == 0 ? linear_ineq : linear_ineq.clamp_min(0.0);
  const Series linear_ineq_grad_flat =
      problem.linear.Aineq.has_value() && linear_ineq_pos.numel() > 0
          ? detail::dense_transpose_matvec_(*problem.linear.Aineq, linear_ineq_pos.to(ScalarType::Float64))
          : pluss::zeros({flat_params.numel()}, ScalarType::Float64).to(flat_params.platform());

  Series bound_grad_flat = pluss::zeros({flat_params.numel()}, ScalarType::Float64).to(flat_params.platform());
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
    auto [autodiff_value, autodiff_gradient] =
        value_and_grad([&](const ParameterPack<Trace>& p) -> Trace { return problem.objective.autodiff(p); }, params);
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
    report.objective.message = "requires both analytic and autodiff objective definitions";
  }

  if (problem.nonlinear.analytic && problem.nonlinear.equality) {
    const auto analytic_eval = problem.nonlinear.analytic(params);
    auto [autodiff_value, autodiff_jacobian] = jacrev(
        [&](const ParameterPack<Trace>& p) -> Trace { return problem.nonlinear.equality(p); },
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
    auto [autodiff_value, autodiff_jacobian] = jacrev(
        [&](const ParameterPack<Trace>& p) -> Trace { return problem.nonlinear.inequality(p); },
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
          : pluss::zeros({flat_params.numel()}, ScalarType::Float64).to(flat_params.platform());
  Series linear_ineq_grad_flat =
      problem.linear.Aineq.has_value() && linear_ineq.numel() > 0
          ? detail::dense_transpose_matvec_(*problem.linear.Aineq, linear_ineq.clamp_min(0.0).to(ScalarType::Float64))
          : pluss::zeros({flat_params.numel()}, ScalarType::Float64).to(flat_params.platform());
  Series bound_grad_flat = pluss::zeros({flat_params.numel()}, ScalarType::Float64).to(flat_params.platform());
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

inline ParameterPack<Series> constraint_violation_gradient_(const ConstrainedProblem& problem,
                                                            const ParameterPack<Series>& params,
                                                            const ProblemEvaluation& evaluation) {
  return add_pack_(
      penalty_gradient(problem, params, evaluation, 1.0),
      scaled_pack_(evaluation.gradient, -1.0));
}

inline RecoveryResult restore_feasibility_(const ConstrainedProblem& problem,
                                           const ParameterPack<Series>& start_params,
                                           const ProblemEvaluation& start_evaluation,
                                           const SequentialQPOptions& options) {
  RecoveryResult result;
  result.params = start_params;
  result.evaluation = start_evaluation;
  result.objective_value = constraint_violation_value_(start_evaluation);
  result.message = "restoration failed to reduce constraint violation";

  if (start_evaluation.status != EvalStatus::Ok) {
    result.message = "restoration requires a valid starting evaluation";
    return result;
  }

  auto current_params = start_params;
  auto current_evaluation = start_evaluation;
  double current_value = result.objective_value;
  const double start_violation = start_evaluation.max_constraint_violation;

  for (std::int64_t iter = 0; iter < options.max_restoration_iterations; ++iter) {
    const auto grad_pack = constraint_violation_gradient_(problem, current_params, current_evaluation);
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
      auto trial = unflatten_values(
          current_params.layout(),
          flatten_values(current_params).to(ScalarType::Float64).add(direction.mul(step_size)));
      if (options.project_bounds) trial = project_bounds_(trial, problem.bounds);

      auto trial_evaluation = evaluate_problem(problem, trial);
      ++result.function_evaluations;
      if (trial_evaluation.status == EvalStatus::Ok) {
        const double trial_value = constraint_violation_value_(trial_evaluation);
        if (std::isfinite(trial_value) &&
            trial_value <= current_value + options.armijo * step_size * directional_derivative) {
          current_params = std::move(trial);
          current_evaluation = std::move(trial_evaluation);
          current_value = trial_value;
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
      result.objective_value + 1e-12 < constraint_violation_value_(start_evaluation) ||
      result.evaluation.max_constraint_violation <= options.constraint_tolerance;
  if (result.success) {
    result.message = "restoration reduced constraint violation";
  }
  return result;
}

inline RecoveryResult recover_barrier_descent_(const ConstrainedProblem& problem,
                                               const ParameterPack<Series>& start_params,
                                               const ProblemEvaluation& start_evaluation,
                                               double barrier_parameter,
                                               const InteriorPointOptions& options) {
  RecoveryResult result;
  result.params = start_params;
  result.evaluation = start_evaluation;

  if (start_evaluation.status != EvalStatus::Ok) {
    result.message = "barrier recovery requires a valid starting evaluation";
    return result;
  }

  auto current_params = start_params;
  auto current_evaluation = start_evaluation;
  auto current_system = assemble_constraint_system_(problem, current_params, current_evaluation);
  if (inequality_margin_(current_system) <= options.initialization_margin) {
    result.message = "barrier recovery requires a strict interior starting point";
    return result;
  }

  double current_value =
      barrier_value_(current_evaluation, current_system, barrier_parameter, options.equality_penalty);
  result.objective_value = current_value;
  result.message = "barrier recovery failed to reduce the barrier objective";
  if (!std::isfinite(current_value)) return result;

  for (std::int64_t iter = 0; iter < options.max_restoration_iterations; ++iter) {
    const auto barrier_gradient =
        barrier_gradient_flat_(current_evaluation, current_system, barrier_parameter, options.equality_penalty);
    const std::int64_t n = current_params.total_numel();
    Series gradient =
        pluss::make_series(barrier_gradient, {n}).to(ScalarType::Float64).to(current_params[0].value.platform());
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
      auto trial = unflatten_values(
          current_params.layout(),
          flatten_values(current_params).to(ScalarType::Float64).add(direction.mul(step_size)));
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
              options.equality_penalty);
          if (std::isfinite(trial_value) &&
              trial_value <= current_value + options.armijo * step_size * directional_derivative) {
            current_params = std::move(trial);
            current_evaluation = std::move(trial_evaluation);
            current_system = std::move(trial_system);
            current_value = trial_value;
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
  result.success = result.iterations > 0 && std::isfinite(result.objective_value) &&
                   result.objective_value + 1e-12 < barrier_value_(
                                                          start_evaluation,
                                                          assemble_constraint_system_(
                                                              problem, start_params, start_evaluation),
                                                          barrier_parameter,
                                                          options.equality_penalty);
  if (result.success) {
    result.message = "barrier recovery reduced the barrier objective";
  }
  return result;
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
  state.penalized_value = state.evaluation.status == EvalStatus::Ok
                              ? detail::penalized_value_(state.evaluation, state.current_penalty)
                              : std::numeric_limits<double>::infinity();
  state.converged = state.evaluation.status != EvalStatus::Ok;
  state.message = state.converged ? state.evaluation.message : "initialized";
  detail::maybe_update_best_feasible_(state);
  return state;
}

inline PenaltyMethodState penalty_method_step(const ConstrainedProblem& problem,
                                              const PenaltyMethodState& state,
                                              const PenaltyMethodOptions& options = {}) {
  PenaltyMethodState next = state;
  if (state.converged || state.evaluation.status != EvalStatus::Ok) return next;

  const auto direction =
      detail::scaled_pack_(penalty_gradient(problem, state.params, state.evaluation, state.current_penalty), -1.0);
  const double grad_norm = detail::pack_inf_norm_(detail::scaled_pack_(direction, -1.0));
  if (grad_norm <= options.optimality_tolerance &&
      state.evaluation.max_constraint_violation <= options.constraint_tolerance) {
    next.converged = true;
    next.message = "first-order and constraint tolerances satisfied";
    return next;
  }

  double step = options.initial_step_size;
  const double base_penalized = state.penalized_value;
  const double descent_measure = grad_norm * grad_norm;

  for (std::int64_t ls_iter = 0; ls_iter < options.max_line_search_iterations; ++ls_iter) {
    std::vector<typename ParameterPack<Series>::Block> trial_blocks;
    trial_blocks.reserve(state.params.size());
    for (std::size_t i = 0; i < state.params.size(); ++i) {
      trial_blocks.push_back({state.params[i].name, state.params[i].value.add(direction[i].value.mul(step)).detach()});
    }
    auto trial = ParameterPack<Series>(std::move(trial_blocks));
    if (options.project_bounds) trial = detail::project_bounds_(trial, problem.bounds);

    auto trial_eval = evaluate_problem(problem, trial);
    next.function_evaluations += 1;
    if (trial_eval.status == EvalStatus::Ok) {
      const double trial_penalized = detail::penalized_value_(trial_eval, state.current_penalty);
      if (std::isfinite(trial_penalized) &&
          trial_penalized <= base_penalized - options.armijo * step * descent_measure) {
        next.params = std::move(trial);
        next.evaluation = std::move(trial_eval);
        next.iteration += 1;
        next.last_step_size = step;
        next.current_penalty = state.current_penalty;
        if (next.evaluation.max_constraint_violation > options.constraint_tolerance) {
          next.current_penalty *= 2.0;
        }
        next.penalized_value = detail::penalized_value_(next.evaluation, next.current_penalty);
        next.message = "step accepted";
        detail::maybe_update_best_feasible_(next);
        if (next.iteration >= options.max_iterations) {
          next.converged = true;
          next.message = "maximum iterations reached";
        }
        if (detail::pack_inf_norm_(penalty_gradient(problem, next.params, next.evaluation, next.current_penalty)) <=
                options.optimality_tolerance &&
            next.evaluation.max_constraint_violation <= options.constraint_tolerance) {
          next.converged = true;
          next.message = "first-order and constraint tolerances satisfied";
        }
        return next;
      }
    }
    step *= options.backtracking;
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
          detail::make_progress_snapshot_(OptimaAlgorithm::Penalty, OptimaProgressStage::Start, state),
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
            detail::make_progress_snapshot_(OptimaAlgorithm::Penalty, OptimaProgressStage::Iteration, state),
            &progress_message)) {
      state.converged = true;
      state.message = progress_message.empty() ? "stopped by progress hook" : progress_message;
      break;
    }
  }
  detail::run_progress_hook_(
      options.progress_hook,
      detail::make_progress_snapshot_(OptimaAlgorithm::Penalty, OptimaProgressStage::Done, state));
  return state;
}

inline SequentialQPState init_sequential_qp_state(const ConstrainedProblem& problem,
                                                  const ParameterPack<Series>& initial_params,
                                                  const SequentialQPOptions& options = {}) {
  SequentialQPState state;
  state.params = options.project_bounds ? detail::project_bounds_(initial_params, problem.bounds) : initial_params;
  state.evaluation = evaluate_problem(problem, state.params);
  state.function_evaluations = 1;
  state.current_penalty = std::max(options.initial_penalty, 1e-8);
  state.merit_value = state.evaluation.status == EvalStatus::Ok
                          ? detail::penalized_value_(state.evaluation, state.current_penalty)
                          : std::numeric_limits<double>::infinity();
  state.kkt_residual = state.evaluation.status == EvalStatus::Ok
                           ? detail::pack_inf_norm_(state.evaluation.gradient)
                           : std::numeric_limits<double>::infinity();
  state.converged = state.evaluation.status != EvalStatus::Ok;
  state.message = state.converged ? state.evaluation.message : "initialized";
  state.hessian = detail::identity_matrix_(
      state.params.total_numel(),
      state.params.empty() ? pluss::codem::Platform::CPU : state.params[0].value.platform());
  detail::maybe_update_best_feasible_(state);
  if (!state.converged && detail::feasible_objective_target_met_(state.evaluation, options.objective_limit)) {
    state.converged = true;
    state.message = "objective limit reached at a feasible point";
  }
  return state;
}

inline SequentialQPState sequential_qp_step(const ConstrainedProblem& problem,
                                            const SequentialQPState& state,
                                            const SequentialQPOptions& options = {}) {
  SequentialQPState next = state;
  if (state.converged || state.evaluation.status != EvalStatus::Ok) return next;

  const auto constraint_system = detail::assemble_constraint_system_(problem, state.params, state.evaluation);
  auto active_constraints =
      detail::active_constraints_(constraint_system, options.active_set_tolerance);
  const Series flat_gradient = flatten_values(state.evaluation.gradient).to(ScalarType::Float64);

  bool kkt_ok = false;
  Series step_direction;
  Series multipliers;
  for (std::size_t attempt = 0; attempt <= active_constraints.size(); ++attempt) {
    std::tie(step_direction, multipliers) = detail::solve_sqp_kkt_(
        state.hessian,
        flat_gradient,
        active_constraints,
        options.regularization,
        options.linear_system_solver,
        kkt_ok);
    if (!kkt_ok) break;

    double most_negative = 0.0;
    std::size_t most_negative_index = active_constraints.size();
    const auto lambda = multipliers.defined() ? multipliers.to_vec<double>() : std::vector<double>{};
    for (std::size_t i = 0; i < active_constraints.size() && i < lambda.size(); ++i) {
      if (!active_constraints[i].equality && lambda[i] < most_negative) {
        most_negative = lambda[i];
        most_negative_index = i;
      }
    }
    if (most_negative_index == active_constraints.size() ||
        most_negative >= -options.active_set_tolerance) {
      break;
    }
    active_constraints.erase(active_constraints.begin() + static_cast<std::ptrdiff_t>(most_negative_index));
  }

  const auto merit_gradient_pack =
      penalty_gradient(problem, state.params, state.evaluation, state.current_penalty);
  const Series merit_gradient = flatten_values(merit_gradient_pack).to(ScalarType::Float64);

  if (!kkt_ok || detail::series_dot_(merit_gradient, step_direction) >= 0.0) {
    step_direction = merit_gradient.mul(-1.0);
    multipliers = detail::zeros_series_(
        static_cast<std::int64_t>(active_constraints.size()),
        merit_gradient.platform());
  }

  const double step_inf_norm = detail::series_inf_norm_(step_direction);
  next.kkt_residual = detail::kkt_residual_(flat_gradient, active_constraints, multipliers);
  if (step_inf_norm <= options.optimality_tolerance) {
    next.converged = true;
    next.message = state.evaluation.max_constraint_violation <= options.constraint_tolerance
                       ? "optimality and feasibility tolerances satisfied"
                       : "constraint violation locally stationary";
    return next;
  }

  double step_size = options.initial_step_size;
  const double base_merit = state.merit_value;
  double directional_derivative = detail::series_dot_(merit_gradient, step_direction);
  if (!(directional_derivative < 0.0)) {
    directional_derivative = -std::max(1.0, detail::series_dot_(step_direction, step_direction));
  }

  for (std::int64_t ls_iter = 0; ls_iter < options.max_line_search_iterations; ++ls_iter) {
    const Series trial_flat =
        flatten_values(state.params).to(ScalarType::Float64).add(step_direction.mul(step_size));
    auto trial = unflatten_values(state.params.layout(), trial_flat);
    if (options.project_bounds) trial = detail::project_bounds_(trial, problem.bounds);

    auto trial_eval = evaluate_problem(problem, trial);
    next.function_evaluations += 1;
    if (trial_eval.status == EvalStatus::Ok) {
      double trial_penalty = state.current_penalty;
      if (trial_eval.max_constraint_violation >
          std::max(options.constraint_tolerance, 0.9 * state.evaluation.max_constraint_violation)) {
        trial_penalty *= 2.0;
      }
      const double trial_merit = detail::penalized_value_(trial_eval, trial_penalty);
      if (std::isfinite(trial_merit) &&
          trial_merit <= base_merit + options.armijo * step_size * directional_derivative) {
        next.params = std::move(trial);
        next.evaluation = std::move(trial_eval);
        next.iteration += 1;
        next.last_step_size = step_size;
        next.current_penalty = trial_penalty;
        next.merit_value = trial_merit;
        next.multipliers = std::move(multipliers);
        next.message = "step accepted";

        const Series x_prev = flatten_values(state.params).to(ScalarType::Float64);
        const Series x_next = flatten_values(next.params).to(ScalarType::Float64);
        const Series s = x_next.sub(x_prev);
        if (state.current_penalty == next.current_penalty) {
          const Series next_merit_gradient =
              flatten_values(penalty_gradient(problem, next.params, next.evaluation, next.current_penalty))
                  .to(ScalarType::Float64);
          const Series y = next_merit_gradient.sub(merit_gradient);
          next.hessian = detail::update_hessian_(
              state.hessian,
              s,
              y,
              options.bfgs_curvature_tolerance,
              options.hessian_approximation,
              next.params.total_numel(),
              next.params[0].value.platform());
        } else {
          next.hessian = detail::identity_matrix_(
              next.params.total_numel(),
              next.params[0].value.platform());
        }

        const auto next_constraints =
            detail::assemble_constraint_system_(problem, next.params, next.evaluation);
        const auto next_active =
            detail::active_constraints_(next_constraints, options.active_set_tolerance);
        const Series next_flat_gradient = flatten_values(next.evaluation.gradient).to(ScalarType::Float64);
        next.kkt_residual = detail::kkt_residual_(next_flat_gradient, next_active, next.multipliers);

        detail::maybe_update_best_feasible_(next);
        if (next.iteration >= options.max_iterations) {
          next.converged = true;
          next.message = "maximum iterations reached";
        }
        if (next.kkt_residual <= options.optimality_tolerance &&
            next.evaluation.max_constraint_violation <= options.constraint_tolerance) {
          next.converged = true;
          next.message = "optimality and feasibility tolerances satisfied";
        }
        if (detail::feasible_objective_target_met_(next.evaluation, options.objective_limit)) {
          next.converged = true;
          next.message = "objective limit reached at a feasible point";
        }
        return next;
      }
    }
    step_size *= options.backtracking;
  }

  if (options.enable_restoration) {
    const auto recovery = detail::restore_feasibility_(problem, state.params, state.evaluation, options);
    next.function_evaluations += recovery.function_evaluations;
    if (recovery.success) {
      next.params = recovery.params;
      next.evaluation = recovery.evaluation;
      next.iteration += 1;
      next.last_step_size = recovery.last_step_size;
      next.current_penalty = state.current_penalty;
      if (next.evaluation.max_constraint_violation >
          std::max(options.constraint_tolerance, 0.5 * state.evaluation.max_constraint_violation)) {
        next.current_penalty *= 2.0;
      }
      next.merit_value = detail::penalized_value_(next.evaluation, next.current_penalty);
      next.hessian = detail::identity_matrix_(
          next.params.total_numel(),
          next.params.empty() ? pluss::codem::Platform::CPU : next.params[0].value.platform());
      next.multipliers = detail::zeros_series_(
          static_cast<std::int64_t>(active_constraints.size()),
          next.hessian.platform());
      const auto next_constraints = detail::assemble_constraint_system_(problem, next.params, next.evaluation);
      const auto next_active =
          detail::active_constraints_(next_constraints, options.active_set_tolerance);
      next.kkt_residual = detail::kkt_residual_(
          flatten_values(next.evaluation.gradient).to(ScalarType::Float64),
          next_active,
          next.multipliers);
      next.message = "restoration step accepted";
      detail::maybe_update_best_feasible_(next);
      if (next.iteration >= options.max_iterations) {
        next.converged = true;
        next.message = "maximum iterations reached";
      }
      if (next.kkt_residual <= options.optimality_tolerance &&
          next.evaluation.max_constraint_violation <= options.constraint_tolerance) {
        next.converged = true;
        next.message = "optimality and feasibility tolerances satisfied";
      }
      if (detail::feasible_objective_target_met_(next.evaluation, options.objective_limit)) {
        next.converged = true;
        next.message = "objective limit reached at a feasible point";
      }
      return next;
    }
  }

  next.converged = true;
  next.message = "line search failed to produce an acceptable SQP step";
  return next;
}

inline SequentialQPState solve_sequential_qp(const ConstrainedProblem& problem,
                                             const ParameterPack<Series>& initial_params,
                                             const SequentialQPOptions& options = {}) {
  SequentialQPState state = init_sequential_qp_state(problem, initial_params, options);
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

inline InteriorPointState init_interior_point_state(const ConstrainedProblem& problem,
                                                    const ParameterPack<Series>& initial_params,
                                                    const InteriorPointOptions& options = {}) {
  InteriorPointState state;
  std::int64_t eval_count = 0;
  bool init_ok = true;
  state.params = detail::make_strictly_interior_(
      problem,
      initial_params,
      options.initialization_margin,
      options.project_bounds,
      eval_count,
      init_ok);
  state.function_evaluations = eval_count;
  state.evaluation = evaluate_problem(problem, state.params);
  state.function_evaluations += 1;
  state.barrier_parameter = std::max(options.initial_barrier_parameter, options.minimum_barrier_parameter);
  if (state.evaluation.status == EvalStatus::Ok) {
    const auto system = detail::assemble_constraint_system_(problem, state.params, state.evaluation);
    state.barrier_value = detail::barrier_value_(
        state.evaluation,
        system,
        state.barrier_parameter,
        options.equality_penalty);
    state.barrier_gradient_norm = detail::inf_norm_(detail::barrier_gradient_flat_(
        state.evaluation,
        system,
        state.barrier_parameter,
        options.equality_penalty));
  } else {
    state.barrier_value = std::numeric_limits<double>::infinity();
    state.barrier_gradient_norm = std::numeric_limits<double>::infinity();
  }
  state.converged = !init_ok || state.evaluation.status != EvalStatus::Ok || !std::isfinite(state.barrier_value);
  state.message = state.converged ? "failed to initialize a strict interior point" : "initialized";
  if (state.evaluation.status != EvalStatus::Ok) state.message = state.evaluation.message;
  state.hessian = detail::identity_matrix_(
      state.params.total_numel(),
      state.params.empty() ? pluss::codem::Platform::CPU : state.params[0].value.platform());
  detail::maybe_update_best_feasible_(state);
  if (!state.converged && detail::feasible_objective_target_met_(state.evaluation, options.objective_limit)) {
    state.converged = true;
    state.message = "objective limit reached at a feasible point";
  }
  return state;
}

inline InteriorPointState interior_point_step(const ConstrainedProblem& problem,
                                              const InteriorPointState& state,
                                              const InteriorPointOptions& options = {}) {
  InteriorPointState next = state;
  if (state.converged || state.evaluation.status != EvalStatus::Ok) return next;

  const auto system = detail::assemble_constraint_system_(problem, state.params, state.evaluation);
  const auto barrier_gradient = detail::barrier_gradient_flat_(
      state.evaluation,
      system,
      state.barrier_parameter,
      options.equality_penalty);
  const double barrier_grad_norm = detail::inf_norm_(barrier_gradient);
  next.barrier_gradient_norm = barrier_grad_norm;

  if (barrier_grad_norm <= options.optimality_tolerance) {
    next.converged = true;
    next.message = (state.evaluation.max_constraint_violation <= options.constraint_tolerance &&
                    state.barrier_parameter <= options.minimum_barrier_parameter)
                       ? "barrier optimality and feasibility tolerances satisfied"
                       : "constraint violation locally stationary";
    return next;
  }

  bool solve_ok = false;
  const std::int64_t n = state.params.total_numel();
  Series barrier_gradient_series =
      pluss::make_series(barrier_gradient, {n}).to(ScalarType::Float64).to(state.hessian.platform());
  Series step_direction = detail::solve_dense_system_series_(
      state.hessian,
      barrier_gradient_series.mul(-1.0),
      1e-8,
      options.linear_system_solver,
      solve_ok);
  if (!solve_ok) {
    step_direction = barrier_gradient_series.mul(-1.0);
  }
  if (detail::series_dot_(barrier_gradient_series, step_direction) >= 0.0) {
    step_direction = barrier_gradient_series.mul(-1.0);
  }

  double max_step = options.initial_step_size;
  for (const auto& row : system.inequalities) {
    const double directional = detail::series_dot_(row.jacobian.to(step_direction.platform()), step_direction);
    if (directional > 0.0) {
      max_step = std::min(max_step, options.fraction_to_boundary * (-row.residual) / directional);
    }
  }
  if (!(max_step > 0.0) || !std::isfinite(max_step)) {
    next.converged = true;
    next.message = "interior-point step hit the boundary";
    return next;
  }

  double step_size = max_step;
  const double base_value = state.barrier_value;
  double directional_derivative = detail::series_dot_(barrier_gradient_series, step_direction);
  if (!(directional_derivative < 0.0)) {
    directional_derivative = -std::max(1.0, detail::series_dot_(step_direction, step_direction));
  }

  for (std::int64_t ls_iter = 0; ls_iter < options.max_line_search_iterations; ++ls_iter) {
    auto trial = unflatten_values(
        state.params.layout(),
        flatten_values(state.params).to(ScalarType::Float64).add(step_direction.mul(step_size)));
    if (options.project_bounds) trial = detail::project_bounds_(trial, problem.bounds);

    auto trial_eval = evaluate_problem(problem, trial);
    next.function_evaluations += 1;
    if (trial_eval.status == EvalStatus::Ok) {
      const auto trial_system = detail::assemble_constraint_system_(problem, trial, trial_eval);
      const double trial_barrier = detail::barrier_value_(
          trial_eval,
          trial_system,
          state.barrier_parameter,
          options.equality_penalty);
      if (std::isfinite(trial_barrier) &&
          trial_barrier <= base_value + options.armijo * step_size * directional_derivative) {
        next.params = std::move(trial);
        next.evaluation = std::move(trial_eval);
        next.iteration += 1;
        next.last_step_size = step_size;
        next.barrier_parameter = state.barrier_parameter;
        next.barrier_value = trial_barrier;
        next.message = "step accepted";

        const Series x_prev = flatten_values(state.params).to(ScalarType::Float64);
        const Series x_next = flatten_values(next.params).to(ScalarType::Float64);
        const Series s = x_next.sub(x_prev);
        const auto next_system = detail::assemble_constraint_system_(problem, next.params, next.evaluation);
        const auto next_barrier_gradient = detail::barrier_gradient_flat_(
            next.evaluation,
            next_system,
            next.barrier_parameter,
            options.equality_penalty);
        const Series next_barrier_gradient_series =
            pluss::make_series(next_barrier_gradient, {n}).to(ScalarType::Float64).to(state.hessian.platform());
        const Series y = next_barrier_gradient_series.sub(barrier_gradient_series);
        next.hessian = detail::update_hessian_(
            state.hessian,
            s,
            y,
            options.bfgs_curvature_tolerance,
            options.hessian_approximation,
            next.params.total_numel(),
            next.params[0].value.platform());
        next.barrier_gradient_norm = detail::series_inf_norm_(next_barrier_gradient_series);

        if (next.barrier_gradient_norm <= std::max(10.0 * options.optimality_tolerance, next.barrier_parameter) &&
            next.evaluation.max_constraint_violation <=
                std::max(options.constraint_tolerance, 10.0 * next.barrier_parameter) &&
            next.barrier_parameter > options.minimum_barrier_parameter) {
          next.barrier_parameter =
              std::max(options.minimum_barrier_parameter, next.barrier_parameter * options.barrier_decay);
          next.hessian = detail::identity_matrix_(
              next.params.total_numel(),
              next.params[0].value.platform());
          const auto reduced_gradient = detail::barrier_gradient_flat_(
              next.evaluation,
              next_system,
              next.barrier_parameter,
              options.equality_penalty);
          next.barrier_value = detail::barrier_value_(
              next.evaluation,
              next_system,
              next.barrier_parameter,
              options.equality_penalty);
          next.barrier_gradient_norm = detail::inf_norm_(reduced_gradient);
          next.message = "barrier reduced";
        }

        detail::maybe_update_best_feasible_(next);
        if (next.iteration >= options.max_iterations) {
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
        return next;
      }
    }
    step_size *= options.backtracking;
  }

  if (options.enable_restoration) {
    const auto recovery = detail::recover_barrier_descent_(
        problem,
        state.params,
        state.evaluation,
        state.barrier_parameter,
        options);
    next.function_evaluations += recovery.function_evaluations;
    if (recovery.success) {
      next.params = recovery.params;
      next.evaluation = recovery.evaluation;
      next.iteration += 1;
      next.last_step_size = recovery.last_step_size;
      next.barrier_parameter = state.barrier_parameter;
      const auto next_system = detail::assemble_constraint_system_(problem, next.params, next.evaluation);
      next.barrier_value = detail::barrier_value_(
          next.evaluation,
          next_system,
          next.barrier_parameter,
          options.equality_penalty);
      const auto next_barrier_gradient = detail::barrier_gradient_flat_(
          next.evaluation,
          next_system,
          next.barrier_parameter,
          options.equality_penalty);
      next.barrier_gradient_norm = detail::inf_norm_(next_barrier_gradient);
      next.hessian = detail::identity_matrix_(
          next.params.total_numel(),
          next.params.empty() ? pluss::codem::Platform::CPU : next.params[0].value.platform());
      next.message = "recovery step accepted";
      detail::maybe_update_best_feasible_(next);
      if (next.iteration >= options.max_iterations) {
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
      return next;
    }
  }

  next.converged = true;
  next.message = "line search failed to produce an acceptable interior-point step";
  return next;
}

inline InteriorPointState solve_interior_point(const ConstrainedProblem& problem,
                                               const ParameterPack<Series>& initial_params,
                                               const InteriorPointOptions& options = {}) {
  InteriorPointState state = init_interior_point_state(problem, initial_params, options);
  std::string progress_message;
  if (detail::run_progress_hook_(
          options.progress_hook,
          detail::make_progress_snapshot_(OptimaAlgorithm::InteriorPoint, OptimaProgressStage::Start, state),
          &progress_message)) {
    state.converged = true;
    state.message = progress_message.empty() ? "stopped by progress hook" : progress_message;
  }
  while (!state.converged && state.iteration < options.max_iterations) {
    InteriorPointState next = interior_point_step(problem, state, options);
    if (next.iteration == state.iteration && next.message == state.message &&
        next.converged == state.converged) {
      break;
    }
    state = std::move(next);
    progress_message.clear();
    if (detail::run_progress_hook_(
            options.progress_hook,
            detail::make_progress_snapshot_(OptimaAlgorithm::InteriorPoint, OptimaProgressStage::Iteration, state),
            &progress_message)) {
      state.converged = true;
      state.message = progress_message.empty() ? "stopped by progress hook" : progress_message;
      break;
    }
  }
  detail::run_progress_hook_(
      options.progress_hook,
      detail::make_progress_snapshot_(OptimaAlgorithm::InteriorPoint, OptimaProgressStage::Done, state));
  return state;
}

inline OptimaState to_optima_state(const ConstrainedProblem& problem, const PenaltyMethodState& state) {
  OptimaState out{
      OptimaAlgorithm::Penalty,
      state.params,
      state.evaluation,
      state.iteration,
      state.function_evaluations,
      state.last_step_size,
      state.current_penalty,
      detail::pack_inf_norm_(state.evaluation.gradient),
      state.converged,
      detail::exit_flag_from_state_(state.converged, state.message, state.evaluation.status),
      state.message,
      {},
      detail::zero_constraint_multipliers_(problem, state.params, state.evaluation),
      state.best_feasible};
  out.output = detail::make_output_(
      out.algorithm,
      out.evaluation,
      out.iteration,
      out.function_evaluations,
      out.last_step_size,
      out.backend_parameter,
      out.stationarity,
      out.best_feasible,
      out.message);
  return out;
}

inline OptimaState to_optima_state(const ConstrainedProblem& problem, const SequentialQPState& state) {
  OptimaState out{
      OptimaAlgorithm::SequentialQP,
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
  } else {
    out.multipliers = detail::zero_constraint_multipliers_(problem, state.params, state.evaluation);
  }
  out.output = detail::make_output_(
      out.algorithm,
      out.evaluation,
      out.iteration,
      out.function_evaluations,
      out.last_step_size,
      out.backend_parameter,
      out.stationarity,
      out.best_feasible,
      out.message);
  return out;
}

inline OptimaState to_optima_state(const ConstrainedProblem& problem, const InteriorPointState& state) {
  OptimaState out{
      OptimaAlgorithm::InteriorPoint,
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
    out.multipliers = detail::estimate_barrier_multipliers_(
        problem, state.params, state.evaluation, system, state.barrier_parameter);
  } else {
    out.multipliers = detail::zero_constraint_multipliers_(problem, state.params, state.evaluation);
  }
  out.output = detail::make_output_(
      out.algorithm,
      out.evaluation,
      out.iteration,
      out.function_evaluations,
      out.last_step_size,
      out.backend_parameter,
      out.stationarity,
      out.best_feasible,
      out.message,
      0);
  return out;
}

inline OptimaState optima(const ConstrainedProblem& problem,
                          const ParameterPack<Series>& initial_params,
                          const OptimaOptions& options = {}) {
  switch (options.algorithm) {
    case OptimaAlgorithm::Penalty: {
      auto penalty_options = options.penalty;
      if (!penalty_options.progress_hook) penalty_options.progress_hook = options.progress_hook;
      return to_optima_state(problem, solve_penalty_method(problem, initial_params, penalty_options));
    }
    case OptimaAlgorithm::SequentialQP: {
      auto sequential_qp_options = options.sequential_qp;
      if (!sequential_qp_options.progress_hook) sequential_qp_options.progress_hook = options.progress_hook;
      return to_optima_state(problem, solve_sequential_qp(problem, initial_params, sequential_qp_options));
    }
    case OptimaAlgorithm::InteriorPoint: {
      auto interior_point_options = options.interior_point;
      if (!interior_point_options.progress_hook) interior_point_options.progress_hook = options.progress_hook;
      return to_optima_state(problem, solve_interior_point(problem, initial_params, interior_point_options));
    }
  }

  throw std::runtime_error("subsp::math::optim::optima: unknown algorithm");
}

struct AdamOptions {
  double learning_rate{1e-3};
  double beta1{0.9};
  double beta2{0.999};
  double eps{1e-8};
  double weight_decay{0.0};
};

struct AdamState {
  std::vector<Series> m{};
  std::vector<Series> v{};
  std::int64_t step{0};
};

inline AdamState adam_state_like(const std::vector<Series>& params) {
  AdamState st;
  st.m.reserve(params.size());
  st.v.reserve(params.size());
  for (const auto& p : params) {
    st.m.push_back(pluss::zeros_like(p, p.scalar_type()));
    st.v.push_back(pluss::zeros_like(p, p.scalar_type()));
  }
  return st;
}

inline void adam_step_inplace(std::vector<Series>& params,
                              const std::vector<Series>& sensitivities,
                              AdamState& state,
                              const AdamOptions& opt = {}) {
  if (params.size() != sensitivities.size()) {
    throw std::runtime_error("subsp::math::optim::adam_step_inplace: params/sensitivities size mismatch");
  }
  if (state.m.size() != params.size() || state.v.size() != params.size()) {
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
    detail::require_float_dtype_(params[i], "adam_step_inplace");

    // Keep optimizer updates outside autodiff recording.
    Series p = params[i].detach();
    Series g = sensitivities[i];
    if (!detail::same_shape_(p, g)) {
      if (p.numel() == 1 && g.numel() == 1) {
        g = g.sum();
      } else {
        throw std::runtime_error("subsp::math::optim::adam_step_inplace: sensitivity shape mismatch");
      }
    }

    if (opt.weight_decay != 0.0) {
      g = g.add(p.mul(opt.weight_decay));
    }

    state.m[i] = state.m[i].mul(opt.beta1).add(g.mul(1.0 - opt.beta1));
    state.v[i] = state.v[i].mul(opt.beta2).add(g.mul(g).mul(1.0 - opt.beta2));

    const Series mhat = state.m[i].div(b1_correction);
    const Series vhat = state.v[i].div(b2_correction);
    const Series update = mhat.div(vhat.sqrt().add(opt.eps)).mul(opt.learning_rate);
    params[i] = p.sub(update);
  }
}

inline std::vector<Series> adam_step(std::vector<Series> params,
                                     const std::vector<Series>& sensitivities,
                                     AdamState& state,
                                     const AdamOptions& opt = {}) {
  adam_step_inplace(params, sensitivities, state, opt);
  return params;
}

} // namespace pluss::subsp::math::optim

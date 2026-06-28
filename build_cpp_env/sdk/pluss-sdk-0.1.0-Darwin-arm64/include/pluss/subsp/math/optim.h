#pragma once

#include "pluss/subsp/math/diff.h"
#include "pluss/subsp/math/linalg.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <limits>
#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
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
    ::pluss::throw_error(std::string("subsp::math::optim::") + op + ": expected defined Series");
  }
}

inline void require_same_platform_(const Series& a, const Series& b, const char* op) {
  if (a.platform() != b.platform()) {
    ::pluss::throw_error(std::string("subsp::math::optim::") + op + ": expected matching platforms");
  }
}

inline void require_float_dtype_(const Series& x, const char* op) {
  if (!x.dtype().is_floating() && !x.dtype().is_complex()) {
    ::pluss::throw_error(std::string("subsp::math::optim::") + op +
                             ": expected floating or complex dtype");
  }
}

} // namespace detail

// Compatibility re-export: existing callers can continue to use the older
// optim-facing trace helpers while new derivative-oriented code targets
// pluss::subsp::math::diff directly.
using pluss::subsp::math::diff::Trace;
using pluss::subsp::math::diff::DualTrace;
using pluss::subsp::math::diff::HvpOptions;
using GradientAuditOptions = pluss::subsp::math::diff::DerivativeAuditOptions;
using CurvatureAuditOptions = pluss::subsp::math::diff::DerivativeAuditOptions;
using GradientAuditReport = pluss::subsp::math::diff::DerivativeAuditReport;
using CurvatureAuditReport = pluss::subsp::math::diff::DerivativeAuditReport;
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
using pluss::subsp::math::diff::gradient_audit;
using pluss::subsp::math::diff::curvature_audit;
using pluss::subsp::math::diff::gradient_audit_report;
using pluss::subsp::math::diff::curvature_audit_report;
using pluss::subsp::math::diff::linearize;
using pluss::subsp::math::diff::nth_diff;
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
    ::pluss::throw_error("subsp::math::optim::ParameterLayout::at: unknown parameter block");
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
    ::pluss::throw_error("subsp::math::optim::ParameterPack::at: unknown parameter block");
  }

  T& at(std::string_view name) {
    for (auto& block : blocks_) {
      if (block.name == name) return block.value;
    }
    ::pluss::throw_error("subsp::math::optim::ParameterPack::at: unknown parameter block");
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
      ::pluss::throw_error("subsp::math::optim::ParameterPack::rebind: size mismatch");
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
      ::pluss::throw_error("subsp::math::optim::ParameterPack::flatten_values: empty pack");
    }

    std::vector<Series> flat;
    flat.reserve(blocks_.size());
    const auto platform = blocks_.front().value.platform();
    const auto scalar_type = blocks_.front().value.scalar_type();
    for (const auto& block : blocks_) {
      detail::require_defined_(block.value, "ParameterPack::flatten_values");
      detail::require_same_platform_(blocks_.front().value, block.value, "ParameterPack::flatten_values");
      if (block.value.scalar_type() != scalar_type) {
        ::pluss::throw_error(
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
        ::pluss::throw_error("subsp::math::optim::ParameterPack: block names must be non-empty");
      }
      if (!seen.insert(block.name).second) {
        ::pluss::throw_error("subsp::math::optim::ParameterPack: duplicate block name");
      }
    }
  }

  std::vector<Block> blocks_{};
};

template <class Fn>
std::pair<Series, ParameterPack<Series>> value_and_grad(Fn&& fn, const ParameterPack<Series>& params) {
  pluss::subsp::math::diff::DerivativePolicy policy{};
  policy.warn_on_disconnected = false;
  policy.keep_graph = false;
  auto [value, grads] = pluss::subsp::math::diff::value_and_grad(
      [&](const std::vector<Trace>& xs) -> Trace {
        return std::forward<Fn>(fn)(params.template rebind<Trace>(xs));
      },
      params.values(),
      policy);
  return {value, params.template rebind<Series>(std::move(grads))};
}

inline ParameterPack<Series> unflatten_values(const ParameterLayout& layout, const Series& flat_values) {
  detail::require_defined_(flat_values, "unflatten_values");

  Series flat = flat_values.reshape({flat_values.numel()});
  if (flat.numel() != layout.total_numel()) {
    ::pluss::throw_error("subsp::math::optim::unflatten_values: flat size mismatch");
  }

  std::vector<typename ParameterPack<Series>::Block> blocks;
  blocks.reserve(layout.size());

  std::int64_t offset = 0;
  for (const auto& block : layout.blocks()) {
    Series piece = flat.slice(0, offset, offset + block.numel, 1);
    if (!piece.is_contiguous()) piece = piece.contiguous();
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

inline Trace flatten_values(const ParameterPack<Trace>& params) {
  if (params.empty()) {
    ::pluss::throw_error("subsp::math::optim::flatten_values<Trace>: empty pack");
  }
  std::vector<Series> flat;
  flat.reserve(params.size());
  for (const auto& block : params.blocks()) {
    flat.push_back(block.value.value().reshape({block.value.value().numel()}));
  }
  Series out = flat.size() == 1 ? flat.front() : flat.front().unite(std::vector<Series>(flat.begin() + 1, flat.end()), 0);
  return Trace(out);
}

inline DualTrace flatten_values(const ParameterPack<DualTrace>& params) {
  if (params.empty()) {
    ::pluss::throw_error("subsp::math::optim::flatten_values<DualTrace>: empty pack");
  }
  std::vector<Series> values;
  std::vector<Series> tangents;
  values.reserve(params.size());
  tangents.reserve(params.size());
  for (const auto& block : params.blocks()) {
    values.push_back(block.value.value().reshape({block.value.value().numel()}));
    tangents.push_back(block.value.tangent().reshape({block.value.tangent().numel()}));
  }
  Series value = values.size() == 1 ? values.front() : values.front().unite(std::vector<Series>(values.begin() + 1, values.end()), 0);
  Series tangent =
      tangents.size() == 1 ? tangents.front() : tangents.front().unite(std::vector<Series>(tangents.begin() + 1, tangents.end()), 0);
  return DualTrace(value, tangent);
}

inline Trace reshape_autodiff_(const Trace& value, const std::vector<std::int64_t>& sizes) {
  return Trace(value.value().reshape(sizes));
}

inline DualTrace reshape_autodiff_(const DualTrace& value, const std::vector<std::int64_t>& sizes) {
  return DualTrace(value.value().reshape(sizes), value.tangent().reshape(sizes));
}

inline Trace constant_autodiff_(const Trace& like, const Series& value) {
  return pluss::subsp::math::diff::constant_like(like, value.to(like.value().platform()));
}

inline DualTrace constant_autodiff_(const DualTrace& like, const Series& value) {
  return pluss::subsp::math::diff::constant_like(like, value.to(like.value().platform()));
}

template <class Fn>
ParameterPack<Series> grad(Fn&& fn, const ParameterPack<Series>& params) {
  return value_and_grad(std::forward<Fn>(fn), params).second;
}

template <class Fn>
std::pair<Series, ParameterPack<Series>> jacrev(Fn&& fn, const ParameterPack<Series>& params) {
  pluss::subsp::math::diff::DerivativePolicy policy{};
  policy.warn_on_disconnected = false;
  policy.keep_graph = false;
  auto [value, jacobians] = pluss::subsp::math::diff::jacrev(
      [&](const std::vector<Trace>& xs) -> Trace {
        return std::forward<Fn>(fn)(params.template rebind<Trace>(xs));
      },
      params.values(),
      policy);
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
  std::function<DualTrace(const ParameterPack<DualTrace>&)> autodiff_dual{};
  std::function<std::pair<Series, ParameterPack<Series>>(const ParameterPack<Series>&)> analytic{};

  bool defined() const noexcept {
    return static_cast<bool>(autodiff) || static_cast<bool>(autodiff_dual) || static_cast<bool>(analytic);
  }
};

struct NonlinearConstraints {
  std::function<Trace(const ParameterPack<Trace>&)> equality{};
  std::function<Trace(const ParameterPack<Trace>&)> inequality{};
  std::function<DualTrace(const ParameterPack<DualTrace>&)> equality_dual{};
  std::function<DualTrace(const ParameterPack<DualTrace>&)> inequality_dual{};
  std::function<NonlinearConstraintEvaluation(const ParameterPack<Series>&)> analytic{};

  bool empty() const noexcept {
    return !static_cast<bool>(equality) && !static_cast<bool>(inequality) &&
           !static_cast<bool>(equality_dual) && !static_cast<bool>(inequality_dual) &&
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
  requires(std::invocable<Fn&, const ParameterPack<Trace>&> &&
           std::convertible_to<std::invoke_result_t<Fn&, const ParameterPack<Trace>&>, Trace>)
  ProblemBuilder& objective(Fn&& fn) {
    problem_.objective.autodiff = std::forward<Fn>(fn);
    return *this;
  }

  template <class Fn>
  requires(!std::invocable<Fn&, const ParameterPack<Trace>&> &&
           std::invocable<Fn&, const Trace&> &&
           std::convertible_to<std::invoke_result_t<Fn&, const Trace&>, Trace>)
  ProblemBuilder& objective(Fn&& fn) {
    problem_.objective.autodiff = [f = std::forward<Fn>(fn)](const ParameterPack<Trace>& p) -> Trace {
      return f(p.at("x"));
    };
    return *this;
  }

  template <class Fn>
  requires(!std::invocable<Fn&, const ParameterPack<Trace>&> &&
           !std::invocable<Fn&, const Trace&> &&
           std::invocable<Fn&, const Series&> &&
           std::convertible_to<std::invoke_result_t<Fn&, const Series&>, Trace>)
  ProblemBuilder& objective(Fn&& fn) {
    problem_.objective.autodiff = [f = std::forward<Fn>(fn)](const ParameterPack<Trace>& p) -> Trace {
      return f(p.at("x").value());
    };
    return *this;
  }

  template <class Fn>
  requires(!std::invocable<Fn&, const ParameterPack<Trace>&> &&
           !std::invocable<Fn&, const Trace&> &&
           std::invocable<Fn&, const Series&> &&
           std::convertible_to<std::invoke_result_t<Fn&, const Series&>, Series> &&
           !std::convertible_to<std::invoke_result_t<Fn&, const Series&>, Trace>)
  ProblemBuilder& objective(Fn&& fn) {
    problem_.objective.autodiff = [f = std::forward<Fn>(fn)](const ParameterPack<Trace>& p) -> Trace {
      return trace(f(p.at("x").value()));
    };
    return *this;
  }

  template <class Fn>
  ProblemBuilder& objective_dual(Fn&& fn) {
    problem_.objective.autodiff_dual = std::forward<Fn>(fn);
    return *this;
  }

  ProblemBuilder& objective_analytic(
      std::function<std::pair<Series, ParameterPack<Series>>(const ParameterPack<Series>&)> fn) {
    problem_.objective.analytic = std::move(fn);
    return *this;
  }

  template <class Fn>
  ProblemBuilder& nonlinear_equality(Fn&& fn) {
    problem_.nonlinear.equality = std::forward<Fn>(fn);
    return *this;
  }

  template <class Fn>
  ProblemBuilder& nonlinear_inequality(Fn&& fn) {
    problem_.nonlinear.inequality = std::forward<Fn>(fn);
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
    constraints_.nonlinear.equality = std::forward<Fn>(fn);
    return *this;
  }

  template <class Fn>
  ObjectiveConstraintsBuilder& nonlinear_inequality(Fn&& fn) {
    constraints_.nonlinear.inequality = std::forward<Fn>(fn);
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

enum class OptimaProblemScaling : std::uint8_t {
  None = 0,
  InitialPoint,
  ObjectiveAndConstraints,
};

struct OptimaNormalizationStats {
  double objective_scale{1.0};
  Series equality_row_scale{};
  Series inequality_row_scale{};
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
  double regularization{1e-6};
  double bfgs_curvature_tolerance{1e-8};
  double second_order_correction_tolerance{1e-4};
  double fraction_to_boundary{0.995};
  double pivot_threshold{1e-12};
  std::int64_t max_krylov_iterations{0};
  std::int64_t iterative_dimension_threshold{16};
  double krylov_tolerance{1e-2};
  double krylov_absolute_tolerance{1e-10};
  double objective_limit{-std::numeric_limits<double>::infinity()};
  bool project_bounds{true};
  bool enable_restoration{true};
  OptimaLinearSystemStructure system_structure{OptimaLinearSystemStructure::Auto};
  OptimaLinearSystemSolver linear_system_solver{OptimaLinearSystemSolver::Auto};
  OptimaHessianApproximation hessian_approximation{OptimaHessianApproximation::Bfgs};
  OptimaProblemScaling problem_scaling{OptimaProblemScaling::None};
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
  Series variable_scale{};
  OptimaNormalizationStats normalization{};
  std::int64_t krylov_iterations{0};
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
  double centrality_fraction{0.1};
  double dual_recentering_fraction{0.99};
  double feasibility_activation_factor{10.0};
  double insufficient_infeas_decrease{0.01};
  double sufficient_infeas_decrease{0.9};
  double complementarity_reject_factor{2.0};
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
  std::int64_t iterative_dimension_threshold{16};
  double krylov_tolerance{1e-2};
  double krylov_absolute_tolerance{1e-10};
  pluss::subsp::math::diff::HvpOptions hvp_options{};
  double objective_limit{-std::numeric_limits<double>::infinity()};
  bool project_bounds{true};
  bool enable_restoration{true};
  bool enable_feasibility_phase{false};
  bool enable_nonfinite_recovery{true};
  OptimaLinearSystemStructure system_structure{OptimaLinearSystemStructure::Auto};
  OptimaLinearSystemSolver linear_system_solver{OptimaLinearSystemSolver::Auto};
  OptimaHessianApproximation hessian_approximation{OptimaHessianApproximation::Bfgs};
  OptimaProblemScaling problem_scaling{OptimaProblemScaling::None};
  std::function<bool(const OptimaProgressSnapshot&)> progress_hook{};
};

struct InternalProtocolState {
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
  Series variable_scale{};
  OptimaNormalizationStats normalization{};
  std::int64_t krylov_iterations{0};
  std::vector<Series> lbfgs_s_history{};
  std::vector<Series> lbfgs_y_history{};
  Series inequality_slack{};
  Series inequality_dual{};
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
};

enum class OptimaAlgorithm : std::uint8_t {
  MeritMethod = 0,
  SequentialQP,
  WorkingSet,
  InternalProtocol,
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
  options.internal_protocol.enable_feasibility_phase = true;
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
  options.internal_protocol.enable_feasibility_phase = true;
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

inline std::function<bool(const OptimaProgressSnapshot&)> iter(std::ostream& os,
                                                               int precision = 6) {
  return iteration_only_trace(os, precision);
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
  Series jacobian{};
  double residual{0.0};
  bool equality{false};
  ConstraintSource source{ConstraintSource::LinearEquality};
  std::int64_t index{0};
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
    ::pluss::throw_error("subsp::math::optim::dense_matvec_: expected a rank-2 matrix");
  }

  const std::int64_t rows = matrix.sizes()[0];
  const std::int64_t cols = matrix.sizes()[1];
  const Series flat_vector = vector.reshape({vector.numel()}).to(ScalarType::Float64);
  if (flat_vector.numel() != cols) {
    ::pluss::throw_error("subsp::math::optim::dense_matvec_: matrix/vector shape mismatch");
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
    ::pluss::throw_error("subsp::math::optim::dense_transpose_matvec_: expected a rank-2 matrix");
  }

  const std::int64_t rows = matrix.sizes()[0];
  const std::int64_t cols = matrix.sizes()[1];
  const Series flat_vector = vector.reshape({vector.numel()}).to(ScalarType::Float64);
  if (flat_vector.numel() != rows) {
    ::pluss::throw_error(
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
    ::pluss::throw_error("subsp::math::optim::evaluate_problem: objective is not defined");
  }
  if (problem.bounds.lower.has_value() && !problem.bounds.lower->same_layout(params)) {
    ::pluss::throw_error("subsp::math::optim::evaluate_problem: lower bounds layout mismatch");
  }
  if (problem.bounds.upper.has_value() && !problem.bounds.upper->same_layout(params)) {
    ::pluss::throw_error("subsp::math::optim::evaluate_problem: upper bounds layout mismatch");
  }
  if (problem.bounds.lower.has_value() && problem.bounds.upper.has_value()) {
    for (const auto& block : params.blocks()) {
      const Series diff = problem.bounds.lower->at(block.name).sub(problem.bounds.upper->at(block.name));
      if (max_positive_scalar_(diff) > 0.0) {
        ::pluss::throw_error("subsp::math::optim::evaluate_problem: lower bound exceeds upper bound");
      }
    }
  }
  const std::int64_t n = params.total_numel();
  if (problem.linear.Aeq.has_value()) {
    require_defined_(*problem.linear.Aeq, "evaluate_problem");
    if (problem.linear.Aeq->sizes().size() != 2 || problem.linear.Aeq->sizes()[1] != n) {
      ::pluss::throw_error("subsp::math::optim::evaluate_problem: Aeq shape mismatch");
    }
    if (problem.linear.beq.has_value() && problem.linear.beq->numel() != problem.linear.Aeq->sizes()[0]) {
      ::pluss::throw_error("subsp::math::optim::evaluate_problem: beq length mismatch");
    }
  } else if (problem.linear.beq.has_value() && problem.linear.beq->numel() != 0) {
    ::pluss::throw_error("subsp::math::optim::evaluate_problem: beq provided without Aeq");
  }
  if (problem.linear.Aineq.has_value()) {
    require_defined_(*problem.linear.Aineq, "evaluate_problem");
    if (problem.linear.Aineq->sizes().size() != 2 || problem.linear.Aineq->sizes()[1] != n) {
      ::pluss::throw_error("subsp::math::optim::evaluate_problem: Aineq shape mismatch");
    }
    if (problem.linear.bineq.has_value() && problem.linear.bineq->numel() != problem.linear.Aineq->sizes()[0]) {
      ::pluss::throw_error("subsp::math::optim::evaluate_problem: bineq length mismatch");
    }
  } else if (problem.linear.bineq.has_value() && problem.linear.bineq->numel() != 0) {
    ::pluss::throw_error("subsp::math::optim::evaluate_problem: bineq provided without Aineq");
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

inline Series identity_matrix_(std::int64_t n,
                               pluss::codem::Platform platform);

inline Series stack_cols_series_(const std::vector<Series>& cols,
                                 std::int64_t nrows,
                                 pluss::codem::Platform platform);

template <class AD>
AD dense_matvec_autodiff_(const Series& matrix, const AD& vector) {
  require_defined_(matrix, "dense_matvec_autodiff");
  if (matrix.sizes().size() != 2) {
    ::pluss::throw_error("subsp::math::optim::dense_matvec_autodiff_: expected a rank-2 matrix");
  }
  const std::int64_t rows = matrix.sizes()[0];
  const std::int64_t cols = matrix.sizes()[1];
  if (vector.value().numel() != cols) {
    ::pluss::throw_error("subsp::math::optim::dense_matvec_autodiff_: matrix/vector shape mismatch");
  }
  const Series aligned =
      matrix.to(ScalarType::Float64).to(vector.value().platform()).reshape({rows, cols});
  if constexpr (std::is_same_v<AD, Trace>) {
    return Trace(aligned.matmul(vector.value().reshape({cols, 1})).reshape({rows}));
  } else {
    return DualTrace(
        aligned.matmul(vector.value().reshape({cols, 1})).reshape({rows}),
        aligned.matmul(vector.tangent().reshape({cols, 1})).reshape({rows}));
  }
}

inline std::pair<Series, ParameterPack<Series>> objective_autodiff_value_and_grad_(
    const ConstrainedProblem& problem,
    const ParameterPack<Series>& params) {
  if (problem.objective.autodiff) {
    return value_and_grad(
        [&](const ParameterPack<Trace>& p) -> Trace { return problem.objective.autodiff(p); },
        params);
  }
  ::pluss::throw_error(
      "subsp::math::optim::objective_autodiff_value_and_grad_: autodiff objective callback is not defined");
}

template <class AD>
AD call_objective_autodiff_(const ConstrainedProblem& problem, const ParameterPack<AD>& params) {
  if constexpr (std::is_same_v<AD, Trace>) {
    if (problem.objective.autodiff) return problem.objective.autodiff(params);
  } else {
    if (problem.objective.autodiff_dual) return problem.objective.autodiff_dual(params);
  }
  ::pluss::throw_error(
      "subsp::math::optim::call_objective_autodiff_: autodiff objective callback is required");
}

template <class AD>
std::optional<AD> call_nonlinear_equality_autodiff_(const ConstrainedProblem& problem,
                                                    const ParameterPack<AD>& params) {
  if constexpr (std::is_same_v<AD, Trace>) {
    if (problem.nonlinear.equality) return problem.nonlinear.equality(params);
  } else {
    if (problem.nonlinear.equality_dual) return problem.nonlinear.equality_dual(params);
  }
  return std::nullopt;
}

template <class AD>
std::optional<AD> call_nonlinear_inequality_autodiff_(const ConstrainedProblem& problem,
                                                      const ParameterPack<AD>& params) {
  if constexpr (std::is_same_v<AD, Trace>) {
    if (problem.nonlinear.inequality) return problem.nonlinear.inequality(params);
  } else {
    if (problem.nonlinear.inequality_dual) return problem.nonlinear.inequality_dual(params);
  }
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
  if (!problem.objective.autodiff_dual) return false;
  if (requires_dual_nonlinear_callbacks_(problem) &&
      (!problem.nonlinear.equality_dual && !problem.nonlinear.inequality_dual)) {
    return false;
  }
  if (problem.nonlinear.equality && !problem.nonlinear.equality_dual) return false;
  if (problem.nonlinear.inequality && !problem.nonlinear.inequality_dual) return false;
  if (problem.nonlinear.analytic &&
      (!problem.nonlinear.equality_dual || !problem.nonlinear.inequality_dual)) {
    return false;
  }
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

  if constexpr (std::is_same_v<AD, DualTrace>) {
    if (!supports_exact_curvature_(problem, OptimaHessianApproximation::Exact)) {
      ::pluss::throw_error(
          "subsp::math::optim::barrier_objective_autodiff_: exact internal-protocol curvature requires objective.autodiff_dual and dual nonlinear callbacks");
    }
  }

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
      auto [_, hv] = hvp(
          [&](const ParameterPack<DualTrace>& p) -> DualTrace {
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

inline ParameterPack<Series> lift_flat_gradient_(const ParameterPack<Series>& params, const Series& flat_grad) {
  return unflatten_values(params.layout(), flat_grad.reshape({flat_grad.numel()}));
}

inline ParameterPack<Series> add_pack_(const ParameterPack<Series>& lhs, const ParameterPack<Series>& rhs) {
  if (!lhs.same_layout(rhs)) {
    ::pluss::throw_error("subsp::math::optim::add_pack_: layout mismatch");
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

inline ParameterPack<Series> lift_flat_std_(const ParameterPack<Series>& like,
                                            const std::vector<double>& flat) {
  if (flat.size() != static_cast<std::size_t>(like.total_numel())) {
    ::pluss::throw_error("subsp::math::optim::lift_flat_std_: size mismatch");
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

inline Series ones_series_(std::int64_t n,
                           pluss::codem::Platform platform = pluss::codem::Platform::CPU) {
  return pluss::ones({n}, ScalarType::Float64).to(platform);
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
    return pluss::empty({0}, ScalarType::Float64).to(x.defined() ? x.platform() : pluss::codem::Platform::CPU);
  }
  return x.reshape({x.numel()}).slice(0, offset, offset + length, 1).reshape({length});
}

inline Series row_scale_series_(const std::vector<FlatConstraintRow>& rows,
                                pluss::codem::Platform platform) {
  if (rows.empty()) return pluss::empty({0}, ScalarType::Float64).to(platform);
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

inline OptimaNormalizationStats compute_problem_normalization_(const ProblemEvaluation& evaluation,
                                                               const FlatConstraintSystem& system,
                                                               OptimaProblemScaling scaling,
                                                               pluss::codem::Platform platform) {
  OptimaNormalizationStats out;
  if (!normalize_objective_constraints_(scaling)) {
    out.equality_row_scale = pluss::ones({static_cast<std::int64_t>(system.equalities.size())}, ScalarType::Float64).to(platform);
    out.inequality_row_scale = pluss::ones({static_cast<std::int64_t>(system.inequalities.size())}, ScalarType::Float64).to(platform);
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

inline std::vector<FlatConstraintRow> scale_constraint_rows_(const std::vector<FlatConstraintRow>& rows,
                                                             const Series& variable_scale,
                                                             const Series& row_scale) {
  if ((!variable_scale.defined() || variable_scale.numel() == 0) &&
      (!row_scale.defined() || row_scale.numel() == 0)) {
    return rows;
  }
  std::vector<FlatConstraintRow> scaled;
  scaled.reserve(rows.size());
  const Series var = variable_scale.defined() ? variable_scale.to(ScalarType::Float64) : Series();
  const Series weights = row_scale.defined() ? row_scale.to(ScalarType::Float64).reshape({row_scale.numel()}) : Series();
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
  if (active_constraints.empty()) return pluss::empty({0}, ScalarType::Float64).to(platform);
  const Series eq = normalization.equality_row_scale.defined()
      ? normalization.equality_row_scale.to(ScalarType::Float64).reshape({normalization.equality_row_scale.numel()})
      : Series();
  const Series ineq = normalization.inequality_row_scale.defined()
      ? normalization.inequality_row_scale.to(ScalarType::Float64).reshape({normalization.inequality_row_scale.numel()})
      : Series();
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
  return scale_constraint_rows_(rows, variable_scale, Series{});
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
    ::pluss::throw_error("subsp::math::optim::solve_dense_system_series_: expected square matrix");
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
    Series solution;

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
    Series solution;
    if (try_device_cg(base.add(identity.mul(std::max(1e-12, base_regularization))), rhs2d, solution)) {
      ok = true;
      return solution;
    }
    return zeros_series_(n, platform);
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
    ::pluss::throw_error("subsp::math::optim::solve_spd_system_cg_series_: expected square matrix");
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
    const Series zero = pluss::zeros_like(dir, ScalarType::Float64).to(block_platform);

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

inline Series stack_cols_series_(const std::vector<Series>& cols,
                                 std::int64_t nrows,
                                 pluss::codem::Platform platform) {
  if (cols.empty()) return pluss::empty({nrows, 0}, ScalarType::Float64).to(platform);
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
      row_pieces.push_back(pluss::zeros({block.value.numel()}, ScalarType::Float64).to(platform));
      continue;
    }
    const Series& block_jacobian = jacobian.at(block.name);
    const std::int64_t cols = block.value.numel();
    if (block_jacobian.numel() == 0) {
      row_pieces.push_back(pluss::zeros({cols}, ScalarType::Float64).to(platform));
      continue;
    }
    if (block_jacobian.sizes().size() != 2 || block_jacobian.sizes()[0] <= row || block_jacobian.sizes()[1] != cols) {
      ::pluss::throw_error("subsp::math::optim::jacobian_row_from_pack_: shape mismatch");
    }
    row_pieces.push_back(
        block_jacobian.to(ScalarType::Float64).to(platform).reshape({block_jacobian.sizes()[0], cols}).select(0, row).reshape({cols}));
  }
  if (row_pieces.empty()) {
    return pluss::empty({0}, ScalarType::Float64).to(platform);
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

inline FlatConstraintSystem assemble_constraint_system_(const ConstrainedProblem& problem,
                                                        const ParameterPack<Series>& params,
                                                        const ProblemEvaluation& evaluation) {
  FlatConstraintSystem out;
  const std::int64_t n = params.total_numel();
  const auto platform = params.empty() ? pluss::codem::Platform::CPU : params[0].value.platform();
  const Series flat_params = flatten_values(params);
  const Series linear_eq = evaluate_linear_eq_(problem.linear, flat_params);
  const Series linear_ineq = evaluate_linear_ineq_(problem.linear, flat_params);

  if (problem.linear.Aeq.has_value()) {
    const Series residuals = linear_eq.to(ScalarType::Float64).reshape({linear_eq.numel()});
    const std::int64_t rows = problem.linear.Aeq->sizes()[0];
    const Series Aeq = problem.linear.Aeq->to(ScalarType::Float64).to(platform).reshape({rows, n});
    for (std::int64_t row = 0; row < rows; ++row) {
      const Series jac = Aeq.select(0, row).reshape({n}).to(ScalarType::Float64).to(platform);
      out.equalities.push_back(
          {jac,
           residuals.select(0, row).item<double>(),
           true,
           ConstraintSource::LinearEquality,
           row});
    }
  }

  const Series nonlinear_eq = evaluation.nonlinear_equality_residual.defined()
      ? evaluation.nonlinear_equality_residual.to(ScalarType::Float64).reshape({evaluation.nonlinear_equality_residual.numel()})
      : pluss::empty({0}, ScalarType::Float64).to(platform);
  for (std::int64_t row = 0; row < evaluation.nonlinear_equality_residual.numel(); ++row) {
    out.equalities.push_back(
        {jacobian_row_from_pack_(params, evaluation.equality_jacobian, row),
         nonlinear_eq.select(0, row).item<double>(),
         true,
         ConstraintSource::NonlinearEquality,
         row});
  }

  if (problem.linear.Aineq.has_value()) {
    const Series residuals = linear_ineq.to(ScalarType::Float64).reshape({linear_ineq.numel()});
    const std::int64_t rows = problem.linear.Aineq->sizes()[0];
    const Series Aineq = problem.linear.Aineq->to(ScalarType::Float64).to(platform).reshape({rows, n});
    for (std::int64_t row = 0; row < rows; ++row) {
      const Series jac = Aineq.select(0, row).reshape({n}).to(ScalarType::Float64).to(platform);
      out.inequalities.push_back(
          {jac,
           residuals.select(0, row).item<double>(),
           false,
           ConstraintSource::LinearInequality,
           row});
    }
  }

  if (problem.bounds.lower.has_value()) {
    std::int64_t offset = 0;
    for (const auto& block : params.blocks()) {
      const Series lower = problem.bounds.lower->at(block.name).to(ScalarType::Float64).to(platform).reshape({block.value.numel()});
      const Series value = block.value.to(ScalarType::Float64).to(platform).reshape({block.value.numel()});
      for (std::int64_t i = 0; i < block.value.numel(); ++i) {
        out.inequalities.push_back(
            {one_hot_jacobian_row_(n, offset + i, -1.0, platform),
             lower.select(0, i).item<double>() - value.select(0, i).item<double>(),
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
      const Series upper = problem.bounds.upper->at(block.name).to(ScalarType::Float64).to(platform).reshape({block.value.numel()});
      const Series value = block.value.to(ScalarType::Float64).to(platform).reshape({block.value.numel()});
      for (std::int64_t i = 0; i < block.value.numel(); ++i) {
        out.inequalities.push_back(
            {one_hot_jacobian_row_(n, offset + i, 1.0, platform),
             value.select(0, i).item<double>() - upper.select(0, i).item<double>(),
             false,
             ConstraintSource::UpperBound,
             offset + i});
      }
      offset += block.value.numel();
    }
  }

  const Series nonlinear_ineq = evaluation.nonlinear_inequality_residual.defined()
      ? evaluation.nonlinear_inequality_residual.to(ScalarType::Float64).reshape({evaluation.nonlinear_inequality_residual.numel()})
      : pluss::empty({0}, ScalarType::Float64).to(platform);
  for (std::int64_t row = 0; row < evaluation.nonlinear_inequality_residual.numel(); ++row) {
    out.inequalities.push_back(
        {jacobian_row_from_pack_(params, evaluation.inequality_jacobian, row),
         nonlinear_ineq.select(0, row).item<double>(),
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
  const Series lambda = multipliers.defined() ? multipliers.to(ScalarType::Float64).reshape({multipliers.numel()}) : Series();
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

  const Series lambda = multipliers.defined() ? multipliers.to(ScalarType::Float64).reshape({multipliers.numel()}) : Series();
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
  const Series ones = pluss::ones({d.numel()}, ScalarType::Float64).to(platform);
  const Series inf_fill = ones.mul(std::numeric_limits<double>::infinity());
  double max_step = std::numeric_limits<double>::infinity();
  if (problem.bounds.lower.has_value()) {
    const Series lower = flatten_values(*problem.bounds.lower).to(ScalarType::Float64).to(platform).reshape({d.numel()});
    const Series mask = d.lt(0.0);
    if (mask.any().item<bool>()) {
      const Series safe_denom = mask.where(d.mul(-1.0), ones);
      Series candidate = x.sub(lower).div(safe_denom).mul(fraction_to_boundary);
      candidate = mask.where(candidate, inf_fill);
      max_step = std::min(max_step, candidate.amin(0, false).item<double>());
    }
  }
  if (problem.bounds.upper.has_value()) {
    const Series upper = flatten_values(*problem.bounds.upper).to(ScalarType::Float64).to(platform).reshape({d.numel()});
    const Series mask = d.gt(0.0);
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
  const Series active = residual.le(active_set_tolerance);
  const Series incoming = directional.gt(0.0);
  const Series mask = active.logical_and(incoming);
  if (!mask.any().item<bool>()) {
    return std::numeric_limits<double>::infinity();
  }
  const Series ones = pluss::ones({residual.numel()}, ScalarType::Float64).to(platform);
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
  *multipliers = drop_mask.where(pluss::zeros_like(lambda, ScalarType::Float64).to(lambda.platform()), lambda)
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
    ::pluss::throw_error("subsp::math::optim::barrier_value_: barrier parameter must be positive");
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
    ::pluss::throw_error("subsp::math::optim::barrier_value_: barrier parameter must be positive");
  }
  double value = normalization.objective_scale * evaluation.value.item<double>();
  const Series eq_scales = normalization.equality_row_scale.defined()
      ? normalization.equality_row_scale.to(ScalarType::Float64).reshape({normalization.equality_row_scale.numel()})
      : Series();
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
  if (system.inequalities.empty()) return pluss::empty({0}, ScalarType::Float64).to(platform);
  std::vector<double> slack(system.inequalities.size(), 0.0);
  for (std::size_t i = 0; i < system.inequalities.size(); ++i) {
    slack[i] = std::max(-system.inequalities[i].residual, 1e-12);
  }
  return pluss::make_series(std::move(slack), {static_cast<std::int64_t>(system.inequalities.size())})
      .to(ScalarType::Float64)
      .to(platform);
}

inline Series inequality_residual_series_(const FlatConstraintSystem& system,
                                          pluss::codem::Platform platform) {
  if (system.inequalities.empty()) return pluss::empty({0}, ScalarType::Float64).to(platform);
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
  if (system.equalities.empty()) return pluss::empty({0}, ScalarType::Float64).to(platform);
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
      : Series();
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
  const Series slack_flat = slack.defined() ? slack.to(ScalarType::Float64).reshape({slack.numel()}) : Series();
  const Series dual_flat = dual.defined() ? dual.to(ScalarType::Float64).reshape({dual.numel()}) : Series();
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

inline Series primal_dual_stationarity_flat_(const ProblemEvaluation& evaluation,
                                             const FlatConstraintSystem& system,
                                             const Series& inequality_dual,
                                             double equality_penalty) {
  const Series flat_grad = flatten_values(evaluation.gradient).to(ScalarType::Float64);
  Series grad = flat_grad.reshape({flat_grad.numel()});
  const auto platform = grad.platform();
  if (!system.equalities.empty() && equality_penalty != 0.0) {
    const Series eq_weights = equality_residual_series_(system, platform).mul(equality_penalty);
    grad = add_weighted_row_jacobians_(grad, system.equalities, eq_weights);
  }
  if (!system.inequalities.empty() && inequality_dual.defined() && inequality_dual.numel() > 0) {
    const Series lambda = inequality_dual.to(ScalarType::Float64).to(platform).reshape({inequality_dual.numel()});
    grad = add_weighted_row_jacobians_(grad, system.inequalities, lambda);
  }
  return grad.reshape({grad.numel()});
}

inline Series primal_dual_stationarity_flat_(const ProblemEvaluation& evaluation,
                                             const FlatConstraintSystem& system,
                                             const Series& inequality_dual,
                                             double equality_penalty,
                                             const OptimaNormalizationStats& normalization) {
  Series grad = flatten_values(evaluation.gradient).to(ScalarType::Float64).mul(normalization.objective_scale);
  const auto platform = grad.platform();
  const Series eq_scales = normalization.equality_row_scale.defined()
      ? normalization.equality_row_scale.to(ScalarType::Float64).to(platform).reshape({normalization.equality_row_scale.numel()})
      : Series();
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
  if (!system.inequalities.empty() && inequality_dual.defined() && inequality_dual.numel() > 0) {
    const Series lambda = inequality_dual.to(ScalarType::Float64).to(platform).reshape({inequality_dual.numel()});
    grad = add_weighted_row_jacobians_(grad, system.inequalities, lambda);
  }
  return grad.reshape({grad.numel()});
}

inline void update_primal_dual_metrics_(InternalProtocolState& state,
                                        const FlatConstraintSystem& system,
                                        double barrier_parameter,
                                        double equality_penalty) {
  const auto platform =
      state.params.empty() ? pluss::codem::Platform::CPU : state.params[0].value.platform();
  state.inequality_slack = inequality_slack_(system, platform);
  state.inequality_dual = current_or_barrier_dual_(state.inequality_dual, state.inequality_slack, barrier_parameter);
  state.primal_residual = std::max(
      equality_residual_norm_(system),
      state.inequality_slack.defined() && state.inequality_slack.numel() > 0
          ? state.inequality_slack.add(inequality_residual_series_(system, platform)).abs().max().item<double>()
          : 0.0);
  state.complementarity_residual =
      complementarity_residual_(state.inequality_slack, state.inequality_dual, barrier_parameter);
  state.barrier_gradient_norm = series_inf_norm_(
      primal_dual_stationarity_flat_(state.evaluation, system, state.inequality_dual, equality_penalty));
  state.feasibility_metric = state.evaluation.max_constraint_violation;
}

inline void update_primal_dual_metrics_(InternalProtocolState& state,
                                        const FlatConstraintSystem& system,
                                        double barrier_parameter,
                                        double equality_penalty,
                                        const OptimaNormalizationStats& normalization) {
  const auto platform =
      state.params.empty() ? pluss::codem::Platform::CPU : state.params[0].value.platform();
  state.inequality_slack = inequality_slack_(system, platform);
  state.inequality_dual = current_or_barrier_dual_(state.inequality_dual, state.inequality_slack, barrier_parameter);
  state.primal_residual = std::max(
      equality_residual_norm_(system),
      state.inequality_slack.defined() && state.inequality_slack.numel() > 0
          ? state.inequality_slack.add(inequality_residual_series_(system, platform)).abs().max().item<double>()
          : 0.0);
  state.complementarity_residual =
      complementarity_residual_(state.inequality_slack, state.inequality_dual, barrier_parameter);
  state.barrier_gradient_norm = series_inf_norm_(
      primal_dual_stationarity_flat_(state.evaluation, system, state.inequality_dual, equality_penalty, normalization));
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
      return pluss::full({grad.numel()}, std::numeric_limits<double>::quiet_NaN(), ScalarType::Float64)
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
      : Series();
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
      return pluss::full({grad.numel()}, std::numeric_limits<double>::quiet_NaN(), ScalarType::Float64)
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
  const Series zero_flat = pluss::zeros_like(seed_flat);
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

inline void reduce_barrier_state_(InternalProtocolState& state,
                                  const FlatConstraintSystem& system,
                                  const InternalProtocolOptions& options) {
  if (!(state.barrier_parameter > options.minimum_barrier_parameter)) return;

  state.barrier_parameter = std::max(options.minimum_barrier_parameter,
                                     state.barrier_parameter * options.barrier_decay);
  state.hessian = identity_matrix_(
      state.params.total_numel(),
      state.params.empty() ? pluss::codem::Platform::CPU : state.params[0].value.platform());
  state.lbfgs_s_history.clear();
  state.lbfgs_y_history.clear();
  state.inequality_dual = blend_dual_toward_centrality_(
      current_or_barrier_dual_(state.inequality_dual, state.inequality_slack, state.barrier_parameter),
      state.inequality_slack,
      state.barrier_parameter,
      1.0);
  state.barrier_value = barrier_value_(
      state.evaluation,
      system,
      state.barrier_parameter,
      options.equality_penalty,
      state.normalization);
  const auto reduced_gradient = barrier_gradient_flat_(
      state.evaluation,
      system,
      state.barrier_parameter,
      options.equality_penalty,
      state.normalization);
  state.barrier_gradient_norm = series_inf_norm_(reduced_gradient);
  update_primal_dual_metrics_(
      state,
      system,
      state.barrier_parameter,
      options.equality_penalty,
      state.normalization);
  state.in_feasibility_phase = should_enter_feasibility_phase_(state, options);
  state.message = "barrier reduced";
}

inline Series bfgs_hessian_update_(const Series& hessian,
                                   const Series& s,
                                   const Series& y,
                                                double curvature_tolerance) {
  const std::int64_t n = s.numel();
  if (!hessian.defined() || hessian.numel() != n * n || y.numel() != n) {
    ::pluss::throw_error("subsp::math::optim::bfgs_hessian_update_: shape mismatch");
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

inline bool lbfgs_curvature_ok_(const Series& s,
                                const Series& y,
                                double curvature_tolerance) {
  const double sy = series_dot_(s, y);
  const double s_norm = s.to(ScalarType::Float64).square().sum().sqrt().item<double>();
  const double y_norm = y.to(ScalarType::Float64).square().sum().sqrt().item<double>();
  return sy > curvature_tolerance * std::max(1.0, s_norm * y_norm);
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
    Series step;
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

  Series solution;
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
  } else {
    solution =
        solve_dense_system_series_(KKT, rhs, regularization, linear_system_solver, pivot_threshold, ok);
  }
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
  Series lag_grad = gradient.to(ScalarType::Float64).reshape({gradient.numel()});
  double multiplier_violation = 0.0;
  const Series lambda = multipliers.defined() ? multipliers.to(ScalarType::Float64).reshape({multipliers.numel()}) : Series();
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
      const Series zeros = pluss::zeros_like(lambda_used, ScalarType::Float64).to(lambda_used.platform());
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
  const Series lambda = multipliers.defined() ? multipliers.to(ScalarType::Float64).reshape({multipliers.numel()}) : Series();
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
      pack_inf_norm_(state.evaluation.gradient),
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
    Series contribution = pluss::zeros_like(param_block.value, param_block.value.scalar_type());
    if (jacobian.contains(param_block.name)) {
      const Series& block_jacobian = jacobian.at(param_block.name);
      if (block_jacobian.defined() && block_jacobian.numel() > 0) {
        const std::int64_t cols = param_block.value.numel();
        const std::int64_t rows = residual.numel();
        if (block_jacobian.numel() != rows * cols) {
          ::pluss::throw_error(
              "subsp::math::optim::combine_constraint_gradient_: jacobian/residual shape mismatch");
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
      auto [value, grad_pack] = detail::objective_autodiff_value_and_grad_(problem, params);
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

  if (problem.objective.analytic && (problem.objective.autodiff || problem.objective.autodiff_dual)) {
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
                              : pluss::ones({evaluation.equality_residual.numel()}, ScalarType::Float64).to(flat_params.platform());
  const Series ineq_scale = normalization.inequality_row_scale.defined()
                                ? normalization.inequality_row_scale.to(flat_params.platform())
                                : pluss::ones({evaluation.inequality_residual.numel()}, ScalarType::Float64).to(flat_params.platform());

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
          : pluss::zeros({flat_params.numel()}, ScalarType::Float64).to(flat_params.platform());
  Series linear_ineq_grad_flat =
      problem.linear.Aineq.has_value() && linear_ineq_weight.numel() > 0
          ? detail::dense_transpose_matvec_(*problem.linear.Aineq, linear_ineq_weight)
          : pluss::zeros({flat_params.numel()}, ScalarType::Float64).to(flat_params.platform());
  Series bound_grad_flat = pluss::zeros({flat_params.numel()}, ScalarType::Float64).to(flat_params.platform());
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
      if (step_size <= std::max(1e-12, options.step_tolerance)) break;
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
  if (inequality_margin_(current_system) <= options.initialization_margin) {
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
      if (step_size <= std::max(1e-12, options.step_tolerance)) break;
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
      std::max(1e4 * options.minimum_barrier_parameter, options.minimum_barrier_parameter);
  if (!(state.barrier_parameter <= barrier_gate)) return false;

  const double stationarity_gate = std::max(2000.0 * options.optimality_tolerance, 1e-12);
  const double feasibility_gate = std::max(5.0 * options.constraint_tolerance, 1e-12);
  const double complementarity_gate = std::max(25.0 * options.optimality_tolerance, 1e-12);

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
                                  std::size_t max_keep = 32) {
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
      if (step_size <= std::max(1e-12, options.step_tolerance)) break;
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
  detail::maybe_update_best_feasible_(state, options.constraint_tolerance);
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
        detail::maybe_update_best_feasible_(next, options.constraint_tolerance);
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
  detail::run_progress_hook_(
      options.progress_hook,
      detail::make_progress_snapshot_(OptimaAlgorithm::MeritMethod, OptimaProgressStage::Done, state));
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
        state.params.empty() ? pluss::codem::Platform::CPU : state.params[0].value.platform());
    state.merit_value = detail::penalized_value_(state.evaluation, state.current_penalty, state.normalization);
    const auto active = detail::active_constraints_(system, options.active_set_tolerance);
    const auto normalized_active = detail::scale_constraint_rows_(
        active,
        Series{},
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

inline SequentialQPState sequential_qp_step(const ConstrainedProblem& problem,
                                            const SequentialQPState& state,
                                            const SequentialQPOptions& options = {}) {
  SequentialQPState next = state;
  if (state.converged || state.evaluation.status != EvalStatus::Ok) return next;

  const auto constraint_system = detail::assemble_constraint_system_(problem, state.params, state.evaluation);
  auto active_constraints =
      detail::active_constraints_(constraint_system, options.active_set_tolerance);
  const Series flat_gradient = flatten_values(state.evaluation.gradient).to(ScalarType::Float64);
  const Series variable_scale = state.variable_scale.defined()
                                    ? state.variable_scale.to(ScalarType::Float64).to(flat_gradient.platform())
                                    : detail::ones_series_(flat_gradient.numel(), flat_gradient.platform());

  bool kkt_ok = false;
  Series step_direction;
  Series multipliers;
  std::int64_t krylov_iterations = 0;
  const OptimaLinearSystemSolver sqp_linear_solver =
      detail::use_iterative_subproblem_solver_(
          options.linear_system_solver,
          options.system_structure,
          flat_gradient.numel() + static_cast<std::int64_t>(active_constraints.size()),
          options.iterative_dimension_threshold)
          ? OptimaLinearSystemSolver::ConjugateGradient
          : options.linear_system_solver;
  const double sqp_krylov_tolerance =
      sqp_linear_solver == OptimaLinearSystemSolver::ConjugateGradient
          ? std::min(options.krylov_tolerance, 1e-8)
          : options.krylov_tolerance;
  const double sqp_krylov_absolute_tolerance =
      sqp_linear_solver == OptimaLinearSystemSolver::ConjugateGradient
          ? std::min(options.krylov_absolute_tolerance, 1e-12)
          : options.krylov_absolute_tolerance;
  for (std::size_t attempt = 0; attempt <= active_constraints.size(); ++attempt) {
    const Series active_row_scale =
        detail::active_constraint_row_scales_(constraint_system, active_constraints, state.normalization, flat_gradient.platform());
    const auto scaled_constraints = detail::scale_constraint_rows_(active_constraints, variable_scale, active_row_scale);
    std::tie(step_direction, multipliers) = detail::solve_sqp_kkt_(
        detail::scale_hessian_(state.hessian, variable_scale),
        detail::scale_gradient_(flat_gradient.mul(state.normalization.objective_scale), variable_scale),
        scaled_constraints,
        options.regularization,
        sqp_linear_solver,
        options.max_krylov_iterations,
        sqp_krylov_tolerance,
        sqp_krylov_absolute_tolerance,
        options.pivot_threshold,
        krylov_iterations,
        kkt_ok);
    step_direction = detail::unscale_step_(step_direction, variable_scale);
    if (!kkt_ok) break;

    const auto [most_negative, most_negative_index] =
        detail::most_negative_inequality_multiplier_(active_constraints, multipliers);
    if (most_negative_index == active_constraints.size() ||
        most_negative >= -options.active_set_tolerance) {
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
  const Series accepted_active_row_scale =
      detail::active_constraint_row_scales_(constraint_system, active_constraints, state.normalization, flat_gradient.platform());
  next.kkt_residual = detail::kkt_residual_(
      flat_gradient.mul(state.normalization.objective_scale),
      detail::scale_constraint_rows_(active_constraints, Series{}, accepted_active_row_scale),
      multipliers);
  next.krylov_iterations = sqp_linear_solver == OptimaLinearSystemSolver::ConjugateGradient ? krylov_iterations : 0;
  if (step_inf_norm <= options.optimality_tolerance &&
      next.kkt_residual <= options.optimality_tolerance) {
    next.converged = true;
    next.message = state.evaluation.max_constraint_violation <= options.constraint_tolerance
                       ? "optimality and feasibility tolerances satisfied"
                       : "constraint violation locally stationary";
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
                           options.active_set_tolerance,
                           options.fraction_to_boundary));
  const double base_merit = state.merit_value;
  double directional_derivative = detail::series_dot_(merit_gradient, step_direction);
  if (!(directional_derivative < 0.0)) {
    directional_derivative = -std::max(1.0, detail::series_dot_(step_direction, step_direction));
  }
  const bool admissible_step_available = step_size > 0.0 && std::isfinite(step_size);
  const double predicted_value_change =
      admissible_step_available ? std::abs(step_size * directional_derivative) : std::numeric_limits<double>::infinity();
  if (admissible_step_available &&
      state.evaluation.max_constraint_violation <= options.constraint_tolerance) {
    if (step_inf_norm <= 2.0 * options.step_tolerance) {
      next.converged = true;
      next.message = "search direction below step tolerance at a feasible point";
      return next;
    }
    if (predicted_value_change <= options.value_progress_tolerance) {
      next.converged = true;
      next.message = "predicted objective improvement below tolerance at a feasible point";
      return next;
    }
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
                               options.active_set_tolerance,
                               options.fraction_to_boundary));
      directional_derivative = detail::series_dot_(merit_gradient, step_direction);
      if (!(directional_derivative < 0.0)) {
        directional_derivative = -std::max(1.0, detail::series_dot_(step_direction, step_direction));
      }
    }
    if (!(step_size > 0.0) || !std::isfinite(step_size)) {
      next.converged = true;
      next.message = "no progress in the bound-feasible region";
      return next;
    }
  }
  const auto state_layout = state.params.layout();
  const Series state_flat64 = flatten_values(state.params).to(ScalarType::Float64);

  for (std::int64_t ls_iter = 0; ls_iter < options.max_line_search_iterations; ++ls_iter) {
    if (step_size <= std::max(1e-12, options.step_tolerance)) break;
    const Series trial_flat = state_flat64.add(step_direction.mul(step_size));
    auto trial = unflatten_values(state_layout, trial_flat);
    if (options.project_bounds) trial = detail::project_bounds_(trial, problem.bounds);

    auto trial_eval = evaluate_problem(problem, trial);
    next.function_evaluations += 1;
    if (trial_eval.status == EvalStatus::Ok) {
      double trial_penalty = state.current_penalty;
      if (trial_eval.max_constraint_violation >
          std::max(options.constraint_tolerance, 0.9 * state.evaluation.max_constraint_violation)) {
        trial_penalty *= 2.0;
      }
      const double trial_merit = detail::penalized_value_(trial_eval, trial_penalty, state.normalization);
      const double armijo_rhs = base_merit + options.armijo * step_size * directional_derivative;
      if (detail::accept_sqp_trial_(
              state.evaluation,
              base_merit,
              trial_eval,
              trial_merit,
              step_size,
              armijo_rhs,
              options.constraint_tolerance)) {
        next.params = std::move(trial);
        next.evaluation = std::move(trial_eval);
        next.iteration += 1;
        next.last_step_size = step_size;
        next.current_penalty = trial_penalty;
        next.merit_value = trial_merit;
        next.multipliers = std::move(multipliers);
        next.message = "step accepted";

        const Series x_prev = state_flat64;
        const Series x_next = flatten_values(next.params).to(ScalarType::Float64);
        const Series s = x_next.sub(x_prev);
        if (state.current_penalty == next.current_penalty) {
          const Series next_merit_gradient =
              flatten_values(
                  normalized_penalty_gradient_(problem, next.params, next.evaluation, next.current_penalty, state.normalization))
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
        next.kkt_residual = detail::kkt_residual_(
            next_flat_gradient.mul(state.normalization.objective_scale),
            detail::scale_constraint_rows_(
                next_active,
                Series{},
                detail::active_constraint_row_scales_(next_constraints, next_active, state.normalization, next_flat_gradient.platform())),
            next.multipliers);
        next.variable_scale = state.variable_scale;
        next.normalization = state.normalization;

        detail::maybe_update_best_feasible_(next, options.constraint_tolerance);
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

      if (options.max_second_order_corrections > 0 &&
          trial_eval.status == EvalStatus::Ok &&
          trial_eval.max_constraint_violation >
              std::max(options.constraint_tolerance,
                       state.evaluation.max_constraint_violation + options.second_order_correction_tolerance)) {
        const auto trial_system = detail::assemble_constraint_system_(problem, trial, trial_eval);
        const auto soc_rows = detail::second_order_correction_rows_(
            constraint_system,
            trial_system,
            options.active_set_tolerance);
        Series corrected_direction = step_direction;
        bool soc_ok = false;
        for (std::int64_t soc_iter = 0; soc_iter < options.max_second_order_corrections; ++soc_iter) {
          const Series soc = detail::solve_second_order_correction_(
              soc_rows,
              trial_system,
              flat_gradient.numel(),
              flat_gradient.platform(),
              options.regularization,
              options.linear_system_solver,
              options.pivot_threshold,
              soc_ok);
          if (!soc_ok || detail::series_inf_norm_(soc) <= options.second_order_correction_tolerance) break;
          corrected_direction = step_direction.add(soc);
          const double corrected_step =
              std::min(step_size,
                       std::min(detail::max_step_to_bounds_(
                                    problem, state.params, corrected_direction, options.fraction_to_boundary),
                                detail::max_step_to_linearized_inequalities_(
                                    constraint_system,
                                    corrected_direction,
                                    options.active_set_tolerance,
                                    options.fraction_to_boundary)));
          if (!(corrected_step > 0.0) || !std::isfinite(corrected_step)) break;
          auto corrected_trial = unflatten_values(
              state_layout,
              state_flat64.add(corrected_direction.mul(corrected_step)));
          if (options.project_bounds) corrected_trial = detail::project_bounds_(corrected_trial, problem.bounds);

          auto corrected_eval = evaluate_problem(problem, corrected_trial);
          next.function_evaluations += 1;
          if (corrected_eval.status != EvalStatus::Ok) break;
          double corrected_penalty = state.current_penalty;
          if (corrected_eval.max_constraint_violation >
              std::max(options.constraint_tolerance, 0.9 * state.evaluation.max_constraint_violation)) {
            corrected_penalty *= 2.0;
          }
          const double corrected_merit = detail::penalized_value_(corrected_eval, corrected_penalty, state.normalization);
          const double corrected_rhs = base_merit + options.armijo * corrected_step * directional_derivative;
          if (!detail::accept_sqp_trial_(
                  state.evaluation,
                  base_merit,
                  corrected_eval,
                  corrected_merit,
                  corrected_step,
                  corrected_rhs,
                  options.constraint_tolerance)) {
            break;
          }

          next.params = std::move(corrected_trial);
          next.evaluation = std::move(corrected_eval);
          next.iteration += 1;
          next.last_step_size = corrected_step;
          next.current_penalty = corrected_penalty;
          next.merit_value = corrected_merit;
          next.multipliers = std::move(multipliers);
          next.message = "second-order correction accepted";

          const Series x_prev = state_flat64;
          const Series x_next = flatten_values(next.params).to(ScalarType::Float64);
          const Series s = x_next.sub(x_prev);
          if (state.current_penalty == next.current_penalty) {
            const Series next_merit_gradient =
                flatten_values(
                    normalized_penalty_gradient_(problem, next.params, next.evaluation, next.current_penalty, state.normalization))
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

          const auto next_constraints = detail::assemble_constraint_system_(problem, next.params, next.evaluation);
          const auto next_active =
              detail::active_constraints_(next_constraints, options.active_set_tolerance);
          next.kkt_residual = detail::kkt_residual_(
              flatten_values(next.evaluation.gradient).to(ScalarType::Float64).mul(state.normalization.objective_scale),
              detail::scale_constraint_rows_(
                  next_active,
                  Series{},
                  detail::active_constraint_row_scales_(next_constraints, next_active, state.normalization, flat_gradient.platform())),
              next.multipliers);
          next.variable_scale = state.variable_scale;
          next.normalization = state.normalization;
          detail::maybe_update_best_feasible_(next, options.constraint_tolerance);
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
          std::max(options.constraint_tolerance, 0.5 * state.evaluation.max_constraint_violation)) {
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
      const auto next_active =
          detail::active_constraints_(next_constraints, options.active_set_tolerance);
      next.kkt_residual = detail::kkt_residual_(
          flatten_values(next.evaluation.gradient).to(ScalarType::Float64).mul(state.normalization.objective_scale),
          detail::scale_constraint_rows_(
              next_active,
              Series{},
              detail::active_constraint_row_scales_(next_constraints, next_active, state.normalization, flat_gradient.platform())),
          next.multipliers);
      next.message = "restoration step accepted";
      next.variable_scale = state.variable_scale;
      next.normalization = state.normalization;
      detail::maybe_update_best_feasible_(next, options.constraint_tolerance);
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
    next.message = recovery.message.empty()
                       ? "line search failed to produce an acceptable SQP step after restoration"
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

  bool kkt_ok = false;
  Series step_direction;
  Series multipliers;
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

  for (std::size_t attempt = 0; attempt <= active_constraints.size(); ++attempt) {
    const Series active_row_scale = detail::active_constraint_row_scales_(
        constraint_system, active_constraints, state.normalization, flat_gradient.platform());
    const auto scaled_constraints = detail::scale_constraint_rows_(active_constraints, variable_scale, active_row_scale);
    std::tie(step_direction, multipliers) = detail::solve_sqp_kkt_(
        detail::scale_hessian_(state.hessian, variable_scale),
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
      detail::scale_constraint_rows_(active_constraints, Series{}, accepted_active_row_scale),
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
    if (step_size <= std::max(1e-12, options.step_tolerance)) break;
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
          next.hessian = detail::update_hessian_(
              state.hessian,
              s,
              y,
              options.bfgs_curvature_tolerance,
              options.hessian_approximation,
              next.params.total_numel(),
              next.params[0].value.platform());
        } else {
          next.hessian = detail::identity_matrix_(next.params.total_numel(), next.params[0].value.platform());
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
                Series{},
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
              Series{},
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
    if (!state.converged &&
        working_set_options.favor_working_set_updates &&
        state.stalled_iterations >= working_set_options.max_stalled_iterations &&
        state.iteration < working_set_options.max_iterations) {
      SequentialQPOptions continuation_options = working_set_options;
      continuation_options.progress_hook = {};
      continuation_options.max_iterations =
          std::max<std::int64_t>(1, working_set_options.max_iterations - state.iteration);
      SequentialQPState continuation = solve_sequential_qp(problem, state.params, continuation_options);
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
        "exact internal-protocol curvature requires objective.autodiff_dual and dual nonlinear callbacks";
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
      init_ok);
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
          reinited_ok);
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
        state.params.empty() ? pluss::codem::Platform::CPU : state.params[0].value.platform());
    state.barrier_value = detail::barrier_value_(
        state.evaluation,
        system,
        state.barrier_parameter,
        options.equality_penalty,
        state.normalization);
    detail::update_primal_dual_metrics_(
        state,
        system,
        state.barrier_parameter,
        options.equality_penalty,
        state.normalization);
  } else {
    state.barrier_value = std::numeric_limits<double>::infinity();
    state.barrier_gradient_norm = std::numeric_limits<double>::infinity();
    state.inequality_slack = pluss::empty({0}, ScalarType::Float64).to(
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
      next.barrier_value = detail::barrier_value_(
          next.evaluation,
          next_system,
          next.barrier_parameter,
          options.equality_penalty,
          next.normalization);
      detail::update_primal_dual_metrics_(
          next,
          next_system,
          next.barrier_parameter,
          options.equality_penalty,
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
  const Series slack = detail::inequality_slack_(system, state.hessian.platform());
  const Series dual = detail::current_or_barrier_dual_(state.inequality_dual, slack, state.barrier_parameter);
  const Series primal_dual_gradient = detail::primal_dual_stationarity_flat_(
      state.evaluation,
      system,
      dual,
      options.equality_penalty,
      state.normalization);
  const double barrier_grad_norm = detail::series_inf_norm_(primal_dual_gradient);
  const double complementarity = detail::complementarity_residual_(slack, dual, state.barrier_parameter);
  next.inequality_slack = slack;
  next.inequality_dual = dual;
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
    next.converged = true;
    next.message = "constraint violation locally stationary";
    return next;
  }

  bool solve_ok = false;
  std::int64_t krylov_iterations = 0;
  const std::int64_t n = state.params.total_numel();
  const Series variable_scale = state.variable_scale.defined()
                                    ? state.variable_scale.to(ScalarType::Float64).to(state.hessian.platform())
                                    : detail::ones_series_(n, state.hessian.platform());
  const bool use_exact_curvature =
      options.hessian_approximation == OptimaHessianApproximation::Exact ||
      options.hessian_approximation == OptimaHessianApproximation::HessianVector;
  const Series base_hessian = state.hessian.to(ScalarType::Float64).to(state.hessian.platform());
  Series reduced_hessian = base_hessian;
  const auto equality_weights =
      detail::equality_quadratic_weights_(system, options.equality_penalty, state.normalization);
  const auto inequality_weights = detail::inequality_quadratic_weights_(slack, dual);
  bool exact_hessian_ok = false;
  if (options.hessian_approximation == OptimaHessianApproximation::Exact) {
    reduced_hessian = detail::assemble_hessian_from_hvp_(
        problem,
        state.params,
        state.barrier_parameter,
        options.equality_penalty,
        state.normalization,
        options.hvp_options,
        exact_hessian_ok);
    if (!exact_hessian_ok) {
      reduced_hessian = base_hessian;
    }
  } else if (!use_exact_curvature) {
    for (std::size_t i = 0; i < system.equalities.size() && i < equality_weights.size(); ++i) {
      const double weight = equality_weights[i];
      if (!(weight > 0.0) || !std::isfinite(weight)) continue;
      reduced_hessian = reduced_hessian.add(
          detail::series_outer_(system.equalities[i].jacobian, system.equalities[i].jacobian).mul(weight));
    }
    for (std::size_t i = 0; i < system.inequalities.size() && i < inequality_weights.size(); ++i) {
      const double weight = inequality_weights[i];
      if (!(weight > 0.0) || !std::isfinite(weight)) continue;
      reduced_hessian = reduced_hessian.add(
          detail::series_outer_(system.inequalities[i].jacobian, system.inequalities[i].jacobian).mul(weight));
    }
  }
  const Series scaled_hessian = detail::scale_hessian_(reduced_hessian, variable_scale);
  const Series scaled_base_hessian = detail::scale_hessian_(base_hessian, variable_scale);
  Series scaled_rhs;
  if (!use_exact_curvature && slack.defined() && slack.numel() > 0) {
    Series centering = dual.mul(1.0 - options.centrality_fraction);
    Series rhs = primal_dual_gradient.mul(-1.0);
    for (std::int64_t i = 0; i < centering.numel(); ++i) {
      rhs = rhs.add(system.inequalities[static_cast<std::size_t>(i)].jacobian.mul(
          centering.select(0, i).item<double>()));
    }
    scaled_rhs = detail::scale_gradient_(rhs, variable_scale);
  } else {
    scaled_rhs = detail::scale_gradient_(primal_dual_gradient.mul(-1.0), variable_scale);
  }
  const double adaptive_regularization = std::clamp(
      std::max(
          {std::max(1e-12, options.hessian_regularization),
           1e-3 * std::max(state.barrier_parameter, options.minimum_barrier_parameter),
           1e-4 * std::max(1.0, barrier_grad_norm)}),
      1e-12,
      std::max(options.hessian_regularization, options.hessian_regularization_max));
  Series step_direction;
  if (options.hessian_approximation == OptimaHessianApproximation::Lbfgs) {
    step_direction = detail::unscale_step_(
        detail::lbfgs_direction_(
            detail::scale_gradient_(primal_dual_gradient, variable_scale),
            state.lbfgs_s_history,
            state.lbfgs_y_history),
        variable_scale);
    solve_ok = detail::series_dot_(primal_dual_gradient, step_direction) < 0.0;
    krylov_iterations = 0;
  } else if (options.hessian_approximation == OptimaHessianApproximation::HessianVector &&
             (options.linear_system_solver == OptimaLinearSystemSolver::ConjugateGradient ||
              (options.linear_system_solver == OptimaLinearSystemSolver::Auto && n >= 16))) {
    const Series preconditioner = scaled_hessian.defined() && scaled_hessian.numel() == n * n
                                      ? scaled_hessian.diagonal(0, 0, 1).reshape({n})
                                      : detail::ones_series_(n, state.hessian.platform());
    auto scaled_matvec = [&](const Series& scaled_direction) -> Series {
      try {
        const auto tangent = unflatten_values(
            state.params.layout(),
            detail::unscale_step_(scaled_direction, variable_scale).to(ScalarType::Float64));
        auto [_, hv] = hvp(
            [&](const ParameterPack<DualTrace>& p) -> DualTrace {
              return detail::barrier_objective_autodiff_(
                  problem, p, state.barrier_parameter, options.equality_penalty, &state.normalization);
            },
            state.params,
            tangent,
            options.hvp_options);
        return detail::scale_gradient_(
            flatten_values(hv).to(ScalarType::Float64).to(state.hessian.platform()),
            variable_scale);
      } catch (...) {
        return scaled_direction;
      }
    };
    step_direction = detail::solve_spd_system_cg_operator_(
        scaled_matvec,
        scaled_rhs,
        preconditioner,
        options.max_krylov_iterations,
        options.krylov_tolerance,
        options.krylov_absolute_tolerance,
        solve_ok,
        krylov_iterations);
  } else if (!use_exact_curvature &&
             options.hessian_approximation != OptimaHessianApproximation::Lbfgs &&
             detail::use_iterative_subproblem_solver_(
                 options.linear_system_solver,
                 options.system_structure,
                 n,
                 options.iterative_dimension_threshold) &&
             options.system_structure == OptimaLinearSystemStructure::Sparse) {
    const auto scaled_eq_rows = detail::scale_constraint_rows_(system.equalities, variable_scale);
    const auto scaled_ineq_rows = detail::scale_constraint_rows_(system.inequalities, variable_scale);
    const Series base_diag = scaled_base_hessian.diagonal(0, 0, 1).reshape({n}).abs().maximum(1e-12);
    const Series preconditioner =
        base_diag.add(detail::quadratic_rows_diagonal_(scaled_eq_rows, equality_weights, n, state.hessian.platform()))
            .add(detail::quadratic_rows_diagonal_(scaled_ineq_rows, inequality_weights, n, state.hessian.platform()))
            .maximum(1e-12);
    auto scaled_matvec = [&](const Series& direction) -> Series {
      return detail::dense_matvec_(scaled_base_hessian, direction)
          .add(detail::quadratic_rows_matvec_(scaled_eq_rows, equality_weights, direction, n, state.hessian.platform()))
          .add(detail::quadratic_rows_matvec_(scaled_ineq_rows, inequality_weights, direction, n, state.hessian.platform()));
    };
    step_direction = detail::solve_spd_system_cg_operator_(
        scaled_matvec,
        scaled_rhs,
        preconditioner,
        options.max_krylov_iterations,
        options.krylov_tolerance,
        options.krylov_absolute_tolerance,
        solve_ok,
        krylov_iterations);
  } else if (detail::use_iterative_subproblem_solver_(
                 options.linear_system_solver,
                 options.system_structure,
                 n,
                 options.iterative_dimension_threshold)) {
    step_direction = detail::solve_spd_system_cg_series_(
        scaled_hessian,
        scaled_rhs,
        adaptive_regularization,
        options.max_krylov_iterations,
        options.krylov_tolerance,
        options.krylov_absolute_tolerance,
        solve_ok,
        krylov_iterations);
  } else {
    step_direction = detail::solve_dense_system_series_(
        scaled_hessian,
        scaled_rhs,
        adaptive_regularization,
        options.linear_system_solver,
        options.pivot_threshold,
        solve_ok);
  }
  step_direction = detail::unscale_step_(step_direction, variable_scale);
  next.krylov_iterations = std::max(state.krylov_iterations, krylov_iterations);
  if (!solve_ok) {
    step_direction = detail::unscale_step_(detail::scale_gradient_(primal_dual_gradient, variable_scale).mul(-1.0),
                                           variable_scale);
    next.krylov_iterations = state.krylov_iterations;
  }
  if (detail::series_dot_(primal_dual_gradient, step_direction) >= 0.0) {
    step_direction = detail::unscale_step_(detail::scale_gradient_(primal_dual_gradient, variable_scale).mul(-1.0),
                                           variable_scale);
  }

  const double trust_radius = std::clamp(
      state.trust_region_radius,
      std::max(1e-12, options.trust_region_min_radius),
      std::max(options.trust_region_min_radius, options.trust_region_max_radius));
  double max_step = std::min(options.initial_step_size, trust_radius);
  Series slack_step = pluss::empty({0}, ScalarType::Float64).to(step_direction.platform());
  Series dual_step = pluss::empty({0}, ScalarType::Float64).to(step_direction.platform());
  const Series inequality_residual = detail::inequality_residual_series_(system, state.hessian.platform());
  if (slack.defined() && slack.numel() > 0) {
    const Series Jdx = detail::stack_inequality_jacobian_(system, n, step_direction.platform())
                           .matmul(step_direction.reshape({n, 1}))
                           .reshape({slack.numel()});
    const Series primal_resid = inequality_residual.add(slack);
    slack_step = primal_resid.add(Jdx).mul(-1.0);
    const Series centering_resid =
        slack.mul(dual).sub(state.barrier_parameter * options.centrality_fraction);
    dual_step = centering_resid.add(dual.mul(slack_step)).div(slack).mul(-1.0);
    const Series ones = pluss::ones({slack.numel()}, ScalarType::Float64).to(slack.platform());
    const Series inf_fill = ones.mul(std::numeric_limits<double>::infinity());
    const Series slack_mask = slack_step.lt(0.0);
    if (slack_mask.any().item<bool>()) {
      const Series safe_slack_denom = slack_mask.where(slack_step.mul(-1.0), ones);
      Series slack_candidate = slack.div(safe_slack_denom).mul(options.fraction_to_boundary);
      slack_candidate = slack_mask.where(slack_candidate, inf_fill);
      max_step = std::min(max_step, slack_candidate.amin(0, false).item<double>());
    }
    const Series dual_mask = dual_step.lt(0.0);
    if (dual_mask.any().item<bool>()) {
      const Series safe_dual_denom = dual_mask.where(dual_step.mul(-1.0), ones);
      Series dual_candidate = dual.div(safe_dual_denom).mul(options.fraction_to_boundary);
      dual_candidate = dual_mask.where(dual_candidate, inf_fill);
      max_step = std::min(max_step, dual_candidate.amin(0, false).item<double>());
    }
  } else {
    if (!system.inequalities.empty()) {
      const Series Jdx = detail::stack_inequality_jacobian_(system, n, step_direction.platform())
                             .matmul(step_direction.reshape({n, 1}))
                             .reshape({static_cast<std::int64_t>(system.inequalities.size())});
      const Series directional_mask = Jdx.gt(0.0);
      if (directional_mask.any().item<bool>()) {
        const Series ones = pluss::ones({Jdx.numel()}, ScalarType::Float64).to(Jdx.platform());
        const Series inf_fill = ones.mul(std::numeric_limits<double>::infinity());
        const Series safe_denom = directional_mask.where(Jdx, ones);
        Series candidate = inequality_residual.mul(-1.0).div(safe_denom).mul(options.fraction_to_boundary);
        candidate = directional_mask.where(candidate, inf_fill);
        max_step = std::min(max_step, candidate.amin(0, false).item<double>());
      }
    }
  }
  max_step = std::min(max_step, trust_radius);
  if (!(max_step > 0.0) || !std::isfinite(max_step)) {
    next.converged = true;
    next.message = "internal-protocol step hit the boundary";
    return next;
  }

  double step_size = max_step;
  const double base_value = state.barrier_value;
  const double watchdog_reference =
      options.enable_watchdog
          ? detail::nonmonotone_reference_(state.barrier_history, options.nonmonotone_window, base_value)
          : base_value;
  double directional_derivative = detail::series_dot_(primal_dual_gradient, step_direction);
  if (!(directional_derivative < 0.0)) {
    directional_derivative = -std::max(1.0, detail::series_dot_(step_direction, step_direction));
  }
  bool saw_nonfinite_trial = false;
  std::int64_t complementarity_rejections = 0;
  const double complementarity_baseline =
      std::max(std::max(state.complementarity_residual, state.barrier_parameter), 1e-12);
  const auto state_layout = state.params.layout();
  const Series state_flat64 = flatten_values(state.params).to(ScalarType::Float64);

  for (std::int64_t ls_iter = 0; ls_iter < options.max_line_search_iterations; ++ls_iter) {
    if (step_size <= std::max(1e-12, options.step_tolerance)) break;
    auto trial = unflatten_values(
        state_layout,
        state_flat64.add(step_direction.mul(step_size)));
    if (options.project_bounds) trial = detail::project_bounds_(trial, problem.bounds);

    auto trial_eval = evaluate_problem(problem, trial);
    next.function_evaluations += 1;
    if (trial_eval.status == EvalStatus::Ok) {
      auto trial_system = detail::assemble_constraint_system_(problem, trial, trial_eval);
      const double trial_barrier = detail::barrier_value_(
          trial_eval,
          trial_system,
          state.barrier_parameter,
          options.equality_penalty,
          state.normalization);
      const Series trial_slack = detail::inequality_slack_(
          trial_system,
          trial.empty() ? pluss::codem::Platform::CPU : trial[0].value.platform());
      const Series trial_dual = detail::blend_dual_toward_centrality_(
          detail::update_dual_step_(dual, dual_step, step_size),
          trial_slack,
          state.barrier_parameter,
          options.dual_recentering_fraction);
      const double trial_complementarity =
          detail::complementarity_residual_(trial_slack, trial_dual, state.barrier_parameter);
      const bool current_feasible =
          state.evaluation.max_constraint_violation <= options.constraint_tolerance;
      const bool trial_feasible =
          trial_eval.max_constraint_violation <= options.constraint_tolerance;
      const bool complementarity_spike =
          std::isfinite(trial_complementarity) &&
          trial_complementarity >
              std::max(complementarity_baseline, 1e-12) *
                  std::max(options.complementarity_reject_factor, 1.0);
      // Avoid over-rejecting productive steps near a feasible point. When both
      // endpoints are feasible and barrier merit still improves, keep moving.
      const bool allow_feasible_progress =
          current_feasible && trial_feasible &&
          std::isfinite(trial_barrier) && trial_barrier < base_value;
      if (complementarity_spike && !allow_feasible_progress) {
        ++complementarity_rejections;
        step_size *= options.backtracking;
        continue;
      }
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
        next.message = "step accepted";

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
        next.inequality_slack = detail::inequality_slack_(
            next_system,
            next.params.empty() ? pluss::codem::Platform::CPU : next.params[0].value.platform());
        next.inequality_dual = detail::blend_dual_toward_centrality_(
            detail::update_dual_step_(dual, dual_step, step_size),
            next.inequality_slack,
            next.barrier_parameter,
            options.dual_recentering_fraction);
        const auto next_barrier_gradient = detail::barrier_gradient_flat_(
            next.evaluation,
            next_system,
            next.barrier_parameter,
            options.equality_penalty,
            state.normalization);
        const Series next_barrier_gradient_series =
            next_barrier_gradient.to(ScalarType::Float64).to(state.hessian.platform()).reshape({n});
        if (use_exact_curvature) {
          next.hessian = reduced_hessian.defined() ? reduced_hessian : state.hessian;
          next.lbfgs_s_history.clear();
          next.lbfgs_y_history.clear();
        } else {
          const Series y = next_barrier_gradient_series.sub(primal_dual_gradient.to(state.hessian.platform()));
          next.hessian = detail::update_hessian_(
              state.hessian,
              s,
              y,
              options.bfgs_curvature_tolerance,
              options.hessian_approximation,
              next.params.total_numel(),
              next.params[0].value.platform());
        }
        if (options.hessian_approximation == OptimaHessianApproximation::Lbfgs) {
          const Series y = next_barrier_gradient_series.sub(primal_dual_gradient.to(state.hessian.platform()));
          next.hessian = state.hessian;
          next.lbfgs_s_history = state.lbfgs_s_history;
          next.lbfgs_y_history = state.lbfgs_y_history;
          detail::update_lbfgs_history_(
              next.lbfgs_s_history,
              next.lbfgs_y_history,
              s,
              y,
              options.lbfgs_memory,
              options.bfgs_curvature_tolerance);
        } else if (!use_exact_curvature) {
          next.lbfgs_s_history = state.lbfgs_s_history;
          next.lbfgs_y_history = state.lbfgs_y_history;
        }
        next.barrier_gradient_norm = detail::series_inf_norm_(next_barrier_gradient_series);
        if (!detail::use_iterative_subproblem_solver_(
                options.linear_system_solver,
                options.system_structure,
                n,
                options.iterative_dimension_threshold)) {
          next.krylov_iterations = state.krylov_iterations;
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
            options.equality_penalty,
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

        if (detail::should_reduce_barrier_(next, options)) {
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
            next.barrier_parameter <= std::max(1e-3, 100.0 * options.minimum_barrier_parameter)) {
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
      const double soc_barrier = detail::barrier_value_(
          soc.evaluation,
          soc_system,
          state.barrier_parameter,
          options.equality_penalty,
          state.normalization);
      const bool improved_feasibility =
          soc.evaluation.max_constraint_violation + 1e-12 < soc_eval.max_constraint_violation;
      const bool improved_barrier =
          std::isfinite(soc_barrier) && soc_barrier + options.watchdog_relaxation < base_value;
      if (improved_feasibility || improved_barrier) {
        next.params = soc.params;
        next.evaluation = soc.evaluation;
        next.iteration += 1;
        next.last_step_size = soc.last_step_size;
        next.barrier_parameter = state.barrier_parameter;
        next.barrier_value = soc_barrier;
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
            options.equality_penalty,
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
      next.evaluation = recovery.evaluation;
      next.iteration += 1;
      next.last_step_size = recovery.last_step_size;
      next.barrier_parameter = state.barrier_parameter;
      const auto next_system = detail::assemble_constraint_system_(problem, next.params, next.evaluation);
      next.barrier_value = detail::barrier_value_(
          next.evaluation,
          next_system,
          next.barrier_parameter,
          options.equality_penalty,
          state.normalization);
      const auto next_barrier_gradient = detail::barrier_gradient_flat_(
          next.evaluation,
          next_system,
          next.barrier_parameter,
          options.equality_penalty,
          state.normalization);
      next.barrier_gradient_norm = detail::series_inf_norm_(next_barrier_gradient);
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
          options.equality_penalty,
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
          if (fallback_step <= std::max(1e-12, options.step_tolerance)) break;
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
            const double trial_barrier = detail::barrier_value_(
                trial_eval,
                trial_system,
                state.barrier_parameter,
                options.equality_penalty,
                state.normalization);
            if (std::isfinite(trial_obj) &&
                std::isfinite(trial_barrier) &&
                trial_obj <= base_obj + options.armijo * fallback_step * directional) {
              next.params = std::move(trial);
              next.evaluation = std::move(trial_eval);
              next.iteration += 1;
              next.last_step_size = fallback_step;
              next.barrier_parameter = state.barrier_parameter;
              next.barrier_value = trial_barrier;
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
                  options.equality_penalty,
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
        next.message = "step tolerance reached at a feasible point";
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
      next.message = complementarity_rejections > 0
                         ? "switching to feasibility phase after complementarity guard rejections"
                         : "switching to feasibility phase";
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
          options.equality_penalty,
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
  OptimaState out{
      OptimaAlgorithm::MeritMethod,
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
      out.message);
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

inline OptimaState optima(const ConstrainedProblem& problem,
                          const ParameterPack<Series>& initial_params,
                          const OptimaOptions& options = {}) {
  switch (options.algorithm) {
    case OptimaAlgorithm::MeritMethod: {
      auto penalty_options = options.penalty;
      if (!penalty_options.progress_hook) penalty_options.progress_hook = options.progress_hook;
      return to_optima_state(problem, solve_penalty_method(problem, initial_params, penalty_options));
    }
    case OptimaAlgorithm::SequentialQP: {
      auto sequential_qp_options = options.sequential_qp;
      if (!sequential_qp_options.progress_hook) sequential_qp_options.progress_hook = options.progress_hook;
      return to_optima_state(problem, solve_sequential_qp(problem, initial_params, sequential_qp_options));
    }
    case OptimaAlgorithm::WorkingSet: {
      auto working_set_options = options.working_set;
      if (!working_set_options.progress_hook) working_set_options.progress_hook = options.progress_hook;
      return to_optima_state(problem,
                             solve_working_set(problem, initial_params, working_set_options),
                             OptimaAlgorithm::WorkingSet);
    }
    case OptimaAlgorithm::InternalProtocol: {
      auto internal_protocol_options = options.internal_protocol;
      if (!internal_protocol_options.progress_hook) internal_protocol_options.progress_hook = options.progress_hook;
      return to_optima_state(problem, solve_internal_protocol(problem, initial_params, internal_protocol_options));
    }
  }

  ::pluss::throw_error("subsp::math::optim::optima: unknown algorithm");
}

inline OptimaState optima(const ConstrainedProblem& problem,
                          const Series& initial_params,
                          const OptimaOptions& options = {}) {
  return optima(problem, pack_series(initial_params), options);
}

inline OptimaState optima(
    std::function<Trace(const ParameterPack<Trace>&)> objective_autodiff,
    const ParameterPack<Series>& initial_params,
    const OptimaOptions& options = {},
    const ObjectiveConstraints& constraints = {}) {
  ConstrainedProblem problem;
  problem.objective.autodiff = std::move(objective_autodiff);
  problem.nonlinear = constraints.nonlinear;
  problem.linear = constraints.linear;
  problem.bounds = constraints.bounds;
  return optima(problem, initial_params, options);
}

inline OptimaState optima(
    std::function<Trace(const Trace&)> objective_autodiff,
    const Series& initial_params,
    const OptimaOptions& options = {},
    const ObjectiveConstraints& constraints = {}) {
  ConstrainedProblem problem;
  problem.objective.autodiff = [f = std::move(objective_autodiff)](const ParameterPack<Trace>& p) -> Trace {
    return f(p.at("x"));
  };
  problem.nonlinear = constraints.nonlinear;
  problem.linear = constraints.linear;
  problem.bounds = constraints.bounds;
  return optima(problem, pack_series(initial_params), options);
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
    st.m.push_back(pluss::zeros_like(p, p.scalar_type()));
    st.v.push_back(pluss::zeros_like(p, p.scalar_type()));
    st.max_v.push_back(pluss::zeros_like(p, p.scalar_type()));
  }
  return st;
}

inline void adam_step_inplace(std::vector<Series>& params,
                              const std::vector<Series>& sensitivities,
                              AdamState& state,
                              const AdamOptions& opt = {}) {
  if (params.size() != sensitivities.size()) {
    ::pluss::throw_error("subsp::math::optim::adam_step_inplace: params/sensitivities size mismatch");
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
        ::pluss::throw_error("subsp::math::optim::adam_step_inplace: sensitivity shape mismatch");
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

    state.m[i] = state.m[i].mul(opt.beta1).add(g.mul(1.0 - opt.beta1));
    state.v[i] = state.v[i].mul(opt.beta2).add(g.mul(g).mul(1.0 - opt.beta2));

    const Series mhat = state.m[i].div(b1_correction);
    Series vhat = state.v[i].div(b2_correction);
    if (opt.amsgrad) {
      state.max_v[i] = state.max_v[i].maximum(state.v[i]);
      vhat = state.max_v[i].div(b2_correction);
    }
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

using SeptimaObjectiveTraceFn = std::function<Trace(const std::vector<Trace>&)>;
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
    double wolfe_c2{0.9};
    LbfgsLineSearch line_search{LbfgsLineSearch::None};
  };

  Optimizer optimizer{Optimizer::Adam};
  AdamOptions adam{};
  LbfgsOptions lbfgs{};
  double dual_learning_rate{1e-2};
  double equality_penalty{10.0};
  double inequality_penalty{10.0};
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
  using TraceResidualFn = std::function<std::vector<Trace>(const std::vector<Trace>&)>;

  ResidualFn equalities{};
  ResidualFn inequalities{};
  TraceResidualFn equalities_trace{};
  TraceResidualFn inequalities_trace{};
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
  Series lbfgs_prev_grad_flat{};
  Series lbfgs_prev_step_flat{};
  std::vector<Series> lambda_equalities{};
  std::vector<Series> lambda_inequalities{};
  double constraint_violation{0.0};
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

struct SeptimaProgressSnapshot {
  std::int64_t iteration{0};
  std::int64_t function_evaluations{0};
  double objective{std::numeric_limits<double>::infinity()};
  double gradient_inf_norm{std::numeric_limits<double>::infinity()};
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

struct SeptimaResult {
  std::vector<Series> params{};
  SeptimaState state{};
  Series objective{};
  std::int64_t iteration{0};
  std::int64_t function_evaluations{0};
  double objective_value{std::numeric_limits<double>::infinity()};
  double gradient_inf_norm{std::numeric_limits<double>::infinity()};
  double step_inf_norm{0.0};
  double constraint_violation{std::numeric_limits<double>::infinity()};
  bool converged{false};
  SeptimaExitFlag exit_flag{SeptimaExitFlag::MaxIterations};
  std::string message{};
};

namespace detail {

inline bool septima_finite_(const Series& x) {
  return x.isfinite().all().item<bool>();
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
      ::pluss::throw_error("subsp::math::optim::septima_step_inplace: adam.learning_rate must be positive and finite");
    }
    if (!(options.adam.beta1 >= 0.0 && options.adam.beta1 < 1.0) || !std::isfinite(options.adam.beta1)) {
      ::pluss::throw_error("subsp::math::optim::septima_step_inplace: adam.beta1 must be in [0,1)");
    }
    if (!(options.adam.beta2 >= 0.0 && options.adam.beta2 < 1.0) || !std::isfinite(options.adam.beta2)) {
      ::pluss::throw_error("subsp::math::optim::septima_step_inplace: adam.beta2 must be in [0,1)");
    }
    if (!(options.adam.eps > 0.0) || !std::isfinite(options.adam.eps)) {
      ::pluss::throw_error("subsp::math::optim::septima_step_inplace: adam.eps must be positive and finite");
    }
    if (options.adam.weight_decay < 0.0 || !std::isfinite(options.adam.weight_decay)) {
      ::pluss::throw_error("subsp::math::optim::septima_step_inplace: adam.weight_decay must be finite and >= 0");
    }
  } else if (options.optimizer == SeptimaOptions::Optimizer::Lbfgs) {
    if (!(options.lbfgs.learning_rate > 0.0) || !std::isfinite(options.lbfgs.learning_rate)) {
      ::pluss::throw_error("subsp::math::optim::septima_step_inplace: lbfgs.learning_rate must be positive and finite");
    }
    if (options.lbfgs.history_size < 0) {
      ::pluss::throw_error("subsp::math::optim::septima_step_inplace: lbfgs.history_size must be >= 0");
    }
    if (!(options.lbfgs.curvature_tolerance >= 0.0) || !std::isfinite(options.lbfgs.curvature_tolerance)) {
      ::pluss::throw_error(
          "subsp::math::optim::septima_step_inplace: lbfgs.curvature_tolerance must be finite and >= 0");
    }
    if (options.lbfgs.max_line_search_iterations < 0) {
      ::pluss::throw_error("subsp::math::optim::septima_step_inplace: lbfgs.max_line_search_iterations must be >= 0");
    }
    if (!(options.lbfgs.tolerance_change >= 0.0) || !std::isfinite(options.lbfgs.tolerance_change)) {
      ::pluss::throw_error(
          "subsp::math::optim::septima_step_inplace: lbfgs.tolerance_change must be finite and >= 0");
    }
    if (!(options.lbfgs.wolfe_c1 > 0.0 && options.lbfgs.wolfe_c1 < 1.0) || !std::isfinite(options.lbfgs.wolfe_c1)) {
      ::pluss::throw_error("subsp::math::optim::septima_step_inplace: lbfgs.wolfe_c1 must be in (0,1)");
    }
    if (!(options.lbfgs.wolfe_c2 > options.lbfgs.wolfe_c1 && options.lbfgs.wolfe_c2 < 1.0) ||
        !std::isfinite(options.lbfgs.wolfe_c2)) {
      ::pluss::throw_error("subsp::math::optim::septima_step_inplace: lbfgs.wolfe_c2 must be in (wolfe_c1,1)");
    }
  } else {
    ::pluss::throw_error("subsp::math::optim::septima_step_inplace: unsupported septima optimizer");
  }
  if (options.dual_learning_rate < 0.0 || !std::isfinite(options.dual_learning_rate)) {
    ::pluss::throw_error("subsp::math::optim::septima_step_inplace: dual_learning_rate must be finite and >= 0");
  }
  if (options.equality_penalty < 0.0 || !std::isfinite(options.equality_penalty)) {
    ::pluss::throw_error("subsp::math::optim::septima_step_inplace: equality_penalty must be finite and >= 0");
  }
  if (options.inequality_penalty < 0.0 || !std::isfinite(options.inequality_penalty)) {
    ::pluss::throw_error("subsp::math::optim::septima_step_inplace: inequality_penalty must be finite and >= 0");
  }
  if (options.nan_recovery_attempts < 0) {
    ::pluss::throw_error("subsp::math::optim::septima_step_inplace: nan_recovery_attempts must be >= 0");
  }
  if (!(options.nan_recovery_shrink > 0.0 && options.nan_recovery_shrink < 1.0) ||
      !std::isfinite(options.nan_recovery_shrink)) {
    ::pluss::throw_error("subsp::math::optim::septima_step_inplace: nan_recovery_shrink must be in (0,1)");
  }
}

inline void septima_validate_solve_options_(const SeptimaSolveOptions& options) {
  if (options.max_iterations <= 0) {
    ::pluss::throw_error("subsp::math::optim::septima: max_iterations must be > 0");
  }
  if (options.max_no_progress_iterations < 0) {
    ::pluss::throw_error("subsp::math::optim::septima: max_no_progress_iterations must be >= 0");
  }
  if (!(options.objective_tolerance >= 0.0) || !std::isfinite(options.objective_tolerance)) {
    ::pluss::throw_error("subsp::math::optim::septima: objective_tolerance must be finite and >= 0");
  }
  if (!(options.gradient_tolerance >= 0.0) || !std::isfinite(options.gradient_tolerance)) {
    ::pluss::throw_error("subsp::math::optim::septima: gradient_tolerance must be finite and >= 0");
  }
  if (!(options.step_tolerance >= 0.0) || !std::isfinite(options.step_tolerance)) {
    ::pluss::throw_error("subsp::math::optim::septima: step_tolerance must be finite and >= 0");
  }
  if (!(options.constraint_tolerance >= 0.0) || !std::isfinite(options.constraint_tolerance)) {
    ::pluss::throw_error("subsp::math::optim::septima: constraint_tolerance must be finite and >= 0");
  }
}

inline void septima_validate_params_(const std::vector<Series>& params) {
  if (params.empty()) {
    ::pluss::throw_error("subsp::math::optim::septima_step_inplace: expected non-empty params");
  }
  for (const auto& p : params) {
    require_defined_(p, "septima_step_inplace");
    require_float_dtype_(p, "septima_step_inplace");
  }
}

inline void septima_validate_constraints_(const std::vector<Series>& params,
                                          const SeptimaConstraints& constraints,
                                          bool require_trace_callbacks) {
  (void)require_trace_callbacks;
  if (!constraints.lbs.empty() && constraints.lbs.size() != params.size()) {
    ::pluss::throw_error("subsp::math::optim::septima_step_inplace: lbs size mismatch");
  }
  if (!constraints.ubs.empty() && constraints.ubs.size() != params.size()) {
    ::pluss::throw_error("subsp::math::optim::septima_step_inplace: ubs size mismatch");
  }

  for (std::size_t i = 1; i < params.size(); ++i) {
    if (params[i].platform() != params[0].platform()) {
      ::pluss::throw_error(
          "subsp::math::optim::septima_step_inplace: constrained updates require params on one platform");
    }
  }

  if (!constraints.equalities && constraints.equalities_trace) {
    ::pluss::throw_error("subsp::math::optim::septima_step_inplace: equalities_trace requires equalities");
  }
  if (!constraints.inequalities && constraints.inequalities_trace) {
    ::pluss::throw_error("subsp::math::optim::septima_step_inplace: inequalities_trace requires inequalities");
  }

  auto check_bound = [&](const std::vector<Series>& bounds, const char* name) {
    for (std::size_t i = 0; i < bounds.size(); ++i) {
      if (!bounds[i].defined()) continue;
      require_same_platform_(params[i], bounds[i], "septima_step_inplace");
      if (!same_shape_(params[i], bounds[i])) {
        if (!(params[i].numel() == 1 && bounds[i].numel() == 1)) {
          ::pluss::throw_error(std::string("subsp::math::optim::septima_step_inplace: ") + name +
                                   " shape mismatch");
        }
      }
    }
  };
  check_bound(constraints.lbs, "lbs");
  check_bound(constraints.ubs, "ubs");
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
    ::pluss::throw_error(std::string("subsp::math::optim::septima_step_inplace: empty ") + name +
                             " residual vector");
  }
  return out;
}

inline std::vector<Trace> septima_evaluate_trace_residuals_(
    const std::vector<Trace>& vars,
    const SeptimaConstraints::ResidualFn& series_fn,
    const SeptimaConstraints::TraceResidualFn& trace_fn,
    const char* name) {
  // Prefer the Series residual callback to keep primal/dual paths aligned and avoid
  // trace-only callback disconnects.
  if (series_fn) {
    std::vector<Series> values;
    values.reserve(vars.size());
    for (const auto& v : vars) values.push_back(v.value());
    const auto residual_values = septima_evaluate_residuals_(values, series_fn, name);
    std::vector<Trace> out;
    out.reserve(residual_values.size());
    for (const auto& r : residual_values) out.emplace_back(r);
    return out;
  }

  if (!trace_fn) return {};
  const auto out = trace_fn(vars);
  for (const auto& r : out) {
    if (!r.defined()) {
      ::pluss::throw_error(std::string("subsp::math::optim::septima_step_inplace: undefined ") + name +
                           " trace residual");
    }
    require_same_platform_(vars.front().value(), r.value(), "septima_step_inplace");
    require_float_dtype_(r.value(), "septima_step_inplace");
  }
  if (out.empty()) {
    ::pluss::throw_error(std::string("subsp::math::optim::septima_step_inplace: empty ") + name +
                         " residual vector");
  }
  return out;
}

inline std::vector<Series> septima_zero_like_(const std::vector<Series>& residuals) {
  std::vector<Series> lambda;
  lambda.reserve(residuals.size());
  for (const auto& r : residuals) {
    lambda.push_back(pluss::zeros_like(r, r.scalar_type()));
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
      lambda[i] = pluss::zeros_like(residuals[i], residuals[i].scalar_type());
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
    ::pluss::throw_error("subsp::math::optim::septima_flatten_values_: expected non-empty values");
  }
  std::vector<Series> flat;
  flat.reserve(values.size());
  for (const auto& value : values) {
    detail::require_defined_(value, "septima_flatten_values_");
    flat.push_back(value.reshape({value.numel()}));
  }
  if (flat.size() == 1) return flat.front().to(ScalarType::Float64);
  return flat.front().unite(std::vector<Series>(flat.begin() + 1, flat.end()), 0).to(ScalarType::Float64);
}

inline std::vector<Series> septima_unflatten_values_(const Series& flat_values,
                                                     const std::vector<SeptimaVectorBlockLayout>& layout) {
  detail::require_defined_(flat_values, "septima_unflatten_values_");
  Series flat = flat_values.reshape({flat_values.numel()});
  std::int64_t total_numel = 0;
  for (const auto& block : layout) total_numel += block.numel;
  if (flat.numel() != total_numel) {
    ::pluss::throw_error("subsp::math::optim::septima_unflatten_values_: flat size mismatch");
  }

  std::vector<Series> out;
  out.reserve(layout.size());
  std::int64_t offset = 0;
  for (const auto& block : layout) {
    Series piece = flat.slice(0, offset, offset + block.numel, 1);
    if (!piece.is_contiguous()) piece = piece.contiguous();
    piece = block.sizes.empty() ? piece.reshape({}) : piece.reshape(block.sizes);
    if (piece.scalar_type() != block.scalar_type) piece = piece.to(block.scalar_type);
    if (piece.platform() != block.platform) piece = piece.to(block.platform);
    out.push_back(std::move(piece));
    offset += block.numel;
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

inline Trace septima_constraint_objective_trace_(const std::vector<Trace>& vars,
                                                 const SeptimaState& state,
                                                 const SeptimaConstraints& constraints,
                                                 const SeptimaOptions& options) {
  Trace objective = sum(vars.front() * 0.0);

  if (constraints.equalities) {
    const auto residuals = septima_evaluate_trace_residuals_(
        vars, constraints.equalities, constraints.equalities_trace, "equalities");
    if (residuals.size() != state.lambda_equalities.size()) {
      ::pluss::throw_error("subsp::math::optim::septima_step_inplace: equality multiplier size mismatch");
    }
    for (std::size_t i = 0; i < residuals.size(); ++i) {
      objective = objective +
                  sum(fixed(state.lambda_equalities[i]) * residuals[i]) +
                  0.5 * options.equality_penalty * sum(square(residuals[i]));
    }
  }

  if (constraints.inequalities) {
    const auto residuals = septima_evaluate_trace_residuals_(
        vars, constraints.inequalities, constraints.inequalities_trace, "inequalities");
    if (residuals.size() != state.lambda_inequalities.size()) {
      ::pluss::throw_error("subsp::math::optim::septima_step_inplace: inequality multiplier size mismatch");
    }
    for (std::size_t i = 0; i < residuals.size(); ++i) {
      const Trace violation = relu(residuals[i]);
      objective = objective +
                  sum(fixed(state.lambda_inequalities[i]) * violation) +
                  0.5 * options.inequality_penalty * sum(square(violation));
    }
  }

  return objective;
}

inline std::pair<Series, std::vector<Series>> septima_augmented_value_grad_(
    const SeptimaObjectiveTraceFn& objective_trace,
    const std::vector<Series>& params,
    const SeptimaState& state,
    const SeptimaOptions& options,
    const SeptimaConstraints& constraints) {
  if (!objective_trace) {
    ::pluss::throw_error("subsp::math::optim::septima_step_inplace: objective callback is required for line search");
  }

  if (options.enable_constraint_autodiff && (constraints.equalities || constraints.inequalities)) {
    return value_and_grad(
        [&](const std::vector<Trace>& vars) -> Trace {
          return objective_trace(vars) + septima_constraint_objective_trace_(vars, state, constraints, options);
        },
        params);
  }
  return value_and_grad(objective_trace, params);
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
    ::pluss::throw_error("subsp::math::optim::septima_step_inplace: params/sensitivities size mismatch");
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
        ::pluss::throw_error("subsp::math::optim::septima_step_inplace: sensitivity shape mismatch");
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
        [&](const std::vector<Trace>& vars) -> Trace {
          return septima_constraint_objective_trace_(vars, state, constraints, options);
        },
        params);
    (void)constraint_loss;

    if (constraint_grads.size() != grads.size()) {
      ::pluss::throw_error("subsp::math::optim::septima_step_inplace: constraint gradient size mismatch");
    }
    for (std::size_t i = 0; i < grads.size(); ++i) {
      grads[i] = grads[i].add(constraint_grads[i]);
    }
  }

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
    if (!(gtd < -options.lbfgs.tolerance_change)) {
      state.constraint_violation = septima_constraint_violation_(params, constraints);
      return 0.0;
    }

    if (options.lbfgs.line_search == SeptimaOptions::LbfgsLineSearch::StrongWolfe) {
      if (!objective_trace) {
        ::pluss::throw_error(
            "subsp::math::optim::septima_step_inplace: LBFGS strong-wolfe line search requires objective callback");
      }

      const auto [base_value, base_grad] = septima_augmented_value_grad_(
          *objective_trace,
          params,
          state,
          options,
          constraints);
      if (function_evaluations) ++(*function_evaluations);
      (void)base_grad;

      const double base_objective = base_value.item<double>();
      double step_size = options.lbfgs.learning_rate;
      const std::int64_t ls_iters = std::max<std::int64_t>(1, options.lbfgs.max_line_search_iterations);

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
        if (!septima_finite_(trial_value) || !septima_finite_(trial_grads)) {
          step_size *= options.nan_recovery_shrink;
          continue;
        }

        const double trial_objective = trial_value.item<double>();
        const Series trial_grad_flat = septima_flatten_values_(trial_grads).to(flat_params.platform());
        const double gtd_new = series_dot_(trial_grad_flat, direction);

        const bool armijo_ok =
            trial_objective <= base_objective + options.lbfgs.wolfe_c1 * step_size * gtd;
        const bool curvature_ok = std::abs(gtd_new) <= -options.lbfgs.wolfe_c2 * gtd;

        if (armijo_ok && curvature_ok) {
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

        step_size *= options.nan_recovery_shrink;
        if (step_size <= options.lbfgs.tolerance_change) break;
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
      state.constraint_violation = septima_constraint_violation_(params, constraints);
      return 0.0;
    }
  }

  double violation = 0.0;

  if (constraints.equalities) {
    const auto residuals = septima_evaluate_residuals_(params, constraints.equalities, "equalities");
    septima_align_multipliers_(state.lambda_equalities, residuals);
    for (std::size_t i = 0; i < residuals.size(); ++i) {
      violation = std::max(violation, septima_max_abs_(residuals[i]));
      state.lambda_equalities[i] = state.lambda_equalities[i].add(residuals[i].mul(options.dual_learning_rate));
    }
  }

  if (constraints.inequalities) {
    const auto residuals = septima_evaluate_residuals_(params, constraints.inequalities, "inequalities");
    septima_align_multipliers_(state.lambda_inequalities, residuals);
    for (std::size_t i = 0; i < residuals.size(); ++i) {
      const Series ineq_violation = residuals[i].maximum(0.0);
      violation = std::max(violation, septima_max_abs_(ineq_violation));
      state.lambda_inequalities[i] =
          state.lambda_inequalities[i].add(ineq_violation.mul(options.dual_learning_rate)).maximum(0.0);
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
      ::pluss::throw_error("subsp::math::optim::septima_step_inplace: undefined sensitivity in params");
    }
    sensitivities.push_back(g);
  }
  (void)septima_step_inplace_(params, sensitivities, state, options, constraints);
}

} // namespace detail

inline SeptimaResult septima(const SeptimaObjectiveTraceFn& objective_trace,
                             std::vector<Series> initial_params,
                             const SeptimaSolveOptions& solve_options = {},
                             const SeptimaConstraints& constraints = {}) {
  if (!objective_trace) {
    ::pluss::throw_error("subsp::math::optim::septima: objective callback is required");
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
    const auto [value, grads] = value_and_grad(objective_trace, out.params);
    ++out.function_evaluations;
    out.objective = value;

    if (!value.defined() || value.numel() != 1) {
      ::pluss::throw_error("subsp::math::optim::septima: objective must return a scalar Trace");
    }
    out.objective_value = value.item<double>();
    if (!std::isfinite(out.objective_value)) {
      out.iteration = iter;
      out.message = "objective became non-finite";
      out.exit_flag = SeptimaExitFlag::NonFiniteEvaluation;
      if (solve_options.stop_on_nonfinite_objective) {
        break;
      }
    }

    out.gradient_inf_norm = 0.0;
    for (const auto& g : grads) {
      out.gradient_inf_norm = std::max(out.gradient_inf_norm, detail::septima_max_abs_(g));
    }

    out.step_inf_norm = detail::septima_step_inplace_(
        out.params,
        grads,
        out.state,
        solve_options.step,
        constraints,
        &objective_trace,
        &out.function_evaluations);
    out.constraint_violation = out.state.constraint_violation;
    out.iteration = iter;

    const bool converged =
        out.gradient_inf_norm <= solve_options.gradient_tolerance &&
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
      if (objective_stalled && step_stalled) {
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

inline SeptimaResult septima(const std::function<Trace(const ParameterPack<Trace>&)>& objective_autodiff,
                             const ParameterPack<Series>& initial_params,
                             const SeptimaSolveOptions& solve_options = {},
                             const SeptimaConstraints& constraints = {}) {
  if (!objective_autodiff) {
    ::pluss::throw_error("subsp::math::optim::septima: objective callback is required");
  }
  std::vector<Series> init_values;
  std::vector<std::string> names;
  init_values.reserve(initial_params.size());
  names.reserve(initial_params.size());
  for (const auto& b : initial_params.blocks()) {
    names.push_back(b.name);
    init_values.push_back(b.value);
  }

  auto out = septima(
      [f = objective_autodiff, names = std::move(names)](const std::vector<Trace>& vars) -> Trace {
        if (vars.size() != names.size()) {
          ::pluss::throw_error("subsp::math::optim::septima: objective parameter block count mismatch");
        }
        std::vector<typename ParameterPack<Trace>::Block> blocks;
        blocks.reserve(vars.size());
        for (std::size_t i = 0; i < vars.size(); ++i) {
          blocks.push_back({names[i], vars[i]});
        }
        return f(ParameterPack<Trace>(std::move(blocks)));
      },
      std::move(init_values),
      solve_options,
      constraints);
  return out;
}

inline SeptimaResult septima(const SeptimaObjectiveSeriesFn& objective_series,
                             std::vector<Series> initial_params,
                             const SeptimaSolveOptions& solve_options = {},
                             const SeptimaConstraints& constraints = {}) {
  if (!objective_series) {
    ::pluss::throw_error("subsp::math::optim::septima: objective callback is required");
  }
  return septima(
      [f = objective_series](const std::vector<Trace>& vars) -> Trace {
        std::vector<Series> values;
        values.reserve(vars.size());
        for (const auto& v : vars) values.push_back(v.value());
        return Trace(f(values));
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
    ::pluss::throw_error("subsp::math::optim::septima: objective callback is required");
  }
  std::vector<Series> init_values;
  std::vector<std::string> names;
  init_values.reserve(initial_params.size());
  names.reserve(initial_params.size());
  for (const auto& b : initial_params.blocks()) {
    names.push_back(b.name);
    init_values.push_back(b.value);
  }

  return septima(
      [f = objective_series, names = std::move(names)](const std::vector<Trace>& vars) -> Trace {
        if (vars.size() != names.size()) {
          ::pluss::throw_error("subsp::math::optim::septima: objective parameter block count mismatch");
        }
        std::vector<typename ParameterPack<Series>::Block> blocks;
        blocks.reserve(vars.size());
        for (std::size_t i = 0; i < vars.size(); ++i) {
          blocks.push_back({names[i], vars[i].value()});
        }
        return Trace(f(ParameterPack<Series>(std::move(blocks))));
      },
      std::move(init_values),
      solve_options,
      constraints);
}

inline SeptimaResult septima(const SeptimaObjectiveTraceFn& objective_trace,
                             const Series& initial_param,
                             const SeptimaSolveOptions& solve_options = {},
                             const SeptimaConstraints& constraints = {}) {
  SeptimaResult out = septima(objective_trace, std::vector<Series>{initial_param}, solve_options, constraints);
  return out;
}

inline SeptimaResult septima(const std::function<Trace(const Trace&)>& objective_trace,
                             const Series& initial_param,
                             const SeptimaSolveOptions& solve_options = {},
                             const SeptimaConstraints& constraints = {}) {
  if (!objective_trace) {
    ::pluss::throw_error("subsp::math::optim::septima: objective callback is required");
  }
  return septima(
      [&](const std::vector<Trace>& vars) -> Trace {
        if (vars.size() != 1) {
          ::pluss::throw_error("subsp::math::optim::septima: expected one parameter block");
        }
        return objective_trace(vars[0]);
      },
      initial_param,
      solve_options,
      constraints);
}

inline SeptimaResult septima(const std::function<Series(const Series&)>& objective_series,
                             const Series& initial_param,
                             const SeptimaSolveOptions& solve_options = {},
                             const SeptimaConstraints& constraints = {}) {
  if (!objective_series) {
    ::pluss::throw_error("subsp::math::optim::septima: objective callback is required");
  }
  return septima(
      [&](const std::vector<Trace>& vars) -> Trace {
        if (vars.size() != 1) {
          ::pluss::throw_error("subsp::math::optim::septima: expected one parameter block");
        }
        return Trace(objective_series(vars[0].value()));
      },
      initial_param,
      solve_options,
      constraints);
}

inline SeptimaResult septima(const std::function<Trace(const Series&)>& objective_autodiff_series,
                             const Series& initial_param,
                             const SeptimaSolveOptions& solve_options = {},
                             const SeptimaConstraints& constraints = {}) {
  if (!objective_autodiff_series) {
    ::pluss::throw_error("subsp::math::optim::septima: objective callback is required");
  }
  return septima(
      [&](const std::vector<Trace>& vars) -> Trace {
        if (vars.size() != 1) {
          ::pluss::throw_error("subsp::math::optim::septima: expected one parameter block");
        }
        return objective_autodiff_series(vars[0].value());
      },
      initial_param,
      solve_options,
      constraints);
}

inline SeptimaResult septima(const std::function<Trace(const Trace&)>& objective_trace,
                             const ParameterPack<Series>& initial_params,
                             const SeptimaSolveOptions& solve_options,
                             const SeptimaConstraints& constraints = {}) {
  if (initial_params.size() != 1) {
    ::pluss::throw_error("subsp::math::optim::septima: single-trace objective requires one parameter block");
  }
  const auto& first = initial_params.blocks().front();
  return septima(objective_trace, initial_params.at(first.name), solve_options, constraints);
}

inline SeptimaResult septima(const std::function<Series(const Series&)>& objective_series,
                             const ParameterPack<Series>& initial_params,
                             const SeptimaSolveOptions& solve_options,
                             const SeptimaConstraints& constraints = {}) {
  if (initial_params.size() != 1) {
    ::pluss::throw_error("subsp::math::optim::septima: single-series objective requires one parameter block");
  }
  const auto& first = initial_params.blocks().front();
  return septima(objective_series, initial_params.at(first.name), solve_options, constraints);
}

inline SeptimaResult septima(const std::function<Trace(const Series&)>& objective_autodiff_series,
                             const ParameterPack<Series>& initial_params,
                             const SeptimaSolveOptions& solve_options,
                             const SeptimaConstraints& constraints = {}) {
  if (initial_params.size() != 1) {
    ::pluss::throw_error("subsp::math::optim::septima: single-series objective requires one parameter block");
  }
  const auto& first = initial_params.blocks().front();
  return septima(objective_autodiff_series, initial_params.at(first.name), solve_options, constraints);
}

} // namespace pluss::subsp::math::optim

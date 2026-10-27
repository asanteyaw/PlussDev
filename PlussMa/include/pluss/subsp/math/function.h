#pragma once

#include "MSharp/IntrusivePtr.h"
#include "pluss/codem/stack_array.h"

#include <concepts>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace pluss::subsp::math {

using Binding = std::pair<std::string_view, pluss::Series>;
class Function;

enum class Assumption : std::uint8_t {
  Real,
  Zero,
  NonZero,
  NonNegative,
  Positive,
  NonPositive,
  Negative,
};

enum class NonSmooth : std::uint8_t {
  Strict,
  Piecewise,
  Subgradient,
  Autodiff,
};

namespace detail {
class FunctionImpl;
struct FunctionAccess;
class TraceState;
} // namespace detail

class Function final {
public:
  Function() noexcept;
  Function(const Function &) noexcept;
  Function(Function &&) noexcept;
  Function &operator=(const Function &) noexcept;
  Function &operator=(Function &&) noexcept;
  ~Function();

  [[nodiscard]] bool defined() const noexcept;
  explicit operator bool() const noexcept { return defined(); }

  [[nodiscard]] std::int64_t arity() const;
  [[nodiscard]] pluss::RangeView<std::string_view> variables() const;
  [[nodiscard]] std::int64_t rank() const;
  [[nodiscard]] pluss::RangeView<std::int64_t> shape() const;
  [[nodiscard]] Function arg(std::string_view variable) const;
  [[nodiscard]] Function assume(std::string_view variable,
                                Assumption assumption) const;
  [[nodiscard]] Assumption assumption(std::string_view variable) const;

  [[nodiscard]] pluss::Series operator()() const;
  [[nodiscard]] pluss::Series
  operator()(pluss::RangeView<Binding> bindings) const;
  [[nodiscard]] pluss::Series
  operator()(std::initializer_list<Binding> bindings) const;
  [[nodiscard]] pluss::Series
  operator()(pluss::RangeView<pluss::Series> arguments) const;
  [[nodiscard]] pluss::Series
  operator()(std::initializer_list<pluss::Series> arguments) const;

  [[nodiscard]] Function add(const Function &rhs) const;
  [[nodiscard]] Function add(double rhs) const;
  [[nodiscard]] Function sub(const Function &rhs) const;
  [[nodiscard]] Function sub(double rhs) const;
  [[nodiscard]] Function mul(const Function &rhs) const;
  [[nodiscard]] Function mul(double rhs) const;
  [[nodiscard]] Function div(const Function &rhs) const;
  [[nodiscard]] Function div(double rhs) const;
  [[nodiscard]] Function pow(const Function &rhs) const;
  [[nodiscard]] Function pow(double rhs) const;

  [[nodiscard]] Function exp() const;
  [[nodiscard]] Function log() const;
  [[nodiscard]] Function log1p() const;
  [[nodiscard]] Function expm1() const;
  [[nodiscard]] Function sqrt() const;
  [[nodiscard]] Function sin() const;
  [[nodiscard]] Function cos() const;
  [[nodiscard]] Function tan() const;
  [[nodiscard]] Function sinh() const;
  [[nodiscard]] Function cosh() const;
  [[nodiscard]] Function tanh() const;
  [[nodiscard]] Function abs() const;
  [[nodiscard]] Function operator-() const;

private:
  explicit Function(
      pluss::msharp::intrusive_ptr<detail::FunctionImpl> impl) noexcept;

  pluss::msharp::intrusive_ptr<detail::FunctionImpl> impl_{};

  friend struct detail::FunctionAccess;
};

[[nodiscard]] Function function(double constant);
[[nodiscard]] Function function(const pluss::Series &constant);

[[nodiscard]] Function operator+(const Function &lhs, const Function &rhs);
[[nodiscard]] Function operator+(const Function &lhs, double rhs);
[[nodiscard]] Function operator+(double lhs, const Function &rhs);
[[nodiscard]] Function operator-(const Function &lhs, const Function &rhs);
[[nodiscard]] Function operator-(const Function &lhs, double rhs);
[[nodiscard]] Function operator-(double lhs, const Function &rhs);
[[nodiscard]] Function operator*(const Function &lhs, const Function &rhs);
[[nodiscard]] Function operator*(const Function &lhs, double rhs);
[[nodiscard]] Function operator*(double lhs, const Function &rhs);
[[nodiscard]] Function operator/(const Function &lhs, const Function &rhs);
[[nodiscard]] Function operator/(const Function &lhs, double rhs);
[[nodiscard]] Function operator/(double lhs, const Function &rhs);

namespace detail {

class Trace final {
public:
  explicit Trace(pluss::StackArray<std::string_view> variables);
  Trace(const Trace &) = delete;
  Trace &operator=(const Trace &) = delete;
  Trace(Trace &&) noexcept;
  Trace &operator=(Trace &&) noexcept;
  ~Trace();

  [[nodiscard]] std::span<const Function> arguments() const noexcept;
  void require_arity(std::size_t callable_arity) const;
  [[nodiscard]] Function finish(Function output) &&;

private:
  pluss::msharp::intrusive_ptr<TraceState> state_{};
  std::vector<Function> arguments_{};
};

template <class T> inline constexpr bool dependent_false_v = false;

template <class T, class = void> struct callable_traits {
  static constexpr bool available = false;
};

template <class R, class... Args> struct callable_traits<R(Args...), void> {
  static constexpr bool available = true;
  static constexpr std::size_t arity = sizeof...(Args);
  using result_type = R;
  using arguments_type = std::tuple<Args...>;
};

template <class R, class... Args>
struct callable_traits<R (*)(Args...), void> : callable_traits<R(Args...)> {};

template <class R, class... Args>
struct callable_traits<R (*)(Args...) noexcept, void>
    : callable_traits<R(Args...)> {};

template <class R, class... Args>
struct callable_traits<R (&)(Args...), void> : callable_traits<R(Args...)> {};

template <class R, class... Args>
struct callable_traits<R (&)(Args...) noexcept, void>
    : callable_traits<R(Args...)> {};

template <class R, class... Args>
struct callable_traits<std::function<R(Args...)>, void>
    : callable_traits<R(Args...)> {};

template <class C, class R, class... Args>
struct callable_traits<R (C::*)(Args...), void> : callable_traits<R(Args...)> {
};

template <class C, class R, class... Args>
struct callable_traits<R (C::*)(Args...) const, void>
    : callable_traits<R(Args...)> {};

template <class C, class R, class... Args>
struct callable_traits<R (C::*)(Args...) noexcept, void>
    : callable_traits<R(Args...)> {};

template <class C, class R, class... Args>
struct callable_traits<R (C::*)(Args...) const noexcept, void>
    : callable_traits<R(Args...)> {};

template <class C, class R, class... Args>
struct callable_traits<R (C::*)(Args...) &, void>
    : callable_traits<R(Args...)> {};

template <class C, class R, class... Args>
struct callable_traits<R (C::*)(Args...) const &, void>
    : callable_traits<R(Args...)> {};

template <class C, class R, class... Args>
struct callable_traits<R (C::*)(Args...) & noexcept, void>
    : callable_traits<R(Args...)> {};

template <class C, class R, class... Args>
struct callable_traits<R (C::*)(Args...) const & noexcept, void>
    : callable_traits<R(Args...)> {};

template <class C, class R, class... Args>
struct callable_traits<R (C::*)(Args...) &&, void>
    : callable_traits<R(Args...)> {};

template <class C, class R, class... Args>
struct callable_traits<R (C::*)(Args...) const &&, void>
    : callable_traits<R(Args...)> {};

template <class C, class R, class... Args>
struct callable_traits<R (C::*)(Args...) && noexcept, void>
    : callable_traits<R(Args...)> {};

template <class C, class R, class... Args>
struct callable_traits<R (C::*)(Args...) const && noexcept, void>
    : callable_traits<R(Args...)> {};

template <class Callable>
struct callable_traits<
    Callable, std::void_t<decltype(&std::remove_cvref_t<Callable>::operator())>>
    : callable_traits<decltype(&std::remove_cvref_t<Callable>::operator())> {};

template <class Result>
[[nodiscard]] Function normalize_trace_result(Result &&result) {
  using Value = std::remove_cvref_t<Result>;
  if constexpr (std::same_as<Value, Function>) {
    return std::forward<Result>(result);
  } else if constexpr (std::same_as<Value, pluss::Series>) {
    return function(result);
  } else if constexpr (std::is_arithmetic_v<Value>) {
    return function(static_cast<double>(result));
  } else {
    static_assert(dependent_false_v<Value>,
                  "math::function: callable must return math::Function, "
                  "Series, or an arithmetic scalar");
  }
}

template <class Callable, std::size_t... Indices>
[[nodiscard]] Function invoke_fixed(Callable &&callable,
                                    std::span<const Function> arguments,
                                    std::index_sequence<Indices...>) {
  static_assert(std::invocable<Callable, decltype(arguments[Indices])...>,
                "math::function: every fixed callable argument must accept "
                "math::Function");
  return normalize_trace_result(
      std::invoke(std::forward<Callable>(callable), arguments[Indices]...));
}

template <class Traits>
inline constexpr bool takes_runtime_span_v = [] {
  if constexpr (!Traits::available || Traits::arity != 1) {
    return false;
  } else {
    using Argument = std::remove_cvref_t<
        std::tuple_element_t<0, typename Traits::arguments_type>>;
    return std::same_as<Argument, std::span<const Function>>;
  }
}();

} // namespace detail

// StackArray is the public owning adapter: brace lists and lvalue ranges are
// copied before the resulting Function stores variable metadata.
template <class Callable>
[[nodiscard]] Function function(pluss::StackArray<std::string_view> variables,
                                Callable &&callable) {
  using Traits = detail::callable_traits<std::remove_cvref_t<Callable>>;
  static_assert(Traits::available,
                "math::function: use a non-overloaded fixed-arity callable or "
                "an explicitly typed std::span<const Function> callable");

  detail::Trace trace(std::move(variables));
  const std::span<const Function> arguments = trace.arguments();

  if constexpr (detail::takes_runtime_span_v<Traits>) {
    return std::move(trace).finish(detail::normalize_trace_result(
        std::invoke(std::forward<Callable>(callable), arguments)));
  } else {
    trace.require_arity(Traits::arity);
    return std::move(trace).finish(
        detail::invoke_fixed(std::forward<Callable>(callable), arguments,
                             std::make_index_sequence<Traits::arity>{}));
  }
}

template <class Callable>
[[nodiscard]] Function function(std::initializer_list<const char *> variables,
                                Callable &&callable) {
  return function(pluss::StackArray<std::string_view>(variables),
                  std::forward<Callable>(callable));
}

} // namespace pluss::subsp::math

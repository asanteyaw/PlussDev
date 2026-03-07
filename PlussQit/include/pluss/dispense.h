#pragma once

#include <any>
#include <array>
#include <sstream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "toolchain/generic_look_up.h"

namespace pluss {

using Value = ::pluss::toolchain::Value;
using Args = ::pluss::toolchain::Args;
using Kwargs = ::pluss::toolchain::Kwargs;

// -------------------------
// Member-pointer invocation (NO registry)
// -------------------------

namespace detail {

template <class T>
  inline ::pluss::toolchain::Value pack_one(T&& x) {
    using U = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<U, const char*> || std::is_same_v<U, char*>) {
      return std::string(x);
    } else if constexpr (std::is_array_v<U> && std::is_same_v<std::remove_extent_t<U>, char>) {
      // string literal / char array
      return std::string(x);
    } else if constexpr (std::is_same_v<U, std::string_view>) {
      return std::string(x);
    } else {
      return ::pluss::toolchain::Value(std::forward<T>(x));
    }
  }

template <class... Ts>
inline Args call_args(Ts&&... xs) {
  Args out;
  out.reserve(sizeof...(Ts));
  (out.emplace_back(detail::pack_one(std::forward<Ts>(xs))), ...);
  return out;
}

} // namespace detail

struct args : public Args {
  args() = default;

  template <class... Ts>
  explicit args(Ts&&... xs) {
    this->reserve(sizeof...(Ts));
    (this->emplace_back(detail::pack_one(std::forward<Ts>(xs))), ...);
  }
};

using kwargs = Kwargs;

template <class... Ts>
inline args call_args(Ts&&... xs) {
  return args(std::forward<Ts>(xs)...);
}

namespace detail {

template <class Obj, class C, class Ret, class... P>
inline std::any call_pmf_any(Obj& obj, Ret (C::*pmf)(P...), std::span<const Value> args)
{
  static_assert(std::is_base_of_v<C, std::decay_t<Obj>>,
                "dispense(pmf): member-function owner must be a base of object type");
  if (args.size() != sizeof...(P)) {
    std::ostringstream oss;
    oss << "dispense(pmf): expected " << sizeof...(P) << " positional args, got " << args.size();
    throw std::runtime_error(oss.str());
  }

  std::array<Value, sizeof...(P)> ordered{};
  for (std::size_t i = 0; i < sizeof...(P); ++i) ordered[i] = args[i];

  if constexpr (std::is_void_v<Ret>) {
    [&]<std::size_t... I>(std::index_sequence<I...>) {
      (obj.*pmf)(::pluss::toolchain::detail::any_to<P>(ordered[I])...);
    }(std::make_index_sequence<sizeof...(P)>{});
    return std::any{};
  } else {
    Ret r = [&]<std::size_t... I>(std::index_sequence<I...>) {
      return (obj.*pmf)(::pluss::toolchain::detail::any_to<P>(ordered[I])...);
    }(std::make_index_sequence<sizeof...(P)>{});
    return std::any(std::move(r));
  }
}

template <class Obj, class C, class Ret, class... P>
inline std::any call_pmf_any(const Obj& obj, Ret (C::*pmf)(P...) const, std::span<const Value> args)
{
  static_assert(std::is_base_of_v<C, std::decay_t<Obj>>,
                "dispense(pmf): member-function owner must be a base of object type");
  if (args.size() != sizeof...(P)) {
    std::ostringstream oss;
    oss << "dispense(pmf): expected " << sizeof...(P) << " positional args, got " << args.size();
    throw std::runtime_error(oss.str());
  }

  std::array<Value, sizeof...(P)> ordered{};
  for (std::size_t i = 0; i < sizeof...(P); ++i) ordered[i] = args[i];

  if constexpr (std::is_void_v<Ret>) {
    [&]<std::size_t... I>(std::index_sequence<I...>) {
      (obj.*pmf)(::pluss::toolchain::detail::any_to<P>(ordered[I])...);
    }(std::make_index_sequence<sizeof...(P)>{});
    return std::any{};
  } else {
    Ret r = [&]<std::size_t... I>(std::index_sequence<I...>) {
      return (obj.*pmf)(::pluss::toolchain::detail::any_to<P>(ordered[I])...);
    }(std::make_index_sequence<sizeof...(P)>{});
    return std::any(std::move(r));
  }
}

} // namespace detail

// Heterogeneous "list" form using Args (std::vector<std::any>)

template <class Obj, class C, class Ret, class... P>
inline std::any dispense(Obj& obj, Ret (C::*pmf)(P...), const Args& args)
{
  return detail::call_pmf_any(obj, pmf, std::span<const Value>(args.data(), args.size()));
}

template <class Obj, class C, class Ret, class... P>
inline std::any dispense(const Obj& obj, Ret (C::*pmf)(P...) const, const Args& args)
{
  return detail::call_pmf_any(obj, pmf, std::span<const Value>(args.data(), args.size()));
}

template <class Obj, class C, class Ret, class... P>
inline std::any dispense(Obj& obj, Ret (C::*pmf)(P...), std::span<const Value> args)
{
  return detail::call_pmf_any(obj, pmf, args);
}

template <class Obj, class C, class Ret, class... P>
inline std::any dispense(const Obj& obj, Ret (C::*pmf)(P...) const, std::span<const Value> args)
{
  return detail::call_pmf_any(obj, pmf, args);
}

// Fully typed tuple form. Accepts convertible tuple element types too,
// e.g. std::make_tuple("John", 29, 190) for (std::string, int, int).

template <class Obj, class C, class Ret, class... P, class... A>
inline decltype(auto) dispense(Obj& obj, Ret (C::*pmf)(P...), const std::tuple<A...>& t)
requires(sizeof...(A) == sizeof...(P) && (std::is_convertible_v<const A&, P> && ...)) {
  static_assert(std::is_base_of_v<C, std::decay_t<Obj>>,
                "dispense(pmf): member-function owner must be a base of object type");
  return std::apply([&](const A&... xs) -> decltype(auto) { return (obj.*pmf)(xs...); }, t);
}

template <class Obj, class C, class Ret, class... P, class... A>
inline decltype(auto) dispense(const Obj& obj, Ret (C::*pmf)(P...) const, const std::tuple<A...>& t)
requires(sizeof...(A) == sizeof...(P) && (std::is_convertible_v<const A&, P> && ...)) {
  static_assert(std::is_base_of_v<C, std::decay_t<Obj>>,
                "dispense(pmf): member-function owner must be a base of object type");
  return std::apply([&](const A&... xs) -> decltype(auto) { return (obj.*pmf)(xs...); }, t);
}

// -------------------------
// OPTIONAL: string-based dispatch (requires registration)
// -------------------------

template <class T, class Ret, class... P>
inline void register_method(std::string name,
                            Ret (T::*pmf)(P...),
                            std::initializer_list<std::string_view> param_names) {
  ::pluss::toolchain::register_method<T, Ret, P...>(std::move(name), pmf, param_names);
}

template <class T, class Ret, class... P>
inline void register_method(std::string name,
                            Ret (T::*pmf)(P...) const,
                            std::initializer_list<std::string_view> param_names) {
  ::pluss::toolchain::register_method<T, Ret, P...>(std::move(name), pmf, param_names);
}

template <class Obj>
inline std::any dispense(Obj& obj, std::string_view method, std::span<const Value> args) {
  return ::pluss::toolchain::invoke(obj, method, args);
}

template <class Obj>
inline std::any dispense(Obj& obj, std::string_view method, const Args& args) {
  return ::pluss::toolchain::invoke(obj, method, std::span<const Value>(args.data(), args.size()));
}

template <class Obj>
inline std::any dispense(Obj& obj, std::string_view method, const Kwargs& kwargs) {
  return ::pluss::toolchain::invoke(obj, method, kwargs);
}

template <class R, class Obj, class... CallArgs>
inline R dispense_as(Obj&& obj, CallArgs&&... call_args) {
  return std::any_cast<R>(dispense(std::forward<Obj>(obj), std::forward<CallArgs>(call_args)...));
}

} // namespace pluss

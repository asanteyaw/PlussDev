#pragma once

#include <algorithm>
#include <any>
#include <array>
#include <cstddef>
#include <exception>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include "CodeM/core/Errors.h"

namespace pluss {

class DispenseArg {
public:
  DispenseArg() = default;

  template <class T,
            std::enable_if_t<!std::is_same_v<std::decay_t<T>, DispenseArg>, int> = 0>
  DispenseArg(T&& value)
      : value_(normalize(std::forward<T>(value))) {}

  template <class T>
  bool is() const noexcept {
    return std::any_cast<T>(&value_) != nullptr;
  }

  template <class T>
  const T& as() const {
    return std::any_cast<const T&>(value_);
  }

  template <class T>
  T& as() {
    return std::any_cast<T&>(value_);
  }

  const std::any& storage() const noexcept { return value_; }
  std::any& storage() noexcept { return value_; }

private:
  template <class T>
  static std::any normalize(T&& value) {
    using X = std::decay_t<T>;
    if constexpr (std::is_same_v<X, const char*> || std::is_same_v<X, char*>) {
      return std::any(std::string(value ? value : ""));
    } else if constexpr (std::is_same_v<X, std::string_view>) {
      return std::any(std::string(value));
    } else {
      return std::any(std::forward<T>(value));
    }
  }

  std::any value_;
};

using DispenseArgs = std::vector<DispenseArg>;
using DispenseKwargs = std::unordered_map<std::string, DispenseArg>;

struct DispenseNamedArg {
  std::string key;
  DispenseArg value;
};

template <class T>
inline DispenseNamedArg kw(std::string key, T&& value) {
  return DispenseNamedArg{std::move(key), DispenseArg(std::forward<T>(value))};
}

inline DispenseKwargs kwargs(std::initializer_list<DispenseNamedArg> items) {
  DispenseKwargs out;
  out.reserve(items.size());
  for (const auto& item : items) {
    auto [it, inserted] = out.emplace(item.key, item.value);
    if (!inserted) {
      PLUSS_FAIL(std::string("dispense kwargs: duplicate key '") + item.key + "'");
    }
  }
  return out;
}

template <class... A>
inline DispenseArgs args(A&&... xs) {
  DispenseArgs out;
  out.reserve(sizeof...(A));
  if constexpr (sizeof...(A) > 0) {
    (out.emplace_back(std::forward<A>(xs)), ...);
  }
  return out;
}

template <class... Names>
constexpr auto params(Names&&... names)
    -> std::array<std::string_view, sizeof...(Names)> {
  return {std::string_view(std::forward<Names>(names))...};
}

template <class Obj>
struct DispenseTraits;

namespace detail {

struct DispenseVoidResult final {};

template <class Obj>
class RegisteredMethod {
public:
  virtual ~RegisteredMethod() = default;

  virtual std::string_view name() const noexcept = 0;

  virtual std::any invoke_mut(Obj& obj, const DispenseArgs& positional) const = 0;
  virtual std::any invoke_mut(Obj& obj, const DispenseKwargs& keyword) const = 0;

  virtual std::any invoke_const(const Obj& obj, const DispenseArgs& positional) const = 0;
  virtual std::any invoke_const(const Obj& obj, const DispenseKwargs& keyword) const = 0;
};

} // namespace detail

template <class Obj>
using DispenseMethod = std::shared_ptr<const detail::RegisteredMethod<Obj>>;

namespace detail {

template <class T, class = void>
struct has_dispense_traits : std::false_type {};

template <class T>
struct has_dispense_traits<T, std::void_t<decltype(DispenseTraits<T>::methods())>>
    : std::true_type {};

template <class T>
inline constexpr bool has_dispense_traits_v = has_dispense_traits<T>::value;

inline std::string any_type_name(const std::any& v) {
  return v.has_value() ? std::string(v.type().name()) : std::string("<empty>");
}

template <class Out>
inline std::optional<Out> numeric_cast_any(const std::any& v) {
  auto cast = [&](auto x) -> std::optional<Out> {
    return static_cast<Out>(x);
  };

  if (const auto* p = std::any_cast<signed char>(&v)) return cast(*p);
  if (const auto* p = std::any_cast<unsigned char>(&v)) return cast(*p);
  if (const auto* p = std::any_cast<short>(&v)) return cast(*p);
  if (const auto* p = std::any_cast<unsigned short>(&v)) return cast(*p);
  if (const auto* p = std::any_cast<int>(&v)) return cast(*p);
  if (const auto* p = std::any_cast<unsigned int>(&v)) return cast(*p);
  if (const auto* p = std::any_cast<long>(&v)) return cast(*p);
  if (const auto* p = std::any_cast<unsigned long>(&v)) return cast(*p);
  if (const auto* p = std::any_cast<long long>(&v)) return cast(*p);
  if (const auto* p = std::any_cast<unsigned long long>(&v)) return cast(*p);
  if (const auto* p = std::any_cast<float>(&v)) return cast(*p);
  if (const auto* p = std::any_cast<double>(&v)) return cast(*p);
  if (const auto* p = std::any_cast<long double>(&v)) return cast(*p);
  if (const auto* p = std::any_cast<bool>(&v)) return cast(*p);

  return std::nullopt;
}

template <class T>
std::enable_if_t<std::is_lvalue_reference_v<T>, T>
cast_dispense_arg(DispenseArg& arg) {
  using U = std::remove_cv_t<std::remove_reference_t<T>>;
  auto& storage = arg.storage();

  if constexpr (std::is_const_v<std::remove_reference_t<T>>) {
    if (auto* p = std::any_cast<U>(&storage)) return static_cast<const U&>(*p);
    PLUSS_FAIL(std::string("dispense argument type mismatch; expected '") +
               typeid(U).name() + "', got '" + any_type_name(storage) + "'");
  } else {
    if (auto* p = std::any_cast<U>(&storage)) return static_cast<U&>(*p);
    PLUSS_FAIL(std::string("dispense argument type mismatch; expected mutable '") +
               typeid(U).name() + "', got '" + any_type_name(storage) + "'");
  }
}

template <class T>
std::enable_if_t<!std::is_lvalue_reference_v<T>, std::remove_cv_t<std::remove_reference_t<T>>>
cast_dispense_arg(DispenseArg& arg) {
  using U = std::remove_cv_t<std::remove_reference_t<T>>;
  auto& storage = arg.storage();

  if constexpr (std::is_same_v<U, std::string_view>) {
    if (const auto* p = std::any_cast<std::string>(&storage)) return std::string_view(*p);
    if (const auto* p = std::any_cast<std::string_view>(&storage)) return *p;
    PLUSS_FAIL(std::string("dispense argument type mismatch; expected 'std::string_view', got '") +
               any_type_name(storage) + "'");
  } else if constexpr (std::is_arithmetic_v<U>) {
    if (const auto* p = std::any_cast<U>(&storage)) return *p;
    if (const auto converted = numeric_cast_any<U>(storage)) return U(*converted);
    PLUSS_FAIL(std::string("dispense argument type mismatch; expected numeric '") +
               typeid(U).name() + "', got '" + any_type_name(storage) + "'");
  } else {
    if (const auto* p = std::any_cast<U>(&storage)) return *p;
    PLUSS_FAIL(std::string("dispense argument type mismatch; expected '") +
               typeid(U).name() + "', got '" + any_type_name(storage) + "'");
  }
}

class MethodArgumentBinder {
public:
  MethodArgumentBinder(std::string_view method_name,
                       std::vector<std::string> parameter_names,
                       DispenseKwargs defaults)
      : method_name_(method_name),
        parameter_names_(std::move(parameter_names)),
        defaults_(std::move(defaults)) {
    for (std::size_t i = 0; i < parameter_names_.size(); ++i) {
      name_to_index_.emplace(parameter_names_[i], i);
    }

    for (const auto& [key, _] : defaults_) {
      if (name_to_index_.find(key) == name_to_index_.end()) {
        PLUSS_FAIL(std::string("dispense method '") + method_name_ +
                   "': default provided for unknown parameter '" + key + "'");
      }
    }
  }

  DispenseArgs bind_positional(const DispenseArgs& positional) const {
    const auto arity = parameter_names_.size();
    if (positional.size() > arity) {
      PLUSS_FAIL(std::string("dispense method '") + method_name_ +
                 "': too many positional arguments (expected " +
                 std::to_string(arity) + ", got " +
                 std::to_string(positional.size()) + ")");
    }

    DispenseArgs ordered;
    ordered.reserve(arity);

    for (std::size_t i = 0; i < arity; ++i) {
      if (i < positional.size()) {
        ordered.push_back(positional[i]);
        continue;
      }

      const auto it = defaults_.find(parameter_names_[i]);
      if (it == defaults_.end()) {
        PLUSS_FAIL(std::string("dispense method '") + method_name_ +
                   "': missing required argument '" + parameter_names_[i] + "'");
      }
      ordered.push_back(it->second);
    }

    return ordered;
  }

  DispenseArgs bind_keyword(const DispenseKwargs& keyword) const {
    const auto arity = parameter_names_.size();
    DispenseArgs ordered(arity);
    std::vector<bool> seen(arity, false);

    for (const auto& [key, value] : keyword) {
      const auto it = name_to_index_.find(key);
      if (it == name_to_index_.end()) {
        PLUSS_FAIL(std::string("dispense method '") + method_name_ +
                   "': unknown keyword argument '" + key + "'");
      }

      const auto idx = it->second;
      if (seen[idx]) {
        PLUSS_FAIL(std::string("dispense method '") + method_name_ +
                   "': duplicate keyword argument '" + key + "'");
      }
      ordered[idx] = value;
      seen[idx] = true;
    }

    for (std::size_t i = 0; i < arity; ++i) {
      if (seen[i]) continue;

      const auto def = defaults_.find(parameter_names_[i]);
      if (def == defaults_.end()) {
        PLUSS_FAIL(std::string("dispense method '") + method_name_ +
                   "': missing required argument '" + parameter_names_[i] + "'");
      }
      ordered[i] = def->second;
      seen[i] = true;
    }

    return ordered;
  }

private:
  std::string method_name_;
  std::vector<std::string> parameter_names_;
  std::unordered_map<std::string, std::size_t> name_to_index_;
  DispenseKwargs defaults_;
};

template <class Obj, class R, class... P>
class MutableMethodModel final : public RegisteredMethod<Obj> {
public:
  using method_type = R (Obj::*)(P...);

  MutableMethodModel(std::string_view method_name,
                     method_type method,
                     std::vector<std::string> parameter_names,
                     DispenseKwargs defaults)
      : name_(method_name),
        method_(method),
        binder_(method_name, std::move(parameter_names), std::move(defaults)) {}

  std::string_view name() const noexcept override { return name_; }

  std::any invoke_mut(Obj& obj, const DispenseArgs& positional) const override {
    auto ordered = binder_.bind_positional(positional);
    return invoke_impl(obj, ordered, std::index_sequence_for<P...>{});
  }

  std::any invoke_mut(Obj& obj, const DispenseKwargs& keyword) const override {
    auto ordered = binder_.bind_keyword(keyword);
    return invoke_impl(obj, ordered, std::index_sequence_for<P...>{});
  }

  std::any invoke_const(const Obj&, const DispenseArgs&) const override {
    PLUSS_FAIL(std::string("dispense method '") + name_ +
               "' is not const-callable");
  }

  std::any invoke_const(const Obj&, const DispenseKwargs&) const override {
    PLUSS_FAIL(std::string("dispense method '") + name_ +
               "' is not const-callable");
  }

private:
  template <std::size_t... I>
  std::any invoke_impl(Obj& obj,
                       DispenseArgs& ordered,
                       std::index_sequence<I...>) const {
    if constexpr (std::is_void_v<R>) {
      (obj.*method_)(cast_dispense_arg<P>(ordered[I])...);
      return std::any(DispenseVoidResult{});
    } else {
      return std::any(std::decay_t<R>((obj.*method_)(cast_dispense_arg<P>(ordered[I])...)));
    }
  }

  std::string name_;
  method_type method_;
  MethodArgumentBinder binder_;
};

template <class Obj, class R, class... P>
class ConstMethodModel final : public RegisteredMethod<Obj> {
public:
  using method_type = R (Obj::*)(P...) const;

  ConstMethodModel(std::string_view method_name,
                   method_type method,
                   std::vector<std::string> parameter_names,
                   DispenseKwargs defaults)
      : name_(method_name),
        method_(method),
        binder_(method_name, std::move(parameter_names), std::move(defaults)) {}

  std::string_view name() const noexcept override { return name_; }

  std::any invoke_mut(Obj& obj, const DispenseArgs& positional) const override {
    auto ordered = binder_.bind_positional(positional);
    return invoke_impl(obj, ordered, std::index_sequence_for<P...>{});
  }

  std::any invoke_mut(Obj& obj, const DispenseKwargs& keyword) const override {
    auto ordered = binder_.bind_keyword(keyword);
    return invoke_impl(obj, ordered, std::index_sequence_for<P...>{});
  }

  std::any invoke_const(const Obj& obj, const DispenseArgs& positional) const override {
    auto ordered = binder_.bind_positional(positional);
    return invoke_impl(obj, ordered, std::index_sequence_for<P...>{});
  }

  std::any invoke_const(const Obj& obj, const DispenseKwargs& keyword) const override {
    auto ordered = binder_.bind_keyword(keyword);
    return invoke_impl(obj, ordered, std::index_sequence_for<P...>{});
  }

private:
  template <std::size_t... I>
  std::any invoke_impl(const Obj& obj,
                       DispenseArgs& ordered,
                       std::index_sequence<I...>) const {
    if constexpr (std::is_void_v<R>) {
      (obj.*method_)(cast_dispense_arg<P>(ordered[I])...);
      return std::any(DispenseVoidResult{});
    } else {
      return std::any(std::decay_t<R>((obj.*method_)(cast_dispense_arg<P>(ordered[I])...)));
    }
  }

  std::string name_;
  method_type method_;
  MethodArgumentBinder binder_;
};

template <class Obj>
inline std::string method_list_string(
    const std::vector<DispenseMethod<Obj>>& methods) {
  std::vector<std::string> names;
  names.reserve(methods.size());
  for (const auto& method : methods) {
    if (!method) continue;
    names.emplace_back(method->name());
  }

  std::sort(names.begin(), names.end());
  names.erase(std::unique(names.begin(), names.end()), names.end());

  std::string out;
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (i) out += ", ";
    out += names[i];
  }
  return out;
}

template <class ObjRef, class... A>
std::any invoke_registered_any(ObjRef&& obj,
                               std::string_view method_name,
                               A&&... call_args) {
  using Obj = std::decay_t<ObjRef>;

  const auto& methods = DispenseTraits<Obj>::methods();
  bool matched = false;
  std::string errors;

  auto append_error = [&](const std::exception& e) {
    if (!errors.empty()) errors.append("\n---\n");
    errors.append(e.what());
  };

  auto try_call_positional = [&](const DispenseArgs& positional) -> std::optional<std::any> {
    for (const auto& method : methods) {
      if (!method || method->name() != method_name) continue;
      matched = true;
      try {
        if constexpr (std::is_const_v<std::remove_reference_t<ObjRef>>) {
          return method->invoke_const(obj, positional);
        } else {
          return method->invoke_mut(obj, positional);
        }
      } catch (const std::exception& e) {
        append_error(e);
      }
    }
    return std::nullopt;
  };

  auto try_call_keyword = [&](const DispenseKwargs& keyword) -> std::optional<std::any> {
    for (const auto& method : methods) {
      if (!method || method->name() != method_name) continue;
      matched = true;
      try {
        if constexpr (std::is_const_v<std::remove_reference_t<ObjRef>>) {
          return method->invoke_const(obj, keyword);
        } else {
          return method->invoke_mut(obj, keyword);
        }
      } catch (const std::exception& e) {
        append_error(e);
      }
    }
    return std::nullopt;
  };

  if constexpr (sizeof...(A) == 1) {
    using First = std::decay_t<std::tuple_element_t<0, std::tuple<A...>>>;
    auto packed = std::forward_as_tuple(std::forward<A>(call_args)...);

    if constexpr (std::is_same_v<First, DispenseKwargs>) {
      if (auto out = try_call_keyword(std::get<0>(std::move(packed))); out) {
        return *out;
      }
    } else if constexpr (std::is_same_v<First, DispenseArgs>) {
      if (auto out = try_call_positional(std::get<0>(std::move(packed))); out) {
        return *out;
      }
    } else {
      const auto positional = args(std::forward<A>(call_args)...);
      if (auto out = try_call_positional(positional); out) {
        return *out;
      }
    }
  } else {
    const auto positional = args(std::forward<A>(call_args)...);
    if (auto out = try_call_positional(positional); out) {
      return *out;
    }
  }

  if (!matched) {
    PLUSS_FAIL(std::string("dispense: unknown method '") + std::string(method_name) +
               "'. Registered methods: [" + method_list_string(methods) + "]");
  }

  PLUSS_FAIL(errors.empty()
                 ? std::string("dispense: method invocation failed")
                 : errors);
}

template <class R>
R cast_return_any(std::any out, std::string_view method_name) {
  if constexpr (std::is_void_v<R>) {
    if (std::any_cast<DispenseVoidResult>(&out) == nullptr) {
      PLUSS_FAIL(std::string("dispense: method '") + std::string(method_name) +
                 "' did not return void");
    }
    return;
  } else {
    static_assert(!std::is_reference_v<R>,
                  "dispense<R>: reference return types are not supported for registered methods");

    if (const auto* p = std::any_cast<R>(&out)) return *p;
    PLUSS_FAIL(std::string("dispense: return type mismatch for method '") +
               std::string(method_name) + "'; requested '" + typeid(R).name() +
               "', actual '" + any_type_name(out) + "'");
  }
}

template <class R, class ObjRef, class... A>
R invoke_registered_typed(ObjRef&& obj,
                          std::string_view method_name,
                          A&&... args) {
  return cast_return_any<R>(
      invoke_registered_any(std::forward<ObjRef>(obj),
                            method_name,
                            std::forward<A>(args)...),
      method_name);
}

} // namespace detail

template <class Obj, class R, class... P, std::size_t N>
inline DispenseMethod<Obj> bind_method(
    std::string_view method_name,
    R (Obj::*method)(P...),
    const std::array<std::string_view, N>& parameter_names,
    DispenseKwargs defaults = {}) {
  static_assert(N == sizeof...(P),
                "bind_method: parameter name count must match method arity");

  std::vector<std::string> names;
  names.reserve(N);
  for (const auto name : parameter_names) names.emplace_back(name);

  return std::make_shared<detail::MutableMethodModel<Obj, R, P...>>(
      method_name,
      method,
      std::move(names),
      std::move(defaults));
}

template <class Obj, class R, class... P, std::size_t N>
inline DispenseMethod<Obj> bind_method(
    std::string_view method_name,
    R (Obj::*method)(P...) const,
    const std::array<std::string_view, N>& parameter_names,
    DispenseKwargs defaults = {}) {
  static_assert(N == sizeof...(P),
                "bind_method: parameter name count must match method arity");

  std::vector<std::string> names;
  names.reserve(N);
  for (const auto name : parameter_names) names.emplace_back(name);

  return std::make_shared<detail::ConstMethodModel<Obj, R, P...>>(
      method_name,
      method,
      std::move(names),
      std::move(defaults));
}

// Untyped dispatch for registered user-defined classes.
template <class Obj, class... A>
std::any dispense(Obj& obj,
                  std::string_view method,
                  A&&... args)
requires(detail::has_dispense_traits_v<std::decay_t<Obj>>) {
  return detail::invoke_registered_any(obj, method, std::forward<A>(args)...);
}

template <class Obj, class... A>
std::any dispense(const Obj& obj,
                  std::string_view method,
                  A&&... args)
requires(detail::has_dispense_traits_v<std::decay_t<Obj>>) {
  return detail::invoke_registered_any(obj, method, std::forward<A>(args)...);
}

} // namespace pluss

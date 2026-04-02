#pragma once

#include <any>
#include <array>
#include <functional>
#include <mutex>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeindex>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pluss::toolchain {

using Value = std::any;
using Args = std::vector<Value>;
using Kwargs = std::unordered_map<std::string, Value>;

struct MethodSpec {
  std::vector<std::string> param_names; // length == arity
};

using Invoker = std::function<std::any(void* obj,
                                      std::span<const Value> args,
                                      const Kwargs* kwargs)>;

class GenericLookUp {
public:
  static GenericLookUp& instance() {
    static GenericLookUp r;
    return r;
  }

  void add(std::type_index ti, std::string method, Invoker inv, MethodSpec spec) {
    std::lock_guard<std::mutex> lk(mu_);
    table_[ti][std::move(method)] = Entry{std::move(inv), std::move(spec)};
  }

  const Invoker* find_invoker(std::type_index ti, std::string_view method) const {
    std::lock_guard<std::mutex> lk(mu_);
    auto it = table_.find(ti);
    if (it == table_.end()) return nullptr;
    auto it2 = it->second.find(std::string(method));
    if (it2 == it->second.end()) return nullptr;
    return &it2->second.inv;
  }

private:
  struct Entry {
    Invoker inv;
    MethodSpec spec;
  };

  mutable std::mutex mu_;
  std::unordered_map<std::type_index, std::unordered_map<std::string, Entry>> table_;
};

namespace detail {

inline std::runtime_error make_error(std::type_index ti, std::string_view method, std::string_view why) {
  std::ostringstream oss;
  oss << "dispense failed for type " << ti.name() << " method '" << method << "': " << why;
  return std::runtime_error(oss.str());
}

inline void throw_error(std::type_index ti, std::string_view method, std::string_view why) {
  throw make_error(ti, method, why);
}

inline std::vector<std::string> to_names(std::initializer_list<std::string_view> xs) {
  std::vector<std::string> out;
  out.reserve(xs.size());
  for (auto s : xs) out.emplace_back(s);
  return out;
}

template <class P>
inline P any_to(const Value& v) {
  try {
    return std::any_cast<P>(v);
  } catch (const std::bad_any_cast&) {
    std::ostringstream oss;
    oss << "bad_any_cast: expected '" << typeid(P).name() << "'";
    throw std::runtime_error(oss.str());
  }
}

inline void validate_kwargs_exact(const MethodSpec& spec, const Kwargs& kwargs) {
  for (const auto& kv : kwargs) {
    bool ok = false;
    for (const auto& pn : spec.param_names) {
      if (kv.first == pn) { ok = true; break; }
    }
    if (!ok) {
      std::ostringstream oss;
      oss << "unexpected kwarg '" << kv.first << "'";
      throw std::runtime_error(oss.str());
    }
  }
  for (const auto& pn : spec.param_names) {
    if (kwargs.find(pn) == kwargs.end()) {
      std::ostringstream oss;
      oss << "missing kwarg '" << pn << "'";
      throw std::runtime_error(oss.str());
    }
  }
}

template <std::size_t N>
inline std::array<Value, N> ordered_from_positional(std::span<const Value> args) {
  if (args.size() != N) {
    std::ostringstream oss;
    oss << "expected " << N << " positional args, got " << args.size();
    throw std::runtime_error(oss.str());
  }
  std::array<Value, N> out{};
  for (std::size_t i = 0; i < N; ++i) out[i] = args[i];
  return out;
}

template <std::size_t N>
inline std::array<Value, N> ordered_from_kwargs(const MethodSpec& spec, const Kwargs& kwargs) {
  validate_kwargs_exact(spec, kwargs);
  std::array<Value, N> out{};
  for (std::size_t i = 0; i < N; ++i) out[i] = kwargs.at(spec.param_names[i]);
  return out;
}

template <class T, class Ret, class... P, std::size_t... I>
inline std::any call_member_impl(T* self,
                                Ret (T::*pmf)(P...),
                                const std::array<Value, sizeof...(P)>& ordered,
                                std::index_sequence<I...>) {
  if constexpr (std::is_void_v<Ret>) {
    (self->*pmf)(any_to<P>(ordered[I])...);
    return std::any{};
  } else {
    Ret r = (self->*pmf)(any_to<P>(ordered[I])...);
    return std::any(std::move(r));
  }
}

template <class T, class Ret, class... P, std::size_t... I>
inline std::any call_member_impl(const T* self,
                                Ret (T::*pmf)(P...) const,
                                const std::array<Value, sizeof...(P)>& ordered,
                                std::index_sequence<I...>) {
  if constexpr (std::is_void_v<Ret>) {
    (self->*pmf)(any_to<P>(ordered[I])...);
    return std::any{};
  } else {
    Ret r = (self->*pmf)(any_to<P>(ordered[I])...);
    return std::any(std::move(r));
  }
}

} // namespace detail

// Optional registration for string-based dispatch.
template <class T, class Ret, class... P>
void register_method(std::string name,
                     Ret (T::*pmf)(P...),
                     std::initializer_list<std::string_view> param_names) {
  constexpr std::size_t arity = sizeof...(P);
  auto names = detail::to_names(param_names);
  if (names.size() != arity) {
    std::ostringstream oss;
    oss << "register_method(" << name << "): param_names size " << names.size() << " != arity " << arity;
    throw std::runtime_error(oss.str());
  }
  MethodSpec spec{names};

  Invoker inv = [pmf, spec](void* obj, std::span<const Value> args, const Kwargs* kwargs) -> std::any {
    T* self = static_cast<T*>(obj);
    if (kwargs) {
      auto ordered = detail::ordered_from_kwargs<arity>(spec, *kwargs);
      return detail::call_member_impl(self, pmf, ordered, std::make_index_sequence<arity>{});
    }
    auto ordered = detail::ordered_from_positional<arity>(args);
    return detail::call_member_impl(self, pmf, ordered, std::make_index_sequence<arity>{});
  };

  GenericLookUp::instance().add(std::type_index(typeid(T)), std::move(name), std::move(inv), std::move(spec));
}

template <class T, class Ret, class... P>
void register_method(std::string name,
                     Ret (T::*pmf)(P...) const,
                     std::initializer_list<std::string_view> param_names) {
  constexpr std::size_t arity = sizeof...(P);
  auto names = detail::to_names(param_names);
  if (names.size() != arity) {
    std::ostringstream oss;
    oss << "register_method(" << name << "): param_names size " << names.size() << " != arity " << arity;
    throw std::runtime_error(oss.str());
  }
  MethodSpec spec{names};

  Invoker inv = [pmf, spec](void* obj, std::span<const Value> args, const Kwargs* kwargs) -> std::any {
    const T* self = static_cast<const T*>(obj);
    if (kwargs) {
      auto ordered = detail::ordered_from_kwargs<arity>(spec, *kwargs);
      return detail::call_member_impl(self, pmf, ordered, std::make_index_sequence<arity>{});
    }
    auto ordered = detail::ordered_from_positional<arity>(args);
    return detail::call_member_impl(self, pmf, ordered, std::make_index_sequence<arity>{});
  };

  GenericLookUp::instance().add(std::type_index(typeid(T)), std::move(name), std::move(inv), std::move(spec));
}

template <class Obj>
std::any invoke(Obj& obj, std::string_view method, std::span<const Value> args) {
  using T = std::decay_t<Obj>;
  const auto ti = std::type_index(typeid(T));
  const Invoker* inv = GenericLookUp::instance().find_invoker(ti, method);
  if (!inv) detail::throw_error(ti, method, "method not registered");

  try {
    return (*inv)(static_cast<void*>(&obj), args, nullptr);
  } catch (const std::exception& e) {
    throw detail::make_error(ti, method, e.what());
  }
}

template <class Obj>
std::any invoke(Obj& obj, std::string_view method, const Kwargs& kwargs) {
  using T = std::decay_t<Obj>;
  const auto ti = std::type_index(typeid(T));
  const Invoker* inv = GenericLookUp::instance().find_invoker(ti, method);
  if (!inv) detail::throw_error(ti, method, "method not registered");

  try {
    return (*inv)(static_cast<void*>(&obj), {}, &kwargs);
  } catch (const std::exception& e) {
    throw detail::make_error(ti, method, e.what());
  }
}

} // namespace pluss::toolchain
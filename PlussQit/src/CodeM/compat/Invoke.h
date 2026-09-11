#ifndef PLUSS_CODEM_COMPAT_INVOKE_H
#define PLUSS_CODEM_COMPAT_INVOKE_H
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

// PlussQit core types used by the compat invoke layer
#include "CodeM/core/Scalar.h"
#include "CodeM/core/exotic_containers.h"
#include "MSharp/RangeView.h"

namespace pluss::codem { class Series; class Scalar; }

namespace pluss::codem::_compat {

struct Arg;

// Non-variadic invoke entry points: all arguments flow through a typed Arg list.
using ArgView = pluss::msharp::RangeView::Of<Arg>;
Series invoke(std::string_view op, const Series& self, ArgView args = {});
Series& invoke_(std::string_view op, Series& self, ArgView args = {});
Series& invoke_(std::string_view op, const Series& self, ArgView args = {});

// Strict-unrouted mode. When enabled, a call that reaches the generic
// "unrouted op" backstop on CPU throws instead of logging a warning and
// returning the receiver unchanged (a silent identity). ON by default; set env
// PLUSS_STRICT_UNROUTED=0 to restore the forgiving legacy behavior while a
// specific unrouted call site is being migrated.
void set_strict_unrouted(bool enabled) noexcept;
bool strict_unrouted() noexcept;


namespace detail {

// Convert SymInt -> int64 without requiring a conversion operator.
// We prefer expect_int() (Torch-style). If SymInt is symbolic, this should throw.
inline std::int64_t symint_to_i64(const pluss::codem::SymInt& x) {
  return static_cast<std::int64_t>(x.expect_int());
}

} // namespace detail

//------------------------------------------------------------------------------
// Torch-compat method backstop
//------------------------------------------------------------------------------
// Generated Series methods (from Torch YAML) forward here.
// Goal: allow the entire method-surface to *compile*, then route what we support
// to real CPU kernels (ops_internal / Series methods), and throw a precise
// NotImplemented for the rest.
//------------------------------------------------------------------------------

struct Arg {
  using IntList = std::vector<std::int64_t>;
  using SeriesList = pluss::msharp::RangeView::Of<Series>;

  using Storage = std::variant<
      std::monostate,
      const Series*,   // non-owning
      SeriesList,
      Scalar,          // by value (Torch Scalar-like)
      double,
      std::int64_t,
      bool,
      std::string,
      IntList
  >;

  Storage v;

  Arg() : v(std::monostate{}) {}

  // Series
  Arg(const Series& s) : v(&s) {}
  Arg(SeriesList xs) : v(xs) {}

  // Scalar-like
  Arg(const Scalar& s) : v(s) {}

  // Floating
  Arg(double d) : v(d) {}
  Arg(float f) : v(static_cast<double>(f)) {}

  // Integral
  Arg(std::int64_t i) : v(i) {}
  Arg(int i) : v(static_cast<std::int64_t>(i)) {}
  Arg(unsigned int i) : v(static_cast<std::int64_t>(i)) {}
  Arg(std::uint64_t i) : v(static_cast<std::int64_t>(i)) {}

  // Bool
  Arg(bool b) : v(b) {}

  // Strings
  Arg(std::string s) : v(std::move(s)) {}
  Arg(std::string_view s) : v(std::string(s)) {}
  Arg(const char* s) : v(std::string(s)) {}

  // int-list-like
  Arg(IntList il) : v(std::move(il)) {}
  Arg(std::initializer_list<std::int64_t> il) : v(IntList(il.begin(), il.end())) {}
  Arg(std::span<const std::int64_t> il) : v(IntList(il.begin(), il.end())) {}

  // RangeView container types
  // Materialize into IntList so Invoke.cpp can keep using as_intlist(...).
  Arg(pluss::msharp::RangeView::Int il) : v(IntList(il.begin(), il.end())) {}

  Arg(pluss::msharp::RangeView::SymInt il) {
    IntList out;
    out.reserve(static_cast<std::size_t>(il.size()));
    for (auto x : il) out.push_back(detail::symint_to_i64(x));
    v = std::move(out);
  }

  // Optional forms: store monostate when empty.
  Arg(pluss::msharp::RangeView::OptionalInt il) {
    if (!il.has_value()) { v = std::monostate{}; return; }
    v = IntList(il->begin(), il->end());
  }

  Arg(pluss::msharp::RangeView::OptionalSymInt il) {
    if (!il.has_value()) { v = std::monostate{}; return; }
    IntList out;
    out.reserve(static_cast<std::size_t>(il->size()));
    for (auto x : *il) out.push_back(detail::symint_to_i64(x));
    v = std::move(out);
  }

  // Enums (e.g., MemoryFormat) – store as int64 to avoid pulling in every enum header.
  template <class E, std::enable_if_t<std::is_enum_v<std::decay_t<E>>, int> = 0>
  Arg(E e) : v(static_cast<std::int64_t>(e)) {}
};

using Args = std::vector<Arg>;

// Torch-style: expose in-place ops as `const` methods on Series and route them
// through a single centralized const_cast here (like PyTorch does).
inline Series& mut_(const Series& s) {
  return const_cast<Series&>(s);
}

Series invoke_any(std::string_view op, const Series& self, const Args& args);
Series& invoke_any_(std::string_view op, Series& self, const Args& args);

} // namespace pluss::codem::_compat

#endif // PLUSS_CODEM_COMPAT_INVOKE_H

#ifndef PLUSS_CODEM_COMPAT_INVOKE_H
#define PLUSS_CODEM_COMPAT_INVOKE_H
#include <cstdint>
#include <initializer_list>
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

namespace pluss::codem { class Series; class Scalar; }

namespace pluss::codem::_compat {

// Forward declarations for template dispatch helpers. The full template
// definitions are provided later when the translation unit includes this
// header a second time with `PLUSS_CODEM_SERIES_COMPLETE` defined (see
// the bottom of this header). Declaring them here allows the generated
// Series method wrappers to call `_compat::invoke`/`invoke_` while the
// body of `pluss::codem::Series` is still being parsed.
template <class... A>
Series invoke(std::string_view op, const Series& self, A&&... a);

template <class... A>
Series& invoke_(std::string_view op, Series& self, A&&... a);

template <class... A>
Series& invoke_(std::string_view op, const Series& self, A&&... a);


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

  using Storage = std::variant<
      std::monostate,
      const Series*,   // non-owning
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

  // Scalar-like
  Arg(const Scalar& s) : v(s) {}

  // Floating
  Arg(double d) : v(d) {}
  Arg(float f) : v(static_cast<double>(f)) {}

  // Integral
  Arg(std::int64_t i) : v(i) {}
  Arg(int i) : v(static_cast<std::int64_t>(i)) {}

  // Bool
  Arg(bool b) : v(b) {}

  // Strings
  Arg(std::string s) : v(std::move(s)) {}
  Arg(std::string_view s) : v(std::string(s)) {}
  Arg(const char* s) : v(std::string(s)) {}

  // IntArrayRef-like
  Arg(IntList il) : v(std::move(il)) {}
  Arg(std::initializer_list<std::int64_t> il) : v(IntList(il.begin(), il.end())) {}
  Arg(std::span<const std::int64_t> il) : v(IntList(il.begin(), il.end())) {}

  // Torch-like exotic container types (defined in exotic_containers.h)
  // Materialize into IntList so Invoke.cpp can keep using as_intlist(...).
  Arg(pluss::codem::IntArrayRef il) : v(IntList(il.begin(), il.end())) {}

  Arg(pluss::codem::SymIntArrayRef il) {
    IntList out;
    out.reserve(static_cast<std::size_t>(il.size()));
    for (auto x : il) out.push_back(detail::symint_to_i64(x));
    v = std::move(out);
  }

  // Optional forms: store monostate when empty.
  Arg(pluss::codem::OptionalIntArrayRef il) {
    if (!il.has_value()) { v = std::monostate{}; return; }
    v = IntList(il->begin(), il->end());
  }

  Arg(pluss::codem::OptionalSymIntArrayRef il) {
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


// -----------------------------------------------------------------------------
// Template definitions
// -----------------------------------------------------------------------------
// These wrappers return Series by value (out-of-place) and therefore require the
// full definition of `pluss::codem::Series`. To avoid include cycles, we only
// compile these definitions when a translation unit explicitly indicates that
// Series is complete by defining `PLUSS_CODEM_SERIES_COMPLETE` before including
// this header a second time.
//
// Series.h does exactly that at the end of the file.
// -----------------------------------------------------------------------------

#if defined(PLUSS_CODEM_SERIES_COMPLETE)

namespace pluss::codem::_compat {

template <class... A>
inline Series invoke(std::string_view op, const Series& self, A&&... a) {
  Args args;
  args.reserve(sizeof...(A));
  (args.emplace_back(Arg(std::forward<A>(a))), ...);
  return invoke_any(op, self, args);
}

template <class... A>
inline Series& invoke_(std::string_view op, Series& self, A&&... a) {
  Args args;
  args.reserve(sizeof...(A));
  (args.emplace_back(Arg(std::forward<A>(a))), ...);
  return invoke_any_(op, self, args);
}

// Overload to support generator-emitted `Series& foo_() const` methods.
// This is the ONLY place we const_cast for in-place semantics.
template <class... A>
inline Series& invoke_(std::string_view op, const Series& self, A&&... a) {
  Args args;
  args.reserve(sizeof...(A));
  (args.emplace_back(Arg(std::forward<A>(a))), ...);
  return invoke_any_(op, mut_(self), args);
}

} // namespace pluss::codem::_compat

#endif // PLUSS_CODEM_SERIES_COMPLETE

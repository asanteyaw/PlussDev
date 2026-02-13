#pragma once
#include <cstdint>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace pluss::codem { class Series; }

namespace pluss::codem::_compat {

//------------------------------------------------------------------------------
// Torch-compat method backstop
//------------------------------------------------------------------------------
// Generated Series methods (from Torch YAML) forward here.
// Goal: allow the entire method-surface to *compile*, then route what we support
// to real CPU kernels (ops_internal / Series methods), and throw a precise
// NotImplemented for the rest.
//
// This is a temporary acceleration layer until we generate real signatures and
// bind everything to concrete kernels.
//------------------------------------------------------------------------------

struct Arg {
  using IntList = std::vector<std::int64_t>;
  using Storage = std::variant<
      std::monostate,
      const Series*,   // non-owning
      double,
      std::int64_t,
      bool,
      std::string,
      IntList
  >;

  Storage v;

  Arg() : v(std::monostate{}) {}
  Arg(const Series& s) : v(&s) {}
  Arg(double d) : v(d) {}
  Arg(float f) : v((double)f) {}
  Arg(std::int64_t i) : v(i) {}
  Arg(int i) : v((std::int64_t)i) {}
  Arg(bool b) : v(b) {}
  Arg(std::string s) : v(std::move(s)) {}
  Arg(std::string_view s) : v(std::string(s)) {}
  Arg(const char* s) : v(std::string(s)) {}
  Arg(IntList il) : v(std::move(il)) {}
  Arg(std::initializer_list<std::int64_t> il) : v(IntList(il.begin(), il.end())) {}
  Arg(std::span<const std::int64_t> il) : v(IntList(il.begin(), il.end())) {}
};

using Args = std::vector<Arg>;

Series invoke_any(std::string_view op, const Series& self, const Args& args);
Series& invoke_any_(std::string_view op, Series& self, const Args& args);

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

} // namespace pluss::codem::_compat

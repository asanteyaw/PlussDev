#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "CodeN/core/Errors.h"

// Forward declarations only.
// We intentionally avoid including Note headers here to prevent include
// cycles with generated inline wrappers.
namespace pluss::coden {
class Note;
class TokenBatch;
} // namespace pluss::coden

namespace pluss::coden::_compat {

// A small, type-safe dispatcher for CodeN.
//
// Design goals:
// - Stable API surface for codegen (Note wrappers call into this)
// - No circular type problems (Slim headers only)
// - Clear errors when op/args mismatch

struct CodenValue {
  using IntList = ::std::vector<::std::int64_t>;

  using Storage = ::std::variant<
      ::std::monostate,
      bool,
      ::std::int64_t,
      double,
      ::std::string,
      IntList,
      const ::pluss::coden::Note*>;

  Storage v{};

  CodenValue() = default;
  CodenValue(bool b) : v(b) {}
  CodenValue(::std::int64_t i) : v(i) {}
  CodenValue(double d) : v(d) {}
  CodenValue(::std::string_view s) : v(::std::string(s)) {}
  CodenValue(::std::string s) : v(::std::move(s)) {}
  CodenValue(IntList xs) : v(::std::move(xs)) {}
  CodenValue(const ::pluss::coden::Note& n) : v(&n) {}
};

using CodenValues = ::std::vector<CodenValue>;

// Core entry points used by wrappers/codegen.
::pluss::coden::Note  call_note (::std::string_view op, const ::pluss::coden::Note&  self, const CodenValues& args);

// Additional typed entry points.
TokenBatch call_tokenbatch_note(::std::string_view op, const ::pluss::coden::Note& self, const CodenValues& args);

} // namespace pluss::coden::_compat

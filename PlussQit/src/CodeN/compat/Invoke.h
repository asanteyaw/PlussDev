#pragma once

#include <cstdint>
#include <initializer_list>
#include <string>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "CodeN/compat/Dispatch.h"
#include "MSharp/RangeView.h"

namespace pluss::coden::_compat {

struct Arg {
  using IntList = CodenValue::IntList;
  CodenValue v{};

  Arg() = default;

  Arg(const CodenValue& value) : v(value) {}
  Arg(CodenValue&& value) : v(::std::move(value)) {}

  Arg(const ::pluss::coden::Note& value) : v(value) {}

  Arg(bool value) : v(value) {}

  Arg(::std::int64_t value) : v(value) {}
  Arg(int value) : v(static_cast<::std::int64_t>(value)) {}
  Arg(unsigned int value) : v(static_cast<::std::int64_t>(value)) {}
  Arg(::std::uint64_t value) : v(static_cast<::std::int64_t>(value)) {}

  Arg(double value) : v(value) {}
  Arg(float value) : v(static_cast<double>(value)) {}

  Arg(::std::string_view value) : v(value) {}
  Arg(::std::string value) : v(::std::move(value)) {}
  Arg(const char* value) : v(::std::string_view(value)) {}

  Arg(IntList values) : v(::std::move(values)) {}
  Arg(::std::initializer_list<::std::int64_t> values)
      : v(IntList(values.begin(), values.end())) {}
  Arg(::std::span<const ::std::int64_t> values)
      : v(IntList(values.begin(), values.end())) {}
  Arg(::pluss::msharp::RangeView::Int values)
      : v(IntList(values.begin(), values.end())) {}
};

using ArgView = ::pluss::msharp::RangeView::Of<Arg>;

template <class T>
inline constexpr bool always_false_v = false;

// Unified typed invoke for CodeN.
// Code-generated wrappers should call invoke<Return>(op, self, args_view).
//
// NOTE: This is intentionally header-only so codegen wrappers remain tiny and
// don't require separate compilation units.
template <class R, class Self>
inline R invoke(::std::string_view op, const Self& self, ArgView args_view = {}) {
  CodenValues args;
  args.reserve(static_cast<::std::size_t>(args_view.size()));
  for (const Arg& a : args_view) {
    args.push_back(a.v);
  }

  if constexpr (::std::is_same_v<Self, ::pluss::coden::Note>) {
    if constexpr (::std::is_same_v<R, ::pluss::coden::Note>) {
      return call_note(op, self, args);
    } else if constexpr (::std::is_same_v<R, TokenBatch>) {
      return call_tokenbatch_note(op, self, args);
    } else {
      static_assert(always_false_v<R>, "Unsupported (Note -> R) in CodeN invoke");
    }
  } else {
    static_assert(always_false_v<Self>, "Unsupported self type for CodeN invoke");
  }
}

} // namespace pluss::coden::_compat

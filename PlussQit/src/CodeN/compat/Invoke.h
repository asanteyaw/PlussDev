#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "CodeN/compat/Dispatch.h"

namespace pluss::coden::_compat {

// Unified typed invoke for CodeN.
// Code-generated wrappers should call invoke<Return>(op, self, args...).

inline void push_arg(CodenValues& args, const CodenValue& a) { args.push_back(a); }
inline void push_arg(CodenValues& args, CodenValue&& a) { args.push_back(::std::move(a)); }
inline void push_arg(CodenValues& args, ::std::span<const ::std::int64_t> xs) {
  CodenValue::IntList v(xs.begin(), xs.end());
  args.emplace_back(CodenValue(::std::move(v)));
}

template <class A>
inline void push_arg(CodenValues& args, A&& a) {
  args.emplace_back(CodenValue(::std::forward<A>(a)));
}

// Typed invoke: routes to the correct dispatcher based on (Self, Return).
//
// NOTE: This is intentionally header-only so codegen wrappers remain tiny and
// don't require separate compilation units.

template <class R, class Self, class... A>
inline R invoke(::std::string_view op, const Self& self, A&&... a) {
  CodenValues args;
  args.reserve(sizeof...(A));
  (push_arg(args, ::std::forward<A>(a)), ...);

  if constexpr (::std::is_same_v<Self, ::pluss::coden::Chord>) {
    if constexpr (::std::is_same_v<R, ::pluss::coden::Chord>) {
      return call_chord(op, self, args);
    } else if constexpr (::std::is_same_v<R, bool>) {
      return call_bool_chord(op, self, args);
    } else if constexpr (::std::is_same_v<R, TokenBatch>) {
      return call_tokenbatch_chord(op, self, args);
    } else {
      static_assert(!sizeof(R), "Unsupported (Chord -> R) in CodeN invoke");
    }
  } else if constexpr (::std::is_same_v<Self, ::pluss::coden::Note>) {
    if constexpr (::std::is_same_v<R, ::pluss::coden::Note>) {
      return call_note(op, self, args);
    } else if constexpr (::std::is_same_v<R, ::pluss::coden::Chord>) {
      return call_chord_note(op, self, args);
    } else if constexpr (::std::is_same_v<R, ::std::vector<::std::uint8_t>>) {
      return call_u8list_note(op, self, args);
    } else if constexpr (::std::is_same_v<R, ::std::vector<::std::int64_t>>) {
      return call_i64list_note(op, self, args);
    } else if constexpr (::std::is_same_v<R, TokenBatch>) {
      return call_tokenbatch_note(op, self, args);
    } else {
      static_assert(!sizeof(R), "Unsupported (Note -> R) in CodeN invoke");
    }
  } else {
    static_assert(!sizeof(Self), "Unsupported self type for CodeN invoke");
  }
}

} // namespace pluss::coden::_compat

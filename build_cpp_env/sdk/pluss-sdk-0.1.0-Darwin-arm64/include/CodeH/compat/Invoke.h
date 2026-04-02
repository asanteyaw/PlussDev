#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "CodeH/compat/Dispatch.h"

namespace pluss::codeh::_compat {

//------------------------------------------------------------------------------
// Lightweight invoke layer for CodeH, mirroring the design philosophy of
// CodeM/CodeN generated methods.
//
// Public methods on Timeline/Duration/TimeIndex call invoke<Return>(op, self, ...)
// which funnels through a single dispatcher.
//------------------------------------------------------------------------------

struct Arg {
  using Storage = std::variant<
      std::monostate,
      const ::pluss::codeh::Timeline*,
      const ::pluss::codeh::Duration*,
      const ::pluss::codeh::TimeIndex*,
      const ::pluss::codeh::CalendarOffset*,
      const ::pluss::codem::Series*,
      std::int64_t,
      bool,
      std::string
  >;

  Storage v;
  Arg() : v(std::monostate{}) {}
  Arg(const ::pluss::codeh::Timeline& x) : v(&x) {}
  Arg(const ::pluss::codeh::Duration& x) : v(&x) {}
  Arg(const ::pluss::codeh::TimeIndex& x) : v(&x) {}
  Arg(const ::pluss::codeh::CalendarOffset& x) : v(&x) {}
  Arg(const ::pluss::codem::Series& x) : v(&x) {}
  Arg(std::int64_t x) : v(x) {}
  Arg(int x) : v(static_cast<std::int64_t>(x)) {}
  Arg(bool x) : v(x) {}
  Arg(std::string x) : v(std::move(x)) {}
  Arg(std::string_view x) : v(std::string(x)) {}
  Arg(const char* x) : v(std::string(x ? x : "")) {}

  template <class E, std::enable_if_t<std::is_enum_v<std::decay_t<E>>, int> = 0>
  Arg(E e) : v(static_cast<std::int64_t>(e)) {}
};

struct Args {
  std::vector<Arg> xs;
};

inline void push_arg(Args& a, const Arg& x) { a.xs.push_back(x); }
inline void push_arg(Args& a, Arg&& x) { a.xs.push_back(std::move(x)); }

template <class A>
inline void push_arg(Args& a, A&& x) {
  a.xs.emplace_back(Arg(std::forward<A>(x)));
}

template <class R, class Self, class... A>
inline R invoke(std::string_view op, const Self& self, A&&... a) {
  Args args;
  args.xs.reserve(sizeof...(A));
  (push_arg(args, std::forward<A>(a)), ...);

  if constexpr (std::is_same_v<Self, ::pluss::codeh::Timeline>) {
    if constexpr (std::is_same_v<R, ::pluss::codeh::Timeline>) {
      return call_timeline(op, self, args);
    } else if constexpr (std::is_same_v<R, ::pluss::codeh::Duration>) {
      return call_duration_from_timeline(op, self, args);
    } else if constexpr (std::is_same_v<R, ::pluss::codem::Series>) {
      return call_series_timeline(op, self, args);
    } else if constexpr (std::is_same_v<R, ::pluss::coden::Note>) {
      return call_note_timeline(op, self, args);
    } else if constexpr (std::is_same_v<R, std::string>) {
      return call_string_timeline(op, self, args);
    } else if constexpr (std::is_same_v<R, bool>) {
      return call_bool_timeline(op, self, args);
    } else if constexpr (std::is_same_v<R, std::vector<std::int64_t>>) {
      return call_i64vec_timeline(op, self, args);
    } else if constexpr (std::is_same_v<R, ::pluss::codeh::TimeIndex>) {
      return call_timeindex_from_timeline(op, self, args);
    } else {
      static_assert(!sizeof(R), "Unsupported (Timeline -> R) in CodeH invoke");
    }
  } else if constexpr (std::is_same_v<Self, ::pluss::codeh::Duration>) {
    if constexpr (std::is_same_v<R, ::pluss::codeh::Duration>) {
      return call_duration(op, self, args);
    } else if constexpr (std::is_same_v<R, ::pluss::codem::Series>) {
      return call_series_duration(op, self, args);
    } else if constexpr (std::is_same_v<R, ::pluss::coden::Note>) {
      return call_note_duration(op, self, args);
    } else if constexpr (std::is_same_v<R, std::vector<std::int64_t>>) {
      return call_i64vec_duration(op, self, args);
    } else {
      static_assert(!sizeof(R), "Unsupported (Duration -> R) in CodeH invoke");
    }
  } else if constexpr (std::is_same_v<Self, ::pluss::codeh::TimeIndex>) {
    if constexpr (std::is_same_v<R, ::pluss::codem::Series>) {
      return call_series_timeindex(op, self, args);
    } else if constexpr (std::is_same_v<R, std::int64_t>) {
      return call_i64_timeindex(op, self, args);
    } else if constexpr (std::is_same_v<R, bool>) {
      return call_bool_timeindex(op, self, args);
    } else if constexpr (std::is_same_v<R, std::string>) {
      return call_string_timeindex(op, self, args);
    } else if constexpr (std::is_same_v<R, ::pluss::coden::Note>) {
      return call_note_timeindex(op, self, args);
    } else if constexpr (std::is_same_v<R, ::pluss::codeh::Timeline>) {
      return call_timeline_from_timeindex(op, self, args);
    } else if constexpr (std::is_same_v<R, ::pluss::codeh::TimeIndex>) {
      return call_timeindex(op, self, args);
    } else if constexpr (std::is_same_v<R, ::pluss::codeh::TimeIndex::ResampleBins>) {
      return call_resamplebins_timeindex(op, self, args);
    } else if constexpr (std::is_same_v<R, ::pluss::codeh::TimeIndex::AlignUnion>) {
      return call_alignunion_timeindex(op, self, args);
    } else {
      static_assert(!sizeof(R), "Unsupported (TimeIndex -> R) in CodeH invoke");
    }
  } else {
    static_assert(!sizeof(Self), "Unsupported self type for CodeH invoke");
  }
}

} // namespace pluss::codeh::_compat

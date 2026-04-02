#pragma once

#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include <exception>

#include "CodeH/compat/InvokeFwd.h"
#include "CodeH/compat/Dispatch.h"
#include "CodeH/core/Errors.h"

namespace pluss::codeh::_compat {

//------------------------------------------------------------------------------
// Lightweight invoke layer for CodeH, mirroring the design philosophy of
// CodeM/CodeN generated methods.
//
// Public methods on Timeline/Duration/TimeIndex call invoke<Return>(op, self, ...)
// which funnels through a single dispatcher.
//------------------------------------------------------------------------------

struct Args {
  std::vector<Arg> xs;
};

template <class T>
inline constexpr bool always_false_v = false;

template <class R, class Self>
inline R invoke(std::string_view op, const Self& self, ArgView args_view) {
  const std::string op_snapshot(op);
  Args args;
  args.xs.reserve(static_cast<std::size_t>(args_view.size()));
  for (const Arg& arg : args_view) {
    args.xs.push_back(arg);
  }
  try {
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
        static_assert(always_false_v<R>, "Unsupported (Timeline -> R) in CodeH invoke");
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
        static_assert(always_false_v<R>, "Unsupported (Duration -> R) in CodeH invoke");
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
        static_assert(always_false_v<R>, "Unsupported (TimeIndex -> R) in CodeH invoke");
      }
    } else {
      static_assert(always_false_v<Self>, "Unsupported self type for CodeH invoke");
    }
  } catch (const ::pluss::Error&) {
    throw;
  } catch (const std::exception& e) {
    ::pluss::codem::throw_error(
        std::string("CodeH dispatch failed for '") + op_snapshot + "': " + e.what());
  } catch (...) {
    ::pluss::codem::throw_error(
        std::string("CodeH dispatch failed for '") + op_snapshot + "': unknown non-std exception");
  }
}

template <class R, class Self>
inline R invoke(std::string_view op, const Self& self, std::initializer_list<Arg> args) {
  return invoke<R>(op, self, ArgView(args.begin(), args.size()));
}

} // namespace pluss::codeh::_compat

#pragma once

#include <cstdint>
#include <exception>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "CodeH/compat/Dispatch.h"
#include "CodeM/compat/Invoke.h"
#include "CodeM/core/Errors.h"
#include "CodeN/compat/Dispatch.h"
#include "pluss/queue.h"

namespace pluss {

class DispenseValue {
public:
  using storage_type = std::variant<
      ::pluss::Series,
      ::pluss::Note,
      ::pluss::codeh::Timeline,
      ::pluss::codeh::Duration,
      ::pluss::codeh::TimeIndex,
      ::pluss::TokenBatch,
      bool,
      std::int64_t,
      std::string,
      std::vector<std::int64_t>,
      std::vector<std::uint8_t>,
      ::pluss::codeh::TimeIndex::ResampleBins,
      ::pluss::codeh::TimeIndex::AlignUnion>;

  DispenseValue(const ::pluss::Series& v) : v_(v) {}
  DispenseValue(::pluss::Series&& v) : v_(std::move(v)) {}
  DispenseValue(const ::pluss::Note& v) : v_(v) {}
  DispenseValue(::pluss::Note&& v) : v_(std::move(v)) {}
  DispenseValue(const ::pluss::codeh::Timeline& v) : v_(v) {}
  DispenseValue(::pluss::codeh::Timeline&& v) : v_(std::move(v)) {}
  DispenseValue(const ::pluss::codeh::Duration& v) : v_(v) {}
  DispenseValue(::pluss::codeh::Duration&& v) : v_(std::move(v)) {}
  DispenseValue(const ::pluss::codeh::TimeIndex& v) : v_(v) {}
  DispenseValue(::pluss::codeh::TimeIndex&& v) : v_(std::move(v)) {}
  DispenseValue(const ::pluss::TokenBatch& v) : v_(v) {}
  DispenseValue(::pluss::TokenBatch&& v) : v_(std::move(v)) {}
  DispenseValue(bool v) : v_(v) {}
  DispenseValue(std::int64_t v) : v_(v) {}
  DispenseValue(std::string v) : v_(std::move(v)) {}
  DispenseValue(std::vector<std::int64_t> v) : v_(std::move(v)) {}
  DispenseValue(std::vector<std::uint8_t> v) : v_(std::move(v)) {}
  DispenseValue(const ::pluss::codeh::TimeIndex::ResampleBins& v) : v_(v) {}
  DispenseValue(::pluss::codeh::TimeIndex::ResampleBins&& v) : v_(std::move(v)) {}
  DispenseValue(const ::pluss::codeh::TimeIndex::AlignUnion& v) : v_(v) {}
  DispenseValue(::pluss::codeh::TimeIndex::AlignUnion&& v) : v_(std::move(v)) {}

  template <class T>
  bool is() const noexcept {
    return std::holds_alternative<T>(v_);
  }

  template <class T>
  const T& as() const {
    return std::get<T>(v_);
  }

  template <class T>
  T& as() {
    return std::get<T>(v_);
  }

private:
  storage_type v_;
};

namespace detail {

template <class T>
inline constexpr bool always_false_v = false;

template <class... A>
inline ::pluss::codem::_compat::Args make_codem_args(A&&... a) {
  ::pluss::codem::_compat::Args out;
  out.reserve(sizeof...(A));
  if constexpr (sizeof...(A) > 0) {
    (out.emplace_back(::pluss::codem::_compat::Arg(std::forward<A>(a))), ...);
  }
  return out;
}

template <class... A>
inline ::pluss::coden::_compat::CodenValues make_coden_args(A&&... a) {
  auto to_coden_value = [](auto&& x) -> ::pluss::coden::_compat::CodenValue {
    using X = std::decay_t<decltype(x)>;
    if constexpr (std::is_same_v<X, const char*> || std::is_same_v<X, char*>) {
      return ::pluss::coden::_compat::CodenValue(std::string_view(x ? x : ""));
    } else {
      return ::pluss::coden::_compat::CodenValue(std::forward<decltype(x)>(x));
    }
  };

  ::pluss::coden::_compat::CodenValues out;
  out.reserve(sizeof...(A));
  if constexpr (sizeof...(A) > 0) {
    (out.emplace_back(to_coden_value(std::forward<A>(a))), ...);
  }
  return out;
}

template <class... A>
inline ::pluss::codeh::_compat::Args make_codeh_args(A&&... a) {
  ::pluss::codeh::_compat::Args out;
  out.xs.reserve(sizeof...(A));
  if constexpr (sizeof...(A) > 0) {
    (out.xs.emplace_back(::pluss::codeh::_compat::Arg(std::forward<A>(a))), ...);
  }
  return out;
}

template <class... A>
inline std::vector<::pluss::Queue::Arg> make_queue_args(A&&... a) {
  std::vector<::pluss::Queue::Arg> out;
  out.reserve(sizeof...(A));
  if constexpr (sizeof...(A) > 0) {
    (out.emplace_back(::pluss::Queue::Arg(std::forward<A>(a))), ...);
  }
  return out;
}

inline std::string_view normalize_timeline_method(std::string_view op) {
  if (op == "to_timeindex") return "make_timeindex";

  if (op == "year") return "dt_year";
  if (op == "month") return "dt_month";
  if (op == "day") return "dt_day";
  if (op == "hour") return "dt_hour";
  if (op == "minute") return "dt_minute";
  if (op == "second") return "dt_second";
  if (op == "quarter") return "dt_quarter";
  if (op == "weekday" || op == "dayofweek" || op == "day_of_week") return "dt_weekday";
  if (op == "dayofyear" || op == "day_of_year") return "dt_dayofyear";
  if (op == "week" || op == "weekofyear" || op == "week_of_year") return "dt_week";

  return op;
}

inline std::string_view normalize_timeindex_method(std::string_view op) {
  if (op == "sort_values") return "sort";
  if (op == "drop_duplicates") return "unique";
  if (op == "freq") return "infer_freq";
  return op;
}

template <class F>
inline std::optional<DispenseValue> try_dispense(F&& fn, std::string& errors) {
  try {
    return DispenseValue(fn());
  } catch (const std::exception& e) {
    if (!errors.empty()) errors.append("\n---\n");
    errors.append(e.what());
    return std::nullopt;
  }
}

} // namespace detail

template <class R, class Obj, class... A>
R dispense(const Obj& obj, std::string_view method, A&&... args) {
  using T = std::decay_t<Obj>;

  if constexpr (std::is_same_v<T, ::pluss::Queue>) {
    static_assert(std::is_same_v<R, ::pluss::Queue>,
                  "dispense<Queue>(Queue, ...) requires R=pluss::Queue");
    auto qargs = detail::make_queue_args(std::forward<A>(args)...);
    return obj.invoke(method, std::span<const ::pluss::Queue::Arg>(qargs.data(), qargs.size()));
  } else if constexpr (std::is_same_v<T, ::pluss::Series>) {
    static_assert(std::is_same_v<R, ::pluss::Series>,
                  "dispense<Series>(Series, ...) currently returns pluss::Series");
    return ::pluss::codem::_compat::invoke_any(
        method,
        obj,
        detail::make_codem_args(std::forward<A>(args)...));
  } else if constexpr (std::is_same_v<T, ::pluss::Note>) {
    const auto cargs = detail::make_coden_args(std::forward<A>(args)...);
    if constexpr (std::is_same_v<R, ::pluss::Note>) {
      return ::pluss::coden::_compat::call_note(method, obj, cargs);
    } else if constexpr (std::is_same_v<R, std::vector<std::uint8_t>>) {
      return ::pluss::coden::_compat::call_u8list_note(method, obj, cargs);
    } else if constexpr (std::is_same_v<R, std::vector<std::int64_t>>) {
      return ::pluss::coden::_compat::call_i64list_note(method, obj, cargs);
    } else if constexpr (std::is_same_v<R, ::pluss::TokenBatch>) {
      return ::pluss::coden::_compat::call_tokenbatch_note(method, obj, cargs);
    } else {
      static_assert(detail::always_false_v<R>,
                    "dispense<R>(Note, ...): unsupported return type R");
    }
  } else if constexpr (std::is_same_v<T, ::pluss::codeh::Timeline>) {
#if defined(CODEH_ENABLE_TIMELINE_SURFACE) || defined(CODEH_BUILD_MAIN_LIB)
    const auto op = detail::normalize_timeline_method(method);
    const auto hargs = detail::make_codeh_args(std::forward<A>(args)...);
    if constexpr (std::is_same_v<R, ::pluss::codeh::Timeline>) {
      return ::pluss::codeh::_compat::call_timeline(op, obj, hargs);
    } else if constexpr (std::is_same_v<R, ::pluss::codeh::Duration>) {
      return ::pluss::codeh::_compat::call_duration_from_timeline(op, obj, hargs);
    } else if constexpr (std::is_same_v<R, ::pluss::Series>) {
      return ::pluss::codeh::_compat::call_series_timeline(op, obj, hargs);
    } else if constexpr (std::is_same_v<R, ::pluss::Note>) {
      return ::pluss::codeh::_compat::call_note_timeline(op, obj, hargs);
    } else if constexpr (std::is_same_v<R, std::string>) {
      return ::pluss::codeh::_compat::call_string_timeline(op, obj, hargs);
    } else if constexpr (std::is_same_v<R, bool>) {
      return ::pluss::codeh::_compat::call_bool_timeline(op, obj, hargs);
    } else if constexpr (std::is_same_v<R, std::vector<std::int64_t>>) {
      return ::pluss::codeh::_compat::call_i64vec_timeline(op, obj, hargs);
    } else if constexpr (std::is_same_v<R, ::pluss::codeh::TimeIndex>) {
      return ::pluss::codeh::_compat::call_timeindex_from_timeline(op, obj, hargs);
    } else {
      static_assert(detail::always_false_v<R>,
                    "dispense<R>(Timeline, ...): unsupported return type R");
    }
#else
    (void)obj;
    (void)method;
    PLUSS_FAIL("dispense(Timeline): CodeH dispense is unavailable in this build. Link with PlussQit::CodeH.");
#endif
  } else if constexpr (std::is_same_v<T, ::pluss::codeh::TimeIndex>) {
#if defined(CODEH_ENABLE_TIMELINE_SURFACE) || defined(CODEH_BUILD_MAIN_LIB)
    const auto op = detail::normalize_timeindex_method(method);
    const auto hargs = detail::make_codeh_args(std::forward<A>(args)...);
    if constexpr (std::is_same_v<R, ::pluss::Series>) {
      return ::pluss::codeh::_compat::call_series_timeindex(op, obj, hargs);
    } else if constexpr (std::is_same_v<R, std::int64_t>) {
      return ::pluss::codeh::_compat::call_i64_timeindex(op, obj, hargs);
    } else if constexpr (std::is_same_v<R, bool>) {
      return ::pluss::codeh::_compat::call_bool_timeindex(op, obj, hargs);
    } else if constexpr (std::is_same_v<R, std::string>) {
      return ::pluss::codeh::_compat::call_string_timeindex(op, obj, hargs);
    } else if constexpr (std::is_same_v<R, ::pluss::Note>) {
      return ::pluss::codeh::_compat::call_note_timeindex(op, obj, hargs);
    } else if constexpr (std::is_same_v<R, ::pluss::codeh::Timeline>) {
      return ::pluss::codeh::_compat::call_timeline_from_timeindex(op, obj, hargs);
    } else if constexpr (std::is_same_v<R, ::pluss::codeh::TimeIndex>) {
      return ::pluss::codeh::_compat::call_timeindex(op, obj, hargs);
    } else if constexpr (std::is_same_v<R, ::pluss::codeh::TimeIndex::ResampleBins>) {
      return ::pluss::codeh::_compat::call_resamplebins_timeindex(op, obj, hargs);
    } else if constexpr (std::is_same_v<R, ::pluss::codeh::TimeIndex::AlignUnion>) {
      return ::pluss::codeh::_compat::call_alignunion_timeindex(op, obj, hargs);
    } else {
      static_assert(detail::always_false_v<R>,
                    "dispense<R>(TimeIndex, ...): unsupported return type R");
    }
#else
    (void)obj;
    (void)method;
    PLUSS_FAIL("dispense(TimeIndex): CodeH dispense is unavailable in this build. Link with PlussQit::CodeH.");
#endif
  } else {
    static_assert(detail::always_false_v<T>,
                  "dispense<R>(obj, ...): unsupported object type");
  }
}

template <class Obj, class... A>
DispenseValue dispense_any(const Obj& obj, std::string_view method, const A&... args) {
  using T = std::decay_t<Obj>;

  if constexpr (std::is_same_v<T, ::pluss::Queue>) {
    const auto q = dispense<::pluss::Queue>(obj, method, args...);
    return q.visit([](const auto& v) -> DispenseValue {
      return DispenseValue(v);
    });
  } else if constexpr (std::is_same_v<T, ::pluss::Series>) {
    return DispenseValue(dispense<::pluss::Series>(obj, method, args...));
  } else if constexpr (std::is_same_v<T, ::pluss::Note>) {
    std::string errors;
    if (auto out = detail::try_dispense([&]() {
          return dispense<::pluss::Note>(obj, method, args...);
        }, errors)) return *out;
    if (auto out = detail::try_dispense([&]() {
          return dispense<std::vector<std::uint8_t>>(obj, method, args...);
        }, errors)) return *out;
    if (auto out = detail::try_dispense([&]() {
          return dispense<std::vector<std::int64_t>>(obj, method, args...);
        }, errors)) return *out;
    if (auto out = detail::try_dispense([&]() {
          return dispense<::pluss::TokenBatch>(obj, method, args...);
        }, errors)) return *out;

    PLUSS_FAIL(errors.empty() ? std::string("dispense_any(Note): unsupported method") : errors);
  } else if constexpr (std::is_same_v<T, ::pluss::codeh::Timeline>) {
    std::string errors;
    if (auto out = detail::try_dispense([&]() {
          return dispense<::pluss::codeh::Timeline>(obj, method, args...);
        }, errors)) return *out;
    if (auto out = detail::try_dispense([&]() {
          return dispense<::pluss::codeh::Duration>(obj, method, args...);
        }, errors)) return *out;
    if (auto out = detail::try_dispense([&]() {
          return dispense<::pluss::Series>(obj, method, args...);
        }, errors)) return *out;
    if (auto out = detail::try_dispense([&]() {
          return dispense<::pluss::Note>(obj, method, args...);
        }, errors)) return *out;
    if (auto out = detail::try_dispense([&]() {
          return dispense<std::string>(obj, method, args...);
        }, errors)) return *out;
    if (auto out = detail::try_dispense([&]() {
          return dispense<bool>(obj, method, args...);
        }, errors)) return *out;
    if (auto out = detail::try_dispense([&]() {
          return dispense<std::vector<std::int64_t>>(obj, method, args...);
        }, errors)) return *out;
    if (auto out = detail::try_dispense([&]() {
          return dispense<::pluss::codeh::TimeIndex>(obj, method, args...);
        }, errors)) return *out;

    PLUSS_FAIL(errors.empty() ? std::string("dispense_any(Timeline): unsupported method") : errors);
  } else if constexpr (std::is_same_v<T, ::pluss::codeh::TimeIndex>) {
    std::string errors;
    if (auto out = detail::try_dispense([&]() {
          return dispense<::pluss::codeh::TimeIndex>(obj, method, args...);
        }, errors)) return *out;
    if (auto out = detail::try_dispense([&]() {
          return dispense<::pluss::codeh::Timeline>(obj, method, args...);
        }, errors)) return *out;
    if (auto out = detail::try_dispense([&]() {
          return dispense<::pluss::Series>(obj, method, args...);
        }, errors)) return *out;
    if (auto out = detail::try_dispense([&]() {
          return dispense<::pluss::Note>(obj, method, args...);
        }, errors)) return *out;
    if (auto out = detail::try_dispense([&]() {
          return dispense<std::string>(obj, method, args...);
        }, errors)) return *out;
    if (auto out = detail::try_dispense([&]() {
          return dispense<bool>(obj, method, args...);
        }, errors)) return *out;
    if (auto out = detail::try_dispense([&]() {
          return dispense<std::int64_t>(obj, method, args...);
        }, errors)) return *out;
    if (auto out = detail::try_dispense([&]() {
          return dispense<::pluss::codeh::TimeIndex::ResampleBins>(obj, method, args...);
        }, errors)) return *out;
    if (auto out = detail::try_dispense([&]() {
          return dispense<::pluss::codeh::TimeIndex::AlignUnion>(obj, method, args...);
        }, errors)) return *out;

    PLUSS_FAIL(errors.empty() ? std::string("dispense_any(TimeIndex): unsupported method") : errors);
  } else {
    static_assert(detail::always_false_v<T>,
                  "dispense_any(obj, ...): unsupported object type");
  }
}

} // namespace pluss

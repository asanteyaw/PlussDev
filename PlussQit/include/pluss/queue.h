#pragma once

#include <cstdint>
#include <exception>
#include <initializer_list>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "CodeH/compat/Invoke.h"
#include "CodeN/compat/Dispatch.h"
#include "CodeM/compat/Invoke.h"
#include "pluss/codem/codem.h"
#include "pluss/coden/coden.h"
#include "pluss/codeh/codeh.h"

namespace pluss {

// Queue: dynamic holder for core Pluss native column/value types.
//
// Supported payloads:
// - Series
// - Note
// - Timeline
// - TimeIndex
//
// Use invoke("method", args...) to route MATLAB-style method-name dispatch
// through the existing CodeM/CodeN/CodeH compat dispatch backends.
class Queue {
public:
  enum class Kind : std::uint8_t {
    Series,
    Note,
    Timeline,
    TimeIndex
  };

  using timeline_type = ::pluss::codeh::Timeline;
  using timeindex_type = ::pluss::codeh::TimeIndex;
  using duration_type = ::pluss::codeh::Duration;

  class Arg {
  public:
    using storage_type = std::variant<
        std::int64_t,
        double,
        bool,
        std::string,
        std::vector<std::int64_t>,
        ::pluss::Series,
        ::pluss::Note,
        timeline_type,
        timeindex_type,
        duration_type>;

    Arg(std::int64_t v) : v_(v) {}
    Arg(int v) : v_(static_cast<std::int64_t>(v)) {}
    Arg(double v) : v_(v) {}
    Arg(float v) : v_(static_cast<double>(v)) {}
    Arg(bool v) : v_(v) {}
    Arg(std::string v) : v_(std::move(v)) {}
    Arg(std::string_view v) : v_(std::string(v)) {}
    Arg(const char* v) : v_(std::string(v ? v : "")) {}
    Arg(std::vector<std::int64_t> v) : v_(std::move(v)) {}
    Arg(std::span<const std::int64_t> v) : v_(std::vector<std::int64_t>(v.begin(), v.end())) {}
    Arg(const ::pluss::Series& v) : v_(v) {}
    Arg(const ::pluss::Note& v) : v_(v) {}
    Arg(const timeline_type& v) : v_(v) {}
    Arg(const timeindex_type& v) : v_(v) {}
    Arg(const duration_type& v) : v_(v) {}

    const storage_type& storage() const noexcept { return v_; }

  private:
    storage_type v_;
  };

  using storage_type = std::variant<
      ::pluss::Series,
      ::pluss::Note,
      timeline_type,
      timeindex_type>;

  Queue(const ::pluss::Series& v) : value_(v) {}
  Queue(::pluss::Series&& v) : value_(std::move(v)) {}
  Queue(const ::pluss::Note& v) : value_(v) {}
  Queue(::pluss::Note&& v) : value_(std::move(v)) {}
  Queue(const timeline_type& v) : value_(v) {}
  Queue(timeline_type&& v) : value_(std::move(v)) {}
  Queue(const timeindex_type& v) : value_(v) {}
  Queue(timeindex_type&& v) : value_(std::move(v)) {}

  Kind kind() const noexcept {
    if (std::holds_alternative<::pluss::Series>(value_)) return Kind::Series;
    if (std::holds_alternative<::pluss::Note>(value_)) return Kind::Note;
    if (std::holds_alternative<timeline_type>(value_)) return Kind::Timeline;
    return Kind::TimeIndex;
  }

  bool defined() const noexcept {
    return std::visit([](const auto& v) -> bool {
      using T = std::decay_t<decltype(v)>;
      if constexpr (std::is_same_v<T, timeindex_type>) {
        return v.timeline().defined();
      } else {
        return v.defined();
      }
    }, value_);
  }

  std::int64_t dim() const {
    return std::visit([](const auto& v) -> std::int64_t {
      using T = std::decay_t<decltype(v)>;
      if constexpr (std::is_same_v<T, timeindex_type>) {
        return 1;
      } else {
        return v.dim();
      }
    }, value_);
  }

  std::int64_t numel() const {
    return std::visit([](const auto& v) -> std::int64_t {
      using T = std::decay_t<decltype(v)>;
      if constexpr (std::is_same_v<T, timeindex_type>) {
        return v.size();
      } else {
        return v.numel();
      }
    }, value_);
  }

  template <class T>
  bool is() const noexcept {
    return std::holds_alternative<T>(value_);
  }

  template <class T>
  const T& as() const {
    return std::get<T>(value_);
  }

  template <class T>
  T& as() {
    return std::get<T>(value_);
  }

  template <class F>
  decltype(auto) visit(F&& f) {
    return std::visit(std::forward<F>(f), value_);
  }

  template <class F>
  decltype(auto) visit(F&& f) const {
    return std::visit(std::forward<F>(f), value_);
  }

  Queue invoke(std::string_view method, std::span<const Arg> args = {}) const {
    return std::visit([&](const auto& self) -> Queue {
      using T = std::decay_t<decltype(self)>;

      if constexpr (std::is_same_v<T, ::pluss::Series>) {
        return Queue(::pluss::codem::_compat::invoke_any(method, self, to_codem_args(args)));
      } else if constexpr (std::is_same_v<T, ::pluss::Note>) {
        return Queue(::pluss::coden::_compat::call_note(method, self, to_coden_args(args)));
      } else if constexpr (std::is_same_v<T, timeline_type>) {
        return invoke_timeline(method, self, args);
      } else {
        return invoke_timeindex(method, self, args);
      }
    }, value_);
  }

  Queue invoke(std::string_view method, std::initializer_list<Arg> args) const {
    return invoke(method, std::span<const Arg>(args.begin(), args.size()));
  }

private:
  static ::pluss::codem::_compat::Args to_codem_args(std::span<const Arg> args) {
    ::pluss::codem::_compat::Args out;
    out.reserve(args.size());
    for (const auto& a : args) {
      std::visit([&](const auto& v) {
        using A = std::decay_t<decltype(v)>;
        if constexpr (
            std::is_same_v<A, std::int64_t> ||
            std::is_same_v<A, double> ||
            std::is_same_v<A, bool> ||
            std::is_same_v<A, std::string> ||
            std::is_same_v<A, std::vector<std::int64_t>> ||
            std::is_same_v<A, ::pluss::Series>) {
          out.emplace_back(::pluss::codem::_compat::Arg(v));
        } else {
          PLUSS_FAIL("Queue.invoke(Series): unsupported argument type");
        }
      }, a.storage());
    }
    return out;
  }

  static ::pluss::coden::_compat::CodenValues to_coden_args(std::span<const Arg> args) {
    ::pluss::coden::_compat::CodenValues out;
    out.reserve(args.size());
    for (const auto& a : args) {
      std::visit([&](const auto& v) {
        using A = std::decay_t<decltype(v)>;
        if constexpr (
            std::is_same_v<A, std::int64_t> ||
            std::is_same_v<A, double> ||
            std::is_same_v<A, bool> ||
            std::is_same_v<A, std::string> ||
            std::is_same_v<A, std::vector<std::int64_t>> ||
            std::is_same_v<A, ::pluss::Note>) {
          out.emplace_back(::pluss::coden::_compat::CodenValue(v));
        } else {
          PLUSS_FAIL("Queue.invoke(Note): unsupported argument type");
        }
      }, a.storage());
    }
    return out;
  }

#if defined(CODEH_ENABLE_TIMELINE_SURFACE) || defined(CODEH_BUILD_MAIN_LIB)
  static ::pluss::codeh::_compat::Args to_codeh_args(std::span<const Arg> args) {
    ::pluss::codeh::_compat::Args out;
    out.xs.reserve(args.size());
    for (const auto& a : args) {
      std::visit([&](const auto& v) {
        using A = std::decay_t<decltype(v)>;
        if constexpr (
            std::is_same_v<A, std::int64_t> ||
            std::is_same_v<A, bool> ||
            std::is_same_v<A, std::string> ||
            std::is_same_v<A, ::pluss::Series> ||
            std::is_same_v<A, timeline_type> ||
            std::is_same_v<A, timeindex_type> ||
            std::is_same_v<A, duration_type>) {
          out.xs.emplace_back(::pluss::codeh::_compat::Arg(v));
        } else {
          PLUSS_FAIL("Queue.invoke(Time): unsupported argument type");
        }
      }, a.storage());
    }
    return out;
  }
#endif

  template <class CallFn>
  static std::optional<Queue> try_queue_call(CallFn&& fn, std::string& errors) {
    try {
      return Queue(fn());
    } catch (const std::exception& e) {
      if (!errors.empty()) errors.append("\n---\n");
      errors.append(e.what());
      return std::nullopt;
    }
  }

  static std::string_view normalize_timeline_op(std::string_view op) {
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

  static std::string_view normalize_timeindex_op(std::string_view op) {
    if (op == "sort_values") return "sort";
    if (op == "drop_duplicates") return "unique";
    if (op == "freq") return "infer_freq";
    return op;
  }

#if defined(CODEH_ENABLE_TIMELINE_SURFACE) || defined(CODEH_BUILD_MAIN_LIB)
  static Queue invoke_timeline(std::string_view method,
                               const timeline_type& self,
                               std::span<const Arg> args) {
    const auto op = normalize_timeline_op(method);
    const auto hargs = to_codeh_args(args);
    std::string errors;

    if (auto out = try_queue_call([&]() { return ::pluss::codeh::_compat::call_timeline(op, self, hargs); }, errors)) {
      return *out;
    }
    if (auto out = try_queue_call([&]() { return ::pluss::codeh::_compat::call_timeindex_from_timeline(op, self, hargs); }, errors)) {
      return *out;
    }
    if (auto out = try_queue_call([&]() { return ::pluss::codeh::_compat::call_series_timeline(op, self, hargs); }, errors)) {
      return *out;
    }
    if (auto out = try_queue_call([&]() { return ::pluss::codeh::_compat::call_note_timeline(op, self, hargs); }, errors)) {
      return *out;
    }

    PLUSS_FAIL(errors.empty() ? std::string("Queue.invoke(Timeline): unsupported method") : errors);
  }

  static Queue invoke_timeindex(std::string_view method,
                                const timeindex_type& self,
                                std::span<const Arg> args) {
    const auto op = normalize_timeindex_op(method);
    const auto hargs = to_codeh_args(args);
    std::string errors;

    if (auto out = try_queue_call([&]() { return ::pluss::codeh::_compat::call_timeindex(op, self, hargs); }, errors)) {
      return *out;
    }
    if (auto out = try_queue_call([&]() { return ::pluss::codeh::_compat::call_timeline_from_timeindex(op, self, hargs); }, errors)) {
      return *out;
    }
    if (auto out = try_queue_call([&]() { return ::pluss::codeh::_compat::call_series_timeindex(op, self, hargs); }, errors)) {
      return *out;
    }
    if (auto out = try_queue_call([&]() { return ::pluss::codeh::_compat::call_note_timeindex(op, self, hargs); }, errors)) {
      return *out;
    }

    PLUSS_FAIL(errors.empty() ? std::string("Queue.invoke(TimeIndex): unsupported method") : errors);
  }
#else
  static Queue invoke_timeline(std::string_view,
                               const timeline_type&,
                               std::span<const Arg>) {
    PLUSS_FAIL("Queue.invoke(Timeline): CodeH dispatch is unavailable in this build. Link with PlussQit::CodeH.");
  }

  static Queue invoke_timeindex(std::string_view,
                                const timeindex_type&,
                                std::span<const Arg>) {
    PLUSS_FAIL("Queue.invoke(TimeIndex): CodeH dispatch is unavailable in this build. Link with PlussQit::CodeH.");
  }
#endif

  storage_type value_;
};

} // namespace pluss

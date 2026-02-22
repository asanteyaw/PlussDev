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

  class SeriesOps;
  class NoteOps;
  class TimelineOps;
  class TimeIndexOps;

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

  const ::pluss::Series& as_series() const {
    PLUSS_CHECK(is<::pluss::Series>(),
                std::string("Queue.as_series(): payload kind is ") + std::string(kind_name(kind())));
    return std::get<::pluss::Series>(value_);
  }

  ::pluss::Series& as_series() {
    PLUSS_CHECK(is<::pluss::Series>(),
                std::string("Queue.as_series(): payload kind is ") + std::string(kind_name(kind())));
    return std::get<::pluss::Series>(value_);
  }

  const ::pluss::Note& as_note() const {
    PLUSS_CHECK(is<::pluss::Note>(),
                std::string("Queue.as_note(): payload kind is ") + std::string(kind_name(kind())));
    return std::get<::pluss::Note>(value_);
  }

  ::pluss::Note& as_note() {
    PLUSS_CHECK(is<::pluss::Note>(),
                std::string("Queue.as_note(): payload kind is ") + std::string(kind_name(kind())));
    return std::get<::pluss::Note>(value_);
  }

  const timeline_type& as_timeline() const {
    PLUSS_CHECK(is<timeline_type>(),
                std::string("Queue.as_timeline(): payload kind is ") + std::string(kind_name(kind())));
    return std::get<timeline_type>(value_);
  }

  timeline_type& as_timeline() {
    PLUSS_CHECK(is<timeline_type>(),
                std::string("Queue.as_timeline(): payload kind is ") + std::string(kind_name(kind())));
    return std::get<timeline_type>(value_);
  }

  const timeindex_type& as_timeindex() const {
    PLUSS_CHECK(is<timeindex_type>(),
                std::string("Queue.as_timeindex(): payload kind is ") + std::string(kind_name(kind())));
    return std::get<timeindex_type>(value_);
  }

  timeindex_type& as_timeindex() {
    PLUSS_CHECK(is<timeindex_type>(),
                std::string("Queue.as_timeindex(): payload kind is ") + std::string(kind_name(kind())));
    return std::get<timeindex_type>(value_);
  }

  SeriesOps series() const;
  NoteOps note() const;
  TimelineOps timeline() const;
  TimeIndexOps timeindex() const;

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

  Queue invoke(std::initializer_list<std::string_view> methods) const {
    Queue out = *this;
    for (const auto method : methods) {
      PLUSS_CHECK(!method.empty(), "Queue.invoke(methods): method name cannot be empty");
      out = out.invoke(method);
    }
    return out;
  }

private:
  static constexpr std::string_view kind_name(Kind k) noexcept {
    switch (k) {
      case Kind::Series:
        return "Series";
      case Kind::Note:
        return "Note";
      case Kind::Timeline:
        return "Timeline";
      case Kind::TimeIndex:
        return "TimeIndex";
    }
    return "Unknown";
  }

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

class Queue::SeriesOps {
public:
  explicit SeriesOps(const Queue& q) : q_(&q) {
    PLUSS_CHECK(q_->kind() == Kind::Series,
                std::string("Queue.series(): payload kind is ") + std::string(Queue::kind_name(q_->kind())));
  }

  Queue invoke(std::string_view method, std::span<const Arg> args = {}) const { return q_->invoke(method, args); }
  Queue invoke(std::string_view method, std::initializer_list<Arg> args) const { return q_->invoke(method, args); }
  Queue call(std::string_view method) const { return q_->invoke(method); }
  Queue call(std::string_view method, std::initializer_list<Arg> args) const { return q_->invoke(method, args); }

  Queue log() const { return q_->invoke("log"); }
  Queue abs() const { return q_->invoke("abs"); }
  Queue exp() const { return q_->invoke("exp"); }
  Queue sqrt() const { return q_->invoke("sqrt"); }
  Queue floor() const { return q_->invoke("floor"); }
  Queue ceil() const { return q_->invoke("ceil"); }
  Queue round(std::int64_t decimals = 0) const { return q_->invoke("round", {decimals}); }

private:
  const Queue* q_{nullptr};
};

class Queue::NoteOps {
public:
  explicit NoteOps(const Queue& q) : q_(&q) {
    PLUSS_CHECK(q_->kind() == Kind::Note,
                std::string("Queue.note(): payload kind is ") + std::string(Queue::kind_name(q_->kind())));
  }

  Queue invoke(std::string_view method, std::span<const Arg> args = {}) const { return q_->invoke(method, args); }
  Queue invoke(std::string_view method, std::initializer_list<Arg> args) const { return q_->invoke(method, args); }
  Queue call(std::string_view method) const { return q_->invoke(method); }
  Queue call(std::string_view method, std::initializer_list<Arg> args) const { return q_->invoke(method, args); }

  Queue lower() const { return q_->invoke("lower"); }
  Queue upper() const { return q_->invoke("upper"); }
  Queue strip() const { return q_->invoke("strip"); }
  Queue replace(std::string_view from, std::string_view to) const { return q_->invoke("replace", {from, to}); }

private:
  const Queue* q_{nullptr};
};

class Queue::TimelineOps {
public:
  explicit TimelineOps(const Queue& q) : q_(&q) {
    PLUSS_CHECK(q_->kind() == Kind::Timeline,
                std::string("Queue.timeline(): payload kind is ") + std::string(Queue::kind_name(q_->kind())));
  }

  Queue invoke(std::string_view method, std::span<const Arg> args = {}) const { return q_->invoke(method, args); }
  Queue invoke(std::string_view method, std::initializer_list<Arg> args) const { return q_->invoke(method, args); }
  Queue call(std::string_view method) const { return q_->invoke(method); }
  Queue call(std::string_view method, std::initializer_list<Arg> args) const { return q_->invoke(method, args); }

  Queue year() const { return q_->invoke("year"); }
  Queue month() const { return q_->invoke("month"); }
  Queue day() const { return q_->invoke("day"); }
  Queue hour() const { return q_->invoke("hour"); }
  Queue minute() const { return q_->invoke("minute"); }
  Queue second() const { return q_->invoke("second"); }
  Queue floor(std::string_view freq) const { return q_->invoke("floor", {freq}); }
  Queue ceil(std::string_view freq) const { return q_->invoke("ceil", {freq}); }
  Queue strftime(std::string_view fmt) const { return q_->invoke("strftime", {fmt}); }
  Queue to_timeindex(bool require_sorted = true) const { return q_->invoke("to_timeindex", {require_sorted}); }

private:
  const Queue* q_{nullptr};
};

class Queue::TimeIndexOps {
public:
  explicit TimeIndexOps(const Queue& q) : q_(&q) {
    PLUSS_CHECK(q_->kind() == Kind::TimeIndex,
                std::string("Queue.timeindex(): payload kind is ") + std::string(Queue::kind_name(q_->kind())));
  }

  Queue invoke(std::string_view method, std::span<const Arg> args = {}) const { return q_->invoke(method, args); }
  Queue invoke(std::string_view method, std::initializer_list<Arg> args) const { return q_->invoke(method, args); }
  Queue call(std::string_view method) const { return q_->invoke(method); }
  Queue call(std::string_view method, std::initializer_list<Arg> args) const { return q_->invoke(method, args); }

  Queue sort_values() const { return q_->invoke("sort_values"); }
  Queue drop_duplicates() const { return q_->invoke("drop_duplicates"); }
  Queue freq() const { return q_->invoke("freq"); }
  Queue to_timeline() const { return q_->invoke("to_timeline"); }

private:
  const Queue* q_{nullptr};
};

inline Queue::SeriesOps Queue::series() const { return SeriesOps(*this); }
inline Queue::NoteOps Queue::note() const { return NoteOps(*this); }
inline Queue::TimelineOps Queue::timeline() const { return TimelineOps(*this); }
inline Queue::TimeIndexOps Queue::timeindex() const { return TimeIndexOps(*this); }

} // namespace pluss

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
#include <ostream>
#include <sstream>

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
// - Duration
//
// Use invoke("method", args...) to route MATLAB-style method-name dispatch
// through the existing CodeM/CodeN/CodeH compat dispatch backends.
class Queue {
public:
  enum class Kind : std::uint8_t {
    Series,
    Note,
    Timeline,
    TimeIndex,
    Duration
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
      timeindex_type,
      duration_type>;

  Queue(const ::pluss::Series& v) : value_(v) {}
  Queue(::pluss::Series&& v) : value_(std::move(v)) {}
  Queue(const ::pluss::Note& v) : value_(v) {}
  Queue(::pluss::Note&& v) : value_(std::move(v)) {}
  Queue(const timeline_type& v) : value_(v) {}
  Queue(timeline_type&& v) : value_(std::move(v)) {}
  Queue(const timeindex_type& v) : value_(v) {}
  Queue(timeindex_type&& v) : value_(std::move(v)) {}
  Queue(const duration_type& v) : value_(v) {}
  Queue(duration_type&& v) : value_(std::move(v)) {}

  Kind kind() const noexcept {
    if (std::holds_alternative<::pluss::Series>(value_)) return Kind::Series;
    if (std::holds_alternative<::pluss::Note>(value_)) return Kind::Note;
    if (std::holds_alternative<timeline_type>(value_)) return Kind::Timeline;
    if (std::holds_alternative<duration_type>(value_)) return Kind::Duration;
    return Kind::TimeIndex;
  }

  bool is_series() const noexcept { return kind() == Kind::Series; }
  bool is_note() const noexcept { return kind() == Kind::Note; }
  bool is_timeline() const noexcept { return kind() == Kind::Timeline; }
  bool is_timeindex() const noexcept { return kind() == Kind::TimeIndex; }
  bool is_duration() const noexcept { return kind() == Kind::Duration; }

  std::string_view kind_name() const noexcept { return kind_name(kind()); }
  static constexpr std::string_view kind_name(Kind k) noexcept { return kind_name_impl_(k); }

  const ::pluss::Series& series() const { return as_series(); }
  ::pluss::Series& series() { return as_series(); }
  const ::pluss::Note& note() const { return as_note(); }
  ::pluss::Note& note() { return as_note(); }
  const timeline_type& timeline() const { return as_timeline(); }
  timeline_type& timeline() { return as_timeline(); }
  const timeindex_type& timeindex() const { return as_timeindex(); }
  timeindex_type& timeindex() { return as_timeindex(); }
  const duration_type& duration() const { return as_duration(); }
  duration_type& duration() { return as_duration(); }

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
      } else if constexpr (std::is_same_v<T, timeindex_type>) {
        return invoke_timeindex(method, self, args);
      } else {
        return invoke_duration(method, self, args);
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

  // Unary operators
  Queue operator+() const { return *this; }
  Queue operator-() const {
    try {
      if (kind() == Kind::Series || kind() == Kind::Duration) {
        return invoke("neg");
      }
      PLUSS_FAIL("Queue::operator-: unsupported for payload '" + std::string(kind_name(kind())) + "'");
    } catch (const std::exception& e) {
      PLUSS_FAIL(std::string("Queue::operator- failed: ") + e.what());
    }
  }

  // Binary arithmetic operators
  Queue operator+(const Queue& rhs) const { return binary_arith_("add", "+", rhs); }
  Queue operator-(const Queue& rhs) const { return binary_arith_("sub", "-", rhs); }
  Queue operator*(const Queue& rhs) const { return binary_arith_("mul", "*", rhs); }
  Queue operator/(const Queue& rhs) const { return binary_arith_("div", "/", rhs); }
  Queue operator%(const Queue& rhs) const { return binary_arith_("mod", "%", rhs); }

  // Binary comparisons (vectorized, Queue result)
  Queue operator==(const Queue& rhs) const { return binary_cmp_("eq", "==", rhs); }
  Queue operator!=(const Queue& rhs) const { return binary_cmp_("ne", "!=", rhs); }
  Queue operator<(const Queue& rhs) const { return binary_cmp_("lt", "<", rhs); }
  Queue operator<=(const Queue& rhs) const { return binary_cmp_("le", "<=", rhs); }
  Queue operator>(const Queue& rhs) const { return binary_cmp_("gt", ">", rhs); }
  Queue operator>=(const Queue& rhs) const { return binary_cmp_("ge", ">=", rhs); }

  // Ternary-style conditional select.
  // Supports:
  // - Series: where(mask, x, y)
  // - Timeline: where(mask, x, y)
  static Queue where(const Queue& condition, const Queue& x, const Queue& y) {
    PLUSS_CHECK(condition.kind() == Kind::Series,
                "Queue::where: condition must be a Series mask");
    PLUSS_CHECK(x.kind() == y.kind(),
                "Queue::where: x/y kind mismatch (" + std::string(kind_name(x.kind()))
                    + " vs " + std::string(kind_name(y.kind())) + ")");

    try {
      switch (x.kind()) {
        case Kind::Series:
          // Force semantics: where(mask, x, y) => mask ? x : y.
          return Queue(condition.as_series().where(x.as_series(), y.as_series()));
        case Kind::Timeline:
          return x.invoke("where", {condition.as_series(), y.as_timeline()});
        case Kind::Duration:
          return x.invoke("where", {condition.as_series(), y.as_duration()});
        case Kind::Note:
          PLUSS_FAIL("Queue::where: Note ternary selection is not supported");
        case Kind::TimeIndex:
          PLUSS_FAIL("Queue::where: TimeIndex ternary selection is not supported");
      }
      PLUSS_FAIL("Queue::where: unknown payload kind");
    } catch (const std::exception& e) {
      PLUSS_FAIL(std::string("Queue::where failed: ") + e.what());
    }
  }

private:
  static constexpr std::string_view kind_name_impl_(Kind k) noexcept {
    switch (k) {
      case Kind::Series:
        return "Series";
      case Kind::Note:
        return "Note";
      case Kind::Timeline:
        return "Timeline";
      case Kind::TimeIndex:
        return "TimeIndex";
      case Kind::Duration:
        return "Duration";
    }
    return "Unknown";
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

  const duration_type& as_duration() const {
    PLUSS_CHECK(is<duration_type>(),
                std::string("Queue.as_duration(): payload kind is ") + std::string(kind_name(kind())));
    return std::get<duration_type>(value_);
  }

  duration_type& as_duration() {
    PLUSS_CHECK(is<duration_type>(),
                std::string("Queue.as_duration(): payload kind is ") + std::string(kind_name(kind())));
    return std::get<duration_type>(value_);
  }

  static ::pluss::Series u8_mask_to_bool_series_(const std::vector<std::uint8_t>& mask) {
    auto out = ::pluss::Series::zeros({static_cast<std::int64_t>(mask.size())},
                                      ::pluss::codem::ScalarType::Bool);
    auto* p = reinterpret_cast<bool*>(out.raw_data());
    for (std::size_t i = 0; i < mask.size(); ++i) {
      p[i] = (mask[i] != 0);
    }
    return out;
  }

  Queue binary_arith_(std::string_view op_name, std::string_view symbol, const Queue& rhs) const {
    PLUSS_CHECK(kind() == rhs.kind(),
                "Queue::operator" + std::string(symbol) + ": kind mismatch ("
                    + std::string(kind_name(kind())) + " vs " + std::string(kind_name(rhs.kind())) + ")");
    try {
      switch (kind()) {
        case Kind::Series:
          return invoke(op_name, {rhs.as_series()});
        case Kind::Note:
          if (op_name == "add") {
            return Queue(note() + rhs.note());
          }
          PLUSS_FAIL("Queue::operator" + std::string(symbol)
                     + ": unsupported for Note payloads");
        case Kind::Timeline:
          return invoke(op_name, {rhs.as_timeline()});
        case Kind::TimeIndex:
          return invoke(op_name, {rhs.as_timeindex()});
        case Kind::Duration:
          return invoke(op_name, {rhs.as_duration()});
      }
      PLUSS_FAIL("Queue::operator" + std::string(symbol) + ": unknown payload kind");
    } catch (const std::exception& e) {
      PLUSS_FAIL("Queue::operator" + std::string(symbol) + " failed: " + std::string(e.what()));
    }
  }

  Queue binary_cmp_(std::string_view op_name, std::string_view symbol, const Queue& rhs) const {
    PLUSS_CHECK(kind() == rhs.kind(),
                "Queue::operator" + std::string(symbol) + ": kind mismatch ("
                    + std::string(kind_name(kind())) + " vs " + std::string(kind_name(rhs.kind())) + ")");
    try {
      switch (kind()) {
        case Kind::Series:
          return invoke(op_name, {rhs.as_series()});
        case Kind::Note: {
          if (op_name == "eq") return Queue(u8_mask_to_bool_series_(note() == rhs.note()));
          if (op_name == "ne") return Queue(u8_mask_to_bool_series_(note() != rhs.note()));
          if (op_name == "lt") return Queue(u8_mask_to_bool_series_(note() < rhs.note()));
          if (op_name == "le") return Queue(u8_mask_to_bool_series_(note() <= rhs.note()));
          if (op_name == "gt") return Queue(u8_mask_to_bool_series_(note() > rhs.note()));
          if (op_name == "ge") return Queue(u8_mask_to_bool_series_(note() >= rhs.note()));
          PLUSS_FAIL("Queue::operator" + std::string(symbol) + ": unsupported Note compare op");
        }
        case Kind::Timeline:
          return invoke(op_name, {rhs.as_timeline()});
        case Kind::TimeIndex:
          return invoke(op_name, {rhs.as_timeindex()});
        case Kind::Duration:
          return invoke(op_name, {rhs.as_duration()});
      }
      PLUSS_FAIL("Queue::operator" + std::string(symbol) + ": unknown payload kind");
    } catch (const std::exception& e) {
      PLUSS_FAIL("Queue::operator" + std::string(symbol) + " failed: " + std::string(e.what()));
    }
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

  static Queue invoke_duration(std::string_view method,
                               const duration_type& self,
                               std::span<const Arg> args) {
    const auto hargs = to_codeh_args(args);
    std::string errors;

    if (auto out = try_queue_call([&]() { return ::pluss::codeh::_compat::call_duration(method, self, hargs); }, errors)) {
      return *out;
    }
    if (auto out = try_queue_call([&]() { return ::pluss::codeh::_compat::call_series_duration(method, self, hargs); }, errors)) {
      return *out;
    }
    if (auto out = try_queue_call([&]() { return ::pluss::codeh::_compat::call_note_duration(method, self, hargs); }, errors)) {
      return *out;
    }

    PLUSS_FAIL(errors.empty() ? std::string("Queue.invoke(Duration): unsupported method") : errors);
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

  static Queue invoke_duration(std::string_view,
                               const duration_type&,
                               std::span<const Arg>) {
    PLUSS_FAIL("Queue.invoke(Duration): CodeH dispatch is unavailable in this build. Link with PlussQit::CodeH.");
  }
#endif

  storage_type value_;
};

inline std::ostream& operator<<(std::ostream& os, const Queue& q) {
  auto write_wrapped = [&](std::string_view kind, std::string_view payload_prefix, const auto& payload) {
    std::ostringstream tmp;
    tmp << payload;
    std::string body = tmp.str();
    if (body.rfind(std::string(payload_prefix), 0) == 0) {
      body.erase(0, payload_prefix.size());
    }
    os << "Queue<" << kind << ">" << body;
  };

  switch (q.kind()) {
    case Queue::Kind::Series:
      write_wrapped("Series", "Series", q.series());
      break;
    case Queue::Kind::Note:
      write_wrapped("Note", "Note", q.note());
      break;
    case Queue::Kind::Timeline:
      write_wrapped("Timeline", "Timeline", q.timeline());
      break;
    case Queue::Kind::TimeIndex:
      os << "Queue<TimeIndex>(size=" << q.timeindex().size() << ", timeline=" << q.timeindex().timeline() << ")";
      break;
    case Queue::Kind::Duration:
      write_wrapped("Duration", "Duration", q.duration());
      break;
  }
  return os;
}

} // namespace pluss

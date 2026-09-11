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

namespace pluss::native {

// Type: dynamic holder for core Pluss native column/value types.
//
// Supported payloads:
// - Series
// - Note
// - Timeline
// - TimeIndex
//
// Use invoke("method", args...) to route MATLAB-style method-name dispatch
// through the existing CodeM/CodeN/CodeH compat dispatch backends.
class Type {
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

  Type(const ::pluss::Series& v) : value_(v) {}
  Type(::pluss::Series&& v) : value_(std::move(v)) {}
  Type(const ::pluss::Note& v) : value_(v) {}
  Type(::pluss::Note&& v) : value_(std::move(v)) {}
  Type(const timeline_type& v) : value_(v) {}
  Type(timeline_type&& v) : value_(std::move(v)) {}
  Type(const timeindex_type& v) : value_(v) {}
  Type(timeindex_type&& v) : value_(std::move(v)) {}

  Kind kind() const noexcept {
    if (std::holds_alternative<::pluss::Series>(value_)) return Kind::Series;
    if (std::holds_alternative<::pluss::Note>(value_)) return Kind::Note;
    if (std::holds_alternative<timeline_type>(value_)) return Kind::Timeline;
    return Kind::TimeIndex;
  }

  bool is_series() const noexcept { return kind() == Kind::Series; }
  bool is_note() const noexcept { return kind() == Kind::Note; }
  bool is_timeline() const noexcept { return kind() == Kind::Timeline; }
  bool is_timeindex() const noexcept { return kind() == Kind::TimeIndex; }

  std::string_view kind_name() const noexcept { return kind_name(kind()); }
  static constexpr std::string_view kind_name(Kind k) noexcept { return kind_name_impl_(k); }

  const ::pluss::Series& as_series() const {
    PLUSS_CHECK(is<::pluss::Series>(),
                std::string("Type.as_series(): payload kind is ") + std::string(kind_name(kind())));
    return std::get<::pluss::Series>(value_);
  }

  ::pluss::Series& as_series() {
    PLUSS_CHECK(is<::pluss::Series>(),
                std::string("Type.as_series(): payload kind is ") + std::string(kind_name(kind())));
    return std::get<::pluss::Series>(value_);
  }

  const ::pluss::Note& as_note() const {
    PLUSS_CHECK(is<::pluss::Note>(),
                std::string("Type.as_note(): payload kind is ") + std::string(kind_name(kind())));
    return std::get<::pluss::Note>(value_);
  }

  ::pluss::Note& as_note() {
    PLUSS_CHECK(is<::pluss::Note>(),
                std::string("Type.as_note(): payload kind is ") + std::string(kind_name(kind())));
    return std::get<::pluss::Note>(value_);
  }

  const timeline_type& as_timeline() const {
    PLUSS_CHECK(is<timeline_type>(),
                std::string("Type.as_timeline(): payload kind is ") + std::string(kind_name(kind())));
    return std::get<timeline_type>(value_);
  }

  timeline_type& as_timeline() {
    PLUSS_CHECK(is<timeline_type>(),
                std::string("Type.as_timeline(): payload kind is ") + std::string(kind_name(kind())));
    return std::get<timeline_type>(value_);
  }

  const timeindex_type& as_timeindex() const {
    PLUSS_CHECK(is<timeindex_type>(),
                std::string("Type.as_timeindex(): payload kind is ") + std::string(kind_name(kind())));
    return std::get<timeindex_type>(value_);
  }

  timeindex_type& as_timeindex() {
    PLUSS_CHECK(is<timeindex_type>(),
                std::string("Type.as_timeindex(): payload kind is ") + std::string(kind_name(kind())));
    return std::get<timeindex_type>(value_);
  }

  const ::pluss::Series& series() const { return as_series(); }
  ::pluss::Series& series() { return as_series(); }
  const ::pluss::Note& note() const { return as_note(); }
  ::pluss::Note& note() { return as_note(); }
  const timeline_type& timeline() const { return as_timeline(); }
  timeline_type& timeline() { return as_timeline(); }
  const timeindex_type& timeindex() const { return as_timeindex(); }
  timeindex_type& timeindex() { return as_timeindex(); }

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

  Type invoke(std::string_view method, std::span<const Arg> args = {}) const {
    return std::visit([&](const auto& self) -> Type {
      using T = std::decay_t<decltype(self)>;

      if constexpr (std::is_same_v<T, ::pluss::Series>) {
        return Type(::pluss::codem::_compat::invoke_any(method, self, to_codem_args(args)));
      } else if constexpr (std::is_same_v<T, ::pluss::Note>) {
        return Type(::pluss::coden::_compat::call_note(method, self, to_coden_args(args)));
      } else if constexpr (std::is_same_v<T, timeline_type>) {
        return invoke_timeline(method, self, args);
      } else {
        return invoke_timeindex(method, self, args);
      }
    }, value_);
  }

  Type invoke(std::string_view method, std::initializer_list<Arg> args) const {
    return invoke(method, std::span<const Arg>(args.begin(), args.size()));
  }

  Type invoke(std::initializer_list<std::string_view> methods) const {
    Type out = *this;
    for (const auto method : methods) {
      PLUSS_CHECK(!method.empty(), "Type.invoke(methods): method name cannot be empty");
      out = out.invoke(method);
    }
    return out;
  }

  // Unary operators
  Type operator+() const { return *this; }
  Type operator-() const {
    try {
      if (kind() == Kind::Series) {
        return invoke("neg");
      }
      PLUSS_FAIL("Type::operator-: unsupported for non-Series payload '" + std::string(kind_name(kind())) + "'");
    } catch (const std::exception& e) {
      PLUSS_FAIL(std::string("Type::operator- failed: ") + e.what());
    }
  }

  // Binary arithmetic operators
  Type operator+(const Type& rhs) const { return binary_arith_("add", "+", rhs); }
  Type operator-(const Type& rhs) const { return binary_arith_("sub", "-", rhs); }
  Type operator*(const Type& rhs) const { return binary_arith_("mul", "*", rhs); }
  Type operator/(const Type& rhs) const { return binary_arith_("div", "/", rhs); }
  Type operator%(const Type& rhs) const { return binary_arith_("mod", "%", rhs); }

  // Binary comparisons (vectorized, Type result)
  Type operator==(const Type& rhs) const { return binary_cmp_("eq", "==", rhs); }
  Type operator!=(const Type& rhs) const { return binary_cmp_("ne", "!=", rhs); }
  Type operator<(const Type& rhs) const { return binary_cmp_("lt", "<", rhs); }
  Type operator<=(const Type& rhs) const { return binary_cmp_("le", "<=", rhs); }
  Type operator>(const Type& rhs) const { return binary_cmp_("gt", ">", rhs); }
  Type operator>=(const Type& rhs) const { return binary_cmp_("ge", ">=", rhs); }

  // Ternary-style conditional select.
  // Supports:
  // - Series: where(mask, x, y)
  // - Timeline: where(mask, x, y)
  static Type where(const Type& condition, const Type& x, const Type& y) {
    PLUSS_CHECK(condition.kind() == Kind::Series,
                "Type::where: condition must be a Series mask");
    PLUSS_CHECK(x.kind() == y.kind(),
                "Type::where: x/y kind mismatch (" + std::string(kind_name(x.kind()))
                    + " vs " + std::string(kind_name(y.kind())) + ")");

    try {
      switch (x.kind()) {
        case Kind::Series:
          // Force torch-style semantics: where(mask, x, y) => mask ? x : y.
          return Type(condition.as_series().where(x.as_series(), y.as_series()));
        case Kind::Timeline:
          return x.invoke("where", {condition.as_series(), y.as_timeline()});
        case Kind::Note:
          PLUSS_FAIL("Type::where: Note ternary selection is not supported");
        case Kind::TimeIndex:
          PLUSS_FAIL("Type::where: TimeIndex ternary selection is not supported");
      }
      PLUSS_FAIL("Type::where: unknown payload kind");
    } catch (const std::exception& e) {
      PLUSS_FAIL(std::string("Type::where failed: ") + e.what());
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
    }
    return "Unknown";
  }

  static ::pluss::Series u8_mask_to_bool_series_(const ::pluss::Note& mask) {
    const auto lanes = mask.bools();
    auto out = ::pluss::Series::zeros({static_cast<std::int64_t>(lanes.size())},
                                      ::pluss::codem::ScalarType::Bool);
    auto* p = reinterpret_cast<bool*>(out.raw_data());
    for (std::size_t i = 0; i < lanes.size(); ++i) {
      p[i] = (lanes[i] != 0);
    }
    return out;
  }

  Type binary_arith_(std::string_view op_name, std::string_view symbol, const Type& rhs) const {
    PLUSS_CHECK(kind() == rhs.kind(),
                "Type::operator" + std::string(symbol) + ": kind mismatch ("
                    + std::string(kind_name(kind())) + " vs " + std::string(kind_name(rhs.kind())) + ")");
    try {
      switch (kind()) {
        case Kind::Series:
          return invoke(op_name, {rhs.as_series()});
        case Kind::Note:
          if (op_name == "add") {
            return Type(note() + rhs.note());
          }
          PLUSS_FAIL("Type::operator" + std::string(symbol)
                     + ": unsupported for Note payloads");
        case Kind::Timeline:
          return invoke(op_name, {rhs.as_timeline()});
        case Kind::TimeIndex:
          return invoke(op_name, {rhs.as_timeindex()});
      }
      PLUSS_FAIL("Type::operator" + std::string(symbol) + ": unknown payload kind");
    } catch (const std::exception& e) {
      PLUSS_FAIL("Type::operator" + std::string(symbol) + " failed: " + std::string(e.what()));
    }
  }

  Type binary_cmp_(std::string_view op_name, std::string_view symbol, const Type& rhs) const {
    PLUSS_CHECK(kind() == rhs.kind(),
                "Type::operator" + std::string(symbol) + ": kind mismatch ("
                    + std::string(kind_name(kind())) + " vs " + std::string(kind_name(rhs.kind())) + ")");
    try {
      switch (kind()) {
        case Kind::Series:
          return invoke(op_name, {rhs.as_series()});
        case Kind::Note: {
          if (op_name == "eq") return Type(u8_mask_to_bool_series_(note() == rhs.note()));
          if (op_name == "ne") return Type(u8_mask_to_bool_series_(note() != rhs.note()));
          if (op_name == "lt") return Type(u8_mask_to_bool_series_(note() < rhs.note()));
          if (op_name == "le") return Type(u8_mask_to_bool_series_(note() <= rhs.note()));
          if (op_name == "gt") return Type(u8_mask_to_bool_series_(note() > rhs.note()));
          if (op_name == "ge") return Type(u8_mask_to_bool_series_(note() >= rhs.note()));
          PLUSS_FAIL("Type::operator" + std::string(symbol) + ": unsupported Note compare op");
        }
        case Kind::Timeline:
          return invoke(op_name, {rhs.as_timeline()});
        case Kind::TimeIndex:
          return invoke(op_name, {rhs.as_timeindex()});
      }
      PLUSS_FAIL("Type::operator" + std::string(symbol) + ": unknown payload kind");
    } catch (const std::exception& e) {
      PLUSS_FAIL("Type::operator" + std::string(symbol) + " failed: " + std::string(e.what()));
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
          PLUSS_FAIL("Type.invoke(Series): unsupported argument type");
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
          PLUSS_FAIL("Type.invoke(Note): unsupported argument type");
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
          PLUSS_FAIL("Type.invoke(Time): unsupported argument type");
        }
      }, a.storage());
    }
    return out;
  }
#endif

  template <class CallFn>
  static std::optional<Type> try_queue_call(CallFn&& fn, std::string& errors) {
    try {
      return Type(fn());
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
    if (op == "weekday") return "dt_weekday";
    if (op == "dayofyear") return "dt_dayofyear";
    if (op == "week") return "dt_week";

    return op;
  }

  static std::string_view normalize_timeindex_op(std::string_view op) {
    if (op == "sort_values") return "sort";
    if (op == "freq") return "infer_freq";
    return op;
  }

#if defined(CODEH_ENABLE_TIMELINE_SURFACE) || defined(CODEH_BUILD_MAIN_LIB)
  static Type invoke_timeline(std::string_view method,
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

    PLUSS_FAIL(errors.empty() ? std::string("Type.invoke(Timeline): unsupported method") : errors);
  }

  static Type invoke_timeindex(std::string_view method,
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

    PLUSS_FAIL(errors.empty() ? std::string("Type.invoke(TimeIndex): unsupported method") : errors);
  }
#else
  static Type invoke_timeline(std::string_view,
                               const timeline_type&,
                               std::span<const Arg>) {
    PLUSS_FAIL("Type.invoke(Timeline): CodeH dispatch is unavailable in this build. Link with PlussQit::CodeH.");
  }

  static Type invoke_timeindex(std::string_view,
                                const timeindex_type&,
                                std::span<const Arg>) {
    PLUSS_FAIL("Type.invoke(TimeIndex): CodeH dispatch is unavailable in this build. Link with PlussQit::CodeH.");
  }
#endif

  storage_type value_;
};

inline std::ostream& operator<<(std::ostream& os, const Type& q) {
  auto write_wrapped = [&](std::string_view kind, std::string_view payload_prefix, const auto& payload) {
    std::ostringstream tmp;
    tmp << payload;
    std::string body = tmp.str();
    if (body.rfind(std::string(payload_prefix), 0) == 0) {
      body.erase(0, payload_prefix.size());
    }
    os << "Type<" << kind << ">" << body;
  };

  switch (q.kind()) {
    case Type::Kind::Series:
      write_wrapped("Series", "Series", q.series());
      break;
    case Type::Kind::Note:
      write_wrapped("Note", "Note", q.note());
      break;
    case Type::Kind::Timeline:
      write_wrapped("Timeline", "Timeline", q.timeline());
      break;
    case Type::Kind::TimeIndex:
      os << "Type<TimeIndex>(size=" << q.timeindex().size() << ", timeline=" << q.timeindex().timeline() << ")";
      break;
  }
  return os;
}

} // namespace pluss::native

namespace pluss {
using Type = ::pluss::native::Type;
} // namespace pluss

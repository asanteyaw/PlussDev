#include "bundle/Slot.h"

#include <algorithm>
#include <cmath>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

#include <fmt/format.h>

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"
#include "pluss/coden/note_factory.h"
#include "signal/Stream.h"

namespace pluss::bundle {

namespace {

template <typename T>
bool all_type(const std::vector<Slot::Value>& values) {
  return std::all_of(values.begin(), values.end(), [](const auto& v) { return std::holds_alternative<T>(v); });
}

std::pair<std::int64_t, std::int64_t> normalize_slice(std::int64_t start, std::int64_t stop, std::int64_t size) {
  std::int64_t s = start < 0 ? start + size : start;
  std::int64_t e = stop < 0 ? stop + size : stop;
  s = std::clamp<std::int64_t>(s, 0, size);
  e = std::clamp<std::int64_t>(e, 0, size);
  if (e < s) e = s;
  return {s, e};
}

std::string json_escape(std::string_view in) {
  std::string out;
  out.reserve(in.size() + 8);
  for (char c : in) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: out.push_back(c); break;
    }
  }
  return out;
}

std::string value_to_json(const Slot::Value& value) {
  return std::visit(
      [](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) return "null";
        if constexpr (std::is_same_v<T, bool>) return v ? "true" : "false";
        if constexpr (std::is_same_v<T, std::int64_t>) return fmt::format("{}", v);
        if constexpr (std::is_same_v<T, double>) return fmt::format("{}", v);
        if constexpr (std::is_same_v<T, std::string>) return fmt::format("\"{}\"", json_escape(v));
        if constexpr (std::is_same_v<T, pluss::Series>) return fmt::format("\"Series[{}]\"", v.dtype().name);
        if constexpr (std::is_same_v<T, pluss::Note>) return "\"Note\"";
        if constexpr (std::is_same_v<T, pluss::Timeline>) return "\"Timeline\"";
        if constexpr (std::is_same_v<T, pluss::TimeIndex>) return "\"TimeIndex\"";
        return "\"unknown\"";
      },
      value);
}

std::string value_type_name(const Slot::Value& value) {
  return std::visit(
      [](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) return "null";
        if constexpr (std::is_same_v<T, bool>) return "bool";
        if constexpr (std::is_same_v<T, std::int64_t>) return "int64";
        if constexpr (std::is_same_v<T, double>) return "float64";
        if constexpr (std::is_same_v<T, std::string>) return "string";
        if constexpr (std::is_same_v<T, pluss::Series>) return "Series";
        if constexpr (std::is_same_v<T, pluss::Note>) return "Note";
        if constexpr (std::is_same_v<T, pluss::Timeline>) return "Timeline";
        if constexpr (std::is_same_v<T, pluss::TimeIndex>) return "TimeIndex";
        return "unknown";
      },
      value);
}

} // namespace

namespace detail {
class SlotImpl final : public pluss::msharp::RefCounted {
public:
  SlotImpl() = default;
  explicit SlotImpl(std::vector<Slot::Value> values) : values_(std::move(values)) {}

  std::vector<Slot::Value> values_{};
};
} // namespace detail

Slot::Slot() = default;
Slot::Slot(std::vector<Value> values)
    : impl_(pluss::msharp::make_intrusive<detail::SlotImpl>(std::move(values))) {}
Slot::Slot(std::initializer_list<Value> values)
    : Slot(std::vector<Value>(values)) {}
Slot::Slot(Value value)
    : Slot(std::vector<Value>{std::move(value)}) {}
Slot::Slot(const Slot& other) = default;
Slot::Slot(Slot&& other) noexcept = default;
Slot& Slot::operator=(const Slot& other) = default;
Slot& Slot::operator=(Slot&& other) noexcept = default;
Slot::~Slot() = default;

Slot::Slot(pluss::msharp::intrusive_ptr<detail::SlotImpl> impl) : impl_(std::move(impl)) {}

bool Slot::defined() const noexcept { return static_cast<bool>(impl_); }
bool Slot::unique() const noexcept { return defined() && impl_.use_count() == 1; }

std::int64_t Slot::size() const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  return static_cast<std::int64_t>(impl_ref_().values_.size());
}

bool Slot::empty() const { return size() == 0; }

const std::vector<Slot::Value>& Slot::values() const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  return impl_ref_().values_;
}

const Slot::Value& Slot::at(std::int64_t i) const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  PLUSS_CHECK(i >= 0 && i < size(), "Slot::at: index out of bounds");
  return impl_ref_().values_[static_cast<std::size_t>(i)];
}

Slot Slot::push(const Value& value) const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  auto out = *this;
  out.push_(value);
  return out;
}

Slot& Slot::push_(const Value& value) {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  mut_impl_().values_.push_back(value);
  return *this;
}

Slot Slot::head(std::int64_t n) const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  const auto clipped = std::clamp<std::int64_t>(n, 0, size());
  return slice(0, clipped);
}

Slot Slot::tail(std::int64_t n) const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  const auto clipped = std::clamp<std::int64_t>(n, 0, size());
  return slice(size() - clipped, size());
}

Slot Slot::slice(std::int64_t start, std::int64_t stop) const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  const auto [s, e] = normalize_slice(start, stop, size());
  std::vector<Value> out;
  out.reserve(static_cast<std::size_t>(e - s));
  for (std::int64_t i = s; i < e; ++i) {
    out.push_back(at(i));
  }
  return Slot(std::move(out));
}

Slot Slot::take(std::span<const std::int64_t> indices) const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  std::vector<Value> out;
  out.reserve(indices.size());
  for (const auto idx : indices) {
    out.push_back(at(idx));
  }
  return Slot(std::move(out));
}

Slot Slot::filter(const std::function<bool(const Value&)>& predicate) const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  PLUSS_CHECK(static_cast<bool>(predicate), "Slot::filter: predicate is empty");
  std::vector<Value> out;
  out.reserve(values().size());
  for (const auto& v : values()) {
    if (predicate(v)) out.push_back(v);
  }
  return Slot(std::move(out));
}

Slot Slot::transform(const std::function<Value(const Value&)>& fn) const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  PLUSS_CHECK(static_cast<bool>(fn), "Slot::transform: fn is empty");
  std::vector<Value> out;
  out.reserve(values().size());
  for (const auto& v : values()) {
    out.push_back(fn(v));
  }
  return Slot(std::move(out));
}

Slot Slot::concat(const Slot& other) const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  PLUSS_CHECK(other.defined(), "Slot::concat: other slot is undefined");
  std::vector<Value> out;
  out.reserve(values().size() + other.values().size());
  out.insert(out.end(), values().begin(), values().end());
  out.insert(out.end(), other.values().begin(), other.values().end());
  return Slot(std::move(out));
}

std::unordered_map<std::string, std::int64_t> Slot::type_counts() const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  std::unordered_map<std::string, std::int64_t> out;
  out.reserve(values().size());
  for (const auto& v : values()) {
    out[value_type_name(v)] += 1;
  }
  return out;
}

pluss::signal::Stream Slot::signalize() const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  if (empty()) return pluss::signal::Stream(pluss::make_note(std::vector<std::string>{}));

  if (all_type<bool>(values())) {
    std::vector<bool> b;
    b.reserve(values().size());
    for (const auto& v : values()) b.push_back(std::get<bool>(v));
    const auto n = static_cast<std::int64_t>(b.size());
    auto s = pluss::Series::zeros({n}, pluss::codem::ScalarType::Bool);
    auto* p = reinterpret_cast<bool*>(s.raw_data());
    for (std::int64_t i = 0; i < n; ++i) p[i] = b[static_cast<std::size_t>(i)];
    return pluss::signal::Stream(s);
  }
  if (all_type<std::int64_t>(values())) {
    std::vector<std::int64_t> x;
    x.reserve(values().size());
    for (const auto& v : values()) x.push_back(std::get<std::int64_t>(v));
    return pluss::signal::Stream(pluss::make_series(x, {static_cast<std::int64_t>(x.size())}));
  }
  if (all_type<double>(values())) {
    std::vector<double> x;
    x.reserve(values().size());
    for (const auto& v : values()) x.push_back(std::get<double>(v));
    return pluss::signal::Stream(pluss::make_series(x, {static_cast<std::int64_t>(x.size())}));
  }
  if (all_type<std::string>(values())) {
    std::vector<std::string> x;
    x.reserve(values().size());
    for (const auto& v : values()) x.push_back(std::get<std::string>(v));
    return pluss::signal::Stream(pluss::make_note(x));
  }
  if (values().size() == 1) {
    const auto& v = values().front();
    if (std::holds_alternative<pluss::Series>(v)) return pluss::signal::Stream(std::get<pluss::Series>(v));
    if (std::holds_alternative<pluss::Note>(v)) return pluss::signal::Stream(std::get<pluss::Note>(v));
    if (std::holds_alternative<pluss::Timeline>(v)) return pluss::signal::Stream(std::get<pluss::Timeline>(v));
    if (std::holds_alternative<pluss::TimeIndex>(v)) return pluss::signal::Stream(std::get<pluss::TimeIndex>(v));
  }

  PLUSS_FAIL("Slot::signalize: heterogeneous slot is not stream-compatible");
}

Slot Slot::from_signal(const pluss::signal::Stream& stream) {
  PLUSS_CHECK(stream.defined(), "Slot::from_signal: stream undefined");
  const auto n = stream.size();
  std::vector<Value> out;
  out.reserve(static_cast<std::size_t>(n));
  switch (stream.kind()) {
    case pluss::signal::StreamKind::Series: {
      const auto s = stream.as_series();
      if (s.scalar_type() == pluss::codem::ScalarType::Bool) {
        const auto x = s.to_vec<bool>();
        for (bool v : x) out.push_back(v);
      } else if (s.scalar_type() == pluss::codem::ScalarType::Int64) {
        const auto x = s.to_vec<std::int64_t>();
        for (auto v : x) out.push_back(v);
      } else {
        const auto x = s.to(pluss::codem::ScalarType::Float64).to_vec<double>();
        for (auto v : x) out.push_back(v);
      }
      break;
    }
    case pluss::signal::StreamKind::Note: {
      const auto& nte = stream.as_note();
      for (std::int64_t i = 0; i < n; ++i) out.push_back(std::string(nte.at(i)));
      break;
    }
    case pluss::signal::StreamKind::Timeline: {
      const auto x = stream.as_timeline().to_epoch_nanos_vector();
      for (auto v : x) out.push_back(v);
      break;
    }
    case pluss::signal::StreamKind::TimeIndex: {
      const auto x = stream.as_timeindex().timeline().to_epoch_nanos_vector();
      for (auto v : x) out.push_back(v);
      break;
    }
  }
  return Slot(std::move(out));
}

std::string Slot::to_string() const {
  return to_json();
}

std::string Slot::to_json() const {
  if (!defined()) return "{ \"_type\": \"Slot\", \"defined\": false }";
  std::ostringstream oss;
  oss << "{\n";
  oss << "  \"_type\": \"Slot\",\n";
  oss << "  \"defined\": true,\n";
  oss << "  \"size\": " << size() << ",\n";
  oss << "  \"values\": [";
  if (!values().empty()) oss << "\n";
  for (std::size_t i = 0; i < values().size(); ++i) {
    oss << "    " << value_to_json(values()[i]);
    if (i + 1 < values().size()) oss << ",";
    oss << "\n";
  }
  if (!values().empty()) oss << "  ";
  oss << "]\n";
  oss << "}";
  return oss.str();
}

void Slot::show(std::ostream& os) const { os << to_string(); }

const detail::SlotImpl& Slot::impl_ref_() const {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  return *impl_;
}

detail::SlotImpl& Slot::mut_impl_() {
  PLUSS_CHECK(defined(), "Slot: undefined handle");
  if (impl_.use_count() > 1) {
    impl_ = pluss::msharp::make_intrusive<detail::SlotImpl>(impl_ref_().values_);
  }
  return *impl_;
}

std::ostream& operator<<(std::ostream& os, const Slot& slot) {
  slot.show(os);
  return os;
}

} // namespace pluss::bundle

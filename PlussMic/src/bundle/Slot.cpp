#include "bundle/Slot.h"

#include <algorithm>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

#include <fmt/format.h>

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"
#include "pluss/coden/note_factory.h"
#include "pluss/codeh/timeindex_factory.h"
#include "pluss/codeh/timeline_factory.h"
#include "kindle/Stream.h"

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
        if constexpr (std::is_same_v<T, pluss::Series>) return fmt::format("\"Series[{}]\"", v.dtype().name);
        if constexpr (std::is_same_v<T, pluss::Note>) return fmt::format("\"Note[numel={}]\"", v.numel());
        if constexpr (std::is_same_v<T, pluss::Timeline>) return fmt::format("\"Timeline[numel={}]\"", v.numel());
        if constexpr (std::is_same_v<T, pluss::TimeIndex>) return fmt::format("\"TimeIndex[size={}]\"", v.size());
        return "\"unknown\"";
      },
      value);
}

std::string value_type_name(const Slot::Value& value) {
  return std::visit(
      [](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) return "null";
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


#include "bundle/ops/slot/SlotLifecycleAndOps.inc"
#include "bundle/ops/slot/SlotInteropAndRender.inc"

} // namespace pluss::bundle

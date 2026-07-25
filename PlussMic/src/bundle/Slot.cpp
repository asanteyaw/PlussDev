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

Slot::Value bool_slot_value(bool value) {
  return pluss::make_series(value, pluss::codem::ScalarType::Bool);
}

pluss::table::Thread thread_from_slot_value(const Slot::Value& value) {
  return std::visit(
      [](const auto& v) -> pluss::table::Thread {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
          PLUSS_FAIL("Slot: null value has no Thread payload");
        } else if constexpr (std::is_same_v<T, pluss::Series>) {
          return pluss::table::Thread(v);
        } else if constexpr (std::is_same_v<T, pluss::Note>) {
          PLUSS_CHECK(v.numel() == 1, "Slot: Note value must be scalar-like for Thread delegation");
          return pluss::table::Thread(pluss::make_note(std::vector<std::string>{std::string(v.at(0))}));
        } else if constexpr (std::is_same_v<T, pluss::Timeline>) {
          PLUSS_CHECK(v.numel() == 1, "Slot: Timeline value must be scalar-like for Thread delegation");
          const auto ticks = v.to_epoch_nanos_vector();
          return pluss::table::Thread(pluss::from_epoch_nanos(
              std::vector<std::int64_t>{ticks[0]},
              std::vector<std::int64_t>{1}));
        } else if constexpr (std::is_same_v<T, pluss::TimeIndex>) {
          PLUSS_CHECK(v.size() == 1, "Slot: TimeIndex value must be scalar-like for Thread delegation");
          const auto ticks = v.timeline().to_epoch_nanos_vector();
          return pluss::table::Thread(pluss::make_timeindex(std::vector<std::int64_t>{ticks[0]}, false));
        }
      },
      value);
}

Slot::Value slot_value_from_thread_payload(const pluss::table::Thread& thread) {
  switch (thread.kind()) {
    case pluss::table::ThreadKind::Series:
      return thread.as_series();
    case pluss::table::ThreadKind::Note:
      return thread.as_note();
    case pluss::table::ThreadKind::Timeline:
      return thread.as_timeline();
    case pluss::table::ThreadKind::TimeIndex:
      return thread.as_timeindex();
  }
  return std::monostate{};
}

std::string series_scalar_to_string(const pluss::Series& series) {
  PLUSS_CHECK(series.numel() == 1, "Slot::cast(Note): Series value must be scalar-like");
  switch (series.scalar_type()) {
    case pluss::codem::ScalarType::Bool:
      return series.to_vec<bool>()[0] ? "true" : "false";
    case pluss::codem::ScalarType::Int64:
      return std::to_string(series.to_vec<std::int64_t>()[0]);
    case pluss::codem::ScalarType::UInt64:
      return std::to_string(series.to_vec<std::uint64_t>()[0]);
    case pluss::codem::ScalarType::Float32: {
      std::ostringstream os;
      os << series.to(pluss::codem::ScalarType::Float32).to_vec<float>()[0];
      return os.str();
    }
    case pluss::codem::ScalarType::Float64: {
      std::ostringstream os;
      os << series.to(pluss::codem::ScalarType::Float64).to_vec<double>()[0];
      return os.str();
    }
    default: {
      std::ostringstream os;
      os << series.to(pluss::codem::ScalarType::Float64).to_vec<double>()[0];
      return os.str();
    }
  }
}

Slot::Value slot_value_to_note(const Slot::Value& value) {
  return std::visit(
      [](const auto& v) -> Slot::Value {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
          return std::monostate{};
        } else if constexpr (std::is_same_v<T, pluss::Series>) {
          return pluss::make_note(std::vector<std::string>{series_scalar_to_string(v)});
        } else if constexpr (std::is_same_v<T, pluss::Note>) {
          PLUSS_CHECK(v.numel() == 1, "Slot::cast(Note): Note value must be scalar-like");
          return pluss::make_note(std::vector<std::string>{std::string(v.at(0))});
        } else if constexpr (std::is_same_v<T, pluss::Timeline>) {
          PLUSS_CHECK(v.numel() == 1, "Slot::cast(Note): Timeline value must be scalar-like");
          const auto ticks = v.to_epoch_nanos_vector();
          return pluss::make_note(std::vector<std::string>{std::to_string(ticks[0])});
        } else if constexpr (std::is_same_v<T, pluss::TimeIndex>) {
          PLUSS_CHECK(v.size() == 1, "Slot::cast(Note): TimeIndex value must be scalar-like");
          const auto ticks = v.timeline().to_epoch_nanos_vector();
          return pluss::make_note(std::vector<std::string>{std::to_string(ticks[0])});
        }
      },
      value);
}

bool slot_value_matches_target(const Slot::Value& value,
                               pluss::table::ThreadCastTarget target) {
  return std::visit(
      [&](const auto& v) -> bool {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
          return true;
        } else if constexpr (std::is_same_v<T, pluss::Series>) {
          return target == pluss::table::ThreadCastTarget::Series;
        } else if constexpr (std::is_same_v<T, pluss::Note>) {
          return target == pluss::table::ThreadCastTarget::Note;
        } else if constexpr (std::is_same_v<T, pluss::Timeline>) {
          return target == pluss::table::ThreadCastTarget::Timeline;
        } else if constexpr (std::is_same_v<T, pluss::TimeIndex>) {
          return target == pluss::table::ThreadCastTarget::TimeIndex;
        }
      },
      value);
}

Slot::Value scalar_fill_value_for_target(pluss::table::ThreadCastTarget target,
                                         double numeric,
                                         std::string_view text,
                                         const pluss::Timeline* timeline,
                                         const pluss::TimeIndex* timeindex) {
  switch (target) {
    case pluss::table::ThreadCastTarget::Series:
      return pluss::make_series(numeric, pluss::codem::ScalarType::Float64);
    case pluss::table::ThreadCastTarget::Note:
      return pluss::make_note(text);
    case pluss::table::ThreadCastTarget::Timeline:
      PLUSS_CHECK(timeline != nullptr, "Slot::fillnull(Timeline): fill value missing");
      PLUSS_CHECK(timeline->numel() == 1, "Slot::fillnull(Timeline): fill value must be scalar-like");
      return *timeline;
    case pluss::table::ThreadCastTarget::TimeIndex:
      PLUSS_CHECK(timeindex != nullptr, "Slot::fillnull(TimeIndex): fill value missing");
      PLUSS_CHECK(timeindex->size() == 1, "Slot::fillnull(TimeIndex): fill value must be scalar-like");
      return *timeindex;
  }
  PLUSS_FAIL("Slot::fillnull: unsupported target");
}

template <class FillFn>
Slot fill_slot_nulls(const Slot& slot,
                     pluss::table::ThreadCastTarget target,
                     std::string_view where,
                     FillFn&& fill_fn,
                     Slot::Value monostate_fill) {
  PLUSS_CHECK(slot.defined(), "Slot: undefined handle");
  std::vector<Slot::Value> out;
  out.reserve(slot.values().size());
  for (const auto& value : slot.values()) {
    if (!slot_value_matches_target(value, target)) {
      throw pluss::Error(std::string(where) + ": non-null Slot values must match fill payload type");
    }
    if (std::holds_alternative<std::monostate>(value)) {
      out.push_back(monostate_fill);
    } else if (target == pluss::table::ThreadCastTarget::Series) {
      out.push_back(fill_fn(thread_from_slot_value(value)));
    } else {
      out.push_back(value);
    }
  }
  return Slot(std::move(out));
}

bool slot_value_invalid(const Slot::Value& value) {
  return std::visit(
      [](const auto& v) -> bool {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
          return true;
        } else if constexpr (std::is_same_v<T, pluss::Series>) {
          const auto dtype = v.scalar_type();
          if (dtype == pluss::codem::ScalarType::Float32) {
            const auto x = v.to(pluss::codem::ScalarType::Float32).template to_vec<float>();
            return std::any_of(x.begin(), x.end(), [](float z) { return !std::isfinite(z); });
          }
          if (dtype == pluss::codem::ScalarType::Float64) {
            const auto x = v.to(pluss::codem::ScalarType::Float64).template to_vec<double>();
            return std::any_of(x.begin(), x.end(), [](double z) { return !std::isfinite(z); });
          }
          return false;
        } else {
          try {
            return pluss::table::Thread(v).invalid().any(false).to_int() != 0;
          } catch (const std::exception&) {
            return false;
          }
        }
      },
      value);
}

bool slot_value_isnull(const Slot::Value& value) {
  return std::visit(
      [](const auto& v) -> bool {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
          return true;
        } else if constexpr (std::is_same_v<T, pluss::Series>) {
          const auto dtype = v.scalar_type();
          if (dtype == pluss::codem::ScalarType::Float32) {
            const auto x = v.to(pluss::codem::ScalarType::Float32).template to_vec<float>();
            return std::any_of(x.begin(), x.end(), [](float z) { return std::isnan(z); });
          }
          if (dtype == pluss::codem::ScalarType::Float64) {
            const auto x = v.to(pluss::codem::ScalarType::Float64).template to_vec<double>();
            return std::any_of(x.begin(), x.end(), [](double z) { return std::isnan(z); });
          }
          return false;
        } else {
          return false;
        }
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

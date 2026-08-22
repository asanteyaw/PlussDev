#include "scribble/Slot.h"

#include <algorithm>
#include <cmath>
#include <optional>
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

namespace pluss::scribble {

namespace {

template <typename T>
bool all_type(const std::vector<Slot::Value>& values) {
  return std::all_of(values.begin(), values.end(), [](const auto& v) { return std::holds_alternative<T>(v); });
}

struct SlotProfile {
  SlotKind kind{SlotKind::Empty};
  bool has_null{false};
  bool stream_compatible{false};
};

SlotKind slot_kind_of_value(const Slot::Value& value) {
  return std::visit(
      [](const auto& v) -> SlotKind {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) return SlotKind::Null;
        if constexpr (std::is_same_v<T, pluss::Series>) return SlotKind::Series;
        if constexpr (std::is_same_v<T, pluss::Note>) return SlotKind::Note;
        if constexpr (std::is_same_v<T, pluss::Timeline>) return SlotKind::Timeline;
        if constexpr (std::is_same_v<T, pluss::TimeIndex>) return SlotKind::TimeIndex;
        return SlotKind::Mixed;
      },
      value);
}

bool slot_value_scalar_like(const Slot::Value& value) {
  return std::visit(
      [](const auto& v) -> bool {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) return false;
        if constexpr (std::is_same_v<T, pluss::Series>) return v.numel() == 1;
        if constexpr (std::is_same_v<T, pluss::Note>) return v.numel() == 1;
        if constexpr (std::is_same_v<T, pluss::Timeline>) return v.numel() == 1;
        if constexpr (std::is_same_v<T, pluss::TimeIndex>) return v.size() == 1;
        return false;
      },
      value);
}

SlotProfile analyze_slot(const std::vector<Slot::Value>& values) {
  if (values.empty()) {
    return SlotProfile{.kind = SlotKind::Empty, .has_null = false, .stream_compatible = true};
  }

  std::optional<SlotKind> seen_kind;
  std::optional<pluss::codem::ScalarType> series_dtype;
  bool has_null = false;
  bool scalar_like = true;

  for (const auto& value : values) {
    const auto value_kind = slot_kind_of_value(value);
    if (value_kind == SlotKind::Null) {
      has_null = true;
      continue;
    }
    if (!seen_kind.has_value()) {
      seen_kind = value_kind;
    } else if (*seen_kind != value_kind) {
      return SlotProfile{.kind = SlotKind::Mixed, .has_null = has_null, .stream_compatible = false};
    }

    scalar_like = scalar_like && slot_value_scalar_like(value);
    if (value_kind == SlotKind::Series) {
      const auto& s = std::get<pluss::Series>(value);
      if (!series_dtype.has_value()) {
        series_dtype = s.scalar_type();
      } else if (*series_dtype != s.scalar_type()) {
        return SlotProfile{.kind = SlotKind::Series, .has_null = has_null, .stream_compatible = false};
      }
    }
  }

  if (!seen_kind.has_value()) {
    return SlotProfile{.kind = SlotKind::Null, .has_null = true, .stream_compatible = false};
  }

  return SlotProfile{
      .kind = *seen_kind,
      .has_null = has_null,
      .stream_compatible = !has_null && scalar_like};
}

std::string slot_kind_name(SlotKind kind) {
  switch (kind) {
    case SlotKind::Empty: return "Empty";
    case SlotKind::Null: return "Null";
    case SlotKind::Series: return "Series";
    case SlotKind::Note: return "Note";
    case SlotKind::Timeline: return "Timeline";
    case SlotKind::TimeIndex: return "TimeIndex";
    case SlotKind::Mixed: return "Mixed";
  }
  return "Unknown";
}

SlotKind slot_kind_from_stream_kind(pluss::kindle::StreamKind kind) {
  switch (kind) {
    case pluss::kindle::StreamKind::Series: return SlotKind::Series;
    case pluss::kindle::StreamKind::Note: return SlotKind::Note;
    case pluss::kindle::StreamKind::Timeline: return SlotKind::Timeline;
    case pluss::kindle::StreamKind::TimeIndex: return SlotKind::TimeIndex;
  }
  return SlotKind::Mixed;
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
  static const pluss::Series* false_scalar = [] {
    auto s = pluss::Series::zeros({1}, pluss::codem::ScalarType::Bool);
    reinterpret_cast<bool*>(s.raw_data())[0] = false;
    return new pluss::Series(std::move(s));
  }();
  static const pluss::Series* true_scalar = [] {
    auto s = pluss::Series::zeros({1}, pluss::codem::ScalarType::Bool);
    reinterpret_cast<bool*>(s.raw_data())[0] = true;
    return new pluss::Series(std::move(s));
  }();
  return value ? *true_scalar : *false_scalar;
}

pluss::Series validity_mask_from_valid(const std::vector<bool>& valid) {
  const auto n = static_cast<std::int64_t>(valid.size());
  auto mask = pluss::Series::zeros({n}, pluss::codem::ScalarType::Bool);
  auto* out = reinterpret_cast<bool*>(mask.raw_data());
  for (std::int64_t i = 0; i < n; ++i) out[i] = valid[static_cast<std::size_t>(i)];
  return mask;
}

pluss::kindle::Stream slot_family_stream(const Slot& slot,
                                         SlotKind required_kind,
                                         std::string_view where) {
  PLUSS_CHECK(slot.defined(), "Slot: undefined handle");
  PLUSS_CHECK(required_kind == SlotKind::Series ||
                  required_kind == SlotKind::Note ||
                  required_kind == SlotKind::Timeline ||
                  required_kind == SlotKind::TimeIndex,
              "Slot accessor: unsupported required family");
  PLUSS_CHECK(slot.kind() == required_kind,
              std::string(where) + ": slot kind must be " + slot_kind_name(required_kind));

  const auto n = slot.size();
  std::vector<bool> valid(static_cast<std::size_t>(n), true);
  bool has_null = false;

  switch (required_kind) {
    case SlotKind::Series: {
      const auto first_it = std::find_if(slot.values().begin(), slot.values().end(), [](const auto& value) {
        return std::holds_alternative<pluss::Series>(value);
      });
      PLUSS_CHECK(first_it != slot.values().end(),
                  std::string(where) + ": Series slot has no concrete Series values");
      const auto& first = std::get<pluss::Series>(*first_it);
      const auto dtype = first.scalar_type();
      if (dtype == pluss::codem::ScalarType::Bool) {
        std::vector<bool> out(static_cast<std::size_t>(n), false);
        for (std::int64_t i = 0; i < n; ++i) {
          const auto& value = slot.at(i);
          if (std::holds_alternative<std::monostate>(value)) {
            has_null = true;
            valid[static_cast<std::size_t>(i)] = false;
            continue;
          }
          const auto& s = std::get<pluss::Series>(value);
          PLUSS_CHECK(s.numel() == 1, std::string(where) + ": Series values must be scalar-like");
          out[static_cast<std::size_t>(i)] = s.to_vec<bool>()[0];
        }
        return pluss::kindle::Stream(
            pluss::make_series(out, {n}),
            has_null ? std::optional<pluss::Series>(validity_mask_from_valid(valid)) : std::nullopt);
      }
      if (dtype == pluss::codem::ScalarType::Int64) {
        std::vector<std::int64_t> out(static_cast<std::size_t>(n), 0);
        for (std::int64_t i = 0; i < n; ++i) {
          const auto& value = slot.at(i);
          if (std::holds_alternative<std::monostate>(value)) {
            has_null = true;
            valid[static_cast<std::size_t>(i)] = false;
            continue;
          }
          const auto& s = std::get<pluss::Series>(value);
          PLUSS_CHECK(s.numel() == 1, std::string(where) + ": Series values must be scalar-like");
          out[static_cast<std::size_t>(i)] = s.to(pluss::codem::ScalarType::Int64).to_vec<std::int64_t>()[0];
        }
        return pluss::kindle::Stream(
            pluss::make_series(out, {n}),
            has_null ? std::optional<pluss::Series>(validity_mask_from_valid(valid)) : std::nullopt);
      }
      std::vector<double> out(static_cast<std::size_t>(n), 0.0);
      for (std::int64_t i = 0; i < n; ++i) {
        const auto& value = slot.at(i);
        if (std::holds_alternative<std::monostate>(value)) {
          has_null = true;
          valid[static_cast<std::size_t>(i)] = false;
          continue;
        }
        const auto& s = std::get<pluss::Series>(value);
        PLUSS_CHECK(s.numel() == 1, std::string(where) + ": Series values must be scalar-like");
        out[static_cast<std::size_t>(i)] = s.to(pluss::codem::ScalarType::Float64).to_vec<double>()[0];
      }
      return pluss::kindle::Stream(
          pluss::make_series(out, {n}, dtype),
          has_null ? std::optional<pluss::Series>(validity_mask_from_valid(valid)) : std::nullopt);
    }
    case SlotKind::Note: {
      std::vector<std::string> out(static_cast<std::size_t>(n));
      for (std::int64_t i = 0; i < n; ++i) {
        const auto& value = slot.at(i);
        if (std::holds_alternative<std::monostate>(value)) {
          has_null = true;
          valid[static_cast<std::size_t>(i)] = false;
          continue;
        }
        const auto& note = std::get<pluss::Note>(value);
        PLUSS_CHECK(note.numel() == 1, std::string(where) + ": Note values must be scalar-like");
        out[static_cast<std::size_t>(i)] = std::string(note.at(0));
      }
      return pluss::kindle::Stream(
          pluss::make_note(out),
          has_null ? std::optional<pluss::Series>(validity_mask_from_valid(valid)) : std::nullopt);
    }
    case SlotKind::Timeline: {
      std::vector<std::int64_t> ticks(static_cast<std::size_t>(n), 0);
      std::optional<std::string> tz_name;
      for (std::int64_t i = 0; i < n; ++i) {
        const auto& value = slot.at(i);
        if (std::holds_alternative<std::monostate>(value)) {
          has_null = true;
          valid[static_cast<std::size_t>(i)] = false;
          continue;
        }
        const auto& ts = std::get<pluss::Timeline>(value);
        PLUSS_CHECK(ts.numel() == 1, std::string(where) + ": Timeline values must be scalar-like");
        ticks[static_cast<std::size_t>(i)] = ts.to_epoch_nanos_vector()[0];
        std::string value_tz(ts.tz());
        if (!tz_name.has_value()) {
          tz_name = std::move(value_tz);
        } else {
          PLUSS_CHECK(*tz_name == value_tz,
                      std::string(where) + ": Timeline slot values carry mismatched timezones ('" +
                          *tz_name + "' vs '" + value_tz +
                          "'); a Stream's Timeline column has one shared tz, so relabel with tz() "
                          "(or localize()) to a common zone before combining");
        }
      }
      // A kindle::Stream's Timeline column carries exactly one shared tz label
      // (mirroring Thread/DataFrame). An all-null slot has nothing to derive a
      // tz from, so it falls back to "UTC" -- same default every other
      // tz-less Timeline factory in this codebase already uses.
      return pluss::kindle::Stream(
          pluss::timeline::from_epoch(ticks, {n}, tz_name.value_or("UTC")),
          has_null ? std::optional<pluss::Series>(validity_mask_from_valid(valid)) : std::nullopt);
    }
    case SlotKind::TimeIndex: {
      std::vector<std::int64_t> ticks(static_cast<std::size_t>(n), 0);
      std::optional<std::string> tz_name;
      for (std::int64_t i = 0; i < n; ++i) {
        const auto& value = slot.at(i);
        if (std::holds_alternative<std::monostate>(value)) {
          has_null = true;
          valid[static_cast<std::size_t>(i)] = false;
          continue;
        }
        const auto& index = std::get<pluss::TimeIndex>(value);
        PLUSS_CHECK(index.size() == 1, std::string(where) + ": TimeIndex values must be scalar-like");
        ticks[static_cast<std::size_t>(i)] = index.timeline().to_epoch_nanos_vector()[0];
        std::string value_tz(index.timeline().tz());
        if (!tz_name.has_value()) {
          tz_name = std::move(value_tz);
        } else {
          PLUSS_CHECK(*tz_name == value_tz,
                      std::string(where) + ": TimeIndex slot values carry mismatched timezones ('" +
                          *tz_name + "' vs '" + value_tz +
                          "'); a Stream's TimeIndex column has one shared tz, so relabel with tz() "
                          "(or localize()) to a common zone before combining");
        }
      }
      return pluss::kindle::Stream(
          pluss::make_timeindex(pluss::timeline::from_epoch(ticks, {n}, tz_name.value_or("UTC")), false),
          has_null ? std::optional<pluss::Series>(validity_mask_from_valid(valid)) : std::nullopt);
    }
    case SlotKind::Empty:
    case SlotKind::Null:
    case SlotKind::Mixed:
      break;
  }
  PLUSS_FAIL("Slot accessor: unsupported slot family");
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
          return pluss::table::Thread(pluss::timeline::from_epoch(
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

Slot fill_slot_nulls(const Slot& slot,
                     pluss::table::ThreadCastTarget target,
                     std::string_view where,
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

  void invalidate_profile() noexcept { profile_cache_.reset(); }

  std::vector<Slot::Value> values_{};
  mutable std::optional<SlotProfile> profile_cache_{};
};
} // namespace detail

namespace {

const SlotProfile& slot_profile(const detail::SlotImpl& impl) {
  if (!impl.profile_cache_.has_value()) {
    impl.profile_cache_ = analyze_slot(impl.values_);
  }
  return *impl.profile_cache_;
}

} // namespace


#include "scribble/ops/slot/SlotLifecycleAndOps.inc"
#include "scribble/ops/slot/SlotAccessors.inc"
#include "scribble/ops/slot/SlotInteropAndRender.inc"

} // namespace pluss::scribble

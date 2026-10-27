#include "scribble/registry.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <fstream>
#include <numeric>
#include <ostream>
#include <random>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>

#include "kindle/catalog.h"
#include "table/DataFrame.h"
#include "CodeM/core/Errors.h"
#include "kindle/catalog.h"
#include "loader/Loader.h"
#include "table/io/CsvSchemaConvert.h"
#include <fmt/format.h>

namespace pluss::scribble {

namespace {

std::vector<std::string> normalize_names(std::initializer_list<std::string_view> keys) {
  std::vector<std::string> out;
  out.reserve(keys.size());
  std::unordered_set<std::string> seen;
  for (auto sv : keys) {
    std::string key(sv);
    PLUSS_CHECK(!key.empty(), "Registry: key cannot be empty");
    PLUSS_CHECK(seen.insert(key).second, "Registry: duplicate key '" + key + "'");
    out.push_back(std::move(key));
  }
  return out;
}

std::vector<std::string> normalize_names(std::span<const std::string_view> keys) {
  std::vector<std::string> out;
  out.reserve(keys.size());
  std::unordered_set<std::string> seen;
  for (auto sv : keys) {
    std::string key(sv);
    PLUSS_CHECK(!key.empty(), "Registry: key cannot be empty");
    PLUSS_CHECK(seen.insert(key).second, "Registry: duplicate key '" + key + "'");
    out.push_back(std::move(key));
  }
  return out;
}

std::vector<std::string> normalize_names(const std::vector<std::string>& keys) {
  std::vector<std::string> out;
  out.reserve(keys.size());
  std::unordered_set<std::string> seen;
  for (const auto& key : keys) {
    PLUSS_CHECK(!key.empty(), "Registry: key cannot be empty");
    PLUSS_CHECK(seen.insert(key).second, "Registry: duplicate key '" + key + "'");
    out.push_back(key);
  }
  return out;
}

std::string trim_copy(std::string_view in) {
  std::size_t start = 0;
  std::size_t end = in.size();
  while (start < end && std::isspace(static_cast<unsigned char>(in[start]))) ++start;
  while (end > start && std::isspace(static_cast<unsigned char>(in[end - 1]))) --end;
  return std::string(in.substr(start, end - start));
}

std::string lower_copy(std::string s) {
  for (auto& ch : s) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return s;
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

std::pair<std::int64_t, std::int64_t> normalize_slice(std::int64_t start, std::int64_t stop, std::int64_t size) {
  std::int64_t s = start < 0 ? start + size : start;
  std::int64_t e = stop < 0 ? stop + size : stop;
  s = std::clamp<std::int64_t>(s, 0, size);
  e = std::clamp<std::int64_t>(e, 0, size);
  if (e < s) e = s;
  return {s, e};
}

Slot::Value slot_value_from_scalar_slate(const pluss::table::Slate& slate) {
  switch (slate.kind()) {
    case pluss::table::SlateKind::Series:
      return slate.as_series();
    case pluss::table::SlateKind::Note:
      return slate.as_note();
    case pluss::table::SlateKind::Timeline:
      return slate.as_timeline();
    case pluss::table::SlateKind::Duration:
      return slate.as_duration();
  }
  return std::monostate{};
}

// Registry::Slot is heterogeneous and cell-by-cell (unlike Catalog's homogeneous
// columnar Stream), so each raw token is intentionally inferred on its own — a
// column can legitimately mix a Series-kind value in one row with a Note-kind value
// in the next. Because each call only sees a single token, it passes
// prefer_numeric_over_bool=true so an ambiguous digit-only literal like "1"/"0"
// resolves to Int64 rather than Bool; a whole real column of such tokens has no
// surrounding context here to fall back on Bool the way Catalog's whole-column
// inference can.
Slot::Value infer_slot_value_from_raw_token(std::string_view col_name,
                                            std::size_t source_col_index,
                                            std::string_view raw_token,
                                            const pluss::loader::ReadCsvOptions& options) {
  std::vector<std::string> one{std::string(raw_token)};
  const auto inferred = pluss::table::detail::infer_slate_from_raw_csv_column(
      col_name,
      source_col_index,
      one,
      options,
      /*prefer_numeric_over_bool=*/true);
  if (pluss::table::detail::has_validity_mask(inferred)) {
    const auto valid = pluss::table::detail::validity_mask(inferred)->host<bool>();
    if (valid.size() == 1 && !valid[0]) {
      return std::monostate{};
    }
  }
  return slot_value_from_scalar_slate(inferred);
}

std::string unique_key_name(std::string base, const std::unordered_set<std::string>& taken) {
  if (taken.find(base) == taken.end()) {
    return base;
  }
  auto candidate = base + "_case";
  std::int64_t suffix = 1;
  while (taken.find(candidate) != taken.end()) {
    candidate = fmt::format("{}_{}", base, suffix++);
  }
  return candidate;
}

Registry registry_from_raw_batch(const pluss::loader::CsvRawBatch& raw,
                                 const pluss::loader::ReadCsvOptions& infer_options) {
  std::vector<std::pair<std::string, Slot>> out;
  out.reserve(raw.features.size() + (raw.cases.has_value() ? 1u : 0u));
  std::unordered_set<std::string> keys_seen;
  keys_seen.reserve(raw.features.size() * 2 + 1);

  for (const auto& col : raw.features) {
    std::vector<Slot::Value> values;
    if (col.note_values.has_value()) {
      values.reserve(static_cast<std::size_t>(col.note_values->numel()));
      for (std::int64_t i = 0; i < col.note_values->numel(); ++i) {
        values.push_back(infer_slot_value_from_raw_token(
            col.name,
            col.source_index,
            col.note_values->at(i),
            infer_options));
      }
    } else {
      values.reserve(col.values.size());
      for (const auto& token : col.values) {
        values.push_back(infer_slot_value_from_raw_token(
            col.name,
            col.source_index,
            token,
            infer_options));
      }
    }
    keys_seen.insert(col.name);
    out.emplace_back(col.name, Slot(std::move(values)));
  }

  if (raw.cases.has_value()) {
    const auto case_name = unique_key_name(raw.cases->name, keys_seen);
    std::vector<Slot::Value> values;
    if (raw.cases->note_values.has_value()) {
      values.reserve(static_cast<std::size_t>(raw.cases->note_values->numel()));
      for (std::int64_t i = 0; i < raw.cases->note_values->numel(); ++i) {
        values.push_back(infer_slot_value_from_raw_token(
            case_name,
            raw.cases->source_index,
            raw.cases->note_values->at(i),
            infer_options));
      }
    } else {
      values.reserve(raw.cases->values.size());
      for (const auto& token : raw.cases->values) {
        values.push_back(infer_slot_value_from_raw_token(
            case_name,
            raw.cases->source_index,
            token,
            infer_options));
      }
    }
    out.emplace_back(case_name, Slot(std::move(values)));
  }

  return Registry(std::move(out));
}

} // namespace

namespace detail {
class RegistryImpl final : public pluss::msharp::RefCounted {
public:
  RegistryImpl() = default;
  explicit RegistryImpl(std::vector<std::pair<std::string, Slot>> fields) {
    keys_.reserve(fields.size());
    slots_.reserve(fields.size());
    for (auto& [k, s] : fields) {
      PLUSS_CHECK(!k.empty(), "Registry: key cannot be empty");
      PLUSS_CHECK(s.defined(), "Registry: slot is undefined");
      PLUSS_CHECK(name_to_pos_.find(k) == name_to_pos_.end(), "Registry: duplicate key '" + k + "'");
      const auto idx = static_cast<std::int64_t>(keys_.size());
      name_to_pos_.emplace(k, idx);
      keys_.push_back(std::move(k));
      slots_.push_back(std::move(s));
    }
  }

  std::vector<std::string> keys_{};
  std::vector<Slot> slots_{};
  std::unordered_map<std::string, std::int64_t> name_to_pos_{};
};
} // namespace detail


#include "scribble/ops/registry/RegistryLifecycleAndOps.inc"
#include "scribble/ops/registry/RegistryAccessors.inc"
#include "scribble/ops/registry/RegistryInteropAndRender.inc"

} // namespace pluss::scribble

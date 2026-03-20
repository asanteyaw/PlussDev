#include "table/DataFrame.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include "pluss/coden/note_factory.h"
#include "table/display/Display.h"

namespace pluss::table {

namespace {

std::string unique_feature_name(std::string candidate,
                                std::unordered_set<std::string>& seen,
                                std::int64_t row_index) {
  if (candidate.empty()) {
    candidate = fmt::format("cs_{}", row_index);
  }
  if (seen.insert(candidate).second) {
    return candidate;
  }

  auto base = candidate;
  std::int64_t suffix = 1;
  while (true) {
    auto probe = fmt::format("{}_{}", base, suffix++);
    if (seen.insert(probe).second) {
      return probe;
    }
  }
}

pluss::Note note_from_strings(const std::vector<std::string>& values) {
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(values.size())};
  return pluss::make_note(values, shape);
}

std::string normalize_case_label(std::string raw) {
  if (raw.empty()) return raw;
  char* end = nullptr;
  const auto parsed = std::strtod(raw.c_str(), &end);
  if (end != nullptr && *end == '\0' && std::isfinite(parsed) && std::floor(parsed) == parsed) {
    return fmt::format("{:.0f}", parsed);
  }
  return raw;
}

std::string case_label_from_axis(const Thread& axis, std::int64_t row) {
  if (!axis.is_valid(row)) {
    return {};
  }
  switch (axis.kind()) {
    case ThreadKind::Series: {
      const auto& s = axis.as_series();
      switch (s.scalar_type()) {
        case pluss::codem::ScalarType::Bool:
          return s.to_vec<bool>()[static_cast<std::size_t>(row)] ? "true" : "false";
        case pluss::codem::ScalarType::Int8:
        case pluss::codem::ScalarType::Int16:
        case pluss::codem::ScalarType::Int32:
        case pluss::codem::ScalarType::Int64:
          return fmt::format("{}", s.to_vec<std::int64_t>()[static_cast<std::size_t>(row)]);
        case pluss::codem::ScalarType::UInt8:
        case pluss::codem::ScalarType::UInt16:
        case pluss::codem::ScalarType::UInt32:
        case pluss::codem::ScalarType::UInt64:
          return fmt::format("{}", s.to_vec<std::uint64_t>()[static_cast<std::size_t>(row)]);
        case pluss::codem::ScalarType::Float16:
        case pluss::codem::ScalarType::BFloat16:
        case pluss::codem::ScalarType::Float32:
        case pluss::codem::ScalarType::Float64: {
          const auto value = s.to_vec<double>()[static_cast<std::size_t>(row)];
          if (std::isfinite(value) && std::floor(value) == value) {
            return fmt::format("{:.0f}", value);
          }
          return fmt::format("{}", value);
        }
        default:
          return std::string(axis.cast(ThreadCastTarget::Note).as_note().at(row));
      }
    }
    case ThreadKind::Note:
      return normalize_case_label(std::string(axis.as_note().at(row)));
    case ThreadKind::Timeline:
      return fmt::format("{}", axis.as_timeline().to_epoch_nanos_vector()[static_cast<std::size_t>(row)]);
    case ThreadKind::TimeIndex:
      return fmt::format("{}",
                         axis.as_timeindex().timeline().to_epoch_nanos_vector()[static_cast<std::size_t>(row)]);
  }
  return {};
}

} // namespace

DataFrameView DataFrame::transpose(bool render) const {
  const auto rows = ncases();
  const auto cols = nfeats();

  const auto src_columns = features();
  std::vector<pluss::Note> src_notes;
  src_notes.reserve(src_columns.size());
  for (const auto& col : src_columns) {
    src_notes.push_back(col.cast(ThreadCastTarget::Note).as_note());
  }

  std::unordered_set<std::string> used_names;
  used_names.reserve(static_cast<std::size_t>(std::max<std::int64_t>(8, rows * 2)));

  std::vector<std::pair<std::string, Thread>> out_cols;
  out_cols.reserve(static_cast<std::size_t>(rows));

  for (std::int64_t r = 0; r < rows; ++r) {
    std::string feature_name;
    feature_name = normalize_case_label(case_label_from_axis(cases(), r));
    feature_name = unique_feature_name(std::move(feature_name), used_names, r);

    std::vector<std::string> values;
    values.reserve(static_cast<std::size_t>(cols));
    for (std::int64_t c = 0; c < cols; ++c) {
      if (!src_columns[static_cast<std::size_t>(c)].is_valid(r)) {
        values.emplace_back("NA");
      } else {
        values.emplace_back(src_notes[static_cast<std::size_t>(c)].at(r));
      }
    }

    out_cols.emplace_back(std::move(feature_name), Thread(note_from_strings(values)));
  }

  Thread out_cases(note_from_strings(feature_names()));
  auto out = DataFrame(std::move(out_cols), std::move(out_cases));
  return DataFrameView(std::move(out), render, "transpose", title());
}

} // namespace pluss::table

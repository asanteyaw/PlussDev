#include "table/io/SnapshotCsv.h"

#include <cctype>
#include <span>
#include <utility>
#include <vector>

#include "CodeM/core/Errors.h"
#include "loader/Loader.h"

namespace pluss::table::snapshot {

namespace {

pluss::table::SlateValue::HostAny snapshot_cell_host(const pluss::table::DataFrame& frame,
                                                      std::string_view feature,
                                                      std::int64_t row) {
  PLUSS_CHECK(frame.get(feature).has_value(),
              "Nexus::load: missing snapshot feature '" + std::string(feature) + "'");
  const auto column = frame.features(feature);
  PLUSS_CHECK(row >= 0 && row < column.size(),
              "snapshot row out of bounds for feature '" + std::string(feature) + "'");
  const auto index = row;
  return column.take(std::span<const std::int64_t>(&index, 1)).value().host();
}
} // namespace

std::string trim_ascii(std::string_view text) {
  std::size_t begin = 0;
  while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin]))) {
    ++begin;
  }
  std::size_t end = text.size();
  while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
    --end;
  }
  return std::string(text.substr(begin, end - begin));
}
std::string lower_trimmed(std::string_view text) {
  auto out = trim_ascii(text);
  for (auto& c : out) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return out;
}
std::string note_cell(const pluss::table::DataFrame& frame, std::string_view feature, std::int64_t row) {
  return snapshot_cell_host(frame, feature, row).to_str();
}
pluss::loader::ReadCsvOptions snapshot_read_options() {
  pluss::loader::ReadCsvOptions options;
  options.engine = pluss::loader::CsvEngine::Serial;
  options.infer_datetime_format = false;
  return options;
}
pluss::table::DataFrame load_snapshot_csv(std::string_view filepath) {
  const auto raw = pluss::loader::read_csv_raw_batch(filepath, snapshot_read_options());
  std::vector<std::pair<std::string, pluss::table::Slate>> features;
  features.reserve(raw.features.size());
  for (const auto& column : raw.features) {
    if (column.note_values.has_value()) {
      features.emplace_back(column.name, pluss::table::Slate(*column.note_values));
    } else {
      features.emplace_back(column.name, pluss::table::Slate(pluss::make_note(column.values)));
    }
  }
  return pluss::table::DataFrame(std::move(features));
}
std::int64_t int_cell(const pluss::table::DataFrame& frame, std::string_view feature, std::int64_t row) {
  const auto host = snapshot_cell_host(frame, feature, row);
  if (host.kind() == pluss::table::SlateValue::HostAny::Kind::String) {
    return std::stoll(trim_ascii(host.to_str()));
  }
  return host.to_int();
}
double double_cell(const pluss::table::DataFrame& frame, std::string_view feature, std::int64_t row) {
  const auto host = snapshot_cell_host(frame, feature, row);
  if (host.kind() == pluss::table::SlateValue::HostAny::Kind::String) {
    return std::stod(trim_ascii(host.to_str()));
  }
  return host.to_dbl();
}
bool bool_cell(const pluss::table::DataFrame& frame, std::string_view feature, std::int64_t row) {
  const auto column = frame.features(feature);
  PLUSS_CHECK(row >= 0 && row < column.size(),
              "snapshot row out of bounds for feature '" + std::string(feature) + "'");
  const auto index = row;
  const auto host = column.take(std::span<const std::int64_t>(&index, 1)).value().host();
  switch (host.kind()) {
    case pluss::table::SlateValue::HostAny::Kind::Bool:
    case pluss::table::SlateValue::HostAny::Kind::Int:
    case pluss::table::SlateValue::HostAny::Kind::UInt:
    case pluss::table::SlateValue::HostAny::Kind::Double:
      return host.to_int() != 0;
    case pluss::table::SlateValue::HostAny::Kind::String: {
      const auto value = lower_trimmed(host.to_str());
      if (value == "1" || value == "true") {
        return true;
      }
      if (value.empty() || value == "0" || value == "false") {
        return false;
      }
      PLUSS_FAIL("Nexus::load: malformed boolean value '" + value + "'");
    }
    case pluss::table::SlateValue::HostAny::Kind::Time:
      PLUSS_FAIL("Nexus::load: time snapshots cannot be decoded as boolean");
    case pluss::table::SlateValue::HostAny::Kind::Duration:
      PLUSS_FAIL("Nexus::load: duration snapshots cannot be decoded as boolean");
  }
  PLUSS_FAIL("Nexus::load: unsupported boolean snapshot scalar kind");
}
} // namespace pluss::table::snapshot

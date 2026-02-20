#include "table/DataFrame.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <numeric>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <variant>

#include <fmt/format.h>

#include "CodeM/core/Errors.h"
#include "CodeM/core/Half.h"
#include "loader/CsvLoader.h"
#include "loader/Loader.h"
#include "table/display/Display.h"

namespace pluss::table {

namespace detail {

class DataFrameImpl final : public pluss::msharp::RefCounted {
public:
  DataFrameImpl(std::vector<std::string> names,
                std::vector<Thread> columns,
                std::optional<Thread> cases = std::nullopt);
  void validate_and_reindex_();

  std::vector<std::string> names_;
  std::vector<Thread> columns_;
  std::unordered_map<std::string, std::size_t> name_to_idx_;
  std::int64_t nrows_{0};
  Thread cases_{};
};

} // namespace detail

namespace {

std::string owned_name(std::string_view sv) {
  return std::string(sv.data(), sv.size());
}

std::string lower_copy(std::string s) {
  for (auto& ch : s) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return s;
}

std::pair<std::int64_t, std::int64_t> normalize_slice(std::int64_t start, std::int64_t stop, std::int64_t size) {
  std::int64_t s = start < 0 ? start + size : start;
  std::int64_t e = stop < 0 ? stop + size : stop;
  s = std::clamp<std::int64_t>(s, 0, size);
  e = std::clamp<std::int64_t>(e, 0, size);
  if (e < s) {
    e = s;
  }
  return {s, e};
}

std::int64_t normalize_row_index(std::int64_t row, std::int64_t size) {
  auto out = row < 0 ? row + size : row;
  PLUSS_CHECK(out >= 0 && out < size, "DataFrame: row index out of bounds");
  return out;
}

std::vector<std::string> normalize_name_list(std::initializer_list<std::string_view> names) {
  std::vector<std::string> out;
  out.reserve(names.size());
  for (const auto n : names) {
    out.push_back(owned_name(n));
  }
  return out;
}

void validate_bool_mask(const pluss::Series& mask, std::int64_t expected_size, std::string_view where) {
  PLUSS_CHECK(mask.defined(), std::string(where) + ": mask is undefined");
  PLUSS_CHECK(mask.dim() == 1, std::string(where) + ": mask must be 1D");
  PLUSS_CHECK(mask.scalar_type() == pluss::codem::ScalarType::Bool, std::string(where) + ": mask dtype must be Bool");
  PLUSS_CHECK(mask.numel() == expected_size, std::string(where) + ": mask length mismatch");
}

pluss::Series bool_series_from_keep(const std::vector<bool>& keep) {
  const auto n = static_cast<std::int64_t>(keep.size());
  auto out = pluss::Series::zeros({n}, pluss::codem::ScalarType::Bool);
  auto* p = reinterpret_cast<bool*>(out.raw_data());
  for (std::int64_t i = 0; i < n; ++i) {
    p[i] = keep[static_cast<std::size_t>(i)];
  }
  return out;
}

Thread default_cases(std::int64_t nrows) {
  PLUSS_CHECK(nrows >= 0, "DataFrame: nrows must be non-negative");
  std::vector<std::int64_t> idx(static_cast<std::size_t>(nrows));
  std::iota(idx.begin(), idx.end(), static_cast<std::int64_t>(0));
  auto s = pluss::Series::empty({nrows}, pluss::codem::ScalarType::Int64);
  if (!idx.empty()) {
    std::memcpy(s.raw_data(), idx.data(), idx.size() * sizeof(std::int64_t));
  }
  return Thread(s);
}

bool is_numeric_series_type(const pluss::Series& s) {
  const auto dt = s.dtype();
  return dt.scalar == pluss::codem::ScalarType::Bool || dt.is_integral() || dt.is_floating();
}

bool extract_numeric_value(const pluss::Series& s, std::int64_t idx, double& out) {
  const auto* base = s.raw_data();
  switch (s.scalar_type()) {
    case pluss::codem::ScalarType::Bool:
      out = reinterpret_cast<const bool*>(base)[idx] ? 1.0 : 0.0;
      return true;
    case pluss::codem::ScalarType::Int8:
      out = static_cast<double>(reinterpret_cast<const std::int8_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::Int16:
      out = static_cast<double>(reinterpret_cast<const std::int16_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::Int32:
      out = static_cast<double>(reinterpret_cast<const std::int32_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::Int64:
      out = static_cast<double>(reinterpret_cast<const std::int64_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::UInt8:
      out = static_cast<double>(reinterpret_cast<const std::uint8_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::UInt16:
      out = static_cast<double>(reinterpret_cast<const std::uint16_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::UInt32:
      out = static_cast<double>(reinterpret_cast<const std::uint32_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::UInt64:
      out = static_cast<double>(reinterpret_cast<const std::uint64_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::Float16:
      out = pluss::codem::float16_to_float(reinterpret_cast<const pluss::codem::Float16*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::BFloat16:
      out = pluss::codem::bfloat16_to_float(reinterpret_cast<const pluss::codem::BFloat16*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::Float32:
      out = static_cast<double>(reinterpret_cast<const float*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::Float64:
      out = reinterpret_cast<const double*>(base)[idx];
      return true;
    default:
      return false;
  }
}

std::string series_scalar_string_at(const pluss::Series& s, std::int64_t row) {
  const auto* base = s.raw_data();
  switch (s.scalar_type()) {
    case pluss::codem::ScalarType::Bool:
      return reinterpret_cast<const bool*>(base)[row] ? "true" : "false";
    case pluss::codem::ScalarType::Int8:
      return fmt::format("{}", static_cast<int>(reinterpret_cast<const std::int8_t*>(base)[row]));
    case pluss::codem::ScalarType::Int16:
      return fmt::format("{}", reinterpret_cast<const std::int16_t*>(base)[row]);
    case pluss::codem::ScalarType::Int32:
      return fmt::format("{}", reinterpret_cast<const std::int32_t*>(base)[row]);
    case pluss::codem::ScalarType::Int64:
      return fmt::format("{}", reinterpret_cast<const std::int64_t*>(base)[row]);
    case pluss::codem::ScalarType::UInt8:
      return fmt::format("{}", static_cast<unsigned>(reinterpret_cast<const std::uint8_t*>(base)[row]));
    case pluss::codem::ScalarType::UInt16:
      return fmt::format("{}", reinterpret_cast<const std::uint16_t*>(base)[row]);
    case pluss::codem::ScalarType::UInt32:
      return fmt::format("{}", reinterpret_cast<const std::uint32_t*>(base)[row]);
    case pluss::codem::ScalarType::UInt64:
      return fmt::format("{}", reinterpret_cast<const std::uint64_t*>(base)[row]);
    case pluss::codem::ScalarType::Float16:
      return fmt::format("{:.6g}",
                         pluss::codem::float16_to_float(reinterpret_cast<const pluss::codem::Float16*>(base)[row]));
    case pluss::codem::ScalarType::BFloat16:
      return fmt::format("{:.6g}",
                         pluss::codem::bfloat16_to_float(reinterpret_cast<const pluss::codem::BFloat16*>(base)[row]));
    case pluss::codem::ScalarType::Float32:
      return fmt::format("{:.8g}", reinterpret_cast<const float*>(base)[row]);
    case pluss::codem::ScalarType::Float64:
      return fmt::format("{:.12g}", reinterpret_cast<const double*>(base)[row]);
    case pluss::codem::ScalarType::Complex64: {
      const auto v = reinterpret_cast<const pluss::codem::Complex64*>(base)[row];
      return fmt::format("({:.6g}{:+.6g}j)", v.real(), v.imag());
    }
    case pluss::codem::ScalarType::Complex128: {
      const auto v = reinterpret_cast<const pluss::codem::Complex128*>(base)[row];
      return fmt::format("({:.6g}{:+.6g}j)", v.real(), v.imag());
    }
    default:
      return "";
  }
}

std::string thread_scalar_string_at(const Thread& t, std::int64_t row) {
  if (t.has_validity_mask()) {
    const auto valid = t.validity_mask()->to_vec<bool>();
    if (!valid[static_cast<std::size_t>(row)]) {
      return "NA";
    }
  }

  switch (t.kind()) {
    case ThreadKind::Series:
      return series_scalar_string_at(t.as_series(), row);
    case ThreadKind::Note:
      return std::string(t.as_note().at(row));
    case ThreadKind::Timeline:
      return fmt::format("{}", t.as_timeline().to_epoch_nanos_vector()[static_cast<std::size_t>(row)]);
    case ThreadKind::TimeIndex:
      return fmt::format("{}", t.as_timeindex().timeline().to_epoch_nanos_vector()[static_cast<std::size_t>(row)]);
    default:
      return "";
  }
}

Thread make_f64_thread(const std::vector<double>& values) {
  const auto n = static_cast<std::int64_t>(values.size());
  auto s = pluss::Series::empty({n}, pluss::codem::ScalarType::Float64);
  if (!values.empty()) {
    std::memcpy(s.raw_data(), values.data(), values.size() * sizeof(double));
  }
  return Thread(s);
}

Thread make_note_thread(std::initializer_list<std::string_view> labels) {
  std::vector<std::string_view> views(labels.begin(), labels.end());
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(views.size())};
  auto note = pluss::coden::ops_internal::note_from_utf8(
      std::span<const std::string_view>(views.data(), views.size()),
      std::span<const std::int64_t>(shape.data(), shape.size()));
  return Thread(note);
}

double quantile_sorted(const std::vector<double>& sorted, double q) {
  PLUSS_CHECK(!sorted.empty(), "DataFrame::describe: quantile on empty data");
  const double pos = (static_cast<double>(sorted.size()) - 1.0) * q;
  const auto lo = static_cast<std::size_t>(std::floor(pos));
  const auto hi = static_cast<std::size_t>(std::ceil(pos));
  if (lo == hi) {
    return sorted[lo];
  }
  const double w = pos - static_cast<double>(lo);
  return sorted[lo] * (1.0 - w) + sorted[hi] * w;
}

std::int64_t normalize_view_start(std::int64_t start, std::int64_t nrows) {
  const std::int64_t s = start < 0 ? nrows + start : start;
  return std::clamp<std::int64_t>(s, 0, nrows);
}

std::int64_t normalize_view_stop(std::int64_t stop, std::int64_t nrows) {
  // Positive stop is exclusive. Negative stop is interpreted as inclusive-from-end
  // so `view(-5, -1)` includes the last row.
  const std::int64_t e = stop < 0 ? (nrows + stop + 1) : stop;
  return std::clamp<std::int64_t>(e, 0, nrows);
}

} // namespace

detail::DataFrameImpl::DataFrameImpl(std::vector<std::string> names,
                                     std::vector<Thread> columns,
                                     std::optional<Thread> cases)
    : names_(std::move(names)), columns_(std::move(columns)) {
  if (cases.has_value()) {
    cases_ = *cases;
  }
  validate_and_reindex_();
}

void detail::DataFrameImpl::validate_and_reindex_() {
  PLUSS_CHECK(names_.size() == columns_.size(), "DataFrame: names/columns size mismatch");
  name_to_idx_.clear();
  name_to_idx_.reserve(names_.size());

  std::optional<std::int64_t> expected_rows;
  for (std::size_t i = 0; i < names_.size(); ++i) {
    const auto& name = names_[i];
    const auto& col = columns_[i];
    PLUSS_CHECK(!name.empty(), "DataFrame: column names must be non-empty");
    PLUSS_CHECK(col.defined(), "DataFrame: column handle is undefined");
    PLUSS_CHECK(name_to_idx_.find(name) == name_to_idx_.end(), "DataFrame: duplicate column name '" + name + "'");
    name_to_idx_.emplace(name, i);

    const auto col_rows = col.size();
    if (!expected_rows.has_value()) {
      expected_rows = col_rows;
    } else {
      PLUSS_CHECK(*expected_rows == col_rows, "DataFrame: all columns must have identical length");
    }
  }

  nrows_ = expected_rows.value_or(0);

  if (!cases_.defined()) {
    cases_ = default_cases(nrows_);
  }
  PLUSS_CHECK(cases_.defined(), "DataFrame: cases are undefined");
  PLUSS_CHECK(cases_.size() == nrows_, "DataFrame: case length mismatch");
}

namespace {

pluss::msharp::intrusive_ptr<detail::DataFrameImpl> clone_impl(const detail::DataFrameImpl& src) {
  return pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, src.columns_, src.cases_);
}

} // namespace

ColumnRef::ColumnRef(std::string name, Thread column)
    : name_(std::move(name)), column_(std::move(column)) {}

const std::string& ColumnRef::name() const noexcept { return name_; }
const Thread& ColumnRef::thread() const noexcept { return column_; }
double ColumnRef::mean() const { return column_.mean(); }
Thread ColumnRef::head(std::int64_t n) const { return column_.head(n); }
ThreadValue ColumnRef::value() const { return column_.value(); }
Thread ColumnRef::values() const { return column_; }

DataFrame::DataFrame(std::vector<std::pair<std::string, Thread>> columns,
                     std::optional<Thread> cases) {
  std::vector<std::string> names;
  std::vector<Thread> values;
  names.reserve(columns.size());
  values.reserve(columns.size());
  for (auto& kv : columns) {
    names.push_back(std::move(kv.first));
    values.push_back(std::move(kv.second));
  }
  impl_ = pluss::msharp::make_intrusive<detail::DataFrameImpl>(std::move(names), std::move(values), std::move(cases));
}

DataFrame::DataFrame() = default;

DataFrame::DataFrame(pluss::msharp::intrusive_ptr<detail::DataFrameImpl> impl) : impl_(std::move(impl)) {}

DataFrame::DataFrame(const DataFrame& other) = default;
DataFrame::DataFrame(DataFrame&& other) noexcept = default;
DataFrame& DataFrame::operator=(const DataFrame& other) = default;
DataFrame& DataFrame::operator=(DataFrame&& other) noexcept = default;
DataFrame::~DataFrame() = default;

DataFrame DataFrame::load(std::string_view source_type, std::string_view filepath) {
  return load(source_type, filepath, pluss::loader::Config{});
}

DataFrame DataFrame::load(std::string_view source_type,
                          std::string_view filepath,
                          const pluss::loader::Config& config) {
  auto loaded = pluss::loader::load(source_type, filepath, config);
  if (loaded.is_frame()) {
    return loaded.dataframe();
  }
  PLUSS_FAIL("DataFrame::load: requested source returned a workbook. Use pluss::loader::load(...) for multi-sheet loads");
}

DataFrame DataFrame::load(std::string_view source_type,
                          std::string_view filepath,
                          std::initializer_list<pluss::loader::ConfigEntry> config_entries) {
  return load(source_type, filepath, pluss::loader::config_from(config_entries));
}

DataFrame DataFrame::load(std::string_view source_type,
                          std::string_view filepath,
                          const pluss::loader::ReadCsvOptions& options) {
  const auto source = lower_copy(owned_name(source_type));
  if (source == "csv") {
    return pluss::loader::load_csv(filepath, options);
  }
  PLUSS_FAIL("DataFrame::load: unsupported source type '" + std::string(source_type) + "'");
}

DataFrame DataFrame::load(std::string_view source_type,
                          std::string_view filepath,
                          char sep,
                          std::optional<std::size_t> nrows,
                          std::vector<std::string> usecols,
                          std::optional<std::string> case_col,
                          pluss::loader::BadLinePolicy on_bad_lines) {
  pluss::loader::ReadCsvOptions options;
  options.sep = sep;
  options.nrows = nrows;
  options.usecols = std::move(usecols);
  options.case_col = std::move(case_col);
  options.on_bad_lines = on_bad_lines;
  return load(source_type, filepath, options);
}

DataFrame DataFrame::load(std::string_view source_type,
                          std::string_view filepath,
                          char sep,
                          std::optional<std::size_t> nrows,
                          std::initializer_list<std::string_view> usecols,
                          std::optional<std::string> case_col,
                          pluss::loader::BadLinePolicy on_bad_lines) {
  return load(source_type, filepath, sep, nrows, normalize_name_list(usecols), std::move(case_col), on_bad_lines);
}

DataFrame DataFrame::load_csv(std::string_view filepath) {
  pluss::loader::ReadCsvOptions options;
  return load_csv(filepath, options);
}

DataFrame DataFrame::load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options) {
  return pluss::loader::load_csv(filepath, options);
}

DataFrame DataFrame::load_csv(std::string_view filepath, const pluss::loader::Config& config) {
  return pluss::loader::load_csv(filepath, config);
}

DataFrame DataFrame::load_csv(std::string_view filepath,
                              std::initializer_list<pluss::loader::ConfigEntry> config_entries) {
  return pluss::loader::load_csv(filepath, config_entries);
}

DataFrame DataFrame::load_csv(std::string_view filepath,
                              std::string_view header,
                              std::string_view case_feature,
                              std::string_view engine,
                              std::optional<std::size_t> ncases) {
  return pluss::loader::load_csv(filepath, header, case_feature, engine, ncases);
}

DataFrame DataFrame::load_csv(std::string_view filepath,
                              std::string_view header,
                              std::size_t ncases) {
  return pluss::loader::load_csv(filepath, header, ncases);
}

DataFrame DataFrame::load_csv(std::string_view filepath,
                              char sep,
                              std::optional<std::size_t> nrows,
                              std::vector<std::string> usecols,
                              std::optional<std::string> case_col,
                              pluss::loader::BadLinePolicy on_bad_lines) {
  pluss::loader::ReadCsvOptions options;
  options.sep = sep;
  options.nrows = nrows;
  options.usecols = std::move(usecols);
  options.case_col = std::move(case_col);
  options.on_bad_lines = on_bad_lines;
  return load_csv(filepath, options);
}

DataFrame DataFrame::load_csv(std::string_view filepath,
                              char sep,
                              std::optional<std::size_t> nrows,
                              std::initializer_list<std::string_view> usecols,
                              std::optional<std::string> case_col,
                              pluss::loader::BadLinePolicy on_bad_lines) {
  return load_csv(filepath, sep, nrows, normalize_name_list(usecols), std::move(case_col), on_bad_lines);
}

DataFrame DataFrame::load_excel(std::string_view filepath) {
  pluss::loader::ReadExcelOptions options;
  return load_excel(filepath, options);
}

DataFrame DataFrame::load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options) {
  return pluss::loader::load_excel(filepath, options);
}

DataFrame DataFrame::load_excel(std::string_view filepath, const pluss::loader::Config& config) {
  return pluss::loader::load_excel(filepath, config);
}

DataFrame DataFrame::load_excel(std::string_view filepath,
                                std::initializer_list<pluss::loader::ConfigEntry> config_entries) {
  return pluss::loader::load_excel(filepath, config_entries);
}

DataFrame DataFrame::load_excel(std::string_view filepath,
                                std::string_view sheet_name,
                                bool header) {
  const auto token = lower_copy(owned_name(sheet_name));
  PLUSS_CHECK(token != "none",
              "DataFrame::load_excel(sheet_name=\"none\") is workbook mode; use DataFrame::load_excel(filepath, nullptr, header)");
  return pluss::loader::load_excel(filepath, sheet_name, header);
}

DataFrame DataFrame::load_excel(std::string_view filepath,
                                std::int64_t sheet_index,
                                bool header) {
  return pluss::loader::load_excel(filepath, sheet_index, header);
}

pluss::loader::LoadResult DataFrame::load_excel(std::string_view filepath, std::nullptr_t all_sheets, bool header) {
  return pluss::loader::load_excel(filepath, all_sheets, header);
}

pluss::loader::LoadResult DataFrame::load_excel(std::string_view filepath,
                                                const std::vector<std::string>& sheet_names,
                                                bool header) {
  return pluss::loader::load_excel(filepath, sheet_names, header);
}

pluss::loader::LoadResult DataFrame::load_excel(std::string_view filepath,
                                                std::initializer_list<std::string_view> sheet_names,
                                                bool header) {
  return pluss::loader::load_excel(filepath, sheet_names, header);
}

pluss::loader::LoadResult DataFrame::load_excel(std::string_view filepath,
                                                const std::vector<std::int64_t>& sheet_indices,
                                                bool header) {
  return pluss::loader::load_excel(filepath, sheet_indices, header);
}

pluss::loader::LoadResult DataFrame::load_excel(std::string_view filepath,
                                                std::initializer_list<std::int64_t> sheet_indices,
                                                bool header) {
  return pluss::loader::load_excel(filepath, sheet_indices, header);
}

DataFrame DataFrame::load_parquet(std::string_view filepath) {
  return load_parquet(filepath, pluss::loader::Config{});
}

DataFrame DataFrame::load_parquet(std::string_view filepath, const pluss::loader::Config& config) {
  return pluss::loader::load_parquet(filepath, config);
}

DataFrame DataFrame::load_parquet(std::string_view filepath,
                                  std::initializer_list<pluss::loader::ConfigEntry> config_entries) {
  return pluss::loader::load_parquet(filepath, config_entries);
}

bool DataFrame::defined() const noexcept { return static_cast<bool>(impl_); }
bool DataFrame::unique() const noexcept { return impl_.unique(); }

const detail::DataFrameImpl& DataFrame::impl_ref_() const {
  PLUSS_CHECK(defined(), "DataFrame: undefined handle");
  return *impl_;
}

detail::DataFrameImpl& DataFrame::mut_impl_() {
  PLUSS_CHECK(defined(), "DataFrame: undefined handle");
  if (!impl_.unique()) {
    impl_ = clone_impl(*impl_);
  }
  return *impl_;
}

std::int64_t DataFrame::nrows() const { return impl_ref_().nrows_; }
std::int64_t DataFrame::ncols() const { return static_cast<std::int64_t>(impl_ref_().columns_.size()); }
std::int64_t DataFrame::nfeatures() const { return ncols(); }

const std::vector<std::string>& DataFrame::column_names() const { return impl_ref_().names_; }

std::vector<Thread> DataFrame::columns() const { return impl_ref_().columns_; }
std::vector<Thread> DataFrame::values() const { return columns(); }
const std::vector<std::string>& DataFrame::feature_names() const { return column_names(); }
std::vector<Thread> DataFrame::features() const { return columns(); }

const Thread& DataFrame::cases() const { return impl_ref_().cases_; }

DataFrame DataFrame::cases(std::int64_t row) const {
  const auto idx = normalize_row_index(row, nrows());
  return slice(idx, idx + 1);
}

DataFrame DataFrame::cases(std::pair<std::int64_t, std::int64_t> row_span) const {
  return slice(row_span.first, row_span.second);
}

std::string DataFrame::cases(std::int64_t row, std::size_t feature_idx) const {
  const auto r = normalize_row_index(row, nrows());
  PLUSS_CHECK(feature_idx < static_cast<std::size_t>(ncols()), "DataFrame::cases(row, feature): feature index out of bounds");
  const auto& src = impl_ref_();
  return thread_scalar_string_at(src.columns_[feature_idx], r);
}

std::string DataFrame::cases(std::int64_t row, std::string_view feature_name) const {
  const auto r = normalize_row_index(row, nrows());
  return thread_scalar_string_at(column(feature_name), r);
}

pluss::Series DataFrame::cases_series() const {
  PLUSS_CHECK(cases().kind() == ThreadKind::Series, "DataFrame::cases_series requires a Series-backed case axis");
  return cases().as_series();
}

DataFrame DataFrame::with_cases(const Thread& case_idx) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(case_idx.defined(), "DataFrame::with_cases: cases are undefined");
  PLUSS_CHECK(case_idx.size() == src.nrows_, "DataFrame::with_cases: case length mismatch");
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, src.columns_, case_idx));
}

DataFrame& DataFrame::with_cases_(const Thread& case_idx) {
  auto& w = mut_impl_();
  PLUSS_CHECK(case_idx.defined(), "DataFrame::with_cases_: cases are undefined");
  PLUSS_CHECK(case_idx.size() == w.nrows_, "DataFrame::with_cases_: case length mismatch");
  w.cases_ = case_idx;
  w.validate_and_reindex_();
  return *this;
}

bool DataFrame::has_column(std::string_view name) const {
  const auto& src = impl_ref_();
  return src.name_to_idx_.find(owned_name(name)) != src.name_to_idx_.end();
}

bool DataFrame::has_feature(std::string_view name) const { return has_column(name); }

Thread DataFrame::column(std::string_view name) const {
  const auto& src = impl_ref_();
  const auto it = src.name_to_idx_.find(owned_name(name));
  PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame: unknown column '" + owned_name(name) + "'");
  return src.columns_[it->second];
}

Thread DataFrame::feature(std::string_view name) const { return column(name); }
Thread DataFrame::features(std::string_view name) const { return feature(name); }

ColumnRef DataFrame::column_ref(std::string_view name) const {
  return ColumnRef(owned_name(name), column(name));
}

ColumnRef DataFrame::feature_ref(std::string_view name) const { return column_ref(name); }

DataFrame DataFrame::features(std::initializer_list<std::string_view> names) const {
  return select(names);
}

DataFrame DataFrame::select(std::initializer_list<std::string_view> names) const {
  const auto picked = normalize_name_list(names);
  const auto& src = impl_ref_();

  std::vector<std::string> out_names;
  std::vector<Thread> out_cols;
  out_names.reserve(picked.size());
  out_cols.reserve(picked.size());

  for (const auto& n : picked) {
    const auto it = src.name_to_idx_.find(n);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::select: unknown column '" + n + "'");
    out_names.push_back(n);
    out_cols.push_back(src.columns_[it->second]);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(std::move(out_names), std::move(out_cols), src.cases_));
}

DataFrame DataFrame::drop(std::initializer_list<std::string_view> names) const {
  const auto drop_names = normalize_name_list(names);
  const auto& src = impl_ref_();
  std::unordered_set<std::string> drop_set;
  drop_set.reserve(drop_names.size());
  for (const auto& n : drop_names) {
    PLUSS_CHECK(src.name_to_idx_.find(n) != src.name_to_idx_.end(), "DataFrame::drop: unknown column '" + n + "'");
    drop_set.insert(n);
  }

  std::vector<std::string> out_names;
  std::vector<Thread> out_cols;
  out_names.reserve(src.names_.size());
  out_cols.reserve(src.columns_.size());

  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    if (drop_set.find(src.names_[i]) == drop_set.end()) {
      out_names.push_back(src.names_[i]);
      out_cols.push_back(src.columns_[i]);
    }
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(std::move(out_names), std::move(out_cols), src.cases_));
}

DataFrame DataFrame::reorder(std::initializer_list<std::string_view> names) const {
  const auto ordered = normalize_name_list(names);
  const auto& src = impl_ref_();

  PLUSS_CHECK(ordered.size() == src.names_.size(), "DataFrame::reorder requires the full column set");
  std::unordered_set<std::string> seen;
  seen.reserve(ordered.size());
  for (const auto& n : ordered) {
    PLUSS_CHECK(seen.insert(n).second, "DataFrame::reorder contains duplicate column name '" + n + "'");
    PLUSS_CHECK(src.name_to_idx_.find(n) != src.name_to_idx_.end(), "DataFrame::reorder unknown column '" + n + "'");
  }
  return select(names);
}

DataFrame DataFrame::with_column(std::string name, const Thread& column_value) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(column_value.defined(), "DataFrame::with_column: input column is undefined");
  if (!src.columns_.empty()) {
    PLUSS_CHECK(column_value.size() == src.nrows_, "DataFrame::with_column: row count mismatch");
  }

  auto out_names = src.names_;
  auto out_cols = src.columns_;
  const auto it = src.name_to_idx_.find(name);
  if (it == src.name_to_idx_.end()) {
    out_names.push_back(std::move(name));
    out_cols.push_back(column_value);
  } else {
    out_cols[it->second] = column_value;
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(std::move(out_names), std::move(out_cols), src.cases_));
}

DataFrame& DataFrame::with_column_(std::string name, const Thread& column_value) {
  auto& w = mut_impl_();
  PLUSS_CHECK(column_value.defined(), "DataFrame::with_column_: input column is undefined");
  if (!w.columns_.empty()) {
    PLUSS_CHECK(column_value.size() == w.nrows_, "DataFrame::with_column_: row count mismatch");
  }

  const auto it = w.name_to_idx_.find(name);
  if (it == w.name_to_idx_.end()) {
    w.names_.push_back(std::move(name));
    w.columns_.push_back(column_value);
  } else {
    w.columns_[it->second] = column_value;
  }
  w.validate_and_reindex_();
  return *this;
}

DataFrame DataFrame::with_feature(std::string name, const Thread& feature_value) const {
  return with_column(std::move(name), feature_value);
}

DataFrame& DataFrame::with_feature_(std::string name, const Thread& feature_value) {
  return with_column_(std::move(name), feature_value);
}

DataFrame DataFrame::rename(std::string_view old_name, std::string new_name) const {
  const auto& src = impl_ref_();
  const auto old_key = owned_name(old_name);
  const auto it = src.name_to_idx_.find(old_key);
  PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::rename: unknown column '" + old_key + "'");
  if (old_key != new_name) {
    PLUSS_CHECK(src.name_to_idx_.find(new_name) == src.name_to_idx_.end(), "DataFrame::rename: target name already exists");
  }

  auto out_names = src.names_;
  out_names[it->second] = std::move(new_name);
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(std::move(out_names), src.columns_, src.cases_));
}

DataFrame& DataFrame::rename_(std::string_view old_name, std::string new_name) {
  auto& w = mut_impl_();
  const auto old_key = owned_name(old_name);
  const auto it = w.name_to_idx_.find(old_key);
  PLUSS_CHECK(it != w.name_to_idx_.end(), "DataFrame::rename_: unknown column '" + old_key + "'");
  if (old_key != new_name) {
    PLUSS_CHECK(w.name_to_idx_.find(new_name) == w.name_to_idx_.end(), "DataFrame::rename_: target name already exists");
  }

  w.names_[it->second] = std::move(new_name);
  w.validate_and_reindex_();
  return *this;
}

DataFrame DataFrame::head(std::int64_t n, bool render) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, nrows());
  auto out = slice(0, clipped);
  if (render) {
    display::print(std::cout, out);
  }
  return out;
}

DataFrame DataFrame::tail(std::int64_t n, bool render) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, nrows());
  auto out = slice(nrows() - clipped, nrows());
  if (render) {
    display::print(std::cout, out);
  }
  return out;
}

DataFrame DataFrame::slice(std::int64_t start, std::int64_t stop) const {
  const auto [s, e] = normalize_slice(start, stop, nrows());
  std::vector<std::int64_t> idx;
  idx.reserve(static_cast<std::size_t>(e - s));
  for (std::int64_t i = s; i < e; ++i) {
    idx.push_back(i);
  }
  return take(idx);
}

DataFrame DataFrame::take(std::span<const std::int64_t> indices) const {
  const auto& src = impl_ref_();
  std::vector<Thread> out_cols;
  out_cols.reserve(src.columns_.size());
  for (const auto& c : src.columns_) {
    out_cols.push_back(c.take(indices));
  }
  auto out_case = src.cases_.take(indices);
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), std::move(out_case)));
}

DataFrame DataFrame::filter(const pluss::Series& bool_mask) const {
  validate_bool_mask(bool_mask, nrows(), "DataFrame::filter");
  const auto& src = impl_ref_();
  std::vector<Thread> out_cols;
  out_cols.reserve(src.columns_.size());
  for (const auto& c : src.columns_) {
    out_cols.push_back(c.filter(bool_mask));
  }
  auto out_case = src.cases_.filter(bool_mask);
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), std::move(out_case)));
}

DataFrame DataFrame::fillna(double value) const {
  const auto& src = impl_ref_();
  std::vector<Thread> out_cols;
  out_cols.reserve(src.columns_.size());
  for (const auto& c : src.columns_) {
    if (c.kind() == ThreadKind::Series && c.has_validity_mask()) {
      out_cols.push_back(c.fillna(value));
    } else {
      out_cols.push_back(c);
    }
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame& DataFrame::fillna_(double value) {
  auto& w = mut_impl_();
  for (auto& c : w.columns_) {
    if (c.kind() == ThreadKind::Series && c.has_validity_mask()) {
      c.fillna_(value);
    }
  }
  return *this;
}

std::int64_t DataFrame::count_null_rows() const {
  const auto& src = impl_ref_();
  if (src.nrows_ == 0) {
    return 0;
  }

  std::vector<bool> has_null(static_cast<std::size_t>(src.nrows_), false);
  for (const auto& c : src.columns_) {
    if (!c.has_validity_mask()) {
      continue;
    }
    const auto valid = c.validity_mask()->to_vec<bool>();
    for (std::int64_t i = 0; i < src.nrows_; ++i) {
      if (!valid[static_cast<std::size_t>(i)]) {
        has_null[static_cast<std::size_t>(i)] = true;
      }
    }
  }

  std::int64_t n = 0;
  for (const bool v : has_null) {
    if (v) {
      ++n;
    }
  }
  return n;
}

DataFrame DataFrame::dropna() const {
  const auto& src = impl_ref_();
  if (src.nrows_ == 0) {
    return *this;
  }

  bool has_any_mask = false;
  std::vector<bool> keep(static_cast<std::size_t>(src.nrows_), true);
  for (const auto& c : src.columns_) {
    if (!c.has_validity_mask()) {
      continue;
    }
    has_any_mask = true;
    const auto valid = c.validity_mask()->to_vec<bool>();
    for (std::int64_t i = 0; i < src.nrows_; ++i) {
      keep[static_cast<std::size_t>(i)] = keep[static_cast<std::size_t>(i)] && valid[static_cast<std::size_t>(i)];
    }
  }

  if (!has_any_mask) {
    return *this;
  }
  return filter(bool_series_from_keep(keep));
}

DataFrame DataFrame::describe(bool render) const {
  const auto& src = impl_ref_();
  std::vector<std::pair<std::string, Thread>> out_cols;
  out_cols.reserve(src.columns_.size());

  for (std::size_t i = 0; i < src.columns_.size(); ++i) {
    const auto& col = src.columns_[i];
    if (col.kind() != ThreadKind::Series) {
      continue;
    }
    const auto& s = col.as_series();
    if (!is_numeric_series_type(s)) {
      continue;
    }

    std::vector<double> values;
    values.reserve(static_cast<std::size_t>(col.size()));
    if (col.has_validity_mask()) {
      const auto valid = col.validity_mask()->to_vec<bool>();
      for (std::int64_t r = 0; r < col.size(); ++r) {
        if (!valid[static_cast<std::size_t>(r)]) {
          continue;
        }
        double v = 0.0;
        if (extract_numeric_value(s, r, v)) {
          values.push_back(v);
        }
      }
    } else {
      for (std::int64_t r = 0; r < col.size(); ++r) {
        double v = 0.0;
        if (extract_numeric_value(s, r, v)) {
          values.push_back(v);
        }
      }
    }

    const double count = static_cast<double>(values.size());
    double mean = std::numeric_limits<double>::quiet_NaN();
    double stdv = std::numeric_limits<double>::quiet_NaN();
    double minv = std::numeric_limits<double>::quiet_NaN();
    double q25 = std::numeric_limits<double>::quiet_NaN();
    double q50 = std::numeric_limits<double>::quiet_NaN();
    double q75 = std::numeric_limits<double>::quiet_NaN();
    double maxv = std::numeric_limits<double>::quiet_NaN();

    if (!values.empty()) {
      std::sort(values.begin(), values.end());
      minv = values.front();
      maxv = values.back();
      const double sum = std::accumulate(values.begin(), values.end(), 0.0);
      mean = sum / count;
      q25 = quantile_sorted(values, 0.25);
      q50 = quantile_sorted(values, 0.50);
      q75 = quantile_sorted(values, 0.75);

      if (values.size() > 1) {
        double sq = 0.0;
        for (const double v : values) {
          const double d = v - mean;
          sq += d * d;
        }
        stdv = std::sqrt(sq / static_cast<double>(values.size() - 1));
      }
    }

    std::vector<double> stats{count, mean, stdv, minv, q25, q50, q75, maxv};
    out_cols.emplace_back(src.names_[i], make_f64_thread(stats));
  }

  PLUSS_CHECK(!out_cols.empty(), "DataFrame::describe: no numeric columns to describe");

  auto out = DataFrame(
      std::move(out_cols),
      make_note_thread({"count", "mean", "std", "min", "25%", "50%", "75%", "max"}));
  if (render) {
    display::print(std::cout, out);
  }
  return out;
}

DataFrame DataFrame::view(std::int64_t start, std::int64_t stop, bool render) const {
  const auto n = nrows();
  const auto s = normalize_view_start(start, n);
  const auto e = normalize_view_stop(stop, n);

  DataFrame out;
  if (e <= s) {
    out = slice(s, s);
  } else if (s == 0) {
    out = head(e, false);
  } else if (e == n) {
    out = tail(n - s, false);
  } else {
    auto after_start = tail(n - s, false);
    out = after_start.head(e - s, false);
  }

  if (render) {
    display::print(std::cout, out);
  }
  return out;
}

std::string DataFrame::to_string(const display::DisplayOptions& options) const {
  return display::render(*this, options);
}

std::string DataFrame::info_string(const display::DisplayOptions& options) const {
  return display::render_info(*this, options);
}

void DataFrame::info(const display::DisplayOptions& options) const {
  display::print_info(std::cout, *this, options);
}

void DataFrame::info(std::ostream& os, const display::DisplayOptions& options) const {
  display::print_info(os, *this, options);
}

void DataFrame::save_csv(std::string_view filepath) const {
  pluss::loader::save_csv(*this, filepath, {});
}

void DataFrame::save_csv(std::string_view filepath,
                         bool add_cases,
                         bool header,
                         char sep,
                         std::string na_rep,
                         std::optional<std::string> case_label,
                         std::optional<std::string> float_format,
                         char quotechar,
                         bool doublequote,
                         std::string line_terminator,
                         char decimal,
                         std::string encoding) const {
  pluss::loader::WriteCsvOptions options;
  options.sep = sep;
  options.header = header;
  options.add_cases = add_cases;
  options.cases = add_cases;
  options.na_rep = std::move(na_rep);
  options.case_label = std::move(case_label);
  options.float_format = std::move(float_format);
  options.quotechar = quotechar;
  options.doublequote = doublequote;
  options.line_terminator = std::move(line_terminator);
  options.decimal = decimal;
  options.encoding = std::move(encoding);
  pluss::loader::save_csv(*this, filepath, options);
}

void DataFrame::save_csv(std::string_view filepath, const pluss::loader::WriteCsvOptions& options) const {
  pluss::loader::save_csv(*this, filepath, options);
}

void DataFrame::save_csv(std::string_view filepath, const pluss::loader::Config& config) const {
  save_as("csv", filepath, config);
}

void DataFrame::save_csv(std::string_view filepath,
                         std::initializer_list<pluss::loader::ConfigEntry> config_entries) const {
  save_as("csv", filepath, config_entries);
}

void DataFrame::save_excel(std::string_view filepath) const {
  pluss::loader::save_excel(*this, filepath, {});
}

void DataFrame::save_excel(std::string_view filepath,
                           std::string sheet_name,
                           bool add_cases,
                           bool header,
                           std::string na_rep,
                           std::optional<std::string> case_label) const {
  pluss::loader::WriteExcelOptions options;
  options.sheet_name = std::move(sheet_name);
  options.add_cases = add_cases;
  options.cases = add_cases;
  options.header = header;
  options.na_rep = std::move(na_rep);
  options.case_label = std::move(case_label);
  pluss::loader::save_excel(*this, filepath, options);
}

void DataFrame::save_excel(std::string_view filepath, const pluss::loader::WriteExcelOptions& options) const {
  pluss::loader::save_excel(*this, filepath, options);
}

void DataFrame::save_excel(std::string_view filepath, const pluss::loader::Config& config) const {
  save_as("excel", filepath, config);
}

void DataFrame::save_excel(std::string_view filepath,
                           std::initializer_list<pluss::loader::ConfigEntry> config_entries) const {
  save_as("excel", filepath, config_entries);
}

void DataFrame::save_as(std::string_view target_type,
                        std::string_view filepath,
                        const pluss::loader::Config& config) const {
  pluss::loader::save_as(*this, target_type, filepath, config);
}

void DataFrame::save_as(std::string_view target_type,
                        std::string_view filepath,
                        std::initializer_list<pluss::loader::ConfigEntry> config_entries) const {
  save_as(target_type, filepath, pluss::loader::config_from(config_entries));
}

std::ostream& operator<<(std::ostream& os, const DataFrame& frame) {
  os << frame.to_string();
  return os;
}

} // namespace pluss::table

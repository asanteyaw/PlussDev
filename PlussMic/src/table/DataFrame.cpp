#include "table/DataFrame.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <optional>
#include <random>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <variant>

#include <fmt/format.h>

#include "CodeM/core/Errors.h"
#include "CodeM/core/Half.h"
#include "pluss/codeh/timeindex_factory.h"
#include "pluss/codeh/timeline_factory.h"
#include "pluss/tils/aux/transmit.h"
#include "loader/CsvLoader.h"
#include "loader/Loader.h"
#include "kindle/catalog.h"
#include "table/Atlas.h"
#include "table/display/Display.h"
#include "table/io/CsvSchemaConvert.h"

namespace pluss::table {

namespace {

pluss::Series pack_series_columns(const std::vector<pluss::Series>& cols,
                                  std::string_view where,
                                  bool force_matrix_2d = false) {
  PLUSS_CHECK(!cols.empty(), std::string(where) + ": no Series payloads available");
  if (cols.size() == 1 && !force_matrix_2d) {
    return cols.front();
  }

  const auto rows = cols.front().numel();
  auto head = cols.front().reshape({rows, 1});
  std::vector<pluss::Series> tail;
  tail.reserve(cols.size() > 0 ? cols.size() - 1 : 0);
  for (std::size_t i = 1; i < cols.size(); ++i) {
    PLUSS_CHECK(cols[i].numel() == rows, std::string(where) + ": Series payload length mismatch");
    tail.push_back(cols[i].reshape({rows, 1}));
  }
  return tail.empty() ? head : head.unite(tail, 1);
}

pluss::Note pack_note_columns(const std::vector<pluss::Note>& cols,
                              std::string_view where,
                              bool force_matrix_2d = false) {
  PLUSS_CHECK(!cols.empty(), std::string(where) + ": no Note payloads available");
  if (cols.size() == 1 && !force_matrix_2d) {
    return cols.front();
  }

  const auto rows = cols.front().numel();
  for (std::size_t i = 1; i < cols.size(); ++i) {
    PLUSS_CHECK(cols[i].numel() == rows, std::string(where) + ": Note payload length mismatch");
  }

  const std::int64_t nrows = rows;
  const std::int64_t ncols = static_cast<std::int64_t>(cols.size());
  const std::array<std::int64_t, 1> col_shape{nrows};
  std::vector<std::vector<std::string_view>> vectors;
  vectors.reserve(cols.size());
  for (const auto& col : cols) {
    vectors.push_back(col.reshape(std::span<const std::int64_t>(col_shape.data(), col_shape.size())).vector());
  }

  std::vector<std::string_view> flat(static_cast<std::size_t>(nrows * ncols));
  for (std::int64_t r = 0; r < nrows; ++r) {
    const auto row_off = static_cast<std::size_t>(r * ncols);
    for (std::int64_t c = 0; c < ncols; ++c) {
      flat[row_off + static_cast<std::size_t>(c)] = vectors[static_cast<std::size_t>(c)][static_cast<std::size_t>(r)];
    }
  }

  const std::array<std::int64_t, 2> shape{nrows, ncols};
  return pluss::coden::ops_internal::note_from_utf8(
      std::span<const std::string_view>(flat.data(), flat.size()),
      std::span<const std::int64_t>(shape.data(), shape.size()));
}

pluss::Timeline pack_timeline_columns(const std::vector<pluss::Timeline>& cols,
                                      std::string_view where,
                                      bool force_matrix_2d = false) {
  PLUSS_CHECK(!cols.empty(), std::string(where) + ": no Timeline payloads available");
  if (cols.size() == 1 && !force_matrix_2d) {
    return cols.front();
  }

  const auto rows = cols.front().numel();
  std::vector<pluss::Series> epoch_cols;
  epoch_cols.reserve(cols.size());
  for (const auto& col : cols) {
    PLUSS_CHECK(col.numel() == rows, std::string(where) + ": Timeline payload length mismatch");
    epoch_cols.push_back(col.epoch_nanos().reshape({rows, 1}));
  }

  auto matrix = epoch_cols.front();
  if (epoch_cols.size() > 1) {
    std::vector<pluss::Series> tail(epoch_cols.begin() + 1, epoch_cols.end());
    matrix = matrix.unite(tail, 1);
  }
  return pluss::Timeline::fromSeries(matrix);
}

} // namespace

pluss::Series DataFrame::MatrixValues::as_series() const {
  const auto& blocks = std::get<0>(*this);
  PLUSS_CHECK(!blocks.empty(), "DataFrame::MatrixValues::as_series: no Series matrix block available");
  PLUSS_CHECK(blocks.size() == 1, "DataFrame::MatrixValues::as_series: expected a single Series matrix block");
  return blocks.front();
}

pluss::Note DataFrame::MatrixValues::as_note() const {
  const auto& blocks = std::get<1>(*this);
  PLUSS_CHECK(!blocks.empty(), "DataFrame::MatrixValues::as_note: no Note matrix block available");
  PLUSS_CHECK(blocks.size() == 1, "DataFrame::MatrixValues::as_note: expected a single Note matrix block");
  return blocks.front();
}

pluss::Timeline DataFrame::MatrixValues::as_timeline() const {
  const auto& blocks = std::get<2>(*this);
  PLUSS_CHECK(!blocks.empty(), "DataFrame::MatrixValues::as_timeline: no Timeline matrix block available");
  PLUSS_CHECK(blocks.size() == 1, "DataFrame::MatrixValues::as_timeline: expected a single Timeline matrix block");
  return blocks.front();
}

pluss::TimeIndex DataFrame::MatrixValues::as_timeindex() const {
  const auto& blocks = std::get<3>(*this);
  PLUSS_CHECK(!blocks.empty(), "DataFrame::MatrixValues::as_timeindex: no TimeIndex matrix block available");
  PLUSS_CHECK(blocks.size() == 1, "DataFrame::MatrixValues::as_timeindex: expected a single TimeIndex matrix block");
  return blocks.front();
}

namespace detail {

struct CaseAxisLevel {
  std::string name{};
  Thread values{};
};

struct CaseAxis {
  std::vector<CaseAxisLevel> levels{};
  // The separator used to join levels[*].values into the composite displayed
  // case label. Preserved across operations that rebuild that composite
  // string from the levels (order_caselbls, drop_caselbls, structured
  // DataFrameLabelAccessor::cast) so a custom separator passed to
  // DataFrame::set_caselbl(...) survives them, instead of silently reverting
  // to the library default.
  std::string separator{"\x1F"};

  bool defined() const noexcept { return !levels.empty(); }
};

class DataFrameImpl final : public pluss::msharp::RefCounted {
public:
  DataFrameImpl(std::vector<std::string> names,
                std::vector<Thread> columns,
                std::optional<Thread> cases = std::nullopt,
                std::string title = {},
                CaseAxis case_axis = {});
  void validate_and_reindex_();

  std::vector<std::string> names_;
  std::vector<Thread> columns_;
  std::unordered_map<std::string, std::size_t> name_to_idx_;
  std::int64_t nrows_{0};
  Thread cases_{};
  CaseAxis case_axis_{};
  std::string title_{};
  mutable bool suppress_next_stream_{false};
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

std::vector<std::string> normalize_name_list(pluss::msharp::RangeView::Of<std::string_view> names) {
  std::vector<std::string> out;
  out.reserve(names.size());
  for (const auto n : names) {
    out.push_back(owned_name(n));
  }
  return out;
}

bool is_none_token(std::string_view s) {
  auto t = lower_copy(owned_name(s));
  std::size_t b = 0;
  std::size_t e = t.size();
  while (b < e && std::isspace(static_cast<unsigned char>(t[b])) != 0) ++b;
  while (e > b && std::isspace(static_cast<unsigned char>(t[e - 1])) != 0) --e;
  t = t.substr(b, e - b);
  return t.empty() || t == "none" || t == "null";
}

Thread infer_padded_thread_from_raw(const pluss::loader::CsvRawColumn& column,
                                    const pluss::loader::ReadCsvOptions& options,
                                    std::int64_t target_len) {
  if (column.note_values.has_value()) {
    std::vector<std::string> values;
    values.reserve(static_cast<std::size_t>(column.note_values->numel()));
    for (std::int64_t i = 0; i < column.note_values->numel(); ++i) {
      values.emplace_back(column.note_values->at(i));
    }
    if (target_len >= 0 && static_cast<std::int64_t>(values.size()) < target_len) {
      values.resize(static_cast<std::size_t>(target_len));
    }
    return detail::infer_thread_from_raw_csv_column(column.name, column.source_index, values, options);
  }
  std::vector<std::string> values = column.values;
  if (target_len >= 0 && static_cast<std::int64_t>(values.size()) < target_len) {
    values.resize(static_cast<std::size_t>(target_len));
  }
  return detail::infer_thread_from_raw_csv_column(column.name, column.source_index, values, options);
}

DataFrame dataframe_from_raw_batch_padded(const pluss::loader::CsvRawBatch& raw,
                                          const pluss::loader::ReadCsvOptions& options) {
  auto raw_col_size = [](const pluss::loader::CsvRawColumn& col) -> std::size_t {
    return col.note_values.has_value() ? static_cast<std::size_t>(col.note_values->numel()) : col.values.size();
  };
  std::int64_t target_len = 0;
  for (const auto& col : raw.features) {
    target_len = std::max<std::int64_t>(target_len, static_cast<std::int64_t>(raw_col_size(col)));
  }
  if (raw.cases.has_value()) {
    target_len = std::max<std::int64_t>(target_len, static_cast<std::int64_t>(raw_col_size(*raw.cases)));
  }

  std::vector<std::pair<std::string, Thread>> cols;
  cols.reserve(raw.features.size());
  for (const auto& col : raw.features) {
    cols.emplace_back(col.name, infer_padded_thread_from_raw(col, options, target_len));
  }

  std::optional<Thread> cases = std::nullopt;
  if (raw.cases.has_value()) {
    cases = infer_padded_thread_from_raw(*raw.cases, options, target_len);
  }

  return DataFrame(std::move(cols), std::move(cases));
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

bool is_number_series_type(const pluss::Series& s) {
  const auto dt = s.dtype();
  return dt.scalar == pluss::codem::ScalarType::Bool || dt.is_integral() || dt.is_floating() || dt.is_complex();
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
  if (detail::has_validity_mask(t)) {
    const auto valid = detail::validity_mask(t)->to_vec<bool>();
    if (!valid[static_cast<std::size_t>(row)]) {
      return "NULL";
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

Thread make_f64_thread(const std::vector<double>& values, const std::vector<bool>& valid) {
  PLUSS_CHECK(values.size() == valid.size(), "make_f64_thread: values/valid length mismatch");
  const auto n = static_cast<std::int64_t>(values.size());
  auto s = pluss::Series::empty({n}, pluss::codem::ScalarType::Float64);
  if (!values.empty()) {
    std::memcpy(s.raw_data(), values.data(), values.size() * sizeof(double));
  }
  return Thread(s, bool_series_from_keep(valid));
}

Thread make_note_thread(std::initializer_list<std::string_view> labels) {
  std::vector<std::string_view> views(labels.begin(), labels.end());
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(views.size())};
  auto note = pluss::coden::ops_internal::note_from_utf8(
      std::span<const std::string_view>(views.data(), views.size()),
      std::span<const std::int64_t>(shape.data(), shape.size()));
  return Thread(note);
}

Thread make_note_thread(const std::vector<std::string>& labels) {
  std::vector<std::string_view> views;
  views.reserve(labels.size());
  for (const auto& v : labels) {
    views.push_back(v);
  }
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(views.size())};
  auto note = pluss::coden::ops_internal::note_from_utf8(
      std::span<const std::string_view>(views.data(), views.size()),
      std::span<const std::int64_t>(shape.data(), shape.size()));
  return Thread(note);
}

Thread make_i64_thread(const std::vector<std::int64_t>& values) {
  const auto n = static_cast<std::int64_t>(values.size());
  auto s = pluss::Series::empty({n}, pluss::codem::ScalarType::Int64);
  if (!values.empty()) {
    std::memcpy(s.raw_data(), values.data(), values.size() * sizeof(std::int64_t));
  }
  return Thread(s);
}

std::string summary_op_name(DataFrame::SummaryOp op) {
  switch (op) {
    case DataFrame::SummaryOp::Count:
      return "count";
    case DataFrame::SummaryOp::Nunique:
      return "distinct_count";
    case DataFrame::SummaryOp::Sum:
      return "sum";
    case DataFrame::SummaryOp::Mean:
      return "mean";
    case DataFrame::SummaryOp::Median:
      return "median";
    case DataFrame::SummaryOp::Min:
      return "min";
    case DataFrame::SummaryOp::Max:
      return "max";
    case DataFrame::SummaryOp::Std:
      return "std";
    case DataFrame::SummaryOp::Var:
      return "var";
    case DataFrame::SummaryOp::Prod:
      return "prod";
    default:
      return "summary";
  }
}

std::string group_op_name(DataFrame::GroupOp op) {
  switch (op) {
    case DataFrame::GroupOp::Count:
      return "count";
    case DataFrame::GroupOp::Nunique:
      return "distinct_count";
    case DataFrame::GroupOp::Sum:
      return "sum";
    case DataFrame::GroupOp::Mean:
      return "mean";
    case DataFrame::GroupOp::Median:
      return "median";
    case DataFrame::GroupOp::Min:
      return "min";
    case DataFrame::GroupOp::Max:
      return "max";
    case DataFrame::GroupOp::Std:
      return "std";
    case DataFrame::GroupOp::Var:
      return "var";
    case DataFrame::GroupOp::Mode:
      return "mode";
    default:
      return "group";
  }
}

std::string window_op_name(ThreadWindowOp op) {
  switch (op) {
    case ThreadWindowOp::RollingSum:
      return "rollsum";
    case ThreadWindowOp::RollingMean:
      return "rollmean";
    case ThreadWindowOp::RollingMin:
      return "rollmin";
    case ThreadWindowOp::RollingMax:
      return "rollmax";
    default:
      return "window";
  }
}

std::int64_t non_null_count(const Thread& t) {
  if (!detail::has_validity_mask(t)) {
    return t.size();
  }
  const auto valid = detail::validity_mask(t)->to_vec<bool>();
  return static_cast<std::int64_t>(std::count(valid.begin(), valid.end(), true));
}

std::int64_t distinct_non_null_count(const Thread& t) {
  std::unordered_set<std::string> seen;
  seen.reserve(static_cast<std::size_t>(t.size()));
  std::optional<std::vector<bool>> valid;
  if (detail::has_validity_mask(t)) {
    valid = detail::validity_mask(t)->to_vec<bool>();
  }
  for (std::int64_t i = 0; i < t.size(); ++i) {
    if (valid.has_value() && !(*valid)[static_cast<std::size_t>(i)]) {
      continue;
    }
    seen.insert(thread_scalar_string_at(t, i));
  }
  return static_cast<std::int64_t>(seen.size());
}

double numeric_reduce_value(const Thread& t, DataFrame::SummaryOp op) {
  PLUSS_CHECK(t.kind() == ThreadKind::Series, "DataFrame::summarize: numeric reductions require Series feature");
  PLUSS_CHECK(is_numeric_series_type(t.as_series()),
              "DataFrame::summarize: numeric reductions require numeric Series feature");

  // Delegate numeric reductions to Thread to keep 1D math semantics in Thread.
  const auto clean = t.dropnull();
  if (clean.size() == 0) {
    if (op == DataFrame::SummaryOp::Sum) {
      return 0.0;
    }
    if (op == DataFrame::SummaryOp::Prod) {
      return 1.0;
    }
    return std::numeric_limits<double>::quiet_NaN();
  }

  auto scalar_series_to_double = [](const pluss::Series& s, std::string_view where) {
    PLUSS_CHECK(s.defined(), std::string(where) + ": scalar reduction produced undefined Series");
    PLUSS_CHECK(s.numel() == 1, std::string(where) + ": scalar reduction must have numel=1");
    double out = 0.0;
    PLUSS_CHECK(extract_numeric_value(s, 0, out), std::string(where) + ": non-numeric scalar reduction result");
    return out;
  };

  switch (op) {
    case DataFrame::SummaryOp::Sum:
      return scalar_series_to_double(clean.sum(), "DataFrame::summarize(sum)");
    case DataFrame::SummaryOp::Mean:
      return scalar_series_to_double(clean.mean(), "DataFrame::summarize(mean)");
    case DataFrame::SummaryOp::Median:
      return scalar_series_to_double(clean.median(), "DataFrame::summarize(median)");
    case DataFrame::SummaryOp::Min:
      return scalar_series_to_double(clean.min().as_series(), "DataFrame::summarize(min)");
    case DataFrame::SummaryOp::Max:
      return scalar_series_to_double(clean.max().as_series(), "DataFrame::summarize(max)");
    case DataFrame::SummaryOp::Std:
      return scalar_series_to_double(clean.std(true), "DataFrame::summarize(std)");
    case DataFrame::SummaryOp::Var:
      return scalar_series_to_double(clean.var(true), "DataFrame::summarize(var)");
    case DataFrame::SummaryOp::Prod:
      return scalar_series_to_double(clean.as_series().prod(), "DataFrame::summarize(prod)");
    default:
      PLUSS_FAIL("DataFrame::summarize: unsupported numeric SummaryOp");
  }
}

enum class BinaryMathOp : std::uint8_t {
  Add = 0,
  Sub = 1,
  Mul = 2,
  Div = 3,
  TrueDiv = 4,
  FloorDiv = 5,
  Mod = 6,
  Pow = 7,
};

enum class BinaryCmpOp : std::uint8_t {
  Eq = 0,
  Ne = 1,
  Lt = 2,
  Le = 3,
  Gt = 4,
  Ge = 5,
};

const char* math_op_name(BinaryMathOp op) {
  switch (op) {
    case BinaryMathOp::Add:
      return "add";
    case BinaryMathOp::Sub:
      return "sub";
    case BinaryMathOp::Mul:
      return "mul";
    case BinaryMathOp::Div:
      return "div";
    case BinaryMathOp::TrueDiv:
      return "truediv";
    case BinaryMathOp::FloorDiv:
      return "floordiv";
    case BinaryMathOp::Mod:
      return "mod";
    case BinaryMathOp::Pow:
      return "pow";
    default:
      return "binary_math";
  }
}

const char* cmp_op_name(BinaryCmpOp op) {
  switch (op) {
    case BinaryCmpOp::Eq:
      return "eq";
    case BinaryCmpOp::Ne:
      return "ne";
    case BinaryCmpOp::Lt:
      return "lt";
    case BinaryCmpOp::Le:
      return "le";
    case BinaryCmpOp::Gt:
      return "gt";
    case BinaryCmpOp::Ge:
      return "ge";
    default:
      return "compare";
  }
}

enum class QueryTokenKind : std::uint8_t {
  Identifier = 0,
  String = 1,
  Number = 2,
  Boolean = 3,
  LParen = 4,
  RParen = 5,
  Comma = 6,
  Eq = 7,
  Ne = 8,
  Lt = 9,
  Le = 10,
  Gt = 11,
  Ge = 12,
  And = 13,
  Or = 14,
  Not = 15,
  IsNull = 16,
  NotNull = 17,
  Contains = 18,
  In = 19,
  Between = 20,
  End = 21,
};

struct QueryToken {
  QueryTokenKind kind{QueryTokenKind::End};
  std::string text;
};

enum class QueryCompareOp : std::uint8_t {
  Eq = 0,
  Ne = 1,
  Lt = 2,
  Le = 3,
  Gt = 4,
  Ge = 5,
};

struct QueryLiteral {
  enum class Kind : std::uint8_t {
    Number = 0,
    String = 1,
    Bool = 2,
  };

  Kind kind{Kind::String};
  double number{0.0};
  std::string text{};
  bool boolean{false};
};

using QueryEvalFn = std::function<bool(std::int64_t)>;

struct QueryRhs {
  bool is_feature{false};
  Thread feature{};
  QueryLiteral literal{};
};

bool is_ident_start(char ch) {
  return std::isalpha(static_cast<unsigned char>(ch)) || ch == '_';
}

bool is_ident_continue(char ch) {
  return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '.';
}

std::string parse_quoted_literal(std::string_view expr, std::size_t& i, char quote) {
  std::string out;
  ++i; // consume opening quote
  while (i < expr.size()) {
    const char ch = expr[i++];
    if (ch == '\\') {
      PLUSS_CHECK(i < expr.size(), "DataFrame::query: dangling escape in string literal");
      out.push_back(expr[i++]);
      continue;
    }
    if (ch == quote) {
      if (i < expr.size() && expr[i] == quote) {
        out.push_back(quote);
        ++i;
        continue;
      }
      return out;
    }
    out.push_back(ch);
  }
  PLUSS_FAIL("DataFrame::query: unterminated string literal");
}

std::string parse_backtick_identifier(std::string_view expr, std::size_t& i) {
  std::string out;
  ++i; // consume opening backtick
  while (i < expr.size()) {
    const char ch = expr[i++];
    if (ch == '`') {
      return out;
    }
    out.push_back(ch);
  }
  PLUSS_FAIL("DataFrame::query: unterminated backtick identifier");
}

bool is_number_start(std::string_view expr, std::size_t i) {
  if (i >= expr.size()) {
    return false;
  }
  const char ch = expr[i];
  if (std::isdigit(static_cast<unsigned char>(ch)) || ch == '.') {
    return true;
  }
  if ((ch == '-' || ch == '+') && i + 1 < expr.size()) {
    const char next = expr[i + 1];
    return std::isdigit(static_cast<unsigned char>(next)) || next == '.';
  }
  return false;
}

std::size_t consume_number(std::string_view expr, std::size_t i) {
  std::size_t p = i;
  if (p < expr.size() && (expr[p] == '+' || expr[p] == '-')) {
    ++p;
  }
  bool has_digit = false;
  while (p < expr.size() && std::isdigit(static_cast<unsigned char>(expr[p]))) {
    has_digit = true;
    ++p;
  }
  if (p < expr.size() && expr[p] == '.') {
    ++p;
    while (p < expr.size() && std::isdigit(static_cast<unsigned char>(expr[p]))) {
      has_digit = true;
      ++p;
    }
  }
  PLUSS_CHECK(has_digit, "DataFrame::query: invalid numeric literal");
  if (p < expr.size() && (expr[p] == 'e' || expr[p] == 'E')) {
    ++p;
    if (p < expr.size() && (expr[p] == '+' || expr[p] == '-')) {
      ++p;
    }
    bool exp_digit = false;
    while (p < expr.size() && std::isdigit(static_cast<unsigned char>(expr[p]))) {
      exp_digit = true;
      ++p;
    }
    PLUSS_CHECK(exp_digit, "DataFrame::query: invalid numeric exponent");
  }
  return p;
}

std::vector<QueryToken> tokenize_query(std::string_view expression) {
  std::vector<QueryToken> out;
  out.reserve(32);

  for (std::size_t i = 0; i < expression.size();) {
    const char ch = expression[i];
    if (std::isspace(static_cast<unsigned char>(ch))) {
      ++i;
      continue;
    }

    if (ch == '(') {
      out.push_back({QueryTokenKind::LParen, "("});
      ++i;
      continue;
    }
    if (ch == ')') {
      out.push_back({QueryTokenKind::RParen, ")"});
      ++i;
      continue;
    }
    if (ch == ',') {
      out.push_back({QueryTokenKind::Comma, ","});
      ++i;
      continue;
    }
    if (ch == '=' && i + 1 < expression.size() && expression[i + 1] == '=') {
      out.push_back({QueryTokenKind::Eq, "=="});
      i += 2;
      continue;
    }
    if (ch == '=') {
      out.push_back({QueryTokenKind::Eq, "="});
      ++i;
      continue;
    }
    if (ch == '!' && i + 1 < expression.size() && expression[i + 1] == '=') {
      out.push_back({QueryTokenKind::Ne, "!="});
      i += 2;
      continue;
    }
    if (ch == '!') {
      out.push_back({QueryTokenKind::Not, "!"});
      ++i;
      continue;
    }
    if (ch == '<' && i + 1 < expression.size() && expression[i + 1] == '=') {
      out.push_back({QueryTokenKind::Le, "<="});
      i += 2;
      continue;
    }
    if (ch == '>') {
      if (i + 1 < expression.size() && expression[i + 1] == '=') {
        out.push_back({QueryTokenKind::Ge, ">="});
        i += 2;
      } else {
        out.push_back({QueryTokenKind::Gt, ">"});
        ++i;
      }
      continue;
    }
    if (ch == '<') {
      out.push_back({QueryTokenKind::Lt, "<"});
      ++i;
      continue;
    }
    if (ch == '&' && i + 1 < expression.size() && expression[i + 1] == '&') {
      out.push_back({QueryTokenKind::And, "&&"});
      i += 2;
      continue;
    }
    if (ch == '|' && i + 1 < expression.size() && expression[i + 1] == '|') {
      out.push_back({QueryTokenKind::Or, "||"});
      i += 2;
      continue;
    }
    if (ch == '\'' || ch == '"') {
      const auto parsed = parse_quoted_literal(expression, i, ch);
      out.push_back({QueryTokenKind::String, parsed});
      continue;
    }
    if (ch == '`') {
      const auto ident = parse_backtick_identifier(expression, i);
      PLUSS_CHECK(!ident.empty(), "DataFrame::query: empty backtick identifier");
      out.push_back({QueryTokenKind::Identifier, ident});
      continue;
    }
    if (is_number_start(expression, i)) {
      const auto end = consume_number(expression, i);
      out.push_back({QueryTokenKind::Number, std::string(expression.substr(i, end - i))});
      i = end;
      continue;
    }
    if (is_ident_start(ch)) {
      std::size_t end = i + 1;
      while (end < expression.size() && is_ident_continue(expression[end])) {
        ++end;
      }
      const auto ident = std::string(expression.substr(i, end - i));
      const auto low = lower_copy(ident);
      if (low == "and") {
        out.push_back({QueryTokenKind::And, ident});
      } else if (low == "or") {
        out.push_back({QueryTokenKind::Or, ident});
      } else if (low == "not") {
        out.push_back({QueryTokenKind::Not, ident});
      } else if (low == "true" || low == "false") {
        out.push_back({QueryTokenKind::Boolean, low});
      } else if (low == "isnull") {
        out.push_back({QueryTokenKind::IsNull, ident});
      } else if (low == "notnull") {
        out.push_back({QueryTokenKind::NotNull, ident});
      } else if (low == "contains") {
        out.push_back({QueryTokenKind::Contains, ident});
      } else if (low == "in") {
        out.push_back({QueryTokenKind::In, ident});
      } else if (low == "between") {
        out.push_back({QueryTokenKind::Between, ident});
      } else {
        out.push_back({QueryTokenKind::Identifier, ident});
      }
      i = end;
      continue;
    }

    PLUSS_FAIL(fmt::format("DataFrame::query: unsupported character '{}'", ch));
  }

  out.push_back({QueryTokenKind::End, ""});
  return out;
}

bool parse_double_exact(std::string_view text, double& out) {
  std::string owned(text);
  char* end = nullptr;
  out = std::strtod(owned.c_str(), &end);
  return end != nullptr && *end == '\0' && std::isfinite(out);
}

bool parse_bool_like(std::string_view text, bool& out) {
  const auto low = lower_copy(owned_name(text));
  if (low == "true" || low == "1" || low == "yes") {
    out = true;
    return true;
  }
  if (low == "false" || low == "0" || low == "no") {
    out = false;
    return true;
  }
  return false;
}

bool row_valid_for_thread(const Thread& t, std::int64_t row) {
  if (!detail::has_validity_mask(t)) {
    return true;
  }
  const auto valid = detail::validity_mask(t)->to_vec<bool>();
  return valid[static_cast<std::size_t>(row)];
}

bool compare_numbers(double lhs, double rhs, QueryCompareOp op) {
  switch (op) {
    case QueryCompareOp::Eq:
      return lhs == rhs;
    case QueryCompareOp::Ne:
      return lhs != rhs;
    case QueryCompareOp::Lt:
      return lhs < rhs;
    case QueryCompareOp::Le:
      return lhs <= rhs;
    case QueryCompareOp::Gt:
      return lhs > rhs;
    case QueryCompareOp::Ge:
      return lhs >= rhs;
    default:
      return false;
  }
}

bool compare_strings(const std::string& lhs, const std::string& rhs, QueryCompareOp op) {
  switch (op) {
    case QueryCompareOp::Eq:
      return lhs == rhs;
    case QueryCompareOp::Ne:
      return lhs != rhs;
    case QueryCompareOp::Lt:
      return lhs < rhs;
    case QueryCompareOp::Le:
      return lhs <= rhs;
    case QueryCompareOp::Gt:
      return lhs > rhs;
    case QueryCompareOp::Ge:
      return lhs >= rhs;
    default:
      return false;
  }
}

bool evaluate_query_compare(
    const Thread& t,
    std::int64_t row,
    QueryCompareOp op,
    const QueryLiteral& rhs) {
  if (!row_valid_for_thread(t, row)) {
    return false;
  }

  if (t.kind() == ThreadKind::Series) {
    if (rhs.kind == QueryLiteral::Kind::String) {
      return compare_strings(thread_scalar_string_at(t, row), rhs.text, op);
    }

    if (rhs.kind == QueryLiteral::Kind::Bool) {
      if (t.as_series().scalar_type() == pluss::codem::ScalarType::Bool) {
        const auto lhs_bool = t.as_series().to_vec<bool>(row);
        return compare_numbers(lhs_bool ? 1.0 : 0.0, rhs.boolean ? 1.0 : 0.0, op);
      }
      double lhs_num = 0.0;
      if (!extract_numeric_value(t.as_series(), row, lhs_num)) {
        return false;
      }
      return compare_numbers(lhs_num, rhs.boolean ? 1.0 : 0.0, op);
    }

    double lhs_num = 0.0;
    if (!extract_numeric_value(t.as_series(), row, lhs_num)) {
      return false;
    }
    return compare_numbers(lhs_num, rhs.number, op);
  }

  const auto lhs = thread_scalar_string_at(t, row);
  if (rhs.kind == QueryLiteral::Kind::String) {
    return compare_strings(lhs, rhs.text, op);
  }
  if (rhs.kind == QueryLiteral::Kind::Bool) {
    bool lhs_bool = false;
    if (!parse_bool_like(lhs, lhs_bool)) {
      return false;
    }
    return compare_numbers(lhs_bool ? 1.0 : 0.0, rhs.boolean ? 1.0 : 0.0, op);
  }

  double lhs_num = 0.0;
  if (!parse_double_exact(lhs, lhs_num)) {
    return false;
  }
  return compare_numbers(lhs_num, rhs.number, op);
}

bool evaluate_query_compare(
    const Thread& lhs,
    const Thread& rhs,
    std::int64_t row,
    QueryCompareOp op) {
  if (!row_valid_for_thread(lhs, row) || !row_valid_for_thread(rhs, row)) {
    return false;
  }

  const bool lhs_numeric = lhs.kind() == ThreadKind::Series && is_numeric_series_type(lhs.as_series());
  const bool rhs_numeric = rhs.kind() == ThreadKind::Series && is_numeric_series_type(rhs.as_series());
  if (lhs_numeric && rhs_numeric) {
    double lv = 0.0;
    double rv = 0.0;
    if (!extract_numeric_value(lhs.as_series(), row, lv) || !extract_numeric_value(rhs.as_series(), row, rv)) {
      return false;
    }
    return compare_numbers(lv, rv, op);
  }
  return compare_strings(thread_scalar_string_at(lhs, row), thread_scalar_string_at(rhs, row), op);
}

bool evaluate_in_list(const Thread& t, std::int64_t row, const std::vector<QueryLiteral>& items) {
  for (const auto& lit : items) {
    if (evaluate_query_compare(t, row, QueryCompareOp::Eq, lit)) {
      return true;
    }
  }
  return false;
}

Thread apply_math_thread_scalar(const Thread& col, double other, BinaryMathOp op, bool reverse) {
  if (col.kind() != ThreadKind::Series || !is_numeric_series_type(col.as_series())) {
    return col;
  }
  switch (op) {
    case BinaryMathOp::Add:
      return reverse ? col.radd(other) : col.add(other);
    case BinaryMathOp::Sub:
      return reverse ? col.rsub(other) : col.sub(other);
    case BinaryMathOp::Mul:
      return reverse ? col.rmul(other) : col.mul(other);
    case BinaryMathOp::Div:
      return reverse ? col.rdiv(other) : col.div(other);
    case BinaryMathOp::TrueDiv:
      return reverse ? col.rtruediv(other) : col.truediv(other);
    case BinaryMathOp::FloorDiv:
      return reverse ? col.rfloordiv(other) : col.floordiv(other);
    case BinaryMathOp::Mod:
      return reverse ? col.rmod(other) : col.mod(other);
    case BinaryMathOp::Pow:
      return reverse ? col.rpow(other) : col.pow(other);
  }
  PLUSS_FAIL("DataFrame::math: unsupported binary math op");
}

Thread apply_math_thread_pair(const Thread& lhs, const Thread& rhs, BinaryMathOp op) {
  if (lhs.kind() != ThreadKind::Series || !is_numeric_series_type(lhs.as_series())) {
    return lhs;
  }
  switch (op) {
    case BinaryMathOp::Add:
      return lhs.add(rhs);
    case BinaryMathOp::Sub:
      return lhs.sub(rhs);
    case BinaryMathOp::Mul:
      return lhs.mul(rhs);
    case BinaryMathOp::Div:
      return lhs.div(rhs);
    case BinaryMathOp::TrueDiv:
      return lhs.truediv(rhs);
    case BinaryMathOp::FloorDiv:
      return lhs.floordiv(rhs);
    case BinaryMathOp::Mod:
      return lhs.mod(rhs);
    case BinaryMathOp::Pow:
      return lhs.pow(rhs);
  }
  PLUSS_FAIL("DataFrame::math: unsupported binary math op");
}

Thread apply_cmp_thread_scalar(const Thread& col, double other, BinaryCmpOp op) {
  switch (op) {
    case BinaryCmpOp::Eq:
      return col.eq(other);
    case BinaryCmpOp::Ne:
      return col.ne(other);
    case BinaryCmpOp::Lt:
      return col.lt(other);
    case BinaryCmpOp::Le:
      return col.le(other);
    case BinaryCmpOp::Gt:
      return col.gt(other);
    case BinaryCmpOp::Ge:
      return col.ge(other);
  }
  PLUSS_FAIL("DataFrame::compare: unsupported binary compare op");
}

Thread apply_cmp_thread_pair(const Thread& lhs, const Thread& rhs, BinaryCmpOp op) {
  switch (op) {
    case BinaryCmpOp::Eq:
      return lhs.eq(rhs);
    case BinaryCmpOp::Ne:
      return lhs.ne(rhs);
    case BinaryCmpOp::Lt:
      return lhs.lt(rhs);
    case BinaryCmpOp::Le:
      return lhs.le(rhs);
    case BinaryCmpOp::Gt:
      return lhs.gt(rhs);
    case BinaryCmpOp::Ge:
      return lhs.ge(rhs);
  }
  PLUSS_FAIL("DataFrame::compare: unsupported binary compare op");
}

pluss::Series dataframe_column_from_matrix_(const pluss::Series& matrix, std::int64_t column) {
  auto index = pluss::Series::arange(
      column,
      column + 1,
      1,
      pluss::codem::ScalarType::Int64,
      matrix.platform());
  return matrix.index_select(1, index).reshape({matrix.sizes()[0]});
}

pluss::Series dataframe_scalar_like_matrix_(const pluss::Series& reference, double value) {
  return pluss::Series::full(
      reference.sizes(),
      value,
      reference.scalar_type(),
      reference.platform());
}

pluss::Series dataframe_apply_math_matrix_pair_(const pluss::Series& lhs,
                                                const pluss::Series& rhs,
                                                BinaryMathOp op) {
  auto floor_divide = [](const pluss::Series& a, const pluss::Series& b) {
    return a.to(pluss::codem::ScalarType::Float64)
        .true_divide(b.to(pluss::codem::ScalarType::Float64))
        .floor();
  };

  switch (op) {
    case BinaryMathOp::Add:
      return lhs.add(rhs);
    case BinaryMathOp::Sub:
      return lhs.sub(rhs);
    case BinaryMathOp::Mul:
      return lhs.mul(rhs);
    case BinaryMathOp::Div:
      return lhs.div(rhs);
    case BinaryMathOp::TrueDiv:
      return lhs.true_divide(rhs);
    case BinaryMathOp::FloorDiv:
      return floor_divide(lhs, rhs);
    case BinaryMathOp::Mod:
      return lhs.remainder(rhs);
    case BinaryMathOp::Pow:
      break;
  }

  PLUSS_FAIL("DataFrame: batched pow is not supported by the Series backend");
}

pluss::Series dataframe_apply_math_matrix_scalar_(const pluss::Series& matrix,
                                                  double other,
                                                  BinaryMathOp op,
                                                  bool reverse) {
  if (!reverse) {
    switch (op) {
      case BinaryMathOp::Add:
        return matrix.add(other);
      case BinaryMathOp::Sub:
        return matrix.sub(other);
      case BinaryMathOp::Mul:
        return matrix.mul(other);
      case BinaryMathOp::Div:
        return matrix.div(other);
      case BinaryMathOp::TrueDiv:
        return matrix.true_divide(other);
      case BinaryMathOp::FloorDiv:
        return dataframe_apply_math_matrix_pair_(
            matrix, dataframe_scalar_like_matrix_(matrix, other), BinaryMathOp::FloorDiv);
      case BinaryMathOp::Mod:
        return matrix.remainder(other);
      case BinaryMathOp::Pow:
        break;
    }
    PLUSS_FAIL("DataFrame: batched pow is not supported by the Series backend");
  }

  return dataframe_apply_math_matrix_pair_(
      dataframe_scalar_like_matrix_(matrix, other), matrix, op);
}

pluss::Series dataframe_apply_cmp_matrix_scalar_(const pluss::Series& matrix,
                                                 double other,
                                                 BinaryCmpOp op) {
  switch (op) {
    case BinaryCmpOp::Eq:
      return matrix.eq(other);
    case BinaryCmpOp::Ne:
      return matrix.ne(other);
    case BinaryCmpOp::Lt:
      return matrix.lt(other);
    case BinaryCmpOp::Le:
      return matrix.le(other);
    case BinaryCmpOp::Gt:
      return matrix.gt(other);
    case BinaryCmpOp::Ge:
      return matrix.ge(other);
  }
  PLUSS_FAIL("DataFrame: unsupported batched comparison op");
}

pluss::Series dataframe_apply_cmp_matrix_pair_(const pluss::Series& lhs,
                                               const pluss::Series& rhs,
                                               BinaryCmpOp op) {
  switch (op) {
    case BinaryCmpOp::Eq:
      return lhs.eq(rhs);
    case BinaryCmpOp::Ne:
      return lhs.ne(rhs);
    case BinaryCmpOp::Lt:
      return lhs.lt(rhs);
    case BinaryCmpOp::Le:
      return lhs.le(rhs);
    case BinaryCmpOp::Gt:
      return lhs.gt(rhs);
    case BinaryCmpOp::Ge:
      return lhs.ge(rhs);
  }
  PLUSS_FAIL("DataFrame: unsupported batched comparison op");
}

struct NumericBatchKey_ {
  pluss::codem::ScalarType dtype;
  pluss::codem::Platform platform;

  bool operator<(const NumericBatchKey_& other) const noexcept {
    return std::tie(dtype, platform) < std::tie(other.dtype, other.platform);
  }
};

std::optional<NumericBatchKey_> numeric_batch_key_(const Thread& value) {
  if (value.kind() != ThreadKind::Series || !is_numeric_series_type(value.as_series())) {
    return std::nullopt;
  }
  return NumericBatchKey_{value.as_series().scalar_type(), value.as_series().platform()};
}

struct NumericPairBatchKey_ {
  pluss::codem::ScalarType lhs_dtype;
  pluss::codem::ScalarType rhs_dtype;
  pluss::codem::Platform platform;

  bool operator<(const NumericPairBatchKey_& other) const noexcept {
    return std::tie(lhs_dtype, rhs_dtype, platform)
           < std::tie(other.lhs_dtype, other.rhs_dtype, other.platform);
  }
};

std::optional<NumericPairBatchKey_> numeric_pair_batch_key_(const Thread& lhs,
                                                            const Thread& rhs) {
  const auto lhs_key = numeric_batch_key_(lhs);
  const auto rhs_key = numeric_batch_key_(rhs);
  if (!lhs_key.has_value() || !rhs_key.has_value()) {
    return std::nullopt;
  }
  if (lhs_key->platform != rhs_key->platform) {
    return std::nullopt;
  }
  return NumericPairBatchKey_{
      lhs_key->dtype,
      rhs_key->dtype,
      lhs_key->platform,
  };
}

pluss::Series thread_valid_on_platform_(const Thread& value,
                                        pluss::codem::Platform platform) {
  auto valid = value.notnull().as_series();
  if (valid.platform() != platform) {
    valid = valid.to(platform);
  }
  return valid;
}

std::vector<Thread> dataframe_columns_(const DataFrame& frame) {
  std::vector<Thread> columns;
  const auto& names = frame.features();
  columns.reserve(names.size());
  for (const auto& name : names) {
    columns.push_back(frame.features(name));
  }
  return columns;
}

Thread null_thread_like_(const Thread& value, std::int64_t size, std::string_view where);

DataFrame dataframe_math_scalar(const DataFrame& frame, double other, BinaryMathOp op, bool reverse) {
  const auto cols = dataframe_columns_(frame);
  const auto& names = frame.features();
  std::vector<Thread> out(cols.size());

  std::map<NumericBatchKey_, std::vector<std::size_t>> batches;
  for (std::size_t i = 0; i < cols.size(); ++i) {
    const auto key = numeric_batch_key_(cols[i]);
    if (key.has_value() && op != BinaryMathOp::Pow) {
      batches[*key].push_back(i);
    } else {
      out[i] = apply_math_thread_scalar(cols[i], other, op, reverse);
    }
  }

  for (const auto& [key, indices] : batches) {
    std::vector<pluss::Series> values;
    std::vector<pluss::Series> valids;
    values.reserve(indices.size());
    valids.reserve(indices.size());
    for (const auto i : indices) {
      values.push_back(cols[i].as_series());
      auto valid = cols[i].notnull().as_series();
      if (valid.platform() != key.platform) {
        valid = valid.to(key.platform);
      }
      valids.push_back(std::move(valid));
    }
    const auto value_matrix = pack_series_columns(values, "DataFrame::numeric_batch", true);
    const auto valid_matrix = pack_series_columns(valids, "DataFrame::numeric_batch", true);
    const auto result_matrix =
        dataframe_apply_math_matrix_scalar_(value_matrix, other, op, reverse);
    for (std::size_t pos = 0; pos < indices.size(); ++pos) {
      out[indices[pos]] = Thread(
          dataframe_column_from_matrix_(result_matrix, static_cast<std::int64_t>(pos)),
          dataframe_column_from_matrix_(valid_matrix, static_cast<std::int64_t>(pos)));
    }
  }

  std::vector<std::pair<std::string, Thread>> columns;
  columns.reserve(cols.size());
  for (std::size_t i = 0; i < cols.size(); ++i) {
    columns.emplace_back(names[i], std::move(out[i]));
  }
  return DataFrame(std::move(columns), frame.label().values());
}

DataFrame dataframe_math_frame(const DataFrame& lhs,
                               const DataFrame& rhs,
                               const AlignOptions& options,
                               BinaryMathOp op);

DataFrame dataframe_math_frame(const DataFrame& lhs, const DataFrame& rhs, BinaryMathOp op) {
  return dataframe_math_frame(lhs, rhs, AlignOptions{}, op);
}

AlignOptions row_mask_default_options_() {
  AlignOptions options;
  options.align = AlignBy::Caselbl;
  return options;
}

Thread fill_numeric_missing_for_alignment_(const Thread& value, const AlignOptions& options) {
  if (!options.fill.has_value() || !value.defined() || value.kind() != ThreadKind::Series) {
    return value;
  }
  return value.fillnull(*options.fill);
}

DataFrame fill_numeric_missing_for_alignment_(const DataFrame& frame, const AlignOptions& options) {
  if (!options.fill.has_value()) {
    return frame;
  }
  const auto cols = dataframe_columns_(frame);
  const auto& names = frame.features();
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(cols.size());
  for (std::size_t i = 0; i < cols.size(); ++i) {
    out.emplace_back(names[i], fill_numeric_missing_for_alignment_(cols[i], options));
  }
  return DataFrame(std::move(out), frame.label().values()).title(frame.title());
}

DataFrame positional_rhs_like_lhs_(const DataFrame& lhs,
                                   const DataFrame& rhs,
                                   std::string_view where) {
  if (lhs.ncases() != rhs.ncases() || lhs.nfeatures() != rhs.nfeatures()) {
    throw ::pluss::Error(
        fmt::format("{}: positional frame alignment requires identical shape (lhs=({}, {}), rhs=({}, {}))",
                    where,
                    lhs.ncases(),
                    lhs.nfeatures(),
                    rhs.ncases(),
                    rhs.nfeatures()));
  }
  const auto rhs_cols = dataframe_columns_(rhs);
  const auto& lhs_names = lhs.features();
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(rhs_cols.size());
  for (std::size_t i = 0; i < rhs_cols.size(); ++i) {
    out.emplace_back(lhs_names[i], rhs_cols[i]);
  }
  return DataFrame(std::move(out), lhs.label().values()).title(rhs.title());
}

FrameMatch prepare_frame_pair_for_alignment_(const DataFrame& lhs,
                                             const DataFrame& rhs,
                                             const AlignOptions& options,
                                             std::string_view where) {
  switch (options.align) {
    case AlignBy::Pos:
      return FrameMatch{lhs, positional_rhs_like_lhs_(lhs, rhs, where)};
    case AlignBy::Caselbl: {
      if (lhs.nfeatures() != rhs.nfeatures()) {
        throw ::pluss::Error(fmt::format(
            "{}: AlignBy::Caselbl requires identical feature count for positional feature pairing "
            "(lhs={}, rhs={})",
            where,
            lhs.nfeatures(),
            rhs.nfeatures()));
      }
      const auto rhs_cols = dataframe_columns_(rhs);
      const auto& lhs_names = lhs.features();
      std::vector<std::pair<std::string, Thread>> renamed;
      renamed.reserve(rhs_cols.size());
      for (std::size_t i = 0; i < rhs_cols.size(); ++i) {
        renamed.emplace_back(lhs_names[i], rhs_cols[i]);
      }
      const auto renamed_rhs = DataFrame(std::move(renamed), rhs.label().values()).title(rhs.title());
      return lhs.match(renamed_rhs, FrameMatchOptions{
          .cases = FrameMatchRule::Unite,
          .features = FrameMatchRule::KeepLeft,
      });
    }
    case AlignBy::Feature: {
      if (lhs.ncases() != rhs.ncases()) {
        throw ::pluss::Error(fmt::format(
            "{}: AlignBy::Feature requires identical case count for positional case pairing "
            "(lhs={}, rhs={})",
            where,
            lhs.ncases(),
            rhs.ncases()));
      }
      const auto rhs_cols = dataframe_columns_(rhs);
      const auto& rhs_names = rhs.features();
      std::vector<std::pair<std::string, Thread>> relabeled;
      relabeled.reserve(rhs_cols.size());
      for (std::size_t i = 0; i < rhs_cols.size(); ++i) {
        relabeled.emplace_back(rhs_names[i], rhs_cols[i]);
      }
      const auto relabeled_rhs = DataFrame(std::move(relabeled), lhs.label().values()).title(rhs.title());
      return lhs.match(relabeled_rhs, FrameMatchOptions{
          .cases = FrameMatchRule::KeepLeft,
          .features = FrameMatchRule::Unite,
      });
    }
    case AlignBy::Both:
      return lhs.match(rhs, FrameMatchOptions{
          .cases = FrameMatchRule::Unite,
          .features = FrameMatchRule::Unite,
      });
  }
  PLUSS_FAIL(std::string(where) + ": unsupported alignment mode");
}

DataFrame dataframe_math_frame(const DataFrame& lhs,
                               const DataFrame& rhs,
                               const AlignOptions& options,
                               BinaryMathOp op) {
  auto matched = prepare_frame_pair_for_alignment_(lhs, rhs, options, "DataFrame::math(DataFrame)");
  matched.left = fill_numeric_missing_for_alignment_(matched.left, options);
  matched.right = fill_numeric_missing_for_alignment_(matched.right, options);
  const auto lhs_cols = dataframe_columns_(matched.left);
  const auto rhs_cols = dataframe_columns_(matched.right);
  const auto& names = matched.left.features();

  std::vector<Thread> out(lhs_cols.size());
  std::map<NumericPairBatchKey_, std::vector<std::size_t>> batches;
  for (std::size_t i = 0; i < lhs_cols.size(); ++i) {
    const auto key = numeric_pair_batch_key_(lhs_cols[i], rhs_cols[i]);
    if (key.has_value() && op != BinaryMathOp::Pow) {
      batches[*key].push_back(i);
    } else {
      out[i] = apply_math_thread_pair(lhs_cols[i], rhs_cols[i], op);
    }
  }

  for (const auto& [key, indices] : batches) {
    std::vector<pluss::Series> lhs_values;
    std::vector<pluss::Series> rhs_values;
    std::vector<pluss::Series> valids;
    lhs_values.reserve(indices.size());
    rhs_values.reserve(indices.size());
    valids.reserve(indices.size());
    for (const auto i : indices) {
      lhs_values.push_back(lhs_cols[i].as_series());
      rhs_values.push_back(rhs_cols[i].as_series());
      valids.push_back(
          thread_valid_on_platform_(lhs_cols[i], key.platform)
              .logical_and(thread_valid_on_platform_(rhs_cols[i], key.platform)));
    }

    const auto lhs_matrix = pack_series_columns(lhs_values, "DataFrame::numeric_pair_batch", true);
    const auto rhs_matrix = pack_series_columns(rhs_values, "DataFrame::numeric_pair_batch", true);
    const auto valid_matrix = pack_series_columns(valids, "DataFrame::numeric_pair_batch", true);
    const auto result_matrix = dataframe_apply_math_matrix_pair_(lhs_matrix, rhs_matrix, op);
    for (std::size_t pos = 0; pos < indices.size(); ++pos) {
      out[indices[pos]] = Thread(
          dataframe_column_from_matrix_(result_matrix, static_cast<std::int64_t>(pos)),
          dataframe_column_from_matrix_(valid_matrix, static_cast<std::int64_t>(pos)));
    }
  }

  std::vector<std::pair<std::string, Thread>> columns;
  columns.reserve(lhs_cols.size());
  for (std::size_t i = 0; i < lhs_cols.size(); ++i) {
    columns.emplace_back(names[i], std::move(out[i]));
  }
  return DataFrame(std::move(columns), matched.left.label().values()).title(lhs.title());
}

Thread align_thread_to_feature_axis_(
    const DataFrame& frame,
    const Thread& value,
    std::string_view where) {
  PLUSS_CHECK(value.defined(), std::string(where) + ": rhs Thread is undefined");
  PLUSS_CHECK(value.kind() == ThreadKind::Series && is_numeric_series_type(value.as_series()),
              std::string(where) + ": rhs Thread must be numeric Series-backed");
  const auto feature_names = frame.features();
  if (feature_names.empty()) {
    return null_thread_like_(value, 0, where);
  }

  const auto labels = value.label().values();
  std::unordered_map<std::string, std::vector<std::int64_t>> positions;
  positions.reserve(static_cast<std::size_t>(value.size()));
  for (std::int64_t i = 0; i < value.size(); ++i) {
    positions[thread_scalar_string_at(labels, i)].push_back(i);
  }

  std::unordered_map<std::string, std::size_t> seen;
  seen.reserve(feature_names.size());
  std::vector<std::int64_t> rows;
  std::vector<bool> present;
  rows.reserve(feature_names.size());
  present.reserve(feature_names.size());
  for (const auto& name : feature_names) {
    const auto occurrence = seen[name]++;
    const auto found = positions.find(name);
    if (found != positions.end() && occurrence < found->second.size()) {
      const auto row = found->second[occurrence];
      rows.push_back(row);
      present.push_back(value.is_valid(row));
    } else {
      rows.push_back(0);
      present.push_back(false);
    }
  }

  auto out = value.take(std::span<const std::int64_t>(rows.data(), rows.size()));
  auto validity = bool_series_from_keep(present);
  if (detail::has_validity_mask(out)) {
    auto prior = *detail::validity_mask(out);
    if (prior.platform() != validity.platform()) {
      prior = prior.to(validity.platform());
    }
    validity = validity.logical_and(prior);
  }
  return detail::with_validity_mask(out, validity);
}

Thread align_thread_to_case_axis_(
    const DataFrame& frame,
    const Thread& value,
    std::string_view where) {
  PLUSS_CHECK(value.defined(), std::string(where) + ": rhs Thread is undefined");
  PLUSS_CHECK(value.kind() == ThreadKind::Series && is_numeric_series_type(value.as_series()),
              std::string(where) + ": rhs Thread must be numeric Series-backed");
  if (frame.ncases() == 0) {
    return null_thread_like_(value, 0, where);
  }

  const auto labels = value.label().values();
  std::unordered_map<std::string, std::vector<std::int64_t>> positions;
  positions.reserve(static_cast<std::size_t>(value.size()));
  for (std::int64_t i = 0; i < value.size(); ++i) {
    positions[thread_scalar_string_at(labels, i)].push_back(i);
  }

  const auto target_cases = frame.label().values();
  std::unordered_map<std::string, std::size_t> seen;
  seen.reserve(static_cast<std::size_t>(frame.ncases()));
  std::vector<std::int64_t> rows;
  std::vector<bool> present;
  rows.reserve(static_cast<std::size_t>(frame.ncases()));
  present.reserve(static_cast<std::size_t>(frame.ncases()));
  for (std::int64_t i = 0; i < frame.ncases(); ++i) {
    const auto key = thread_scalar_string_at(target_cases, i);
    const auto occurrence = seen[key]++;
    const auto found = positions.find(key);
    if (found != positions.end() && occurrence < found->second.size()) {
      const auto row = found->second[occurrence];
      rows.push_back(row);
      present.push_back(value.is_valid(row));
    } else {
      rows.push_back(0);
      present.push_back(false);
    }
  }

  auto out = value.take(std::span<const std::int64_t>(rows.data(), rows.size()));
  auto validity = bool_series_from_keep(present);
  if (detail::has_validity_mask(out)) {
    auto prior = *detail::validity_mask(out);
    if (prior.platform() != validity.platform()) {
      prior = prior.to(validity.platform());
    }
    validity = validity.logical_and(prior);
  }
  return detail::with_validity_mask(out, validity);
}

DataFrame dataframe_math_thread(const DataFrame& frame,
                                const Thread& other,
                                Axis axis,
                                BinaryMathOp op,
                                bool reverse,
                                const AlignOptions& options) {
  PLUSS_CHECK(other.defined(), "DataFrame::math(Thread): rhs Thread is undefined");
  PLUSS_CHECK(other.kind() == ThreadKind::Series && is_numeric_series_type(other.as_series()),
              "DataFrame::math(Thread): rhs Thread must be numeric Series-backed");
  const auto cols = dataframe_columns_(frame);
  const auto& names = frame.features();
  std::vector<std::pair<std::string, Thread>> columns;
  columns.reserve(cols.size());

  if (axis == Axis::Case) {
    Thread aligned;
    switch (options.align) {
      case AlignBy::Pos:
        if (other.size() != frame.ncases()) {
          throw ::pluss::Error(fmt::format(
              "DataFrame::math(Thread, Axis::Case): positional alignment requires rhs length to match "
              "DataFrame cases (rhs={}, cases={})",
              other.size(),
              frame.ncases()));
        }
        aligned = other;
        break;
      case AlignBy::Caselbl:
        aligned = align_thread_to_case_axis_(frame, other, "DataFrame::math(Thread, Axis::Case)");
        break;
      case AlignBy::Feature:
        PLUSS_FAIL("DataFrame::math(Thread, Axis::Case): AlignBy::Feature is not valid for case-axis Thread alignment");
      case AlignBy::Both:
        PLUSS_FAIL("DataFrame::math(Thread): AlignBy::Both is only valid for DataFrame-to-DataFrame alignment");
    }
    aligned = fill_numeric_missing_for_alignment_(aligned, options);
    for (std::size_t i = 0; i < cols.size(); ++i) {
      columns.emplace_back(names[i], reverse ? apply_math_thread_pair(aligned, cols[i], op)
                                             : apply_math_thread_pair(cols[i], aligned, op));
    }
    return DataFrame(std::move(columns), frame.label().values()).title(frame.title());
  }

  Thread aligned;
  switch (options.align) {
    case AlignBy::Pos:
      if (other.size() != frame.nfeatures()) {
        throw ::pluss::Error(fmt::format(
            "DataFrame::math(Thread, Axis::Feature): positional alignment requires rhs length to match "
            "DataFrame features (rhs={}, features={})",
            other.size(),
            frame.nfeatures()));
      }
      aligned = other;
      break;
    case AlignBy::Feature:
      aligned = align_thread_to_feature_axis_(frame, other, "DataFrame::math(Thread, Axis::Feature)");
      break;
    case AlignBy::Caselbl:
      PLUSS_FAIL("DataFrame::math(Thread, Axis::Feature): AlignBy::Caselbl is not valid for feature-axis Thread alignment");
    case AlignBy::Both:
      PLUSS_FAIL("DataFrame::math(Thread): AlignBy::Both is only valid for DataFrame-to-DataFrame alignment");
  }
  aligned = fill_numeric_missing_for_alignment_(aligned, options);
  const auto values = aligned.as_series().to(pluss::codem::ScalarType::Float64).to_vec<double>();
  for (std::size_t i = 0; i < cols.size(); ++i) {
    if (!aligned.is_valid(static_cast<std::int64_t>(i))) {
      columns.emplace_back(
          names[i],
          null_thread_like_(cols[i], frame.ncases(), "DataFrame::math(Thread, Axis::Feature)"));
      continue;
    }
    columns.emplace_back(
        names[i],
        apply_math_thread_scalar(cols[i], values[i], op, reverse));
  }
  return DataFrame(std::move(columns), frame.label().values()).title(frame.title());
}

DataFrame dataframe_math_thread(const DataFrame& frame,
                                const Thread& other,
                                Axis axis,
                                BinaryMathOp op,
                                bool reverse) {
  return dataframe_math_thread(frame, other, axis, op, reverse, AlignOptions{});
}

DataFrame dataframe_cmp_scalar(const DataFrame& frame, double other, BinaryCmpOp op) {
  const auto cols = dataframe_columns_(frame);
  const auto& names = frame.features();
  std::vector<Thread> out(cols.size());

  std::map<NumericBatchKey_, std::vector<std::size_t>> batches;
  for (std::size_t i = 0; i < cols.size(); ++i) {
    const auto key = numeric_batch_key_(cols[i]);
    if (key.has_value()) {
      batches[*key].push_back(i);
    } else {
      out[i] = apply_cmp_thread_scalar(cols[i], other, op);
    }
  }

  for (const auto& [key, indices] : batches) {
    std::vector<pluss::Series> values;
    std::vector<pluss::Series> valids;
    values.reserve(indices.size());
    valids.reserve(indices.size());
    for (const auto i : indices) {
      values.push_back(cols[i].as_series());
      auto valid = cols[i].notnull().as_series();
      if (valid.platform() != key.platform) {
        valid = valid.to(key.platform);
      }
      valids.push_back(std::move(valid));
    }
    const auto value_matrix = pack_series_columns(values, "DataFrame::numeric_batch", true);
    const auto valid_matrix = pack_series_columns(valids, "DataFrame::numeric_batch", true);
    const auto result_matrix =
        dataframe_apply_cmp_matrix_scalar_(value_matrix, other, op);
    for (std::size_t pos = 0; pos < indices.size(); ++pos) {
      out[indices[pos]] = Thread(
          dataframe_column_from_matrix_(result_matrix, static_cast<std::int64_t>(pos)),
          dataframe_column_from_matrix_(valid_matrix, static_cast<std::int64_t>(pos)));
    }
  }

  std::vector<std::pair<std::string, Thread>> columns;
  columns.reserve(cols.size());
  for (std::size_t i = 0; i < cols.size(); ++i) {
    columns.emplace_back(names[i], std::move(out[i]));
  }
  return DataFrame(std::move(columns), frame.label().values());
}

DataFrame dataframe_cmp_frame(const DataFrame& lhs,
                              const DataFrame& rhs,
                              const AlignOptions& options,
                              BinaryCmpOp op);

DataFrame dataframe_cmp_frame(const DataFrame& lhs, const DataFrame& rhs, BinaryCmpOp op) {
  return dataframe_cmp_frame(lhs, rhs, AlignOptions{}, op);
}

DataFrame dataframe_cmp_frame(const DataFrame& lhs,
                              const DataFrame& rhs,
                              const AlignOptions& options,
                              BinaryCmpOp op) {
  auto matched = prepare_frame_pair_for_alignment_(lhs, rhs, options, "DataFrame::compare(DataFrame)");
  matched.left = fill_numeric_missing_for_alignment_(matched.left, options);
  matched.right = fill_numeric_missing_for_alignment_(matched.right, options);
  const auto lhs_cols = dataframe_columns_(matched.left);
  const auto rhs_cols = dataframe_columns_(matched.right);
  const auto& names = matched.left.features();

  std::vector<Thread> out(lhs_cols.size());
  std::map<NumericPairBatchKey_, std::vector<std::size_t>> batches;
  for (std::size_t i = 0; i < lhs_cols.size(); ++i) {
    const auto key = numeric_pair_batch_key_(lhs_cols[i], rhs_cols[i]);
    if (key.has_value()) {
      batches[*key].push_back(i);
    } else {
      out[i] = apply_cmp_thread_pair(lhs_cols[i], rhs_cols[i], op);
    }
  }

  for (const auto& [key, indices] : batches) {
    std::vector<pluss::Series> lhs_values;
    std::vector<pluss::Series> rhs_values;
    std::vector<pluss::Series> valids;
    lhs_values.reserve(indices.size());
    rhs_values.reserve(indices.size());
    valids.reserve(indices.size());
    for (const auto i : indices) {
      lhs_values.push_back(lhs_cols[i].as_series());
      rhs_values.push_back(rhs_cols[i].as_series());
      valids.push_back(
          thread_valid_on_platform_(lhs_cols[i], key.platform)
              .logical_and(thread_valid_on_platform_(rhs_cols[i], key.platform)));
    }

    const auto lhs_matrix = pack_series_columns(lhs_values, "DataFrame::numeric_pair_batch", true);
    const auto rhs_matrix = pack_series_columns(rhs_values, "DataFrame::numeric_pair_batch", true);
    const auto valid_matrix = pack_series_columns(valids, "DataFrame::numeric_pair_batch", true);
    const auto result_matrix = dataframe_apply_cmp_matrix_pair_(lhs_matrix, rhs_matrix, op);
    for (std::size_t pos = 0; pos < indices.size(); ++pos) {
      out[indices[pos]] = Thread(
          dataframe_column_from_matrix_(result_matrix, static_cast<std::int64_t>(pos)),
          dataframe_column_from_matrix_(valid_matrix, static_cast<std::int64_t>(pos)));
    }
  }

  std::vector<std::pair<std::string, Thread>> columns;
  columns.reserve(lhs_cols.size());
  for (std::size_t i = 0; i < lhs_cols.size(); ++i) {
    columns.emplace_back(names[i], std::move(out[i]));
  }
  return DataFrame(std::move(columns), matched.left.label().values()).title(lhs.title());
}

DataFrame dataframe_cmp_thread(const DataFrame& frame,
                               const Thread& other,
                               Axis axis,
                               BinaryCmpOp op,
                               const AlignOptions& options) {
  PLUSS_CHECK(other.defined(), "DataFrame::compare(Thread): rhs Thread is undefined");
  const auto cols = dataframe_columns_(frame);
  const auto& names = frame.features();
  std::vector<std::pair<std::string, Thread>> columns;
  columns.reserve(cols.size());

  if (axis == Axis::Case) {
    Thread aligned;
    switch (options.align) {
      case AlignBy::Pos:
        if (other.size() != frame.ncases()) {
          throw ::pluss::Error(fmt::format(
              "DataFrame::compare(Thread, Axis::Case): positional alignment requires rhs length to match "
              "DataFrame cases (rhs={}, cases={})",
              other.size(),
              frame.ncases()));
        }
        aligned = other;
        break;
      case AlignBy::Caselbl:
        aligned = other.align(frame.label().values(), options);
        break;
      case AlignBy::Feature:
        PLUSS_FAIL("DataFrame::compare(Thread, Axis::Case): AlignBy::Feature is not valid for case-axis Thread alignment");
      case AlignBy::Both:
        PLUSS_FAIL("DataFrame::compare(Thread): AlignBy::Both is only valid for DataFrame-to-DataFrame alignment");
    }
    aligned = fill_numeric_missing_for_alignment_(aligned, options);
    for (std::size_t i = 0; i < cols.size(); ++i) {
      columns.emplace_back(names[i], apply_cmp_thread_pair(cols[i], aligned, op));
    }
    return DataFrame(std::move(columns), frame.label().values()).title(frame.title());
  }

  Thread aligned;
  switch (options.align) {
    case AlignBy::Pos:
      if (other.size() != frame.nfeatures()) {
        throw ::pluss::Error(fmt::format(
            "DataFrame::compare(Thread, Axis::Feature): positional alignment requires rhs length to match "
            "DataFrame features (rhs={}, features={})",
            other.size(),
            frame.nfeatures()));
      }
      aligned = other;
      break;
    case AlignBy::Feature:
      aligned = align_thread_to_feature_axis_(frame, other, "DataFrame::compare(Thread, Axis::Feature)");
      break;
    case AlignBy::Caselbl:
      PLUSS_FAIL("DataFrame::compare(Thread, Axis::Feature): AlignBy::Caselbl is not valid for feature-axis Thread alignment");
    case AlignBy::Both:
      PLUSS_FAIL("DataFrame::compare(Thread): AlignBy::Both is only valid for DataFrame-to-DataFrame alignment");
  }
  aligned = fill_numeric_missing_for_alignment_(aligned, options);
  const auto values = aligned.as_series().to(pluss::codem::ScalarType::Float64).to_vec<double>();
  for (std::size_t i = 0; i < cols.size(); ++i) {
    if (!aligned.is_valid(static_cast<std::int64_t>(i))) {
      columns.emplace_back(
          names[i],
          null_thread_like_(cols[i], frame.ncases(), "DataFrame::compare(Thread, Axis::Feature)"));
      continue;
    }
    columns.emplace_back(names[i], apply_cmp_thread_scalar(cols[i], values[i], op));
  }
  return DataFrame(std::move(columns), frame.label().values()).title(frame.title());
}

DataFrame dataframe_cmp_thread(const DataFrame& frame,
                               const Thread& other,
                               Axis axis,
                               BinaryCmpOp op) {
  return dataframe_cmp_thread(frame, other, axis, op, AlignOptions{});
}

DataFrame arrange_impl_frame(
    const DataFrame& frame,
    const std::vector<DataFrame::ArrangeSpec>& input_specs) {
  if (frame.ncases() <= 1 || input_specs.empty()) {
    return frame;
  }

  std::vector<DataFrame::ArrangeSpec> normalized;
  normalized.reserve(input_specs.size());
  std::unordered_set<std::string> seen;
  seen.reserve(input_specs.size());
  for (const auto& spec : input_specs) {
    PLUSS_CHECK(!spec.feature.empty(), "DataFrame::arrange: feature name cannot be empty");
    PLUSS_CHECK(frame.get(spec.feature).has_value(),
                "DataFrame::arrange: unknown feature '" + spec.feature + "'");
    if (seen.insert(spec.feature).second) {
      normalized.push_back(spec);
    }
  }
  if (normalized.empty()) {
    return frame;
  }

  std::vector<Thread> keys;
  keys.reserve(normalized.size());
  for (const auto& spec : normalized) {
    keys.push_back(frame.features(spec.feature));
  }

  std::vector<std::int64_t> indices(static_cast<std::size_t>(frame.ncases()));
  std::iota(indices.begin(), indices.end(), static_cast<std::int64_t>(0));

  std::stable_sort(indices.begin(), indices.end(), [&](std::int64_t lhs, std::int64_t rhs) {
    for (std::size_t i = 0; i < keys.size(); ++i) {
      const auto lhs_valid = keys[i].is_valid(lhs);
      const auto rhs_valid = keys[i].is_valid(rhs);
      if (lhs_valid != rhs_valid) {
        if (normalized[i].nulls_last) {
          return lhs_valid; // valid values first
        }
        return !lhs_valid; // null values first
      }

      const auto cmp = keys[i].compare_rows(lhs, rhs, true);
      if (cmp < 0) {
        return normalized[i].ascending;
      }
      if (cmp > 0) {
        return !normalized[i].ascending;
      }
    }
    return false;
  });

  return frame.take(indices);
}

class QueryParser final {
public:
  QueryParser(const DataFrame& frame, std::vector<QueryToken> tokens)
      : frame_(frame), tokens_(std::move(tokens)) {}

  QueryEvalFn parse() {
    auto eval = parse_or_expr();
    expect(QueryTokenKind::End, "DataFrame::query: unexpected trailing tokens");
    return eval;
  }

private:
  const QueryToken& peek() const {
    return tokens_[pos_];
  }

  bool match(QueryTokenKind kind) {
    if (peek().kind == kind) {
      ++pos_;
      return true;
    }
    return false;
  }

  const QueryToken& advance() {
    const auto& tok = peek();
    if (tok.kind != QueryTokenKind::End) {
      ++pos_;
    }
    return tok;
  }

  const QueryToken& expect(QueryTokenKind kind, std::string_view message) {
    PLUSS_CHECK(peek().kind == kind, std::string(message));
    return advance();
  }

  Thread resolve_feature(std::string_view ident) const {
    const auto low = lower_copy(owned_name(ident));
    if (low == "case" || low == "cases") {
      return frame_.label().values();
    }
    return frame_.features(ident);
  }

  QueryLiteral parse_literal() {
    QueryLiteral out;
    if (match(QueryTokenKind::String)) {
      out.kind = QueryLiteral::Kind::String;
      out.text = tokens_[pos_ - 1].text;
      return out;
    }
    if (match(QueryTokenKind::Boolean)) {
      out.kind = QueryLiteral::Kind::Bool;
      out.boolean = lower_copy(tokens_[pos_ - 1].text) == "true";
      return out;
    }
    if (match(QueryTokenKind::Number)) {
      out.kind = QueryLiteral::Kind::Number;
      double parsed = 0.0;
      PLUSS_CHECK(parse_double_exact(tokens_[pos_ - 1].text, parsed),
                  "DataFrame::query: invalid numeric literal");
      out.number = parsed;
      return out;
    }
    PLUSS_FAIL("DataFrame::query: expected a string, bool, or numeric literal");
  }

  QueryRhs parse_rhs_feature_or_literal() {
    QueryRhs rhs;
    if (peek().kind == QueryTokenKind::Identifier) {
      rhs.is_feature = true;
      rhs.feature = resolve_feature(advance().text);
      return rhs;
    }
    rhs.is_feature = false;
    rhs.literal = parse_literal();
    return rhs;
  }

  std::vector<QueryLiteral> parse_literal_list() {
    expect(QueryTokenKind::LParen, "DataFrame::query: expected '(' after in/not in");
    std::vector<QueryLiteral> items;
    if (match(QueryTokenKind::RParen)) {
      return items;
    }
    while (true) {
      items.push_back(parse_literal());
      if (match(QueryTokenKind::Comma)) {
        continue;
      }
      expect(QueryTokenKind::RParen, "DataFrame::query: expected ')' to close in-list");
      break;
    }
    return items;
  }

  QueryCompareOp parse_compare_op() {
    if (match(QueryTokenKind::Eq)) {
      return QueryCompareOp::Eq;
    }
    if (match(QueryTokenKind::Ne)) {
      return QueryCompareOp::Ne;
    }
    if (match(QueryTokenKind::Lt)) {
      return QueryCompareOp::Lt;
    }
    if (match(QueryTokenKind::Le)) {
      return QueryCompareOp::Le;
    }
    if (match(QueryTokenKind::Gt)) {
      return QueryCompareOp::Gt;
    }
    if (match(QueryTokenKind::Ge)) {
      return QueryCompareOp::Ge;
    }
    PLUSS_FAIL("DataFrame::query: expected a comparison operator");
  }

  QueryEvalFn parse_primary() {
    if (match(QueryTokenKind::LParen)) {
      auto inner = parse_or_expr();
      expect(QueryTokenKind::RParen, "DataFrame::query: expected ')'");
      return inner;
    }

    if (match(QueryTokenKind::IsNull)) {
      expect(QueryTokenKind::LParen, "DataFrame::query: expected '(' after isnull");
      const auto ident = expect(QueryTokenKind::Identifier, "DataFrame::query: expected feature name in isnull");
      auto target = resolve_feature(ident.text);
      expect(QueryTokenKind::RParen, "DataFrame::query: expected ')' after isnull(feature)");
      return [target](std::int64_t row) { return !row_valid_for_thread(target, row); };
    }

    if (match(QueryTokenKind::NotNull)) {
      expect(QueryTokenKind::LParen, "DataFrame::query: expected '(' after notnull");
      const auto ident = expect(QueryTokenKind::Identifier, "DataFrame::query: expected feature name in notnull");
      auto target = resolve_feature(ident.text);
      expect(QueryTokenKind::RParen, "DataFrame::query: expected ')' after notnull(feature)");
      return [target](std::int64_t row) { return row_valid_for_thread(target, row); };
    }

    if (match(QueryTokenKind::Contains)) {
      expect(QueryTokenKind::LParen, "DataFrame::query: expected '(' after contains");
      const auto ident = expect(QueryTokenKind::Identifier, "DataFrame::query: expected feature name in contains");
      auto target = resolve_feature(ident.text);
      expect(QueryTokenKind::Comma, "DataFrame::query: expected ',' in contains(feature, \"text\")");
      const auto literal = parse_literal();
      PLUSS_CHECK(literal.kind == QueryLiteral::Kind::String,
                  "DataFrame::query: contains(feature, literal) requires string literal");
      expect(QueryTokenKind::RParen, "DataFrame::query: expected ')' after contains(feature, literal)");
      const auto needle = literal.text;
      return [target, needle](std::int64_t row) {
        if (!row_valid_for_thread(target, row)) {
          return false;
        }
        return thread_scalar_string_at(target, row).find(needle) != std::string::npos;
      };
    }

    const auto ident = expect(QueryTokenKind::Identifier, "DataFrame::query: expected feature name");
    auto target = resolve_feature(ident.text);

    if (match(QueryTokenKind::IsNull)) {
      return [target](std::int64_t row) { return !row_valid_for_thread(target, row); };
    }
    if (match(QueryTokenKind::NotNull)) {
      return [target](std::int64_t row) { return row_valid_for_thread(target, row); };
    }

    bool negate = false;
    if (match(QueryTokenKind::Not)) {
      negate = true;
    }
    if (match(QueryTokenKind::In)) {
      const auto items = parse_literal_list();
      PLUSS_CHECK(!items.empty(), "DataFrame::query: in-list cannot be empty");
      return [target, items, negate](std::int64_t row) {
        const auto present = evaluate_in_list(target, row, items);
        return negate ? !present : present;
      };
    }
    PLUSS_CHECK(!negate, "DataFrame::query: expected 'in' after 'not' in predicate");

    if (match(QueryTokenKind::Between)) {
      const auto lower = parse_literal();
      expect(QueryTokenKind::And, "DataFrame::query: expected 'and' in between expression");
      const auto upper = parse_literal();
      return [target, lower, upper](std::int64_t row) {
        return evaluate_query_compare(target, row, QueryCompareOp::Ge, lower)
            && evaluate_query_compare(target, row, QueryCompareOp::Le, upper);
      };
    }

    const auto op = parse_compare_op();
    const auto rhs = parse_rhs_feature_or_literal();
    return [target, op, rhs](std::int64_t row) {
      if (rhs.is_feature) {
        return evaluate_query_compare(target, rhs.feature, row, op);
      }
      return evaluate_query_compare(target, row, op, rhs.literal);
    };
  }

  QueryEvalFn parse_unary_expr() {
    if (match(QueryTokenKind::Not)) {
      auto inner = parse_unary_expr();
      return [inner](std::int64_t row) { return !inner(row); };
    }
    return parse_primary();
  }

  QueryEvalFn parse_and_expr() {
    auto left = parse_unary_expr();
    while (match(QueryTokenKind::And)) {
      auto right = parse_unary_expr();
      auto prev = std::move(left);
      left = [prev, right](std::int64_t row) { return prev(row) && right(row); };
    }
    return left;
  }

  QueryEvalFn parse_or_expr() {
    auto left = parse_and_expr();
    while (match(QueryTokenKind::Or)) {
      auto right = parse_and_expr();
      auto prev = std::move(left);
      left = [prev, right](std::int64_t row) { return prev(row) || right(row); };
    }
    return left;
  }

  const DataFrame& frame_;
  std::vector<QueryToken> tokens_;
  std::size_t pos_{0};
};

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

} // namespace

detail::DataFrameImpl::DataFrameImpl(std::vector<std::string> names,
                                     std::vector<Thread> columns,
                                     std::optional<Thread> cases,
                                     std::string title,
                                     CaseAxis case_axis)
    : names_(std::move(names)),
      columns_(std::move(columns)),
      case_axis_(std::move(case_axis)),
      title_(std::move(title)) {
  if (cases.has_value()) {
    cases_ = detail::with_label_kind(*cases, LabelKind::Case);
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

  if (expected_rows.has_value()) {
    nrows_ = *expected_rows;
  } else if (cases_.defined()) {
    nrows_ = cases_.size();
  } else {
    nrows_ = 0;
  }

  if (!cases_.defined()) {
    cases_ = default_cases(nrows_);
  }
  PLUSS_CHECK(cases_.defined(), "DataFrame: cases are undefined");
  PLUSS_CHECK(cases_.size() == nrows_,
              fmt::format("DataFrame: case length mismatch (cases={}, nrows={}, cols={})",
                          cases_.size(),
                          nrows_,
                          columns_.size()));
  if (case_axis_.defined()) {
    std::unordered_set<std::string> level_names;
    level_names.reserve(case_axis_.levels.size());
    for (const auto& level : case_axis_.levels) {
      PLUSS_CHECK(!level.name.empty(), "DataFrame: case level names must be non-empty");
      PLUSS_CHECK(level.values.defined(), "DataFrame: case level values are undefined");
      PLUSS_CHECK(level.values.size() == nrows_,
                  "DataFrame: case level length mismatch for '" + level.name + "'");
      PLUSS_CHECK(level_names.insert(level.name).second,
                  "DataFrame: duplicate case level name '" + level.name + "'");
    }
  }
}

namespace {

pluss::msharp::intrusive_ptr<detail::DataFrameImpl> clone_impl(const detail::DataFrameImpl& src) {
  return pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, src.columns_, src.cases_, src.title_, src.case_axis_);
}

Thread clear_assignment_metadata_(const Thread& value) {
  auto out = value;
  if (detail::has_display_labels(out)) {
    out = detail::clear_display_labels(out);
  }
  if (out.has_display_name()) {
    out = out.clear_display_name();
  }
  return out;
}

Thread null_thread_like_(const Thread& value, std::int64_t size, std::string_view where) {
  PLUSS_CHECK(size >= 0, std::string(where) + ": output size must be non-negative");
  Thread out;
  switch (value.kind()) {
    case ThreadKind::Series:
      out = Thread(pluss::Series::zeros(
          {size}, value.as_series().scalar_type(), value.as_series().platform()));
      break;
    case ThreadKind::Note:
      out = Thread(pluss::make_note(
          std::vector<std::string>(static_cast<std::size_t>(size), std::string{})));
      break;
    case ThreadKind::Timeline:
      out = Thread(pluss::make_timeline(
          std::vector<std::string>(static_cast<std::size_t>(size), "1970-01-01T00:00:00Z")));
      break;
    case ThreadKind::TimeIndex:
      out = Thread(pluss::make_timeindex(
          std::vector<std::string>(static_cast<std::size_t>(size), "1970-01-01T00:00:00Z")));
      break;
  }
  return clear_assignment_metadata_(detail::with_validity_mask(
      out, pluss::Series::zeros({size}, pluss::codem::ScalarType::Bool)));
}

Thread broadcast_assignment_thread_(const Thread& value, std::int64_t size, std::string_view where) {
  PLUSS_CHECK(value.defined(), std::string(where) + ": input feature is undefined");
  PLUSS_CHECK(value.size() == 1, std::string(where) + ": scalar broadcast requires a single value");
  if (size == 0) {
    return null_thread_like_(value, 0, where);
  }
  std::vector<std::int64_t> indices(static_cast<std::size_t>(size), 0);
  return clear_assignment_metadata_(value.take(indices));
}

Thread align_assignment_thread_(
    const detail::DataFrameImpl& src,
    const Thread& value,
    std::string_view where) {
  if (value.size() == 0) {
    return null_thread_like_(value, src.nrows_, where);
  }

  const auto labels = value.label().values();
  std::unordered_map<std::string, std::int64_t> positions;
  positions.reserve(static_cast<std::size_t>(value.size()));
  for (std::int64_t i = 0; i < value.size(); ++i) {
    const auto key = thread_scalar_string_at(labels, i);
    PLUSS_CHECK(positions.emplace(key, i).second,
                std::string(where) + ": duplicate rhs case label '" + key + "'");
  }

  std::vector<std::int64_t> take_indices;
  take_indices.reserve(static_cast<std::size_t>(src.nrows_));
  std::vector<bool> matched;
  matched.reserve(static_cast<std::size_t>(src.nrows_));
  for (std::int64_t row = 0; row < src.nrows_; ++row) {
    const auto it = positions.find(thread_scalar_string_at(src.cases_, row));
    if (it == positions.end()) {
      take_indices.push_back(0);
      matched.push_back(false);
    } else {
      take_indices.push_back(it->second);
      matched.push_back(value.is_valid(it->second));
    }
  }

  auto out = value.take(take_indices);
  auto validity = bool_series_from_keep(matched);
  if (detail::has_validity_mask(out)) {
    if (validity.platform() != detail::validity_mask(out)->platform()) {
      validity = validity.to(detail::validity_mask(out)->platform());
    }
    validity = validity.logical_and(*detail::validity_mask(out));
  }
  return clear_assignment_metadata_(detail::with_validity_mask(out, validity));
}

bool assignment_cases_match_(
    const detail::DataFrameImpl& src,
    const Thread& value) {
  if (!value.defined() || value.size() != src.nrows_) {
    return false;
  }
  const auto labels = value.label().values();
  for (std::int64_t row = 0; row < src.nrows_; ++row) {
    if (thread_scalar_string_at(labels, row) != thread_scalar_string_at(src.cases_, row)) {
      return false;
    }
  }
  return true;
}

enum class AlignUse_ : std::uint8_t {
  Assignment = 0,
  Mask = 1,
};

std::string align_use_name_(AlignUse_ use) {
  switch (use) {
    case AlignUse_::Assignment:
      return "assignment";
    case AlignUse_::Mask:
      return "mask";
  }
  return "operation";
}

Thread prepare_thread_for_use_(
    const detail::DataFrameImpl& src,
    const Thread& value,
    std::string_view where,
    const AlignOptions& options,
    AlignUse_ use) {
  PLUSS_CHECK(value.defined(), std::string(where) + ": input Thread is undefined");
  if (src.columns_.empty()) {
    return clear_assignment_metadata_(value);
  }

  switch (options.align) {
    case AlignBy::Pos:
      if (value.size() != src.nrows_) {
        throw ::pluss::Error(
            fmt::format("{}: positional {} requires rhs length to match DataFrame cases "
                        "(rhs={}, cases={})",
                        where,
                        align_use_name_(use),
                        value.size(),
                        src.nrows_));
      }
      return clear_assignment_metadata_(value);
    case AlignBy::Caselbl:
      if (assignment_cases_match_(src, value)) {
        return clear_assignment_metadata_(value);
      }
      return clear_assignment_metadata_(value.align(src.cases_, options));
    case AlignBy::Feature:
      PLUSS_FAIL(std::string(where) + ": AlignBy::Feature is not valid here; "
                 + align_use_name_(use) + " expects one value per case");
    case AlignBy::Both:
      PLUSS_FAIL(std::string(where) + ": AlignBy::Both is only valid for DataFrame-to-DataFrame alignment");
  }
  PLUSS_FAIL(std::string(where) + ": unsupported alignment mode");
}

Thread prepare_feature_assignment_(
    const detail::DataFrameImpl& src,
    const Thread& value,
    std::string_view where,
    const AlignOptions& options) {
  return prepare_thread_for_use_(src, value, where, options, AlignUse_::Assignment);
}

Thread prepare_mask_thread_(
    const detail::DataFrameImpl& src,
    const Thread& bool_mask,
    std::string_view where,
    const AlignOptions& options) {
  auto prepared = prepare_thread_for_use_(src, bool_mask, where, options, AlignUse_::Mask);
  PLUSS_CHECK(prepared.kind() == ThreadKind::Series, std::string(where) + ": mask must be a Series thread");
  return prepared;
}

Thread prepare_feature_assignment_(
    const detail::DataFrameImpl& src,
    const Thread& value,
    std::string_view where) {
  return prepare_feature_assignment_(src, value, where, AlignOptions{});
}

Thread prepare_mask_thread_(
    const detail::DataFrameImpl& src,
    const Thread& bool_mask,
    std::string_view where) {
  return prepare_mask_thread_(src, bool_mask, where, AlignOptions{});
}

void set_prepared_feature_(
    detail::DataFrameImpl& frame,
    std::string_view name,
    const Thread& prepared) {
  PLUSS_CHECK(prepared.defined(), "DataFrame::set: prepared feature is undefined");
  const auto key = owned_name(name);
  const auto it = frame.name_to_idx_.find(key);
  if (it == frame.name_to_idx_.end()) {
    frame.names_.push_back(key);
    frame.columns_.push_back(prepared);
  } else {
    frame.columns_[it->second] = prepared;
  }
  frame.validate_and_reindex_();
}

} // namespace


#include "table/ops/dataframe/DataFrameLifecycleAndIO.inc"
#include "table/ops/dataframe/DataFrameCoreAndFeatures.inc"
#include "table/ops/dataframe/DataFrameSelectionAndTransform.inc"
#include "table/ops/dataframe/DataFrameTimeSeries.inc"
#include "table/ops/dataframe/DataFrameGroupAndCombine.inc"
#include "table/ops/dataframe/DataFrameMathMissingAndDescribe.inc"
#include "table/ops/dataframe/DataFrameStatisticsAndWindows.inc"

} // namespace pluss::table

#include "table/DataFrame.h"

#include <algorithm>
#include <array>
#include <cmath>
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

#include "CodeM/core/Errors.h"
#include "CodeM/core/Half.h"
#include "loader/CsvLoader.h"
#include "table/display/Display.h"

namespace pluss::table {

namespace {

std::string owned_name(std::string_view sv) {
  return std::string(sv.data(), sv.size());
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

Thread default_case_index(std::int64_t nrows) {
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

DataFrameImpl::DataFrameImpl(std::vector<std::string> names,
                             std::vector<Thread> columns,
                             std::optional<Thread> case_index)
    : names_(std::move(names)), columns_(std::move(columns)) {
  if (case_index.has_value()) {
    case_index_ = *case_index;
  }
  validate_and_reindex_();
}

void DataFrameImpl::validate_and_reindex_() {
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

  if (!case_index_.defined()) {
    case_index_ = default_case_index(nrows_);
  }
  PLUSS_CHECK(case_index_.defined(), "DataFrame: case index is undefined");
  PLUSS_CHECK(case_index_.size() == nrows_, "DataFrame: case index length mismatch");
}

namespace {

pluss::msharp::intrusive_ptr<DataFrameImpl> clone_impl(const DataFrameImpl& src) {
  return pluss::msharp::make_intrusive<DataFrameImpl>(src.names_, src.columns_, src.case_index_);
}

} // namespace

ColumnRef::ColumnRef(std::string name, Thread column)
    : name_(std::move(name)), column_(std::move(column)) {}

const std::string& ColumnRef::name() const noexcept { return name_; }
const Thread& ColumnRef::thread() const noexcept { return column_; }
double ColumnRef::mean() const { return column_.mean(); }
Thread ColumnRef::head(std::int64_t n) const { return column_.head(n); }

DataFrame::DataFrame(std::vector<std::pair<std::string, Thread>> columns,
                     std::optional<Thread> case_index) {
  std::vector<std::string> names;
  std::vector<Thread> values;
  names.reserve(columns.size());
  values.reserve(columns.size());
  for (auto& kv : columns) {
    names.push_back(std::move(kv.first));
    values.push_back(std::move(kv.second));
  }
  impl_ = pluss::msharp::make_intrusive<DataFrameImpl>(std::move(names), std::move(values), std::move(case_index));
}

DataFrame::DataFrame(pluss::msharp::intrusive_ptr<DataFrameImpl> impl) : impl_(std::move(impl)) {}

DataFrame DataFrame::load(std::string_view source_type, const pluss::loader::ReadCsvOptions& options) {
  if (source_type == "csv") {
    return pluss::loader::load_csv(options);
  }
  PLUSS_FAIL("DataFrame::load: unsupported source type '" + std::string(source_type) + "'");
}

DataFrame DataFrame::read_csv(const pluss::loader::ReadCsvOptions& options) {
  return pluss::loader::load_csv(options);
}

bool DataFrame::defined() const noexcept { return static_cast<bool>(impl_); }
bool DataFrame::unique() const noexcept { return impl_.unique(); }

const DataFrameImpl& DataFrame::impl_ref_() const {
  PLUSS_CHECK(defined(), "DataFrame: undefined handle");
  return *impl_;
}

DataFrameImpl& DataFrame::mut_impl_() {
  PLUSS_CHECK(defined(), "DataFrame: undefined handle");
  if (!impl_.unique()) {
    impl_ = clone_impl(*impl_);
  }
  return *impl_;
}

std::int64_t DataFrame::nrows() const { return impl_ref_().nrows_; }
std::int64_t DataFrame::ncols() const { return static_cast<std::int64_t>(impl_ref_().columns_.size()); }

const std::vector<std::string>& DataFrame::column_names() const { return impl_ref_().names_; }

std::vector<Thread> DataFrame::columns() const { return impl_ref_().columns_; }

const Thread& DataFrame::case_index() const { return impl_ref_().case_index_; }

pluss::Series DataFrame::index() const {
  PLUSS_CHECK(case_index().kind() == ThreadKind::Series, "DataFrame::index requires a Series-backed case index");
  return case_index().as_series();
}

pluss::Series DataFrame::case_() const { return index(); }

DataFrame DataFrame::with_case_index(const Thread& case_idx) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(case_idx.defined(), "DataFrame::with_case_index: case index is undefined");
  PLUSS_CHECK(case_idx.size() == src.nrows_, "DataFrame::with_case_index: case index length mismatch");
  return DataFrame(pluss::msharp::make_intrusive<DataFrameImpl>(src.names_, src.columns_, case_idx));
}

DataFrame& DataFrame::with_case_index_(const Thread& case_idx) {
  auto& w = mut_impl_();
  PLUSS_CHECK(case_idx.defined(), "DataFrame::with_case_index_: case index is undefined");
  PLUSS_CHECK(case_idx.size() == w.nrows_, "DataFrame::with_case_index_: case index length mismatch");
  w.case_index_ = case_idx;
  w.validate_and_reindex_();
  return *this;
}

bool DataFrame::has_column(std::string_view name) const {
  const auto& src = impl_ref_();
  return src.name_to_idx_.find(owned_name(name)) != src.name_to_idx_.end();
}

Thread DataFrame::column(std::string_view name) const {
  const auto& src = impl_ref_();
  const auto it = src.name_to_idx_.find(owned_name(name));
  PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame: unknown column '" + owned_name(name) + "'");
  return src.columns_[it->second];
}

ColumnRef DataFrame::column_ref(std::string_view name) const {
  return ColumnRef(owned_name(name), column(name));
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
  return DataFrame(pluss::msharp::make_intrusive<DataFrameImpl>(std::move(out_names), std::move(out_cols), src.case_index_));
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
  return DataFrame(pluss::msharp::make_intrusive<DataFrameImpl>(std::move(out_names), std::move(out_cols), src.case_index_));
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
  return DataFrame(pluss::msharp::make_intrusive<DataFrameImpl>(std::move(out_names), std::move(out_cols), src.case_index_));
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
  return DataFrame(pluss::msharp::make_intrusive<DataFrameImpl>(std::move(out_names), src.columns_, src.case_index_));
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
  auto out_case = src.case_index_.take(indices);
  return DataFrame(pluss::msharp::make_intrusive<DataFrameImpl>(src.names_, std::move(out_cols), std::move(out_case)));
}

DataFrame DataFrame::filter(const pluss::Series& bool_mask) const {
  validate_bool_mask(bool_mask, nrows(), "DataFrame::filter");
  const auto& src = impl_ref_();
  std::vector<Thread> out_cols;
  out_cols.reserve(src.columns_.size());
  for (const auto& c : src.columns_) {
    out_cols.push_back(c.filter(bool_mask));
  }
  auto out_case = src.case_index_.filter(bool_mask);
  return DataFrame(pluss::msharp::make_intrusive<DataFrameImpl>(src.names_, std::move(out_cols), std::move(out_case)));
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
  return DataFrame(pluss::msharp::make_intrusive<DataFrameImpl>(src.names_, std::move(out_cols), src.case_index_));
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
    out.show();
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
    out.show();
  }
  return out;
}

std::string DataFrame::to_string(const display::DisplayOptions& options) const {
  return display::render(*this, options);
}

std::string DataFrame::info_string(const display::DisplayOptions& options) const {
  return display::render_info(*this, options);
}

void DataFrame::show(const display::DisplayOptions& options) const {
  display::print(std::cout, *this, options);
}

void DataFrame::show(std::ostream& os, const display::DisplayOptions& options) const {
  display::print(os, *this, options);
}

void DataFrame::info(const display::DisplayOptions& options) const {
  display::print_info(std::cout, *this, options);
}

void DataFrame::info(std::ostream& os, const display::DisplayOptions& options) const {
  display::print_info(os, *this, options);
}

std::ostream& operator<<(std::ostream& os, const DataFrame& frame) {
  frame.show(os);
  return os;
}

} // namespace pluss::table

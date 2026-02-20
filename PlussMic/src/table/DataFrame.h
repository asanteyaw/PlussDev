#pragma once

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iosfwd>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "MSharp/IntrusivePtr.h"
#include "table/Thread.h"
#include "table/display/Display.h"
#include "pluss/codem/codem.h"

namespace pluss::loader {
struct ReadCsvOptions;
} // namespace pluss::loader

namespace pluss::table {

class DataFrameImpl final : public pluss::msharp::RefCounted {
public:
  DataFrameImpl(std::vector<std::string> names,
                std::vector<Thread> columns,
                std::optional<Thread> case_index = std::nullopt);
  void validate_and_reindex_();

  std::vector<std::string> names_;
  std::vector<Thread> columns_;
  std::unordered_map<std::string, std::size_t> name_to_idx_;
  std::int64_t nrows_{0};
  Thread case_index_{};
};

class ColumnRef {
public:
  ColumnRef(std::string name, Thread column);

  const std::string& name() const noexcept;
  const Thread& thread() const noexcept;

  double mean() const;
  Thread head(std::int64_t n) const;

private:
  std::string name_;
  Thread column_;
};

class DataFrame {
public:
  DataFrame() = default;
  explicit DataFrame(std::vector<std::pair<std::string, Thread>> columns,
                     std::optional<Thread> case_index = std::nullopt);

  static DataFrame load(std::string_view source_type, const pluss::loader::ReadCsvOptions& options);
  static DataFrame read_csv(const pluss::loader::ReadCsvOptions& options);

  bool defined() const noexcept;
  bool unique() const noexcept;

  std::int64_t nrows() const;
  std::int64_t ncols() const;
  const std::vector<std::string>& column_names() const;
  std::vector<Thread> columns() const;
  const Thread& case_index() const;
  pluss::Series index() const;
  pluss::Series case_() const;

  DataFrame with_case_index(const Thread& case_index) const;
  DataFrame& with_case_index_(const Thread& case_index);

  bool has_column(std::string_view name) const;
  Thread column(std::string_view name) const;
  ColumnRef column_ref(std::string_view name) const;

  DataFrame select(std::initializer_list<std::string_view> names) const;
  DataFrame drop(std::initializer_list<std::string_view> names) const;
  DataFrame reorder(std::initializer_list<std::string_view> names) const;

  DataFrame with_column(std::string name, const Thread& column) const;
  DataFrame& with_column_(std::string name, const Thread& column);

  DataFrame rename(std::string_view old_name, std::string new_name) const;
  DataFrame& rename_(std::string_view old_name, std::string new_name);

  DataFrame head(std::int64_t n = 5, bool render = true) const;
  DataFrame tail(std::int64_t n = 5, bool render = false) const;
  DataFrame slice(std::int64_t start, std::int64_t stop) const;
  DataFrame take(std::span<const std::int64_t> indices) const;
  DataFrame filter(const pluss::Series& bool_mask) const;

  DataFrame fillna(double value) const;
  DataFrame& fillna_(double value);
  DataFrame dropna() const;
  std::int64_t count_null_rows() const;
  DataFrame describe(bool render = true) const;
  DataFrame view(std::int64_t start, std::int64_t stop, bool render = true) const;

  std::string to_string(const display::DisplayOptions& options = {}) const;
  std::string info_string(const display::DisplayOptions& options = {}) const;
  void show(const display::DisplayOptions& options = {}) const;
  void show(std::ostream& os, const display::DisplayOptions& options = {}) const;
  void info(const display::DisplayOptions& options = {}) const;
  void info(std::ostream& os, const display::DisplayOptions& options = {}) const;

private:
  explicit DataFrame(pluss::msharp::intrusive_ptr<DataFrameImpl> impl);
  DataFrameImpl& mut_impl_();
  const DataFrameImpl& impl_ref_() const;

  pluss::msharp::intrusive_ptr<DataFrameImpl> impl_{nullptr};
};

std::ostream& operator<<(std::ostream& os, const DataFrame& frame);

} // namespace pluss::table

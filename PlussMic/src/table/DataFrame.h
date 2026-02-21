#pragma once

#include <cstddef>
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
#include "loader/Config.h"
#include "loader/CsvLoader.h"
#include "loader/ExcelLoader.h"
#include "table/Thread.h"
#include "table/display/Display.h"
#include "pluss/codem/codem.h"

namespace pluss::loader {
class LoadResult;
}

namespace pluss::table {

namespace detail {
class DataFrameImpl;
}

class ColumnRef {
public:
  ColumnRef(std::string name, Thread column);

  const std::string& name() const noexcept;
  const Thread& thread() const noexcept;

  double mean() const;
  Thread head(std::int64_t n) const;
  ThreadValue value() const;
  Thread values() const;

private:
  std::string name_;
  Thread column_;
};

class DataFrame {
public:
  enum class SummaryOp : std::uint8_t {
    Count = 0,
    Nunique = 1,
    Sum = 2,
    Mean = 3,
    Min = 4,
    Max = 5,
    Std = 6,
    Var = 7,
  };

  enum class MissingOp : std::uint8_t {
    Fill = 0,
    ForwardFill = 1,
    BackwardFill = 2,
    DropAny = 3,
    DropAll = 4,
    IsNull = 5,
    NotNull = 6,
  };

  struct SummarySpec {
    std::string output;
    std::string feature;
    SummaryOp op{SummaryOp::Mean};
  };

  struct DeriveSpec {
    std::string feature;
    std::function<Thread(const DataFrame&)> expr;
  };

  struct MissingSpec {
    MissingOp op{MissingOp::DropAny};
    std::vector<std::string> features{};
    std::optional<double> value{std::nullopt};
  };

  struct ArrangeSpec {
    std::string feature;
    bool ascending{true};
    bool nulls_last{true};
  };

  struct CastSpec {
    std::string feature;
    ThreadCastTarget target{ThreadCastTarget::Series};
    std::optional<pluss::codem::ScalarType> series_dtype{std::nullopt};
  };

  struct WindowSpec {
    std::string output;
    std::string feature;
    ThreadWindowOp op{ThreadWindowOp::RollingMean};
    std::int64_t window{3};
    std::int64_t min_periods{1};
  };

  using QueryFn = std::function<pluss::Series(const DataFrame&)>;

  DataFrame();
  explicit DataFrame(std::vector<std::pair<std::string, Thread>> columns,
                     std::optional<Thread> cases = std::nullopt);
  DataFrame(const DataFrame& other);
  DataFrame(DataFrame&& other) noexcept;
  DataFrame& operator=(const DataFrame& other);
  DataFrame& operator=(DataFrame&& other) noexcept;
  ~DataFrame();

  static DataFrame load(std::string_view source_type, std::string_view filepath);
  static DataFrame load(std::string_view source_type,
                        std::string_view filepath,
                        const pluss::loader::Config& config);
  static DataFrame load(std::string_view source_type,
                        std::string_view filepath,
                        std::initializer_list<pluss::loader::ConfigEntry> config_entries);
  static DataFrame load(std::string_view source_type,
                        std::string_view filepath,
                        const pluss::loader::ReadCsvOptions& options);
  static DataFrame load(std::string_view source_type,
                        std::string_view filepath,
                        char sep,
                        std::optional<std::size_t> nrows = std::nullopt,
                        std::vector<std::string> usecols = {},
                        std::optional<std::string> case_col = std::nullopt,
                        pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error);
  static DataFrame load(std::string_view source_type,
                        std::string_view filepath,
                        char sep,
                        std::optional<std::size_t> nrows,
                        std::initializer_list<std::string_view> usecols,
                        std::optional<std::string> case_col = std::nullopt,
                        pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error);
  static DataFrame load_csv(std::string_view filepath);
  static DataFrame load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options);
  static DataFrame load_csv(std::string_view filepath, const pluss::loader::Config& config);
  static DataFrame load_csv(std::string_view filepath,
                            std::initializer_list<pluss::loader::ConfigEntry> config_entries);
  static DataFrame load_csv(std::string_view filepath,
                            std::string_view header,
                            std::string_view case_feature = "none",
                            std::string_view engine = "auto",
                            std::optional<std::size_t> ncases = std::nullopt);
  static DataFrame load_csv(std::string_view filepath,
                            std::string_view header,
                            std::size_t ncases);
  static DataFrame load_csv(std::string_view filepath,
                            char sep,
                            std::optional<std::size_t> nrows = std::nullopt,
                            std::vector<std::string> usecols = {},
                            std::optional<std::string> case_col = std::nullopt,
                            pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error);
  static DataFrame load_csv(std::string_view filepath,
                            char sep,
                            std::optional<std::size_t> nrows,
                            std::initializer_list<std::string_view> usecols,
                            std::optional<std::string> case_col = std::nullopt,
                            pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error);
  static DataFrame load_excel(std::string_view filepath);
  static DataFrame load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options);
  static DataFrame load_excel(std::string_view filepath, const pluss::loader::Config& config);
  static DataFrame load_excel(std::string_view filepath,
                              std::initializer_list<pluss::loader::ConfigEntry> config_entries);
  static DataFrame load_excel(std::string_view filepath,
                              std::string_view sheet_name,
                              bool header = true);
  static DataFrame load_excel(std::string_view filepath,
                              std::int64_t sheet_index,
                              bool header = true);
  static pluss::loader::LoadResult load_excel(std::string_view filepath,
                                              std::nullptr_t,
                                              bool header = true);
  static pluss::loader::LoadResult load_excel(std::string_view filepath,
                                              const std::vector<std::string>& sheet_names,
                                              bool header = true);
  static pluss::loader::LoadResult load_excel(std::string_view filepath,
                                              std::initializer_list<std::string_view> sheet_names,
                                              bool header = true);
  static pluss::loader::LoadResult load_excel(std::string_view filepath,
                                              const std::vector<std::int64_t>& sheet_indices,
                                              bool header = true);
  static pluss::loader::LoadResult load_excel(std::string_view filepath,
                                              std::initializer_list<std::int64_t> sheet_indices,
                                              bool header = true);
  static DataFrame load_parquet(std::string_view filepath);
  static DataFrame load_parquet(std::string_view filepath, const pluss::loader::Config& config);
  static DataFrame load_parquet(std::string_view filepath,
                                std::initializer_list<pluss::loader::ConfigEntry> config_entries);

  bool defined() const noexcept;
  bool unique() const noexcept;

  std::int64_t nrows() const;
  std::int64_t ncols() const;
  std::int64_t nfeatures() const;
  const std::vector<std::string>& column_names() const;
  std::vector<Thread> columns() const;
  std::vector<Thread> values() const;
  const std::vector<std::string>& feature_names() const;
  std::vector<Thread> features() const;
  const Thread& cases() const;
  DataFrame cases(std::int64_t row) const;
  DataFrame cases(std::pair<std::int64_t, std::int64_t> row_span) const;
  std::string cases(std::int64_t row, std::size_t feature_idx) const;
  std::string cases(std::int64_t row, std::string_view feature_name) const;
  pluss::Series cases_series() const;

  DataFrame with_cases(const Thread& cases) const;
  DataFrame& with_cases_(const Thread& cases);

  bool has_column(std::string_view name) const;
  Thread column(std::string_view name) const;
  ColumnRef column_ref(std::string_view name) const;
  bool has_feature(std::string_view name) const;
  Thread feature(std::string_view name) const;
  Thread features(std::string_view name) const;
  ColumnRef feature_ref(std::string_view name) const;
  DataFrame features(std::initializer_list<std::string_view> names) const;

  DataFrame select(std::initializer_list<std::string_view> names) const;
  DataFrame drop(std::initializer_list<std::string_view> names) const;
  DataFrame reorder(std::initializer_list<std::string_view> names) const;
  DataFrame arrange(std::initializer_list<std::string_view> features,
                    bool ascending = true,
                    bool nulls_last = true) const;
  DataFrame arrange(std::initializer_list<ArrangeSpec> specs) const;

  DataFrame with_column(std::string name, const Thread& column) const;
  DataFrame& with_column_(std::string name, const Thread& column);
  DataFrame with_feature(std::string name, const Thread& feature) const;
  DataFrame& with_feature_(std::string name, const Thread& feature);

  DataFrame rename(std::string_view old_name, std::string new_name) const;
  DataFrame& rename_(std::string_view old_name, std::string new_name);

  DataFrame head(std::int64_t n = 5, bool render = true) const;
  DataFrame tail(std::int64_t n = 5, bool render = true) const;
  DataFrame slice(std::int64_t start, std::int64_t stop) const;
  DataFrame take(std::span<const std::int64_t> indices) const;
  DataFrame where(const pluss::Series& bool_mask) const;
  DataFrame filter(const pluss::Series& bool_mask) const;
  DataFrame query(const pluss::Series& bool_mask) const;
  DataFrame query(const QueryFn& predicate) const;
  DataFrame query(std::string_view expression) const;
  DataFrame derive(std::initializer_list<std::pair<std::string, Thread>> assignments) const;
  DataFrame derive(std::initializer_list<DeriveSpec> specs) const;
  DataFrame assign(std::initializer_list<std::pair<std::string, Thread>> assignments) const;
  DataFrame assign(std::initializer_list<DeriveSpec> specs) const;
  DataFrame summarize(std::initializer_list<SummarySpec> specs, bool render = true) const;
  DataFrame agg(std::initializer_list<SummarySpec> specs, bool render = true) const;
  DataFrame add(double other) const;
  DataFrame add(const DataFrame& other) const;
  DataFrame sub(double other) const;
  DataFrame sub(const DataFrame& other) const;
  DataFrame subtract(double other) const;
  DataFrame subtract(const DataFrame& other) const;
  DataFrame mul(double other) const;
  DataFrame mul(const DataFrame& other) const;
  DataFrame multiply(double other) const;
  DataFrame multiply(const DataFrame& other) const;
  DataFrame div(double other) const;
  DataFrame div(const DataFrame& other) const;
  DataFrame divide(double other) const;
  DataFrame divide(const DataFrame& other) const;
  DataFrame truediv(double other) const;
  DataFrame truediv(const DataFrame& other) const;
  DataFrame floordiv(double other) const;
  DataFrame floordiv(const DataFrame& other) const;
  DataFrame mod(double other) const;
  DataFrame mod(const DataFrame& other) const;
  DataFrame pow(double other) const;
  DataFrame pow(const DataFrame& other) const;
  DataFrame radd(double other) const;
  DataFrame rsub(double other) const;
  DataFrame rmul(double other) const;
  DataFrame rdiv(double other) const;
  DataFrame rtruediv(double other) const;
  DataFrame rfloordiv(double other) const;
  DataFrame rmod(double other) const;
  DataFrame rpow(double other) const;
  DataFrame eq(double other) const;
  DataFrame eq(const DataFrame& other) const;
  DataFrame ne(double other) const;
  DataFrame ne(const DataFrame& other) const;
  DataFrame lt(double other) const;
  DataFrame lt(const DataFrame& other) const;
  DataFrame le(double other) const;
  DataFrame le(const DataFrame& other) const;
  DataFrame gt(double other) const;
  DataFrame gt(const DataFrame& other) const;
  DataFrame ge(double other) const;
  DataFrame ge(const DataFrame& other) const;
  DataFrame compare(double other) const;
  DataFrame compare(const DataFrame& other) const;
  DataFrame missing(const MissingSpec& spec) const;
  DataFrame missing(MissingOp op,
                    std::initializer_list<std::string_view> features = {},
                    std::optional<double> value = std::nullopt) const;
  DataFrame cast(std::initializer_list<CastSpec> specs) const;
  DataFrame cast(
      std::initializer_list<std::pair<std::string_view, pluss::codem::ScalarType>> feature_dtypes) const;
  DataFrame astype(
      std::initializer_list<std::pair<std::string_view, pluss::codem::ScalarType>> feature_dtypes) const;
  DataFrame window(std::initializer_list<WindowSpec> specs, bool keep_input = true) const;
  DataFrame rolling(std::initializer_list<WindowSpec> specs, bool keep_input = true) const;

  DataFrame fillna(double value) const;
  DataFrame& fillna_(double value);
  DataFrame dropna() const;
  DataFrame ffill() const;
  DataFrame bfill() const;
  std::int64_t count_null_rows() const;
  DataFrame describe(bool render = true) const;
  DataFrame view(std::int64_t start, std::int64_t stop, bool render = true) const;

  std::string to_string(const display::DisplayOptions& options = {}) const;
  std::string info_string(const display::DisplayOptions& options = {}) const;
  void info(const display::DisplayOptions& options = {}) const;
  void info(std::ostream& os, const display::DisplayOptions& options = {}) const;
  void save_csv(std::string_view filepath) const;
  void save_csv(std::string_view filepath,
                bool add_cases,
                bool header = true,
                char sep = ',',
                std::string na_rep = "NA",
                std::optional<std::string> case_label = std::nullopt,
                std::optional<std::string> float_format = std::nullopt,
                char quotechar = '"',
                bool doublequote = true,
                std::string line_terminator = "\n",
                char decimal = '.',
                std::string encoding = "utf-8") const;
  void save_csv(std::string_view filepath, const pluss::loader::WriteCsvOptions& options) const;
  void save_csv(std::string_view filepath, const pluss::loader::Config& config) const;
  void save_csv(std::string_view filepath,
                std::initializer_list<pluss::loader::ConfigEntry> config_entries) const;
  void save_excel(std::string_view filepath) const;
  void save_excel(std::string_view filepath,
                  std::string sheet_name,
                  bool add_cases = false,
                  bool header = true,
                  std::string na_rep = "NA",
                  std::optional<std::string> case_label = std::nullopt) const;
  void save_excel(std::string_view filepath, const pluss::loader::WriteExcelOptions& options) const;
  void save_excel(std::string_view filepath, const pluss::loader::Config& config) const;
  void save_excel(std::string_view filepath,
                  std::initializer_list<pluss::loader::ConfigEntry> config_entries) const;
  void save_as(std::string_view target_type,
               std::string_view filepath,
               const pluss::loader::Config& config = {}) const;
  void save_as(std::string_view target_type,
               std::string_view filepath,
               std::initializer_list<pluss::loader::ConfigEntry> config_entries) const;

private:
  explicit DataFrame(pluss::msharp::intrusive_ptr<detail::DataFrameImpl> impl);
  detail::DataFrameImpl& mut_impl_();
  const detail::DataFrameImpl& impl_ref_() const;
  void suppress_next_stream_() const;
  bool consume_stream_suppression_() const;

  pluss::msharp::intrusive_ptr<detail::DataFrameImpl> impl_{nullptr};
  friend std::ostream& operator<<(std::ostream& os, const DataFrame& frame);
};

std::ostream& operator<<(std::ostream& os, const DataFrame& frame);

} // namespace pluss::table

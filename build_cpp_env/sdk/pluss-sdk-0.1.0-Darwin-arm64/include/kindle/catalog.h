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
#include "loader/Config.h"
#include "loader/CsvLoader.h"
#include "loader/ExcelLoader.h"
#include "kindle/Stream.h"

namespace pluss::table {
class DataFrame;
}

namespace pluss::kindle {

struct NamedStream {
  std::string_view key{};
  Stream value{};
};

struct KeyToken {
  std::string_view key{};

  NamedStream operator=(const Stream& value) const { return NamedStream{key, value}; }
  NamedStream operator=(Stream&& value) const { return NamedStream{key, std::move(value)}; }
};

namespace literals {
constexpr KeyToken operator""_k(const char* s, std::size_t n) noexcept {
  return KeyToken{std::string_view{s, n}};
}
} // namespace literals

namespace detail {
class CatalogImpl;
}

enum class AlignPolicy : std::uint8_t {
  Strict = 0,
  Inner = 1,
  Outer = 2,
  Left = 3,
  Right = 4,
};

enum class IndexMode : std::uint8_t {
  Positional = 0,
  ByCaseLabel = 1,
};

enum class FillPolicy : std::uint8_t {
  Null = 0,
  Scalar = 1,
  ForwardFill = 2,
  BackwardFill = 3,
};

enum class DTypePolicy : std::uint8_t {
  Strict = 0,
  BestEffort = 1,
};

enum class CollisionPolicy : std::uint8_t {
  Error = 0,
  Suffix = 1,
  Overwrite = 2,
};

struct KindelizePolicy {
  bool attach_cases_to_streams{true};
  bool materialize_case_stream{false};
  std::string case_stream_name{"_case"};
};

struct TabelizePolicy {
  AlignPolicy align{AlignPolicy::Inner};
  IndexMode index_mode{IndexMode::ByCaseLabel};
  FillPolicy fill{FillPolicy::Null};
  std::optional<double> fill_scalar{std::nullopt};
  DTypePolicy dtype{DTypePolicy::BestEffort};
  CollisionPolicy collision{CollisionPolicy::Error};
  std::optional<std::string> anchor{std::nullopt};
  bool include_case_feature{false};
  std::string case_feature_name{"case"};
};

class Catalog {
public:
  using const_iterator = std::vector<std::string>::const_iterator;
  using QueryFn = std::function<pluss::Series(const Catalog&)>;

  Catalog();
  explicit Catalog(std::vector<std::pair<std::string, Stream>> streams);
  Catalog(std::initializer_list<std::pair<std::string, Stream>> streams);
  Catalog(std::initializer_list<NamedStream> streams);
  Catalog(const Catalog& other);
  Catalog(Catalog&& other) noexcept;
  Catalog& operator=(const Catalog& other);
  Catalog& operator=(Catalog&& other) noexcept;
  ~Catalog();

  static Catalog kindelize(const pluss::table::DataFrame& frame,
                           const KindelizePolicy& policy = {});
  static Catalog load(std::string_view source_type,
                      std::string_view filepath);
  static Catalog load(std::string_view source_type,
                      std::string_view filepath,
                      const pluss::loader::Config& config);
  static Catalog load_csv(std::string_view filepath);
  static Catalog load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options);
  static Catalog load_txt(std::string_view filepath);
  static Catalog load_txt(std::string_view filepath, const pluss::loader::Config& config);
  static Catalog load_excel(std::string_view filepath);
  static Catalog load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options);
  static Catalog load_parquet(std::string_view filepath);
  static Catalog load_parquet(std::string_view filepath, const pluss::loader::Config& config);
  static Catalog load_json(std::string_view filepath);
  static Catalog load_json(std::string_view filepath, const pluss::loader::Config& config);
  static Catalog load_csv_with_key(std::string_view filepath,
                                   std::string_view key_field,
                                   const pluss::loader::ReadCsvOptions& options = {});
  static Catalog from_json(std::string_view json_text);

  bool defined() const noexcept;
  bool unique() const noexcept;

  std::int64_t nstreams() const;
  std::int64_t nfields() const;
  bool empty() const;

  const std::vector<std::string>& keys() const;
  bool has(std::string_view key) const;
  bool has_stream(std::string_view key) const;
  Stream stream(std::string_view key) const;
  Stream at(std::string_view key) const;
  std::optional<Stream> get(std::string_view key) const;
  std::optional<Stream> get_stream(std::string_view key) const;
  std::vector<Stream> streams() const;

  // Kind projections.
  Catalog series() const;
  Catalog note() const;
  Catalog timeline() const;
  Catalog timeindex() const;

  // Vectorized stream delegation.
  Catalog head(std::int64_t n) const;
  Catalog tail(std::int64_t n) const;
  Catalog slice(std::int64_t start, std::int64_t stop) const;
  Catalog cloc(std::int64_t row) const;
  Catalog cloc(std::int64_t start, std::int64_t stop) const;
  Catalog take(std::span<const std::int64_t> indices) const;
  Catalog sample(std::int64_t n, std::optional<std::uint64_t> random_state = std::nullopt) const;
  Catalog filter(const pluss::Series& condition) const;
  Catalog where(const pluss::Series& condition) const;
  Catalog query(const QueryFn& fn) const;
  Catalog masked_select(const pluss::Series& mask) const;
  Catalog select(std::initializer_list<std::string_view> field_names) const;
  Catalog select(const std::vector<std::string>& field_names) const;
  Catalog drop(std::string_view field_name) const;
  Catalog drop(const std::vector<std::string>& field_names) const;
  Catalog rename(std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const;
  Catalog rename(std::string_view from, std::string_view to) const;
  Catalog& rename_(std::string_view from, std::string_view to);
  Catalog rename(const std::unordered_map<std::string, std::string>& mapping) const;
  Catalog sort(std::string_view by_field, bool ascending = true) const;
  Catalog sort_by(const std::vector<std::string>& fields,
                  const std::vector<bool>& ascending = {}) const;
  std::unordered_map<std::string, Catalog> group_by(std::string_view key_field) const;
  Catalog fillna(double value, std::initializer_list<std::string_view> keys = {}) const;
  Catalog fillna(std::string_view value, std::initializer_list<std::string_view> keys = {}) const;
  Catalog fillna(const char* value, std::initializer_list<std::string_view> keys = {}) const {
    return fillna(std::string_view{value != nullptr ? value : ""}, keys);
  }
  Catalog fillna(const pluss::Timeline& value, std::initializer_list<std::string_view> keys = {}) const;
  Catalog fillna(const pluss::TimeIndex& value, std::initializer_list<std::string_view> keys = {}) const;
  Catalog dropna(std::initializer_list<std::string_view> keys = {}) const;
  Catalog ffill(std::initializer_list<std::string_view> keys = {}) const;
  Catalog bfill(std::initializer_list<std::string_view> keys = {}) const;
  Catalog isnull(std::initializer_list<std::string_view> keys = {}) const;
  Catalog notnull(std::initializer_list<std::string_view> keys = {}) const;
  Catalog cast_series(std::initializer_list<std::string_view> keys,
                      std::optional<pluss::codem::ScalarType> dtype = std::nullopt) const;
  Catalog cast_note(std::initializer_list<std::string_view> keys) const;
  Catalog cast_timeline(std::initializer_list<std::string_view> keys) const;
  Catalog cast_timeindex(std::initializer_list<std::string_view> keys) const;
  Catalog astype(std::initializer_list<std::string_view> keys,
                 pluss::codem::ScalarType dtype) const;
  Stream sum(std::string_view field) const;
  Stream mean(std::string_view field) const;
  Stream min(std::string_view field) const;
  Stream max(std::string_view field) const;
  std::int64_t count() const;
  Catalog merge(const Catalog& other, std::string_view on, std::string_view how = "inner") const;
  Catalog apply(std::string_view field, const std::function<Stream(const Stream&)>& func) const;
  Catalog& apply_(std::string_view field, const std::function<Stream(const Stream&)>& func);
  Catalog apply_all(const std::function<Stream(const Stream&)>& fn) const;
  Catalog& apply_all_(const std::function<Stream(const Stream&)>& fn);
  Catalog transform(const std::function<Stream(const Stream&)>& fn) const;
  Catalog transform(std::string_view field, const std::function<Stream(const Stream&)>& fn) const;
  Catalog text_preprocess(std::string_view field) const;
  Catalog extract_keywords(std::string_view field, int top_n = 10) const;
  Catalog compute_similarity(std::string_view field1,
                             std::string_view field2,
                             std::string_view method = "cosine") const;
  Catalog filter_by_text(std::string_view field, std::string_view pattern) const;
  Catalog add_text_features(std::string_view field) const;
  Catalog aggregate_text_stats(std::string_view field, std::string_view stat_type) const;
  Catalog tfidf(std::string_view field) const;

  Catalog set_stream(std::string_view key, const Stream& value) const;
  Catalog& set_stream_(std::string_view key, const Stream& value);
  Catalog set(std::string_view key, const Stream& value) const;
  Catalog& set_(std::string_view key, const Stream& value);
  Catalog set_streams(std::initializer_list<std::pair<std::string_view, Stream>> items) const;
  Catalog& set_streams_(std::initializer_list<std::pair<std::string_view, Stream>> items);
  Catalog set_streams(std::initializer_list<NamedStream> items) const;
  Catalog& set_streams_(std::initializer_list<NamedStream> items);
  Catalog drop(std::initializer_list<std::string_view> keys) const;

  pluss::table::DataFrame tabelize(const TabelizePolicy& policy = {}) const;
  pluss::table::DataFrame to_table(const TabelizePolicy& policy = {}, bool render = true) const;

  void display() const;
  void info() const;
  void save_csv(std::string_view filename) const;
  void save_csv(std::string_view filename, const pluss::loader::Config& config) const;
  void save_txt(std::string_view filename) const;
  void save_txt(std::string_view filename, const pluss::loader::Config& config) const;
  void save_excel(std::string_view filename) const;
  void save_excel(std::string_view filename, const pluss::loader::Config& config) const;
  void save_json(std::string_view filename) const;
  void save_json(std::string_view filename, const pluss::loader::Config& config) const;
  void save_parquet(std::string_view filename) const;
  void save_parquet(std::string_view filename, const pluss::loader::Config& config) const;
  void save_as(std::string_view target_type,
               std::string_view filename,
               const pluss::loader::Config& config = {}) const;
  std::string to_string() const;
  std::string to_json() const;
  void show(std::ostream& os) const;
  const_iterator begin() const;
  const_iterator end() const;

private:
  explicit Catalog(pluss::msharp::intrusive_ptr<detail::CatalogImpl> impl);
  const detail::CatalogImpl& impl_ref_() const;
  detail::CatalogImpl& mut_impl_();

  pluss::msharp::intrusive_ptr<detail::CatalogImpl> impl_{nullptr};
};

std::ostream& operator<<(std::ostream& os, const Catalog& catalog);

} // namespace pluss::kindle

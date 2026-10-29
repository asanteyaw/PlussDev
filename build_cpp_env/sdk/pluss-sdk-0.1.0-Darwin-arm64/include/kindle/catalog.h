#pragma once

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iosfwd>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/StringViewNameRange.h"
#include "kindle/Join.h"
#include "kindle/Ragged.h"
#include "MSharp/IntrusivePtr.h"
#include "MSharp/RangeView.h"
#include "loader/Config.h"
#include "loader/CsvLoader.h"
#include "loader/ExcelLoader.h"
#include "kindle/Stream.h"

namespace pluss::table {
class DataFrame;
}
namespace pluss::scribble {
class Registry;
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

class CatalogKeyAccessor;
class CatalogKeyAxisAccessor;
class CatalogCastAccessor;
class CatalogNoteAccessor;
class CatalogTimelineAccessor;
class CatalogTimeSeriesAccessor;
class CatalogPlotAccessor;
class CatalogPlot3DAccessor;
class CatalogGroupBuilder;

enum class Align : std::uint8_t {
  Strict = 0,
  Inner = 1,
  Outer = 2,
  Left = 3,
  Right = 4,
};

enum class AlignBy : std::uint8_t {
  Pos = 0,
  Label = 1,
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

enum class ReduceAxis : std::uint8_t {
  Record = 0,
  Key = 1,
};

enum class PairAlign : std::uint8_t {
  PairwiseInner = 0,
  GlobalInner = 1,
};

enum class PairAlignBy : std::uint8_t {
  Auto = 0,
  Label = 1,
  Pos = 2,
};

// Reductions across keys (ReduceAxis::Key) on ragged streams. Defaults, which
// run without padding: position i is reduced over the streams that have a
// value there (a finished stream contributes nothing), the result is as long
// as the longest stream, streams line up from their start, and in-range nulls
// are skipped. Each choice has an explicit switch:
//   what a finished stream means  ended      Skip (default) | Null: result is null wherever
//                                            any stream has finished
//   how many streams must report  min_streams  1 (default): null where fewer contribute
//   where streams line up         anchor     Head (default) | Tail: align the last values
//   in-range nulls                skipnull   true (default) | false: a null nulls its position
//   result length / lengths       align      Outer (default) | Inner: trim to the shortest |
//                                            Strict: error on unequal lengths
//   pad instead of jag            fill_scalar / align_by = Label: fall back to padding
// ended, anchor and min_streams apply only to the default Outer/Pos/no-fill
// path and are rejected (not silently ignored) elsewhere.
enum class RaggedEnd : std::uint8_t {
  Skip = 0,
  Null = 1,
};

enum class RaggedAnchor : std::uint8_t {
  Head = 0,
  Tail = 1,
};

struct CatalogReduceOptions {
  ReduceAxis axis{ReduceAxis::Record};
  Align align{Align::Outer};
  AlignBy align_by{AlignBy::Pos};
  bool skipnull{true};
  std::optional<double> fill_scalar{std::nullopt};
  bool unbiased{true};
  RaggedEnd ended{RaggedEnd::Skip};
  RaggedAnchor anchor{RaggedAnchor::Head};
  std::int64_t min_streams{1};
};

struct KindelizePolicy {
  bool attach_labels{true};
  bool materialize_label{false};
  std::string label_key{"_label"};
};

// Tabelize feeds rectangular consumers (DataFrame, SQL, wide export), so by
// default it pads: every record of the longest stream is kept and missing
// ones become nulls. (It used to inner-join by label, which dropped ragged
// records silently; ask for Align::Inner explicitly if that is what you want.)
struct TabelizePolicy {
  Align align{Align::Outer};
  AlignBy align_by{AlignBy::Label};
  FillPolicy fill{FillPolicy::Null};
  std::optional<double> fill_scalar{std::nullopt};
  DTypePolicy dtype{DTypePolicy::BestEffort};
  CollisionPolicy collision{CollisionPolicy::Error};
  std::optional<std::string> anchor{std::nullopt};
  bool include_label_feature{false};
  std::string label_feature{"label"};
};

struct CatalogPairAlignOptions {
  PairAlign align{PairAlign::PairwiseInner};
  PairAlignBy align_by{PairAlignBy::Auto};
};

struct CatalogScatterPlotOptions {
  CatalogPairAlignOptions align{};
  pluss::table::ScatterPlotOptions scatter{};
};

struct CatalogScatterMatrixPlotOptions {
  CatalogPairAlignOptions align{};
  pluss::table::ScatterMatrixPlotOptions matrix{};
};

struct CatalogCorrHeatmapPlotOptions {
  CatalogPairAlignOptions align{};
  pluss::table::CorrHeatmapPlotOptions heatmap{};
  pluss::table::DataFrame::CorrMethod method{pluss::table::DataFrame::CorrMethod::Pearson};
};

// CatalogPlot3DAccessor's scatter/line/hist reconcile all of their requested
// keys together as one group (not a matrix of independent pairs), so unlike
// CatalogPairAlignOptions there is no PairwiseInner/GlobalInner axis to
// expose here -- align_by is the only knob that applies.
struct CatalogScatter3DPlotOptions {
  pluss::table::XYAlignMode align{pluss::table::XYAlignMode::Inner};
  pluss::table::XYAlignBy align_by{pluss::table::XYAlignBy::Auto};
  double pad_scalar{0.0};
  pluss::table::Scatter3DPlotOptions scatter{};
};

struct CatalogLine3DPlotOptions {
  pluss::table::XYAlignMode align{pluss::table::XYAlignMode::Inner};
  pluss::table::XYAlignBy align_by{pluss::table::XYAlignBy::Auto};
  double pad_scalar{0.0};
  pluss::table::Line3DPlotOptions line{};
};

struct CatalogHist3DPlotOptions {
  pluss::table::XYAlignMode align{pluss::table::XYAlignMode::Inner};
  pluss::table::XYAlignBy align_by{pluss::table::XYAlignBy::Auto};
  double pad_scalar{0.0};
  pluss::table::Hist3DPlotOptions hist{};
};

class Catalog {
public:
  using const_iterator = std::vector<std::string>::const_iterator;
  using QueryFn = std::function<pluss::Series(const Catalog&)>;

  Catalog();
  // Opaque handle identity: equal for copies of the same handle, which share
  // contents until one of them is edited (edits clone). Valid as a cache key
  // only while a copy of the handle is kept alive.
  const void* identity() const noexcept;
  // Content fingerprint: key names plus every stream's fingerprint. Equal while
  // nothing changed (including in-place edits of shared Series storage); valid
  // as a cache key only while a copy of the catalog is kept alive.
  std::uint64_t fingerprint() const;
  explicit Catalog(std::vector<std::pair<std::string, Stream>> streams);
  Catalog(std::initializer_list<std::pair<std::string, Stream>> streams);
  Catalog(std::initializer_list<NamedStream> streams);
  Catalog(const Catalog& other);
  Catalog(Catalog&& other) noexcept;
  Catalog& operator=(const Catalog& other);
  Catalog& operator=(Catalog&& other) noexcept;
  ~Catalog();

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
  static Catalog load_labeled(std::string_view filepath,
                              std::string_view label_field,
                              const pluss::loader::ReadCsvOptions& options = {});
  static Catalog from_json(std::string_view json_text);

  bool defined() const noexcept;
  bool unique() const noexcept;

  std::int64_t nkeys() const;
  std::int64_t nrows() const;
  std::int64_t minrows() const;
  std::int64_t maxrows() const;
  std::int64_t size() const;
  std::pair<std::int64_t, std::int64_t> shape() const;
  bool empty() const;

  const std::vector<std::string>& keys() const;
  Stream keys(std::string_view key) const;
  Catalog keys(std::span<const std::string_view> field_names) const;
  Catalog keys(std::initializer_list<std::string_view> field_names) const;
  template <pluss::detail::StringViewNameRange NameRange>
  Catalog keys(const NameRange& field_names) const;
  CatalogKeyAxisAccessor keyaxis() const;
  CatalogGroupBuilder group() const;
  bool has(std::string_view key) const;
  std::optional<Stream> get(std::string_view key) const;

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
  static Catalog select(pluss::msharp::RangeView::Of<pluss::table::Slate> conditions,
                        pluss::msharp::RangeView::Of<Catalog> choices,
                        const Catalog& default_value);
  static Catalog select(std::initializer_list<pluss::table::Slate> conditions,
                        std::initializer_list<Catalog> choices,
                        const Catalog& default_value) {
    return select(
        pluss::msharp::RangeView::Of<pluss::table::Slate>(conditions),
        pluss::msharp::RangeView::Of<Catalog>(choices),
        default_value);
  }
  Catalog drop(std::string_view field_name) const;
  Catalog drop(std::span<const std::string_view> field_names) const;
  Catalog drop(const std::vector<std::string>& field_names) const;
  template <pluss::detail::StringViewNameRange NameRange>
  Catalog drop(const NameRange& field_names) const {
    return pluss::detail::with_name_span(field_names, [&](auto names) { return drop(names); });
  }
  Catalog rename(std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const;
  Catalog rename(std::string_view from, std::string_view to) const;
  Catalog& rename_(std::string_view from, std::string_view to);
  Catalog rename(const std::unordered_map<std::string, std::string>& mapping) const;
  Catalog sort(std::string_view by_field, bool ascending = true) const;
  Catalog sort(std::span<const std::string_view> fields,
               const std::vector<bool>& ascending = {}) const;
  Catalog sort(std::initializer_list<std::string_view> fields,
               const std::vector<bool>& ascending = {}) const;
  Catalog sort(const std::vector<std::string>& fields,
               const std::vector<bool>& ascending = {}) const;
  template <pluss::detail::StringViewNameRange NameRange>
  Catalog sort(const NameRange& fields,
               const std::vector<bool>& ascending = {}) const {
    return pluss::detail::with_name_span(fields, [&](auto names) { return sort(names, ascending); });
  }
  Catalog fillnull(double value) const;
  Catalog fillnull(std::string_view value) const;
  Catalog fillnull(const char* value) const {
    return fillnull(std::string_view{value != nullptr ? value : ""});
  }
  Catalog fillnull(const pluss::Timeline& value) const;
  Catalog fillnull(const pluss::Duration& value) const;
  Catalog dropnull() const;
  Catalog ffill() const;
  Catalog bfill() const;
  Catalog isnull() const;
  Catalog notnull() const;
  Catalog invalid() const;
  CatalogCastAccessor cast() const;
  CatalogCastAccessor cast(const pluss::table::SlateCastOptions& options) const;
  Catalog cast(pluss::table::SlateCastTarget target,
               const pluss::table::SlateCastOptions& options = {}) const;
  CatalogNoteAccessor nt() const;
  CatalogTimelineAccessor tl() const;
  CatalogTimeSeriesAccessor ts() const;
  CatalogPlotAccessor plot() const;
  CatalogPlotAccessor plot(const pluss::table::DataFramePlotOptions& options) const;
  CatalogPlot3DAccessor plot3d() const;
  CatalogPlot3DAccessor plot3d(const pluss::table::DataFramePlotOptions& options) const;
  Catalog astype(pluss::codem::ScalarType dtype) const;
  Catalog distinct(DistinctOptions options = {}) const;
  Catalog freq(FreqOptions options = {}) const;
  Catalog ndistinct(DistinctOptions options = {}) const;
  // Several cross-key reductions in one pass over the data (columns loaded
  // once, one sweep). ops: "sum", "mean", "min", "max", "var", "std", "count"
  // ("count" = contributing streams per index). The result has one stream per op,
  // named after it, in the order given. Always across keys (options.axis is ignored); the policy knobs
  // in `options` apply to every op, exactly as for the single reductions.
  Catalog reduce(std::initializer_list<std::string_view> ops, const CatalogReduceOptions& options = {}) const;
  Catalog reduce(const std::vector<std::string>& ops, const CatalogReduceOptions& options = {}) const;
  // A new stream `lhs op rhs` (see ragged_binary) added under out_key (default
  // "<lhs><op><rhs>"); ragged operands follow the policy, nothing is dropped by default.
  Catalog combine(std::string_view lhs, std::string_view rhs, std::string_view op, std::string_view out_key = {},
                  const RaggedPolicy& policy = RaggedPolicy::defaults(OpClass::Elementwise)) const;
  Catalog sum() const;
  Catalog sum(ReduceAxis axis) const;
  Catalog sum(const CatalogReduceOptions& options) const;
  Catalog mean() const;
  Catalog mean(ReduceAxis axis) const;
  Catalog mean(const CatalogReduceOptions& options) const;
  Catalog min() const;
  Catalog min(ReduceAxis axis) const;
  Catalog min(const CatalogReduceOptions& options) const;
  Catalog max() const;
  Catalog max(ReduceAxis axis) const;
  Catalog max(const CatalogReduceOptions& options) const;
  Catalog var() const;
  Catalog var(ReduceAxis axis) const;
  Catalog var(const CatalogReduceOptions& options) const;
  Catalog std() const;
  Catalog std(ReduceAxis axis) const;
  Catalog std(const CatalogReduceOptions& options) const;
  // merge keeps every record the policy keeps; how = inner | left | right | outer.
  Catalog merge(const Catalog& other, std::string_view on, std::string_view how = "inner") const;
  // Relational join of two catalogs: match on the key streams alone, then rebuild
  // every stream through the matched pairs, so ragged catalogs stay ragged.
  Catalog join(const Catalog& other, const CatalogJoinSpec& spec, const CatalogJoinOptions& options = {}) const;
  CatalogJoinResult join_detailed(const Catalog& other, const CatalogJoinSpec& spec,
                                  const CatalogJoinOptions& options = {}) const;
  // Just the matching half of a join: which records of this catalog pair with
  // which records of `other` on the given keys (record r of a side is stream
  // position r + layout.shift_of(stream)). Pull and traversal compose these.
  CatalogMatch match_keys(const Catalog& other, const std::vector<std::string>& left_keys,
                          const std::vector<std::string>& right_keys, relational::JoinHow how,
                          std::optional<relational::RelationshipCardinality> validate = std::nullopt,
                          const RaggedPolicy& ragged = RaggedPolicy::defaults(OpClass::Relational)) const;
  // Joins reuse the hashed right-hand side of a recent join on the same data and
  // keys (non-text keys, large sides). (hits, misses) since start; clear drops it.
  static std::pair<std::int64_t, std::int64_t> join_cache_stats();
  static void clear_join_cache();
  CatalogKeyAudit audit_keys(const Catalog& other, const std::vector<std::string>& left_keys,
                             const std::vector<std::string>& right_keys,
                             const RaggedPolicy& ragged = RaggedPolicy::defaults(OpClass::Relational)) const;
  Catalog map(const std::function<Stream(const Stream&)>& fn) const;
  Catalog& map_(const std::function<Stream(const Stream&)>& fn);
  Catalog transform(const std::function<Stream(const Stream&)>& fn) const;
  Catalog clean(std::string_view field) const;
  Catalog keywords(std::string_view field, int top_n = 10) const;
  Catalog similarity(std::string_view field1,
                     std::string_view field2,
                     std::string_view method = "cosine") const;
  Catalog grep(std::string_view field, std::string_view pattern) const;
  Catalog featurize(std::string_view field) const;
  Catalog stats(std::string_view field, std::string_view stat_type) const;
  Catalog tfidf(std::string_view field) const;

  Catalog set(std::string_view key, const Stream& value) const;
  Catalog& set_(std::string_view key, const Stream& value);
  Catalog set(std::initializer_list<std::pair<std::string_view, Stream>> items) const;
  Catalog& set_(std::initializer_list<std::pair<std::string_view, Stream>> items);
  Catalog set(std::initializer_list<NamedStream> items) const;
  Catalog& set_(std::initializer_list<NamedStream> items);
  Catalog drop(std::initializer_list<std::string_view> keys) const;

  pluss::table::DataFrame tabelize(const TabelizePolicy& policy = {}) const;
  // The same, decided by a RaggedPolicy (Rectangular defaults): enforces
  // domain = Exact and loss = Error before padding.
  pluss::table::DataFrame tabelize(const RaggedPolicy& policy) const;
  pluss::scribble::Registry scribbelize() const;

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
  // Ragged-native persistence: one file per stream (with its label as a second
  // column) plus a streams.csv manifest. store_kind: "parquet" (default), "csv", "json".
  // Lengths, nulls, kinds and labels round-trip exactly; nothing is padded.
  void save_streams(std::string_view directory, std::string_view store_kind = "parquet") const;
  static Catalog load_streams(std::string_view directory);
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
  // Retired from the public surface -- Catalog::keys(std::string_view) is
  // the public single-key lookup now; this stays as its implementation.
  Stream at(std::string_view key) const;
  std::int64_t nstreams() const;
  std::int64_t count() const;
  std::vector<Stream> streams() const;
  Catalog set_stream(std::string_view key, const Stream& value) const;
  Catalog& set_stream_(std::string_view key, const Stream& value);
  const detail::CatalogImpl& impl_ref_() const;
  detail::CatalogImpl& mut_impl_();

  pluss::msharp::intrusive_ptr<detail::CatalogImpl> impl_{nullptr};
};

struct CatalogMatch {
  relational::MatchResult pairs{};
  RaggedLayout left{};
  RaggedLayout right{};
};

struct CatalogJoinResult {
  Catalog catalog{};
  CatalogJoinReport report{};
};

class CatalogKeyAccessor {
public:
  CatalogKeyAccessor(Catalog base, std::vector<std::string> keys);

  CatalogCastAccessor cast() const;
  CatalogCastAccessor cast(const pluss::table::SlateCastOptions& options) const;
  CatalogNoteAccessor nt() const;
  CatalogTimelineAccessor tl() const;
  CatalogTimeSeriesAccessor ts() const;

  Catalog fillnull(double value) const;
  Catalog fillnull(std::string_view value) const;
  Catalog fillnull(const char* value) const {
    return fillnull(std::string_view{value != nullptr ? value : ""});
  }
  Catalog fillnull(const pluss::Timeline& value) const;
  Catalog fillnull(const pluss::Duration& value) const;
  Catalog dropnull() const;
  Catalog ffill() const;
  Catalog bfill() const;
  Catalog isnull() const;
  Catalog notnull() const;
  Catalog invalid() const;
  Catalog distinct(DistinctOptions options = {}) const;
  Catalog freq(FreqOptions options = {}) const;
  Catalog ndistinct(DistinctOptions options = {}) const;
  Catalog astype(pluss::codem::ScalarType dtype) const;
  Catalog map(const std::function<Stream(const Stream&)>& fn) const;

private:
  template <class Fn>
  Catalog map_selected_(Fn&& fn) const;

  Catalog base_{};
  std::vector<std::string> keys_{};

  friend class CatalogCastAccessor;
  friend class CatalogKeyAxisAccessor;
  friend class CatalogNoteAccessor;
  friend class CatalogTimelineAccessor;
  friend class CatalogTimeSeriesAccessor;
};

class CatalogKeyAxisAccessor {
public:
  explicit CatalogKeyAxisAccessor(Catalog base);

  Catalog like(std::string_view needle, bool case_sensitive = true) const;
  Catalog match(std::string_view pattern) const;
  Catalog series() const;
  Catalog note() const;
  Catalog timeline() const;
  Catalog duration() const;
  Catalog prefix(std::string_view prefix) const;
  Catalog suffix(std::string_view suffix) const;

private:
  Catalog base_{};
};

class CatalogCastAccessor {
public:
  CatalogCastAccessor(Catalog base,
                      std::vector<std::string> keys,
                      pluss::table::SlateCastOptions options = {});

  Catalog series() const;
  Catalog note() const;
  Catalog timeline() const;
  Catalog duration() const;

private:
  CatalogKeyAccessor keysel_;
  pluss::table::SlateCastOptions options_{};
};

class CatalogNoteAccessor {
public:
  CatalogNoteAccessor(Catalog base, std::vector<std::string> keys);

  Catalog strip() const;
  Catalog strip(std::string_view chars) const;
  Catalog lstrip() const;
  Catalog lstrip(std::string_view chars) const;
  Catalog rstrip() const;
  Catalog rstrip(std::string_view chars) const;
  Catalog lower() const;
  Catalog upper() const;
  Catalog casefold() const;
  Catalog titlecase() const;
  Catalog length() const;
  Catalog starts(std::string_view prefix) const;
  Catalog ends(std::string_view suffix) const;
  Catalog has(std::string_view needle,
              const pluss::table::TextOptions& options = {}) const;
  Catalog count(std::string_view pattern) const;
  Catalog count(std::string_view pattern,
                const pluss::table::TextOptions& options) const;
  Catalog matches(std::string_view pattern, bool full = false) const;
  Catalog fullmatch(std::string_view pattern) const;
  std::unordered_map<std::string, pluss::table::DataFrame> findall(std::string_view pattern) const;
  std::unordered_map<std::string, pluss::table::DataFrame> findall(
      std::string_view pattern,
      const pluss::table::TextOptions& options) const;
  std::unordered_map<std::string, pluss::table::DataFrame> split(
      std::string_view delimiter,
      std::int64_t limit = -1,
      std::string prefix = "part") const;
  std::unordered_map<std::string, pluss::table::DataFrame> split(
      std::string_view pattern,
      const pluss::table::TextOptions& options) const;
  std::unordered_map<std::string, pluss::table::DataFrame> partition(
      std::string_view separator,
      std::string prefix = "part") const;
  Catalog pad(std::int64_t width,
              pluss::table::TextAlign align = pluss::table::TextAlign::Left,
              char fill = ' ') const;
  Catalog chars(std::int64_t start,
                std::optional<std::int64_t> stop = std::nullopt,
                std::int64_t step = 1) const;
  Catalog normalize(pluss::table::UnicodeForm form = pluss::table::UnicodeForm::NFC) const;
  Catalog encode(std::string_view encoding = "base64") const;
  std::unordered_map<std::string, pluss::table::DataFrame> indicate(
      std::string_view delimiter = "|",
      std::string prefix = "") const;
  Catalog replace(std::string_view from, std::string_view to) const;
  Catalog replace(std::string_view from,
                  std::string_view to,
                  const pluss::table::TextOptions& options) const;

private:
  CatalogKeyAccessor keysel_;
};

class CatalogTimelineAccessor {
public:
  CatalogTimelineAccessor(Catalog base, std::vector<std::string> keys);

  Catalog isnat() const;
  Catalog valid() const;
  Catalog year() const;
  Catalog month() const;
  Catalog day() const;
  Catalog hour() const;
  Catalog minute() const;
  Catalog second() const;
  Catalog weekday() const;
  Catalog yearday() const;
  Catalog quarter() const;
  Catalog week() const;
  Catalog weekend() const;
  Catalog month_start() const;
  Catalog month_end() const;
  Catalog leap() const;
  Catalog weekday_name() const;
  Catalog month_name() const;
  Catalog fmt(std::string_view pattern) const;
  Catalog iso() const;
  Catalog epoch(pluss::table::TimeUnit unit = pluss::table::TimeUnit::Nanos) const;
  Catalog floor(std::string_view unit) const;
  Catalog ceil(std::string_view unit) const;
  Catalog round(std::string_view unit) const;
  Catalog date() const;
  Catalog time() const;
  Catalog shift(std::int64_t periods, std::string_view freq) const;
  Catalog period(std::string_view freq) const;
  Catalog tz(std::string_view name) const;
  Catalog localize(const pluss::table::TimelineLocalizeOptions& options = {}) const;
  Catalog utc() const;

private:
  CatalogKeyAccessor keysel_;
};

class CatalogTimeSeriesAccessor {
public:
  CatalogTimeSeriesAccessor(Catalog base, std::vector<std::string> keys);

  Catalog returns(std::int64_t periods = 1) const;
  Catalog log_returns(std::int64_t periods = 1) const;
  Catalog diff(std::int64_t periods = 1) const;
  Catalog lag(std::int64_t periods = 1) const;
  Catalog lead(std::int64_t periods = 1) const;
  Catalog rolling(std::int64_t window,
                  pluss::table::WindowStatistic statistic = pluss::table::WindowStatistic::Mean,
                  const pluss::table::WindowOptions& options = {}) const;

private:
  CatalogKeyAccessor keysel_;
};

class CatalogPlotAccessor {
public:
  explicit CatalogPlotAccessor(Catalog base, pluss::table::DataFramePlotOptions options = {});

  pluss::pl::Studio line(std::string_view y_key,
                         std::optional<std::string_view> x_key = std::nullopt) const;
  pluss::pl::Studio line(pluss::msharp::RangeView::Of<std::string_view> y_keys = {},
                         std::optional<std::string_view> x_key = std::nullopt) const;
  pluss::pl::Studio line(std::initializer_list<std::string_view> y_keys,
                         std::optional<std::string_view> x_key = std::nullopt) const {
    return line(pluss::msharp::RangeView::Of<std::string_view>(y_keys), x_key);
  }
  pluss::pl::Studio line(std::string_view y_key,
                         std::optional<std::string_view> x_key,
                         const pluss::table::LinePlotOptions& options) const;
  pluss::pl::Studio line(pluss::msharp::RangeView::Of<std::string_view> y_keys,
                         std::optional<std::string_view> x_key,
                         const pluss::table::LinePlotOptions& options) const;
  pluss::pl::Studio line(std::initializer_list<std::string_view> y_keys,
                         std::optional<std::string_view> x_key,
                         const pluss::table::LinePlotOptions& options) const {
    return line(pluss::msharp::RangeView::Of<std::string_view>(y_keys), x_key, options);
  }

  pluss::pl::Studio step(std::string_view y_key,
                         std::optional<std::string_view> x_key = std::nullopt) const;
  pluss::pl::Studio step(pluss::msharp::RangeView::Of<std::string_view> y_keys = {},
                         std::optional<std::string_view> x_key = std::nullopt) const;
  pluss::pl::Studio step(std::initializer_list<std::string_view> y_keys,
                         std::optional<std::string_view> x_key = std::nullopt) const {
    return step(pluss::msharp::RangeView::Of<std::string_view>(y_keys), x_key);
  }
  pluss::pl::Studio step(std::string_view y_key,
                         std::optional<std::string_view> x_key,
                         const pluss::table::StepPlotOptions& options) const;
  pluss::pl::Studio step(pluss::msharp::RangeView::Of<std::string_view> y_keys,
                         std::optional<std::string_view> x_key,
                         const pluss::table::StepPlotOptions& options) const;
  pluss::pl::Studio step(std::initializer_list<std::string_view> y_keys,
                         std::optional<std::string_view> x_key,
                         const pluss::table::StepPlotOptions& options) const {
    return step(pluss::msharp::RangeView::Of<std::string_view>(y_keys), x_key, options);
  }

  pluss::pl::Studio area(std::string_view y_key,
                         std::optional<std::string_view> x_key = std::nullopt) const;
  pluss::pl::Studio area(pluss::msharp::RangeView::Of<std::string_view> y_keys = {},
                         std::optional<std::string_view> x_key = std::nullopt) const;
  pluss::pl::Studio area(std::initializer_list<std::string_view> y_keys,
                         std::optional<std::string_view> x_key = std::nullopt) const {
    return area(pluss::msharp::RangeView::Of<std::string_view>(y_keys), x_key);
  }
  pluss::pl::Studio area(std::string_view y_key,
                         std::optional<std::string_view> x_key,
                         const pluss::table::AreaPlotOptions& options) const;
  pluss::pl::Studio area(pluss::msharp::RangeView::Of<std::string_view> y_keys,
                         std::optional<std::string_view> x_key,
                         const pluss::table::AreaPlotOptions& options) const;
  pluss::pl::Studio area(std::initializer_list<std::string_view> y_keys,
                         std::optional<std::string_view> x_key,
                         const pluss::table::AreaPlotOptions& options) const {
    return area(pluss::msharp::RangeView::Of<std::string_view>(y_keys), x_key, options);
  }

  pluss::pl::Studio scatter(std::string_view x_key,
                            std::string_view y_key,
                            std::optional<std::string_view> hue_key = std::nullopt) const;
  pluss::pl::Studio scatter(std::string_view x_key,
                            std::string_view y_key,
                            std::optional<std::string_view> hue_key,
                            const pluss::table::ScatterPlotOptions& options) const;
  pluss::pl::Studio scatter(std::string_view x_key,
                            std::string_view y_key,
                            std::optional<std::string_view> hue_key,
                            const CatalogScatterPlotOptions& options) const;

  pluss::pl::Studio bar(std::string_view y_key,
                        std::optional<std::string_view> x_key = std::nullopt) const;
  pluss::pl::Studio bar(pluss::msharp::RangeView::Of<std::string_view> y_keys = {},
                        std::optional<std::string_view> x_key = std::nullopt) const;
  pluss::pl::Studio bar(std::initializer_list<std::string_view> y_keys,
                        std::optional<std::string_view> x_key = std::nullopt) const {
    return bar(pluss::msharp::RangeView::Of<std::string_view>(y_keys), x_key);
  }
  pluss::pl::Studio bar(std::string_view y_key,
                        std::optional<std::string_view> x_key,
                        const pluss::table::BarPlotOptions& options) const;
  pluss::pl::Studio bar(pluss::msharp::RangeView::Of<std::string_view> y_keys,
                        std::optional<std::string_view> x_key,
                        const pluss::table::BarPlotOptions& options) const;
  pluss::pl::Studio bar(std::initializer_list<std::string_view> y_keys,
                        std::optional<std::string_view> x_key,
                        const pluss::table::BarPlotOptions& options) const {
    return bar(pluss::msharp::RangeView::Of<std::string_view>(y_keys), x_key, options);
  }

  pluss::pl::Studio hist(std::string_view key, std::int64_t bins = 20) const;
  pluss::pl::Studio hist(pluss::msharp::RangeView::Of<std::string_view> keys = {},
                         std::int64_t bins = 20) const;
  pluss::pl::Studio hist(std::initializer_list<std::string_view> keys,
                         std::int64_t bins = 20) const {
    return hist(pluss::msharp::RangeView::Of<std::string_view>(keys), bins);
  }
  pluss::pl::Studio hist(std::string_view key, const pluss::table::HistPlotOptions& options) const;
  pluss::pl::Studio hist(pluss::msharp::RangeView::Of<std::string_view> keys,
                         const pluss::table::HistPlotOptions& options) const;
  pluss::pl::Studio hist(std::initializer_list<std::string_view> keys,
                         const pluss::table::HistPlotOptions& options) const {
    return hist(pluss::msharp::RangeView::Of<std::string_view>(keys), options);
  }

  pluss::pl::Studio density(std::string_view key, std::int64_t points = 128) const;
  pluss::pl::Studio density(pluss::msharp::RangeView::Of<std::string_view> keys = {},
                            std::int64_t points = 128) const;
  pluss::pl::Studio density(std::initializer_list<std::string_view> keys,
                            std::int64_t points = 128) const {
    return density(pluss::msharp::RangeView::Of<std::string_view>(keys), points);
  }
  pluss::pl::Studio density(std::string_view key, const pluss::table::DensityPlotOptions& options) const;
  pluss::pl::Studio density(pluss::msharp::RangeView::Of<std::string_view> keys,
                            const pluss::table::DensityPlotOptions& options) const;
  pluss::pl::Studio density(std::initializer_list<std::string_view> keys,
                            const pluss::table::DensityPlotOptions& options) const {
    return density(pluss::msharp::RangeView::Of<std::string_view>(keys), options);
  }

  pluss::pl::Studio corr_heatmap(pluss::msharp::RangeView::Of<std::string_view> keys = {}) const;
  pluss::pl::Studio corr_heatmap(std::initializer_list<std::string_view> keys) const {
    return corr_heatmap(pluss::msharp::RangeView::Of<std::string_view>(keys));
  }
  pluss::pl::Studio corr_heatmap(pluss::msharp::RangeView::Of<std::string_view> keys,
                                 const CatalogCorrHeatmapPlotOptions& options) const;
  pluss::pl::Studio corr_heatmap(std::initializer_list<std::string_view> keys,
                                 const CatalogCorrHeatmapPlotOptions& options) const {
    return corr_heatmap(pluss::msharp::RangeView::Of<std::string_view>(keys), options);
  }

  pluss::pl::Studio scatter_matrix(pluss::msharp::RangeView::Of<std::string_view> keys = {}) const;
  pluss::pl::Studio scatter_matrix(std::initializer_list<std::string_view> keys) const {
    return scatter_matrix(pluss::msharp::RangeView::Of<std::string_view>(keys));
  }
  pluss::pl::Studio scatter_matrix(pluss::msharp::RangeView::Of<std::string_view> keys,
                                   pluss::table::ScatterMatrixTriangle triangle) const;
  pluss::pl::Studio scatter_matrix(std::initializer_list<std::string_view> keys,
                                   pluss::table::ScatterMatrixTriangle triangle) const {
    return scatter_matrix(pluss::msharp::RangeView::Of<std::string_view>(keys), triangle);
  }
  pluss::pl::Studio scatter_matrix(pluss::msharp::RangeView::Of<std::string_view> keys,
                                   const CatalogScatterMatrixPlotOptions& options) const;
  pluss::pl::Studio scatter_matrix(std::initializer_list<std::string_view> keys,
                                   const CatalogScatterMatrixPlotOptions& options) const {
    return scatter_matrix(pluss::msharp::RangeView::Of<std::string_view>(keys), options);
  }

  pluss::pl::Studio auto_plot() const;
  pluss::pl::Studio auto_plot(const pluss::table::AutoPlotOptions& options) const;

private:
  Catalog base_{};
  pluss::table::DataFramePlotOptions options_{};
};

// Chart-level 3D plots (scatter/line/hist/surface/wireframe) drawn from a
// single Catalog's own key columns -- the kindle-layer counterpart to
// DataFramePlot3DAccessor. Compare to Archive::plot(), which visualizes a
// *stack* of catalogs rather than one catalog's own columns.
class CatalogPlot3DAccessor {
public:
  explicit CatalogPlot3DAccessor(Catalog base, pluss::table::DataFramePlotOptions options = {});

  pluss::pl::Studio scatter(std::string_view x_key,
                            std::string_view y_key,
                            std::string_view z_key,
                            std::optional<std::string_view> size_key = std::nullopt,
                            std::optional<std::string_view> color_key = std::nullopt,
                            const pluss::table::Scatter3DPlotOptions& options = {}) const;
  pluss::pl::Studio scatter(std::string_view x_key,
                            std::string_view y_key,
                            std::string_view z_key,
                            std::optional<std::string_view> size_key,
                            std::optional<std::string_view> color_key,
                            const CatalogScatter3DPlotOptions& options) const;
  pluss::pl::Studio line(std::string_view x_key,
                         std::string_view y_key,
                         std::string_view z_key,
                         const pluss::table::Line3DPlotOptions& options = {}) const;
  pluss::pl::Studio line(std::string_view x_key,
                         std::string_view y_key,
                         std::string_view z_key,
                         const CatalogLine3DPlotOptions& options) const;
  pluss::pl::Studio hist(std::string_view x_key,
                         std::string_view y_key,
                         const pluss::table::Hist3DPlotOptions& options = {}) const;
  pluss::pl::Studio hist(std::string_view x_key,
                         std::string_view y_key,
                         const CatalogHist3DPlotOptions& options) const;
  pluss::pl::Studio surface(std::string_view x_key,
                            std::string_view y_key,
                            std::string_view z_key,
                            pluss::table::DataFrame::GroupOp op = pluss::table::DataFrame::GroupOp::Mean,
                            const pluss::table::Surface3DPlotOptions& options = {}) const;
  pluss::pl::Studio wireframe(std::string_view x_key,
                              std::string_view y_key,
                              std::string_view z_key,
                              pluss::table::DataFrame::GroupOp op = pluss::table::DataFrame::GroupOp::Mean,
                              const pluss::table::Wireframe3DPlotOptions& options = {}) const;

private:
  void apply_figure_options(pluss::pl::Studio& figure) const;

  Catalog base_{};
  pluss::table::DataFramePlotOptions options_{};
};

class CatalogGroupBuilder {
public:
  explicit CatalogGroupBuilder(Catalog base);

  std::unordered_map<std::string, Catalog> by(std::string_view key) const;

private:
  Catalog base_{};
};

template <class Fn>
Catalog CatalogKeyAccessor::map_selected_(Fn&& fn) const {
  Catalog out = base_;
  for (const auto& key : keys_) {
    out.set_(key, std::forward<Fn>(fn)(base_.keys(key)));
  }
  return out;
}

template <pluss::detail::StringViewNameRange NameRange>
Catalog Catalog::keys(const NameRange& field_names) const {
  return pluss::detail::with_name_span(field_names, [&](auto names) { return keys(names); });
}

std::ostream& operator<<(std::ostream& os, const Catalog& catalog);

} // namespace pluss::kindle

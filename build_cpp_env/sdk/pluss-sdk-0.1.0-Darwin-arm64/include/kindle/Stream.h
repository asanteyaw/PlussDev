#pragma once

#include <cstdint>
#include <functional>
#include <iosfwd>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "MSharp/IntrusivePtr.h"
#include "MSharp/RangeView.h"
#include "kindle/Ragged.h"
#include "table/Slate.h"
#include "table/plot/DataFramePlot.h"

namespace pluss::pl {
class Studio;
}

namespace pluss::table {
struct DataFramePlotOptions;
struct LinePlotOptions;
struct ScatterPlotOptions;
struct BarPlotOptions;
struct HistPlotOptions;
struct DensityPlotOptions;
struct AutoPlotOptions;
}

namespace pluss::scribble {
class Slot;
}

namespace pluss::kindle {

class Stream;

namespace detail {
class StreamImpl;
bool has_validity_mask(const Stream& stream);
const std::optional<pluss::Series>& validity_mask(const Stream& stream);
Stream with_validity_mask(const Stream& stream, const pluss::Series& mask);
Stream& with_validity_mask_(Stream& stream, const pluss::Series& mask);
Stream clear_validity_mask(const Stream& stream);
Stream& clear_validity_mask_(Stream& stream);
}

// The reduction applied to each group of records by Stream::reduce_groups.
enum class GroupReduce : std::uint8_t {
  Count = 0,      // non-null values
  Ndistinct = 1,  // distinct non-null values
  Sum = 2,
  Mean = 3,
  Median = 4,     // even counts average the two middle values
  Min = 5,
  Max = 6,
  Std = 7,
  Var = 8,
};

enum class StreamKind : std::uint8_t {
  Series = 0,
  Note = 1,
  Timeline = 2,
  Duration = 4,
};

struct DistinctOptions {
  bool dropnull{true};
};

struct FreqOptions {
  std::int64_t top{0};
  bool dropnull{true};
};

class StreamCastAccessor;
class StreamNoteAccessor;
class StreamTimelineAccessor;
class StreamTimeSeriesAccessor;
class StreamLabelAccessor;
class StreamPlotAccessor;

class Stream {
public:
  Stream();
  Stream(const pluss::Series& data,
         std::optional<pluss::Series> validity_mask = std::nullopt,
         std::optional<pluss::table::Slate> cases = std::nullopt);
  Stream(const pluss::Note& data,
         std::optional<pluss::Series> validity_mask = std::nullopt,
         std::optional<pluss::table::Slate> cases = std::nullopt);
  Stream(const pluss::Timeline& data,
         std::optional<pluss::Series> validity_mask = std::nullopt,
         std::optional<pluss::table::Slate> cases = std::nullopt);
  Stream(const pluss::Duration& data,
         std::optional<pluss::Series> validity_mask = std::nullopt,
         std::optional<pluss::table::Slate> cases = std::nullopt);
  Stream(const Stream& other);
  Stream(Stream&& other) noexcept;
  Stream& operator=(const Stream& other);
  Stream& operator=(Stream&& other) noexcept;
  ~Stream();

  static Stream from_slate(const pluss::table::Slate& slate,
                            std::optional<pluss::table::Slate> cases = std::nullopt);

  // Changes whenever the stream's contents can have changed: a different handle
  // (edits clone), or an in-place edit of the shared Series storage (storage
  // version). Only meaningful while a copy of the stream is kept alive, since it
  // folds in the handle's address. Used by caches keyed on stream contents.
  std::uint64_t fingerprint() const;

  bool defined() const noexcept;
  bool unique() const noexcept;
  std::int64_t size() const;
  std::int64_t nrecords() const;
  bool empty() const;
  std::pair<std::int64_t, std::int64_t> shape() const;
  StreamKind kind() const;
  std::string ptype() const;
  std::string dtype() const;

  // Stream-native missing/predicate helpers (delegates to Slate).
  Stream isnull() const;
  Stream notnull() const;
  Stream invalid() const;

  // Stream-native transforms (delegates to Slate where applicable).
  Stream fillnull(double value) const;
  Stream fillnull(std::string_view value) const;
  Stream fillnull(const char* value) const {
    return fillnull(std::string_view{value != nullptr ? value : ""});
  }
  Stream fillnull(const pluss::Timeline& value) const;
  Stream fillnull(const pluss::Duration& value) const;
  Stream abs() const;
  Stream clip(double min_value, double max_value) const;
  Stream log() const;
  Stream exp() const;

  bool has_label() const;
  const std::optional<pluss::table::Slate>& label() const;
  Stream with_label(const pluss::table::Slate& label) const;
  Stream& with_label_(const pluss::table::Slate& label);
  Stream clear_label() const;
  Stream& clear_label_();

  // Free-form metadata attached to the stream (extensible control surface).
  bool has_meta(std::string_view key) const;
  std::optional<std::string> meta(std::string_view key) const;
  const std::unordered_map<std::string, std::string>& meta() const;
  Stream with_meta(std::string key, std::string value) const;
  Stream& with_meta_(std::string key, std::string value);
  Stream with_meta(std::initializer_list<std::pair<std::string_view, std::string_view>> items) const;
  Stream& with_meta_(std::initializer_list<std::pair<std::string_view, std::string_view>> items);
  Stream drop_meta(std::string_view key) const;
  Stream& drop_meta_(std::string_view key);

  pluss::table::Slate to_slate() const;
  pluss::table::Slate to_slate(std::int64_t start, std::int64_t stop) const;
  pluss::table::Slate tabelize() const;
  pluss::scribble::Slot scribbelize() const;

  // 1D row-space operations.
  Stream head(std::int64_t n) const;
  Stream tail(std::int64_t n) const;
  Stream slice(std::int64_t start, std::int64_t stop) const;
  Stream take(std::span<const std::int64_t> indices) const;
  // Rebuild the stream through one source record per output row. Record r of
  // `source` is stream position r + shift (shift lets a tail-aligned catalog
  // address its streams); the three ways a row can have no value are kept apart:
  //   source[p] == -1                  no partner: an explicit null
  //   position beyond the stream's end absent: trailing absence is trimmed when
  //                                    trim_trailing, interior absence is a null
  //   position before its start        absent (a null if it is not trailing)
  // A record that exists keeps its own nullness. Never throws on a missing record.
  Stream gather(const std::vector<std::int64_t>& source, std::int64_t shift = 0, bool trim_trailing = true) const;

  // Reduce groups of records. Group g covers source[offsets[g] .. offsets[g+1]);
  // each entry is a record index of this stream (its position is entry + shift),
  // and entries outside the stream, null values and NaN are skipped. The result
  // has one entry per group. A group with no usable value gives 0 for Count and
  // Ndistinct and a null for every other reduction; Var/Std are null when the
  // group has no more than `ddof` values (ddof = 1 when unbiased). Count and
  // Ndistinct work on any kind of stream, the others need a numeric Series.
  // Groups are processed in parallel; offsets must be non-decreasing, start at 0
  // and end at source.size().
  Stream reduce_groups(const std::vector<std::int64_t>& source,
                       std::int64_t shift,
                       const std::vector<std::int64_t>& offsets,
                       GroupReduce op,
                       bool unbiased = true) const;
  Stream filter(const pluss::Series& bool_mask) const;
  Stream dropnull() const;
  Stream ffill() const;
  Stream bfill() const;
  Stream distinct(DistinctOptions options = {}) const;
  Stream freq(FreqOptions options = {}) const;
  Stream ndistinct(DistinctOptions options = {}) const;
  Stream map(const std::function<Stream(const Stream&)>& fn) const;

  static Stream select(pluss::msharp::RangeView::Of<pluss::table::Slate> conditions,
                       pluss::msharp::RangeView::Of<Stream> choices,
                       const Stream& default_value);
  static Stream select(std::initializer_list<pluss::table::Slate> conditions,
                       std::initializer_list<Stream> choices,
                       const Stream& default_value) {
    return select(
        pluss::msharp::RangeView::Of<pluss::table::Slate>(conditions),
        pluss::msharp::RangeView::Of<Stream>(choices),
        default_value);
  }

  // Type conversion.
  StreamCastAccessor cast() const;
  StreamCastAccessor cast(const pluss::table::SlateCastOptions& options) const;
  Stream cast(pluss::table::SlateCastTarget target,
              std::optional<pluss::codem::ScalarType> series_dtype = std::nullopt) const;
  Stream cast(pluss::table::SlateCastTarget target,
              const pluss::table::SlateCastOptions& options) const;
  Stream astype(pluss::codem::ScalarType dtype) const;

  StreamNoteAccessor nt() const;
  StreamTimelineAccessor tl() const;
  StreamTimeSeriesAccessor ts() const;
  StreamLabelAccessor lbl() const;
  StreamPlotAccessor plot() const;
  StreamPlotAccessor plot(const pluss::table::DataFramePlotOptions& options) const;

  // Reductions return scalar Stream (Series payload with numel == 1).
  Stream sum() const;
  Stream mean() const;
  Stream min() const;
  Stream max() const;
  Stream var(bool unbiased = true) const;
  Stream std(bool unbiased = true) const;

  const pluss::Series& as_series() const;
  const pluss::Note& as_note() const;
  const pluss::Timeline& as_timeline() const;
  const pluss::Duration& as_duration() const;

  std::string to_string() const;
  std::string to_json() const;
  void show(std::ostream& os) const;

private:
  friend bool detail::has_validity_mask(const Stream& stream);
  friend const std::optional<pluss::Series>& detail::validity_mask(const Stream& stream);
  friend Stream detail::with_validity_mask(const Stream& stream, const pluss::Series& mask);
  friend Stream& detail::with_validity_mask_(Stream& stream, const pluss::Series& mask);
  friend Stream detail::clear_validity_mask(const Stream& stream);
  friend Stream& detail::clear_validity_mask_(Stream& stream);

  explicit Stream(pluss::msharp::intrusive_ptr<detail::StreamImpl> impl);
  bool has_validity_mask() const;
  const std::optional<pluss::Series>& validity_mask() const;
  Stream with_validity_mask(const pluss::Series& mask) const;
  Stream& with_validity_mask_(const pluss::Series& mask);
  Stream clear_validity_mask() const;
  Stream& clear_validity_mask_();
  detail::StreamImpl& mut_impl_();
  const detail::StreamImpl& impl_ref_() const;

  pluss::msharp::intrusive_ptr<detail::StreamImpl> impl_{nullptr};
};

class StreamCastAccessor {
public:
  explicit StreamCastAccessor(Stream base, pluss::table::SlateCastOptions options = {});

  Stream series() const;
  Stream note() const;
  Stream timeline() const;
  Stream duration() const;

private:
  Stream base_{};
  pluss::table::SlateCastOptions options_{};
};

class StreamNoteAccessor {
public:
  explicit StreamNoteAccessor(Stream base);

  Stream strip() const;
  Stream strip(std::string_view chars) const;
  Stream lstrip() const;
  Stream lstrip(std::string_view chars) const;
  Stream rstrip() const;
  Stream rstrip(std::string_view chars) const;
  Stream lower() const;
  Stream upper() const;
  Stream casefold() const;
  Stream titlecase() const;
  Stream length() const;
  Stream starts(std::string_view prefix) const;
  Stream ends(std::string_view suffix) const;
  Stream has(std::string_view needle,
             const pluss::table::TextOptions& options = {}) const;
  Stream count(std::string_view pattern) const;
  Stream count(std::string_view pattern,
               const pluss::table::TextOptions& options) const;
  Stream matches(std::string_view pattern, bool full = false) const;
  Stream fullmatch(std::string_view pattern) const;
  pluss::table::DataFrame findall(std::string_view pattern) const;
  pluss::table::DataFrame findall(std::string_view pattern,
                                  const pluss::table::TextOptions& options) const;
  pluss::table::DataFrame split(std::string_view delimiter,
                                std::int64_t limit = -1,
                                std::string prefix = "part") const;
  pluss::table::DataFrame split(std::string_view pattern,
                                const pluss::table::TextOptions& options) const;
  pluss::table::DataFrame partition(std::string_view separator,
                                    std::string prefix = "part") const;
  Stream pad(std::int64_t width,
             pluss::table::TextAlign align = pluss::table::TextAlign::Left,
             char fill = ' ') const;
  Stream chars(std::int64_t start,
               std::optional<std::int64_t> stop = std::nullopt,
               std::int64_t step = 1) const;
  Stream normalize(pluss::table::UnicodeForm form = pluss::table::UnicodeForm::NFC) const;
  Stream encode(std::string_view encoding = "base64") const;
  pluss::table::DataFrame indicate(std::string_view delimiter = "|",
                                   std::string prefix = "") const;
  Stream replace(std::string_view from, std::string_view to) const;
  Stream replace(std::string_view from,
                 std::string_view to,
                 const pluss::table::TextOptions& options) const;

private:
  Stream base_{};
};

class StreamTimelineAccessor {
public:
  explicit StreamTimelineAccessor(Stream base);

  Stream isnat() const;
  Stream valid() const;
  Stream year() const;
  Stream month() const;
  Stream day() const;
  Stream hour() const;
  Stream minute() const;
  Stream second() const;
  Stream weekday() const;
  Stream yearday() const;
  Stream quarter() const;
  Stream week() const;
  Stream weekend() const;
  Stream month_start() const;
  Stream month_end() const;
  Stream leap() const;
  Stream weekday_name() const;
  Stream month_name() const;
  Stream fmt(std::string_view pattern) const;
  Stream iso() const;
  Stream epoch(pluss::table::TimeUnit unit = pluss::table::TimeUnit::Nanos) const;
  Stream floor(std::string_view unit) const;
  Stream ceil(std::string_view unit) const;
  Stream round(std::string_view unit) const;
  Stream date() const;
  Stream time() const;
  Stream shift(std::int64_t periods, std::string_view freq) const;
  Stream period(std::string_view freq) const;
  Stream tz(std::string_view name) const;
  Stream localize(const pluss::table::TimelineLocalizeOptions& options = {}) const;
  Stream utc() const;

private:
  Stream base_{};
};

class StreamTimeSeriesAccessor {
public:
  explicit StreamTimeSeriesAccessor(Stream base);

  Stream returns(std::int64_t periods = 1) const;
  Stream log_returns(std::int64_t periods = 1) const;
  Stream diff(std::int64_t periods = 1) const;
  Stream lag(std::int64_t periods = 1) const;
  Stream lead(std::int64_t periods = 1) const;
  Stream rolling(std::int64_t window,
                 pluss::table::WindowStatistic statistic = pluss::table::WindowStatistic::Mean,
                 const pluss::table::WindowOptions& options = {}) const;
  Stream rolling(const Stream& axis,
                 std::string_view duration,
                 pluss::table::WindowStatistic statistic = pluss::table::WindowStatistic::Mean,
                 const pluss::table::StatisticOptions& options = {},
                 pluss::table::WindowClosure closure = pluss::table::WindowClosure::Right) const;
  Stream rolling(const Stream& axis,
                 const Stream& groups,
                 std::string_view duration,
                 pluss::table::WindowStatistic statistic = pluss::table::WindowStatistic::Mean,
                 const pluss::table::StatisticOptions& options = {},
                 pluss::table::WindowClosure closure = pluss::table::WindowClosure::Right) const;
  Stream timed(const Stream& axis,
               std::int64_t duration_nanos,
               pluss::table::TimeWindowStatistic statistic = pluss::table::TimeWindowStatistic::Mean,
               const pluss::table::StatisticOptions& options = {},
               pluss::table::WindowClosure closure = pluss::table::WindowClosure::Right) const;
  Stream grouped(const Stream& groups,
                 std::int64_t window,
                 pluss::table::WindowStatistic statistic = pluss::table::WindowStatistic::Mean,
                 const pluss::table::WindowOptions& options = {}) const;
  Stream expanding(pluss::table::WindowStatistic statistic = pluss::table::WindowStatistic::Mean,
                   const pluss::table::StatisticOptions& options = {}) const;
  Stream ewm(double alpha,
             pluss::table::WindowStatistic statistic = pluss::table::WindowStatistic::Mean,
             bool adjusted = true,
             bool bias_corrected = true,
             std::int64_t min_count = 1,
             bool ignore_nulls = false) const;
  Stream zscore_rolling(std::int64_t window,
                        std::int64_t min_periods = 1,
                        double epsilon = 1e-12) const;
  Stream fill_gaps(pluss::subsp::stats::ts::GapFillMode mode =
                       pluss::subsp::stats::ts::GapFillMode::Forward,
                   std::optional<double> constant = std::nullopt) const;
  Stream align_to(const Stream& source_axis,
                  const Stream& target_axis,
                  pluss::table::TsAlignMode mode = pluss::table::TsAlignMode::Exact) const;
  Stream align_to(const Stream& source_axis,
                  const Stream& target_axis,
                  pluss::table::TsAlignMode mode,
                  pluss::subsp::stats::ts::GapFillMode fill_mode,
                  std::optional<double> constant = std::nullopt) const;
  pluss::table::DataFrame lag_block(pluss::msharp::RangeView::Int lags) const;
  pluss::table::DataFrame rolling_block(pluss::msharp::RangeView::Int windows,
                                        const pluss::table::TsRollingBlockOptions& options = {}) const;
  pluss::table::DataFrame calendar(const Stream& axis) const;
  Stream sort(const Stream& axis,
              bool ascending = true,
              bool nulls_last = true) const;

private:
  Stream base_{};
};

class StreamLabelAccessor {
public:
  explicit StreamLabelAccessor(Stream base);

  Stream values() const;
  bool has() const;
  Stream set(const Stream& labels) const;
  Stream set(const pluss::table::Slate& labels) const;
  Stream range() const;
  Stream reset() const;
  Stream align(const Stream& target_labels,
               const pluss::table::AlignOptions& options = {}) const;
  Stream align(const pluss::table::Slate& target_labels,
               const pluss::table::AlignOptions& options = {}) const;

private:
  Stream base_{};
};

class StreamPlotAccessor {
public:
  explicit StreamPlotAccessor(Stream base, pluss::table::DataFramePlotOptions options = {});

  pluss::pl::Studio line() const;
  pluss::pl::Studio line(const Stream& x) const;
  pluss::pl::Studio line(const pluss::table::LinePlotOptions& options) const;
  pluss::pl::Studio line(const Stream& x, const pluss::table::LinePlotOptions& options) const;

  pluss::pl::Studio hist(std::int64_t bins = 20) const;
  pluss::pl::Studio hist(const pluss::table::HistPlotOptions& options) const;

  pluss::pl::Studio bar() const;
  pluss::pl::Studio bar(const Stream& x) const;
  pluss::pl::Studio bar(const pluss::table::BarPlotOptions& options) const;
  pluss::pl::Studio bar(const Stream& x, const pluss::table::BarPlotOptions& options) const;

  pluss::pl::Studio scatter(const Stream& x) const;
  pluss::pl::Studio scatter(const Stream& x, const pluss::table::ScatterPlotOptions& options) const;
  pluss::pl::Studio scatter(const Stream& x, const Stream& hue) const;
  pluss::pl::Studio scatter(const Stream& x,
                            const Stream& hue,
                            const pluss::table::ScatterPlotOptions& options) const;

  pluss::pl::Studio density(std::int64_t points = 128) const;
  pluss::pl::Studio density(const pluss::table::DensityPlotOptions& options) const;
  pluss::pl::Studio auto_plot() const;
  pluss::pl::Studio auto_plot(const pluss::table::AutoPlotOptions& options) const;

private:
  Stream base_{};
  pluss::table::DataFramePlotOptions options_{};
};

namespace detail {
inline bool has_validity_mask(const Stream& stream) {
  return stream.has_validity_mask();
}

inline const std::optional<pluss::Series>& validity_mask(const Stream& stream) {
  return stream.validity_mask();
}

inline Stream with_validity_mask(const Stream& stream, const pluss::Series& mask) {
  return stream.with_validity_mask(mask);
}

inline Stream& with_validity_mask_(Stream& stream, const pluss::Series& mask) {
  return stream.with_validity_mask_(mask);
}

inline Stream clear_validity_mask(const Stream& stream) {
  return stream.clear_validity_mask();
}

inline Stream& clear_validity_mask_(Stream& stream) {
  return stream.clear_validity_mask_();
}
} // namespace detail

std::ostream& operator<<(std::ostream& os, const Stream& stream);

Stream operator+(const Stream& lhs, double rhs);
Stream operator+(double lhs, const Stream& rhs);
Stream operator+(const Stream& lhs, const Stream& rhs);
Stream operator-(const Stream& lhs, double rhs);
Stream operator-(double lhs, const Stream& rhs);
Stream operator-(const Stream& lhs, const Stream& rhs);
Stream operator*(const Stream& lhs, double rhs);
Stream operator*(double lhs, const Stream& rhs);
Stream operator*(const Stream& lhs, const Stream& rhs);
Stream operator/(const Stream& lhs, double rhs);
Stream operator/(double lhs, const Stream& rhs);
Stream operator/(const Stream& lhs, const Stream& rhs);
Stream operator%(const Stream& lhs, double rhs);
Stream operator%(double lhs, const Stream& rhs);
Stream operator%(const Stream& lhs, const Stream& rhs);
Stream operator==(const Stream& lhs, double rhs);
Stream operator==(double lhs, const Stream& rhs);
Stream operator==(const Stream& lhs, const Stream& rhs);
Stream operator!=(const Stream& lhs, double rhs);
Stream operator!=(double lhs, const Stream& rhs);
Stream operator!=(const Stream& lhs, const Stream& rhs);
Stream operator<(const Stream& lhs, double rhs);
Stream operator<(double lhs, const Stream& rhs);
Stream operator<(const Stream& lhs, const Stream& rhs);
Stream operator<=(const Stream& lhs, double rhs);
Stream operator<=(double lhs, const Stream& rhs);
Stream operator<=(const Stream& lhs, const Stream& rhs);
Stream operator>(const Stream& lhs, double rhs);
Stream operator>(double lhs, const Stream& rhs);
Stream operator>(const Stream& lhs, const Stream& rhs);
Stream operator>=(const Stream& lhs, double rhs);
Stream operator>=(double lhs, const Stream& rhs);
Stream operator>=(const Stream& lhs, const Stream& rhs);

// Elementwise `lhs op rhs` between streams that may differ in length, decided by
// a RaggedPolicy (default: OpClass::Elementwise = longest domain, head aligned,
// an absent operand makes the result null there; nothing dropped). op is one of
// + - * / % == != < <= > >= (or add sub mul div mod eq ne lt le gt ge). Knobs:
// domain Shortest/Exact, alignment Tail, absent Fill (uses policy.fill; numeric
// streams, float result) / ForwardFill / BackwardFill / Error, loss Error.
Stream ragged_binary(const Stream& lhs, const Stream& rhs, std::string_view op,
                     const RaggedPolicy& policy = RaggedPolicy::defaults(OpClass::Elementwise));

} // namespace pluss::kindle

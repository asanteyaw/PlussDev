#pragma once

#include <cstdint>
#include <functional>
#include <iosfwd>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "MSharp/IntrusivePtr.h"
#include "MSharp/RangeView.h"
#include "table/display/Display.h"
#include "pluss/null.h"
#include "pluss/codem/codem.h"
#include "pluss/coden/coden.h"
#include "pluss/codeh/codeh.h"
#include "pluss/subsp/stats/ts.h"

namespace pluss::table {

namespace detail {
class ThreadImpl;
}
class ThreadValue;
class DataFrame;
class DataFrameView;
class ThreadView;
class TimeSeriesAccessor;

enum class ThreadKind : std::uint8_t {
  Series = 0,
  Note = 1,
  Timeline = 2,
  TimeIndex = 3,
};

enum class ThreadBinaryMathOp : std::uint8_t {
  Add = 0,
  Sub = 1,
  Mul = 2,
  Div = 3,
  TrueDiv = 4,
  FloorDiv = 5,
  Mod = 6,
  Pow = 7,
};

enum class ThreadBinaryCompareOp : std::uint8_t {
  Eq = 0,
  Ne = 1,
  Lt = 2,
  Le = 3,
  Gt = 4,
  Ge = 5,
};

enum class ThreadCastTarget : std::uint8_t {
  Series = 0,
  Note = 1,
  Timeline = 2,
  TimeIndex = 3,
};

enum class ThreadWindowOp : std::uint8_t {
  RollingSum = 0,
  RollingMean = 1,
  RollingMin = 2,
  RollingMax = 3,
};

enum class ThreadCorrMethod : std::uint8_t {
  Pearson = 0,
  Spearman = 1,
  KendallTau = 2,
};

enum class RankTieRule : std::uint8_t {
  Average = 0,
  Lowest = 1,
  Highest = 2,
  First = 3,
  Dense = 4,
};

enum class RankNullRule : std::uint8_t {
  Keep = 0,
  Top = 1,
  Bottom = 2,
};

struct StatisticOptions {
  std::int64_t ddof{1};
  std::int64_t min_count{1};
};

enum class WindowClosure : std::uint8_t {
  Right = 0,
  Left = 1,
  Both = 2,
  Neither = 3,
};

struct WindowOptions {
  std::int64_t min_count{1};
  std::int64_t ddof{1};
  bool centered{false};
  WindowClosure closure{WindowClosure::Right};
};

enum class WindowStatistic : std::uint8_t {
  Sum = 0,
  Mean = 1,
  Minimum = 2,
  Maximum = 3,
  Variance = 4,
  Deviation = 5,
};

enum class TimeWindowStatistic : std::uint8_t {
  Sum = 0,
  Mean = 1,
  Variance = 2,
  Deviation = 3,
};

enum class UnicodeForm : std::uint8_t {
  NFC = 0,
  NFD = 1,
  NFKC = 2,
  NFKD = 3,
};

enum class TextAlign : std::uint8_t {
  Left = 0,
  Right = 1,
  Center = 2,
};

enum class ResultShape : std::uint8_t {
  Infer = 0,
  Reduce = 1,
  Preserve = 2,
  Expand = 3,
};

using WindowBoundsFn =
    std::function<std::pair<std::vector<std::int64_t>, std::vector<std::int64_t>>(
        std::int64_t size,
        const WindowOptions& options)>;

class Thread {
public:
  Thread();
  Thread(const pluss::Series& data, std::optional<pluss::Series> validity_mask = std::nullopt);
  Thread(const pluss::Note& data, std::optional<pluss::Series> validity_mask = std::nullopt);
  Thread(const pluss::Timeline& data, std::optional<pluss::Series> validity_mask = std::nullopt);
  Thread(const pluss::TimeIndex& data, std::optional<pluss::Series> validity_mask = std::nullopt);
  Thread(const Thread& other);
  Thread(Thread&& other) noexcept;
  Thread& operator=(const Thread& other);
  Thread& operator=(Thread&& other) noexcept;
  ~Thread();

  bool defined() const noexcept;
  bool unique() const noexcept;
  bool shares_payload_with(const Thread& other) const noexcept;
  std::int64_t size() const;
  ThreadKind kind() const;
  std::string dtype_name() const;
  ThreadValue value() const;
  bool is_valid(std::int64_t row) const;

  bool has_validity_mask() const;
  const std::optional<pluss::Series>& validity_mask() const;
  Thread with_validity_mask(const pluss::Series& validity_mask) const;
  Thread& with_validity_mask_(const pluss::Series& validity_mask);
  Thread clear_validity_mask() const;
  Thread& clear_validity_mask_();

  Thread isnull() const;
  Thread notnull() const;
  Thread isnan() const;
  Thread isfinite() const;

  // Functional-by-default transforms.
  Thread fillna(double value) const;
  Thread fillna(std::string_view value) const;
  Thread fillna(const char* value) const {
    return fillna(std::string_view{value != nullptr ? value : ""});
  }
  Thread fillna(const pluss::Timeline& value) const;
  Thread fillna(const pluss::TimeIndex& value) const;
  Thread abs() const;
  Thread clip(double min_value, double max_value) const;
  Thread log() const;
  Thread exp() const;

  // Explicit mutating transforms (copy-on-write when shared).
  Thread& fillna_(double value);
  Thread& fillna_(std::string_view value);
  Thread& fillna_(const char* value) {
    return fillna_(std::string_view{value != nullptr ? value : ""});
  }
  Thread& fillna_(const pluss::Timeline& value);
  Thread& fillna_(const pluss::TimeIndex& value);

  // 1D row-space transforms.
  ThreadView head(std::int64_t n = 5, bool render = true) const;
  ThreadView tail(std::int64_t n = 5, bool render = true) const;
  ThreadView view(std::int64_t start, std::int64_t stop, bool render = true) const;
  Thread shift(std::int64_t periods = 1) const;
  Thread slice(std::int64_t start, std::int64_t stop) const;
  Thread take(std::span<const std::int64_t> indices) const;
  Thread filter(const pluss::Series& bool_mask) const;
  Thread dropna() const;
  Thread sort_values(bool ascending = true, bool nulls_last = true) const;
  Thread ffill() const;
  Thread bfill() const;
  Thread distinct(bool dropna = true) const;
  DataFrameView tally(std::int64_t top_n = 0, bool dropna = true, bool render = true) const;
  ThreadView describe(bool render = true) const;
  Thread bucketize(pluss::msharp::RangeView::Double bins,
                   pluss::msharp::RangeView::Int labels = {},
                   bool right = true,
                   bool include_lowest = true) const;

  // Reductions (Series payload only) return scalar Series (numel == 1).
  pluss::Series sum() const;
  pluss::Series count(bool count_true = true) const;
  pluss::Series any(bool skipna = true) const;
  pluss::Series all(bool skipna = true) const;
  pluss::Series mean() const;
  pluss::Series median() const;
  pluss::Series min() const;
  pluss::Series max() const;
  pluss::Series var(bool unbiased = true) const;
  pluss::Series std(bool unbiased = true) const;
  double corr(const Thread& other, ThreadCorrMethod method = ThreadCorrMethod::Pearson) const;
  Thread covariance(const Thread& other, const StatisticOptions& options = {}) const;
  Thread percentiles(pluss::msharp::RangeView::Double points = {0.25, 0.5, 0.75},
                     std::int64_t min_count = 1) const;
  Thread most_frequent(bool drop_nulls = true) const;
  Thread ranks(RankTieRule ties = RankTieRule::Average,
               RankNullRule nulls = RankNullRule::Keep,
               bool ascending = true,
               bool percentage = false) const;
  Thread asymmetry(std::int64_t min_count = 3) const;
  Thread tail_weight(std::int64_t min_count = 4) const;
  Thread variance(const StatisticOptions& options = {}) const;
  Thread deviation(const StatisticOptions& options = {}) const;
  Thread mean_error(const StatisticOptions& options = {}) const;
  Thread running_sum(bool skip_nulls = true) const;
  Thread running_product(bool skip_nulls = true) const;
  Thread running_minimum(bool skip_nulls = true) const;
  Thread running_maximum(bool skip_nulls = true) const;
  Thread correlate_with(const Thread& other,
                        ThreadCorrMethod method = ThreadCorrMethod::Pearson,
                        std::int64_t min_count = 1) const;
  pluss::Series matrix_product(const Thread& other) const;

  Thread moving(std::int64_t window,
                WindowStatistic statistic,
                const WindowOptions& options = {}) const;
  Thread moving_mean_error(std::int64_t window,
                           const WindowOptions& options = {}) const;
  Thread moving_asymmetry(std::int64_t window,
                          const WindowOptions& options = {}) const;
  Thread moving_tail_weight(std::int64_t window,
                            const WindowOptions& options = {}) const;
  Thread growing(WindowStatistic statistic,
                 const StatisticOptions& options = {}) const;
  Thread growing_mean_error(const StatisticOptions& options = {}) const;
  Thread growing_asymmetry(std::int64_t min_count = 3) const;
  Thread growing_tail_weight(std::int64_t min_count = 4) const;
  Thread exponential_mean(double alpha,
                          bool adjusted = true,
                          std::int64_t min_count = 1,
                          bool ignore_nulls = false) const;
  Thread exponential_variance(double alpha,
                              bool adjusted = true,
                              bool bias_corrected = true,
                              std::int64_t min_count = 1,
                              bool ignore_nulls = false) const;
  Thread exponential_deviation(double alpha,
                               bool adjusted = true,
                               bool bias_corrected = true,
                               std::int64_t min_count = 1,
                               bool ignore_nulls = false) const;
  Thread weighted_moving(pluss::msharp::RangeView::Double weights,
                         WindowStatistic statistic = WindowStatistic::Mean,
                         std::int64_t min_count = 1) const;
  Thread indexed_moving(const WindowBoundsFn& bounds,
                        WindowStatistic statistic = WindowStatistic::Mean,
                        const WindowOptions& options = {}) const;
  Thread time_moving(const Thread& axis,
                     std::int64_t duration_nanos,
                     TimeWindowStatistic statistic = TimeWindowStatistic::Mean,
                     const StatisticOptions& options = {},
                     WindowClosure closure = WindowClosure::Right) const;
  Thread grouped_moving(const Thread& groups,
                        std::int64_t window,
                        WindowStatistic statistic = WindowStatistic::Mean,
                        const WindowOptions& options = {}) const;

  Thread map_elements(const std::function<ThreadValue(const ThreadValue&)>& fn) const;
  Thread aggregate_with(const std::function<Thread(const Thread&)>& fn,
                        ResultShape shape = ResultShape::Reduce) const;
  Thread flow(const std::function<Thread(const Thread&)>& fn) const;

  // Text operations (Note payload only).
  Thread strip() const;
  Thread strip(std::string_view chars) const;
  Thread lstrip() const;
  Thread lstrip(std::string_view chars) const;
  Thread rstrip() const;
  Thread rstrip(std::string_view chars) const;
  Thread lower() const;
  Thread upper() const;
  Thread casefold() const;
  Thread title_case() const;
  Thread text_length() const;
  Thread starts_with(std::string_view prefix) const;
  Thread ends_with(std::string_view suffix) const;
  Thread regex_match(std::string_view pattern) const;
  Thread regex_fullmatch(std::string_view pattern) const;
  DataFrame regex_extract(std::string_view pattern,
                          pluss::msharp::RangeView::Of<std::string_view> names = {}) const;
  DataFrame split_text(std::string_view delimiter,
                       std::int64_t limit = -1,
                       std::string prefix = "part") const;
  DataFrame partition_text(std::string_view separator,
                           std::string prefix = "part") const;
  Thread pad_text(std::int64_t width,
                  TextAlign align = TextAlign::Left,
                  char fill = ' ') const;
  Thread slice_text(std::int64_t start,
                    std::optional<std::int64_t> stop = std::nullopt,
                    std::int64_t step = 1) const;
  Thread normalize_text(UnicodeForm form = UnicodeForm::NFC) const;
  Thread encode_text(std::string_view encoding = "base64") const;
  DataFrame text_indicators(std::string_view delimiter = "|",
                            std::string prefix = "") const;
  Thread replace(std::string_view from, std::string_view to) const;
  Thread put(const Thread& bool_mask, const Thread& value) const;
  Thread put(const Thread& bool_mask, double value) const;
  Thread put(const Thread& bool_mask, std::string_view value) const;
  Thread put(const Thread& bool_mask, const char* value) const {
    return put(bool_mask, std::string_view{value != nullptr ? value : ""});
  }
  Thread put(const Thread& bool_mask, pluss::null_t value) const;
  Thread put(const pluss::Series& bool_mask, const Thread& value) const {
    return put(Thread(bool_mask), value);
  }
  Thread put(const pluss::Series& bool_mask, double value) const {
    return put(Thread(bool_mask), value);
  }
  Thread put(const pluss::Series& bool_mask, std::string_view value) const {
    return put(Thread(bool_mask), value);
  }
  Thread put(const pluss::Series& bool_mask, const char* value) const {
    return put(Thread(bool_mask), value);
  }
  Thread put(const pluss::Series& bool_mask, pluss::null_t value) const {
    return put(Thread(bool_mask), value);
  }
  Thread replace(double from, double to) const;
  Thread replace(double from, pluss::null_t to) const;
  Thread replace(pluss::null_t from, double to) const;
  Thread replace(const pluss::Series& from, const pluss::Series& to) const;
  Thread replace(const pluss::Series& from, pluss::null_t to) const;
  Thread replace(pluss::null_t from, const pluss::Series& to) const;
  Thread replace(const pluss::Note& from, const pluss::Note& to) const;
  Thread replace(const pluss::Note& from, pluss::null_t to) const;
  Thread replace(pluss::null_t from, const pluss::Note& to) const;
  Thread replace(const pluss::Timeline& from, const pluss::Timeline& to) const;
  Thread replace(const pluss::Timeline& from, pluss::null_t to) const;
  Thread replace(pluss::null_t from, const pluss::Timeline& to) const;
  Thread replace(const pluss::TimeIndex& from, const pluss::TimeIndex& to) const;
  Thread replace(const pluss::TimeIndex& from, pluss::null_t to) const;
  Thread replace(pluss::null_t from, const pluss::TimeIndex& to) const;
  static Thread select(pluss::msharp::RangeView::Of<Thread> conditions,
                       pluss::msharp::RangeView::Of<Thread> choices,
                       const Thread& default_value);
  static Thread select(pluss::msharp::RangeView::Of<Thread> conditions,
                       pluss::msharp::RangeView::Of<Thread> choices,
                       double default_value);
  static Thread select(pluss::msharp::RangeView::Of<Thread> conditions,
                       pluss::msharp::RangeView::Of<Thread> choices,
                       std::string_view default_value);
  static Thread select(pluss::msharp::RangeView::Of<Thread> conditions,
                       pluss::msharp::RangeView::Of<Thread> choices,
                       const char* default_value) {
    return select(
        conditions,
        choices,
        std::string_view{default_value != nullptr ? default_value : ""});
  }
  static Thread select(pluss::msharp::RangeView::Of<Thread> conditions,
                       pluss::msharp::RangeView::Of<Thread> choices,
                       const pluss::Timeline& default_value);
  static Thread select(pluss::msharp::RangeView::Of<Thread> conditions,
                       pluss::msharp::RangeView::Of<Thread> choices,
                       const pluss::TimeIndex& default_value);
  static Thread select(pluss::msharp::RangeView::Of<Thread> conditions,
                       pluss::msharp::RangeView::Of<Thread> choices,
                       pluss::null_t default_value);
  static Thread select(std::initializer_list<Thread> conditions,
                       std::initializer_list<Thread> choices,
                       const Thread& default_value) {
    return select(
        pluss::msharp::RangeView::Of<Thread>(conditions),
        pluss::msharp::RangeView::Of<Thread>(choices),
        default_value);
  }
  static Thread select(std::initializer_list<Thread> conditions,
                       std::initializer_list<Thread> choices,
                       double default_value) {
    return select(
        pluss::msharp::RangeView::Of<Thread>(conditions),
        pluss::msharp::RangeView::Of<Thread>(choices),
        default_value);
  }
  static Thread select(std::initializer_list<Thread> conditions,
                       std::initializer_list<Thread> choices,
                       std::string_view default_value) {
    return select(
        pluss::msharp::RangeView::Of<Thread>(conditions),
        pluss::msharp::RangeView::Of<Thread>(choices),
        default_value);
  }
  static Thread select(std::initializer_list<Thread> conditions,
                       std::initializer_list<Thread> choices,
                       const char* default_value) {
    return select(
        pluss::msharp::RangeView::Of<Thread>(conditions),
        pluss::msharp::RangeView::Of<Thread>(choices),
        default_value);
  }
  static Thread select(std::initializer_list<Thread> conditions,
                       std::initializer_list<Thread> choices,
                       const pluss::Timeline& default_value) {
    return select(
        pluss::msharp::RangeView::Of<Thread>(conditions),
        pluss::msharp::RangeView::Of<Thread>(choices),
        default_value);
  }
  static Thread select(std::initializer_list<Thread> conditions,
                       std::initializer_list<Thread> choices,
                       const pluss::TimeIndex& default_value) {
    return select(
        pluss::msharp::RangeView::Of<Thread>(conditions),
        pluss::msharp::RangeView::Of<Thread>(choices),
        default_value);
  }
  static Thread select(std::initializer_list<Thread> conditions,
                       std::initializer_list<Thread> choices,
                       pluss::null_t default_value) {
    return select(
        pluss::msharp::RangeView::Of<Thread>(conditions),
        pluss::msharp::RangeView::Of<Thread>(choices),
        default_value);
  }
  pluss::Series contains(std::string_view needle) const;

  // Time operations (Timeline / TimeIndex payload only).
  Thread year() const;
  Thread month() const;
  Thread day() const;
  Thread hour() const;
  Thread to_note_weekday() const;

  // Binary operations for DataFrame-level delegation.
  Thread numeric_binary(double other, ThreadBinaryMathOp op, bool reverse = false) const;
  Thread numeric_binary(const Thread& other, ThreadBinaryMathOp op) const;
  Thread compare(double other, ThreadBinaryCompareOp op) const;
  Thread compare(std::string_view other, ThreadBinaryCompareOp op) const;
  Thread compare(const char* other, ThreadBinaryCompareOp op) const {
    return compare(std::string_view{other != nullptr ? other : ""}, op);
  }
  Thread compare(const pluss::Series& other, ThreadBinaryCompareOp op) const;
  Thread compare(const pluss::Note& other, ThreadBinaryCompareOp op) const;
  Thread compare(const pluss::Timeline& other, ThreadBinaryCompareOp op) const;
  Thread compare(const pluss::TimeIndex& other, ThreadBinaryCompareOp op) const;
  Thread compare(const Thread& other, ThreadBinaryCompareOp op) const;
  Thread btw(double a, double b, bool inclusive = false) const;
  Thread btw(double a, double b, bool include_left, bool include_right) const;
  Thread btw(std::string_view lo, std::string_view hi, bool inclusive = true) const;
  Thread btw(std::string_view lo, std::string_view hi, bool include_left, bool include_right) const;
  Thread btw(const Thread& lo, const Thread& hi, bool inclusive = true) const;
  Thread btw(const Thread& lo, const Thread& hi, bool include_left, bool include_right) const;
  int compare_rows(std::int64_t lhs_row, std::int64_t rhs_row, bool nulls_last = true) const;
  Thread window(std::int64_t window, ThreadWindowOp op, std::int64_t min_periods = 1) const;
  Thread cast(ThreadCastTarget target,
              std::optional<pluss::codem::ScalarType> series_dtype = std::nullopt) const;
  Thread astype(pluss::codem::ScalarType dtype) const;

  std::string to_string(const display::DisplayOptions& options = {}) const;
  std::string info_string() const;
  void show(const display::DisplayOptions& options = {}) const;
  void show(std::ostream& os, const display::DisplayOptions& options = {}) const;
  void info() const;
  void info(std::ostream& os) const;
  bool has_display_labels() const;
  Thread display_labels() const;
  Thread with_display_labels(const Thread& labels) const;
  Thread clear_display_labels() const;
  bool has_display_name() const;
  std::string display_name() const;
  Thread with_display_name(std::string name) const;
  Thread clear_display_name() const;

  const pluss::Series& as_series() const;
  const pluss::Note& as_note() const;
  const pluss::Timeline& as_timeline() const;
  const pluss::TimeIndex& as_timeindex() const;

  TimeSeriesAccessor ts() const;

private:
  explicit Thread(pluss::msharp::intrusive_ptr<detail::ThreadImpl> impl);

  detail::ThreadImpl& mut_impl_();
  const detail::ThreadImpl& impl_ref_() const;

  pluss::msharp::intrusive_ptr<detail::ThreadImpl> impl_{nullptr};
};

struct TimeSeriesResample {
  Thread labels{};
  Thread values{};
};

class TimeSeriesAccessor {
 public:
  explicit TimeSeriesAccessor(Thread base);

  Thread returns(std::int64_t periods = 1) const;
  Thread log_returns(std::int64_t periods = 1) const;
  Thread diff(std::int64_t periods = 1) const;
  Thread lag(std::int64_t periods = 1) const;
  Thread lead(std::int64_t periods = 1) const;
  Thread rolling_mean(std::int64_t window, std::int64_t min_periods = 1) const;
  Thread rolling_std(std::int64_t window, std::int64_t min_periods = 1) const;
  Thread rolling_sum(std::int64_t window, std::int64_t min_periods = 1) const;
  Thread rolling_var(std::int64_t window, std::int64_t min_periods = 1) const;
  Thread rolling_min(std::int64_t window, std::int64_t min_periods = 1) const;
  Thread rolling_max(std::int64_t window, std::int64_t min_periods = 1) const;
  Thread ema(double alpha) const;
  Thread ewm_mean(double alpha) const;
  Thread ewm_std(double alpha, double epsilon = 1e-12) const;
  Thread expanding_mean(std::int64_t min_periods = 1) const;
  Thread expanding_std(std::int64_t min_periods = 1, double epsilon = 1e-12) const;
  Thread zscore_rolling(std::int64_t window,
                        std::int64_t min_periods = 1,
                        double epsilon = 1e-12) const;
  Thread fill_gaps(pluss::subsp::stats::ts::GapFillMode mode =
                       pluss::subsp::stats::ts::GapFillMode::Forward,
                   std::optional<double> constant = std::nullopt) const;
  TimeSeriesResample resample(const Thread& axis,
                              std::string_view freq,
                              pluss::subsp::stats::ts::ResampleOp op =
                                  pluss::subsp::stats::ts::ResampleOp::Mean,
                              std::int64_t step = 1) const;

 private:
  const pluss::Series& series_payload_(const char* op) const;
  Thread base_{};
};

class ThreadView final : public Thread {
 public:
  ThreadView();
  explicit ThreadView(Thread thread,
                      bool auto_render_terminal = false,
                      std::string operation = {});
  ThreadView(const ThreadView& other);
  ThreadView(ThreadView&& other) noexcept;
  ThreadView& operator=(const ThreadView& other);
  ThreadView& operator=(ThreadView&& other) noexcept;
  ~ThreadView();

  void show(const display::DisplayOptions& options = {}) const;
  void show(std::ostream& os, const display::DisplayOptions& options = {}) const;

 private:
  struct State;
  std::shared_ptr<State> state_{};

  void consume_pending_display_() const;
  void maybe_autorender_() const;
  std::string resolved_window_title_() const;

  friend void output_render(std::ostream& os, const ThreadView& thread);
  friend std::ostream& operator<<(std::ostream& os, const ThreadView& thread);
};

Thread operator+(const Thread& lhs, double rhs);
Thread operator+(double lhs, const Thread& rhs);
Thread operator+(const Thread& lhs, const Thread& rhs);
Thread operator-(const Thread& lhs, double rhs);
Thread operator-(double lhs, const Thread& rhs);
Thread operator-(const Thread& lhs, const Thread& rhs);
Thread operator*(const Thread& lhs, double rhs);
Thread operator*(double lhs, const Thread& rhs);
Thread operator*(const Thread& lhs, const Thread& rhs);
Thread operator/(const Thread& lhs, double rhs);
Thread operator/(double lhs, const Thread& rhs);
Thread operator/(const Thread& lhs, const Thread& rhs);
Thread operator%(const Thread& lhs, double rhs);
Thread operator%(double lhs, const Thread& rhs);
Thread operator%(const Thread& lhs, const Thread& rhs);
Thread operator&(const Thread& lhs, const Thread& rhs);
Thread operator|(const Thread& lhs, const Thread& rhs);
Thread operator==(const Thread& lhs, double rhs);
Thread operator==(double lhs, const Thread& rhs);
Thread operator==(const Thread& lhs, std::string_view rhs);
Thread operator==(std::string_view lhs, const Thread& rhs);
Thread operator==(const Thread& lhs, const char* rhs);
Thread operator==(const char* lhs, const Thread& rhs);
Thread operator==(const Thread& lhs, const pluss::Series& rhs);
Thread operator==(const Thread& lhs, const pluss::Note& rhs);
Thread operator==(const Thread& lhs, const pluss::Timeline& rhs);
Thread operator==(const Thread& lhs, const pluss::TimeIndex& rhs);
Thread operator==(const Thread& lhs, const Thread& rhs);
Thread operator!=(const Thread& lhs, double rhs);
Thread operator!=(double lhs, const Thread& rhs);
Thread operator!=(const Thread& lhs, std::string_view rhs);
Thread operator!=(std::string_view lhs, const Thread& rhs);
Thread operator!=(const Thread& lhs, const char* rhs);
Thread operator!=(const char* lhs, const Thread& rhs);
Thread operator!=(const Thread& lhs, const pluss::Series& rhs);
Thread operator!=(const Thread& lhs, const pluss::Note& rhs);
Thread operator!=(const Thread& lhs, const pluss::Timeline& rhs);
Thread operator!=(const Thread& lhs, const pluss::TimeIndex& rhs);
Thread operator!=(const Thread& lhs, const Thread& rhs);
Thread operator<(const Thread& lhs, double rhs);
Thread operator<(double lhs, const Thread& rhs);
Thread operator<(const Thread& lhs, std::string_view rhs);
Thread operator<(std::string_view lhs, const Thread& rhs);
Thread operator<(const Thread& lhs, const char* rhs);
Thread operator<(const char* lhs, const Thread& rhs);
Thread operator<(const Thread& lhs, const pluss::Series& rhs);
Thread operator<(const Thread& lhs, const pluss::Note& rhs);
Thread operator<(const Thread& lhs, const pluss::Timeline& rhs);
Thread operator<(const Thread& lhs, const pluss::TimeIndex& rhs);
Thread operator<(const Thread& lhs, const Thread& rhs);
Thread operator<=(const Thread& lhs, double rhs);
Thread operator<=(double lhs, const Thread& rhs);
Thread operator<=(const Thread& lhs, std::string_view rhs);
Thread operator<=(std::string_view lhs, const Thread& rhs);
Thread operator<=(const Thread& lhs, const char* rhs);
Thread operator<=(const char* lhs, const Thread& rhs);
Thread operator<=(const Thread& lhs, const pluss::Series& rhs);
Thread operator<=(const Thread& lhs, const pluss::Note& rhs);
Thread operator<=(const Thread& lhs, const pluss::Timeline& rhs);
Thread operator<=(const Thread& lhs, const pluss::TimeIndex& rhs);
Thread operator<=(const Thread& lhs, const Thread& rhs);
Thread operator>(const Thread& lhs, double rhs);
Thread operator>(double lhs, const Thread& rhs);
Thread operator>(const Thread& lhs, std::string_view rhs);
Thread operator>(std::string_view lhs, const Thread& rhs);
Thread operator>(const Thread& lhs, const char* rhs);
Thread operator>(const char* lhs, const Thread& rhs);
Thread operator>(const Thread& lhs, const pluss::Series& rhs);
Thread operator>(const Thread& lhs, const pluss::Note& rhs);
Thread operator>(const Thread& lhs, const pluss::Timeline& rhs);
Thread operator>(const Thread& lhs, const pluss::TimeIndex& rhs);
Thread operator>(const Thread& lhs, const Thread& rhs);
Thread operator>=(const Thread& lhs, double rhs);
Thread operator>=(double lhs, const Thread& rhs);
Thread operator>=(const Thread& lhs, std::string_view rhs);
Thread operator>=(std::string_view lhs, const Thread& rhs);
Thread operator>=(const Thread& lhs, const char* rhs);
Thread operator>=(const char* lhs, const Thread& rhs);
Thread operator>=(const Thread& lhs, const pluss::Series& rhs);
Thread operator>=(const Thread& lhs, const pluss::Note& rhs);
Thread operator>=(const Thread& lhs, const pluss::Timeline& rhs);
Thread operator>=(const Thread& lhs, const pluss::TimeIndex& rhs);
Thread operator>=(const Thread& lhs, const Thread& rhs);

class ThreadValue {
public:
  using Payload = std::variant<pluss::Series, pluss::Note, pluss::Timeline, pluss::TimeIndex>;

  ThreadValue() = default;
  explicit ThreadValue(Thread thread);

  bool defined() const noexcept;
  ThreadKind kind() const;
  const Thread& thread() const noexcept;
  Payload payload() const;

  pluss::Series as_series() const;
  pluss::Note as_note() const;
  pluss::Timeline as_timeline() const;
  pluss::TimeIndex as_timeindex() const;

  operator pluss::Series() const;
  operator pluss::Note() const;
  operator pluss::Timeline() const;
  operator pluss::TimeIndex() const;

private:
  Thread thread_{};
};

std::ostream& operator<<(std::ostream& os, const Thread& thread);
void output_render(std::ostream& os, const Thread& thread);
std::ostream& operator<<(std::ostream& os, const ThreadView& thread);
void output_render(std::ostream& os, const ThreadView& thread);
std::ostream& operator<<(std::ostream& os, const ThreadValue& value);
void output_render(std::ostream& os, const ThreadValue& value);

} // namespace pluss::table

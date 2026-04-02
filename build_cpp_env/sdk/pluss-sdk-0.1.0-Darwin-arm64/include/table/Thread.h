#pragma once

#include <cstdint>
#include <initializer_list>
#include <iosfwd>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "MSharp/IntrusivePtr.h"
#include "table/display/Display.h"
#include "pluss/codem/codem.h"
#include "pluss/coden/coden.h"
#include "pluss/codeh/codeh.h"
#include "pluss/subsp/stats/ts.h"

namespace pluss::table {

namespace detail {
class ThreadImpl;
}
class ThreadValue;
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
  Thread bucketize(std::initializer_list<double> bins,
                   std::initializer_list<std::int64_t> labels = {},
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

  // Text operations (Note payload only).
  Thread lower() const;
  Thread replace(std::string_view from, std::string_view to) const;
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
  Thread compare(const Thread& other, ThreadBinaryCompareOp op) const;
  Thread btw(double a, double b, bool inclusive = false) const;
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
Thread operator==(const Thread& lhs, const Thread& rhs);
Thread operator!=(const Thread& lhs, double rhs);
Thread operator!=(double lhs, const Thread& rhs);
Thread operator!=(const Thread& lhs, const Thread& rhs);
Thread operator<(const Thread& lhs, double rhs);
Thread operator<(double lhs, const Thread& rhs);
Thread operator<(const Thread& lhs, const Thread& rhs);
Thread operator<=(const Thread& lhs, double rhs);
Thread operator<=(double lhs, const Thread& rhs);
Thread operator<=(const Thread& lhs, const Thread& rhs);
Thread operator>(const Thread& lhs, double rhs);
Thread operator>(double lhs, const Thread& rhs);
Thread operator>(const Thread& lhs, const Thread& rhs);
Thread operator>=(const Thread& lhs, double rhs);
Thread operator>=(double lhs, const Thread& rhs);
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

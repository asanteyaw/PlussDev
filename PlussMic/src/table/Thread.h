#pragma once

#include <cstdint>
#include <iosfwd>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "MSharp/IntrusivePtr.h"
#include "table/display/Display.h"
#include "pluss/codem/codem.h"
#include "pluss/coden/coden.h"
#include "pluss/codeh/codeh.h"

namespace pluss::table {

namespace detail {
class ThreadImpl;
}
class ThreadValue;

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
  const Thread& values() const noexcept;
  bool is_valid(std::int64_t row) const;

  bool has_validity_mask() const;
  const std::optional<pluss::Series>& validity_mask() const;
  Thread with_validity_mask(const pluss::Series& validity_mask) const;
  Thread& with_validity_mask_(const pluss::Series& validity_mask);
  Thread clear_validity_mask() const;
  Thread& clear_validity_mask_();

  pluss::Series isnull() const;
  pluss::Series notnull() const;
  pluss::Series isnan() const;
  pluss::Series isfinite() const;

  // Functional-by-default transforms.
  Thread fillna(double value) const;
  Thread abs() const;
  Thread clip(double min_value, double max_value) const;
  Thread log() const;
  Thread exp() const;

  // Type-scoped full-surface accessors (delegate into underlying pluss types).
  const pluss::Series& series() const;
  const pluss::Note& note() const;
  const pluss::Timeline& timeline() const;
  const pluss::TimeIndex& timeindex() const;

  // Explicit mutating transforms (copy-on-write when shared).
  Thread& fillna_(double value);

  // 1D row-space transforms.
  Thread head(std::int64_t n) const;
  Thread tail(std::int64_t n) const;
  Thread slice(std::int64_t start, std::int64_t stop) const;
  Thread take(std::span<const std::int64_t> indices) const;
  Thread filter(const pluss::Series& bool_mask) const;
  Thread dropna() const;
  Thread ffill() const;
  Thread bfill() const;

  // Reductions (Series payload only) return scalar Series (numel == 1).
  pluss::Series sum() const;
  pluss::Series mean() const;
  pluss::Series min() const;
  pluss::Series max() const;
  pluss::Series var(bool unbiased = true) const;
  pluss::Series std(bool unbiased = true) const;

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
  int compare_rows(std::int64_t lhs_row, std::int64_t rhs_row, bool nulls_last = true) const;
  Thread window(std::int64_t window, ThreadWindowOp op, std::int64_t min_periods = 1) const;
  Thread cast(ThreadCastTarget target,
              std::optional<pluss::codem::ScalarType> series_dtype = std::nullopt) const;
  Thread astype(pluss::codem::ScalarType dtype) const;

  std::string to_string(const display::DisplayOptions& options = {}) const;
  void show(const display::DisplayOptions& options = {}) const;
  void show(std::ostream& os, const display::DisplayOptions& options = {}) const;

  const pluss::Series& as_series() const;
  const pluss::Note& as_note() const;
  const pluss::Timeline& as_timeline() const;
  const pluss::TimeIndex& as_timeindex() const;

private:
  explicit Thread(pluss::msharp::intrusive_ptr<detail::ThreadImpl> impl);

  detail::ThreadImpl& mut_impl_();
  const detail::ThreadImpl& impl_ref_() const;

  pluss::msharp::intrusive_ptr<detail::ThreadImpl> impl_{nullptr};
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
  ThreadValue() = default;
  explicit ThreadValue(Thread thread);

  bool defined() const noexcept;
  ThreadKind kind() const;
  const Thread& thread() const noexcept;

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

} // namespace pluss::table

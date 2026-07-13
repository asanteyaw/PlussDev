#pragma once

#include <cstdint>
#include <iosfwd>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>

#include "MSharp/IntrusivePtr.h"
#include "table/Thread.h"

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

enum class StreamKind : std::uint8_t {
  Series = 0,
  Note = 1,
  Timeline = 2,
  TimeIndex = 3,
};

class Stream {
public:
  Stream();
  Stream(const pluss::Series& data,
         std::optional<pluss::Series> validity_mask = std::nullopt,
         std::optional<pluss::table::Thread> cases = std::nullopt);
  Stream(const pluss::Note& data,
         std::optional<pluss::Series> validity_mask = std::nullopt,
         std::optional<pluss::table::Thread> cases = std::nullopt);
  Stream(const pluss::Timeline& data,
         std::optional<pluss::Series> validity_mask = std::nullopt,
         std::optional<pluss::table::Thread> cases = std::nullopt);
  Stream(const pluss::TimeIndex& data,
         std::optional<pluss::Series> validity_mask = std::nullopt,
         std::optional<pluss::table::Thread> cases = std::nullopt);
  Stream(const Stream& other);
  Stream(Stream&& other) noexcept;
  Stream& operator=(const Stream& other);
  Stream& operator=(Stream&& other) noexcept;
  ~Stream();

  static Stream from_thread(const pluss::table::Thread& thread,
                            std::optional<pluss::table::Thread> cases = std::nullopt);

  bool defined() const noexcept;
  bool unique() const noexcept;
  std::int64_t size() const;
  StreamKind kind() const;
  std::string dtype() const;

  // Stream-native missing/predicate helpers (delegates to Thread).
  Stream isnull() const;
  Stream notnull() const;

  // Stream-native transforms (delegates to Thread where applicable).
  Stream fillna(double value) const;
  Stream fillna(std::string_view value) const;
  Stream fillna(const char* value) const {
    return fillna(std::string_view{value != nullptr ? value : ""});
  }
  Stream fillna(const pluss::Timeline& value) const;
  Stream fillna(const pluss::TimeIndex& value) const;
  Stream abs() const;
  Stream clip(double min_value, double max_value) const;
  Stream log() const;
  Stream exp() const;

  bool has_cases() const;
  const std::optional<pluss::table::Thread>& cases() const;
  Stream with_cases(const pluss::table::Thread& cases) const;
  Stream& with_cases_(const pluss::table::Thread& cases);
  Stream clear_cases() const;
  Stream& clear_cases_();

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

  pluss::table::Thread to_thread() const;
  pluss::table::Thread to_thread(std::int64_t start, std::int64_t stop) const;

  // 1D row-space operations.
  Stream head(std::int64_t n) const;
  Stream tail(std::int64_t n) const;
  Stream slice(std::int64_t start, std::int64_t stop) const;
  Stream take(std::span<const std::int64_t> indices) const;
  Stream filter(const pluss::Series& bool_mask) const;
  Stream dropna() const;
  Stream ffill() const;
  Stream bfill() const;

  // Type conversion.
  Stream cast(pluss::table::ThreadCastTarget target,
              std::optional<pluss::codem::ScalarType> series_dtype = std::nullopt) const;
  Stream astype(pluss::codem::ScalarType dtype) const;

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
  const pluss::TimeIndex& as_timeindex() const;

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

} // namespace pluss::kindle

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

  bool has_validity_mask() const;
  const std::optional<pluss::Series>& validity_mask() const;
  Thread with_validity_mask(const pluss::Series& validity_mask) const;
  Thread& with_validity_mask_(const pluss::Series& validity_mask);
  Thread clear_validity_mask() const;
  Thread& clear_validity_mask_();

  pluss::Series isnull() const;
  pluss::Series isnan() const;
  pluss::Series isfinite() const;

  // Functional-by-default transforms.
  Thread fillna(double value) const;
  Thread abs() const;
  Thread clip(double min_value, double max_value) const;

  // Explicit mutating transforms (copy-on-write when shared).
  Thread& fillna_(double value);

  // 1D row-space transforms.
  Thread head(std::int64_t n) const;
  Thread tail(std::int64_t n) const;
  Thread slice(std::int64_t start, std::int64_t stop) const;
  Thread take(std::span<const std::int64_t> indices) const;
  Thread filter(const pluss::Series& bool_mask) const;

  // Reductions (Series payload only).
  double sum() const;
  double mean() const;
  double min() const;
  double max() const;
  double var(bool unbiased = true) const;
  double std(bool unbiased = true) const;

  // Text operations (Note payload only).
  Thread lower() const;
  pluss::Series contains(std::string_view needle) const;

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

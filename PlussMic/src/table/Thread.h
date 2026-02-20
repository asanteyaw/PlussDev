#pragma once

#include <cstdint>
#include <iosfwd>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "MSharp/IntrusivePtr.h"
#include "table/display/Display.h"
#include "pluss/codem/codem.h"
#include "pluss/coden/coden.h"
#include "pluss/codeh/codeh.h"

namespace pluss::table {

enum class ThreadKind : std::uint8_t {
  Series = 0,
  Note = 1,
  Timeline = 2,
  TimeIndex = 3,
};

using ThreadPayload = std::variant<pluss::Series, pluss::Note, pluss::Timeline, pluss::TimeIndex>;

class ThreadImpl final : public pluss::msharp::RefCounted {
public:
  ThreadImpl(ThreadPayload payload, std::optional<pluss::Series> validity_mask);

  ThreadPayload payload_;
  ThreadKind kind_{ThreadKind::Series};
  std::int64_t size_{0};
  std::string dtype_name_;
  std::optional<pluss::Series> validity_mask_;
};

class Thread {
public:
  Thread() = default;
  Thread(const pluss::Series& data, std::optional<pluss::Series> validity_mask = std::nullopt);
  Thread(const pluss::Note& data, std::optional<pluss::Series> validity_mask = std::nullopt);
  Thread(const pluss::Timeline& data, std::optional<pluss::Series> validity_mask = std::nullopt);
  Thread(const pluss::TimeIndex& data, std::optional<pluss::Series> validity_mask = std::nullopt);

  bool defined() const noexcept;
  bool unique() const noexcept;
  std::int64_t size() const;
  ThreadKind kind() const;
  std::string dtype_name() const;

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
  explicit Thread(pluss::msharp::intrusive_ptr<ThreadImpl> impl);

  ThreadImpl& mut_impl_();
  const ThreadImpl& impl_ref_() const;

  pluss::msharp::intrusive_ptr<ThreadImpl> impl_{nullptr};
};

std::ostream& operator<<(std::ostream& os, const Thread& thread);

} // namespace pluss::table

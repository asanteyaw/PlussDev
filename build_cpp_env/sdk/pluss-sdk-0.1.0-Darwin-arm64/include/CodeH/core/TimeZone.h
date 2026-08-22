#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pluss::codeh {

// Minimal, extensible timezone engine.
// - Timeline ticks are always UTC epoch nanos.
// - TimeZone provides offset minutes for a given instant and localize rules.
//
// This implementation ships with:
//   - "UTC"
//   - "America/Edmonton" (DST rules, 2007+ style)
//   - "America/New_York" (DST rules, 2007+ style)
//   - "Europe/London" (UK/EU DST rules)
//
// The API is deliberately small so we can later swap in a real tzdb.
class TimeZone {
public:
  static TimeZone load(std::string_view name);

  std::string_view name() const noexcept { return name_; }

  // Offset minutes east of UTC at a given UTC instant.
  // Example: -420 for America/Edmonton standard time.
  int offset_minutes_at_utc(std::int64_t utc_epoch_nanos) const;

  // Convert a naive local wall time (epoch nanos interpreted in local clock)
  // into a UTC epoch nanos.
  //
  // fold: for ambiguous times (fall-back overlap):
  //   0 => earlier (DST side), 1 => later (standard side)
  // nonexistent: "raise" or "shift_forward" (spring-forward gap)
  std::int64_t local_to_utc(std::int64_t local_epoch_nanos, int fold, std::string_view nonexistent) const;

private:
  explicit TimeZone(std::string name) : name_(std::move(name)) {}

  // Implementation currently supports UTC, America/Edmonton,
  // America/New_York, and Europe/London.
  std::string name_;
};

} // namespace pluss::codeh

#pragma once

#include "pluss/ai/lymph/common.h"

#include <cstdint>
#include <string>

// time.h -- time-role metadata for leakage-aware classical ML workflows.
//
// The current implementation uses row-step offsets because NodeJ's
// TSCrossVal kernel is sample-index based today. The value type is explicit
// so Workflow/Payload/Validate can share one contract now and NodeJ can later
// upgrade it to calendar-aware Timeline index semantics without changing
// the public family split.

namespace pluss::ai::lymph::workflow::time {

using TimeOffset = std::int64_t;

[[nodiscard]] constexpr TimeOffset steps(std::int64_t value) noexcept { return value; }
[[nodiscard]] constexpr TimeOffset days(std::int64_t value) noexcept { return value; }

struct TimeOptions {
  std::string index{};
  std::string observed_at{};
  std::string target_at{};
  TimeOffset horizon = 0;
  TimeOffset gap = 0;
  TimeOffset lookback = 0;
  bool require_ordered = true;
  bool reject_missing_index = true;
};

[[nodiscard]] inline bool has_time_index(const TimeOptions& options) noexcept {
  return !options.index.empty();
}

inline void validate_time_options(const TimeOptions& options, const char* where) {
  if (options.horizon < 0) {
    ::pluss::throw_error(std::string(where) + ": horizon cannot be negative.");
  }
  if (options.gap < 0) {
    ::pluss::throw_error(std::string(where) + ": gap cannot be negative.");
  }
  if (options.lookback < 0) {
    ::pluss::throw_error(std::string(where) + ": lookback cannot be negative.");
  }
  if (options.reject_missing_index && options.index.empty()) {
    ::pluss::throw_error(std::string(where) + ": time index cannot be empty when reject_missing_index is true.");
  }
}

} // namespace pluss::ai::lymph::workflow::time

namespace pluss::ai::lymph::workflow {

using TimeOffset = time::TimeOffset;
using TimeOptions = time::TimeOptions;
using time::days;
using time::has_time_index;
using time::steps;
using time::validate_time_options;

} // namespace pluss::ai::lymph::workflow

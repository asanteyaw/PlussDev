#pragma once

#include <cstdint>
#include <limits>
#include <stdexcept>

namespace pluss::codeh::detail {

// A difference of two int64 ticks, or one tick adjusted by a timezone offset,
// may exceed INT64_MAX but still fits in an unsigned 64-bit magnitude.
// Dividing that magnitude directly avoids ClangCL's unavailable __divti3.
inline __int128 divide_bounded_wide_by_u64(__int128 numerator, std::uint64_t denominator) {
  if (denominator == 0) {
    throw std::invalid_argument("wide division by zero");
  }
  const bool negative = numerator < 0;
  const auto magnitude = negative
      ? static_cast<unsigned __int128>(-(numerator + 1)) + 1
      : static_cast<unsigned __int128>(numerator);
  if (magnitude > (std::numeric_limits<std::uint64_t>::max)()) {
    throw std::overflow_error("wide tick magnitude exceeds uint64");
  }
  const auto quotient = static_cast<std::uint64_t>(magnitude) / denominator;
  return negative ? -static_cast<__int128>(quotient) : static_cast<__int128>(quotient);
}

} // namespace pluss::codeh::detail

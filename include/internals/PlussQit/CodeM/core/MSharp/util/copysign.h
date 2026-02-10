#pragma once

#include <MSharp/util/BFloat16.h>
#include <MSharp/util/Half.h>

namespace MSharp {

// Note: Explicit implementation of copysign for Half and BFloat16
// is needed to workaround g++-7/8 crash on aarch64, but also makes
// copysign faster for the half-precision types
template <typename T, typename U>
inline auto copysign(const T& a, const U& b) {
  return std::copysign(a, b);
}

// Implement copysign for half precision floats using bit ops
// Sign is the most significant bit for both half and bfloat16 types
inline MSharp::Half copysign(MSharp::Half a, MSharp::Half b) {
  return MSharp::Half((a.x & 0x7fff) | (b.x & 0x8000), MSharp::Half::from_bits());
}

inline MSharp::BFloat16 copysign(MSharp::BFloat16 a, MSharp::BFloat16 b) {
  return MSharp::BFloat16(
      (a.x & 0x7fff) | (b.x & 0x8000), MSharp::BFloat16::from_bits());
}

} // namespace MSharp

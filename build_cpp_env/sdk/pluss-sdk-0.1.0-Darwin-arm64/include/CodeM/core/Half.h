#pragma once
#include <cstdint>
#include <cstring>
#include "CodeM/core/Float8.h"

namespace pluss::codem {

// Storage wrappers (CPU). Compute typically upcasts to float32.
struct Float16 final { std::uint16_t x{}; };
struct BFloat16 final { std::uint16_t x{}; };

// ---- float32 <-> bfloat16 (round-to-nearest-even) ----
inline BFloat16 float_to_bfloat16(float f) noexcept {
  std::uint32_t u;
  std::memcpy(&u, &f, sizeof(u));
  // RNE: add 0x7FFF + LSB of upper16
  const std::uint32_t lsb = (u >> 16) & 1u;
  u += 0x7FFFu + lsb;
  return BFloat16{static_cast<std::uint16_t>(u >> 16)};
}

inline float bfloat16_to_float(BFloat16 b) noexcept {
  std::uint32_t u = static_cast<std::uint32_t>(b.x) << 16;
  float f;
  std::memcpy(&f, &u, sizeof(f));
  return f;
}

// Compatibility aliases used in some kernels/older call sites.
inline BFloat16 bfloat16_from_float(float f) noexcept { return float_to_bfloat16(f); }

// ---- float32 <-> IEEE754 half (float16) ----
// Reference-style conversion (no intrinsics), RNE.
inline Float16 float_to_float16(float f) noexcept {
  std::uint32_t x;
  std::memcpy(&x, &f, sizeof(x));

  const std::uint32_t sign = (x >> 16) & 0x8000u;
  const std::uint32_t mant = x & 0x007FFFFFu;
  const std::int32_t exp = static_cast<std::int32_t>((x >> 23) & 0xFFu);

  // NaN/Inf
  if (exp == 255) {
    if (mant == 0) return Float16{static_cast<std::uint16_t>(sign | 0x7C00u)};
    // quiet NaN
    std::uint16_t nan = static_cast<std::uint16_t>(sign | 0x7E00u);
    return Float16{nan};
  }

  // exponent adjust
  std::int32_t half_exp = exp - 127 + 15;

  if (half_exp >= 31) {
    // overflow -> inf
    return Float16{static_cast<std::uint16_t>(sign | 0x7C00u)};
  }

  if (half_exp <= 0) {
    // subnormal or underflow to zero
    if (half_exp < -10) {
      return Float16{static_cast<std::uint16_t>(sign)};
    }
    // subnormal: mantissa with hidden leading 1
    std::uint32_t m = mant | 0x00800000u;
    const std::int32_t shift = 1 - half_exp;
    // RNE on shift
    const std::uint32_t round_bit = 1u << (shift - 1);
    const std::uint32_t sticky = (m & (round_bit - 1)) != 0;
    std::uint32_t mm = (m >> shift);
    const std::uint32_t rb = (m & round_bit) != 0;
    if (rb && (sticky || (mm & 1u))) mm += 1u;
    return Float16{static_cast<std::uint16_t>(sign | (mm & 0x03FFu))};
  }

  // normal
  std::uint32_t mm = mant;
  // RNE: take top 10 bits of mantissa (23 -> 10)
  const std::uint32_t round_mask = 0x00001FFFu; // lower 13 bits
  const std::uint32_t rb = (mm >> 12) & 1u;
  const std::uint32_t rest = mm & round_mask;
  std::uint32_t half_mant = mm >> 13;
  if (rb && (rest != 0 || (half_mant & 1u))) {
    half_mant += 1u;
    if (half_mant == 0x0400u) {
      // mantissa overflow
      half_mant = 0;
      half_exp += 1;
      if (half_exp >= 31) {
        return Float16{static_cast<std::uint16_t>(sign | 0x7C00u)};
      }
    }
  }

  return Float16{static_cast<std::uint16_t>(sign | (static_cast<std::uint32_t>(half_exp) << 10) | (half_mant & 0x03FFu))};
}

// Compatibility alias.
inline Float16 float16_from_float(float f) noexcept { return float_to_float16(f); }

inline float float16_to_float(Float16 h) noexcept {
  const std::uint32_t sign = static_cast<std::uint32_t>(h.x & 0x8000u) << 16;
  const std::uint32_t exp  = (h.x >> 10) & 0x1Fu;
  const std::uint32_t mant = h.x & 0x03FFu;

  std::uint32_t out;
  if (exp == 0) {
    if (mant == 0) {
      out = sign;
    } else {
      // subnormal -> normalize
      std::uint32_t m = mant;
      std::int32_t e = -1;
      do { e++; m <<= 1; } while ((m & 0x0400u) == 0);
      m &= 0x03FFu;
      const std::uint32_t exp32 = static_cast<std::uint32_t>(127 - 15 - e);
      out = sign | (exp32 << 23) | (m << 13);
    }
  } else if (exp == 31) {
    // inf/nan
    out = sign | 0x7F800000u | (mant << 13);
  } else {
    const std::uint32_t exp32 = exp + (127 - 15);
    out = sign | (exp32 << 23) | (mant << 13);
  }

  float f;
  std::memcpy(&f, &out, sizeof(f));
  return f;
}

} // namespace pluss::codem

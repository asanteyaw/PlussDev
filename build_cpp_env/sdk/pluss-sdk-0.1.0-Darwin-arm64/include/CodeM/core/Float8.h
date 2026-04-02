#pragma once
#include <cstdint>
#include <cmath>

namespace pluss::codem {

// Two float8 storage formats (CPU). Compute policy: upcast to float32 for math kernels.
// These are *storage* types; exact arithmetic follows float32 after conversion.

struct Float8E4M3FN final { std::uint8_t x{}; }; // E4M3FN (no inf; NaN supported)
struct Float8E5M2 final { std::uint8_t x{}; };   // E5M2 (inf + NaN supported)

inline float float8e4m3fn_to_float(Float8E4M3FN v) noexcept {
  // Layout: 1 sign, 4 exponent (bias 7), 3 mantissa. "FN" means no inf; max exponent encodes NaN.
  const std::uint8_t b = v.x;
  const std::uint8_t s = (b >> 7) & 0x1;
  const std::uint8_t e = (b >> 3) & 0xF;
  const std::uint8_t m = b & 0x7;
  if (e == 0) {
    if (m == 0) return s ? -0.0f : 0.0f;
    // subnormal: exponent = 1-bias
    const float frac = (float)m / 8.0f;
    float out = std::ldexp(frac, -6); // 2^(1-bias) * (m/8), bias=7 -> 2^-6
    return s ? -out : out;
  }
  if (e == 0xF) {
    // No inf in this format; treat as NaN
    return std::nanf("");
  }
  // normal
  const int exp = (int)e - 7;
  const float frac = 1.0f + (float)m / 8.0f;
  float out = std::ldexp(frac, exp);
  return s ? -out : out;
}

inline Float8E4M3FN float8e4m3fn_from_float(float f) noexcept {
  if (std::isnan(f)) return Float8E4M3FN{(std::uint8_t)0x7F};
  // This format has no inf; clamp large values to max finite.
  const std::uint8_t sign = std::signbit(f) ? 0x80 : 0x00;
  float af = std::fabs(f);
  if (af == 0.0f) return Float8E4M3FN{sign};
  // Find exponent such that af = frac * 2^exp with frac in [1,2)
  int exp;
  float frac = std::frexp(af, &exp); // af = frac * 2^exp, frac in [0.5,1)
  frac *= 2.0f;
  exp -= 1;
  // bias=7
  int e = exp + 7;
  if (e <= 0) {
    // subnormal
    // value = (m/8) * 2^-6  => m = round(value * 8 * 2^6)
    int m = (int)std::lrint(af * 512.0f); // 8*64 = 512
    if (m <= 0) return Float8E4M3FN{sign};
    if (m > 7) m = 7;
    return Float8E4M3FN{ (std::uint8_t)(sign | (std::uint8_t)m) };
  }
  if (e >= 0xF) {
    // clamp to max finite (e=0xE, m=0x7)
    return Float8E4M3FN{ (std::uint8_t)(sign | (0xE << 3) | 0x7) };
  }
  int m = (int)std::lrint((frac - 1.0f) * 8.0f);
  if (m < 0) m = 0;
  if (m > 7) m = 7;
  return Float8E4M3FN{ (std::uint8_t)(sign | ((std::uint8_t)e << 3) | (std::uint8_t)m) };
}

inline float float8e5m2_to_float(Float8E5M2 v) noexcept {
  // Layout: 1 sign, 5 exponent (bias 15), 2 mantissa.
  const std::uint8_t b = v.x;
  const std::uint8_t s = (b >> 7) & 0x1;
  const std::uint8_t e = (b >> 2) & 0x1F;
  const std::uint8_t m = b & 0x3;
  if (e == 0) {
    if (m == 0) return s ? -0.0f : 0.0f;
    // subnormal exponent = 1-bias = -14
    const float frac = (float)m / 4.0f;
    float out = std::ldexp(frac, -14);
    return s ? -out : out;
  }
  if (e == 0x1F) {
    if (m == 0) return s ? -INFINITY : INFINITY;
    return std::nanf("");
  }
  const int exp = (int)e - 15;
  const float frac = 1.0f + (float)m / 4.0f;
  float out = std::ldexp(frac, exp);
  return s ? -out : out;
}

inline Float8E5M2 float8e5m2_from_float(float f) noexcept {
  const std::uint8_t sign = std::signbit(f) ? 0x80 : 0x00;
  if (std::isnan(f)) return Float8E5M2{(std::uint8_t)(sign | 0x7D)};
  if (std::isinf(f)) return Float8E5M2{(std::uint8_t)(sign | 0x7C)};
  float af = std::fabs(f);
  if (af == 0.0f) return Float8E5M2{sign};
  int exp;
  float frac = std::frexp(af, &exp);
  frac *= 2.0f;
  exp -= 1;
  int e = exp + 15;
  if (e <= 0) {
    int m = (int)std::lrint(af * std::ldexp(1.0f, 16)); // value * 2^(bias-1+mantbits)=2^(14+2)=2^16 times /4? approx
    m = (m + 1) / 2; // rough scale to 2 mant bits
    if (m <= 0) return Float8E5M2{sign};
    if (m > 3) m = 3;
    return Float8E5M2{(std::uint8_t)(sign | (std::uint8_t)m)};
  }
  if (e >= 0x1F) return Float8E5M2{(std::uint8_t)(sign | 0x7C)}; // inf
  int m = (int)std::lrint((frac - 1.0f) * 4.0f);
  if (m < 0) m = 0;
  if (m > 3) m = 3;
  return Float8E5M2{ (std::uint8_t)(sign | ((std::uint8_t)e << 2) | (std::uint8_t)m) };
}

} // namespace pluss::codem

#pragma once

#include <CodeM/core/Errors.h>
#include <CodeM/core/Complex.h>
#include <CodeM/core/ScalarType.h>
#include <CodeM/core/Series.h>
#include <pluss/codem/series_factory.h>
#include <pluss/config.h>

#include <complex>
#include <cstdint>
#include <type_traits>

namespace pluss::aux {

inline Config as_dtype(Config cfg, ScalarType dtype) {
  return Config{}.with_dtype(dtype).with_platform(cfg.platform).with_pinned(cfg.pinned);
}

inline Series as_dtype(const Series& x, ScalarType dtype, Config cfg = {}) {
  PLUSS_CHECK(x.defined(), "pluss::as_dtype: expected defined Series");
  return x.to(cfg.platform, dtype).contiguous();
}

inline Series as_dtype(double value, ScalarType dtype, Config cfg = {}) {
  return full({}, value, as_dtype(cfg, dtype));
}

inline Series as_c64(const codem::Complex64& value, Config cfg = {}) {
  const auto real = full({}, static_cast<double>(value.real()), as_dtype(cfg, ScalarType::Float32));
  const auto imag = full({}, static_cast<double>(value.imag()), as_dtype(cfg, ScalarType::Float32));
  return complex(real, imag, ScalarType::Complex64).to(cfg.platform);
}

inline Series as_c128(const codem::Complex128& value, Config cfg = {}) {
  const auto real = full({}, value.real(), as_dtype(cfg, ScalarType::Float64));
  const auto imag = full({}, value.imag(), as_dtype(cfg, ScalarType::Float64));
  return complex(real, imag, ScalarType::Complex128).to(cfg.platform);
}

inline Config as_bool(Config cfg = {}) { return as_dtype(cfg, ScalarType::Bool); }
inline Config as_f32(Config cfg = {}) { return as_dtype(cfg, ScalarType::Float32); }
inline Config as_f64(Config cfg = {}) { return as_dtype(cfg, ScalarType::Float64); }
inline Config as_i8(Config cfg = {}) { return as_dtype(cfg, ScalarType::Int8); }
inline Config as_i16(Config cfg = {}) { return as_dtype(cfg, ScalarType::Int16); }
inline Config as_i32(Config cfg = {}) { return as_dtype(cfg, ScalarType::Int32); }
inline Config as_i64(Config cfg = {}) { return as_dtype(cfg, ScalarType::Int64); }
inline Config as_u8(Config cfg = {}) { return as_dtype(cfg, ScalarType::UInt8); }
inline Config as_u16(Config cfg = {}) { return as_dtype(cfg, ScalarType::UInt16); }
inline Config as_u32(Config cfg = {}) { return as_dtype(cfg, ScalarType::UInt32); }
inline Config as_u64(Config cfg = {}) { return as_dtype(cfg, ScalarType::UInt64); }
inline Config as_c64(Config cfg = {}) { return as_dtype(cfg, ScalarType::Complex64); }
inline Config as_c128(Config cfg = {}) { return as_dtype(cfg, ScalarType::Complex128); }

inline Series as_bool(const Series& x, Config cfg) { return as_dtype(x, ScalarType::Bool, cfg); }
inline Series as_f32(const Series& x, Config cfg) { return as_dtype(x, ScalarType::Float32, cfg); }
inline Series as_f64(const Series& x, Config cfg) { return as_dtype(x, ScalarType::Float64, cfg); }
inline Series as_i8(const Series& x, Config cfg) { return as_dtype(x, ScalarType::Int8, cfg); }
inline Series as_i16(const Series& x, Config cfg) { return as_dtype(x, ScalarType::Int16, cfg); }
inline Series as_i32(const Series& x, Config cfg) { return as_dtype(x, ScalarType::Int32, cfg); }
inline Series as_i64(const Series& x, Config cfg) { return as_dtype(x, ScalarType::Int64, cfg); }
inline Series as_u8(const Series& x, Config cfg) { return as_dtype(x, ScalarType::UInt8, cfg); }
inline Series as_u16(const Series& x, Config cfg) { return as_dtype(x, ScalarType::UInt16, cfg); }
inline Series as_u32(const Series& x, Config cfg) { return as_dtype(x, ScalarType::UInt32, cfg); }
inline Series as_u64(const Series& x, Config cfg) { return as_dtype(x, ScalarType::UInt64, cfg); }
inline Series as_c64(const Series& x, Config cfg) { return as_dtype(x, ScalarType::Complex64, cfg); }
inline Series as_c128(const Series& x, Config cfg) { return as_dtype(x, ScalarType::Complex128, cfg); }

inline Series as_bool(bool value, Config cfg) { return as_dtype(static_cast<double>(value), ScalarType::Bool, cfg); }
inline Series as_f32(float value, Config cfg) { return as_dtype(static_cast<double>(value), ScalarType::Float32, cfg); }
inline Series as_f64(double value, Config cfg) { return as_dtype(value, ScalarType::Float64, cfg); }
inline Series as_i8(std::int8_t value, Config cfg) { return as_dtype(static_cast<double>(value), ScalarType::Int8, cfg); }
inline Series as_i16(std::int16_t value, Config cfg) { return as_dtype(static_cast<double>(value), ScalarType::Int16, cfg); }
inline Series as_i32(std::int32_t value, Config cfg) { return as_dtype(static_cast<double>(value), ScalarType::Int32, cfg); }
inline Series as_i64(std::int64_t value, Config cfg) { return as_dtype(static_cast<double>(value), ScalarType::Int64, cfg); }
inline Series as_u8(std::uint8_t value, Config cfg) { return as_dtype(static_cast<double>(value), ScalarType::UInt8, cfg); }
inline Series as_u16(std::uint16_t value, Config cfg) { return as_dtype(static_cast<double>(value), ScalarType::UInt16, cfg); }
inline Series as_u32(std::uint32_t value, Config cfg) { return as_dtype(static_cast<double>(value), ScalarType::UInt32, cfg); }
inline Series as_u64(std::uint64_t value, Config cfg) { return as_dtype(static_cast<double>(value), ScalarType::UInt64, cfg); }

namespace detail {
template <class T>
struct is_scalar_extractable
    : std::bool_constant<std::is_arithmetic_v<T> ||
                         std::is_same_v<T, codem::Complex64> ||
                         std::is_same_v<T, codem::Complex128>> {};
} // namespace detail

template <class T>
inline constexpr bool is_scalar_extractable_v = detail::is_scalar_extractable<T>::value;

inline codem::Complex64 as_c64(const Series& x);
inline codem::Complex128 as_c128(const Series& x);

inline codem::Complex64 as_C64(const Series& x) { return as_c64(x); }
inline codem::Complex128 as_C128(const Series& x) { return as_c128(x); }

inline Series as_C64(const Series& x, Config cfg) { return as_c64(x, cfg); }
inline Series as_C128(const Series& x, Config cfg) { return as_c128(x, cfg); }

inline Config as_C64(Config cfg = {}) { return as_c64(cfg); }
inline Config as_C128(Config cfg = {}) { return as_c128(cfg); }

inline Series as_C64(std::complex<float> value, Config cfg) { return as_c64(value, cfg); }
inline Series as_C128(std::complex<double> value, Config cfg) { return as_c128(value, cfg); }

template <class T>
T to_scalar(const Series& x) {
  static_assert(is_scalar_extractable_v<T>, "pluss::to_scalar<T>: T must be an arithmetic or CodeM complex scalar type");
  PLUSS_CHECK(x.defined(), "pluss::to_scalar<T>: expected defined Series");
  PLUSS_CHECK(x.numel() == 1, "pluss::to_scalar<T>: expected scalar Series (numel==1)");
  return x.item<T>();
}

inline double as_f64(const Series& x) { return to_scalar<double>(x); }
inline float as_f32(const Series& x) { return to_scalar<float>(x); }
inline bool as_bool(const Series& x) { return to_scalar<bool>(x); }
inline std::int8_t as_i8(const Series& x) { return to_scalar<std::int8_t>(x); }
inline std::int16_t as_i16(const Series& x) { return to_scalar<std::int16_t>(x); }
inline std::int32_t as_i32(const Series& x) { return to_scalar<std::int32_t>(x); }
inline std::int64_t as_i64(const Series& x) { return to_scalar<std::int64_t>(x); }
inline std::uint8_t as_u8(const Series& x) { return to_scalar<std::uint8_t>(x); }
inline std::uint16_t as_u16(const Series& x) { return to_scalar<std::uint16_t>(x); }
inline std::uint32_t as_u32(const Series& x) { return to_scalar<std::uint32_t>(x); }
inline std::uint64_t as_u64(const Series& x) { return to_scalar<std::uint64_t>(x); }
inline codem::Complex64 as_c64(const Series& x) { return to_scalar<codem::Complex64>(x); }
inline codem::Complex128 as_c128(const Series& x) { return to_scalar<codem::Complex128>(x); }

// Upper-case numeric aliases for teams that prefer this style.
inline std::int8_t as_I8(const Series& x) { return as_i8(x); }
inline std::int16_t as_I16(const Series& x) { return as_i16(x); }
inline std::int32_t as_I32(const Series& x) { return as_i32(x); }
inline std::int64_t as_I64(const Series& x) { return as_i64(x); }
inline std::uint8_t as_U8(const Series& x) { return as_u8(x); }
inline std::uint16_t as_U16(const Series& x) { return as_u16(x); }
inline std::uint32_t as_U32(const Series& x) { return as_u32(x); }
inline std::uint64_t as_U64(const Series& x) { return as_u64(x); }

inline Series as_I8(const Series& x, Config cfg) { return as_i8(x, cfg); }
inline Series as_I16(const Series& x, Config cfg) { return as_i16(x, cfg); }
inline Series as_I32(const Series& x, Config cfg) { return as_i32(x, cfg); }
inline Series as_I64(const Series& x, Config cfg) { return as_i64(x, cfg); }
inline Series as_U8(const Series& x, Config cfg) { return as_u8(x, cfg); }
inline Series as_U16(const Series& x, Config cfg) { return as_u16(x, cfg); }
inline Series as_U32(const Series& x, Config cfg) { return as_u32(x, cfg); }
inline Series as_U64(const Series& x, Config cfg) { return as_u64(x, cfg); }

inline Config as_I8(Config cfg = {}) { return as_i8(cfg); }
inline Config as_I16(Config cfg = {}) { return as_i16(cfg); }
inline Config as_I32(Config cfg = {}) { return as_i32(cfg); }
inline Config as_I64(Config cfg = {}) { return as_i64(cfg); }
inline Config as_U8(Config cfg = {}) { return as_u8(cfg); }
inline Config as_U16(Config cfg = {}) { return as_u16(cfg); }
inline Config as_U32(Config cfg = {}) { return as_u32(cfg); }
inline Config as_U64(Config cfg = {}) { return as_u64(cfg); }

inline Series as_I8(std::int8_t value, Config cfg) { return as_i8(value, cfg); }
inline Series as_I16(std::int16_t value, Config cfg) { return as_i16(value, cfg); }
inline Series as_I32(std::int32_t value, Config cfg) { return as_i32(value, cfg); }
inline Series as_I64(std::int64_t value, Config cfg) { return as_i64(value, cfg); }
inline Series as_U8(std::uint8_t value, Config cfg) { return as_u8(value, cfg); }
inline Series as_U16(std::uint16_t value, Config cfg) { return as_u16(value, cfg); }
inline Series as_U32(std::uint32_t value, Config cfg) { return as_u32(value, cfg); }
inline Series as_U64(std::uint64_t value, Config cfg) { return as_u64(value, cfg); }

} // namespace pluss::aux

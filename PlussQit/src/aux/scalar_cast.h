#pragma once

#include <CodeM/core/Errors.h>
#include <CodeM/core/Series.h>

#include <cstdint>
#include <type_traits>

namespace pluss::aux {

template <class T>
T to_scalar(const Series& x) {
  static_assert(std::is_arithmetic_v<T>, "pluss::to_scalar<T>: T must be an arithmetic type");
  PLUSS_CHECK(x.defined(), "pluss::to_scalar<T>: expected defined Series");
  PLUSS_CHECK(x.numel() == 1, "pluss::to_scalar<T>: expected scalar Series (numel==1)");
  return x.item<T>();
}

inline double as_f64(const Series& x) { return to_scalar<double>(x); }
inline float as_f32(const Series& x) { return to_scalar<float>(x); }
inline bool as_bool(const Series& x) { return to_scalar<bool>(x); }
inline std::int16_t as_i16(const Series& x) { return to_scalar<std::int16_t>(x); }
inline std::int32_t as_i32(const Series& x) { return to_scalar<std::int32_t>(x); }
inline std::int64_t as_i64(const Series& x) { return to_scalar<std::int64_t>(x); }
inline std::uint16_t as_u16(const Series& x) { return to_scalar<std::uint16_t>(x); }
inline std::uint32_t as_u32(const Series& x) { return to_scalar<std::uint32_t>(x); }
inline std::uint64_t as_u64(const Series& x) { return to_scalar<std::uint64_t>(x); }

// Upper-case numeric aliases for teams that prefer this style.
inline std::int16_t as_I16(const Series& x) { return as_i16(x); }
inline std::int32_t as_I32(const Series& x) { return as_i32(x); }
inline std::int64_t as_I64(const Series& x) { return as_i64(x); }
inline std::uint16_t as_U16(const Series& x) { return as_u16(x); }
inline std::uint32_t as_U32(const Series& x) { return as_u32(x); }
inline std::uint64_t as_U64(const Series& x) { return as_u64(x); }

} // namespace pluss::aux

#pragma once
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pluss::codem {

// CPU-only scalar types. Mirrors torch's ScalarType set in spirit.
// Float8 types are included as storage formats; compute paths can upcast later.
enum class ScalarType : std::uint8_t {
  Bool,
  UInt8,
  UInt16,
  UInt32,
  UInt64,
  Int8,
  Int16,
  Int32,
  Int64,
  Float16,
  BFloat16,
  Float32,
  Float64,
  Float8E4M3FN,
  Float8E5M2,
  Complex64,
  Complex128,
};

struct DType final {
  ScalarType scalar{};
  std::size_t itemsize{};
  std::string_view name{};

  constexpr bool is_floating() const noexcept {
    switch (scalar) {
      case ScalarType::Float16:
      case ScalarType::BFloat16:
      case ScalarType::Float32:
      case ScalarType::Float64:
      case ScalarType::Float8E4M3FN:
      case ScalarType::Float8E5M2:
        return true;
      default:
        return false;
    }
  }

  constexpr bool is_complex() const noexcept {
    return scalar == ScalarType::Complex64 || scalar == ScalarType::Complex128;
  }

  constexpr bool is_integral() const noexcept {
    switch (scalar) {
      case ScalarType::UInt8:
      case ScalarType::UInt16:
      case ScalarType::UInt32:
      case ScalarType::UInt64:
      case ScalarType::Int8:
      case ScalarType::Int16:
      case ScalarType::Int32:
      case ScalarType::Int64:
        return true;
      default:
        return false;
    }
  }
};

constexpr DType dtype_of(ScalarType st) noexcept {
  switch (st) {
    case ScalarType::Bool:         return {st, 1, "bool"};
    case ScalarType::UInt8:        return {st, 1, "uint8"};
    case ScalarType::UInt16:       return {st, 2, "uint16"};
    case ScalarType::UInt32:       return {st, 4, "uint32"};
    case ScalarType::UInt64:       return {st, 8, "uint64"};
    case ScalarType::Int8:         return {st, 1, "int8"};
    case ScalarType::Int16:        return {st, 2, "int16"};
    case ScalarType::Int32:        return {st, 4, "int32"};
    case ScalarType::Int64:        return {st, 8, "int64"};
    case ScalarType::Float16:      return {st, 2, "float16"};
    case ScalarType::BFloat16:     return {st, 2, "bfloat16"};
    case ScalarType::Float32:      return {st, 4, "float32"};
    case ScalarType::Float64:      return {st, 8, "float64"};
    case ScalarType::Float8E4M3FN: return {st, 1, "float8_e4m3fn"};
    case ScalarType::Float8E5M2:   return {st, 1, "float8_e5m2"};
    case ScalarType::Complex64:    return {st, 8, "complex64"};
    case ScalarType::Complex128:   return {st, 16, "complex128"};
    default:                       return {ScalarType::Float32, 4, "float32"};
  }
}


} // namespace pluss::codem

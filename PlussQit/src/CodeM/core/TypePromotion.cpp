#include "CodeM/core/TypePromotion.h"
#include "CodeM/core/Errors.h"

namespace pluss::codem {

static bool is_bool(ScalarType t){ return t == ScalarType::Bool; }
static bool is_uint(ScalarType t){ return t == ScalarType::UInt8; }
static bool is_signed_int(ScalarType t){
  return t == ScalarType::Int8 || t == ScalarType::Int16 || t == ScalarType::Int32 || t == ScalarType::Int64;
}
static bool is_integral(ScalarType t){
  return is_signed_int(t) || is_uint(t) || is_bool(t);
}
static bool is_floating(ScalarType t){
  return t == ScalarType::Float16 || t == ScalarType::BFloat16 || t == ScalarType::Float32 || t == ScalarType::Float64 ||
         t == ScalarType::Float8E4M3FN || t == ScalarType::Float8E5M2;
}
static bool is_complex(ScalarType t){
  return t == ScalarType::Complex64 || t == ScalarType::Complex128;
}

static int float_rank(ScalarType t){
  switch (t) {
    case ScalarType::Float8E4M3FN: return 0;
    case ScalarType::Float8E5M2: return 0;
    case ScalarType::Float16: return 1;
    case ScalarType::BFloat16: return 1;
    case ScalarType::Float32: return 2;
    case ScalarType::Float64: return 3;
    default: return -1;
  }
}
static int int_rank(ScalarType t){
  switch (t) {
    case ScalarType::Int8: return 0;
    case ScalarType::UInt8: return 0;
    case ScalarType::Int16: return 1;
    case ScalarType::Int32: return 2;
    case ScalarType::Int64: return 3;
    default: return -1;
  }
}
static int complex_rank(ScalarType t){
  switch (t) {
    case ScalarType::Complex64: return 0;
    case ScalarType::Complex128: return 1;
    default: return -1;
  }
}

static ScalarType higher_float(ScalarType a, ScalarType b){
  return (float_rank(a) >= float_rank(b)) ? a : b;
}
static ScalarType higher_int(ScalarType a, ScalarType b){
  return (int_rank(a) >= int_rank(b)) ? a : b;
}
static ScalarType higher_complex(ScalarType a, ScalarType b){
  return (complex_rank(a) >= complex_rank(b)) ? a : b;
}

static ScalarType float_promote_pair(ScalarType a, ScalarType b){
  // Torch-like: float16/bfloat16 with float32 => float32; with float64 => float64.
  // float8 behaves as low-precision storage: promote at least to float16, but to float32 if mixed with float32.
  const bool a32 = (a == ScalarType::Float32);
  const bool b32 = (b == ScalarType::Float32);
  const bool a64 = (a == ScalarType::Float64);
  const bool b64 = (b == ScalarType::Float64);
  if (a64 || b64) return ScalarType::Float64;
  if (a32 || b32) return ScalarType::Float32;
  // float16 <-> bfloat16 mixes promote to float32 (Torch behavior)
  if ((a == ScalarType::Float16 && b == ScalarType::BFloat16) || (a == ScalarType::BFloat16 && b == ScalarType::Float16))
    return ScalarType::Float32;

  // float8 behaves as low-precision storage: mixing with half/bf16 yields float16 compute.
  if ((a == ScalarType::Float8E4M3FN || a == ScalarType::Float8E5M2 || b == ScalarType::Float8E4M3FN || b == ScalarType::Float8E5M2))
    return ScalarType::Float16;

  // both in {float16,bfloat16} => float16 by default
  return ScalarType::Float16;
}

ScalarType promote_types(ScalarType a, ScalarType b) {
  if (a == b) return a;

  // Complex dominates
  if (is_complex(a) || is_complex(b)) {
    const ScalarType ca = is_complex(a) ? a : ScalarType::Complex64;
    const ScalarType cb = is_complex(b) ? b : ScalarType::Complex64;

    // float64 promotes to complex128, else complex64; complex128 dominates.
    if (a == ScalarType::Float64 || b == ScalarType::Float64) return ScalarType::Complex128;
    if (a == ScalarType::Float32 || b == ScalarType::Float32) return ScalarType::Complex64;
    return higher_complex(ca, cb);
  }

  // Floating dominates
  if (is_floating(a) || is_floating(b)) {
    const ScalarType fa = is_floating(a) ? a : ScalarType::Float32;
    const ScalarType fb = is_floating(b) ? b : ScalarType::Float32;
    return float_promote_pair(fa, fb);
  }

  // Integral/bool: Torch-like safe widening.
  if (is_integral(a) && is_integral(b)) {
    if (is_bool(a) && is_bool(b)) return ScalarType::Bool;
    // bool with ints => int64
    if (is_bool(a) || is_bool(b)) return ScalarType::Int64;

    // uint8 with signed int: torch usually promotes to int16/int32/int64 depending; for safety choose int64
    if (is_uint(a) || is_uint(b)) {
      // If both uint8 => uint8
      if (a == ScalarType::UInt8 && b == ScalarType::UInt8) return ScalarType::UInt8;
      return ScalarType::Int64;
    }

    // signed ints: choose widest
    return higher_int(a, b);
  }

  throw_error("promote_types: unsupported dtype combination");
}

} // namespace pluss::codem

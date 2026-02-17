#include "CodeM/core/Errors.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/Complex.h"
#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesSlim.h"
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/native/cpu/TernaryApply.h"

#include <cmath>
#include <cstdint>
#include <algorithm>

namespace pluss::codem::native::cpu {

static bool fast_contiguous_binary_ok_(const Series& a, const Series& b, const Series& out) {
  const bool same_ab = a.sizes().size() == b.sizes().size() &&
                       std::equal(a.sizes().begin(), a.sizes().end(), b.sizes().begin());
  const bool same_ao = a.sizes().size() == out.sizes().size() &&
                       std::equal(a.sizes().begin(), a.sizes().end(), out.sizes().begin());
  return a.is_contiguous() && b.is_contiguous() && out.is_contiguous() &&
         a.numel() == b.numel() && a.numel() == out.numel() &&
         same_ab && same_ao;
}

template <class T, class Op>
static void fast_contiguous_binary_(const Series& a, const Series& b, Series& out, Op&& op) {
  const auto n = out.numel();
  const auto* ap = reinterpret_cast<const T*>(a.raw_data());
  const auto* bp = reinterpret_cast<const T*>(b.raw_data());
  auto* optr = reinterpret_cast<T*>(out.raw_data());
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
  for (std::int64_t i=0;i<n;++i) optr[i] = op(ap[i], bp[i]);
}


static bool is_contig_same_shape(const Series& a, const Series& b, const Series& out) {
  const bool same_ab = a.sizes().size() == b.sizes().size() &&
                       std::equal(a.sizes().begin(), a.sizes().end(), b.sizes().begin());
  const bool same_ao = a.sizes().size() == out.sizes().size() &&
                       std::equal(a.sizes().begin(), a.sizes().end(), out.sizes().begin());
  return a.is_contiguous() && b.is_contiguous() && out.is_contiguous() &&
         a.numel() == b.numel() && a.numel() == out.numel() &&
         same_ab && same_ao;
}

static bool fast_path_add(const Series& a, const Series& b, const Series& out) {
  if (!is_contig_same_shape(a,b,out)) return false;
  if (a.scalar_type() != b.scalar_type() || a.scalar_type() != out.scalar_type()) return false;
  const auto n = a.numel();
  if (a.scalar_type() == ScalarType::Float32) {
    const float* ap = reinterpret_cast<const float*>(a.raw_data());
    const float* bp = reinterpret_cast<const float*>(b.raw_data());
    float* op = reinterpret_cast<float*>(const_cast<std::byte*>(out.raw_data()));
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
    for (std::int64_t i=0;i<n;++i) op[i] = ap[i] + bp[i];
    return true;
  }
  if (a.scalar_type() == ScalarType::Float64) {
    const double* ap = reinterpret_cast<const double*>(a.raw_data());
    const double* bp = reinterpret_cast<const double*>(b.raw_data());
    double* op = reinterpret_cast<double*>(const_cast<std::byte*>(out.raw_data()));
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
    for (std::int64_t i=0;i<n;++i) op[i] = ap[i] + bp[i];
    return true;
  }
  return false;
}

static bool fast_path_mul(const Series& a, const Series& b, const Series& out) {
  if (!is_contig_same_shape(a,b,out)) return false;
  if (a.scalar_type() != b.scalar_type() || a.scalar_type() != out.scalar_type()) return false;
  const auto n = a.numel();
  if (a.scalar_type() == ScalarType::Float32) {
    const float* ap = reinterpret_cast<const float*>(a.raw_data());
    const float* bp = reinterpret_cast<const float*>(b.raw_data());
    float* op = reinterpret_cast<float*>(const_cast<std::byte*>(out.raw_data()));
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
    for (std::int64_t i=0;i<n;++i) op[i] = ap[i] * bp[i];
    return true;
  }
  if (a.scalar_type() == ScalarType::Float64) {
    const double* ap = reinterpret_cast<const double*>(a.raw_data());
    const double* bp = reinterpret_cast<const double*>(b.raw_data());
    double* op = reinterpret_cast<double*>(const_cast<std::byte*>(out.raw_data()));
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
    for (std::int64_t i=0;i<n;++i) op[i] = ap[i] * bp[i];
    return true;
  }
  return false;
}
template <class T, class Op>
static void binary_apply(const Series& a, const Series& b, Series& out, Op&& op) {
  if (fast_contiguous_binary_ok_(a,b,out)) { fast_contiguous_binary_<T>(a,b,out,op); return; }
  SeriesIterator it = SeriesIterator::binary(a, b, out);
  const auto* ap = reinterpret_cast<const T*>(a.raw_data());
  const auto* bp = reinterpret_cast<const T*>(b.raw_data());
  auto* optr = reinterpret_cast<T*>(out.raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t bo, std::int64_t){
    optr[lin] = op(ap[ao], bp[bo]);
  });
}



template <class T, class Op>
static void unary_apply(const Series& a, Series& out, Op&& op) {
  SeriesIterator it = SeriesIterator::unary(a, out);
  const auto* ap = reinterpret_cast<const T*>(a.raw_data());
  auto* optr = reinterpret_cast<T*>(out.raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){
    optr[lin] = op(ap[ao]);
  });
}

// Float16/BFloat16: compute in float32 then cast back to storage type.
template <class HalfT, class FromF32, class ToF32, class Op>
static void binary_half_apply(const Series& a, const Series& b, Series& out, Op&& op, FromF32&& from, ToF32&& to) {
  SeriesIterator it = SeriesIterator::binary(a, b, out);
  const auto* ap = reinterpret_cast<const HalfT*>(a.raw_data());
  const auto* bp = reinterpret_cast<const HalfT*>(b.raw_data());
  auto* optr = reinterpret_cast<HalfT*>(out.raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t bo, std::int64_t){
    const float x = to(ap[ao]);
    const float y = to(bp[bo]);
    optr[lin] = from(op(x,y));
  });
}

template <class HalfT, class FromF32, class ToF32, class Op>
static void unary_half_apply(const Series& a, Series& out, Op&& op, FromF32&& from, ToF32&& to) {
  SeriesIterator it = SeriesIterator::unary(a, out);
  const auto* ap = reinterpret_cast<const HalfT*>(a.raw_data());
  auto* optr = reinterpret_cast<HalfT*>(out.raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){
    const float x = to(ap[ao]);
    optr[lin] = from(op(x));
  });
}

#define DISPATCH_BINARY_INTS(name, OP) \
  void name(const Series& a, const Series& b, const Series& out_) { \
    auto& out = const_cast<Series&>(out_); \
    switch (a.scalar_type()) { \
      case ScalarType::Float64: binary_apply<double>(a,b,out,[](double x,double y){ return (double)(x OP y); }); return; \
      case ScalarType::Float32: binary_apply<float>(a,b,out,[](float x,float y){ return (float)(x OP y); }); return; \
      case ScalarType::Float16: binary_half_apply<Float16>(a,b,out,[](float x,float y){ return (float)(x OP y); }, float16_from_float, float16_to_float); return; \
      case ScalarType::BFloat16: binary_half_apply<BFloat16>(a,b,out,[](float x,float y){ return (float)(x OP y); }, bfloat16_from_float, bfloat16_to_float); return; \
      case ScalarType::Complex64: binary_apply<Complex64>(a,b,out,[](Complex64 x, Complex64 y){ return x OP y; }); return; \
      case ScalarType::Complex128: binary_apply<Complex128>(a,b,out,[](Complex128 x, Complex128 y){ return x OP y; }); return; \
      case ScalarType::Int8: binary_apply<std::int8_t>(a,b,out,[](auto x,auto y){ return (std::int8_t)(x OP y); }); return; \
      case ScalarType::UInt8: binary_apply<std::uint8_t>(a,b,out,[](auto x,auto y){ return (std::uint8_t)(x OP y); }); return; \
      case ScalarType::Int16: binary_apply<std::int16_t>(a,b,out,[](auto x,auto y){ return (std::int16_t)(x OP y); }); return; \
      case ScalarType::UInt16: binary_apply<std::uint16_t>(a,b,out,[](auto x,auto y){ return (std::uint16_t)(x OP y); }); return; \
      case ScalarType::Int32: binary_apply<std::int32_t>(a,b,out,[](auto x,auto y){ return (std::int32_t)(x OP y); }); return; \
      case ScalarType::UInt32: binary_apply<std::uint32_t>(a,b,out,[](auto x,auto y){ return (std::uint32_t)(x OP y); }); return; \
      case ScalarType::Int64: binary_apply<std::int64_t>(a,b,out,[](auto x,auto y){ return (std::int64_t)(x OP y); }); return; \
      case ScalarType::UInt64: binary_apply<std::uint64_t>(a,b,out,[](auto x,auto y){ return (std::uint64_t)(x OP y); }); return; \
      default: throw_error(#name ": dtype not implemented"); \
    } \
  }

DISPATCH_BINARY_INTS(add, +)
DISPATCH_BINARY_INTS(sub, -)
DISPATCH_BINARY_INTS(mul, *)
DISPATCH_BINARY_INTS(div, /)

#undef DISPATCH_BINARY_INTS

void neg(const Series& a, Series& out) {
  switch (a.scalar_type()) {
    case ScalarType::Float64: unary_apply<double>(a, out, [](double x) { return -x; }); return;
    case ScalarType::Float32: unary_apply<float>(a, out, [](float x) { return -x; }); return;
    case ScalarType::Float16:
      unary_half_apply<Float16>(a, out, [](float x) { return -x; }, float16_from_float, float16_to_float);
      return;
    case ScalarType::BFloat16:
      unary_half_apply<BFloat16>(a, out, [](float x) { return -x; }, bfloat16_from_float, bfloat16_to_float);
      return;
    case ScalarType::Complex64: unary_apply<Complex64>(a, out, [](Complex64 x) { return -x; }); return;
    case ScalarType::Complex128: unary_apply<Complex128>(a, out, [](Complex128 x) { return -x; }); return;
    case ScalarType::Int8: unary_apply<std::int8_t>(a, out, [](std::int8_t x) { return (std::int8_t)(-x); }); return;
    case ScalarType::UInt8: unary_apply<std::uint8_t>(a, out, [](std::uint8_t x) { return (std::uint8_t)(-x); }); return;
    case ScalarType::Int16: unary_apply<std::int16_t>(a, out, [](std::int16_t x) { return (std::int16_t)(-x); }); return;
    case ScalarType::UInt16: unary_apply<std::uint16_t>(a, out, [](std::uint16_t x) { return (std::uint16_t)(-x); }); return;
    case ScalarType::Int32: unary_apply<std::int32_t>(a, out, [](std::int32_t x) { return -x; }); return;
    case ScalarType::UInt32: unary_apply<std::uint32_t>(a, out, [](std::uint32_t x) { return (std::uint32_t)(-x); }); return;
    case ScalarType::Int64: unary_apply<std::int64_t>(a, out, [](std::int64_t x) { return -x; }); return;
    case ScalarType::UInt64: unary_apply<std::uint64_t>(a, out, [](std::uint64_t x) { return (std::uint64_t)(-x); }); return;
    default: throw_error("neg: dtype not implemented");
  }
}

void pow(const Series& a, const Series& b, const Series& out_) {
  auto& out = const_cast<Series&>(out_);
  switch (a.scalar_type()) {
    case ScalarType::Float64: binary_apply<double>(a,b,out,[](double x,double y){ return std::pow(x,y); }); return;
    case ScalarType::Float32: binary_apply<float>(a,b,out,[](float x,float y){ return std::pow(x,y); }); return;
    case ScalarType::Float16: binary_half_apply<Float16>(a,b,out,[](float x,float y){ return std::pow(x,y); }, float16_from_float, float16_to_float); return;
    case ScalarType::BFloat16: binary_half_apply<BFloat16>(a,b,out,[](float x,float y){ return std::pow(x,y); }, bfloat16_from_float, bfloat16_to_float); return;
    case ScalarType::Complex64: binary_apply<Complex64>(a,b,out,[](Complex64 x, Complex64 y){ return std::pow(x,y); }); return;
    case ScalarType::Complex128: binary_apply<Complex128>(a,b,out,[](Complex128 x, Complex128 y){ return std::pow(x,y); }); return;
    default: throw_error("pow: dtype not implemented");
  }
}

template <class T>
static T iabs(T x) { return x < 0 ? (T)(-x) : x; }

void abs(const Series& a, Series& out) {
  switch (a.scalar_type()) {
    case ScalarType::Float64: unary_apply<double>(a,out,[](double x){ return std::fabs(x); }); return;
    case ScalarType::Float32: unary_apply<float>(a,out,[](float x){ return std::fabs(x); }); return;
    case ScalarType::Float16: unary_half_apply<Float16>(a,out,[](float x){ return std::fabs(x); }, float16_from_float, float16_to_float); return;
    case ScalarType::BFloat16: unary_half_apply<BFloat16>(a,out,[](float x){ return std::fabs(x); }, bfloat16_from_float, bfloat16_to_float); return;
    case ScalarType::Int8: unary_apply<std::int8_t>(a,out,[](std::int8_t x){ return iabs(x); }); return;
    case ScalarType::Int16: unary_apply<std::int16_t>(a,out,[](std::int16_t x){ return iabs(x); }); return;
    case ScalarType::Int32: unary_apply<std::int32_t>(a,out,[](std::int32_t x){ return iabs(x); }); return;
    case ScalarType::Int64: unary_apply<std::int64_t>(a,out,[](std::int64_t x){ return iabs(x); }); return;
    default: throw_error("abs: dtype not implemented");
  }
}

template <class T>
static T isign(T x) { return (T)((x>0) - (x<0)); }

void sign(const Series& a, Series& out) {
  switch (a.scalar_type()) {
    case ScalarType::Float64: unary_apply<double>(a,out,[](double x){ return (x>0) - (x<0); }); return;
    case ScalarType::Float32: unary_apply<float>(a,out,[](float x){ return (x>0) - (x<0); }); return;
    case ScalarType::Float16: unary_half_apply<Float16>(a,out,[](float x){ return (x>0) - (x<0); }, float16_from_float, float16_to_float); return;
    case ScalarType::BFloat16: unary_half_apply<BFloat16>(a,out,[](float x){ return (x>0) - (x<0); }, bfloat16_from_float, bfloat16_to_float); return;
    case ScalarType::Int8: unary_apply<std::int8_t>(a,out,[](std::int8_t x){ return isign(x); }); return;
    case ScalarType::UInt8: unary_apply<std::uint8_t>(a,out,[](std::uint8_t x){ return (std::uint8_t)(x!=0); }); return;
    case ScalarType::Int16: unary_apply<std::int16_t>(a,out,[](std::int16_t x){ return isign(x); }); return;
    case ScalarType::UInt16: unary_apply<std::uint16_t>(a,out,[](std::uint16_t x){ return (std::uint16_t)(x!=0); }); return;
    case ScalarType::Int32: unary_apply<std::int32_t>(a,out,[](std::int32_t x){ return isign(x); }); return;
    case ScalarType::UInt32: unary_apply<std::uint32_t>(a,out,[](std::uint32_t x){ return (std::uint32_t)(x!=0); }); return;
    case ScalarType::Int64: unary_apply<std::int64_t>(a,out,[](std::int64_t x){ return isign(x); }); return;
    case ScalarType::UInt64: unary_apply<std::uint64_t>(a,out,[](std::uint64_t x){ return (std::uint64_t)(x!=0); }); return;
    default: throw_error("sign: dtype not implemented");
  }
}

} // namespace pluss::codem::native::cpu

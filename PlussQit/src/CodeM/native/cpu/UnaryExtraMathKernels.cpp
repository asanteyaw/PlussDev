#include "CodeM/native/cpu/UnaryExtraMathKernels.h"

#include "CodeM/core/Errors.h"
#include "CodeM/core/Complex.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/SeriesIterator.h"

#include <cmath>
#include <cstdint>

namespace pluss::codem::native::cpu {

template <class T, class F>
static void unary_apply(const Series& a, const Series& out_, F fn) {
  auto& out = const_cast<Series&>(out_);
  SeriesIterator it = SeriesIterator::unary(a, out);
  const auto* ap = reinterpret_cast<const T*>(a.raw_data());
  auto* op = reinterpret_cast<T*>(out.raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){
    op[lin] = fn(ap[ao]);
  });
}

template <class HalfT, class F>
static void unary_half(const Series& a, const Series& out_, F fn,
                       float(*to_f)(HalfT), HalfT(*from_f)(float)) {
  auto& out = const_cast<Series&>(out_);
  SeriesIterator it = SeriesIterator::unary(a, out);
  const auto* ap = reinterpret_cast<const HalfT*>(a.raw_data());
  auto* op = reinterpret_cast<HalfT*>(out.raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){
    const float x = to_f(ap[ao]);
    op[lin] = from_f(fn(x));
  });
}

void rsqrt(const Series& a, Series& out) {
  switch (a.scalar_type()) {
    case ScalarType::Float64: unary_apply<double>(a, out, [](double x){ return 1.0/std::sqrt(x); }); return;
    case ScalarType::Float32: unary_apply<float>(a, out, [](float x){ return 1.0f/std::sqrt(x); }); return;
    case ScalarType::Float16: unary_half<Float16>(a,out,[](float x){ return 1.0f/std::sqrt(x); }, float16_to_float, float16_from_float); return;
    case ScalarType::BFloat16: unary_half<BFloat16>(a,out,[](float x){ return 1.0f/std::sqrt(x); }, bfloat16_to_float, bfloat16_from_float); return;
    case ScalarType::Complex64: unary_apply<Complex64>(a, out, [](Complex64 x){ return Complex64(1.0f) / std::sqrt(x); }); return;
    case ScalarType::Complex128: unary_apply<Complex128>(a, out, [](Complex128 x){ return Complex128(1.0) / std::sqrt(x); }); return;
    default: throw_error("rsqrt: dtype not implemented");
  }
}

void log2(const Series& a, const Series& out) {
  switch (a.scalar_type()) {
    case ScalarType::Float64: unary_apply<double>(a, out, [](double x){ return std::log2(x); }); return;
    case ScalarType::Float32: unary_apply<float>(a, out, [](float x){ return std::log2(x); }); return;
    case ScalarType::Float16: unary_half<Float16>(a,out,[](float x){ return std::log2(x); }, float16_to_float, float16_from_float); return;
    case ScalarType::BFloat16: unary_half<BFloat16>(a,out,[](float x){ return std::log2(x); }, bfloat16_to_float, bfloat16_from_float); return;
    default: throw_error("log2: dtype not implemented");
  }
}

void log10(const Series& a, const Series& out) {
  switch (a.scalar_type()) {
    case ScalarType::Float64: unary_apply<double>(a, out, [](double x){ return std::log10(x); }); return;
    case ScalarType::Float32: unary_apply<float>(a, out, [](float x){ return std::log10(x); }); return;
    case ScalarType::Float16: unary_half<Float16>(a,out,[](float x){ return std::log10(x); }, float16_to_float, float16_from_float); return;
    case ScalarType::BFloat16: unary_half<BFloat16>(a,out,[](float x){ return std::log10(x); }, bfloat16_to_float, bfloat16_from_float); return;
    default: throw_error("log10: dtype not implemented");
  }
}

void exp2(const Series& a, const Series& out) {
  switch (a.scalar_type()) {
    case ScalarType::Float64: unary_apply<double>(a, out, [](double x){ return std::exp2(x); }); return;
    case ScalarType::Float32: unary_apply<float>(a, out, [](float x){ return std::exp2(x); }); return;
    case ScalarType::Float16: unary_half<Float16>(a,out,[](float x){ return std::exp2(x); }, float16_to_float, float16_from_float); return;
    case ScalarType::BFloat16: unary_half<BFloat16>(a,out,[](float x){ return std::exp2(x); }, bfloat16_to_float, bfloat16_from_float); return;
    default: throw_error("exp2: dtype not implemented");
  }
}

void cbrt(const Series& a, const Series& out) {
  switch (a.scalar_type()) {
    case ScalarType::Float64: unary_apply<double>(a, out, [](double x){ return std::cbrt(x); }); return;
    case ScalarType::Float32: unary_apply<float>(a, out, [](float x){ return std::cbrt(x); }); return;
    case ScalarType::Float16: unary_half<Float16>(a,out,[](float x){ return std::cbrt(x); }, float16_to_float, float16_from_float); return;
    case ScalarType::BFloat16: unary_half<BFloat16>(a,out,[](float x){ return std::cbrt(x); }, bfloat16_to_float, bfloat16_from_float); return;
    default: throw_error("cbrt: dtype not implemented");
  }
}

void square(const Series& a, const Series& out) {
  switch (a.scalar_type()) {
    case ScalarType::Float64: unary_apply<double>(a, out, [](double x){ return x*x; }); return;
    case ScalarType::Float32: unary_apply<float>(a, out, [](float x){ return x*x; }); return;
    case ScalarType::Float16: unary_half<Float16>(a,out,[](float x){ return x*x; }, float16_to_float, float16_from_float); return;
    case ScalarType::BFloat16: unary_half<BFloat16>(a,out,[](float x){ return x*x; }, bfloat16_to_float, bfloat16_from_float); return;
    case ScalarType::Complex64: unary_apply<Complex64>(a, out, [](Complex64 x){ return x*x; }); return;
    case ScalarType::Complex128: unary_apply<Complex128>(a, out, [](Complex128 x){ return x*x; }); return;
    case ScalarType::Int64: unary_apply<std::int64_t>(a, out, [](std::int64_t x){ return x*x; }); return;
    case ScalarType::Int32: unary_apply<std::int32_t>(a, out, [](std::int32_t x){ return x*x; }); return;
    case ScalarType::Int16: unary_apply<std::int16_t>(a, out, [](std::int16_t x){ return (std::int16_t)(x*x); }); return;
    case ScalarType::Int8: unary_apply<std::int8_t>(a, out, [](std::int8_t x){ return (std::int8_t)(x*x); }); return;
    default: throw_error("square: dtype not implemented");
  }
}

} // namespace

#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesSlim.h"
#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/Complex.h"
#include <complex>

#include <cmath>

namespace pluss::codem::native::cpu {

template <class InT, class OutT, class ToF, class FromF, class Fn>
static void unary_apply_convert(const Series& a, Series& out, ToF&& to_f, FromF&& from_f, Fn&& fn) {
  SeriesIterator it = SeriesIterator::unary(a, out);
  auto* op = reinterpret_cast<OutT*>(out.raw_data());
  const auto* ap = reinterpret_cast<const InT*>(a.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo){
    const float xf = to_f(ap[ao]);
    op[oo] = from_f(fn(xf));
  });
}

template <class T, class Fn>
static void unary_apply(const Series& a, Series& out, Fn&& fn) {
  SeriesIterator it = SeriesIterator::unary(a, out);
  auto* op = reinterpret_cast<T*>(out.raw_data());
  const auto* ap = reinterpret_cast<const T*>(a.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo){
    op[oo] = fn(ap[ao]);
  });
}


static bool fast_unary_contig_f32_f64(const Series& a, const Series& out, const char* name,
                                     float (*f32)(float), double (*f64)(double)) {
  if (!(a.is_contiguous() && out.is_contiguous())) return false;
  if (a.scalar_type() != out.scalar_type()) return false;
  const auto n = a.numel();
  if (a.scalar_type() == ScalarType::Float32) {
    const float* ap = reinterpret_cast<const float*>(a.raw_data());
    float* op = reinterpret_cast<float*>(const_cast<std::byte*>(out.raw_data()));
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
    for (std::int64_t i=0;i<n;++i) op[i] = f32(ap[i]);
    return true;
  }
  if (a.scalar_type() == ScalarType::Float64) {
    const double* ap = reinterpret_cast<const double*>(a.raw_data());
    double* op = reinterpret_cast<double*>(const_cast<std::byte*>(out.raw_data()));
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
    for (std::int64_t i=0;i<n;++i) op[i] = f64(ap[i]);
    return true;
  }
  return false;
}

static float f32_exp(float x){ return std::exp(x); }
static double f64_exp(double x){ return std::exp(x); }
static float f32_log(float x){ return std::log(x); }
static double f64_log(double x){ return std::log(x); }
static float f32_sqrt(float x){ return std::sqrt(x); }
static double f64_sqrt(double x){ return std::sqrt(x); }

#define DEFINE_UNARY(name, EXPR_D, EXPR_F)   void name(const Series& a, Series& out) {     switch (a.scalar_type()) {       case ScalarType::Float64: unary_apply<double>(a, out, [](double x){ return (EXPR_D); }); return;       case ScalarType::Float32: unary_apply<float>(a, out, [](float x){ return (float)(EXPR_F); }); return;       case ScalarType::Float16: unary_apply_convert<Float16, Float16>(a, out,         [](Float16 x){ return float16_to_float(x); },         [](float y){ return float16_from_float(y); },         [](float x){ return (float)(EXPR_F); }); return;       case ScalarType::BFloat16: unary_apply_convert<BFloat16, BFloat16>(a, out,         [](BFloat16 x){ return bfloat16_to_float(x); },         [](float y){ return bfloat16_from_float(y); },         [](float x){ return (float)(EXPR_F); }); return;       case ScalarType::Complex64: unary_apply<Complex64>(a, out, [](Complex64 x){ return (Complex64)(EXPR_D); }); return;       case ScalarType::Complex128: unary_apply<Complex128>(a, out, [](Complex128 x){ return (Complex128)(EXPR_D); }); return;       default: break;     }     throw_error(#name ": dtype not implemented");   }

DEFINE_UNARY(exp, std::exp(x), std::exp(x))
DEFINE_UNARY(log, std::log(x), std::log(x))
DEFINE_UNARY(sqrt, std::sqrt(x), std::sqrt(x))
DEFINE_UNARY(sin, std::sin(x), std::sin(x))
DEFINE_UNARY(cos, std::cos(x), std::cos(x))
DEFINE_UNARY(tanh, std::tanh(x), std::tanh(x))

#undef DEFINE_UNARY

} // namespace pluss::codem::native::cpu

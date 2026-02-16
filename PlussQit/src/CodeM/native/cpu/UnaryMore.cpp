#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesSlim.h"
#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/Half.h"

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

#define DISPATCH_FLOAT_ONLY(name, EXPR_D, EXPR_F)   void name(const Series& a, Series& out) {     switch (a.scalar_type()) {       case ScalarType::Float64: unary_apply<double>(a, out, [](double x){ return (EXPR_D); }); return;       case ScalarType::Float32: unary_apply<float>(a, out, [](float x){ return (float)(EXPR_F); }); return;       case ScalarType::Float16: unary_apply_convert<Float16, Float16>(a, out,         [](Float16 x){ return float16_to_float(x); },         [](float y){ return float16_from_float(y); },         [](float x){ return (float)(EXPR_F); }); return;       case ScalarType::BFloat16: unary_apply_convert<BFloat16, BFloat16>(a, out,         [](BFloat16 x){ return bfloat16_to_float(x); },         [](float y){ return bfloat16_from_float(y); },         [](float x){ return (float)(EXPR_F); }); return;       default: break;     }     throw_error(#name ": dtype not implemented (float-only)");   }

DISPATCH_FLOAT_ONLY(tan, std::tan(x), std::tan(x))
DISPATCH_FLOAT_ONLY(asin, std::asin(x), std::asin(x))
DISPATCH_FLOAT_ONLY(acos, std::acos(x), std::acos(x))
DISPATCH_FLOAT_ONLY(atan, std::atan(x), std::atan(x))
DISPATCH_FLOAT_ONLY(sinh, std::sinh(x), std::sinh(x))
DISPATCH_FLOAT_ONLY(cosh, std::cosh(x), std::cosh(x))

DISPATCH_FLOAT_ONLY(floor, std::floor(x), std::floor(x))
DISPATCH_FLOAT_ONLY(ceil, std::ceil(x), std::ceil(x))
DISPATCH_FLOAT_ONLY(round, std::round(x), std::round(x))
DISPATCH_FLOAT_ONLY(trunc, std::trunc(x), std::trunc(x))

DISPATCH_FLOAT_ONLY(reciprocal, (1.0 / x), (1.0f / x))
DISPATCH_FLOAT_ONLY(sigmoid, (1.0 / (1.0 + std::exp(-x))), (1.0f / (1.0f + std::exp(-x))))

DISPATCH_FLOAT_ONLY(log1p, std::log1p(x), std::log1p(x))
DISPATCH_FLOAT_ONLY(expm1, std::expm1(x), std::expm1(x))
DISPATCH_FLOAT_ONLY(erf, std::erf(x), std::erf(x))
DISPATCH_FLOAT_ONLY(erfc, std::erfc(x), std::erfc(x))
DISPATCH_FLOAT_ONLY(lgamma, std::lgamma(x), std::lgamma(x))
DISPATCH_FLOAT_ONLY(asinh, std::asinh(x), std::asinh(x))
DISPATCH_FLOAT_ONLY(acosh, std::acosh(x), std::acosh(x))
DISPATCH_FLOAT_ONLY(atanh, std::atanh(x), std::atanh(x))

DISPATCH_FLOAT_ONLY(relu, (x > 0 ? x : 0.0), (x > 0 ? x : 0.0f))

#undef DISPATCH_FLOAT_ONLY


static void unary_is_predicate_(const Series& a, Series& out, bool(*pred)(double)) {
  if (out.scalar_type() != ScalarType::Bool) {
    throw_error("predicate unary: out must be Bool");
  }
  SeriesIterator it = SeriesIterator::unary(a, out);
  auto* op = reinterpret_cast<std::uint8_t*>(out.raw_data());
  switch (a.scalar_type()) {
    case ScalarType::Float64: {
      const auto* ap = reinterpret_cast<const double*>(a.raw_data());
      it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo){
        op[oo] = pred(ap[ao]) ? 1 : 0;
      });
      return;
    }
    case ScalarType::Float32: {
      const auto* ap = reinterpret_cast<const float*>(a.raw_data());
      it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo){
        op[oo] = pred((double)ap[ao]) ? 1 : 0;
      });
      return;
    }
    case ScalarType::Float16: {
      const auto* ap = reinterpret_cast<const Float16*>(a.raw_data());
      it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo){
        op[oo] = pred((double)float16_to_float(ap[ao])) ? 1 : 0;
      });
      return;
    }
    case ScalarType::BFloat16: {
      const auto* ap = reinterpret_cast<const BFloat16*>(a.raw_data());
      it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo){
        op[oo] = pred((double)bfloat16_to_float(ap[ao])) ? 1 : 0;
      });
      return;
    }
    default: break;
  }
  throw_error("predicate unary: dtype not implemented");
}

static bool pred_isnan(double x) { return std::isnan(x); }
static bool pred_isinf(double x) { return std::isinf(x); }
static bool pred_isfinite(double x) { return std::isfinite(x); }

void isnan(const Series& a, Series& out) { unary_is_predicate_(a, out, &pred_isnan); }
void isinf(const Series& a, Series& out) { unary_is_predicate_(a, out, &pred_isinf); }
void isfinite(const Series& a, Series& out) { unary_is_predicate_(a, out, &pred_isfinite); }

void logical_not(const Series& a, Series& out) {
  if (a.scalar_type() != ScalarType::Bool || out.scalar_type() != ScalarType::Bool) {
    throw_error("logical_not: dtype must be Bool");
  }
  unary_apply<std::uint8_t>(a, out, [](std::uint8_t x){ return (std::uint8_t)(x ? 0 : 1); });
}

} // namespace pluss::codem::native::cpu

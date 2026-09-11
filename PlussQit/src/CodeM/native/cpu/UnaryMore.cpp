#include "CodeM/core/Errors.h"
#include "CodeM/core/Complex.h"
#include "CodeM/core/SeriesSlim.h"
#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/Half.h"

#include <cmath>
#include <limits>

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

#define DISPATCH_FLOAT_COMPLEX(name, EXPR_D, EXPR_F, EXPR_C64, EXPR_C128) \
  void name(const Series& a, Series& out) { \
    switch (a.scalar_type()) { \
      case ScalarType::Float64: unary_apply<double>(a, out, [](double x){ return (EXPR_D); }); return; \
      case ScalarType::Float32: unary_apply<float>(a, out, [](float x){ return (float)(EXPR_F); }); return; \
      case ScalarType::Float16: unary_apply_convert<Float16, Float16>(a, out, \
        [](Float16 x){ return float16_to_float(x); }, \
        [](float y){ return float16_from_float(y); }, \
        [](float x){ return (float)(EXPR_F); }); return; \
      case ScalarType::BFloat16: unary_apply_convert<BFloat16, BFloat16>(a, out, \
        [](BFloat16 x){ return bfloat16_to_float(x); }, \
        [](float y){ return bfloat16_from_float(y); }, \
        [](float x){ return (float)(EXPR_F); }); return; \
      case ScalarType::Complex64: unary_apply<Complex64>(a, out, [](Complex64 x){ return (EXPR_C64); }); return; \
      case ScalarType::Complex128: unary_apply<Complex128>(a, out, [](Complex128 x){ return (EXPR_C128); }); return; \
      default: break; \
    } \
    throw_error(#name ": dtype not implemented"); \
  }

// psi(x) = d/dx ln|Gamma(x)|. Reflection for x <= 0, upward recurrence to the
// asymptotic-series regime (x >= 6), then the standard Bernoulli tail. Poles at
// 0 and the negative integers return NaN; NaN in maps to NaN out.
static double digamma_real_(double x) {
  if (std::isnan(x)) return x;
  if (x <= 0.0) {
    const double frac = x - std::floor(x);
    if (frac == 0.0) return std::numeric_limits<double>::quiet_NaN();
    constexpr double kPi = 3.14159265358979323846;
    return digamma_real_(1.0 - x) - kPi / std::tan(kPi * x);
  }
  double acc = 0.0;
  double y = x;
  while (y < 6.0) {
    acc -= 1.0 / y;
    y += 1.0;
  }
  const double inv = 1.0 / y;
  const double inv2 = inv * inv;
  acc += std::log(y) - 0.5 * inv -
      inv2 * (1.0 / 12.0 - inv2 * (1.0 / 120.0 - inv2 * (1.0 / 252.0)));
  return acc;
}

DISPATCH_FLOAT_ONLY(digamma, digamma_real_(x), digamma_real_(x))

DISPATCH_FLOAT_COMPLEX(tan, std::tan(x), std::tan(x), std::tan(x), std::tan(x))
DISPATCH_FLOAT_COMPLEX(asin, std::asin(x), std::asin(x), std::asin(x), std::asin(x))
DISPATCH_FLOAT_COMPLEX(acos, std::acos(x), std::acos(x), std::acos(x), std::acos(x))
DISPATCH_FLOAT_COMPLEX(atan, std::atan(x), std::atan(x), std::atan(x), std::atan(x))
DISPATCH_FLOAT_COMPLEX(sinh, std::sinh(x), std::sinh(x), std::sinh(x), std::sinh(x))
DISPATCH_FLOAT_COMPLEX(cosh, std::cosh(x), std::cosh(x), std::cosh(x), std::cosh(x))

DISPATCH_FLOAT_COMPLEX(reciprocal, (1.0 / x), (1.0f / x), (Complex64(1.0f) / x), (Complex128(1.0) / x))

DISPATCH_FLOAT_COMPLEX(log1p, std::log1p(x), std::log1p(x), std::log(Complex64(1.0f) + x), std::log(Complex128(1.0) + x))
DISPATCH_FLOAT_COMPLEX(expm1, std::expm1(x), std::expm1(x), std::exp(x) - Complex64(1.0f), std::exp(x) - Complex128(1.0))
DISPATCH_FLOAT_ONLY(erf, std::erf(x), std::erf(x))
DISPATCH_FLOAT_ONLY(erfc, std::erfc(x), std::erfc(x))
DISPATCH_FLOAT_ONLY(lgamma, std::lgamma(x), std::lgamma(x))
DISPATCH_FLOAT_COMPLEX(asinh, std::asinh(x), std::asinh(x), std::asinh(x), std::asinh(x))
DISPATCH_FLOAT_COMPLEX(acosh, std::acosh(x), std::acosh(x), std::acosh(x), std::acosh(x))
DISPATCH_FLOAT_COMPLEX(atanh, std::atanh(x), std::atanh(x), std::atanh(x), std::atanh(x))


#undef DISPATCH_FLOAT_ONLY
#undef DISPATCH_FLOAT_COMPLEX


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

static bool pred_isfinite(double x) { return std::isfinite(x); }

void isfinite(const Series& a, Series& out) { unary_is_predicate_(a, out, &pred_isfinite); }

static bool pred_signbit(double x) { return std::signbit(x); }

void signbit(const Series& a, Series& out) { unary_is_predicate_(a, out, &pred_signbit); }

void logical_not(const Series& a, Series& out) {
  if (a.scalar_type() != ScalarType::Bool || out.scalar_type() != ScalarType::Bool) {
    throw_error("logical_not: dtype must be Bool");
  }
  unary_apply<std::uint8_t>(a, out, [](std::uint8_t x){ return (std::uint8_t)(x ? 0 : 1); });
}

} // namespace pluss::codem::native::cpu

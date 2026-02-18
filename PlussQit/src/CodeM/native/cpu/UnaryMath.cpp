#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesSlim.h"
#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/Complex.h"
#include "MSharp/Parallel.h"

#include <cmath>
#include <algorithm>
#include <limits>
#if defined(__APPLE__) && defined(PLUSS_HAS_ACCELERATE) && (PLUSS_HAS_ACCELERATE == 1)
#include <Accelerate/Accelerate.h>
#endif

namespace pluss::codem::native::cpu {

template <class Fn>
static inline void for_each_chunk_(std::int64_t n, Fn&& fn) {
  constexpr std::int64_t kParallelThreshold = (1LL << 20); // 1M elems
  if (n < kParallelThreshold) {
    fn(0, n);
    return;
  }
  const std::int64_t grain = std::max<std::int64_t>(1LL << 17, n / 32);
  pluss::msharp::parallel_for(0, n, grain, [&](std::int64_t b0, std::int64_t b1) { fn(b0, b1); });
}

static bool fast_contiguous_unary_ok_(const Series& a, const Series& out) {
  if (!(a.is_contiguous() && out.is_contiguous() && a.numel() == out.numel())) return false;
  if (a.sizes().size() != out.sizes().size()) return false;
  return std::equal(a.sizes().begin(), a.sizes().end(), out.sizes().begin());
}

template <class InT, class OutT, class ToF, class FromF, class Fn>
static void unary_apply_convert(const Series& a, Series& out, ToF&& to_f, FromF&& from_f, Fn&& fn) {
  if (fast_contiguous_unary_ok_(a, out)) {
    const auto n = out.numel();
    auto* op = reinterpret_cast<OutT*>(out.raw_data());
    const auto* ap = reinterpret_cast<const InT*>(a.raw_data());
    for_each_chunk_(n, [&](std::int64_t b0, std::int64_t b1) {
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
      for (std::int64_t i = b0; i < b1; ++i) {
        op[i] = from_f(fn(to_f(ap[i])));
      }
    });
    return;
  }
  SeriesIterator it = SeriesIterator::unary(a, out);
  auto* op = reinterpret_cast<OutT*>(out.raw_data());
  const auto* ap = reinterpret_cast<const InT*>(a.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
    const float xf = to_f(ap[ao]);
    op[oo] = from_f(fn(xf));
  });
}

template <class T, class Fn>
static void unary_apply(const Series& a, Series& out, Fn&& fn) {
  if (fast_contiguous_unary_ok_(a, out)) {
    const auto n = out.numel();
    auto* op = reinterpret_cast<T*>(out.raw_data());
    const auto* ap = reinterpret_cast<const T*>(a.raw_data());
    for_each_chunk_(n, [&](std::int64_t b0, std::int64_t b1) {
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
      for (std::int64_t i = b0; i < b1; ++i) {
        op[i] = fn(ap[i]);
      }
    });
    return;
  }
  SeriesIterator it = SeriesIterator::unary(a, out);
  auto* op = reinterpret_cast<T*>(out.raw_data());
  const auto* ap = reinterpret_cast<const T*>(a.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
    op[oo] = fn(ap[ao]);
  });
}

#define DEFINE_UNARY(name, EXPR_D, EXPR_F) \
  void name(const Series& a, Series& out) { \
    switch (a.scalar_type()) { \
      case ScalarType::Float64: \
        unary_apply<double>(a, out, [](double x) { return (EXPR_D); }); \
        return; \
      case ScalarType::Float32: \
        unary_apply<float>(a, out, [](float x) { return static_cast<float>(EXPR_F); }); \
        return; \
      case ScalarType::Float16: \
        unary_apply_convert<Float16, Float16>( \
            a, out, \
            [](Float16 x) { return float16_to_float(x); }, \
            [](float y) { return float16_from_float(y); }, \
            [](float x) { return static_cast<float>(EXPR_F); }); \
        return; \
      case ScalarType::BFloat16: \
        unary_apply_convert<BFloat16, BFloat16>( \
            a, out, \
            [](BFloat16 x) { return bfloat16_to_float(x); }, \
            [](float y) { return bfloat16_from_float(y); }, \
            [](float x) { return static_cast<float>(EXPR_F); }); \
        return; \
      case ScalarType::Complex64: \
        unary_apply<Complex64>(a, out, [](Complex64 x) { return static_cast<Complex64>(EXPR_D); }); \
        return; \
      case ScalarType::Complex128: \
        unary_apply<Complex128>(a, out, [](Complex128 x) { return static_cast<Complex128>(EXPR_D); }); \
        return; \
      default: \
        break; \
    } \
    throw_error(#name ": dtype not implemented"); \
  }

void exp(const Series& a, Series& out) {
  switch (a.scalar_type()) {
    case ScalarType::Float64: {
      if (fast_contiguous_unary_ok_(a, out)) {
        const auto n = a.numel();
        auto* op = reinterpret_cast<double*>(out.raw_data());
        const auto* ap = reinterpret_cast<const double*>(a.raw_data());
#if defined(__APPLE__) && defined(PLUSS_HAS_ACCELERATE) && (PLUSS_HAS_ACCELERATE == 1)
        if (n <= static_cast<std::int64_t>(std::numeric_limits<int>::max())) {
          int nn = static_cast<int>(n);
          vvexp(op, ap, &nn);
          return;
        }
#endif
        unary_apply<double>(a, out, [](double x) { return std::exp(x); });
        return;
      }
      unary_apply<double>(a, out, [](double x) { return std::exp(x); });
      return;
    }
    case ScalarType::Float32: {
      if (fast_contiguous_unary_ok_(a, out)) {
        const auto n = a.numel();
        auto* op = reinterpret_cast<float*>(out.raw_data());
        const auto* ap = reinterpret_cast<const float*>(a.raw_data());
#if defined(__APPLE__) && defined(PLUSS_HAS_ACCELERATE) && (PLUSS_HAS_ACCELERATE == 1)
        if (n <= static_cast<std::int64_t>(std::numeric_limits<int>::max())) {
          int nn = static_cast<int>(n);
          vvexpf(op, ap, &nn);
          return;
        }
#endif
        unary_apply<float>(a, out, [](float x) { return std::exp(x); });
        return;
      }
      unary_apply<float>(a, out, [](float x) { return std::exp(x); });
      return;
    }
    case ScalarType::Float16:
      unary_apply_convert<Float16, Float16>(
          a, out,
          [](Float16 x) { return float16_to_float(x); },
          [](float y) { return float16_from_float(y); },
          [](float x) { return std::exp(x); });
      return;
    case ScalarType::BFloat16:
      unary_apply_convert<BFloat16, BFloat16>(
          a, out,
          [](BFloat16 x) { return bfloat16_to_float(x); },
          [](float y) { return bfloat16_from_float(y); },
          [](float x) { return std::exp(x); });
      return;
    case ScalarType::Complex64:
      unary_apply<Complex64>(a, out, [](Complex64 x) { return std::exp(x); });
      return;
    case ScalarType::Complex128:
      unary_apply<Complex128>(a, out, [](Complex128 x) { return std::exp(x); });
      return;
    default: break;
  }
  throw_error("exp: dtype not implemented");
}

DEFINE_UNARY(log, std::log(x), std::log(x))
DEFINE_UNARY(sqrt, std::sqrt(x), std::sqrt(x))
DEFINE_UNARY(sin, std::sin(x), std::sin(x))
DEFINE_UNARY(cos, std::cos(x), std::cos(x))
DEFINE_UNARY(tanh, std::tanh(x), std::tanh(x))

#undef DEFINE_UNARY

} // namespace pluss::codem::native::cpu

#include "CodeM/native/cpu/UnaryFastOps.h"

#include "CodeM/core/Errors.h"
#include "CodeM/core/Float8.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/SeriesIterator.h"
#include "MSharp/Parallel.h"

#include <algorithm>
#include <type_traits>
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

template <class T>
static void relu_loop(const Series& a, Series& out) {
  if (fast_contiguous_unary_ok_(a, out)) {
    const auto n = out.numel();
    auto* op = reinterpret_cast<T*>(out.raw_data());
    const auto* ap = reinterpret_cast<const T*>(a.raw_data());
#if defined(__APPLE__) && defined(PLUSS_HAS_ACCELERATE) && (PLUSS_HAS_ACCELERATE == 1)
    if constexpr (std::is_same_v<T, float>) {
      const float zero = 0.0f;
      vDSP_vthres(ap, 1, &zero, op, 1, static_cast<vDSP_Length>(n));
      return;
    }
    if constexpr (std::is_same_v<T, double>) {
      const double zero = 0.0;
      vDSP_vthresD(ap, 1, &zero, op, 1, static_cast<vDSP_Length>(n));
      return;
    }
#endif
    for_each_chunk_(n, [&](std::int64_t b0, std::int64_t b1) {
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
      for (std::int64_t i = b0; i < b1; ++i) {
        const T x = ap[i];
        op[i] = x > static_cast<T>(0) ? x : static_cast<T>(0);
      }
    });
    return;
  }
  SeriesIterator it = SeriesIterator::unary(a, out);
  auto* op = reinterpret_cast<T*>(out.raw_data());
  const auto* ap = reinterpret_cast<const T*>(a.raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t) {
    const T x = ap[ao];
    op[lin] = x > static_cast<T>(0) ? x : static_cast<T>(0);
  });
}

void relu(const Series& a, Series& out) {
  switch (a.scalar_type()) {
    case ScalarType::Float64: relu_loop<double>(a, out); return;
    case ScalarType::Float32: relu_loop<float>(a, out); return;
    case ScalarType::Float16: {
      SeriesIterator it = SeriesIterator::unary(a, out);
      auto* op = reinterpret_cast<Float16*>(out.raw_data());
      const auto* ap = reinterpret_cast<const Float16*>(a.raw_data());
      it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t) {
        const float x = float16_to_float(ap[ao]);
        op[lin] = float16_from_float(x > 0.0f ? x : 0.0f);
      });
      return;
    }
    case ScalarType::BFloat16: {
      SeriesIterator it = SeriesIterator::unary(a, out);
      auto* op = reinterpret_cast<BFloat16*>(out.raw_data());
      const auto* ap = reinterpret_cast<const BFloat16*>(a.raw_data());
      it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t) {
        const float x = bfloat16_to_float(ap[ao]);
        op[lin] = bfloat16_from_float(x > 0.0f ? x : 0.0f);
      });
      return;
    }
    case ScalarType::Float8E4M3FN:
    case ScalarType::Float8E5M2: {
      SeriesIterator it = SeriesIterator::unary(a, out);
      if (a.scalar_type() == ScalarType::Float8E4M3FN) {
        auto* op = reinterpret_cast<Float8E4M3FN*>(out.raw_data());
        const auto* ap = reinterpret_cast<const Float8E4M3FN*>(a.raw_data());
        it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t) {
          const float x = float8e4m3fn_to_float(ap[ao]);
          op[lin] = float8e4m3fn_from_float(x > 0.0f ? x : 0.0f);
        });
      } else {
        auto* op = reinterpret_cast<Float8E5M2*>(out.raw_data());
        const auto* ap = reinterpret_cast<const Float8E5M2*>(a.raw_data());
        it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t) {
          const float x = float8e5m2_to_float(ap[ao]);
          op[lin] = float8e5m2_from_float(x > 0.0f ? x : 0.0f);
        });
      }
      return;
    }
    default: break;
  }
  throw_error("relu: dtype not implemented");
}

} // namespace pluss::codem::native::cpu

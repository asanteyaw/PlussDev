#include "CodeM/native/cpu/CompareOps.h"
#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/Complex.h"
#include <algorithm>
#include <cstdint>

namespace pluss::codem::native::cpu {

static bool fast_contiguous_cmp_ok_(const Series& a, const Series& b, const Series& out) {
  const bool same_ab = a.sizes().size() == b.sizes().size() &&
                       std::equal(a.sizes().begin(), a.sizes().end(), b.sizes().begin());
  const bool same_ao = a.sizes().size() == out.sizes().size() &&
                       std::equal(a.sizes().begin(), a.sizes().end(), out.sizes().begin());
  return a.is_contiguous() && b.is_contiguous() && out.is_contiguous() &&
         a.numel() == b.numel() && a.numel() == out.numel() &&
         same_ab && same_ao;
}

template <class T, class Pred>
static void cmp_contiguous_fast_(const Series& a, const Series& b, const Series& out, Pred&& pred) {
  auto* outp = reinterpret_cast<std::uint8_t*>(const_cast<std::byte*>(out.raw_data()));
  const auto* ap = reinterpret_cast<const T*>(a.raw_data());
  const auto* bp = reinterpret_cast<const T*>(b.raw_data());
  const auto n = out.numel();
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
  for (std::int64_t i = 0; i < n; ++i) {
    outp[i] = pred(ap[i], bp[i]) ? 1 : 0;
  }
}

template <class T, class Pred>
static void cmp_loop(const Series& a, const Series& b, const Series& out, Pred&& pred) {
  if (fast_contiguous_cmp_ok_(a, b, out)) {
    cmp_contiguous_fast_<T>(a, b, out, pred);
    return;
  }
  SeriesIterator it = SeriesIterator::binary(a, b, out);
  auto* outp = reinterpret_cast<std::uint8_t*>(const_cast<std::byte*>(out.raw_data()));
  const auto* ap = reinterpret_cast<const T*>(a.raw_data());
  const auto* bp = reinterpret_cast<const T*>(b.raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t bo, std::int64_t) {
    outp[lin] = pred(ap[ao], bp[bo]) ? 1 : 0;
  });
}

template <class HalfT, class ToF32, class Pred>
static void cmp_half_contiguous_fast_(const Series& a, const Series& b, const Series& out, Pred&& pred, ToF32&& to) {
  auto* outp = reinterpret_cast<std::uint8_t*>(const_cast<std::byte*>(out.raw_data()));
  const auto* ap = reinterpret_cast<const HalfT*>(a.raw_data());
  const auto* bp = reinterpret_cast<const HalfT*>(b.raw_data());
  const auto n = out.numel();
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
  for (std::int64_t i = 0; i < n; ++i) {
    outp[i] = pred(to(ap[i]), to(bp[i])) ? 1 : 0;
  }
}

template <class HalfT, class ToF32, class Pred>
static void cmp_half_loop(const Series& a, const Series& b, const Series& out, Pred&& pred, ToF32&& to) {
  if (fast_contiguous_cmp_ok_(a, b, out)) {
    cmp_half_contiguous_fast_<HalfT>(a, b, out, pred, to);
    return;
  }
  SeriesIterator it = SeriesIterator::binary(a, b, out);
  auto* outp = reinterpret_cast<std::uint8_t*>(const_cast<std::byte*>(out.raw_data()));
  const auto* ap = reinterpret_cast<const HalfT*>(a.raw_data());
  const auto* bp = reinterpret_cast<const HalfT*>(b.raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t bo, std::int64_t) {
    outp[lin] = pred(to(ap[ao]), to(bp[bo])) ? 1 : 0;
  });
}

template <class Pred>
static void dispatch_cmp(const Series& a, const Series& b, const Series& out, Pred&& pred, const char* name) {
  PLUSS_CHECK(out.scalar_type() == ScalarType::Bool, "compare: out must be Bool");
  switch (a.scalar_type()) {
    case ScalarType::Float64: cmp_loop<double>(a, b, out, pred); return;
    case ScalarType::Float32: cmp_loop<float>(a, b, out, pred); return;
    case ScalarType::Float16: cmp_half_loop<Float16>(a, b, out, pred, float16_to_float); return;
    case ScalarType::BFloat16: cmp_half_loop<BFloat16>(a, b, out, pred, bfloat16_to_float); return;
    case ScalarType::Int64: cmp_loop<std::int64_t>(a, b, out, pred); return;
    case ScalarType::Int32: cmp_loop<std::int32_t>(a, b, out, pred); return;
    case ScalarType::Int16: cmp_loop<std::int16_t>(a, b, out, pred); return;
    case ScalarType::Int8:  cmp_loop<std::int8_t>(a, b, out, pred); return;
    case ScalarType::UInt64: cmp_loop<std::uint64_t>(a, b, out, pred); return;
    case ScalarType::UInt32: cmp_loop<std::uint32_t>(a, b, out, pred); return;
    case ScalarType::UInt16: cmp_loop<std::uint16_t>(a, b, out, pred); return;
    case ScalarType::UInt8:  cmp_loop<std::uint8_t>(a, b, out, pred); return;
    case ScalarType::Bool:   cmp_loop<std::uint8_t>(a, b, out, pred); return;
    default: break;
  }
  throw_error(std::string(name) + ": dtype not implemented");
}

static void cmp_complex_eq_ne_(const Series& a, const Series& b, const Series& out, bool eq_mode) {
  PLUSS_CHECK(out.scalar_type() == ScalarType::Bool, "compare: out must be Bool");
  const bool is_c64 = a.scalar_type() == ScalarType::Complex64;
  PLUSS_CHECK(is_c64 || a.scalar_type() == ScalarType::Complex128, "compare: expected complex dtype");

  if (fast_contiguous_cmp_ok_(a, b, out)) {
    auto* outp = reinterpret_cast<std::uint8_t*>(const_cast<std::byte*>(out.raw_data()));
    const auto n = out.numel();
    if (is_c64) {
      const auto* ap = reinterpret_cast<const Complex64*>(a.raw_data());
      const auto* bp = reinterpret_cast<const Complex64*>(b.raw_data());
      for (std::int64_t i = 0; i < n; ++i) {
        const bool same = (ap[i] == bp[i]);
        outp[i] = (eq_mode ? same : !same) ? 1 : 0;
      }
    } else {
      const auto* ap = reinterpret_cast<const Complex128*>(a.raw_data());
      const auto* bp = reinterpret_cast<const Complex128*>(b.raw_data());
      for (std::int64_t i = 0; i < n; ++i) {
        const bool same = (ap[i] == bp[i]);
        outp[i] = (eq_mode ? same : !same) ? 1 : 0;
      }
    }
    return;
  }

  SeriesIterator it = SeriesIterator::binary(a, b, out);
  auto* outp = reinterpret_cast<std::uint8_t*>(const_cast<std::byte*>(out.raw_data()));
  if (is_c64) {
    const auto* ap = reinterpret_cast<const Complex64*>(a.raw_data());
    const auto* bp = reinterpret_cast<const Complex64*>(b.raw_data());
    it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t bo, std::int64_t) {
      const bool same = (ap[ao] == bp[bo]);
      outp[lin] = (eq_mode ? same : !same) ? 1 : 0;
    });
  } else {
    const auto* ap = reinterpret_cast<const Complex128*>(a.raw_data());
    const auto* bp = reinterpret_cast<const Complex128*>(b.raw_data());
    it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t bo, std::int64_t) {
      const bool same = (ap[ao] == bp[bo]);
      outp[lin] = (eq_mode ? same : !same) ? 1 : 0;
    });
  }
}

void eq(const Series& a, const Series& b, const Series& out) {
  if (a.scalar_type() == ScalarType::Complex64 || a.scalar_type() == ScalarType::Complex128) {
    cmp_complex_eq_ne_(a, b, out, /*eq_mode=*/true);
    return;
  }
  dispatch_cmp(a,b,out,[](auto x, auto y){return x==y;}, "eq");
}
void ne(const Series& a, const Series& b, const Series& out) {
  if (a.scalar_type() == ScalarType::Complex64 || a.scalar_type() == ScalarType::Complex128) {
    cmp_complex_eq_ne_(a, b, out, /*eq_mode=*/false);
    return;
  }
  dispatch_cmp(a,b,out,[](auto x, auto y){return x!=y;}, "ne");
}
void lt(const Series& a, const Series& b, const Series& out) { dispatch_cmp(a,b,out,[](auto x, auto y){return x< y;}, "lt"); }
void le(const Series& a, const Series& b, const Series& out) { dispatch_cmp(a,b,out,[](auto x, auto y){return x<=y;}, "le"); }
void gt(const Series& a, const Series& b, const Series& out) { dispatch_cmp(a,b,out,[](auto x, auto y){return x> y;}, "gt"); }
void ge(const Series& a, const Series& b, const Series& out) { dispatch_cmp(a,b,out,[](auto x, auto y){return x>=y;}, "ge"); }

} // namespace

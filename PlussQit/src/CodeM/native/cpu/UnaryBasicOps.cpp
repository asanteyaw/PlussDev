#include "CodeM/native/cpu/UnaryBasicOps.h"
#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/Half.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace pluss::codem::native::cpu {

static bool fast_contiguous_unary_ok_(const Series& a, const Series& out) {
  if (!(a.is_contiguous() && out.is_contiguous() && a.numel() == out.numel())) return false;
  if (a.sizes().size() != out.sizes().size()) return false;
  return a.is_contiguous() && out.is_contiguous() &&
         std::equal(a.sizes().begin(), a.sizes().end(), out.sizes().begin());
}

template <class T, class Op>
static void fast_contiguous_unary_(const Series& a, Series& out, Op&& op) {
  const auto n = out.numel();
  const auto* ap = reinterpret_cast<const T*>(a.raw_data());
  auto* optr = reinterpret_cast<T*>(out.raw_data());
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
  for (std::int64_t i=0;i<n;++i) optr[i] = op(ap[i]);
}

template <class T, class Fn>
static void unary_loop(const Series& a, Series& out, Fn&& fn) {
  if (fast_contiguous_unary_ok_(a,out)) { fast_contiguous_unary_<T>(a,out,fn); return; }
  SeriesIterator it = SeriesIterator::unary(a,out);
  auto* op = reinterpret_cast<T*>(out.raw_data());
  const auto* ap = reinterpret_cast<const T*>(a.raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){
    op[lin] = fn(ap[ao]);
  });
}

template <class HalfT, class ToF32, class FromF32, class Fn>
static void unary_half_loop(const Series& a, Series& out, Fn&& fn, ToF32&& to, FromF32&& from) {
  SeriesIterator it = SeriesIterator::unary(a,out);
  auto* op = reinterpret_cast<HalfT*>(out.raw_data());
  const auto* ap = reinterpret_cast<const HalfT*>(a.raw_data());
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){
    op[lin] = from(fn(to(ap[ao])));
  });
}

void sqrt(const Series& a, Series& out) {
  switch (a.scalar_type()) {
    case ScalarType::Float64: unary_loop<double>(a,out,[](double x){ return std::sqrt(x); }); return;
    case ScalarType::Float32: unary_loop<float>(a,out,[](float x){ return std::sqrt(x); }); return;
    case ScalarType::Float16: unary_half_loop<Float16>(a,out,[](float x){ return std::sqrt(x); }, float16_to_float, float16_from_float); return;
    case ScalarType::BFloat16: unary_half_loop<BFloat16>(a,out,[](float x){ return std::sqrt(x); }, bfloat16_to_float, bfloat16_from_float); return;
    default: break;
  }
  throw_error("sqrt: dtype not implemented");
}

void floor(const Series& a, Series& out) {
  switch (a.scalar_type()) {
    case ScalarType::Float64: unary_loop<double>(a,out,[](double x){ return std::floor(x); }); return;
    case ScalarType::Float32: unary_loop<float>(a,out,[](float x){ return std::floor(x); }); return;
    case ScalarType::Float16: unary_half_loop<Float16>(a,out,[](float x){ return std::floor(x); }, float16_to_float, float16_from_float); return;
    case ScalarType::BFloat16: unary_half_loop<BFloat16>(a,out,[](float x){ return std::floor(x); }, bfloat16_to_float, bfloat16_from_float); return;
    default: break;
  }
  throw_error("floor: dtype not implemented");
}

void ceil(const Series& a, Series& out) {
  switch (a.scalar_type()) {
    case ScalarType::Float64: unary_loop<double>(a,out,[](double x){ return std::ceil(x); }); return;
    case ScalarType::Float32: unary_loop<float>(a,out,[](float x){ return std::ceil(x); }); return;
    case ScalarType::Float16: unary_half_loop<Float16>(a,out,[](float x){ return std::ceil(x); }, float16_to_float, float16_from_float); return;
    case ScalarType::BFloat16: unary_half_loop<BFloat16>(a,out,[](float x){ return std::ceil(x); }, bfloat16_to_float, bfloat16_from_float); return;
    default: break;
  }
  throw_error("ceil: dtype not implemented");
}

void round(const Series& a, Series& out) {
  switch (a.scalar_type()) {
    case ScalarType::Float64: unary_loop<double>(a,out,[](double x){ return std::nearbyint(x); }); return;
    case ScalarType::Float32: unary_loop<float>(a,out,[](float x){ return std::nearbyint(x); }); return;
    case ScalarType::Float16: unary_half_loop<Float16>(a,out,[](float x){ return std::nearbyint(x); }, float16_to_float, float16_from_float); return;
    case ScalarType::BFloat16: unary_half_loop<BFloat16>(a,out,[](float x){ return std::nearbyint(x); }, bfloat16_to_float, bfloat16_from_float); return;
    default: break;
  }
  throw_error("round: dtype not implemented");
}

void trunc(const Series& a, Series& out) {
  switch (a.scalar_type()) {
    case ScalarType::Float64: unary_loop<double>(a,out,[](double x){ return std::trunc(x); }); return;
    case ScalarType::Float32: unary_loop<float>(a,out,[](float x){ return std::trunc(x); }); return;
    case ScalarType::Float16: unary_half_loop<Float16>(a,out,[](float x){ return std::trunc(x); }, float16_to_float, float16_from_float); return;
    case ScalarType::BFloat16: unary_half_loop<BFloat16>(a,out,[](float x){ return std::trunc(x); }, bfloat16_to_float, bfloat16_from_float); return;
    default: break;
  }
  throw_error("trunc: dtype not implemented");
}

void isnan(const Series& a, Series& out) {
  PLUSS_CHECK(out.scalar_type()==ScalarType::Bool, "isnan: out must be Bool");
  SeriesIterator it = SeriesIterator::unary(a,out);
  auto* op = reinterpret_cast<std::uint8_t*>(out.raw_data());
  switch (a.scalar_type()) {
    case ScalarType::Float64: {
      const auto* ap = reinterpret_cast<const double*>(a.raw_data());
      it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){ op[lin] = std::isnan(ap[ao]) ? 1 : 0; });
      return;
    }
    case ScalarType::Float32: {
      const auto* ap = reinterpret_cast<const float*>(a.raw_data());
      it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){ op[lin] = std::isnan(ap[ao]) ? 1 : 0; });
      return;
    }
    case ScalarType::Float16: {
      const auto* ap = reinterpret_cast<const Float16*>(a.raw_data());
      it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){ op[lin] = std::isnan(float16_to_float(ap[ao])) ? 1 : 0; });
      return;
    }
    case ScalarType::BFloat16: {
      const auto* ap = reinterpret_cast<const BFloat16*>(a.raw_data());
      it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){ op[lin] = std::isnan(bfloat16_to_float(ap[ao])) ? 1 : 0; });
      return;
    }
    default: break;
  }
  // ints => always false
  it.for_each_index([&](std::int64_t lin, std::int64_t, std::int64_t, std::int64_t){ op[lin] = 0; });
}

void isinf(const Series& a, Series& out) {
  PLUSS_CHECK(out.scalar_type()==ScalarType::Bool, "isinf: out must be Bool");
  SeriesIterator it = SeriesIterator::unary(a,out);
  auto* op = reinterpret_cast<std::uint8_t*>(out.raw_data());
  switch (a.scalar_type()) {
    case ScalarType::Float64: {
      const auto* ap = reinterpret_cast<const double*>(a.raw_data());
      it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){ op[lin] = std::isinf(ap[ao]) ? 1 : 0; });
      return;
    }
    case ScalarType::Float32: {
      const auto* ap = reinterpret_cast<const float*>(a.raw_data());
      it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){ op[lin] = std::isinf(ap[ao]) ? 1 : 0; });
      return;
    }
    case ScalarType::Float16: {
      const auto* ap = reinterpret_cast<const Float16*>(a.raw_data());
      it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){ op[lin] = std::isinf(float16_to_float(ap[ao])) ? 1 : 0; });
      return;
    }
    case ScalarType::BFloat16: {
      const auto* ap = reinterpret_cast<const BFloat16*>(a.raw_data());
      it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){ op[lin] = std::isinf(bfloat16_to_float(ap[ao])) ? 1 : 0; });
      return;
    }
    default: break;
  }
  it.for_each_index([&](std::int64_t lin, std::int64_t, std::int64_t, std::int64_t){ op[lin] = 0; });
}

} // namespace pluss::codem::native::cpu

#include "CodeM/native/cpu/WhereClampKernels.h"
#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/TypePromotion.h"
#include <cmath>
#include <algorithm>

namespace pluss::codem::native::cpu {

void clamp_inplace(const Series& a, double minv, double maxv) {
  PLUSS_CHECK(a.is_contiguous(), "clamp_inplace: requires contiguous");
  const auto n = a.numel();
  switch (a.scalar_type()) {
    case ScalarType::Float32: {
      float* p = reinterpret_cast<float*>(const_cast<std::byte*>(a.raw_data()));
      const float mn = (float)minv, mx = (float)maxv;
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
      for (std::int64_t i=0;i<n;++i) p[i] = std::min(mx, std::max(mn, p[i]));
      return;
    }
    case ScalarType::Float64: {
      double* p = reinterpret_cast<double*>(const_cast<std::byte*>(a.raw_data()));
      const double mn = (double)minv, mx = (double)maxv;
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
      for (std::int64_t i=0;i<n;++i) p[i] = std::min(mx, std::max(mn, p[i]));
      return;
    }
    case ScalarType::Int64: {
      std::int64_t* p = reinterpret_cast<std::int64_t*>(const_cast<std::byte*>(a.raw_data()));
      const std::int64_t mn = (std::int64_t)minv, mx = (std::int64_t)maxv;
      for (std::int64_t i=0;i<n;++i) p[i] = std::min(mx, std::max(mn, p[i]));
      return;
    }
    case ScalarType::Int32: {
      std::int32_t* p = reinterpret_cast<std::int32_t*>(const_cast<std::byte*>(a.raw_data()));
      const std::int32_t mn = (std::int32_t)minv, mx = (std::int32_t)maxv;
      for (std::int64_t i=0;i<n;++i) p[i] = std::min(mx, std::max(mn, p[i]));
      return;
    }
    case ScalarType::Int16: {
      std::int16_t* p = reinterpret_cast<std::int16_t*>(const_cast<std::byte*>(a.raw_data()));
      const std::int16_t mn = (std::int16_t)minv, mx = (std::int16_t)maxv;
      for (std::int64_t i=0;i<n;++i) p[i] = std::min(mx, std::max(mn, p[i]));
      return;
    }
    case ScalarType::Int8: {
      std::int8_t* p = reinterpret_cast<std::int8_t*>(const_cast<std::byte*>(a.raw_data()));
      const std::int8_t mn = (std::int8_t)minv, mx = (std::int8_t)maxv;
      for (std::int64_t i=0;i<n;++i) p[i] = std::min(mx, std::max(mn, p[i]));
      return;
    }
    case ScalarType::UInt64: {
      std::uint64_t* p = reinterpret_cast<std::uint64_t*>(const_cast<std::byte*>(a.raw_data()));
      const std::uint64_t mn = (std::uint64_t)std::max(0.0, minv);
      const std::uint64_t mx = (std::uint64_t)std::max(0.0, maxv);
      for (std::int64_t i=0;i<n;++i) p[i] = std::min(mx, std::max(mn, p[i]));
      return;
    }
    case ScalarType::UInt32: {
      std::uint32_t* p = reinterpret_cast<std::uint32_t*>(const_cast<std::byte*>(a.raw_data()));
      const std::uint32_t mn = (std::uint32_t)std::max(0.0, minv);
      const std::uint32_t mx = (std::uint32_t)std::max(0.0, maxv);
      for (std::int64_t i=0;i<n;++i) p[i] = std::min(mx, std::max(mn, p[i]));
      return;
    }
    case ScalarType::UInt16: {
      std::uint16_t* p = reinterpret_cast<std::uint16_t*>(const_cast<std::byte*>(a.raw_data()));
      const std::uint16_t mn = (std::uint16_t)std::max(0.0, minv);
      const std::uint16_t mx = (std::uint16_t)std::max(0.0, maxv);
      for (std::int64_t i=0;i<n;++i) p[i] = std::min(mx, std::max(mn, p[i]));
      return;
    }
    case ScalarType::UInt8: {
      std::uint8_t* p = reinterpret_cast<std::uint8_t*>(const_cast<std::byte*>(a.raw_data()));
      const std::uint8_t mn = (std::uint8_t)std::max(0.0, minv);
      const std::uint8_t mx = (std::uint8_t)std::max(0.0, maxv);
      for (std::int64_t i=0;i<n;++i) p[i] = std::min(mx, std::max(mn, p[i]));
      return;
    }
    case ScalarType::Float16: {
      Float16* p = reinterpret_cast<Float16*>(const_cast<std::byte*>(a.raw_data()));
      const float mn = (float)minv, mx = (float)maxv;
      for (std::int64_t i=0;i<n;++i) {
        const float x = float16_to_float(p[i]);
        p[i] = float16_from_float(std::min(mx, std::max(mn, x)));
      }
      return;
    }
    case ScalarType::BFloat16: {
      BFloat16* p = reinterpret_cast<BFloat16*>(const_cast<std::byte*>(a.raw_data()));
      const float mn = (float)minv, mx = (float)maxv;
      for (std::int64_t i=0;i<n;++i) {
        const float x = bfloat16_to_float(p[i]);
        p[i] = bfloat16_from_float(std::min(mx, std::max(mn, x)));
      }
      return;
    }
    default:
      throw_error("clamp_inplace: dtype not implemented");
  }
}

void relu_inplace(const Series& a) {
  PLUSS_CHECK(a.is_contiguous(), "relu_inplace: requires contiguous");
  const auto n = a.numel();
  switch (a.scalar_type()) {
    case ScalarType::Float32: {
      float* p = reinterpret_cast<float*>(const_cast<std::byte*>(a.raw_data()));
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
      for (std::int64_t i=0;i<n;++i) p[i] = std::max(0.0f, p[i]);
      return;
    }
    case ScalarType::Float64: {
      double* p = reinterpret_cast<double*>(const_cast<std::byte*>(a.raw_data()));
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
      for (std::int64_t i=0;i<n;++i) p[i] = std::max(0.0, p[i]);
      return;
    }
    default:
      throw_error("relu_inplace: dtype not implemented");
  }
}



void where(const Series& cond, const Series& a, const Series& b, Series& out) {
  PLUSS_CHECK(a.scalar_type() == out.scalar_type() && b.scalar_type() == out.scalar_type(),
              "where: current kernel requires a,b,out have same dtype (promotion handled at Series level)");

  Series cond_bool = (cond.scalar_type() == ScalarType::Bool) ? cond : cond.to(ScalarType::Bool);
  SeriesIterator it = SeriesIterator::ternary(cond_bool, a, b, out);
  const auto* mp = reinterpret_cast<const std::uint8_t*>(cond_bool.raw_data());
  switch (out.scalar_type()) {
    case ScalarType::Float64: {
      auto* op = reinterpret_cast<double*>(out.raw_data());
      auto* ap = reinterpret_cast<const double*>(a.raw_data());
      auto* bp = reinterpret_cast<const double*>(b.raw_data());
      it.for_each_index_ternary([&](std::int64_t /*lin*/, std::int64_t mo, std::int64_t ao, std::int64_t bo, std::int64_t oo){
        op[oo] = mp[mo] ? ap[ao] : bp[bo];
      });
      return;
    }
    case ScalarType::Float32: {
      auto* op = reinterpret_cast<float*>(out.raw_data());
      auto* ap = reinterpret_cast<const float*>(a.raw_data());
      auto* bp = reinterpret_cast<const float*>(b.raw_data());
      it.for_each_index_ternary([&](std::int64_t /*lin*/, std::int64_t mo, std::int64_t ao, std::int64_t bo, std::int64_t oo){
        op[oo] = mp[mo] ? ap[ao] : bp[bo];
      });
      return;
    }
    case ScalarType::Int64: {
      auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());
      auto* ap = reinterpret_cast<const std::int64_t*>(a.raw_data());
      auto* bp = reinterpret_cast<const std::int64_t*>(b.raw_data());
      it.for_each_index_ternary([&](std::int64_t /*lin*/, std::int64_t mo, std::int64_t ao, std::int64_t bo, std::int64_t oo){
        op[oo] = mp[mo] ? ap[ao] : bp[bo];
      });
      return;
    }
    default: break;
  }
  throw_error("where: dtype not implemented");
}

template <class T>
static void clamp_min_loop(const Series& a, double min_v, Series& out) {
  SeriesIterator it = SeriesIterator::unary(a,out);
  auto* op = reinterpret_cast<T*>(out.raw_data());
  auto* ap = reinterpret_cast<const T*>(a.raw_data());
  const T mv = (T)min_v;
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){
    const T x = ap[ao];
    op[lin] = (x < mv) ? mv : x;
  });
}
template <class T>
static void clamp_max_loop(const Series& a, double max_v, Series& out) {
  SeriesIterator it = SeriesIterator::unary(a,out);
  auto* op = reinterpret_cast<T*>(out.raw_data());
  auto* ap = reinterpret_cast<const T*>(a.raw_data());
  const T mv = (T)max_v;
  it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){
    const T x = ap[ao];
    op[lin] = (x > mv) ? mv : x;
  });
}

void clamp_min(const Series& a, double min_v, Series& out) {
  PLUSS_CHECK(a.scalar_type()==out.scalar_type(), "clamp_min: dtype mismatch");
  switch (a.scalar_type()) {
    case ScalarType::Float64: clamp_min_loop<double>(a,min_v,out); return;
    case ScalarType::Float32: clamp_min_loop<float>(a,min_v,out); return;
    case ScalarType::Int32: clamp_min_loop<std::int32_t>(a,min_v,out); return;
    case ScalarType::Int64: clamp_min_loop<std::int64_t>(a,min_v,out); return;
    default: break;
  }
  throw_error("clamp_min: dtype not implemented");
}

void clamp_max(const Series& a, double max_v, Series& out) {
  PLUSS_CHECK(a.scalar_type()==out.scalar_type(), "clamp_max: dtype mismatch");
  switch (a.scalar_type()) {
    case ScalarType::Float64: clamp_max_loop<double>(a,max_v,out); return;
    case ScalarType::Float32: clamp_max_loop<float>(a,max_v,out); return;
    case ScalarType::Int32: clamp_max_loop<std::int32_t>(a,max_v,out); return;
    case ScalarType::Int64: clamp_max_loop<std::int64_t>(a,max_v,out); return;
    default: break;
  }
  throw_error("clamp_max: dtype not implemented");
}

void nan_to_num(const Series& a, double nan, double posinf, double neginf, Series& out) {
  PLUSS_CHECK(a.scalar_type()==out.scalar_type(), "nan_to_num: dtype mismatch");
  SeriesIterator it = SeriesIterator::unary(a,out);
  switch (a.scalar_type()) {
    case ScalarType::Float64: {
      auto* op = reinterpret_cast<double*>(out.raw_data());
      auto* ap = reinterpret_cast<const double*>(a.raw_data());
      it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){
        double x = ap[ao];
        if (std::isnan(x)) x = nan;
        else if (std::isinf(x)) x = (x > 0) ? posinf : neginf;
        op[lin] = x;
      });
      return;
    }
    case ScalarType::Float32: {
      auto* op = reinterpret_cast<float*>(out.raw_data());
      auto* ap = reinterpret_cast<const float*>(a.raw_data());
      const float n = (float)nan, pi=(float)posinf, ni=(float)neginf;
      it.for_each_index([&](std::int64_t lin, std::int64_t ao, std::int64_t, std::int64_t){
        float x = ap[ao];
        if (std::isnan(x)) x = n;
        else if (std::isinf(x)) x = (x > 0) ? pi : ni;
        op[lin] = x;
      });
      return;
    }
    default: break;
  }
  throw_error("nan_to_num: dtype not implemented");
}

} // namespace

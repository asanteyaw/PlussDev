#include "CodeM/native/cpu/ActivationKernels.h"
#include "CodeM/core/Errors.h"
#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/Half.h"

#include <cmath>
#include <algorithm>
#include <cstring>

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

static void check_real(const Series& a, const char* name) {
  if (a.scalar_type() == ScalarType::Complex64 || a.scalar_type() == ScalarType::Complex128) {
    throw_error(std::string(name) + ": complex dtype not supported");
  }
}

static inline float sigmoidf(float x) {
  // stable sigmoid
  if (x >= 0) {
    const float z = std::exp(-x);
    return 1.0f / (1.0f + z);
  } else {
    const float z = std::exp(x);
    return z / (1.0f + z);
  }
}



static bool fast_tanh_contig(const Series& a, Series& out) {
  if (!(a.is_contiguous() && out.is_contiguous())) return false;
  if (a.scalar_type() != out.scalar_type()) return false;
  const auto n = a.numel();
  if (a.scalar_type() == ScalarType::Float32) {
    const float* ap = reinterpret_cast<const float*>(a.raw_data());
    float* op = reinterpret_cast<float*>(out.raw_data());
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
    for (std::int64_t i=0;i<n;++i) op[i] = std::tanh(ap[i]);
    return true;
  }
  if (a.scalar_type() == ScalarType::Float64) {
    const double* ap = reinterpret_cast<const double*>(a.raw_data());
    double* op = reinterpret_cast<double*>(out.raw_data());
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
    for (std::int64_t i=0;i<n;++i) op[i] = std::tanh(ap[i]);
    return true;
  }
  return false;
}

static inline float softplus_f(float x) {
  // stable softplus: log1p(exp(x)) with threshold
  if (x > 20.0f) return x;
  if (x < -20.0f) return std::exp(x);
  return std::log1p(std::exp(x));
}
static inline double softplus_d(double x) {
  if (x > 20.0) return x;
  if (x < -20.0) return std::exp(x);
  return std::log1p(std::exp(x));
}

static bool fast_softplus_contig(const Series& a, Series& out) {
  if (!(a.is_contiguous() && out.is_contiguous())) return false;
  if (a.scalar_type() != out.scalar_type()) return false;
  const auto n = a.numel();
  if (a.scalar_type() == ScalarType::Float32) {
    const float* ap = reinterpret_cast<const float*>(a.raw_data());
    float* op = reinterpret_cast<float*>(out.raw_data());
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
    for (std::int64_t i=0;i<n;++i) op[i] = softplus_f(ap[i]);
    return true;
  }
  if (a.scalar_type() == ScalarType::Float64) {
    const double* ap = reinterpret_cast<const double*>(a.raw_data());
    double* op = reinterpret_cast<double*>(out.raw_data());
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
    for (std::int64_t i=0;i<n;++i) op[i] = softplus_d(ap[i]);
    return true;
  }
  return false;
}

static bool fast_relu_contig(const Series& a, Series& out) {
  if (!(a.is_contiguous() && out.is_contiguous())) return false;
  if (a.scalar_type() != out.scalar_type()) return false;
  const auto n = a.numel();
  if (a.scalar_type() == ScalarType::Float32) {
    const float* ap = reinterpret_cast<const float*>(a.raw_data());
    float* op = reinterpret_cast<float*>(out.raw_data());
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
    for (std::int64_t i=0;i<n;++i) op[i] = (ap[i] > 0.0f) ? ap[i] : 0.0f;
    return true;
  }
  if (a.scalar_type() == ScalarType::Float64) {
    const double* ap = reinterpret_cast<const double*>(a.raw_data());
    double* op = reinterpret_cast<double*>(out.raw_data());
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
    for (std::int64_t i=0;i<n;++i) op[i] = (ap[i] > 0.0) ? ap[i] : 0.0;
    return true;
  }
  return false;
}

static bool fast_sigmoid_contig(const Series& a, Series& out) {
  if (!(a.is_contiguous() && out.is_contiguous())) return false;
  if (a.scalar_type() != out.scalar_type()) return false;
  const auto n = a.numel();
  if (a.scalar_type() == ScalarType::Float32) {
    const float* ap = reinterpret_cast<const float*>(a.raw_data());
    float* op = reinterpret_cast<float*>(out.raw_data());
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
    for (std::int64_t i=0;i<n;++i) op[i] = sigmoidf(ap[i]);
    return true;
  }
  if (a.scalar_type() == ScalarType::Float64) {
    const double* ap = reinterpret_cast<const double*>(a.raw_data());
    double* op = reinterpret_cast<double*>(out.raw_data());
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
    for (std::int64_t i=0;i<n;++i) op[i] = 1.0 / (1.0 + std::exp(-ap[i]));
    return true;
  }
  return false;
}

void sigmoid(const Series& a, Series& out) {
  if (fast_sigmoid_contig(a,out)) return;
  check_real(a, "sigmoid");
  switch (a.scalar_type()) {
    case ScalarType::Float64: unary_apply<double>(a, out, [](double x){ return 1.0 / (1.0 + std::exp(-x)); }); return;
    case ScalarType::Float32: unary_apply<float>(a, out, [](float x){ return sigmoidf(x); }); return;
    case ScalarType::Float16:
      unary_apply_convert<Float16, Float16>(a, out,
        [](Float16 x){ return float16_to_float(x); },
        [](float y){ return float16_from_float(y); },
        [](float x){ return sigmoidf(x); });
      return;
    case ScalarType::BFloat16:
      unary_apply_convert<BFloat16, BFloat16>(a, out,
        [](BFloat16 x){ return bfloat16_to_float(x); },
        [](float y){ return bfloat16_from_float(y); },
        [](float x){ return sigmoidf(x); });
      return;
    default: break;
  }
  throw_error("sigmoid: dtype not implemented");
}

void sigmoid_inplace(const Series& a) {
  PLUSS_CHECK(a.is_contiguous(), "sigmoid_inplace: requires contiguous");
  Series out = Series::empty(a.sizes(), a.scalar_type());
  sigmoid(a, out);
  std::memcpy(const_cast<std::byte*>(a.raw_data()), out.raw_data(), (std::size_t)(a.dtype().itemsize * a.numel()));
}

static inline float softplusf(float x, float beta, float threshold) {
  // Torch: softplus(x) = 1/beta * log(1 + exp(beta*x)), with threshold to avoid overflow
  const float bx = beta * x;
  if (bx > threshold) return x; // approx to x/beta? Torch returns x for large bx (since log(1+exp)=bx)
  return (1.0f / beta) * std::log1p(std::exp(bx));
}

void softplus(const Series& a, Series& out, double beta, double threshold) {
  check_real(a, "softplus");
  const float b = (float)beta;
  const float th = (float)threshold;
  switch (a.scalar_type()) {
    case ScalarType::Float64:
      unary_apply<double>(a, out, [&](double x){
        const double bx = beta * x;
        if (bx > threshold) return x;
        return (1.0/beta) * std::log1p(std::exp(bx));
      });
      return;
    case ScalarType::Float32:
      unary_apply<float>(a, out, [&](float x){ return softplusf(x,b,th); });
      return;
    case ScalarType::Float16:
      unary_apply_convert<Float16, Float16>(a, out,
        [](Float16 x){ return float16_to_float(x); },
        [](float y){ return float16_from_float(y); },
        [&](float x){ return softplusf(x,b,th); });
      return;
    case ScalarType::BFloat16:
      unary_apply_convert<BFloat16, BFloat16>(a, out,
        [](BFloat16 x){ return bfloat16_to_float(x); },
        [](float y){ return bfloat16_from_float(y); },
        [&](float x){ return softplusf(x,b,th); });
      return;
    default: break;
  }
  throw_error("softplus: dtype not implemented");
}

void softplus_inplace(const Series& a, double beta, double threshold) {
  PLUSS_CHECK(a.is_contiguous(), "softplus_inplace: requires contiguous");
  Series out = Series::empty(a.sizes(), a.scalar_type());
  softplus(a, out, beta, threshold);
  std::memcpy(const_cast<std::byte*>(a.raw_data()), out.raw_data(), (std::size_t)(a.dtype().itemsize * a.numel()));
}

void silu(const Series& a, Series& out) {
  // silu(x) = x * sigmoid(x)
  check_real(a, "silu");
  switch (a.scalar_type()) {
    case ScalarType::Float64:
      unary_apply<double>(a, out, [](double x){ return x / (1.0 + std::exp(-x)); });
      return;
    case ScalarType::Float32:
      unary_apply<float>(a, out, [](float x){ return x * sigmoidf(x); });
      return;
    case ScalarType::Float16:
      unary_apply_convert<Float16, Float16>(a, out,
        [](Float16 x){ return float16_to_float(x); },
        [](float y){ return float16_from_float(y); },
        [](float x){ return x * sigmoidf(x); });
      return;
    case ScalarType::BFloat16:
      unary_apply_convert<BFloat16, BFloat16>(a, out,
        [](BFloat16 x){ return bfloat16_to_float(x); },
        [](float y){ return bfloat16_from_float(y); },
        [](float x){ return x * sigmoidf(x); });
      return;
    default: break;
  }
  throw_error("silu: dtype not implemented");
}

void silu_inplace(const Series& a) {
  PLUSS_CHECK(a.is_contiguous(), "silu_inplace: requires contiguous");
  Series out = Series::empty(a.sizes(), a.scalar_type());
  silu(a,out);
  std::memcpy(const_cast<std::byte*>(a.raw_data()), out.raw_data(), (std::size_t)(a.dtype().itemsize * a.numel()));
}

static inline float gelu_tanh_approx(float x) {
  // 0.5*x*(1+tanh(sqrt(2/pi)*(x+0.044715*x^3)))
  const float k0 = 0.7978845608028654f; // sqrt(2/pi)
  const float k1 = 0.044715f;
  const float x3 = x*x*x;
  return 0.5f*x*(1.0f + std::tanh(k0*(x + k1*x3)));
}

void gelu(const Series& a, Series& out, bool approximate) {
  check_real(a, "gelu");
  switch (a.scalar_type()) {
    case ScalarType::Float64:
      unary_apply<double>(a, out, [&](double x){
        if (approximate) {
          const double k0 = 0.7978845608028654;
          const double k1 = 0.044715;
          const double x3 = x*x*x;
          return 0.5*x*(1.0 + std::tanh(k0*(x + k1*x3)));
        }
        // exact using erf
        return 0.5*x*(1.0 + std::erf(x / std::sqrt(2.0)));
      });
      return;
    case ScalarType::Float32:
      unary_apply<float>(a, out, [&](float x){
        return approximate ? gelu_tanh_approx(x) : (0.5f*x*(1.0f + std::erff(x * 0.7071067811865475f)));
      });
      return;
    case ScalarType::Float16:
      unary_apply_convert<Float16, Float16>(a, out,
        [](Float16 x){ return float16_to_float(x); },
        [](float y){ return float16_from_float(y); },
        [&](float x){
          return approximate ? gelu_tanh_approx(x) : (0.5f*x*(1.0f + std::erff(x * 0.7071067811865475f)));
        });
      return;
    case ScalarType::BFloat16:
      unary_apply_convert<BFloat16, BFloat16>(a, out,
        [](BFloat16 x){ return bfloat16_to_float(x); },
        [](float y){ return bfloat16_from_float(y); },
        [&](float x){
          return approximate ? gelu_tanh_approx(x) : (0.5f*x*(1.0f + std::erff(x * 0.7071067811865475f)));
        });
      return;
    default: break;
  }
  throw_error("gelu: dtype not implemented");
}

void gelu_inplace(const Series& a, bool approximate) {
  PLUSS_CHECK(a.is_contiguous(), "gelu_inplace: requires contiguous");
  Series out = Series::empty(a.sizes(), a.scalar_type());
  gelu(a,out,approximate);
  std::memcpy(const_cast<std::byte*>(a.raw_data()), out.raw_data(), (std::size_t)(a.dtype().itemsize * a.numel()));
}

void leaky_relu(const Series& a, Series& out, double negative_slope) {
  check_real(a, "leaky_relu");
  const float ns = (float)negative_slope;
  switch (a.scalar_type()) {
    case ScalarType::Float64:
      unary_apply<double>(a, out, [&](double x){ return x >= 0 ? x : x*negative_slope; });
      return;
    case ScalarType::Float32:
      unary_apply<float>(a, out, [&](float x){ return x >= 0 ? x : x*ns; });
      return;
    case ScalarType::Float16:
      unary_apply_convert<Float16, Float16>(a, out,
        [](Float16 x){ return float16_to_float(x); },
        [](float y){ return float16_from_float(y); },
        [&](float x){ return x >= 0 ? x : x*ns; });
      return;
    case ScalarType::BFloat16:
      unary_apply_convert<BFloat16, BFloat16>(a, out,
        [](BFloat16 x){ return bfloat16_to_float(x); },
        [](float y){ return bfloat16_from_float(y); },
        [&](float x){ return x >= 0 ? x : x*ns; });
      return;
    default: break;
  }
  throw_error("leaky_relu: dtype not implemented");
}

void leaky_relu_inplace(const Series& a, double negative_slope) {
  PLUSS_CHECK(a.is_contiguous(), "leaky_relu_inplace: requires contiguous");
  Series out = Series::empty(a.sizes(), a.scalar_type());
  leaky_relu(a,out,negative_slope);
  std::memcpy(const_cast<std::byte*>(a.raw_data()), out.raw_data(), (std::size_t)(a.dtype().itemsize * a.numel()));
}

} // namespace pluss::codem::native::cpu

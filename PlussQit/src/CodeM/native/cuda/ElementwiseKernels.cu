#include "CodeM/native/cuda/ElementwiseOps.h"

#include "CodeM/core/Storage.h"

#include <cuda_runtime_api.h>

#include <cmath>
#include <cstdint>

namespace pluss::codem::native::cuda {
namespace {

constexpr int kBlockSize = 256;

// psi(x): iterative recurrence to x >= 6 then Bernoulli asymptotic tail, with
// the reflection formula for x <= 0. Matches the CPU digamma kernel.
__device__ inline double pluss_digamma_device_(double x) {
  if (isnan(x)) return x;
  double reflect = 0.0;
  if (x <= 0.0) {
    if (x == floor(x)) return nan("");
    const double kPi = 3.14159265358979323846;
    reflect = -kPi / tan(kPi * x);
    x = 1.0 - x;
  }
  double acc = 0.0;
  while (x < 6.0) {
    acc -= 1.0 / x;
    x += 1.0;
  }
  const double inv = 1.0 / x;
  const double inv2 = inv * inv;
  acc += log(x) - 0.5 * inv -
      inv2 * (1.0 / 12.0 - inv2 * (1.0 / 120.0 - inv2 * (1.0 / 252.0)));
  return acc + reflect;
}

template <typename T>
__device__ T unary_apply_(native::cpu::UnaryOpId op, T x) {
  switch (op) {
    case native::cpu::UnaryOpId::Neg:
      return -x;
    case native::cpu::UnaryOpId::Abs:
      return x < T(0) ? -x : x;
    case native::cpu::UnaryOpId::Sqrt:
      return static_cast<T>(sqrt(static_cast<double>(x)));
    case native::cpu::UnaryOpId::Exp:
      return static_cast<T>(exp(static_cast<double>(x)));
    case native::cpu::UnaryOpId::Log:
      return static_cast<T>(log(static_cast<double>(x)));
    case native::cpu::UnaryOpId::Sin:
      return static_cast<T>(sin(static_cast<double>(x)));
    case native::cpu::UnaryOpId::Cos:
      return static_cast<T>(cos(static_cast<double>(x)));
    case native::cpu::UnaryOpId::Tanh:
      return static_cast<T>(tanh(static_cast<double>(x)));
    case native::cpu::UnaryOpId::Tan:
      return static_cast<T>(tan(static_cast<double>(x)));
    case native::cpu::UnaryOpId::Asin:
      return static_cast<T>(asin(static_cast<double>(x)));
    case native::cpu::UnaryOpId::Acos:
      return static_cast<T>(acos(static_cast<double>(x)));
    case native::cpu::UnaryOpId::Atan:
      return static_cast<T>(atan(static_cast<double>(x)));
    case native::cpu::UnaryOpId::Sinh:
      return static_cast<T>(sinh(static_cast<double>(x)));
    case native::cpu::UnaryOpId::Cosh:
      return static_cast<T>(cosh(static_cast<double>(x)));
    case native::cpu::UnaryOpId::Floor:
      return static_cast<T>(floor(static_cast<double>(x)));
    case native::cpu::UnaryOpId::Ceil:
      return static_cast<T>(ceil(static_cast<double>(x)));
    case native::cpu::UnaryOpId::Round:
      return static_cast<T>(round(static_cast<double>(x)));
    case native::cpu::UnaryOpId::Trunc:
      return static_cast<T>(trunc(static_cast<double>(x)));
    case native::cpu::UnaryOpId::Reciprocal:
      return T(1) / x;
    case native::cpu::UnaryOpId::Log1p:
      return static_cast<T>(log1p(static_cast<double>(x)));
    case native::cpu::UnaryOpId::Expm1:
      return static_cast<T>(expm1(static_cast<double>(x)));
    case native::cpu::UnaryOpId::Erf:
      return static_cast<T>(erf(static_cast<double>(x)));
    case native::cpu::UnaryOpId::Erfc:
      return static_cast<T>(erfc(static_cast<double>(x)));
    case native::cpu::UnaryOpId::LGamma:
      return static_cast<T>(lgamma(static_cast<double>(x)));
    case native::cpu::UnaryOpId::Digamma:
      return static_cast<T>(pluss_digamma_device_(static_cast<double>(x)));
    case native::cpu::UnaryOpId::Asinh:
      return static_cast<T>(asinh(static_cast<double>(x)));
    case native::cpu::UnaryOpId::Acosh:
      return static_cast<T>(acosh(static_cast<double>(x)));
    case native::cpu::UnaryOpId::Atanh:
      return static_cast<T>(atanh(static_cast<double>(x)));
    case native::cpu::UnaryOpId::Rsqrt:
      return T(1) / static_cast<T>(sqrt(static_cast<double>(x)));
  }
  return x;
}

template <>
__device__ float unary_apply_<float>(native::cpu::UnaryOpId op, float x) {
  switch (op) {
    case native::cpu::UnaryOpId::Neg:
      return -x;
    case native::cpu::UnaryOpId::Abs:
      return fabsf(x);
    case native::cpu::UnaryOpId::Sqrt:
      return sqrtf(x);
    case native::cpu::UnaryOpId::Exp:
      return expf(x);
    case native::cpu::UnaryOpId::Log:
      return logf(x);
    case native::cpu::UnaryOpId::Sin:
      return sinf(x);
    case native::cpu::UnaryOpId::Cos:
      return cosf(x);
    case native::cpu::UnaryOpId::Tanh:
      return tanhf(x);
    case native::cpu::UnaryOpId::Tan:
      return tanf(x);
    case native::cpu::UnaryOpId::Asin:
      return asinf(x);
    case native::cpu::UnaryOpId::Acos:
      return acosf(x);
    case native::cpu::UnaryOpId::Atan:
      return atanf(x);
    case native::cpu::UnaryOpId::Sinh:
      return sinhf(x);
    case native::cpu::UnaryOpId::Cosh:
      return coshf(x);
    case native::cpu::UnaryOpId::Floor:
      return floorf(x);
    case native::cpu::UnaryOpId::Ceil:
      return ceilf(x);
    case native::cpu::UnaryOpId::Round:
      return roundf(x);
    case native::cpu::UnaryOpId::Trunc:
      return truncf(x);
    case native::cpu::UnaryOpId::Reciprocal:
      return 1.0f / x;
    case native::cpu::UnaryOpId::Log1p:
      return log1pf(x);
    case native::cpu::UnaryOpId::Expm1:
      return expm1f(x);
    case native::cpu::UnaryOpId::Erf:
      return erff(x);
    case native::cpu::UnaryOpId::Erfc:
      return erfcf(x);
    case native::cpu::UnaryOpId::LGamma:
      return lgammaf(x);
    case native::cpu::UnaryOpId::Digamma:
      return static_cast<float>(pluss_digamma_device_(static_cast<double>(x)));
    case native::cpu::UnaryOpId::Asinh:
      return asinhf(x);
    case native::cpu::UnaryOpId::Acosh:
      return acoshf(x);
    case native::cpu::UnaryOpId::Atanh:
      return atanhf(x);
    case native::cpu::UnaryOpId::Rsqrt:
      return rsqrtf(x);
  }
  return x;
}

template <typename T>
__device__ T binary_apply_(native::cpu::BinaryOpId op, T a, T b) {
  switch (op) {
    case native::cpu::BinaryOpId::Add:
      return a + b;
    case native::cpu::BinaryOpId::Sub:
      return a - b;
    case native::cpu::BinaryOpId::Mul:
      return a * b;
    case native::cpu::BinaryOpId::Div:
      return a / b;
    case native::cpu::BinaryOpId::Pow:
      return static_cast<T>(pow(static_cast<double>(a), static_cast<double>(b)));
    case native::cpu::BinaryOpId::Maximum:
      return a > b ? a : b;
    case native::cpu::BinaryOpId::Minimum:
      return a < b ? a : b;
    default:
      return T(0);
  }
}

template <>
__device__ float binary_apply_<float>(native::cpu::BinaryOpId op, float a, float b) {
  switch (op) {
    case native::cpu::BinaryOpId::Add:
      return a + b;
    case native::cpu::BinaryOpId::Sub:
      return a - b;
    case native::cpu::BinaryOpId::Mul:
      return a * b;
    case native::cpu::BinaryOpId::Div:
      return a / b;
    case native::cpu::BinaryOpId::Pow:
      return powf(a, b);
    case native::cpu::BinaryOpId::Maximum:
      return fmaxf(a, b);
    case native::cpu::BinaryOpId::Minimum:
      return fminf(a, b);
    default:
      return 0.0f;
  }
}

template <typename T>
__device__ bool compare_apply_(native::cpu::BinaryOpId op, T a, T b) {
  switch (op) {
    case native::cpu::BinaryOpId::Eq:
      return a == b;
    case native::cpu::BinaryOpId::Ne:
      return a != b;
    case native::cpu::BinaryOpId::Lt:
      return a < b;
    case native::cpu::BinaryOpId::Le:
      return a <= b;
    case native::cpu::BinaryOpId::Gt:
      return a > b;
    case native::cpu::BinaryOpId::Ge:
      return a >= b;
    default:
      return false;
  }
}

__host__ __device__ bool is_compare_op_(native::cpu::BinaryOpId op) {
  return op == native::cpu::BinaryOpId::Eq ||
      op == native::cpu::BinaryOpId::Ne ||
      op == native::cpu::BinaryOpId::Lt ||
      op == native::cpu::BinaryOpId::Le ||
      op == native::cpu::BinaryOpId::Gt ||
      op == native::cpu::BinaryOpId::Ge;
}

template <typename T>
__global__ void unary_kernel_(native::cpu::UnaryOpId op, const T* in, T* out, std::int64_t n) {
  const std::int64_t i = static_cast<std::int64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (i < n) {
    out[i] = unary_apply_(op, in[i]);
  }
}

template <typename T>
__global__ void binary_kernel_(
    native::cpu::BinaryOpId op,
    const T* a,
    const T* b,
    T* out,
    std::int64_t n,
    bool a_scalar,
    bool b_scalar) {
  const std::int64_t i = static_cast<std::int64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (i < n) {
    out[i] = binary_apply_(op, a_scalar ? a[0] : a[i], b_scalar ? b[0] : b[i]);
  }
}

template <typename T>
__global__ void compare_kernel_(
    native::cpu::BinaryOpId op,
    const T* a,
    const T* b,
    std::uint8_t* out,
    std::int64_t n,
    bool a_scalar,
    bool b_scalar) {
  const std::int64_t i = static_cast<std::int64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (i < n) {
    out[i] = compare_apply_(op, a_scalar ? a[0] : a[i], b_scalar ? b[0] : b[i]) ? std::uint8_t{1} : std::uint8_t{0};
  }
}

template <typename T>
__global__ void where_kernel_(
    const std::uint8_t* cond,
    const T* a,
    const T* b,
    T* out,
    std::int64_t n,
    bool cond_scalar,
    bool a_scalar,
    bool b_scalar) {
  const std::int64_t i = static_cast<std::int64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (i < n) {
    out[i] = (cond_scalar ? cond[0] : cond[i]) ? (a_scalar ? a[0] : a[i]) : (b_scalar ? b[0] : b[i]);
  }
}

bool valid_common_(const Series& a, const Series& out) noexcept {
  return a.platform() == Platform::CUDA &&
      out.platform() == Platform::CUDA &&
      a.is_contiguous() &&
      out.is_contiguous() &&
      a.storage().location() == StorageLocation::CudaDevice &&
      out.storage().location() == StorageLocation::CudaDevice &&
      a.numel() == out.numel();
}

bool valid_common_(const Series& a, const Series& b, const Series& out) noexcept {
  return valid_common_(a, out) &&
      b.platform() == Platform::CUDA &&
      b.is_contiguous() &&
      b.storage().location() == StorageLocation::CudaDevice &&
      (b.numel() == out.numel() || b.numel() == 1) &&
      (a.numel() == out.numel() || a.numel() == 1) &&
      a.scalar_type() == b.scalar_type();
}

bool finish_kernel_() noexcept {
  return cudaPeekAtLastError() == cudaSuccess;
}

} // namespace

bool run_unary_elementwise(native::cpu::UnaryOpId op, const Series& a, Series& out) noexcept {
  if (!valid_common_(a, out) || a.scalar_type() != out.scalar_type()) {
    return false;
  }
  const std::int64_t n = a.numel();
  if (n == 0) {
    return true;
  }
  const int grid = static_cast<int>((n + kBlockSize - 1) / kBlockSize);
  switch (a.scalar_type()) {
    case ScalarType::Float32:
      unary_kernel_<<<grid, kBlockSize>>>(
          op,
          reinterpret_cast<const float*>(a.raw_data()),
          reinterpret_cast<float*>(out.raw_mutable_data()),
          n);
      return finish_kernel_();
    case ScalarType::Float64:
      unary_kernel_<<<grid, kBlockSize>>>(
          op,
          reinterpret_cast<const double*>(a.raw_data()),
          reinterpret_cast<double*>(out.raw_mutable_data()),
          n);
      return finish_kernel_();
    default:
      return false;
  }
}

bool run_binary_elementwise(native::cpu::BinaryOpId op, const Series& a, const Series& b, Series& out) noexcept {
  if (!valid_common_(a, b, out)) {
    return false;
  }
  const std::int64_t n = a.numel();
  const std::int64_t out_n = out.numel();
  if (out_n == 0) {
    return true;
  }
  const int grid = static_cast<int>((out_n + kBlockSize - 1) / kBlockSize);
  const bool a_scalar = a.numel() == 1;
  const bool b_scalar = b.numel() == 1;
  if (is_compare_op_(op)) {
    if (out.scalar_type() != ScalarType::Bool) {
      return false;
    }
    switch (a.scalar_type()) {
      case ScalarType::Float32:
        compare_kernel_<<<grid, kBlockSize>>>(
            op,
            reinterpret_cast<const float*>(a.raw_data()),
            reinterpret_cast<const float*>(b.raw_data()),
            reinterpret_cast<std::uint8_t*>(out.raw_mutable_data()),
            out_n,
            a_scalar,
            b_scalar);
        return finish_kernel_();
      case ScalarType::Float64:
        compare_kernel_<<<grid, kBlockSize>>>(
            op,
            reinterpret_cast<const double*>(a.raw_data()),
            reinterpret_cast<const double*>(b.raw_data()),
            reinterpret_cast<std::uint8_t*>(out.raw_mutable_data()),
            out_n,
            a_scalar,
            b_scalar);
        return finish_kernel_();
      default:
        return false;
    }
  }
  if (out.scalar_type() != a.scalar_type()) {
    return false;
  }
  switch (a.scalar_type()) {
    case ScalarType::Float32:
      binary_kernel_<<<grid, kBlockSize>>>(
          op,
          reinterpret_cast<const float*>(a.raw_data()),
          reinterpret_cast<const float*>(b.raw_data()),
          reinterpret_cast<float*>(out.raw_mutable_data()),
          out_n,
          a_scalar,
          b_scalar);
      return finish_kernel_();
    case ScalarType::Float64:
      binary_kernel_<<<grid, kBlockSize>>>(
          op,
          reinterpret_cast<const double*>(a.raw_data()),
          reinterpret_cast<const double*>(b.raw_data()),
          reinterpret_cast<double*>(out.raw_mutable_data()),
          out_n,
          a_scalar,
          b_scalar);
      return finish_kernel_();
    default:
      return false;
  }
}

bool run_where_elementwise(const Series& cond, const Series& a, const Series& b, Series& out) noexcept {
  if (!valid_common_(a, b, out) ||
      cond.platform() != Platform::CUDA ||
      cond.scalar_type() != ScalarType::Bool ||
      cond.is_contiguous() == false ||
      cond.storage().location() != StorageLocation::CudaDevice ||
      (cond.numel() != out.numel() && cond.numel() != 1) ||
      out.scalar_type() != a.scalar_type()) {
    return false;
  }
  const std::int64_t n = out.numel();
  if (n == 0) {
    return true;
  }
  const int grid = static_cast<int>((n + kBlockSize - 1) / kBlockSize);
  const bool cond_scalar = cond.numel() == 1;
  const bool a_scalar = a.numel() == 1;
  const bool b_scalar = b.numel() == 1;
  switch (out.scalar_type()) {
    case ScalarType::Float32:
      where_kernel_<<<grid, kBlockSize>>>(
          reinterpret_cast<const std::uint8_t*>(cond.raw_data()),
          reinterpret_cast<const float*>(a.raw_data()),
          reinterpret_cast<const float*>(b.raw_data()),
          reinterpret_cast<float*>(out.raw_mutable_data()),
          n,
          cond_scalar,
          a_scalar,
          b_scalar);
      return finish_kernel_();
    case ScalarType::Float64:
      where_kernel_<<<grid, kBlockSize>>>(
          reinterpret_cast<const std::uint8_t*>(cond.raw_data()),
          reinterpret_cast<const double*>(a.raw_data()),
          reinterpret_cast<const double*>(b.raw_data()),
          reinterpret_cast<double*>(out.raw_mutable_data()),
          n,
          cond_scalar,
          a_scalar,
          b_scalar);
      return finish_kernel_();
    default:
      return false;
  }
}

} // namespace pluss::codem::native::cuda

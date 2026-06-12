#include "CodeM/native/cuda/FactoryOps.h"

#include "CodeM/core/Storage.h"

#include <cuda_runtime_api.h>

#include <cmath>
#include <cstdint>
#include <numeric>

namespace pluss::codem::native::cuda {
namespace {

constexpr int kBlockSize = 256;

template <typename T>
__global__ void fill_kernel_(T* out, double value, std::int64_t n, bool logical_bool) {
  const std::int64_t i = static_cast<std::int64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (i < n) {
    out[i] = logical_bool ? static_cast<T>(value != 0.0 ? 1U : 0U) : static_cast<T>(value);
  }
}

template <typename T>
__global__ void arange_kernel_(T* out, double start, double step, std::int64_t n, bool logical_bool) {
  const std::int64_t i = static_cast<std::int64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (i < n) {
    const double value = start + static_cast<double>(i) * step;
    out[i] = logical_bool ? static_cast<T>(value != 0.0 ? 1U : 0U) : static_cast<T>(value);
  }
}

template <typename T>
__global__ void linspace_kernel_(T* out, double start, double delta, std::int64_t n) {
  const std::int64_t i = static_cast<std::int64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (i < n) {
    out[i] = static_cast<T>(start + static_cast<double>(i) * delta);
  }
}

template <typename T>
__global__ void eye_kernel_(T* out, std::int64_t n, std::int64_t m, std::int64_t diag) {
  (void)n;
  const std::int64_t i = static_cast<std::int64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (i < diag) {
    out[i * m + i] = T(1);
  }
}

__host__ __device__ std::uint64_t splitmix64_(std::uint64_t x) {
  x += 0x9E3779B97F4A7C15ull;
  x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
  x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
  return x ^ (x >> 31);
}

__device__ double uniform01_(std::uint64_t seed, std::uint64_t index, std::uint64_t stream) {
  const std::uint64_t x = splitmix64_(seed ^ (index * 0xD1B54A32D192ED03ull) ^ stream);
  return static_cast<double>(x >> 11) * 0x1.0p-53;
}

template <typename T>
__global__ void uniform_kernel_(T* out, std::uint64_t seed, double lo, double hi, std::int64_t n) {
  const std::int64_t i = static_cast<std::int64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (i < n) {
    const double u = uniform01_(seed, static_cast<std::uint64_t>(i), 0xA5A5A5A5A5A5A5A5ull);
    out[i] = static_cast<T>(lo + (hi - lo) * u);
  }
}

template <typename T>
__global__ void normal_kernel_(T* out, std::uint64_t seed, double mean, double stddev, std::int64_t n) {
  const std::int64_t i = static_cast<std::int64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (i < n) {
    const double u1 = fmax(uniform01_(seed, static_cast<std::uint64_t>(i), 0x243F6A8885A308D3ull), 0x1.0p-53);
    const double u2 = uniform01_(seed, static_cast<std::uint64_t>(i), 0x13198A2E03707344ull);
    constexpr double kTwoPi = 6.283185307179586476925286766559;
    const double z = sqrt(-2.0 * log(u1)) * cos(kTwoPi * u2);
    out[i] = static_cast<T>(mean + stddev * z);
  }
}

template <typename T>
__global__ void randint_kernel_(T* out, std::uint64_t seed, std::int64_t lo, std::int64_t span, std::int64_t n, bool logical_bool) {
  const std::int64_t i = static_cast<std::int64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (i < n) {
    const auto r = static_cast<std::int64_t>(
        splitmix64_(seed ^ (static_cast<std::uint64_t>(i) * 0xD1B54A32D192ED03ull)) %
        static_cast<std::uint64_t>(span));
    const std::int64_t v = lo + r;
    out[i] = logical_bool ? static_cast<T>(v != 0 ? 1U : 0U) : static_cast<T>(v);
  }
}

template <typename T>
__global__ void bernoulli_kernel_(T* out, std::uint64_t seed, double p, std::int64_t n, bool logical_bool) {
  const std::int64_t i = static_cast<std::int64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (i < n) {
    const bool hit = uniform01_(seed, static_cast<std::uint64_t>(i), 0xB7E151628AED2A6Bull) < p;
    out[i] = logical_bool ? static_cast<T>(hit ? 1U : 0U) : static_cast<T>(hit ? 1 : 0);
  }
}

template <typename T>
__global__ void randperm_kernel_(T* out, std::int64_t n, std::int64_t mult, std::int64_t add) {
  const std::int64_t i = static_cast<std::int64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (i < n) {
    out[i] = static_cast<T>((mult * i + add) % n);
  }
}

template <typename T>
struct ComplexPair_ {
  T real;
  T imag;
};

template <typename T>
__global__ void complex_kernel_(const T* real, const T* imag, ComplexPair_<T>* out, std::int64_t n) {
  const std::int64_t i = static_cast<std::int64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (i < n) {
    out[i] = ComplexPair_<T>{real[i], imag[i]};
  }
}

template <typename T>
__global__ void polar_kernel_(const T* mag, const T* angle, ComplexPair_<T>* out, std::int64_t n) {
  const std::int64_t i = static_cast<std::int64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (i < n) {
    const T m = mag[i];
    const T a = angle[i];
    out[i] = ComplexPair_<T>{m * cos(a), m * sin(a)};
  }
}

__global__ void polar_kernel_f32_(const float* mag, const float* angle, ComplexPair_<float>* out, std::int64_t n) {
  const std::int64_t i = static_cast<std::int64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (i < n) {
    const float m = mag[i];
    const float a = angle[i];
    out[i] = ComplexPair_<float>{m * cosf(a), m * sinf(a)};
  }
}

bool valid_output_(const Series& out) noexcept {
  return out.platform() == Platform::CUDA &&
      out.is_contiguous() &&
      out.storage().location() == StorageLocation::CudaDevice;
}

bool valid_input_(const Series& input) noexcept {
  return input.platform() == Platform::CUDA &&
      input.is_contiguous() &&
      input.storage().location() == StorageLocation::CudaDevice;
}

bool same_shape_(const Series& a, const Series& b) noexcept {
  const auto as = a.sizes();
  const auto bs = b.sizes();
  if (as.size() != bs.size()) {
    return false;
  }
  for (std::size_t i = 0; i < as.size(); ++i) {
    if (as[i] != bs[i]) {
      return false;
    }
  }
  return true;
}

bool finish_kernel_() noexcept {
  return cudaPeekAtLastError() == cudaSuccess;
}

int grid_for_(std::int64_t n) noexcept {
  return static_cast<int>((n + kBlockSize - 1) / kBlockSize);
}

template <typename Launcher>
bool dispatch_numeric_(ScalarType dtype, Launcher& launcher) noexcept {
  switch (dtype) {
    case ScalarType::Float32:
      return launcher.template run<float>();
    case ScalarType::Float64:
      return launcher.template run<double>();
    case ScalarType::Int64:
      return launcher.template run<std::int64_t>();
    case ScalarType::Int32:
      return launcher.template run<std::int32_t>();
    case ScalarType::Int16:
      return launcher.template run<std::int16_t>();
    case ScalarType::Int8:
      return launcher.template run<std::int8_t>();
    case ScalarType::UInt64:
      return launcher.template run<std::uint64_t>();
    case ScalarType::UInt32:
      return launcher.template run<std::uint32_t>();
    case ScalarType::UInt16:
      return launcher.template run<std::uint16_t>();
    case ScalarType::UInt8:
    case ScalarType::Bool:
      return launcher.template run<std::uint8_t>();
    default:
      return false;
  }
}

struct FillLauncher {
  Series& out;
  double value;
  bool logical_bool;

  template <typename T>
  bool run() noexcept {
    fill_kernel_<<<grid_for_(out.numel()), kBlockSize>>>(
        reinterpret_cast<T*>(out.raw_mutable_data()),
        value,
        out.numel(),
        logical_bool);
    return finish_kernel_();
  }
};

struct ArangeLauncher {
  Series& out;
  double start;
  double step;
  bool logical_bool;

  template <typename T>
  bool run() noexcept {
    arange_kernel_<<<grid_for_(out.numel()), kBlockSize>>>(
        reinterpret_cast<T*>(out.raw_mutable_data()),
        start,
        step,
        out.numel(),
        logical_bool);
    return finish_kernel_();
  }
};

struct LinspaceLauncher {
  Series& out;
  double start;
  double delta;

  template <typename T>
  bool run() noexcept {
    linspace_kernel_<<<grid_for_(out.numel()), kBlockSize>>>(
        reinterpret_cast<T*>(out.raw_mutable_data()),
        start,
        delta,
        out.numel());
    return finish_kernel_();
  }
};

struct EyeLauncher {
  Series& out;
  std::int64_t n;
  std::int64_t m;
  std::int64_t diag;

  template <typename T>
  bool run() noexcept {
    eye_kernel_<<<grid_for_(diag), kBlockSize>>>(
        reinterpret_cast<T*>(out.raw_mutable_data()),
        n,
        m,
        diag);
    return finish_kernel_();
  }
};

struct UniformLauncher {
  Series& out;
  std::uint64_t seed;
  double lo;
  double hi;

  template <typename T>
  bool run() noexcept {
    uniform_kernel_<<<grid_for_(out.numel()), kBlockSize>>>(
        reinterpret_cast<T*>(out.raw_mutable_data()),
        seed,
        lo,
        hi,
        out.numel());
    return finish_kernel_();
  }
};

struct NormalLauncher {
  Series& out;
  std::uint64_t seed;
  double mean;
  double stddev;

  template <typename T>
  bool run() noexcept {
    normal_kernel_<<<grid_for_(out.numel()), kBlockSize>>>(
        reinterpret_cast<T*>(out.raw_mutable_data()),
        seed,
        mean,
        stddev,
        out.numel());
    return finish_kernel_();
  }
};

struct RandintLauncher {
  Series& out;
  std::uint64_t seed;
  std::int64_t lo;
  std::int64_t span;
  bool logical_bool;

  template <typename T>
  bool run() noexcept {
    randint_kernel_<<<grid_for_(out.numel()), kBlockSize>>>(
        reinterpret_cast<T*>(out.raw_mutable_data()),
        seed,
        lo,
        span,
        out.numel(),
        logical_bool);
    return finish_kernel_();
  }
};

struct BernoulliLauncher {
  Series& out;
  std::uint64_t seed;
  double p;
  bool logical_bool;

  template <typename T>
  bool run() noexcept {
    bernoulli_kernel_<<<grid_for_(out.numel()), kBlockSize>>>(
        reinterpret_cast<T*>(out.raw_mutable_data()),
        seed,
        p,
        out.numel(),
        logical_bool);
    return finish_kernel_();
  }
};

struct RandpermLauncher {
  Series& out;
  std::int64_t mult;
  std::int64_t add;

  template <typename T>
  bool run() noexcept {
    randperm_kernel_<<<grid_for_(out.numel()), kBlockSize>>>(
        reinterpret_cast<T*>(out.raw_mutable_data()),
        out.numel(),
        mult,
        add);
    return finish_kernel_();
  }
};

bool valid_complex_args_(const Series& real, const Series& imag, const Series& out) noexcept {
  if (!valid_input_(real) || !valid_input_(imag) || !valid_output_(out) ||
      !same_shape_(real, imag) || !same_shape_(real, out)) {
    return false;
  }
  if (out.scalar_type() == ScalarType::Complex64) {
    return real.scalar_type() == ScalarType::Float32 && imag.scalar_type() == ScalarType::Float32;
  }
  if (out.scalar_type() == ScalarType::Complex128) {
    return real.scalar_type() == ScalarType::Float64 && imag.scalar_type() == ScalarType::Float64;
  }
  return false;
}

} // namespace

bool run_fill_factory(Series& out, double value) noexcept {
  if (!valid_output_(out)) {
    return false;
  }
  if (out.numel() == 0) {
    return true;
  }
  FillLauncher launcher{out, value, out.scalar_type() == ScalarType::Bool};
  return dispatch_numeric_(out.scalar_type(), launcher);
}

bool run_arange_factory(Series& out, double start, double step) noexcept {
  if (!valid_output_(out)) {
    return false;
  }
  if (out.numel() == 0) {
    return true;
  }
  ArangeLauncher launcher{out, start, step, out.scalar_type() == ScalarType::Bool};
  return dispatch_numeric_(out.scalar_type(), launcher);
}

bool run_linspace_factory(Series& out, double start, double end) noexcept {
  if (!valid_output_(out)) {
    return false;
  }
  if (out.numel() == 0) {
    return true;
  }
  if (out.scalar_type() != ScalarType::Float32 && out.scalar_type() != ScalarType::Float64) {
    return false;
  }
  const double delta = (out.numel() == 1) ? 0.0 : ((end - start) / static_cast<double>(out.numel() - 1));
  LinspaceLauncher launcher{out, start, delta};
  switch (out.scalar_type()) {
    case ScalarType::Float32:
      return launcher.template run<float>();
    case ScalarType::Float64:
      return launcher.template run<double>();
    default:
      return false;
  }
}

bool run_eye_factory(Series& out, std::int64_t n, std::int64_t m) noexcept {
  if (!valid_output_(out) || n < 0 || m < 0 || out.dim() != 2 || out.size(0) != n || out.size(1) != m) {
    return false;
  }
  const std::int64_t diag = (n < m) ? n : m;
  if (diag == 0) {
    return true;
  }
  EyeLauncher launcher{out, n, m, diag};
  return dispatch_numeric_(out.scalar_type(), launcher);
}

bool run_uniform_factory(Series& out, std::uint64_t seed, double lo, double hi) noexcept {
  if (!valid_output_(out)) {
    return false;
  }
  if (out.numel() == 0) {
    return true;
  }
  UniformLauncher launcher{out, seed, lo, hi};
  switch (out.scalar_type()) {
    case ScalarType::Float32:
      return launcher.template run<float>();
    case ScalarType::Float64:
      return launcher.template run<double>();
    default:
      return false;
  }
}

bool run_normal_factory(Series& out, std::uint64_t seed, double mean, double stddev) noexcept {
  if (!valid_output_(out)) {
    return false;
  }
  if (out.numel() == 0) {
    return true;
  }
  NormalLauncher launcher{out, seed, mean, stddev};
  switch (out.scalar_type()) {
    case ScalarType::Float32:
      return launcher.template run<float>();
    case ScalarType::Float64:
      return launcher.template run<double>();
    default:
      return false;
  }
}

bool run_randint_factory(Series& out, std::uint64_t seed, std::int64_t lo, std::int64_t hi) noexcept {
  if (!valid_output_(out) || hi <= lo) {
    return false;
  }
  if (out.numel() == 0) {
    return true;
  }
  RandintLauncher launcher{out, seed, lo, hi - lo, out.scalar_type() == ScalarType::Bool};
  return dispatch_numeric_(out.scalar_type(), launcher);
}

bool run_bernoulli_factory(Series& out, std::uint64_t seed, double p) noexcept {
  if (!valid_output_(out) || !(p >= 0.0 && p <= 1.0)) {
    return false;
  }
  if (out.numel() == 0) {
    return true;
  }
  BernoulliLauncher launcher{out, seed, p, out.scalar_type() == ScalarType::Bool};
  return dispatch_numeric_(out.scalar_type(), launcher);
}

bool run_randperm_factory(Series& out, std::uint64_t seed) noexcept {
  if (!valid_output_(out) || out.dim() != 1) {
    return false;
  }
  const std::int64_t n = out.numel();
  if (n == 0) {
    return true;
  }
  std::int64_t mult = static_cast<std::int64_t>((seed | 1ull) % static_cast<std::uint64_t>(n));
  if (mult == 0) {
    mult = 1;
  }
  while (std::gcd(mult, n) != 1) {
    mult += 2;
    if (mult >= n) {
      mult = 1;
    }
  }
  const std::int64_t add = static_cast<std::int64_t>(
      splitmix64_(seed ^ 0xC6EF372FE94F82BEull) % static_cast<std::uint64_t>(n));
  RandpermLauncher launcher{out, mult, add};
  return dispatch_numeric_(out.scalar_type(), launcher);
}

bool run_complex_factory(const Series& real, const Series& imag, Series& out) noexcept {
  if (!valid_complex_args_(real, imag, out)) {
    return false;
  }
  if (out.numel() == 0) {
    return true;
  }
  if (out.scalar_type() == ScalarType::Complex64) {
    complex_kernel_<<<grid_for_(out.numel()), kBlockSize>>>(
        reinterpret_cast<const float*>(real.raw_data()),
        reinterpret_cast<const float*>(imag.raw_data()),
        reinterpret_cast<ComplexPair_<float>*>(out.raw_mutable_data()),
        out.numel());
    return finish_kernel_();
  }
  if (out.scalar_type() == ScalarType::Complex128) {
    complex_kernel_<<<grid_for_(out.numel()), kBlockSize>>>(
        reinterpret_cast<const double*>(real.raw_data()),
        reinterpret_cast<const double*>(imag.raw_data()),
        reinterpret_cast<ComplexPair_<double>*>(out.raw_mutable_data()),
        out.numel());
    return finish_kernel_();
  }
  return false;
}

bool run_polar_factory(const Series& abs, const Series& angle, Series& out) noexcept {
  if (!valid_complex_args_(abs, angle, out)) {
    return false;
  }
  if (out.numel() == 0) {
    return true;
  }
  if (out.scalar_type() == ScalarType::Complex64) {
    polar_kernel_f32_<<<grid_for_(out.numel()), kBlockSize>>>(
        reinterpret_cast<const float*>(abs.raw_data()),
        reinterpret_cast<const float*>(angle.raw_data()),
        reinterpret_cast<ComplexPair_<float>*>(out.raw_mutable_data()),
        out.numel());
    return finish_kernel_();
  }
  if (out.scalar_type() == ScalarType::Complex128) {
    polar_kernel_<<<grid_for_(out.numel()), kBlockSize>>>(
        reinterpret_cast<const double*>(abs.raw_data()),
        reinterpret_cast<const double*>(angle.raw_data()),
        reinterpret_cast<ComplexPair_<double>*>(out.raw_mutable_data()),
        out.numel());
    return finish_kernel_();
  }
  return false;
}

} // namespace pluss::codem::native::cuda

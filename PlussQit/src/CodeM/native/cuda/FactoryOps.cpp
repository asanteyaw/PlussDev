#include "CodeM/native/cuda/FactoryOps.h"

#include "CodeM/native/cuda/BaselineOps.h"
#include "CodeM/native/cuda/CUDARuntime.h"

#include <cctype>
#include <cstdlib>

namespace pluss::codem::native::cuda {
namespace {

bool ieq_(const char* a, const char* b) noexcept {
  if (a == nullptr || b == nullptr) return false;
  while (*a != '\0' && *b != '\0') {
    if (std::tolower(static_cast<unsigned char>(*a)) != std::tolower(static_cast<unsigned char>(*b))) {
      return false;
    }
    ++a;
    ++b;
  }
  return *a == '\0' && *b == '\0';
}

bool parse_bool_env_(const char* name, bool default_value) noexcept {
  const char* raw = std::getenv(name);
  if (raw == nullptr || *raw == '\0') {
    return default_value;
  }
  if (ieq_(raw, "1") || ieq_(raw, "true") || ieq_(raw, "yes") || ieq_(raw, "on")) {
    return true;
  }
  if (ieq_(raw, "0") || ieq_(raw, "false") || ieq_(raw, "no") || ieq_(raw, "off")) {
    return false;
  }
  return default_value;
}

bool supports_zeros_dtype_(ScalarType dtype) noexcept {
  switch (dtype) {
    case ScalarType::Bool:
    case ScalarType::UInt8:
    case ScalarType::UInt16:
    case ScalarType::UInt32:
    case ScalarType::UInt64:
    case ScalarType::Int8:
    case ScalarType::Int16:
    case ScalarType::Int32:
    case ScalarType::Int64:
    case ScalarType::Float16:
    case ScalarType::BFloat16:
    case ScalarType::Float32:
    case ScalarType::Float64:
    case ScalarType::Float8E4M3FN:
    case ScalarType::Float8E5M2:
    case ScalarType::Complex64:
    case ScalarType::Complex128:
      return true;
    default:
      return false;
  }
}

bool supports_ones_dtype_(ScalarType dtype) noexcept {
  switch (dtype) {
    case ScalarType::Float64:
    case ScalarType::Float32:
    case ScalarType::Int64:
    case ScalarType::Int32:
    case ScalarType::Int16:
    case ScalarType::Int8:
    case ScalarType::UInt64:
    case ScalarType::UInt32:
    case ScalarType::UInt16:
    case ScalarType::UInt8:
    case ScalarType::Bool:
      return true;
    default:
      return false;
  }
}

bool supports_full_dtype_(ScalarType dtype) noexcept {
  switch (dtype) {
    case ScalarType::Float64:
    case ScalarType::Float32:
    case ScalarType::Int64:
    case ScalarType::Int32:
    case ScalarType::Int16:
    case ScalarType::Int8:
    case ScalarType::UInt64:
    case ScalarType::UInt32:
    case ScalarType::UInt16:
    case ScalarType::UInt8:
    case ScalarType::Bool:
      return true;
    default:
      return false;
  }
}

bool supports_arange_dtype_(ScalarType dtype) noexcept {
  switch (dtype) {
    case ScalarType::Float64:
    case ScalarType::Float32:
    case ScalarType::Int64:
    case ScalarType::Int32:
    case ScalarType::Int16:
    case ScalarType::Int8:
    case ScalarType::UInt64:
    case ScalarType::UInt32:
    case ScalarType::UInt16:
    case ScalarType::UInt8:
    case ScalarType::Bool:
      return true;
    default:
      return false;
  }
}

bool supports_linspace_dtype_(ScalarType dtype) noexcept {
  switch (dtype) {
    case ScalarType::Float64:
    case ScalarType::Float32:
      return true;
    default:
      return false;
  }
}

bool supports_eye_dtype_(ScalarType dtype) noexcept {
  switch (dtype) {
    case ScalarType::Float64:
    case ScalarType::Float32:
    case ScalarType::Int64:
    case ScalarType::Int32:
    case ScalarType::Int16:
    case ScalarType::Int8:
    case ScalarType::UInt64:
    case ScalarType::UInt32:
    case ScalarType::UInt16:
    case ScalarType::UInt8:
    case ScalarType::Bool:
      return true;
    default:
      return false;
  }
}

bool supports_rand_dtype_(ScalarType dtype) noexcept {
  return dtype == ScalarType::Float32 || dtype == ScalarType::Float64;
}

bool supports_factory_common_(ScalarType dtype, bool (*dtype_support)(ScalarType) noexcept) noexcept {
  return runtime_available() &&
      native_baseline_enabled() &&
      native_factory_enabled() &&
      dtype_support(dtype);
}

} // namespace

bool native_factory_enabled() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_FACTORY", false);
}

bool native_factory_strict() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_FACTORY_STRICT", false);
}

bool supports_zeros_factory_baseline(ScalarType dtype) noexcept {
  return supports_factory_common_(dtype, supports_zeros_dtype_);
}

bool supports_ones_factory_baseline(ScalarType dtype) noexcept {
  return supports_factory_common_(dtype, supports_ones_dtype_);
}

bool supports_full_factory_baseline(ScalarType dtype) noexcept {
  return supports_factory_common_(dtype, supports_full_dtype_);
}

bool supports_arange_factory_baseline(ScalarType dtype) noexcept {
  return supports_factory_common_(dtype, supports_arange_dtype_);
}

bool supports_linspace_factory_baseline(ScalarType dtype) noexcept {
  return supports_factory_common_(dtype, supports_linspace_dtype_);
}

bool supports_eye_factory_baseline(ScalarType dtype) noexcept {
  return supports_factory_common_(dtype, supports_eye_dtype_);
}

bool supports_rand_factory_baseline(ScalarType dtype) noexcept {
  return supports_factory_common_(dtype, supports_rand_dtype_);
}

bool supports_randn_factory_baseline(ScalarType dtype) noexcept {
  return supports_factory_common_(dtype, supports_rand_dtype_);
}

#if !(defined(PLUSS_HAS_CUDA) && PLUSS_HAS_CUDA)
bool run_fill_factory(Series& out, double value) noexcept {
  (void)out;
  (void)value;
  return false;
}

bool run_arange_factory(Series& out, double start, double step) noexcept {
  (void)out;
  (void)start;
  (void)step;
  return false;
}

bool run_linspace_factory(Series& out, double start, double end) noexcept {
  (void)out;
  (void)start;
  (void)end;
  return false;
}

bool run_eye_factory(Series& out, std::int64_t n, std::int64_t m) noexcept {
  (void)out;
  (void)n;
  (void)m;
  return false;
}

bool run_uniform_factory(Series& out, std::uint64_t seed, double lo, double hi) noexcept {
  (void)out;
  (void)seed;
  (void)lo;
  (void)hi;
  return false;
}

bool run_normal_factory(Series& out, std::uint64_t seed, double mean, double stddev) noexcept {
  (void)out;
  (void)seed;
  (void)mean;
  (void)stddev;
  return false;
}

bool run_randint_factory(Series& out, std::uint64_t seed, std::int64_t lo, std::int64_t hi) noexcept {
  (void)out;
  (void)seed;
  (void)lo;
  (void)hi;
  return false;
}

bool run_bernoulli_factory(Series& out, std::uint64_t seed, double p) noexcept {
  (void)out;
  (void)seed;
  (void)p;
  return false;
}

bool run_randperm_factory(Series& out, std::uint64_t seed) noexcept {
  (void)out;
  (void)seed;
  return false;
}

bool run_complex_factory(const Series& real, const Series& imag, Series& out) noexcept {
  (void)real;
  (void)imag;
  (void)out;
  return false;
}

bool run_polar_factory(const Series& abs, const Series& angle, Series& out) noexcept {
  (void)abs;
  (void)angle;
  (void)out;
  return false;
}
#endif

} // namespace pluss::codem::native::cuda

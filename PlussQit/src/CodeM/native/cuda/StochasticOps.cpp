#include "CodeM/native/cuda/StochasticOps.h"

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

bool supports_non_complex_numeric_dtype_(ScalarType dtype) noexcept {
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
    case ScalarType::Float64:
    case ScalarType::Float32:
    case ScalarType::Float16:
    case ScalarType::BFloat16:
    case ScalarType::Float8E4M3FN:
    case ScalarType::Float8E5M2:
      return true;
    default:
      return false;
  }
}

bool supports_stochastic_common_(ScalarType dtype) noexcept {
  return runtime_available() &&
      native_baseline_enabled() &&
      native_stochastic_enabled() &&
      supports_non_complex_numeric_dtype_(dtype);
}

} // namespace

bool native_stochastic_enabled() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_STOCHASTIC", false);
}

bool native_stochastic_strict() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_STOCHASTIC_STRICT", false);
}

bool supports_bernoulli_stochastic_baseline(ScalarType dtype) noexcept {
  return supports_stochastic_common_(dtype);
}

bool supports_normal_stochastic_baseline(ScalarType dtype) noexcept {
  return supports_stochastic_common_(dtype);
}

bool supports_log_normal_stochastic_baseline(ScalarType dtype) noexcept {
  return supports_stochastic_common_(dtype);
}

bool supports_uniform_stochastic_baseline(ScalarType dtype) noexcept {
  return supports_stochastic_common_(dtype);
}

bool supports_exponential_stochastic_baseline(ScalarType dtype) noexcept {
  return supports_stochastic_common_(dtype);
}

bool supports_geometric_stochastic_baseline(ScalarType dtype) noexcept {
  return supports_stochastic_common_(dtype);
}

bool supports_cauchy_stochastic_baseline(ScalarType dtype) noexcept {
  return supports_stochastic_common_(dtype);
}

bool supports_random_stochastic_baseline(ScalarType dtype) noexcept {
  return supports_stochastic_common_(dtype);
}

bool supports_multinomial_stochastic_baseline(ScalarType dtype) noexcept {
  return supports_stochastic_common_(dtype);
}

bool supports_bernoulli_stochastic(const Series& a) noexcept {
  return a.platform() == Platform::CUDA &&
      supports_bernoulli_stochastic_baseline(a.scalar_type());
}

bool supports_normal_stochastic(const Series& a) noexcept {
  return a.platform() == Platform::CUDA &&
      supports_normal_stochastic_baseline(a.scalar_type());
}

bool supports_log_normal_stochastic(const Series& a) noexcept {
  return a.platform() == Platform::CUDA &&
      supports_log_normal_stochastic_baseline(a.scalar_type());
}

bool supports_uniform_stochastic(const Series& a) noexcept {
  return a.platform() == Platform::CUDA &&
      supports_uniform_stochastic_baseline(a.scalar_type());
}

bool supports_exponential_stochastic(const Series& a) noexcept {
  return a.platform() == Platform::CUDA &&
      supports_exponential_stochastic_baseline(a.scalar_type());
}

bool supports_geometric_stochastic(const Series& a) noexcept {
  return a.platform() == Platform::CUDA &&
      supports_geometric_stochastic_baseline(a.scalar_type());
}

bool supports_cauchy_stochastic(const Series& a) noexcept {
  return a.platform() == Platform::CUDA &&
      supports_cauchy_stochastic_baseline(a.scalar_type());
}

bool supports_random_stochastic(const Series& a) noexcept {
  return a.platform() == Platform::CUDA &&
      supports_random_stochastic_baseline(a.scalar_type());
}

bool supports_multinomial_stochastic(const Series& a, std::int64_t num_samples, bool replacement) noexcept {
  if (a.platform() != Platform::CUDA ||
      !supports_multinomial_stochastic_baseline(a.scalar_type())) {
    return false;
  }
  if (a.numel() <= 0 || num_samples < 0) {
    return false;
  }
  if (!replacement && num_samples > a.numel()) {
    return false;
  }
  return true;
}

} // namespace pluss::codem::native::cuda


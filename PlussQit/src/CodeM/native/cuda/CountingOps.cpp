#include "CodeM/native/cuda/CountingOps.h"

#include "CodeM/native/cuda/BaselineOps.h"
#include "CodeM/native/cuda/CUDARuntime.h"

#include <cctype>
#include <cstdlib>

namespace pluss::codem::native::cuda {
namespace {

bool ieq_(const char* a, const char* b) noexcept {
  if (a == nullptr || b == nullptr) return false;
  while (*a != '\0' && *b != '\0') {
    if (std::tolower(static_cast<unsigned char>(*a)) !=
        std::tolower(static_cast<unsigned char>(*b))) {
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

bool baseline_ready_() noexcept {
  return runtime_available() && native_baseline_enabled() && native_counting_enabled();
}

bool is_cuda_series_(const Series& s) noexcept {
  return s.platform() == Platform::CUDA;
}

bool supports_unique_dtype_(ScalarType dtype) noexcept {
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
    case ScalarType::Float32:
    case ScalarType::Float64:
      return true;
    default:
      return false;
  }
}

bool supports_bincount_dtype_(ScalarType dtype) noexcept {
  switch (dtype) {
    case ScalarType::Int8:
    case ScalarType::Int16:
    case ScalarType::Int32:
    case ScalarType::Int64:
      return true;
    default:
      return false;
  }
}

bool supports_histc_dtype_(ScalarType dtype) noexcept {
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
      return true;
    default:
      return false;
  }
}

} // namespace

bool native_counting_enabled() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_COUNTING", false);
}

bool native_counting_strict() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_COUNTING_STRICT", false);
}

bool supports_unique_counting_baseline(ScalarType dtype, bool /*sorted*/) noexcept {
  return baseline_ready_() && supports_unique_dtype_(dtype);
}

bool supports_bincount_counting_baseline(ScalarType dtype) noexcept {
  return baseline_ready_() && supports_bincount_dtype_(dtype);
}

bool supports_histc_counting_baseline(ScalarType dtype) noexcept {
  return baseline_ready_() && supports_histc_dtype_(dtype);
}

bool supports_unique_counting(const Series& a, bool sorted) noexcept {
  if (!is_cuda_series_(a)) return false;
  if (!(a.dim() == 1 || a.numel() == 0)) return false;
  return supports_unique_counting_baseline(a.scalar_type(), sorted);
}

bool supports_bincount_counting(const Series& a, std::int64_t minlength) noexcept {
  if (!is_cuda_series_(a)) return false;
  if (!(a.dim() == 1 || a.numel() == 0)) return false;
  if (minlength < 0) return false;
  return supports_bincount_counting_baseline(a.scalar_type());
}

bool supports_histc_counting(const Series& a, std::int64_t bins) noexcept {
  if (!is_cuda_series_(a)) return false;
  if (bins <= 0) return false;
  return supports_histc_counting_baseline(a.scalar_type());
}

} // namespace pluss::codem::native::cuda

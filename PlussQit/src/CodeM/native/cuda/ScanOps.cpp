#include "CodeM/native/cuda/ScanOps.h"

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
  return runtime_available() && native_baseline_enabled() && native_scan_enabled();
}

bool is_cuda_series_(const Series& s) noexcept {
  return s.platform() == Platform::CUDA;
}

std::int64_t normalize_dim_(std::int64_t dim, std::int64_t rank) noexcept {
  if (rank <= 0) {
    return -1;
  }
  if (dim < -rank || dim >= rank) {
    return -1;
  }
  return dim < 0 ? dim + rank : dim;
}

bool supports_scan_dtype_(ScalarType dtype) noexcept {
  switch (dtype) {
    case ScalarType::Float64:
    case ScalarType::Float32:
    case ScalarType::Float16:
    case ScalarType::BFloat16:
    case ScalarType::Int64:
    case ScalarType::Int32:
    case ScalarType::Int16:
    case ScalarType::Int8:
    case ScalarType::UInt64:
    case ScalarType::UInt32:
    case ScalarType::UInt16:
    case ScalarType::UInt8:
      return true;
    default:
      return false;
  }
}

bool supports_scan_common_(const Series& a, std::int64_t dim) noexcept {
  if (!is_cuda_series_(a)) {
    return false;
  }
  if (!supports_scan_dtype_(a.scalar_type())) {
    return false;
  }
  if (a.dim() == 0) {
    return true;
  }
  return normalize_dim_(dim, a.dim()) >= 0;
}

} // namespace

bool native_scan_enabled() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_SCAN", false);
}

bool native_scan_strict() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_SCAN_STRICT", false);
}

bool supports_cumsum_scan_baseline(ScalarType dtype) noexcept {
  return baseline_ready_() && supports_scan_dtype_(dtype);
}

bool supports_cumprod_scan_baseline(ScalarType dtype) noexcept {
  return baseline_ready_() && supports_scan_dtype_(dtype);
}

bool supports_cumsum_scan(const Series& a, std::int64_t dim) noexcept {
  if (!baseline_ready_()) {
    return false;
  }
  return supports_scan_common_(a, dim);
}

bool supports_cumprod_scan(const Series& a, std::int64_t dim) noexcept {
  if (!baseline_ready_()) {
    return false;
  }
  return supports_scan_common_(a, dim);
}

} // namespace pluss::codem::native::cuda


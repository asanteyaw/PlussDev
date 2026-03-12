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

bool supports_baseline_dtype_(ScalarType dtype) noexcept {
  switch (dtype) {
    case ScalarType::Float32:
    case ScalarType::Float16:
    case ScalarType::BFloat16:
      return true;
    default:
      return false;
  }
}

} // namespace

bool native_baseline_enabled() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_BASELINE", false);
}

bool supports_elementwise_lane(ScalarType dtype) noexcept {
  return runtime_available() && native_baseline_enabled() && supports_baseline_dtype_(dtype);
}

bool supports_reduction_lane(ScalarType dtype) noexcept {
  return runtime_available() && native_baseline_enabled() && supports_baseline_dtype_(dtype);
}

bool supports_matmul_lane(ScalarType lhs_dtype, ScalarType rhs_dtype) noexcept {
  if (!runtime_available() || !native_baseline_enabled()) {
    return false;
  }
  // Baseline tranche starts with homogeneous floating lanes.
  return lhs_dtype == rhs_dtype && supports_baseline_dtype_(lhs_dtype);
}

} // namespace pluss::codem::native::cuda

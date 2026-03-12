#include "CodeM/native/cuda/MatmulOps.h"

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

} // namespace

bool native_matmul_enabled() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_MATMUL", false);
}

bool native_matmul_strict() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_MATMUL_STRICT", false);
}

bool supports_matmul_baseline(ScalarType lhs_dtype, ScalarType rhs_dtype) noexcept {
  if (!runtime_available() || !native_baseline_enabled() || !native_matmul_enabled()) {
    return false;
  }
  return supports_matmul_lane(lhs_dtype, rhs_dtype);
}

bool supports_matmul(const Series& a, const Series& b) noexcept {
  if (!supports_matmul_baseline(a.scalar_type(), b.scalar_type())) {
    return false;
  }
  if (a.platform() != Platform::CUDA || b.platform() != Platform::CUDA) {
    return false;
  }
  if (!a.is_contiguous() || !b.is_contiguous()) {
    return false;
  }
  if (a.dim() != 2 || b.dim() != 2) {
    return false;
  }
  return a.sizes()[1] == b.sizes()[0];
}

} // namespace pluss::codem::native::cuda

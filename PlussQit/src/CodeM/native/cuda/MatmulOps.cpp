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

bool matmul_batch_broadcastable_(const Series& a, const Series& b) noexcept {
  const std::int64_t da = a.dim();
  const std::int64_t db = b.dim();
  const std::int64_t ba = da > 2 ? da - 2 : 0;
  const std::int64_t bb = db > 2 ? db - 2 : 0;
  const std::int64_t rank = ba > bb ? ba : bb;
  for (std::int64_t i = 0; i < rank; ++i) {
    const std::int64_t ai = (da - 3) - i;
    const std::int64_t bi = (db - 3) - i;
    const std::int64_t sa = ai >= 0 ? a.sizes()[static_cast<std::size_t>(ai)] : 1;
    const std::int64_t sb = bi >= 0 ? b.sizes()[static_cast<std::size_t>(bi)] : 1;
    if (sa != sb && sa != 1 && sb != 1) {
      return false;
    }
  }
  return true;
}

bool matmul_core_shapes_compatible_(const Series& a, const Series& b) noexcept {
  if (a.dim() < 1 || b.dim() < 1) {
    return false;
  }
  const std::int64_t k_left = a.sizes().back();
  const std::int64_t k_right = (b.dim() == 1)
      ? b.sizes().front()
      : b.sizes()[static_cast<std::size_t>(b.dim() - 2)];
  if (k_left != k_right) {
    return false;
  }
  return matmul_batch_broadcastable_(a, b);
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
  return matmul_core_shapes_compatible_(a, b);
}

} // namespace pluss::codem::native::cuda

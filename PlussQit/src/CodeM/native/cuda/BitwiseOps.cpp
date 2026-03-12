#include "CodeM/native/cuda/BitwiseOps.h"

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
  return runtime_available() && native_baseline_enabled() && native_bitwise_enabled();
}

bool is_cuda_series_(const Series& s) noexcept {
  return s.platform() == Platform::CUDA;
}

bool is_integral_or_bool_(ScalarType dtype) noexcept {
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
      return true;
    default:
      return false;
  }
}

} // namespace

bool native_bitwise_enabled() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_BITWISE", false);
}

bool native_bitwise_strict() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_BITWISE_STRICT", false);
}

bool supports_bitwise_unary_baseline(ScalarType dtype) noexcept {
  return baseline_ready_() && is_integral_or_bool_(dtype);
}

bool supports_bitwise_binary_baseline(ScalarType lhs_dtype, ScalarType rhs_dtype) noexcept {
  if (!baseline_ready_()) {
    return false;
  }
  return lhs_dtype == rhs_dtype && is_integral_or_bool_(lhs_dtype);
}

bool supports_bitwise_unary(const Series& a) noexcept {
  return is_cuda_series_(a) && supports_bitwise_unary_baseline(a.scalar_type());
}

bool supports_bitwise_binary(const Series& a, const Series& b) noexcept {
  if (!is_cuda_series_(a) || !is_cuda_series_(b)) {
    return false;
  }
  return supports_bitwise_binary_baseline(a.scalar_type(), b.scalar_type());
}

} // namespace pluss::codem::native::cuda


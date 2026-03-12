#include "CodeM/native/cuda/ElementwiseOps.h"

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

bool baseline_ready_() noexcept {
  return runtime_available() && native_baseline_enabled() && native_elementwise_enabled();
}

bool is_cuda_series_(const Series& s) noexcept {
  return s.platform() == Platform::CUDA;
}

bool supports_numeric_dtype_(ScalarType dtype) noexcept {
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
    case ScalarType::Complex64:
    case ScalarType::Complex128:
      return true;
    default:
      return false;
  }
}

bool supports_compare_dtype_(ScalarType dtype) noexcept {
  switch (dtype) {
    case ScalarType::Bool:
      return true;
    default:
      return supports_numeric_dtype_(dtype);
  }
}

} // namespace

bool native_elementwise_enabled() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_ELEMENTWISE", false);
}

bool native_elementwise_strict() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_ELEMENTWISE_STRICT", false);
}

bool supports_unary_elementwise_baseline(ScalarType dtype) noexcept {
  return baseline_ready_() && supports_numeric_dtype_(dtype);
}

bool supports_binary_elementwise_baseline(ScalarType lhs_dtype, ScalarType rhs_dtype) noexcept {
  if (!baseline_ready_()) {
    return false;
  }
  return lhs_dtype == rhs_dtype && supports_numeric_dtype_(lhs_dtype);
}

bool supports_unary_elementwise(const Series& a) noexcept {
  return is_cuda_series_(a) && supports_unary_elementwise_baseline(a.scalar_type());
}

bool supports_binary_elementwise(const Series& a, const Series& b) noexcept {
  if (!is_cuda_series_(a) || !is_cuda_series_(b)) {
    return false;
  }
  return supports_binary_elementwise_baseline(a.scalar_type(), b.scalar_type());
}

bool supports_comparison_elementwise(const Series& a, const Series& b) noexcept {
  if (!is_cuda_series_(a) || !is_cuda_series_(b)) {
    return false;
  }
  if (!baseline_ready_()) {
    return false;
  }
  return a.scalar_type() == b.scalar_type() && supports_compare_dtype_(a.scalar_type());
}

} // namespace pluss::codem::native::cuda

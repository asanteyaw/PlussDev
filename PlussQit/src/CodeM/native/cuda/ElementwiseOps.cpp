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
  return is_cuda_series_(a) &&
      a.is_contiguous() &&
      a.storage().location() == StorageLocation::CudaDevice &&
      supports_unary_elementwise_baseline(a.scalar_type());
}

bool supports_binary_elementwise(const Series& a, const Series& b) noexcept {
  if (!is_cuda_series_(a) || !is_cuda_series_(b)) {
    return false;
  }
  return a.is_contiguous() &&
      b.is_contiguous() &&
      a.storage().location() == StorageLocation::CudaDevice &&
      b.storage().location() == StorageLocation::CudaDevice &&
      (a.numel() == b.numel() || a.numel() == 1 || b.numel() == 1) &&
      supports_binary_elementwise_baseline(a.scalar_type(), b.scalar_type());
}

bool supports_comparison_elementwise(const Series& a, const Series& b) noexcept {
  if (!is_cuda_series_(a) || !is_cuda_series_(b)) {
    return false;
  }
  if (!baseline_ready_()) {
    return false;
  }
  return a.is_contiguous() &&
      b.is_contiguous() &&
      a.storage().location() == StorageLocation::CudaDevice &&
      b.storage().location() == StorageLocation::CudaDevice &&
      (a.numel() == b.numel() || a.numel() == 1 || b.numel() == 1) &&
      a.scalar_type() == b.scalar_type() &&
      supports_compare_dtype_(a.scalar_type());
}

bool supports_where_elementwise(const Series& cond, const Series& a, const Series& b) noexcept {
  if (!is_cuda_series_(cond) || !is_cuda_series_(a) || !is_cuda_series_(b) || !baseline_ready_()) {
    return false;
  }
  const auto n = std::max(cond.numel(), std::max(a.numel(), b.numel()));
  return cond.scalar_type() == ScalarType::Bool &&
      a.scalar_type() == b.scalar_type() &&
      supports_numeric_dtype_(a.scalar_type()) &&
      cond.is_contiguous() &&
      a.is_contiguous() &&
      b.is_contiguous() &&
      cond.storage().location() == StorageLocation::CudaDevice &&
      a.storage().location() == StorageLocation::CudaDevice &&
      b.storage().location() == StorageLocation::CudaDevice &&
      (cond.numel() == n || cond.numel() == 1) &&
      (a.numel() == n || a.numel() == 1) &&
      (b.numel() == n || b.numel() == 1);
}

#if !(defined(PLUSS_HAS_CUDA) && PLUSS_HAS_CUDA)
bool run_unary_elementwise(native::cpu::UnaryOpId, const Series&, Series&) noexcept {
  return false;
}

bool run_binary_elementwise(native::cpu::BinaryOpId, const Series&, const Series&, Series&) noexcept {
  return false;
}

bool run_where_elementwise(const Series&, const Series&, const Series&, Series&) noexcept {
  return false;
}
#endif

} // namespace pluss::codem::native::cuda

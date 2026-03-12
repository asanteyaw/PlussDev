#include "CodeM/native/cuda/MaskingOps.h"

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
  return runtime_available() && native_baseline_enabled() && native_masking_enabled();
}

bool is_cuda_series_(const Series& s) noexcept {
  return s.platform() == Platform::CUDA;
}

bool supports_value_dtype_(ScalarType dtype) noexcept {
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
    case ScalarType::Complex64:
    case ScalarType::Complex128:
      return true;
    default:
      return false;
  }
}

bool supports_mask_dtype_(ScalarType dtype) noexcept {
  return dtype == ScalarType::Bool;
}

} // namespace

bool native_masking_enabled() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_MASKING", false);
}

bool native_masking_strict() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_MASKING_STRICT", false);
}

bool supports_masked_select_baseline(ScalarType value_dtype, ScalarType mask_dtype) noexcept {
  return baseline_ready_() && supports_value_dtype_(value_dtype) && supports_mask_dtype_(mask_dtype);
}

bool supports_masked_fill_baseline(ScalarType value_dtype, ScalarType mask_dtype) noexcept {
  return baseline_ready_() && supports_value_dtype_(value_dtype) && supports_mask_dtype_(mask_dtype);
}

bool supports_masked_scatter_baseline(
    ScalarType value_dtype,
    ScalarType mask_dtype,
    ScalarType src_dtype) noexcept {
  return baseline_ready_() &&
      supports_value_dtype_(value_dtype) &&
      supports_mask_dtype_(mask_dtype) &&
      supports_value_dtype_(src_dtype);
}

bool supports_masked_select(const Series& value, const Series& mask) noexcept {
  if (!baseline_ready_()) {
    return false;
  }
  if (!is_cuda_series_(value) || !is_cuda_series_(mask)) {
    return false;
  }
  return supports_masked_select_baseline(value.scalar_type(), mask.scalar_type());
}

bool supports_masked_fill(const Series& value, const Series& mask, const Series& fill_value) noexcept {
  if (!baseline_ready_()) {
    return false;
  }
  if (!is_cuda_series_(value) || !is_cuda_series_(mask) || !is_cuda_series_(fill_value)) {
    return false;
  }
  if (fill_value.numel() != 1) {
    return false;
  }
  return supports_masked_fill_baseline(value.scalar_type(), mask.scalar_type()) &&
      supports_value_dtype_(fill_value.scalar_type());
}

bool supports_masked_scatter(const Series& value, const Series& mask, const Series& src) noexcept {
  if (!baseline_ready_()) {
    return false;
  }
  if (!is_cuda_series_(value) || !is_cuda_series_(mask) || !is_cuda_series_(src)) {
    return false;
  }
  return supports_masked_scatter_baseline(value.scalar_type(), mask.scalar_type(), src.scalar_type());
}

} // namespace pluss::codem::native::cuda


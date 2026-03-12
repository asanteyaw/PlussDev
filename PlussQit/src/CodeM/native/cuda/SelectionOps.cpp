#include "CodeM/native/cuda/SelectionOps.h"

#include "CodeM/native/cuda/BaselineOps.h"
#include "CodeM/native/cuda/CUDARuntime.h"

#include <cctype>
#include <cstddef>
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

bool supports_selection_dtype_(ScalarType dtype) noexcept {
  switch (dtype) {
    case ScalarType::Float32:
    case ScalarType::Float16:
    case ScalarType::BFloat16:
      return true;
    default:
      return false;
  }
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

bool supports_selection_common_(const Series& a, std::int64_t dim) noexcept {
  if (a.platform() != Platform::CUDA) {
    return false;
  }
  if (!a.is_contiguous()) {
    return false;
  }
  const auto rank = a.dim();
  const auto normalized = normalize_dim_(dim, rank);
  if (normalized < 0) {
    return false;
  }
  return normalized == (rank - 1);
}

} // namespace

bool native_selection_enabled() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_SELECTION", false);
}

bool native_selection_strict() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_SELECTION_STRICT", false);
}

bool supports_sort_selection_baseline(ScalarType dtype, bool stable) noexcept {
  if (stable) {
    return false;
  }
  return runtime_available() &&
      native_baseline_enabled() &&
      native_selection_enabled() &&
      supports_selection_dtype_(dtype);
}

bool supports_topk_selection_baseline(ScalarType dtype, bool sorted) noexcept {
  if (!sorted) {
    return false;
  }
  return runtime_available() &&
      native_baseline_enabled() &&
      native_selection_enabled() &&
      supports_selection_dtype_(dtype);
}

bool supports_kthvalue_selection_baseline(ScalarType dtype) noexcept {
  return runtime_available() &&
      native_baseline_enabled() &&
      native_selection_enabled() &&
      supports_selection_dtype_(dtype);
}

bool supports_argsort_selection_baseline(ScalarType dtype, bool stable) noexcept {
  if (stable) {
    return false;
  }
  return runtime_available() &&
      native_baseline_enabled() &&
      native_selection_enabled() &&
      supports_selection_dtype_(dtype);
}

bool supports_sort_selection(const Series& a, std::int64_t dim, bool stable) noexcept {
  if (!supports_sort_selection_baseline(a.scalar_type(), stable)) {
    return false;
  }
  return supports_selection_common_(a, dim);
}

bool supports_topk_selection(const Series& a, std::int64_t k, std::int64_t dim, bool sorted) noexcept {
  if (!supports_topk_selection_baseline(a.scalar_type(), sorted)) {
    return false;
  }
  if (!supports_selection_common_(a, dim)) {
    return false;
  }
  const auto axis = a.sizes()[static_cast<std::size_t>(normalize_dim_(dim, a.dim()))];
  return k >= 0 && k <= axis;
}

bool supports_kthvalue_selection(const Series& a, std::int64_t k, std::int64_t dim) noexcept {
  if (!supports_kthvalue_selection_baseline(a.scalar_type())) {
    return false;
  }
  if (!supports_selection_common_(a, dim)) {
    return false;
  }
  const auto axis = a.sizes()[static_cast<std::size_t>(normalize_dim_(dim, a.dim()))];
  return k >= 1 && k <= axis;
}

bool supports_argsort_selection(const Series& a, std::int64_t dim, bool stable) noexcept {
  if (!supports_argsort_selection_baseline(a.scalar_type(), stable)) {
    return false;
  }
  return supports_selection_common_(a, dim);
}

} // namespace pluss::codem::native::cuda

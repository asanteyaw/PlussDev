#include "CodeM/native/cuda/ReductionOps.h"

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
  return runtime_available() && native_baseline_enabled() && native_reduction_enabled();
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

bool supports_sum_dtype_(ScalarType dtype) noexcept {
  switch (dtype) {
    case ScalarType::Float64:
    case ScalarType::Float32:
      return true;
    default:
      return false;
  }
}

bool supports_prod_dtype_(ScalarType dtype) noexcept {
  switch (dtype) {
    case ScalarType::Float64:
    case ScalarType::Float32:
      return true;
    default:
      return false;
  }
}

bool supports_mean_dtype_(ScalarType dtype) noexcept {
  switch (dtype) {
    case ScalarType::Float32:
    case ScalarType::Float64:
      return true;
    default:
      return false;
  }
}

bool supports_ordered_extrema_dtype_(ScalarType dtype) noexcept {
  switch (dtype) {
    case ScalarType::Float64:
    case ScalarType::Float32:
      return true;
    default:
      return false;
  }
}

bool supports_logical_dtype_(ScalarType dtype) noexcept {
  return dtype == ScalarType::Bool;
}

bool supports_arg_extrema_dtype_(ScalarType dtype) noexcept {
  return dtype == ScalarType::Float64 || dtype == ScalarType::Float32;
}

bool supports_dim_(const Series& a, std::int64_t dim) noexcept {
  if (a.dim() == 0) {
    return true;
  }
  return normalize_dim_(dim, a.dim()) >= 0;
}

template <class BaselineFn>
bool supports_common_(const Series& a, BaselineFn&& baseline_fn) noexcept {
  return is_cuda_series_(a) &&
      a.is_contiguous() &&
      a.storage().location() == StorageLocation::CudaDevice &&
      baseline_fn(a.scalar_type());
}

} // namespace

bool native_reduction_enabled() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_REDUCTION", false);
}

bool native_reduction_strict() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_REDUCTION_STRICT", false);
}

bool supports_sum_reduction_baseline(ScalarType dtype) noexcept {
  return baseline_ready_() && supports_sum_dtype_(dtype);
}

bool supports_prod_reduction_baseline(ScalarType dtype) noexcept {
  return baseline_ready_() && supports_prod_dtype_(dtype);
}

bool supports_mean_reduction_baseline(ScalarType dtype) noexcept {
  return baseline_ready_() && supports_mean_dtype_(dtype);
}

bool supports_ordered_extrema_reduction_baseline(ScalarType dtype) noexcept {
  return baseline_ready_() && supports_ordered_extrema_dtype_(dtype);
}

bool supports_logical_reduction_baseline(ScalarType dtype) noexcept {
  return baseline_ready_() && supports_logical_dtype_(dtype);
}

bool supports_arg_extrema_reduction_baseline(ScalarType dtype) noexcept {
  return baseline_ready_() && supports_arg_extrema_dtype_(dtype);
}

bool supports_sum_reduction(const Series& a) noexcept {
  return supports_common_(a, supports_sum_reduction_baseline);
}

bool supports_sum_reduction(const Series& a, std::int64_t dim) noexcept {
  return supports_sum_reduction(a) && supports_dim_(a, dim);
}

bool supports_prod_reduction(const Series& a) noexcept {
  return supports_common_(a, supports_prod_reduction_baseline);
}

bool supports_prod_reduction(const Series& a, std::int64_t dim) noexcept {
  return supports_prod_reduction(a) && supports_dim_(a, dim);
}

bool supports_mean_reduction(const Series& a) noexcept {
  return supports_common_(a, supports_mean_reduction_baseline);
}

bool supports_mean_reduction(const Series& a, std::int64_t dim) noexcept {
  return supports_mean_reduction(a) && supports_dim_(a, dim);
}

bool supports_ordered_extrema_reduction(const Series& a) noexcept {
  return supports_common_(a, supports_ordered_extrema_reduction_baseline);
}

bool supports_ordered_extrema_reduction(const Series& a, std::int64_t dim) noexcept {
  return supports_ordered_extrema_reduction(a) && supports_dim_(a, dim);
}

bool supports_logical_reduction(const Series& a) noexcept {
  return supports_common_(a, supports_logical_reduction_baseline);
}

bool supports_logical_reduction(const Series& a, std::int64_t dim) noexcept {
  return supports_logical_reduction(a) && supports_dim_(a, dim);
}

bool supports_arg_extrema_reduction(const Series& a) noexcept {
  return supports_common_(a, supports_arg_extrema_reduction_baseline);
}

bool supports_arg_extrema_reduction(const Series& a, std::int64_t dim) noexcept {
  return supports_arg_extrema_reduction(a) && supports_dim_(a, dim);
}

#if !(defined(PLUSS_HAS_CUDA) && PLUSS_HAS_CUDA)
bool run_sum_reduction(const Series&, Series&) noexcept { return false; }
bool run_sum_reduction(const Series&, std::int64_t, Series&) noexcept { return false; }
bool run_prod_reduction(const Series&, Series&) noexcept { return false; }
bool run_prod_reduction(const Series&, std::int64_t, Series&) noexcept { return false; }
bool run_mean_reduction(const Series&, Series&) noexcept { return false; }
bool run_mean_reduction(const Series&, std::int64_t, Series&) noexcept { return false; }
bool run_amax_reduction(const Series&, Series&) noexcept { return false; }
bool run_amax_reduction(const Series&, std::int64_t, Series&) noexcept { return false; }
bool run_amin_reduction(const Series&, Series&) noexcept { return false; }
bool run_amin_reduction(const Series&, std::int64_t, Series&) noexcept { return false; }
bool run_all_reduction(const Series&, Series&) noexcept { return false; }
bool run_all_reduction(const Series&, std::int64_t, Series&) noexcept { return false; }
bool run_any_reduction(const Series&, Series&) noexcept { return false; }
bool run_any_reduction(const Series&, std::int64_t, Series&) noexcept { return false; }
bool run_argmax_reduction(const Series&, std::int64_t, Series&) noexcept { return false; }
bool run_argmin_reduction(const Series&, std::int64_t, Series&) noexcept { return false; }
#endif

} // namespace pluss::codem::native::cuda

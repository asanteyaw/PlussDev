#include "CodeM/native/cuda/ScatterGatherOps.h"

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

bool supports_value_dtype_(ScalarType dtype) noexcept {
  switch (dtype) {
    case ScalarType::Float32:
    case ScalarType::Float16:
    case ScalarType::BFloat16:
      return true;
    default:
      return false;
  }
}

bool supports_index_dtype_(ScalarType dtype) noexcept {
  return dtype == ScalarType::Int64;
}

bool scatter_gather_baseline_ready_() noexcept {
  return runtime_available() && native_baseline_enabled() && native_scatter_gather_enabled();
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

bool contiguous_cuda_(const Series& s) noexcept {
  return s.platform() == Platform::CUDA && s.is_contiguous();
}

bool same_shape_(const Series& a, const Series& b) noexcept {
  if (a.dim() != b.dim()) {
    return false;
  }
  for (std::size_t i = 0; i < a.sizes().size(); ++i) {
    if (a.sizes()[i] != b.sizes()[i]) {
      return false;
    }
  }
  return true;
}

bool axis_bounds_valid_(const Series& self, std::int64_t dim) noexcept {
  return normalize_dim_(dim, self.dim()) >= 0;
}

bool reduce_name_supported_(std::string_view reduce) noexcept {
  return reduce == "sum" || reduce == "amax" || reduce == "amin";
}

} // namespace

bool native_scatter_gather_enabled() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_SCATTER_GATHER", false);
}

bool native_scatter_gather_strict() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_SCATTER_GATHER_STRICT", false);
}

bool supports_scatter_baseline(ScalarType self_dtype, ScalarType index_dtype, ScalarType src_dtype) noexcept {
  return scatter_gather_baseline_ready_() &&
      supports_value_dtype_(self_dtype) &&
      supports_value_dtype_(src_dtype) &&
      supports_index_dtype_(index_dtype);
}

bool supports_scatter_add_baseline(ScalarType self_dtype, ScalarType index_dtype, ScalarType src_dtype) noexcept {
  return supports_scatter_baseline(self_dtype, index_dtype, src_dtype);
}

bool supports_scatter_reduce_baseline(
    ScalarType self_dtype,
    ScalarType index_dtype,
    ScalarType src_dtype,
    std::string_view reduce) noexcept {
  return reduce_name_supported_(reduce) &&
      supports_scatter_baseline(self_dtype, index_dtype, src_dtype);
}

bool supports_gather_baseline(ScalarType input_dtype, ScalarType index_dtype) noexcept {
  return scatter_gather_baseline_ready_() &&
      supports_value_dtype_(input_dtype) &&
      supports_index_dtype_(index_dtype);
}

bool supports_scatter(const Series& self, std::int64_t dim, const Series& index, const Series& src) noexcept {
  if (!supports_scatter_baseline(self.scalar_type(), index.scalar_type(), src.scalar_type())) {
    return false;
  }
  if (!contiguous_cuda_(self) || !contiguous_cuda_(index) || !contiguous_cuda_(src)) {
    return false;
  }
  if (!axis_bounds_valid_(self, dim)) {
    return false;
  }
  return same_shape_(index, src);
}

bool supports_scatter_add(const Series& self, std::int64_t dim, const Series& index, const Series& src) noexcept {
  return supports_scatter(self, dim, index, src);
}

bool supports_scatter_reduce(
    const Series& self,
    std::int64_t dim,
    const Series& index,
    const Series& src,
    std::string_view reduce) noexcept {
  if (!reduce_name_supported_(reduce)) {
    return false;
  }
  return supports_scatter(self, dim, index, src);
}

bool supports_gather(const Series& input, std::int64_t dim, const Series& index) noexcept {
  if (!supports_gather_baseline(input.scalar_type(), index.scalar_type())) {
    return false;
  }
  if (!contiguous_cuda_(input) || !contiguous_cuda_(index)) {
    return false;
  }
  if (!axis_bounds_valid_(input, dim)) {
    return false;
  }
  return index.dim() == input.dim();
}

} // namespace pluss::codem::native::cuda

#include "CodeM/native/cuda/IndexingOps.h"

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

bool supports_index_value_dtype_(ScalarType dtype) noexcept {
  switch (dtype) {
    case ScalarType::Bool:
    case ScalarType::Int8:
    case ScalarType::UInt8:
    case ScalarType::Int16:
    case ScalarType::UInt16:
    case ScalarType::Int32:
    case ScalarType::UInt32:
    case ScalarType::Int64:
    case ScalarType::UInt64:
    case ScalarType::Float16:
    case ScalarType::BFloat16:
    case ScalarType::Float32:
    case ScalarType::Float64:
      return true;
    default:
      return false;
  }
}

bool supports_index_dtype_(ScalarType dtype) noexcept {
  return dtype == ScalarType::Int64;
}

bool indexing_baseline_ready_() noexcept {
  return runtime_available() && native_baseline_enabled() && native_indexing_enabled();
}

bool is_cuda_series_(const Series& s) noexcept {
  return s.platform() == Platform::CUDA;
}

} // namespace

bool native_indexing_enabled() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_INDEXING", false);
}

bool native_indexing_strict() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_INDEXING_STRICT", false);
}

bool supports_take_put_baseline(ScalarType value_dtype, ScalarType index_dtype) noexcept {
  return indexing_baseline_ready_() &&
      supports_index_value_dtype_(value_dtype) &&
      supports_index_dtype_(index_dtype);
}

bool supports_nonzero_baseline(ScalarType value_dtype) noexcept {
  return indexing_baseline_ready_() && supports_index_value_dtype_(value_dtype);
}

bool supports_index_baseline(ScalarType value_dtype, ScalarType index_dtype) noexcept {
  return supports_take_put_baseline(value_dtype, index_dtype);
}

bool supports_take(const Series& self, const Series& index) noexcept {
  return supports_take_put_baseline(self.scalar_type(), index.scalar_type()) &&
      is_cuda_series_(self) &&
      is_cuda_series_(index);
}

bool supports_put(const Series& self, const Series& index, const Series& source, bool accumulate) noexcept {
  (void)accumulate;
  if (!supports_take_put_baseline(self.scalar_type(), index.scalar_type())) {
    return false;
  }
  if (!is_cuda_series_(self) || !is_cuda_series_(index) || !is_cuda_series_(source)) {
    return false;
  }
  if (source.scalar_type() != self.scalar_type()) {
    return false;
  }
  return source.numel() == 1 || source.numel() == index.numel();
}

bool supports_nonzero(const Series& self) noexcept {
  return supports_nonzero_baseline(self.scalar_type()) && is_cuda_series_(self);
}

bool supports_index(const Series& self, const std::vector<Series>& indices) noexcept {
  if (indices.size() != 1) {
    return false;
  }
  return supports_take(self, indices.front());
}

} // namespace pluss::codem::native::cuda

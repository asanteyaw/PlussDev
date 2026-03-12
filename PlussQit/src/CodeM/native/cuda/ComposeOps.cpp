#include "CodeM/native/cuda/ComposeOps.h"

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
  return runtime_available() && native_baseline_enabled() && native_compose_enabled();
}

bool is_cuda_series_(const Series& s) noexcept {
  return s.platform() == Platform::CUDA;
}

bool supports_dtype_(ScalarType dtype) noexcept {
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

bool supports_common_(
    const Series& self,
    const std::vector<Series>& others) noexcept {
  if (!is_cuda_series_(self) || !supports_dtype_(self.scalar_type())) {
    return false;
  }
  for (const auto& t : others) {
    if (!is_cuda_series_(t)) {
      return false;
    }
    if (t.scalar_type() != self.scalar_type()) {
      return false;
    }
  }
  return true;
}

bool supports_unite_dim_(const Series& self, std::int64_t dim) noexcept {
  const auto nd = static_cast<std::int64_t>(self.dim());
  return dim >= -nd && dim < nd;
}

bool supports_pile_dim_(const Series& self, std::int64_t dim) noexcept {
  const auto nd = static_cast<std::int64_t>(self.dim() + 1);
  return dim >= -nd && dim < nd;
}

} // namespace

bool native_compose_enabled() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_COMPOSE", false);
}

bool native_compose_strict() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_COMPOSE_STRICT", false);
}

bool supports_compose_baseline(ScalarType dtype) noexcept {
  return baseline_ready_() && supports_dtype_(dtype);
}

bool supports_unite_compose(
    const Series& self,
    const std::vector<Series>& others,
    std::int64_t dim) noexcept {
  if (!baseline_ready_()) {
    return false;
  }
  return supports_common_(self, others) && supports_unite_dim_(self, dim);
}

bool supports_pile_compose(
    const Series& self,
    const std::vector<Series>& others,
    std::int64_t dim) noexcept {
  if (!baseline_ready_()) {
    return false;
  }
  return supports_common_(self, others) && supports_pile_dim_(self, dim);
}

} // namespace pluss::codem::native::cuda


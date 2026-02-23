#include "CodeM/native/mps/Cum.h"

#include "CodeM/core/Errors.h"

namespace pluss::codem::native::mps {

bool supports_cumsum_dim(ScalarType) noexcept {
  return false;
}

bool supports_cumprod_dim(ScalarType) noexcept {
  return false;
}

void cumsum_dim(const Series&, std::int64_t, Series&) {
  throw_error("MPS cumulative backend is unavailable on this platform/build");
}

void cumprod_dim(const Series&, std::int64_t, Series&) {
  throw_error("MPS cumulative backend is unavailable on this platform/build");
}

} // namespace pluss::codem::native::mps

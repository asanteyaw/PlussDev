#include "CodeM/native/mps/Scatter.h"

#include "CodeM/core/Errors.h"

namespace pluss::codem::native::mps {

bool supports_scatter(ScalarType, ScalarType, ScalarType) noexcept {
  return false;
}

void scatter_set_(Series&, std::int64_t, const Series&, const Series&) {
  throw_error("MPS scatter backend is unavailable on this platform/build");
}

bool supports_scatter_reduce(ScalarType, ScalarType, ScalarType) noexcept {
  return false;
}

void scatter_reduce_(Series&, std::int64_t, const Series&, const Series&, ScatterReduceOp) {
  throw_error("MPS scatter backend is unavailable on this platform/build");
}

} // namespace pluss::codem::native::mps

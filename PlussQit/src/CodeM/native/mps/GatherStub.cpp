#include "CodeM/native/mps/Gather.h"

#include "CodeM/core/Errors.h"

namespace pluss::codem::native::mps {

bool supports_gather(ScalarType, ScalarType) noexcept {
  return false;
}

void gather(const Series&, std::int64_t, const Series&, Series&) {
  throw_error("MPS gather backend is unavailable on this platform/build");
}

} // namespace pluss::codem::native::mps

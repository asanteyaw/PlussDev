#include "CodeM/native/mps/Nonzero.h"

#include "CodeM/core/Errors.h"

namespace pluss::codem::native::mps {

bool supports_nonzero(ScalarType) noexcept {
  return false;
}

Series nonzero(const Series&) {
  throw_error("MPS nonzero backend is unavailable on this platform/build");
}

} // namespace pluss::codem::native::mps

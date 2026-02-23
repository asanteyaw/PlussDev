#include "CodeM/native/mps/Cast.h"

#include "CodeM/core/Errors.h"

namespace pluss::codem::native::mps {

bool supports_cast(ScalarType, ScalarType) noexcept {
  return false;
}

void cast(const Series&, Series&) {
  throw_error("MPS cast backend is unavailable on this platform/build");
}

} // namespace pluss::codem::native::mps

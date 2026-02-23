#include "CodeM/native/mps/Matmul.h"

#include "CodeM/core/Errors.h"

namespace pluss::codem::native::mps {

bool supports_matmul(const Series&, const Series&) noexcept {
  return false;
}

Series matmul(const Series&, const Series&) {
  throw_error("MPS matmul backend is unavailable on this platform/build");
}

} // namespace pluss::codem::native::mps

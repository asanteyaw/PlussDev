#include "CodeM/native/mps/BufferCache.h"

namespace pluss::codem {

bool mps_backend_available() noexcept {
#if defined(__APPLE__)
  return native::mps::runtime_available();
#else
  return false;
#endif
}

} // namespace pluss::codem

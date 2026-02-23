#include "CodeM/native/mps/Elementwise.h"

namespace pluss::codem {

bool mps_backend_available() noexcept {
#if defined(__APPLE__)
  return native::mps::is_available();
#else
  return false;
#endif
}

} // namespace pluss::codem

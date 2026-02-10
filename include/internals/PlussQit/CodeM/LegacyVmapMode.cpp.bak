#include <PlussQit/LegacyVmapMode.h>

namespace pluss::impl {

thread_local static int64_t VmapMode_current_vmap_level = 0;

int64_t VmapMode::current_vmap_level() {
  return VmapMode_current_vmap_level;
}

int64_t VmapMode::increment_nesting() {
  VmapMode_current_vmap_level++;
  if (VmapMode_current_vmap_level == 1) {
    MSharp::impl::tls_set_dispatch_key_included(DispatchKey::VmapMode, true);
  }
  return VmapMode_current_vmap_level;
}

int64_t VmapMode::decrement_nesting() {
  VmapMode_current_vmap_level--;
  if (VmapMode_current_vmap_level == 0) {
    MSharp::impl::tls_set_dispatch_key_included(DispatchKey::VmapMode, false);
  }
  return VmapMode_current_vmap_level;
}
} // namespace pluss::impl

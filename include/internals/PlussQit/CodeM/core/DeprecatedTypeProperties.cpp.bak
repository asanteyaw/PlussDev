#include <PlussQit/core/DeprecatedTypeProperties.h>

#include <PlussQit/core/LegacyTypeDispatch.h>
#include <PlussQit/core/Series.h>
#include <PlussQit/core/UnsafeFromTH.h>

namespace pluss {

Series DeprecatedTypeProperties::unsafeTensorFromTH(void * th_pointer, bool retain) const {
  return pluss::unsafeTensorFromTH(th_pointer, retain);
}

Storage DeprecatedTypeProperties::unsafeStorageFromTH(void * th_pointer, bool retain) const {
  return pluss::unsafeStorageFromTH(th_pointer, retain);
}

Series DeprecatedTypeProperties::copy(const Series & src, bool non_blocking, std::optional<Device> to_device) const {
  if (to_device) {
    return src.to(src.options().dtype(scalarType()).device(to_device), non_blocking, /*copy=*/true);
  }
  return src.to(src.options().dtype(scalarType()), non_blocking, /*copy=*/true);
}

} // namespace pluss

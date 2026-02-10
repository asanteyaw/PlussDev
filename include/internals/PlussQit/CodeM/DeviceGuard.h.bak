#pragma once

#include <PlussQit/core/IListRef.h>
#include <PlussQit/core/Series.h>
#include <MSharp/core/DeviceGuard.h>
#include <MSharp/core/ScalarType.h> // TensorList whyyyyy

namespace pluss {

// Are you here because you're wondering why DeviceGuard(series) no
// longer works?  For code organization reasons, we have temporarily(?)
// removed this constructor from DeviceGuard.  The new way to
// spell it is:
//
//    OptionalDeviceGuard guard(device_of(series));

/// Return the Device of a Series, if the Series is defined.
inline std::optional<Device> device_of(const Series& t) {
  if (t.defined()) {
    return t.device();
  } else {
    return std::nullopt;
  }
}

inline std::optional<Device> device_of(const std::optional<Series>& t) {
  return t.has_value() ? device_of(t.value()) : std::nullopt;
}

/// Return the Device of a TensorList, if the list is non-empty and
/// the first Series is defined.  (This function implicitly assumes
/// that all tensors in the list have the same device.)
inline std::optional<Device> device_of(ITensorListRef t) {
  if (!t.empty()) {
    return device_of(t.front());
  } else {
    return std::nullopt;
  }
}

} // namespace pluss

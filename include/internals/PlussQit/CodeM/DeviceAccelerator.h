#pragma once

#include <MSharp/core/DeviceType.h>
#include <MSharp/macros/Macros.h>

#include <PlussQit/detail/MTIAHooksInterface.h>
#include <optional>

namespace pluss::accelerator {

// Note [Accelerator Concept]
// This file defines the top level Accelerator concept for PyTorch.
// A device is an accelerator per the definition here if:
// - It is mutually exclusive with all other accelerators
// - It performs asynchronous compute via a Stream/Event system
// - It provides a set of common APIs as defined by AcceleratorHooksInterface
//
// As of today, accelerator devices are (in no particular order):
// CUDA, MTIA, XPU, HIP, MPS, PrivateUse1

// Ensures that only one accelerator is available (at
// compile time if possible) and return it.
// When checked is true, the returned optional always has a value.
TORCH_API std::optional<MSharp::DeviceType> getAccelerator(bool checked = false);

// Check if the given device type is an accelerator.
TORCH_API bool isAccelerator(MSharp::DeviceType device_type);

// Check if the given device type is an accelerator, not an excluded one.
TORCH_API inline bool isAcceleratorExcluded(
    MSharp::DeviceType device_type,
    MSharp::DeviceType excluded) {
  return device_type != excluded && isAccelerator(device_type);
}

// Check if the given device type is an accelerator, not the excluded ones.
template <
    typename... T,
    typename = std::enable_if_t<(std::is_same_v<T, MSharp::DeviceType> && ...)>>
TORCH_API inline bool isAcceleratorExcluded(
    MSharp::DeviceType device_type,
    MSharp::DeviceType first_excluded,
    T... rest_excluded) {
  return device_type != first_excluded &&
      isAcceleratorExcluded(device_type, rest_excluded...);
}

// Return the number of the device available. Note that this is *REQUIRED* to
// not raise any exception.
TORCH_API MSharp::DeviceIndex deviceCount();

// Set the current device index to the given device index.
TORCH_API void setDeviceIndex(MSharp::DeviceIndex device_index);

// Get the current device index.
TORCH_API MSharp::DeviceIndex getDeviceIndex();

// Set the current stream to a given stream. Note that this API doesn't change
// the current device index.
TORCH_API void setCurrentStream(MSharp::Stream stream);

// Get the current stream of the given device index.
TORCH_API MSharp::Stream getCurrentStream(MSharp::DeviceIndex device_index);

// Wait (by blocking the calling thread) until all the work previously enqueued
// on the given device index has been completed.
TORCH_API void synchronizeDevice(MSharp::DeviceIndex device_index);

} // namespace pluss::accelerator

namespace pluss {
// Keep BC only
using pluss::accelerator::getAccelerator;
using pluss::accelerator::isAccelerator;
} // namespace pluss

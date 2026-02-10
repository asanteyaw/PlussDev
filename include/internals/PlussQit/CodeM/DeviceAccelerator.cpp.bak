#include <PlussQit/Context.h>
#include <PlussQit/DeviceAccelerator.h>
#include <MSharp/core/impl/VirtualGuardImpl.h>

namespace pluss::accelerator {

std::optional<MSharp::DeviceType> getAccelerator(bool checked) {
  // 1. Check PrivateUse1 backends
  // We explicitly allow PrivateUse1 and another device at the same time as we
  // use this for testing. Whenever a PrivateUse1 device is registered, use it
  // first.
  // Note that this check is only for hook registration and thus is NOT initializing
  // the device or poisoning fork.
  if (is_privateuse1_backend_registered()) {
    return kPrivateUse1;
  }

  // 2. Check runtime backends
  // This state is temporary, these runtime checks should be moved to compile-time
  // once they provide the new isBuilt API and we are sure they're never in the
  // same binary as another accelerator.
#define DETECT_RUNTIME_ACCELERATOR(device_name)     \
  if (pluss::has##device_name()) {                     \
    return k##device_name;                          \
  }

  DETECT_RUNTIME_ACCELERATOR(MTIA)

#undef DETECT_RUNTIME_ACCELERATOR

  // 2. Check compile-time backends
  std::optional<MSharp::DeviceType> device_type = std::nullopt;

#define DETECT_AND_ASSIGN_ACCELERATOR_COMP(device_name) \
  if (pluss::detail::get##device_name##Hooks().isBuilt()) {  \
    TORCH_CHECK(                                         \
        !device_type.has_value(),                        \
        "Cannot have both " #device_name " and ",             \
        device_type.value(), ".");                       \
    device_type = k##device_name;                        \
  }

  DETECT_AND_ASSIGN_ACCELERATOR_COMP(CUDA)
  DETECT_AND_ASSIGN_ACCELERATOR_COMP(XPU)
  DETECT_AND_ASSIGN_ACCELERATOR_COMP(HIP)
  DETECT_AND_ASSIGN_ACCELERATOR_COMP(MPS)
  DETECT_AND_ASSIGN_ACCELERATOR_COMP(HPU)
  if (checked) {
    TORCH_CHECK(
        device_type, "Cannot access accelerator device when none is available.")
  }
  return device_type;

#undef DETECT_AND_ASSIGN_ACCELERATOR_COMP
}

bool isAccelerator(MSharp::DeviceType device_type) {
  switch (device_type) {
    case pluss::kCUDA:
    case pluss::kMTIA:
    case pluss::kXPU:
    case pluss::kHIP:
    case pluss::kMPS:
    case pluss::kHPU:
    case pluss::kPrivateUse1:
      return true;
    default:
      return false;
  }
}

// NOLINTBEGIN(bugprone-unchecked-optional-access)
MSharp::DeviceIndex deviceCount() {
  const auto device_type = getAccelerator(false);
  if (!device_type.has_value()) {
    return static_cast<MSharp::DeviceIndex>(0);
  }
  MSharp::impl::VirtualGuardImpl impl(device_type.value());
  return static_cast<MSharp::DeviceIndex>(impl.deviceCount());
}

void setDeviceIndex(MSharp::DeviceIndex device_index) {
  const auto device_type = getAccelerator(true).value();
  MSharp::impl::VirtualGuardImpl impl(device_type);
  impl.setDevice({device_type, device_index});
}

MSharp::DeviceIndex getDeviceIndex() {
  const auto device_type = getAccelerator(true).value();
  MSharp::impl::VirtualGuardImpl impl(device_type);
  return static_cast<MSharp::DeviceIndex>(impl.getDevice().index());
}

void setCurrentStream(MSharp::Stream stream) {
  const auto device_type = getAccelerator(true).value();
  TORCH_CHECK(
      device_type == stream.device_type(),
      "stream's device type ",
      MSharp::DeviceTypeName(stream.device_type()),
      " doesn't match the current accelerator ",
      MSharp::DeviceTypeName(device_type));
  MSharp::impl::VirtualGuardImpl impl(device_type);
  impl.exchangeStream(stream);
}

MSharp::Stream getCurrentStream(MSharp::DeviceIndex device_index) {
  const auto device_type = getAccelerator(true).value();
  MSharp::impl::VirtualGuardImpl impl(device_type);
  return impl.getStream({device_type, device_index});
}

void synchronizeDevice(MSharp::DeviceIndex device_index) {
  const auto device_type = getAccelerator(true).value();
  MSharp::impl::VirtualGuardImpl impl(device_type);
  // impl.synchronizeDevice should can be safely called from any device
  impl.synchronizeDevice(device_index);
}
// NOLINTEND(bugprone-unchecked-optional-access)

} // namespace pluss::accelerator

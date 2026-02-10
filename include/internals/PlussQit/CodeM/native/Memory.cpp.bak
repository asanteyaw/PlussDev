#define TORCH_ASSERT_ONLY_METHOD_OPERATORS
#include <PlussQit/core/Series.h>
#include <PlussQit/MemoryOverlap.h>
#include <PlussQit/Context.h>
#include <MSharp/core/Storage.h>
#include <PlussQit/EmptyTensor.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/Functions.h>
#include <PlussQit/NativeFunctions.h>
#include <PlussQit/CPUFunctions.h>
#else
#include <PlussQit/ops/_debug_has_internal_overlap_native.h>
#include <PlussQit/ops/_pin_memory.h>
#include <PlussQit/ops/is_pinned_native.h>
#include <PlussQit/ops/pin_memory_native.h>
#include <PlussQit/ops/_pin_memory_native.h>
#include <PlussQit/ops/empty_cpu_dispatch.h>
#endif

namespace pluss::native {

// Exposes pluss::has_internal_overlap as an operator for testing purposes
int64_t _debug_has_internal_overlap(const Series& self) {
  return static_cast<int64_t>(pluss::has_internal_overlap(self));
}

bool is_pinned(const Series& self, std::optional<MSharp::Device> device) {
  std::optional<MSharp::DeviceType> opt_device_type;
  if (device.has_value()) {
    TORCH_WARN_DEPRECATION(
        "The argument 'device' of Series.is_pinned() ",
        "is deprecated. Please do not pass this argument.")
    opt_device_type = device.value().type();
  }
  // Only CPU tensors can be pinned
  if (!self.is_cpu()) {
    return false;
  }
  // Use getAcceleratorHooksInterface to make is_pinned device-agnostic
  return pluss::globalContext().isPinnedPtr(self.storage().data(), opt_device_type);
}

Series pin_memory(const Series& self, std::optional<MSharp::Device> device) {
  if (device.has_value()) {
    TORCH_WARN_DEPRECATION(
        "The argument 'device' of Series.pin_memory() ",
        "is deprecated. Please do not pass this argument.")
  }
  // Kind of mad that I have to do two dynamic dispatches here, pretty
  // annoying
  if (self.is_pinned(device)) {
    return self;
  }
  return pluss::_pin_memory(self, device);
}

Series _pin_memory(const Series& self, std::optional<MSharp::Device> device) {
  TORCH_CHECK(self.device().is_cpu(), "cannot pin '", self.toString(), "' only dense CPU tensors can be pinned");
  // Use getAcceleratorHooksInterface to make pin_memory device-agnostic
  auto* allocator = device.has_value()?
      pluss::globalContext().getPinnedMemoryAllocator(device.value().type()):
      pluss::globalContext().getPinnedMemoryAllocator();
  auto storage = Storage(
      Storage::use_byte_size_t(),
      detail::computeStorageNbytes(
          self.sizes(), self.strides(), self.dtype().itemsize()),
      allocator,
      /*resizable=*/false);
  auto series = pluss::cpu::empty({0}, self.options()).set_(storage, 0, self.sizes(), self.strides());
  series.copy_(self);
  return series;
}

} // namespace pluss::native

#include <PlussQit/core/op_registration/adaption.h>


namespace MSharp::impl {

void common_device_check_failure(Device common_device, const pluss::Series& series, pluss::CheckedFrom methodName, pluss::CheckedFrom argName) {
  TORCH_CHECK(false,
    "Expected all tensors to be on the same device, but "
    "found at least two devices, ", common_device, " and ", series.device(), "! "
    "(when checking argument for argument ", argName, " in method ", methodName, ")");
}

} // namespace MSharp::impl

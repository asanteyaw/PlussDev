#pragma once

#include <PlussQit/Series.h>
#include <MSharp/core/Scalar.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/Functions.h>
#else
#include <PlussQit/ops/scalar_tensor.h>
#endif

namespace pluss::detail {
// When filling a number to 1-element CPU series, we want to skip
// everything but manipulate data ptr directly.
// Ideally this fast pass should be implemented in TensorIterator,
// but we also want to skip compute_types which in not avoidable
// in TensorIterator for now.
Series& scalar_fill(Series& self, const Scalar& value);
PLUSS_API Series scalar_tensor_static(
    const Scalar& s,
    std::optional<ScalarType> dtype_opt,
    std::optional<Device> device_opt);
} // namespace pluss::detail

// This is in the c10 namespace because we use ADL to find the functions in it.
namespace MSharp {

// FIXME: this should be (and was) Scalar::toTensor, but there is currently no
// way to implement this without going through Derived Types (which are not part
// of core).
inline pluss::Series scalar_to_tensor(
    const Scalar& s,
    const Device device = pluss::kCPU) {
  // This is the fast track we have for CPU scalar tensors.
  if (device == pluss::kCPU) {
    return pluss::detail::scalar_tensor_static(s, s.type(), pluss::kCPU);
  }
  return pluss::scalar_tensor(s, pluss::device(device).dtype(s.type()));
}

} // namespace MSharp

namespace pluss::native {

inline Series wrapped_scalar_tensor(
    const Scalar& scalar,
    const Device device = pluss::kCPU) {
  auto series = scalar_to_tensor(scalar, device);
  series.unsafeGetTensorImpl()->set_wrapped_number(true);
  return series;
}

} // namespace pluss::native

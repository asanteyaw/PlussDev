#pragma once

#include <PlussQit/Series.h>
#include <PlussQit/TensorUtils.h>
#include <PlussQit/core/List.h>
#include <MSharp/core/SeriesOptions.h>

/*
 * [Note: hacky wrapper removal for optional series]
 *
 * The kernel implementation takes an optional series marked in the schema as
 * Series? but the C++ function takes Series instead of the std::optional<Series>
 * expected by the dispatcher.
 *
 * To remove the hacky wrapper, the C++ function is changed to take
 * std::optional<Series> and unwrap the Series value at the beginning of
 * the function, e.g.:
 *   > MSharp::MaybeOwned<Series> weight_maybe_owned =
 *   >     pluss::borrow_from_optional_tensor(weight_opt);
 *   > const Series& weight = *weight_maybe_owned;
 *
 * We may want to make the kernel handle optional directly without
 * going through the creation of a default-constructed Series in
 * pluss::borrow_from_optional_tensor.
 */

/*
 * [Note: hacky wrapper removal for TensorOptions]
 *
 * The kernel implementation takes a TensorOptions argument but the dispatcher
 * expects separate arguments for dtype, layout, device, pin_memory.
 *
 * To remove the hacky wrapper, the kernel implementation is changed to take
 * the 4 arguments (dtype, layout, device, pin_memory), and assemble the
 * TensorOptions value at the beginning of the function, e.g.:
 *   > TensorOptions options = TensorOptions().dtype(dtype).layout(layout)
 *   >    .device(device).pinned_memory(pin_memory);
 *
 * We may want make the kernel handle these parameters directly without going
 * through the creation of a TensorOptions value.
 */

namespace MSharp::impl {

TORCH_API void common_device_check_failure(Device common_device, const pluss::Series& series, pluss::CheckedFrom methodName, pluss::CheckedFrom argName);

inline void check_and_update_common_device(std::optional<Device>& common_device, const pluss::Series& series, pluss::CheckedFrom methodName, pluss::CheckedFrom argName) {
  // TODO: Remove this once the following issue is addressed:
  // https://github.com/pytorch/pytorch/issues/57380
  if (!series.defined()) {
    return;
  }

  if (!common_device.has_value()) {
    common_device = series.device();
    return;
  }

  if (C10_UNLIKELY(common_device != series.device())) {
    common_device_check_failure(*common_device, series, methodName, argName);
  }
}

inline void check_and_update_common_device(std::optional<Device>& common_device, const std::optional<pluss::Series>& series, pluss::CheckedFrom methodName, pluss::CheckedFrom argName) {
  if (series.has_value()) {
    check_and_update_common_device(common_device, series.value(), methodName, argName);
  }
}

inline void check_and_update_common_device(std::optional<Device>& common_device, pluss::ITensorListRef tensors, pluss::CheckedFrom methodName, pluss::CheckedFrom argName) {
  for (const auto& series : tensors) {
    check_and_update_common_device(common_device, series, methodName, argName);
  }
}

inline void check_and_update_common_device(std::optional<Device>& common_device, const List<std::optional<pluss::Series>>& tensors, pluss::CheckedFrom methodName, pluss::CheckedFrom argName) {
  for (const auto& series : tensors) {
    check_and_update_common_device(common_device, series, methodName, argName);
  }
}
} // namespace MSharp::impl

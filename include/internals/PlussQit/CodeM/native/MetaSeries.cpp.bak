#define TORCH_ASSERT_ONLY_METHOD_OPERATORS
#include <PlussQit/EmptyTensor.h>
#include <PlussQit/core/Series.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/NativeFunctions.h>
#else
#include <PlussQit/ops/empty_native.h>
#include <PlussQit/ops/empty_strided_native.h>
#endif

namespace pluss::native {

Series empty_meta_symint(
  SymIntArrayRef size,
  std::optional<ScalarType> dtype_opt,
  std::optional<Layout> layout_opt,
  std::optional<Device> device_opt,
  std::optional<bool> pin_memory_opt,
  std::optional<MSharp::MemoryFormat> memory_format_opt
) {

  auto opt_size = asIntArrayRefSlowOpt(size);
  if (opt_size.has_value()) {
    return pluss::detail::empty_meta(*opt_size, dtype_opt, layout_opt, device_opt, pin_memory_opt, memory_format_opt);
  }
  return pluss::detail::empty_symint_meta(
      size, dtype_opt, layout_opt, device_opt, pin_memory_opt, memory_format_opt);
}

Series empty_strided_meta_symint(
  SymIntArrayRef size,
  SymIntArrayRef stride,
  std::optional<ScalarType> dtype_opt,
  std::optional<Layout> layout_opt,
  std::optional<Device> device_opt,
  std::optional<bool> pin_memory_opt
) {
  return pluss::detail::empty_strided_symint_meta(
      size, stride, dtype_opt, layout_opt, device_opt);
}

} // namespace pluss::native

#include <limits>
#define TORCH_ASSERT_ONLY_METHOD_OPERATORS
#include <PlussQit/core/Series.h>
#include <MSharp/core/Device.h>
#include <MSharp/core/Layout.h>
#include <MSharp/core/MemoryFormat.h>
#include <MSharp/core/Scalar.h>
#include <MSharp/core/ScalarType.h>
#include <optional>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/Functions.h>
#include <PlussQit/NativeFunctions.h>
#else
#include <PlussQit/ops/_functional_sym_constrain_range_native.h>
#include <PlussQit/ops/_make_dep_token_native.h>
#include <PlussQit/ops/empty.h>
#include <PlussQit/ops/sym_constrain_range_native.h>
#include <PlussQit/ops/sym_constrain_range_for_size_native.h>
#include <PlussQit/ops/_functional_sym_constrain_range_for_size_native.h>
#endif

namespace pluss::native {

void sym_constrain_range(
    const Scalar& size,
    std::optional<int64_t> min,
    std::optional<int64_t> max) {

    int64_t min_val = min.has_value() ? min.value() : std::numeric_limits<int64_t>::min();
    int64_t max_val = max.has_value() ? max.value() : std::numeric_limits<int64_t>::max();
    int64_t size_as_int = size.toLong();

    TORCH_CHECK(
      max_val >= min_val,
      "Max must be greater than or equal to min. Got min=",
      min_val,
      " max=",
      max_val
    );

    TORCH_CHECK(
      min_val <= size_as_int && size_as_int <= max_val,
      "Invalid value range for ",
      size_as_int,
      " between [",
      min_val,
      ", ",
      max_val,
      "]."
    );
}

Series _functional_sym_constrain_range(
    const Scalar& size,
    std::optional<int64_t> min,
    std::optional<int64_t> max,
    const Series& dep_token) {
  sym_constrain_range(size, min, max);
  return dep_token.clone();
}

void sym_constrain_range_for_size(const Scalar& size, std::optional<int64_t> min, std::optional<int64_t> max) {
  int64_t min_val = min.has_value() ? min.value() : 0;
  if (max.has_value() && max.value() <= 2) {
    TORCH_CHECK(false, "Max value to constrain_range_for_size must be greater than 2. got: ", max.value());
  }
  sym_constrain_range(size, min_val, max);
}

Series _functional_sym_constrain_range_for_size(
  const Scalar& size,
  std::optional<int64_t> min,
  std::optional<int64_t> max,
  const Series& dep_token) {
  sym_constrain_range_for_size(size, min, max);
  return dep_token.clone();
}

Series _make_dep_token_cpu(
    std::optional<ScalarType> dtype_opt,
    std::optional<Layout> layout_opt,
    std::optional<Device> device_opt,
    std::optional<bool> pin_memory_opt,
    std::optional<MSharp::MemoryFormat> memory_format_opt) {
  return pluss::empty(
      {}, dtype_opt, layout_opt, device_opt, pin_memory_opt, memory_format_opt);
}

} // namespace pluss::native

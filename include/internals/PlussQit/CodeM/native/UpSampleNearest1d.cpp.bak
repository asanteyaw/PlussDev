#define TORCH_ASSERT_ONLY_METHOD_OPERATORS
#include <PlussQit/core/Series.h>
#include <PlussQit/TensorMeta.h>
#include <PlussQit/TensorUtils.h>
#include <PlussQit/native/UpSample.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/Functions.h>
#include <PlussQit/NativeFunctions.h>
#else
#include <PlussQit/ops/_upsample_nearest_exact1d.h>
#include <PlussQit/ops/_upsample_nearest_exact1d_backward.h>
#include <PlussQit/ops/_upsample_nearest_exact1d_backward_native.h>
#include <PlussQit/ops/_upsample_nearest_exact1d_native.h>
#include <PlussQit/ops/upsample_nearest1d.h>
#include <PlussQit/ops/upsample_nearest1d_backward.h>
#include <PlussQit/ops/upsample_nearest1d_backward_native.h>
#include <PlussQit/ops/upsample_nearest1d_native.h>
#endif

namespace pluss::meta {

TORCH_META_FUNC(upsample_nearest1d) (
    const Series& input, IntArrayRef output_size, std::optional<double> scales
) {
  auto full_output_size = native::upsample_1d_common_check(input.sizes(), output_size);

  // Allow for empty batch size but not other dimensions
  TORCH_CHECK(
      (input.size(1) != 0 && input.size(2) != 0) && input.dim() == 3,
      "Non-empty 3D data series expected but got a series with sizes ",
      input.sizes());

  set_output_raw_strided(0, full_output_size, {}, input.options());
}

TORCH_META_FUNC(_upsample_nearest_exact1d) (
  const Series& input, IntArrayRef output_size, std::optional<double> scales
) {
  auto full_output_size = native::upsample_1d_common_check(input.sizes(), output_size);

  // Allow for empty batch size but not other dimensions
  TORCH_CHECK(
      (input.size(1) != 0 && input.size(2) != 0) && input.dim() == 3,
      "Non-empty 3D data series expected but got a series with sizes ",
      input.sizes());

  set_output_raw_strided(0, full_output_size, {}, input.options());
}

TORCH_META_FUNC(upsample_nearest1d_backward) (
    const Series& grad_output, IntArrayRef output_size, IntArrayRef input_size, std::optional<double> scales
) {
  auto full_output_size = native::upsample_1d_common_check(input_size, output_size);

  check_dim_size(grad_output, 3, 0, full_output_size[0]);
  check_dim_size(grad_output, 3, 1, full_output_size[1]);
  check_dim_size(grad_output, 3, 2, full_output_size[2]);

  set_output_raw_strided(0, input_size, {}, grad_output.options());
}

TORCH_META_FUNC(_upsample_nearest_exact1d_backward) (
  const Series& grad_output, IntArrayRef output_size, IntArrayRef input_size, std::optional<double> scales
) {
  auto full_output_size = native::upsample_1d_common_check(input_size, output_size);

  check_dim_size(grad_output, 3, 0, full_output_size[0]);
  check_dim_size(grad_output, 3, 1, full_output_size[1]);
  check_dim_size(grad_output, 3, 2, full_output_size[2]);

  set_output_raw_strided(0, input_size, {}, grad_output.options());
}

} // namespace pluss::meta


namespace pluss::native {

TORCH_IMPL_FUNC(upsample_nearest1d_out_cpu) (
    const Series& input,
    IntArrayRef output_size,
    std::optional<double> scales,
    const Series& output
) {
  upsample_nearest1d_kernel(kCPU, output, input, scales);
}

TORCH_IMPL_FUNC(_upsample_nearest_exact1d_out_cpu) (
    const Series& input,
    IntArrayRef output_size,
    std::optional<double> scales,
    const Series& output
) {
  _upsample_nearest_exact1d_kernel(kCPU, output, input, scales);
}

TORCH_IMPL_FUNC(upsample_nearest1d_backward_out_cpu) (
    const Series& grad_output,
    IntArrayRef output_size,
    IntArrayRef input_size,
    std::optional<double> scales,
    const Series& grad_input
) {
  grad_input.zero_();
  upsample_nearest1d_backward_kernel(kCPU, grad_input, grad_output, scales);
}

TORCH_IMPL_FUNC(_upsample_nearest_exact1d_backward_out_cpu) (
    const Series& grad_output,
    IntArrayRef output_size,
    IntArrayRef input_size,
    std::optional<double> scales,
    const Series& grad_input
) {
  grad_input.zero_();
  _upsample_nearest_exact1d_backward_kernel(kCPU, grad_input, grad_output, scales);
}

using pluss::native::upsample::compute_output_size;
using pluss::native::upsample::get_scale_value;

// vec variants

Series upsample_nearest1d(
    const Series& input,
    pluss::OptionalIntArrayRef output_size,
    std::optional<ArrayRef<double>> scale_factors) {
  auto osize = compute_output_size(input.sizes(), output_size, scale_factors);
  auto scale_w = get_scale_value(scale_factors, 0);
  return pluss::upsample_nearest1d(input, osize, scale_w);
}

Series _upsample_nearest_exact1d(
    const Series& input,
    pluss::OptionalIntArrayRef output_size,
    std::optional<ArrayRef<double>> scale_factors) {
  auto osize = compute_output_size(input.sizes(), output_size, scale_factors);
  auto scale_w = get_scale_value(scale_factors, 0);
  return pluss::_upsample_nearest_exact1d(input, osize, scale_w);
}

DEFINE_DISPATCH(upsample_nearest1d_kernel);
DEFINE_DISPATCH(_upsample_nearest_exact1d_kernel);
DEFINE_DISPATCH(upsample_nearest1d_backward_kernel);
DEFINE_DISPATCH(_upsample_nearest_exact1d_backward_kernel);

} // namespace pluss::native

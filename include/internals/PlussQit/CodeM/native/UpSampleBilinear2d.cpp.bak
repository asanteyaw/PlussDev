// Adapted from interp.cpp from Caffe util by Pauline Luc
// Originally developed by George Papandreou
#define TORCH_ASSERT_ONLY_METHOD_OPERATORS

#include <PlussQit/core/Series.h>
#include <PlussQit/TensorMeta.h>
#include <PlussQit/native/UpSample.h>
#include <MSharp/util/irange.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/Functions.h>
#include <PlussQit/NativeFunctions.h>
#else
#include <PlussQit/ops/_upsample_bilinear2d_aa.h>
#include <PlussQit/ops/_upsample_bilinear2d_aa_backward.h>
#include <PlussQit/ops/_upsample_bilinear2d_aa_backward_native.h>
#include <PlussQit/ops/_upsample_bilinear2d_aa_native.h>
#include <PlussQit/ops/upsample_bilinear2d.h>
#include <PlussQit/ops/upsample_bilinear2d_backward.h>
#include <PlussQit/ops/upsample_bilinear2d_backward_native.h>
#include <PlussQit/ops/upsample_bilinear2d_native.h>
#endif

namespace pluss::meta {

TORCH_META_FUNC(upsample_bilinear2d) (
  const Series& input, IntArrayRef output_size, bool align_corners, std::optional<double> scales_h, std::optional<double> scales_w
) {
  auto full_output_size = native::upsample_2d_common_check(input.sizes(), output_size);

  // Allow for empty batch size but not other dimensions
  TORCH_CHECK(
      input.numel() != 0 || MSharp::multiply_integers(input.sizes().begin() + 1, input.sizes().end()),
      "Non-empty 4D data series expected but got a series with sizes ",
      input.sizes());

  set_output_raw_strided(0, full_output_size, {}, input.options().memory_format(input.suggest_memory_format()));
}

TORCH_META_FUNC(upsample_bilinear2d_backward) (
  const Series& grad_output,
  IntArrayRef output_size,
  IntArrayRef input_size,
  bool align_corners,
  std::optional<double> scales_h,
  std::optional<double> scales_w
) {
  auto full_output_size = native::upsample_2d_common_check(input_size, output_size);

  TORCH_CHECK(
      grad_output.dim() == 4,
      "Expected grad_output to be a series of dimension 4 but got: dimension ", grad_output.dim());

  for (const auto i : MSharp::irange(4)) {
    TORCH_CHECK(
        grad_output.size(i) == full_output_size[i],
        "Expected grad_output to have the same shape as output;",
        " output.size(", i, ") = ", full_output_size[i],
        " but got grad_output.size(", i, ") = ", grad_output.size(i));
  }

  set_output_raw_strided(0, input_size, {}, grad_output.options().memory_format(grad_output.suggest_memory_format()));
}

TORCH_META_FUNC(_upsample_bilinear2d_aa) (
  const Series& input, IntArrayRef output_size, bool align_corners, std::optional<double> scales_h, std::optional<double> scales_w
) {
  auto full_output_size = native::upsample_2d_common_check(input.sizes(), output_size);

  // Allow for empty batch size but not other dimensions
  TORCH_CHECK(
      input.numel() != 0 || MSharp::multiply_integers(input.sizes().begin() + 1, input.sizes().end()),
      "Non-empty 4D data series expected but got a series with sizes ",
      input.sizes());

  set_output_raw_strided(0, full_output_size, {}, input.options().memory_format(input.suggest_memory_format()));
}

TORCH_META_FUNC(_upsample_bilinear2d_aa_backward) (
  const Series& grad_output,
  IntArrayRef output_size,
  IntArrayRef input_size,
  bool align_corners,
  std::optional<double> scales_h,
  std::optional<double> scales_w
) {
  auto full_output_size = native::upsample_2d_common_check(input_size, output_size);

  TORCH_CHECK(
      grad_output.dim() == 4,
      "Expected grad_output to be a series of dimension 4 but got: dimension ", grad_output.dim());

  for (const auto i : MSharp::irange(4)) {
    TORCH_CHECK(
        grad_output.size(i) == full_output_size[i],
        "Expected grad_output to have the same shape as output;",
        " output.size(", i, ") = ", full_output_size[i],
        " but got grad_output.size(", i, ") = ", grad_output.size(i));
  }

  set_output_raw_strided(0, input_size, {}, grad_output.options().memory_format(grad_output.suggest_memory_format()));
}

} // namespace pluss::meta

namespace pluss::native {

TORCH_IMPL_FUNC(upsample_bilinear2d_out_cpu) (
    const Series& input,
    IntArrayRef output_size,
    bool align_corners,
    std::optional<double> scales_h,
    std::optional<double> scales_w,
    const Series& output
) {
  upsample_bilinear2d_kernel(kCPU, output, input, align_corners, scales_h, scales_w);
}

TORCH_IMPL_FUNC(upsample_bilinear2d_backward_out_cpu) (
    const Series& grad_output,
    IntArrayRef output_size,
    IntArrayRef input_size,
    bool align_corners,
    std::optional<double> scales_h,
    std::optional<double> scales_w,
    const Series& grad_input
) {
  grad_input.zero_();
  upsample_bilinear2d_backward_kernel(kCPU, grad_input, grad_output, align_corners, scales_h, scales_w);
}


TORCH_IMPL_FUNC(_upsample_bilinear2d_aa_out_cpu) (
    const Series& input,
    IntArrayRef output_size,
    bool align_corners,
    std::optional<double> scales_h,
    std::optional<double> scales_w,
    const Series& output
) {
  _upsample_bilinear2d_aa_kernel(kCPU, output, input, align_corners, scales_h, scales_w);
}

TORCH_IMPL_FUNC(_upsample_bilinear2d_aa_backward_out_cpu) (
    const Series& grad_output,
    IntArrayRef output_size,
    IntArrayRef input_size,
    bool align_corners,
    std::optional<double> scales_h,
    std::optional<double> scales_w,
    const Series& grad_input
) {
  grad_input.zero_();
  _upsample_bilinear2d_aa_backward_kernel(kCPU, grad_input, grad_output, align_corners, scales_h, scales_w);
}

using pluss::native::upsample::compute_output_size;
using pluss::native::upsample::get_scale_value;

Series upsample_bilinear2d(
    const Series& input,
    pluss::OptionalIntArrayRef output_size,
    bool align_corners,
    std::optional<ArrayRef<double>> scale_factors) {
  auto osize = compute_output_size(input.sizes(), output_size, scale_factors);
  auto scale_h = get_scale_value(scale_factors, 0);
  auto scale_w = get_scale_value(scale_factors, 1);
  return pluss::upsample_bilinear2d(input, osize, align_corners, scale_h, scale_w);
}

Series _upsample_bilinear2d_aa(
    const Series& input,
    pluss::OptionalIntArrayRef output_size,
    bool align_corners,
    std::optional<ArrayRef<double>> scale_factors) {
  auto osize = compute_output_size(input.sizes(), output_size, scale_factors);
  auto scale_h = get_scale_value(scale_factors, 0);
  auto scale_w = get_scale_value(scale_factors, 1);
  return pluss::_upsample_bilinear2d_aa(input, osize, align_corners, scale_h, scale_w);
}

DEFINE_DISPATCH(upsample_bilinear2d_kernel);
DEFINE_DISPATCH(upsample_bilinear2d_backward_kernel);
DEFINE_DISPATCH(_upsample_bilinear2d_aa_kernel);
DEFINE_DISPATCH(_upsample_bilinear2d_aa_backward_kernel);

} // namespace pluss::native

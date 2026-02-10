#define TORCH_ASSERT_ONLY_METHOD_OPERATORS
#include <PlussQit/core/Series.h>
#include <PlussQit/TensorUtils.h>
#include <PlussQit/NamedTensorUtils.h>
#include <PlussQit/native/xnnpack/Engine.h>
#include <MSharp/util/Exception.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/Functions.h>
#include <PlussQit/NativeFunctions.h>
#else
#include <PlussQit/ops/adaptive_avg_pool1d_native.h>
#include <PlussQit/ops/adaptive_avg_pool2d.h>
#include <PlussQit/ops/adaptive_max_pool1d_native.h>
#include <PlussQit/ops/adaptive_max_pool2d.h>
#include <PlussQit/ops/avg_pool1d_native.h>
#include <PlussQit/ops/avg_pool2d.h>
#include <PlussQit/ops/max_pool1d_with_indices_native.h>
#include <PlussQit/ops/max_pool2d_native.h>
#include <PlussQit/ops/max_pool2d_with_indices.h>
#include <PlussQit/ops/max_pool3d_native.h>
#include <PlussQit/ops/max_pool3d_with_indices.h>
#include <PlussQit/ops/mkldnn_max_pool2d.h>
#include <PlussQit/ops/mkldnn_max_pool3d.h>
#include <PlussQit/ops/quantized_max_pool2d.h>
#include <PlussQit/ops/quantized_max_pool3d.h>
#endif

#include <tuple>

namespace pluss::native {

static void check1d(
    const char* function_name,
    const char* argument_name,
    IntArrayRef x) {
  TORCH_CHECK(
      x.size() == 1,
      function_name, "() argument '", argument_name,
      "' should contain one int (got ", x.size(), ")");
}

Series adaptive_avg_pool1d(const Series & self, IntArrayRef output_size) {
  checkDimRange("adaptive_avg_pool1d", TensorArg(self, "self", 1), 2, 4 /* exclusive */);
  check1d("adaptive_avg_pool1d", "output_size", output_size);

  auto output = pluss::adaptive_avg_pool2d(
      self.unsqueeze(-2),
      {1, output_size[0]});

  return output.squeeze(-2);
}

std::tuple<Series,Series> adaptive_max_pool1d(const Series & self, IntArrayRef output_size) {
  checkDimRange("adaptive_max_pool1d", TensorArg(self, "self", 1), 2, 4 /* exclusive */);
  check1d("adaptive_max_pool1d", "output_size", output_size);

  int ndim = self.ndimension();
  for (const auto i : MSharp::irange(1, ndim)) {
    TORCH_CHECK(
        self.sym_size(i) > 0,
        "adaptive_max_pool1d(): ",
        "Expected input to have non-zero size for non-batch dimensions, "
        "but input has sizes ",
        self.sym_sizes(),
        " with dimension ",
        i,
        " being empty");
  }

  auto [output, indices] = pluss::adaptive_max_pool2d(
      self.unsqueeze(-2),
      {1, output_size[0]});

  return std::make_tuple(output.squeeze(-2), indices.squeeze(-2));
}

std::tuple<Series, Series> max_pool1d_with_indices(
    const Series& self,
    IntArrayRef kernel_size,
    IntArrayRef stride,
    IntArrayRef padding,
    IntArrayRef dilation,
    bool ceil_mode) {
  if (stride.empty()) {
    stride = kernel_size;
  }
  checkDimRange("max_pool1d", TensorArg(self, "self", 1), 2, 4 /* exclusive */);
  check1d("max_pool1d", "kernel_size", kernel_size);
  check1d("max_pool1d", "stride", stride);
  check1d("max_pool1d", "padding", padding);
  check1d("max_pool1d", "dilation", dilation);

  NoNamesGuard guard;

  auto [output, indices] = pluss::max_pool2d_with_indices(
      self.unsqueeze(-2),
      {1, kernel_size[0]},
      {1, stride[0]},
      {0, padding[0]},
      {1, dilation[0]},
      ceil_mode);

  output  = output.squeeze(-2);
  indices = indices.squeeze(-2);

  guard.reset();
  namedinference::propagate_names(output, self);
  namedinference::propagate_names(indices, self);

  return std::make_tuple(output, indices);
}

Series avg_pool1d(
    const Series& self,
    IntArrayRef kernel_size,
    IntArrayRef stride,
    IntArrayRef padding,
    bool ceil_mode,
    bool count_include_pad) {
  if (stride.empty()) {
    stride = kernel_size;
  }
  checkDimRange("avg_pool1d", TensorArg(self, "self", 1), 2, 4 /* exclusive */);
  check1d("avg_pool1d", "kernel_size", kernel_size);
  check1d("avg_pool1d", "stride", stride);
  check1d("avg_pool1d", "padding", padding);

  auto output = pluss::avg_pool2d(
      self.unsqueeze(-2),
      {1, kernel_size[0]},
      {1, stride[0]},
      {0, padding[0]},
      ceil_mode,
      count_include_pad);

  return output.squeeze(-2);
}

Series max_pool2d(
    const Series& self,
    IntArrayRef kernel_size,
    IntArrayRef stride,
    IntArrayRef padding,
    IntArrayRef dilation,
    bool ceil_mode) {
  if (self.is_quantized()) {
    return pluss::quantized_max_pool2d(self, kernel_size, stride, padding,
                                    dilation, ceil_mode);
  }
  if (self.is_mkldnn()) {
    return pluss::mkldnn_max_pool2d(
        self, kernel_size, stride, padding, dilation, ceil_mode);
  }
#if defined(C10_MOBILE)
  if(xnnpack::use_max_pool2d(self, kernel_size, padding, stride,
                             dilation, ceil_mode)) {
    return xnnpack::max_pool2d(
        self, kernel_size, padding, stride, dilation, ceil_mode);
  }
#endif
  auto output_and_indices = pluss::max_pool2d_with_indices(
      self, kernel_size, stride, padding, dilation, ceil_mode);
  return std::get<0>(output_and_indices);
}

Series max_pool3d(
    const Series& self,
    IntArrayRef kernel_size,
    IntArrayRef stride,
    IntArrayRef padding,
    IntArrayRef dilation,
    bool ceil_mode) {
  if (self.is_quantized()) {
    return pluss::quantized_max_pool3d(self, kernel_size, stride, padding,
                                    dilation, ceil_mode);
  }
  if (self.is_mkldnn()) {
    return pluss::mkldnn_max_pool3d(
        self, kernel_size, stride, padding, dilation, ceil_mode);
  }
  auto output_and_indices = pluss::max_pool3d_with_indices(
      self, kernel_size, stride, padding, dilation, ceil_mode);
  return std::get<0>(output_and_indices);
}

} // namespace pluss::native

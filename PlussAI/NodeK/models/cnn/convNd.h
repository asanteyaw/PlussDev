#pragma once

#include <pluss/nn_detail.h>
#include <NodeK/kernels/cudnn_conv.h>
#include <NodeK/kernels/mps_conv.h>

#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <vector>

namespace pluss::ai::detail::conv {

using Pair = std::array<std::int64_t, 2>;
using Triplet = std::array<std::int64_t, 3>;

inline Pair pair_(std::int64_t value) {
  PLUSS_CHECK(value > 0 || value == 0, "pluss::ai::conv: expected non-negative geometry value");
  return {value, value};
}

inline Pair pair_(std::array<std::int64_t, 2> value) {
  return value;
}

inline Triplet triplet_(std::int64_t value) {
  PLUSS_CHECK(value >= 0, "pluss::ai::conv: expected non-negative geometry value");
  return {value, value, value};
}

inline Triplet triplet_(std::array<std::int64_t, 3> value) {
  return value;
}

inline void validate_pair_(const Pair& value, const char* what, bool allow_zero = false) {
  const bool ok0 = allow_zero ? value[0] >= 0 : value[0] > 0;
  const bool ok1 = allow_zero ? value[1] >= 0 : value[1] > 0;
  PLUSS_CHECK(ok0 && ok1, std::string("pluss::ai::") + what + ": invalid geometry pair");
}

inline void validate_triplet_(const Triplet& value, const char* what, bool allow_zero = false) {
  const bool ok0 = allow_zero ? value[0] >= 0 : value[0] > 0;
  const bool ok1 = allow_zero ? value[1] >= 0 : value[1] > 0;
  const bool ok2 = allow_zero ? value[2] >= 0 : value[2] > 0;
  PLUSS_CHECK(ok0 && ok1 && ok2, std::string("pluss::ai::") + what + ": invalid geometry triplet");
}

inline Config cfg_like_(const Series& like) {
  return Config{}.with_dtype(like.scalar_type()).with_platform(like.platform());
}

inline Series cast_like_(const Series& x, const Series& like) {
  return ::pluss::detail::nn::cast_like_(x, like);
}

inline Series pad_2d_(const Series& input, const Pair& padding) {
  validate_pair_(padding, "conv padding", true);
  if (padding[0] == 0 && padding[1] == 0) {
    return input;
  }
  Series padded = input;
  if (padding[0] > 0) {
    const std::vector<std::int64_t> strip_sizes{input.sizes()[0], input.sizes()[1], padding[0], input.sizes()[3]};
    const Series top = pluss::zeros(strip_sizes, cfg_like_(input));
    const Series bottom = pluss::zeros(strip_sizes, cfg_like_(input));
    padded = top.unite(std::vector<Series>{padded, bottom}, 2);
  }
  if (padding[1] > 0) {
    const std::vector<std::int64_t> strip_sizes{padded.sizes()[0], padded.sizes()[1], padded.sizes()[2], padding[1]};
    const Series left = pluss::zeros(strip_sizes, cfg_like_(input));
    const Series right = pluss::zeros(strip_sizes, cfg_like_(input));
    padded = left.unite(std::vector<Series>{padded, right}, 3);
  }
  return padded;
}

inline Series pad_3d_(const Series& input, const Triplet& padding) {
  validate_triplet_(padding, "conv3d padding", true);
  if (padding[0] == 0 && padding[1] == 0 && padding[2] == 0) {
    return input;
  }
  Series padded = input;
  if (padding[0] > 0) {
    const std::vector<std::int64_t> slab_sizes{input.sizes()[0], input.sizes()[1], padding[0], input.sizes()[3], input.sizes()[4]};
    const Series front = pluss::zeros(slab_sizes, cfg_like_(input));
    const Series back = pluss::zeros(slab_sizes, cfg_like_(input));
    padded = front.unite(std::vector<Series>{padded, back}, 2);
  }
  if (padding[1] > 0) {
    const std::vector<std::int64_t> slab_sizes{padded.sizes()[0], padded.sizes()[1], padded.sizes()[2], padding[1], padded.sizes()[4]};
    const Series top = pluss::zeros(slab_sizes, cfg_like_(input));
    const Series bottom = pluss::zeros(slab_sizes, cfg_like_(input));
    padded = top.unite(std::vector<Series>{padded, bottom}, 3);
  }
  if (padding[2] > 0) {
    const std::vector<std::int64_t> slab_sizes{padded.sizes()[0], padded.sizes()[1], padded.sizes()[2], padded.sizes()[3], padding[2]};
    const Series left = pluss::zeros(slab_sizes, cfg_like_(input));
    const Series right = pluss::zeros(slab_sizes, cfg_like_(input));
    padded = left.unite(std::vector<Series>{padded, right}, 4);
  }
  return padded;
}

inline std::int64_t output_extent_(std::int64_t input, std::int64_t kernel, std::int64_t stride, std::int64_t dilation) {
  return (input - dilation * (kernel - 1) - 1) / stride + 1;
}

inline Series add_bias_(Series output, const Series& bias) {
  if (!bias.defined()) {
    return output;
  }
  const Series shaped = bias.reshape({1, bias.sizes()[0], 1, 1});
  return output.add(shaped);
}

inline Series add_bias_3d_(Series output, const Series& bias) {
  if (!bias.defined()) {
    return output;
  }
  const Series shaped = bias.reshape({1, bias.sizes()[0], 1, 1, 1});
  return output.add(shaped);
}

inline Series conv2d_impl_(const Series& input,
                           const Series& weight,
                           const Series& bias,
                           const Pair& stride,
                           const Pair& padding,
                           const Pair& dilation,
                           std::int64_t groups) {
  validate_pair_(stride, "conv stride");
  validate_pair_(padding, "conv padding", true);
  validate_pair_(dilation, "conv dilation");
  ::pluss::detail::nn::require_rank_at_least_(input, 3, "conv");
  ::pluss::detail::nn::require_rank_at_least_(weight, 4, "conv");
  PLUSS_CHECK(groups > 0, "pluss::ai::conv: groups must be > 0");

  const bool batched = input.dim() == 4;
  const Series x = batched ? input : input.unsqueeze(0);
  PLUSS_CHECK(x.dim() == 4, "pluss::ai::conv: expected input with shape [N,C,H,W] or [C,H,W]");

  const Series w = cast_like_(weight, x);
  const Series b = bias.defined() ? cast_like_(bias, x) : Series();

  const std::int64_t batch = x.sizes()[0];
  const std::int64_t in_channels = x.sizes()[1];
  const std::int64_t out_channels = w.sizes()[0];
  const std::int64_t channels_per_group = w.sizes()[1];
  const std::int64_t kernel_h = w.sizes()[2];
  const std::int64_t kernel_w = w.sizes()[3];

  PLUSS_CHECK(in_channels % groups == 0, "pluss::ai::conv: input channels must be divisible by groups");
  PLUSS_CHECK(out_channels % groups == 0, "pluss::ai::conv: output channels must be divisible by groups");
  PLUSS_CHECK(channels_per_group * groups == in_channels, "pluss::ai::conv: weight input channels mismatch");
  if (b.defined()) {
    PLUSS_CHECK(b.dim() == 1 && b.sizes()[0] == out_channels, "pluss::ai::conv: bias size mismatch");
  }

  const bool require_backward =
      ::pluss::codem::series_tape::recording_enabled() &&
      (x.tracks_slope() || w.tracks_slope());
  if (x.platform() == pluss::pCUDA) {
    const auto y = ::pluss::ai::kernels::cudnn::try_conv2d(
        x,
        w,
        b,
        ::pluss::ai::kernels::cudnn::Conv2dGeometry{
            .stride = stride,
            .padding = padding,
            .dilation = dilation,
            .groups = groups,
        },
        require_backward);
    if (y.has_value()) {
      return batched ? *y : y->squeeze(0);
    }
  }
  if (x.platform() == pluss::pMPS) {
    const auto mps_conv_cpu_bridge_enabled_ = []() noexcept {
      const char* raw = std::getenv("PLUSS_MPS_CONV_CPU_BRIDGE");
      if (raw == nullptr || *raw == '\0') return true;
      auto eq_lit = [&](const char* lit) noexcept {
        const char* a = raw;
        const char* b = lit;
        while (*a != '\0' && *b != '\0') {
          if (std::tolower(static_cast<unsigned char>(*a)) !=
              std::tolower(static_cast<unsigned char>(*b))) {
            return false;
          }
          ++a;
          ++b;
        }
        return *a == '\0' && *b == '\0';
      };
      return eq_lit("1") || eq_lit("true") || eq_lit("yes") || eq_lit("on");
    }();
    if (mps_conv_cpu_bridge_enabled_) {
      const Series x_cpu = x.to(pluss::pCPU, x.scalar_type());
      const Series w_cpu = w.to(pluss::pCPU, w.scalar_type());
      const Series b_cpu = b.defined() ? b.to(pluss::pCPU, b.scalar_type()) : Series();
      const Series out_cpu = conv2d_impl_(x_cpu, w_cpu, b_cpu, stride, padding, dilation, groups);
      const Series out = out_cpu.to(x.platform(), x.scalar_type());
      return batched ? out : out.squeeze(0);
    }
  }

  const Series padded = pad_2d_(x, padding);
  const std::int64_t padded_h = padded.sizes()[2];
  const std::int64_t padded_w = padded.sizes()[3];
  const std::int64_t effective_kh = dilation[0] * (kernel_h - 1) + 1;
  const std::int64_t effective_kw = dilation[1] * (kernel_w - 1) + 1;
  PLUSS_CHECK(padded_h >= effective_kh && padded_w >= effective_kw, "pluss::ai::conv: kernel larger than padded input");

  const std::int64_t out_h = output_extent_(padded_h, kernel_h, stride[0], dilation[0]);
  const std::int64_t out_w = output_extent_(padded_w, kernel_w, stride[1], dilation[1]);

  Series patches = padded.unfold(2, effective_kh, stride[0]).unfold(4, effective_kw, stride[1]);
  if (dilation[0] > 1) {
    std::vector<std::int64_t> kh_idx(static_cast<std::size_t>(kernel_h));
    for (std::int64_t i = 0; i < kernel_h; ++i) kh_idx[static_cast<std::size_t>(i)] = i * dilation[0];
    const Series idx = pluss::make_series(kh_idx, {kernel_h}, Config{}.with_dtype(ScalarType::Int64).with_platform(patches.platform()));
    patches = patches.index_select(3, idx);
  }
  if (dilation[1] > 1) {
    std::vector<std::int64_t> kw_idx(static_cast<std::size_t>(kernel_w));
    for (std::int64_t i = 0; i < kernel_w; ++i) kw_idx[static_cast<std::size_t>(i)] = i * dilation[1];
    const Series idx = pluss::make_series(kw_idx, {kernel_w}, Config{}.with_dtype(ScalarType::Int64).with_platform(patches.platform()));
    patches = patches.index_select(5, idx);
  }

  const std::array<std::int64_t, 6> permute_order{0, 2, 4, 1, 3, 5};
  const Series cols =
      patches.permute(std::span<const std::int64_t>(permute_order.data(), permute_order.size()))
          .contiguous()
          .reshape({batch * out_h * out_w, in_channels * kernel_h * kernel_w});
  const Series flat_weight = w.reshape({out_channels, channels_per_group * kernel_h * kernel_w});
  const std::int64_t out_per_group = out_channels / groups;
  const std::int64_t col_per_group = channels_per_group * kernel_h * kernel_w;

  std::vector<Series> group_outputs;
  group_outputs.reserve(static_cast<std::size_t>(groups));
  for (std::int64_t group = 0; group < groups; ++group) {
    const Series group_cols = ::pluss::detail::nn::narrow_dim_(cols, 1, group * col_per_group, col_per_group);
    const Series group_weight = ::pluss::detail::nn::narrow_dim_(flat_weight, 0, group * out_per_group, out_per_group);
    group_outputs.push_back(group_cols.matmul(group_weight.t()));
  }

  Series flat_output = group_outputs.size() == 1
                           ? group_outputs.front()
                           : group_outputs.front().unite(std::vector<Series>(group_outputs.begin() + 1, group_outputs.end()), 1);
  const std::array<std::int64_t, 4> output_order{0, 3, 1, 2};
  Series output = flat_output.reshape({batch, out_h, out_w, out_channels})
                      .permute(std::span<const std::int64_t>(output_order.data(), output_order.size()));
  output = add_bias_(output, b);
  return batched ? output : output.squeeze(0);
}

inline Series conv3d_impl_(const Series& input,
                           const Series& weight,
                           const Series& bias,
                           const Triplet& stride,
                           const Triplet& padding,
                           const Triplet& dilation,
                           std::int64_t groups) {
  validate_triplet_(stride, "conv3d stride");
  validate_triplet_(padding, "conv3d padding", true);
  validate_triplet_(dilation, "conv3d dilation");
  ::pluss::detail::nn::require_rank_at_least_(input, 4, "conv3d");
  ::pluss::detail::nn::require_rank_at_least_(weight, 5, "conv3d");
  PLUSS_CHECK(groups > 0, "pluss::ai::conv3d: groups must be > 0");

  const bool batched = input.dim() == 5;
  const Series x = batched ? input : input.unsqueeze(0);
  PLUSS_CHECK(x.dim() == 5, "pluss::ai::conv3d: expected input with shape [N,C,D,H,W] or [C,D,H,W]");

  const Series w = cast_like_(weight, x);
  const Series b = bias.defined() ? cast_like_(bias, x) : Series();

  const std::int64_t batch = x.sizes()[0];
  const std::int64_t in_channels = x.sizes()[1];
  const std::int64_t out_channels = w.sizes()[0];
  const std::int64_t channels_per_group = w.sizes()[1];
  const std::int64_t kernel_d = w.sizes()[2];
  const std::int64_t kernel_h = w.sizes()[3];
  const std::int64_t kernel_w = w.sizes()[4];

  PLUSS_CHECK(in_channels % groups == 0, "pluss::ai::conv3d: input channels must be divisible by groups");
  PLUSS_CHECK(out_channels % groups == 0, "pluss::ai::conv3d: output channels must be divisible by groups");
  PLUSS_CHECK(channels_per_group * groups == in_channels, "pluss::ai::conv3d: weight input channels mismatch");
  if (b.defined()) {
    PLUSS_CHECK(b.dim() == 1 && b.sizes()[0] == out_channels, "pluss::ai::conv3d: bias size mismatch");
  }

  const bool require_backward =
      ::pluss::codem::series_tape::recording_enabled() &&
      (x.tracks_slope() || w.tracks_slope() || (b.defined() && b.tracks_slope()));
  if (x.platform() == pluss::pMPS) {
    const auto routed = ::pluss::ai::kernels::mps::try_conv3d(x, w, b, stride, padding, dilation, groups, require_backward);
    if (routed.has_value()) {
      return batched ? *routed : routed->squeeze(0);
    }
  }

  const Series padded = pad_3d_(x, padding);
  const std::int64_t padded_d = padded.sizes()[2];
  const std::int64_t padded_h = padded.sizes()[3];
  const std::int64_t padded_w = padded.sizes()[4];
  const std::int64_t effective_kd = dilation[0] * (kernel_d - 1) + 1;
  const std::int64_t effective_kh = dilation[1] * (kernel_h - 1) + 1;
  const std::int64_t effective_kw = dilation[2] * (kernel_w - 1) + 1;
  PLUSS_CHECK(
      padded_d >= effective_kd && padded_h >= effective_kh && padded_w >= effective_kw,
      "pluss::ai::conv3d: kernel larger than padded input");

  const std::int64_t out_d = output_extent_(padded_d, kernel_d, stride[0], dilation[0]);
  const std::int64_t out_h = output_extent_(padded_h, kernel_h, stride[1], dilation[1]);
  const std::int64_t out_w = output_extent_(padded_w, kernel_w, stride[2], dilation[2]);

  std::vector<Series> depth_outputs;
  depth_outputs.reserve(static_cast<std::size_t>(out_d));
  for (std::int64_t od = 0; od < out_d; ++od) {
    Series depth_accum;
    for (std::int64_t kd = 0; kd < kernel_d; ++kd) {
      const std::int64_t src_depth = od * stride[0] + kd * dilation[0];
      const Series x_slice = padded.select(2, src_depth).contiguous();
      const Series w_slice = w.select(2, kd).contiguous();
      const Series plane = conv2d_impl_(
          x_slice,
          w_slice,
          Series(),
          Pair{stride[1], stride[2]},
          Pair{0, 0},
          Pair{dilation[1], dilation[2]},
          groups);
      depth_accum = depth_accum.defined() ? depth_accum.add(plane) : plane;
    }
    depth_outputs.push_back(depth_accum.unsqueeze(2));
  }

  Series output = depth_outputs.front();
  if (depth_outputs.size() > 1) {
    output = output.unite(std::vector<Series>(depth_outputs.begin() + 1, depth_outputs.end()), 2);
  }
  output = add_bias_3d_(output, b);
  return batched ? output : output.squeeze(0);
}

} // namespace pluss::ai::detail::conv

namespace pluss::ai {

inline Series conv(const Series& input,
                   const Series& weight,
                   const Series& bias = Series(),
                   std::array<std::int64_t, 2> stride = {1, 1},
                   std::array<std::int64_t, 2> padding = {0, 0},
                   std::array<std::int64_t, 2> dilation = {1, 1},
                   std::int64_t groups = 1) {
  return detail::conv::conv2d_impl_(input, weight, bias, stride, padding, dilation, groups);
}

inline Series conv(const Series& input,
                   const Series& weight,
                   const Series& bias,
                   std::int64_t stride,
                   std::int64_t padding = 0,
                   std::int64_t dilation = 1,
                   std::int64_t groups = 1) {
  return conv(input, weight, bias, {stride, stride}, {padding, padding}, {dilation, dilation}, groups);
}

inline Series conv2d(const Series& input,
                     const Series& weight,
                     const Series& bias = Series(),
                     std::array<std::int64_t, 2> stride = {1, 1},
                     std::array<std::int64_t, 2> padding = {0, 0},
                     std::array<std::int64_t, 2> dilation = {1, 1},
                     std::int64_t groups = 1) {
  return conv(input, weight, bias, stride, padding, dilation, groups);
}

inline Series conv2d(const Series& input,
                     const Series& weight,
                     const Series& bias,
                     std::int64_t stride,
                     std::int64_t padding = 0,
                     std::int64_t dilation = 1,
                     std::int64_t groups = 1) {
  return conv(input, weight, bias, stride, padding, dilation, groups);
}

inline Series conv3d(const Series& input,
                     const Series& weight,
                     const Series& bias = Series(),
                     std::array<std::int64_t, 3> stride = {1, 1, 1},
                     std::array<std::int64_t, 3> padding = {0, 0, 0},
                     std::array<std::int64_t, 3> dilation = {1, 1, 1},
                     std::int64_t groups = 1) {
  return detail::conv::conv3d_impl_(input, weight, bias, stride, padding, dilation, groups);
}

inline Series conv3d(const Series& input,
                     const Series& weight,
                     const Series& bias,
                     std::int64_t stride,
                     std::int64_t padding = 0,
                     std::int64_t dilation = 1,
                     std::int64_t groups = 1) {
  return conv3d(input, weight, bias, {stride, stride, stride}, {padding, padding, padding}, {dilation, dilation, dilation}, groups);
}

} // namespace pluss::ai

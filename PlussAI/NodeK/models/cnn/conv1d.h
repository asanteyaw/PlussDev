#pragma once

#include "convNd.h"

namespace pluss::ai {

using Padding1d = std::array<std::int64_t, 2>;

inline Padding1d causal_padding1d(std::int64_t kernel_size, std::int64_t dilation = 1) {
  PLUSS_CHECK(kernel_size > 0, "pluss::ai::causal_padding1d: kernel_size must be > 0");
  PLUSS_CHECK(dilation > 0, "pluss::ai::causal_padding1d: dilation must be > 0");
  return {dilation * (kernel_size - 1), 0};
}

namespace detail::conv1d {

inline void validate_padding_(const Padding1d& padding) {
  PLUSS_CHECK(padding[0] >= 0 && padding[1] >= 0, "pluss::ai::conv1d: padding values must be >= 0");
}

inline Series zero_pad_(const Series& input, const Padding1d& padding) {
  validate_padding_(padding);
  if (padding[0] == 0 && padding[1] == 0) {
    return input;
  }

  std::vector<Series> pieces;
  pieces.reserve(3);
  const Config cfg = Config{}.with_dtype(input.scalar_type()).with_platform(input.platform());
  if (padding[0] > 0) {
    pieces.push_back(pluss::series::zeros({input.sizes()[0], input.sizes()[1], padding[0]}, cfg));
  }
  pieces.push_back(input);
  if (padding[1] > 0) {
    pieces.push_back(pluss::series::zeros({input.sizes()[0], input.sizes()[1], padding[1]}, cfg));
  }
  return pluss::series::unite(pieces, 2);
}

} // namespace detail::conv1d

inline Series conv1d(const Series& input,
                     const Series& weight,
                     const Series& bias = Series(),
                     Padding1d padding = {0, 0},
                     std::int64_t stride = 1,
                     std::int64_t dilation = 1,
                     std::int64_t groups = 1) {
  PLUSS_CHECK(stride > 0, "pluss::ai::conv1d: stride must be > 0");
  detail::conv1d::validate_padding_(padding);
  PLUSS_CHECK(dilation > 0, "pluss::ai::conv1d: dilation must be > 0");
  ::pluss::detail::nn::require_rank_at_least_(input, 2, "conv1d");
  ::pluss::detail::nn::require_rank_at_least_(weight, 3, "conv1d");

  const bool batched = input.dim() == 3;
  const Series x = batched ? input : input.unsqueeze(0);
  PLUSS_CHECK(x.dim() == 3, "pluss::ai::conv1d: expected input with shape [N,C,L] or [C,L]");
  PLUSS_CHECK(weight.dim() == 3, "pluss::ai::conv1d: expected weight with shape [O,I,K]");

  const Series padded = detail::conv1d::zero_pad_(x, padding);
  const Series x2 = padded.unsqueeze(3);
  const Series w2 = weight.unsqueeze(3);
  const Series out2 = ::pluss::ai::conv(
      x2,
      w2,
      bias,
      std::array<std::int64_t, 2>{stride, 1},
      std::array<std::int64_t, 2>{0, 0},
      std::array<std::int64_t, 2>{dilation, 1},
      groups);
  const Series out = out2.squeeze(3);
  return batched ? out : out.squeeze(0);
}

inline Series conv1d(const Series& input,
                     const Series& weight,
                     const Series& bias,
                     std::int64_t stride = 1,
                     std::int64_t padding = 0,
                     std::int64_t dilation = 1,
                     std::int64_t groups = 1) {
  PLUSS_CHECK(stride > 0, "pluss::ai::conv1d: stride must be > 0");
  PLUSS_CHECK(padding >= 0, "pluss::ai::conv1d: padding must be >= 0");
  return conv1d(input, weight, bias, Padding1d{padding, padding}, stride, dilation, groups);
}

} // namespace pluss::ai

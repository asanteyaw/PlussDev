#pragma once

#include "../Module.h"
#include <pluss/ai/conv.h>

#include <array>
#include <cmath>
#include <utility>

namespace pluss::ai::nn {

struct ConvOptions {
  std::int64_t in_channels = 1;
  std::int64_t out_channels = 1;
  std::array<std::int64_t, 2> kernel_size{3, 3};
  std::array<std::int64_t, 2> stride{1, 1};
  std::array<std::int64_t, 2> padding{0, 0};
  std::array<std::int64_t, 2> dilation{1, 1};
  std::int64_t groups = 1;
  bool bias = true;
  Config cfg = ::pluss::detail::nn::default_cfg_();
  std::uint64_t seed = 0;
};

class PLUSS_API ConvImpl final : public Module {
public:
  explicit ConvImpl(ConvOptions options) : Module(true), options_(std::move(options)) {
    register_parameters_();
    reset_parameters();
  }
  ConvImpl(std::int64_t in_channels,
           std::int64_t out_channels,
           std::array<std::int64_t, 2> kernel_size,
           std::array<std::int64_t, 2> stride = {1, 1},
           std::array<std::int64_t, 2> padding = {0, 0},
           std::array<std::int64_t, 2> dilation = {1, 1},
           std::int64_t groups = 1,
           bool bias = true,
           Config cfg = ::pluss::detail::nn::default_cfg_(),
           std::uint64_t seed = 0)
      : ConvImpl(ConvOptions{in_channels, out_channels, kernel_size, stride, padding, dilation, groups, bias, cfg, seed}) {}

  void reset_parameters() {
    PLUSS_CHECK(options_.in_channels % options_.groups == 0, "pluss::ai::nn::Conv: in_channels must be divisible by groups");
    const Config cfg = ::pluss::detail::nn::floating_cfg_(options_.cfg);
    const std::int64_t group_in = options_.in_channels / options_.groups;
    const double fan_in = static_cast<double>(group_in * options_.kernel_size[0] * options_.kernel_size[1]);
    const double bound = 1.0 / std::sqrt(fan_in);
    weight_ = ::pluss::detail::nn::uniform_(
        {options_.out_channels, group_in, options_.kernel_size[0], options_.kernel_size[1]},
        -bound,
        bound,
        cfg,
        options_.seed);
    bias_ = options_.bias ? ::pluss::detail::nn::uniform_({options_.out_channels}, -bound, bound, cfg, options_.seed + 1ULL) : Series();
  }

  Series forward(const Series& input) const {
    return pluss::ai::conv(input, weight_, bias_, options_.stride, options_.padding, options_.dilation, options_.groups);
  }

  Series operator()(const Series& input) const { return forward(input); }
  ConvImpl* operator->() noexcept { return this; }
  const ConvImpl* operator->() const noexcept { return this; }

  Series& weight() noexcept { return weight_; }
  const Series& weight() const noexcept { return weight_; }
  Series& bias() noexcept { return bias_; }
  const Series& bias() const noexcept { return bias_; }
  ConvOptions& options() noexcept { return options_; }
  const ConvOptions& options() const noexcept { return options_; }

private:
  void register_parameters_() {
    this->clear_parameter_records_();
    this->parameter_record("weight", weight_);
    this->parameter_record("bias", bias_);
  }

  void on_platform_changed_(Platform platform) override { options_.cfg = options_.cfg.with_platform(platform); }
  void on_dtype_changed_(ScalarType dtype) override { options_.cfg = options_.cfg.with_dtype(dtype); }

  ConvOptions options_;
  Series weight_;
  Series bias_;
};

using Conv2dOptions = ConvOptions;
PLUSS_MODULE_IMPL(Conv, ConvImpl);
using Conv2d = Conv;
using ConvNdOptions = ConvOptions;
using ConvNd = Conv;

} // namespace pluss::ai::nn

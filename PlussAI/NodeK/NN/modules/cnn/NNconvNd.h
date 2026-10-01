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
    // register_parameters_() allocates real (defined) storage for
    // weight_/bias_ and registers each immediately after -- see
    // FusionImpl's identical fix and Module::parameter_record()'s contract.
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

  // Re-randomizes weight/bias values in place. Never reassigns weight_/
  // bias_, so it's safe to call any number of times after construction --
  // registration and tracks_slope_ (set once, in register_parameters_())
  // are never disturbed.
  void reset_parameters() {
    const Config cfg = ::pluss::detail::nn::floating_cfg_(options_.cfg);
    const std::int64_t group_in = weight_.sizes()[1];
    const double fan_in = static_cast<double>(group_in * options_.kernel_size[0] * options_.kernel_size[1]);
    const double bound = 1.0 / std::sqrt(fan_in);
    weight_.copy_(::pluss::detail::nn::uniform_(weight_.sizes(), -bound, bound, cfg, options_.seed));
    if (options_.bias) {
      bias_.copy_(::pluss::detail::nn::uniform_({options_.out_channels}, -bound, bound, cfg, options_.seed + 1ULL));
    }
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
    PLUSS_CHECK(options_.kernel_size[0] > 0 && options_.kernel_size[1] > 0,
                "pluss::ai::nn::Conv: kernel_size values must be > 0");
    PLUSS_CHECK(options_.stride[0] > 0 && options_.stride[1] > 0,
                "pluss::ai::nn::Conv: stride values must be > 0");
    PLUSS_CHECK(options_.padding[0] >= 0 && options_.padding[1] >= 0,
                "pluss::ai::nn::Conv: padding values must be >= 0");
    PLUSS_CHECK(options_.dilation[0] > 0 && options_.dilation[1] > 0,
                "pluss::ai::nn::Conv: dilation values must be > 0");
    PLUSS_CHECK(options_.groups > 0, "pluss::ai::nn::Conv: groups must be > 0");
    PLUSS_CHECK(options_.in_channels % options_.groups == 0, "pluss::ai::nn::Conv: in_channels must be divisible by groups");
    const Config cfg = ::pluss::detail::nn::floating_cfg_(options_.cfg);
    const std::int64_t group_in = options_.in_channels / options_.groups;
    weight_ = Series::empty(
        {options_.out_channels, group_in, options_.kernel_size[0], options_.kernel_size[1]}, cfg.dtype, cfg.platform);
    this->parameter_record("weight", weight_);
    if (options_.bias) {
      bias_ = Series::empty({options_.out_channels}, cfg.dtype, cfg.platform);
      this->parameter_record("bias", bias_);
    }
  }

  void on_platform_changed_(Platform platform) override { options_.cfg = options_.cfg.with_platform(platform); }
  void on_dtype_changed_(ScalarType dtype) override { options_.cfg = options_.cfg.with_dtype(dtype); }

  ConvOptions options_;
  Series weight_ = pluss::series::undefined();
  Series bias_ = pluss::series::undefined();
};

using Conv2dOptions = ConvOptions;
PLUSS_MODULE_IMPL(Conv, ConvImpl);
using Conv2d = Conv;
using ConvNdOptions = ConvOptions;
using ConvNd = Conv;

} // namespace pluss::ai::nn

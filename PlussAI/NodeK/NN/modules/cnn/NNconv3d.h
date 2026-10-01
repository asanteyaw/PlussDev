#pragma once

#include "../Module.h"
#include <pluss/ai/conv.h>

#include <array>
#include <cmath>
#include <utility>

namespace pluss::ai::nn {

struct Conv3dOptions {
  std::int64_t in_channels = 1;
  std::int64_t out_channels = 1;
  std::array<std::int64_t, 3> kernel_size{3, 3, 3};
  std::array<std::int64_t, 3> stride{1, 1, 1};
  std::array<std::int64_t, 3> padding{0, 0, 0};
  std::array<std::int64_t, 3> dilation{1, 1, 1};
  std::int64_t groups = 1;
  bool bias = true;
  Config cfg = ::pluss::detail::nn::default_cfg_();
  std::uint64_t seed = 0;
};

class PLUSS_API Conv3dImpl final : public Module {
public:
  explicit Conv3dImpl(Conv3dOptions options) : Module(true), options_(std::move(options)) {
    // register_parameters_() allocates real (defined) storage for
    // weight_/bias_ and registers each immediately after -- see
    // FusionImpl's identical fix and Module::parameter_record()'s contract.
    register_parameters_();
    reset_parameters();
  }

  Conv3dImpl(std::int64_t in_channels,
             std::int64_t out_channels,
             std::array<std::int64_t, 3> kernel_size,
             std::array<std::int64_t, 3> stride = {1, 1, 1},
             std::array<std::int64_t, 3> padding = {0, 0, 0},
             std::array<std::int64_t, 3> dilation = {1, 1, 1},
             std::int64_t groups = 1,
             bool bias = true,
             Config cfg = ::pluss::detail::nn::default_cfg_(),
             std::uint64_t seed = 0)
      : Conv3dImpl(Conv3dOptions{
            .in_channels = in_channels,
            .out_channels = out_channels,
            .kernel_size = kernel_size,
            .stride = stride,
            .padding = padding,
            .dilation = dilation,
            .groups = groups,
            .bias = bias,
            .cfg = cfg,
            .seed = seed,
        }) {}

  // Re-randomizes weight/bias values in place. Never reassigns weight_/
  // bias_, so it's safe to call any number of times after construction --
  // registration and tracks_slope_ (set once, in register_parameters_())
  // are never disturbed.
  void reset_parameters() {
    const Config cfg = ::pluss::detail::nn::floating_cfg_(options_.cfg);
    const std::int64_t group_in = weight_.sizes()[1];
    const double fan_in = static_cast<double>(group_in * options_.kernel_size[0] * options_.kernel_size[1] * options_.kernel_size[2]);
    const double bound = 1.0 / std::sqrt(fan_in);
    weight_.copy_(::pluss::detail::nn::uniform_(weight_.sizes(), -bound, bound, cfg, options_.seed));
    if (options_.bias) {
      bias_.copy_(::pluss::detail::nn::uniform_({options_.out_channels}, -bound, bound, cfg, options_.seed + 1ULL));
    }
  }

  Series forward(const Series& input) const {
    return pluss::ai::conv3d(input, weight_, bias_, options_.stride, options_.padding, options_.dilation, options_.groups);
  }

  Series operator()(const Series& input) const { return forward(input); }
  Conv3dImpl* operator->() noexcept { return this; }
  const Conv3dImpl* operator->() const noexcept { return this; }

  Series& weight() noexcept { return weight_; }
  const Series& weight() const noexcept { return weight_; }
  Series& bias() noexcept { return bias_; }
  const Series& bias() const noexcept { return bias_; }
  Conv3dOptions& options() noexcept { return options_; }
  const Conv3dOptions& options() const noexcept { return options_; }

private:
  void register_parameters_() {
    this->clear_parameter_records_();
    PLUSS_CHECK(options_.kernel_size[0] > 0 && options_.kernel_size[1] > 0 && options_.kernel_size[2] > 0,
                "pluss::ai::nn::Conv3d: kernel_size values must be > 0");
    PLUSS_CHECK(options_.stride[0] > 0 && options_.stride[1] > 0 && options_.stride[2] > 0,
                "pluss::ai::nn::Conv3d: stride values must be > 0");
    PLUSS_CHECK(options_.padding[0] >= 0 && options_.padding[1] >= 0 && options_.padding[2] >= 0,
                "pluss::ai::nn::Conv3d: padding values must be >= 0");
    PLUSS_CHECK(options_.dilation[0] > 0 && options_.dilation[1] > 0 && options_.dilation[2] > 0,
                "pluss::ai::nn::Conv3d: dilation values must be > 0");
    PLUSS_CHECK(options_.groups > 0, "pluss::ai::nn::Conv3d: groups must be > 0");
    PLUSS_CHECK(options_.in_channels % options_.groups == 0, "pluss::ai::nn::Conv3d: in_channels must be divisible by groups");

    const Config cfg = ::pluss::detail::nn::floating_cfg_(options_.cfg);
    const std::int64_t group_in = options_.in_channels / options_.groups;
    weight_ = Series::empty(
        {options_.out_channels, group_in, options_.kernel_size[0], options_.kernel_size[1], options_.kernel_size[2]},
        cfg.dtype, cfg.platform);
    this->parameter_record("weight", weight_);
    if (options_.bias) {
      bias_ = Series::empty({options_.out_channels}, cfg.dtype, cfg.platform);
      this->parameter_record("bias", bias_);
    }
  }

  void on_platform_changed_(Platform platform) override { options_.cfg = options_.cfg.with_platform(platform); }
  void on_dtype_changed_(ScalarType dtype) override { options_.cfg = options_.cfg.with_dtype(dtype); }

  Conv3dOptions options_;
  Series weight_ = pluss::series::undefined();
  Series bias_ = pluss::series::undefined();
};

PLUSS_MODULE_IMPL(Conv3d, Conv3dImpl);

} // namespace pluss::ai::nn

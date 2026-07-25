#pragma once

#include "../Module.h"
#include <pluss/ai/conv.h>

#include <cmath>
#include <utility>

namespace pluss::ai::nn {

struct Conv1dOptions {
  std::int64_t in_channels = 1;
  std::int64_t out_channels = 1;
  std::int64_t kernel_size = 3;
  std::int64_t stride = 1;
  std::int64_t padding = 0;
  Padding1d padding_lr{0, 0};
  std::int64_t dilation = 1;
  std::int64_t groups = 1;
  bool bias = true;
  Config cfg = ::pluss::detail::nn::default_cfg_();
  std::uint64_t seed = 0;
};

class PLUSS_API Conv1dImpl final : public Module {
public:
  explicit Conv1dImpl(Conv1dOptions options) : Module(true), options_(std::move(options)) {
    // register_parameters_() allocates real (defined) storage for
    // weight_/bias_ and registers each immediately after -- see
    // FusionImpl's identical fix and Module::parameter_record()'s contract.
    register_parameters_();
    reset_parameters();
  }
  Conv1dImpl(std::int64_t in_channels,
             std::int64_t out_channels,
             std::int64_t kernel_size,
             std::int64_t stride = 1,
             std::int64_t padding = 0,
             std::int64_t dilation = 1,
             std::int64_t groups = 1,
             bool bias = true,
             Config cfg = ::pluss::detail::nn::default_cfg_(),
             std::uint64_t seed = 0)
      : Conv1dImpl(Conv1dOptions{
            .in_channels = in_channels,
            .out_channels = out_channels,
            .kernel_size = kernel_size,
            .stride = stride,
            .padding = padding,
            .padding_lr = {0, 0},
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
    const double fan_in = static_cast<double>(group_in * options_.kernel_size);
    const double bound = 1.0 / std::sqrt(fan_in);
    weight_.copy_(::pluss::detail::nn::uniform_(weight_.sizes(), -bound, bound, cfg, options_.seed));
    if (options_.bias) {
      bias_.copy_(::pluss::detail::nn::uniform_({options_.out_channels}, -bound, bound, cfg, options_.seed + 1ULL));
    }
  }

  Padding1d padding() const noexcept {
    if (options_.padding_lr[0] != 0 || options_.padding_lr[1] != 0) {
      return options_.padding_lr;
    }
    return {options_.padding, options_.padding};
  }

  Series forward(const Series& input) const {
    return pluss::ai::conv1d(input, weight_, bias_, padding(), options_.stride, options_.dilation, options_.groups);
  }

  Series operator()(const Series& input) const { return forward(input); }
  Conv1dImpl* operator->() noexcept { return this; }
  const Conv1dImpl* operator->() const noexcept { return this; }

  Series& weight() noexcept { return weight_; }
  const Series& weight() const noexcept { return weight_; }
  Series& bias() noexcept { return bias_; }
  const Series& bias() const noexcept { return bias_; }
  Conv1dOptions& options() noexcept { return options_; }
  const Conv1dOptions& options() const noexcept { return options_; }

private:
  void register_parameters_() {
    this->clear_parameter_records_();
    PLUSS_CHECK(options_.kernel_size > 0, "pluss::ai::nn::Conv1d: kernel_size must be > 0");
    PLUSS_CHECK(options_.stride > 0, "pluss::ai::nn::Conv1d: stride must be > 0");
    PLUSS_CHECK(options_.padding >= 0, "pluss::ai::nn::Conv1d: padding must be >= 0");
    PLUSS_CHECK(options_.padding_lr[0] >= 0 && options_.padding_lr[1] >= 0, "pluss::ai::nn::Conv1d: padding_lr values must be >= 0");
    PLUSS_CHECK(options_.dilation > 0, "pluss::ai::nn::Conv1d: dilation must be > 0");
    PLUSS_CHECK(options_.groups > 0, "pluss::ai::nn::Conv1d: groups must be > 0");
    PLUSS_CHECK(options_.in_channels % options_.groups == 0, "pluss::ai::nn::Conv1d: in_channels must be divisible by groups");
    const Config cfg = ::pluss::detail::nn::floating_cfg_(options_.cfg);
    const std::int64_t group_in = options_.in_channels / options_.groups;
    weight_ = Series::empty({options_.out_channels, group_in, options_.kernel_size}, cfg.dtype, cfg.platform);
    this->parameter_record("weight", weight_);
    if (options_.bias) {
      bias_ = Series::empty({options_.out_channels}, cfg.dtype, cfg.platform);
      this->parameter_record("bias", bias_);
    }
  }

  void on_platform_changed_(Platform platform) override { options_.cfg = options_.cfg.with_platform(platform); }
  void on_dtype_changed_(ScalarType dtype) override { options_.cfg = options_.cfg.with_dtype(dtype); }

  Conv1dOptions options_;
  Series weight_;
  Series bias_;
};

PLUSS_MODULE_IMPL(Conv1d, Conv1dImpl);

} // namespace pluss::ai::nn

#pragma once

#include "../Module.h"
#include <pluss/ai/fusion.h>

#include <cmath>
#include <utility>

namespace pluss::ai::nn {

struct FusionOptions {
  std::int64_t in_features = 1;
  std::int64_t out_features = 1;
  bool bias = true;
  Config cfg = ::pluss::detail::nn::default_cfg_();
  std::uint64_t seed = 0;
};

class PLUSS_API FusionImpl final : public Module {
public:
  explicit FusionImpl(FusionOptions options) : Module(true), options_(std::move(options)) {
    // register_parameters_() allocates real (defined) storage for
    // weight_/bias_ and registers each immediately after -- allocation and
    // registration happen together, per parameter, so there is no window
    // where a parameter is registered before it exists (see
    // Module::parameter_record()'s contract). reset_parameters() then only
    // fills in values, in place, and is safe to call again later.
    register_parameters_();
    reset_parameters();
  }
  FusionImpl(std::int64_t in_features, std::int64_t out_features, bool bias = true, Config cfg = ::pluss::detail::nn::default_cfg_(), std::uint64_t seed = 0)
      : FusionImpl(FusionOptions{in_features, out_features, bias, cfg, seed}) {}

  // Re-randomizes weight/bias values in place. Never reassigns weight_/
  // bias_, so it's safe to call any number of times after construction --
  // registration and tracks_slope_ (set once, in register_parameters_())
  // are never disturbed.
  void reset_parameters() {
    const Config cfg = ::pluss::detail::nn::floating_cfg_(options_.cfg);
    const double bound = 1.0 / std::sqrt(static_cast<double>(options_.in_features));
    weight_.copy_(::pluss::detail::nn::uniform_({options_.out_features, options_.in_features}, -bound, bound, cfg, options_.seed));
    if (options_.bias) {
      bias_.copy_(::pluss::detail::nn::uniform_({options_.out_features}, -bound, bound, cfg, options_.seed + 1ULL));
    }
  }

  Series forward(const Series& input) const { return pluss::ai::ff(input, weight_, bias_); }
  Series operator()(const Series& input) const { return forward(input); }
  FusionImpl* operator->() noexcept { return this; }
  const FusionImpl* operator->() const noexcept { return this; }

  Series& weight() noexcept { return weight_; }
  const Series& weight() const noexcept { return weight_; }
  Series& bias() noexcept { return bias_; }
  const Series& bias() const noexcept { return bias_; }
  FusionOptions& options() noexcept { return options_; }
  const FusionOptions& options() const noexcept { return options_; }

private:
  void register_parameters_() {
    this->clear_parameter_records_();
    const Config cfg = ::pluss::detail::nn::floating_cfg_(options_.cfg);
    weight_ = Series::empty({options_.out_features, options_.in_features}, cfg.dtype, cfg.platform);
    this->parameter_record("weight", weight_);
    if (options_.bias) {
      bias_ = Series::empty({options_.out_features}, cfg.dtype, cfg.platform);
      this->parameter_record("bias", bias_);
    }
  }

  void on_platform_changed_(Platform platform) override { options_.cfg = options_.cfg.with_platform(platform); }
  void on_dtype_changed_(ScalarType dtype) override { options_.cfg = options_.cfg.with_dtype(dtype); }

  FusionOptions options_;
  Series weight_;
  Series bias_;
};

PLUSS_MODULE_IMPL(Fusion, FusionImpl);

} // namespace pluss::ai::nn

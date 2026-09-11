#pragma once

#include "../Module.h"

#include <utility>

namespace pluss::ai::nn {

struct LayerNormOptions {
  std::int64_t normalized_size = 0;
  double eps = 1e-5;
  Config cfg = ::pluss::detail::nn::default_cfg_();
};

// LayerNorm over the last dimension: (x - mean) / sqrt(var + eps) * weight
// + bias, mean/var computed per-row over the last `normalized_size`
// elements. Promoted out of ice::models::TemporalFusionTransformer's
// originally-private building blocks (Ice/models/ts/tft_blocks.h) once a
// second consumer needed it -- see ICE_BLUEPRINT.md's centralization note.
// Forward/reverse run through Series::layer_norm's single fused tape node
// rather than this file's own mean/sub/var/rsqrt/mul/mul/add chain.
class PLUSS_API LayerNormImpl final : public Module {
public:
  explicit LayerNormImpl(LayerNormOptions options) : Module(true), options_(std::move(options)) {
    const Config cfg = options_.cfg;
    weight_ = pluss::series::ones({options_.normalized_size}, cfg);
    bias_ = pluss::series::zeros({options_.normalized_size}, cfg);
    this->parameter_record("weight", weight_);
    this->parameter_record("bias", bias_);
  }

  [[nodiscard]] Series forward(const Series& x) const {
    return x.layer_norm(std::vector<std::int64_t>{options_.normalized_size}, weight_, bias_, options_.eps);
  }
  Series operator()(const Series& x) const { return forward(x); }
  LayerNormImpl* operator->() noexcept { return this; }
  const LayerNormImpl* operator->() const noexcept { return this; }

  Series& weight() noexcept { return weight_; }
  const Series& weight() const noexcept { return weight_; }
  Series& bias() noexcept { return bias_; }
  const Series& bias() const noexcept { return bias_; }
  LayerNormOptions& options() noexcept { return options_; }
  const LayerNormOptions& options() const noexcept { return options_; }

private:
  void on_platform_changed_(Platform platform) override { options_.cfg = options_.cfg.with_platform(platform); }
  void on_dtype_changed_(ScalarType dtype) override { options_.cfg = options_.cfg.with_dtype(dtype); }

  LayerNormOptions options_;
  Series weight_;
  Series bias_;
};
PLUSS_MODULE_IMPL(LayerNorm, LayerNormImpl);

} // namespace pluss::ai::nn

#pragma once

#include "../Module.h"
#include <pluss/ai/attention.h>

#include <cmath>
#include <tuple>
#include <utility>

namespace pluss::ai::nn {

struct MultiheadAttentionOptions {
  std::int64_t embed_dim = 8;
  std::int64_t num_heads = 8;
  double dropout = 0.0;
  bool bias = true;
  bool batch_first = false;
  bool average_weights = true;
  double scale = 0.0;
  Config cfg = ::pluss::detail::nn::default_cfg_();
  std::uint64_t seed = 0;
  bool training = true;
};

class PLUSS_API MultiheadAttentionImpl final : public Module {
public:
  explicit MultiheadAttentionImpl(MultiheadAttentionOptions options)
      : Module(options.training), options_(std::move(options)) {
    // sync_parameter_records_() allocates real (defined) storage for every
    // weight/bias and registers each immediately after -- see FusionImpl's
    // identical fix and Module::parameter_record()'s contract.
    sync_parameter_records_();
    reset_parameters();
  }

  MultiheadAttentionImpl(std::int64_t embed_dim,
                         std::int64_t num_heads,
                         double dropout = 0.0,
                         bool bias = true,
                         bool batch_first = false,
                         bool average_weights = true,
                         Config cfg = ::pluss::detail::nn::default_cfg_(),
                         std::uint64_t seed = 0)
      : MultiheadAttentionImpl(MultiheadAttentionOptions{
            .embed_dim = embed_dim,
            .num_heads = num_heads,
            .dropout = dropout,
            .bias = bias,
            .batch_first = batch_first,
            .average_weights = average_weights,
            .scale = 0.0,
            .cfg = cfg,
            .seed = seed,
            .training = true,
        }) {}

  // Re-randomizes every weight/bias value in place. Never reassigns the
  // *_weight_/*_bias_ members, so it's safe to call any number of times
  // after construction -- registration and tracks_slope_ (set once, in
  // sync_parameter_records_()) are never disturbed.
  void reset_parameters() {
    const Config cfg = ::pluss::detail::nn::floating_cfg_(options_.cfg);
    const double bound = 1.0 / std::sqrt(static_cast<double>(options_.embed_dim));
    q_weight_.copy_(::pluss::detail::nn::uniform_({options_.embed_dim, options_.embed_dim}, -bound, bound, cfg, options_.seed + 11ULL));
    k_weight_.copy_(::pluss::detail::nn::uniform_({options_.embed_dim, options_.embed_dim}, -bound, bound, cfg, options_.seed + 13ULL));
    v_weight_.copy_(::pluss::detail::nn::uniform_({options_.embed_dim, options_.embed_dim}, -bound, bound, cfg, options_.seed + 17ULL));
    out_weight_.copy_(::pluss::detail::nn::uniform_({options_.embed_dim, options_.embed_dim}, -bound, bound, cfg, options_.seed + 19ULL));

    if (options_.bias) {
      q_bias_.copy_(::pluss::detail::nn::uniform_({options_.embed_dim}, -bound, bound, cfg, options_.seed + 23ULL));
      k_bias_.copy_(::pluss::detail::nn::uniform_({options_.embed_dim}, -bound, bound, cfg, options_.seed + 29ULL));
      v_bias_.copy_(::pluss::detail::nn::uniform_({options_.embed_dim}, -bound, bound, cfg, options_.seed + 31ULL));
      out_bias_.copy_(::pluss::detail::nn::uniform_({options_.embed_dim}, -bound, bound, cfg, options_.seed + 37ULL));
    }
  }

  std::tuple<Series, Series> forward(const Series& query,
                                     const Series& key,
                                     const Series& value,
                                     const Series& attn_mask = pluss::series::undefined(),
                                     const Series& key_padding_mask = pluss::series::undefined(),
                                     bool causal = false) const {
    pluss::ai::AttentionOptions attn_options{
        .dropout = options_.dropout,
        .training = options_.training,
        .causal = causal,
        .scale = options_.scale,
        .seed = options_.seed,
    };
    pluss::ai::MultiheadOptions run_options{
        .num_heads = options_.num_heads,
        .batch_first = options_.batch_first,
        .average_weights = options_.average_weights,
        .attention = attn_options,
    };
    return pluss::ai::multihead_attention(
        query,
        key,
        value,
        pluss::ai::MultiheadWeights{
            .q_weight = q_weight_,
            .k_weight = k_weight_,
            .v_weight = v_weight_,
            .out_weight = out_weight_,
            .q_bias = q_bias_,
            .k_bias = k_bias_,
            .v_bias = v_bias_,
            .out_bias = out_bias_,
        },
        run_options,
        attn_mask,
        key_padding_mask);
  }

  auto operator()(const Series& query,
                  const Series& key,
                  const Series& value,
                  const Series& attn_mask = pluss::series::undefined(),
                  const Series& key_padding_mask = pluss::series::undefined(),
                  bool causal = false) const {
    return forward(query, key, value, attn_mask, key_padding_mask, causal);
  }

  MultiheadAttentionImpl* operator->() noexcept { return this; }
  const MultiheadAttentionImpl* operator->() const noexcept { return this; }

  MultiheadAttentionOptions& options() noexcept { return options_; }
  const MultiheadAttentionOptions& options() const noexcept { return options_; }

  Series& q_weight() noexcept { return q_weight_; }
  const Series& q_weight() const noexcept { return q_weight_; }
  Series& k_weight() noexcept { return k_weight_; }
  const Series& k_weight() const noexcept { return k_weight_; }
  Series& v_weight() noexcept { return v_weight_; }
  const Series& v_weight() const noexcept { return v_weight_; }
  Series& out_weight() noexcept { return out_weight_; }
  const Series& out_weight() const noexcept { return out_weight_; }
  Series& q_bias() noexcept { return q_bias_; }
  const Series& q_bias() const noexcept { return q_bias_; }
  Series& k_bias() noexcept { return k_bias_; }
  const Series& k_bias() const noexcept { return k_bias_; }
  Series& v_bias() noexcept { return v_bias_; }
  const Series& v_bias() const noexcept { return v_bias_; }
  Series& out_bias() noexcept { return out_bias_; }
  const Series& out_bias() const noexcept { return out_bias_; }

private:
  void on_training_changed_(bool enabled) noexcept override { options_.training = enabled; }
  void on_platform_changed_(Platform platform) override { options_.cfg = options_.cfg.with_platform(platform); }
  void on_dtype_changed_(ScalarType dtype) override { options_.cfg = options_.cfg.with_dtype(dtype); }

  void sync_parameter_records_() {
    this->clear_parameter_records_();
    PLUSS_CHECK(options_.embed_dim > 0, "pluss::ai::nn::MultiheadAttention: embed_dim must be > 0");
    PLUSS_CHECK(options_.num_heads > 0, "pluss::ai::nn::MultiheadAttention: num_heads must be > 0");
    PLUSS_CHECK(
        options_.embed_dim % options_.num_heads == 0,
        "pluss::ai::nn::MultiheadAttention: embed_dim must be divisible by num_heads");
    PLUSS_CHECK(
        options_.dropout >= 0.0 && options_.dropout <= 1.0,
        "pluss::ai::nn::MultiheadAttention: dropout must be in [0,1]");

    const Config cfg = ::pluss::detail::nn::floating_cfg_(options_.cfg);
    q_weight_ = Series::empty({options_.embed_dim, options_.embed_dim}, cfg.dtype, cfg.platform);
    k_weight_ = Series::empty({options_.embed_dim, options_.embed_dim}, cfg.dtype, cfg.platform);
    v_weight_ = Series::empty({options_.embed_dim, options_.embed_dim}, cfg.dtype, cfg.platform);
    out_weight_ = Series::empty({options_.embed_dim, options_.embed_dim}, cfg.dtype, cfg.platform);
    this->parameter_record("q_weight", q_weight_);
    this->parameter_record("k_weight", k_weight_);
    this->parameter_record("v_weight", v_weight_);
    this->parameter_record("out_weight", out_weight_);
    if (options_.bias) {
      q_bias_ = Series::empty({options_.embed_dim}, cfg.dtype, cfg.platform);
      k_bias_ = Series::empty({options_.embed_dim}, cfg.dtype, cfg.platform);
      v_bias_ = Series::empty({options_.embed_dim}, cfg.dtype, cfg.platform);
      out_bias_ = Series::empty({options_.embed_dim}, cfg.dtype, cfg.platform);
      this->parameter_record("q_bias", q_bias_);
      this->parameter_record("k_bias", k_bias_);
      this->parameter_record("v_bias", v_bias_);
      this->parameter_record("out_bias", out_bias_);
    }
  }

  MultiheadAttentionOptions options_;
  Series q_weight_ = pluss::series::undefined();
  Series k_weight_ = pluss::series::undefined();
  Series v_weight_ = pluss::series::undefined();
  Series out_weight_ = pluss::series::undefined();
  Series q_bias_ = pluss::series::undefined();
  Series k_bias_ = pluss::series::undefined();
  Series v_bias_ = pluss::series::undefined();
  Series out_bias_ = pluss::series::undefined();
};

PLUSS_MODULE(MultiheadAttention);

} // namespace pluss::ai::nn

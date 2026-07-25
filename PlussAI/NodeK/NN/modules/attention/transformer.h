#pragma once

#include "multihead.h"
#include "../fusion/fusion.h"

#include <utility>
#include <vector>

namespace pluss::ai::nn {

enum class TransformerActivation {
  ReLU,
  GELU,
};

namespace detail::transformer {

inline Series apply_activation_(const Series& x, TransformerActivation activation) {
  return activation == TransformerActivation::GELU ? x.gelu() : x.relu();
}

inline Series layer_norm_(const Series& x, const Series& weight, const Series& bias, double eps) {
  ::pluss::detail::nn::require_rank_at_least_(x, 1, "nn::transformer::layer_norm");
  PLUSS_CHECK(eps > 0.0, "pluss::ai::nn::Transformer: layer_norm_eps must be > 0");
  const Series mean = x.mean(-1, true);
  const Series var = x.var(-1, false, true);
  Series out = x.sub(mean).mul(var.add(eps).rsqrt());
  if (weight.defined()) out = out.mul(weight);
  if (bias.defined()) out = out.add(bias);
  return out;
}

} // namespace detail::transformer

inline Series generate_square_subsequent_mask(
    std::int64_t size,
    Config cfg = Config{}.with_dtype(ScalarType::Bool).with_platform(pluss::pCPU)) {
  return ::pluss::ai::square_subsequent_mask(size, std::move(cfg));
}

inline Series generate_causal_mask(
    std::int64_t target_len,
    std::int64_t source_len,
    Config cfg = Config{}.with_dtype(ScalarType::Bool).with_platform(pluss::pCPU)) {
  return ::pluss::ai::causal_attention_mask(target_len, source_len, std::move(cfg));
}

inline Series generate_square_subsequent_mask(std::int64_t size, const Series& like) {
  return ::pluss::ai::square_subsequent_mask(size, like);
}

inline Series generate_causal_mask(std::int64_t target_len, std::int64_t source_len, const Series& like) {
  return ::pluss::ai::causal_attention_mask(target_len, source_len, like);
}

struct TransformerEncoderLayerOptions {
  std::int64_t d_model = 512;
  std::int64_t nhead = 8;
  std::int64_t dim_feedforward = 2048;
  double dropout = 0.1;
  TransformerActivation activation = TransformerActivation::ReLU;
  double layer_norm_eps = 1e-5;
  bool batch_first = false;
  bool norm_first = false;
  bool bias = true;
  bool average_attn_weights = true;
  Config cfg = ::pluss::detail::nn::default_cfg_();
  std::uint64_t seed = 0;
  bool training = true;
};

class PLUSS_API TransformerEncoderLayerImpl final : public Module {
public:
  explicit TransformerEncoderLayerImpl(TransformerEncoderLayerOptions options)
      : Module(options.training),
        options_(std::move(options)),
        self_attn_(MultiheadAttentionOptions{
            .embed_dim = options_.d_model,
            .num_heads = options_.nhead,
            .dropout = options_.dropout,
            .bias = options_.bias,
            .batch_first = options_.batch_first,
            .average_weights = options_.average_attn_weights,
            .scale = 0.0,
            .cfg = options_.cfg,
            .seed = options_.seed + 101ULL,
            .training = options_.training,
        }),
        linear1_(options_.d_model, options_.dim_feedforward, options_.bias, options_.cfg, options_.seed + 103ULL),
        linear2_(options_.dim_feedforward, options_.d_model, options_.bias, options_.cfg, options_.seed + 107ULL) {
    validate_options_();
    this->module_record("self_attn", self_attn_);
    this->module_record("linear1", linear1_);
    this->module_record("linear2", linear2_);
    // sync_parameter_records_() allocates real (defined) storage for this
    // layer's own norm1_*/norm2_* and registers each immediately after --
    // see FusionImpl's identical fix and Module::parameter_record()'s
    // contract. reset_parameters() (which also re-delegates to the
    // already-registered self_attn_/linear1_/linear2_'s own
    // reset_parameters(), each of which is now itself copy_()-based rather
    // than reassignment-based) only fills in values afterwards, so nothing
    // here can ever end up untracked no matter how many times it runs.
    sync_parameter_records_();
    reset_parameters();
  }

  TransformerEncoderLayerImpl(std::int64_t d_model,
                              std::int64_t nhead,
                              std::int64_t dim_feedforward = 2048,
                              double dropout = 0.1,
                              TransformerActivation activation = TransformerActivation::ReLU,
                              bool batch_first = false,
                              bool norm_first = false,
                              bool bias = true,
                              Config cfg = ::pluss::detail::nn::default_cfg_(),
                              std::uint64_t seed = 0)
      : TransformerEncoderLayerImpl(TransformerEncoderLayerOptions{
            .d_model = d_model,
            .nhead = nhead,
            .dim_feedforward = dim_feedforward,
            .dropout = dropout,
            .activation = activation,
            .layer_norm_eps = 1e-5,
            .batch_first = batch_first,
            .norm_first = norm_first,
            .bias = bias,
            .average_attn_weights = true,
            .cfg = cfg,
            .seed = seed,
            .training = true,
        }) {}

  // Re-randomizes this layer's own norm weights/biases in place, and
  // re-delegates to self_attn_/linear1_/linear2_'s own reset_parameters()
  // (also in-place). Never reassigns norm1_weight_/norm2_weight_/etc, so
  // it's safe to call any number of times after construction.
  void reset_parameters() {
    self_attn_->reset_parameters();
    linear1_->reset_parameters();
    linear2_->reset_parameters();

    const Config cfg = ::pluss::detail::nn::floating_cfg_(options_.cfg);
    norm1_weight_.copy_(pluss::ones({options_.d_model}, cfg));
    norm2_weight_.copy_(pluss::ones({options_.d_model}, cfg));
    if (options_.bias) {
      norm1_bias_.copy_(pluss::zeros({options_.d_model}, cfg));
      norm2_bias_.copy_(pluss::zeros({options_.d_model}, cfg));
    }
  }

  Series forward(const Series& src,
                 const Series& src_mask = Series(),
                 const Series& src_key_padding_mask = Series(),
                 bool is_causal = false) const {
    const auto self_attn_block = [&](const Series& x) {
      auto [attn_out, _] = self_attn_->forward(x, x, x, src_mask, src_key_padding_mask, is_causal);
      return pluss::ai::dropout(attn_out, options_.dropout, options_.training, options_.seed + 211ULL);
    };

    const auto ff_block = [&](const Series& x) {
      Series y = linear1_->forward(x);
      y = detail::transformer::apply_activation_(y, options_.activation);
      y = pluss::ai::dropout(y, options_.dropout, options_.training, options_.seed + 223ULL);
      y = linear2_->forward(y);
      return pluss::ai::dropout(y, options_.dropout, options_.training, options_.seed + 227ULL);
    };

    if (options_.norm_first) {
      Series x = src;
      x = x.add(self_attn_block(detail::transformer::layer_norm_(x, norm1_weight_, norm1_bias_, options_.layer_norm_eps)));
      x = x.add(ff_block(detail::transformer::layer_norm_(x, norm2_weight_, norm2_bias_, options_.layer_norm_eps)));
      return x;
    }

    Series x = src.add(self_attn_block(src));
    x = detail::transformer::layer_norm_(x, norm1_weight_, norm1_bias_, options_.layer_norm_eps);
    x = x.add(ff_block(x));
    x = detail::transformer::layer_norm_(x, norm2_weight_, norm2_bias_, options_.layer_norm_eps);
    return x;
  }

  Series operator()(const Series& src,
                    const Series& src_mask = Series(),
                    const Series& src_key_padding_mask = Series(),
                    bool is_causal = false) const {
    return forward(src, src_mask, src_key_padding_mask, is_causal);
  }

  TransformerEncoderLayerImpl* operator->() noexcept { return this; }
  const TransformerEncoderLayerImpl* operator->() const noexcept { return this; }

  TransformerEncoderLayerOptions& options() noexcept { return options_; }
  const TransformerEncoderLayerOptions& options() const noexcept { return options_; }

  MultiheadAttention& self_attn() noexcept { return self_attn_; }
  const MultiheadAttention& self_attn() const noexcept { return self_attn_; }
  Fusion& linear1() noexcept { return linear1_; }
  const Fusion& linear1() const noexcept { return linear1_; }
  Fusion& linear2() noexcept { return linear2_; }
  const Fusion& linear2() const noexcept { return linear2_; }

private:
  void validate_options_() const {
    PLUSS_CHECK(options_.d_model > 0, "pluss::ai::nn::TransformerEncoderLayer: d_model must be > 0");
    PLUSS_CHECK(options_.nhead > 0, "pluss::ai::nn::TransformerEncoderLayer: nhead must be > 0");
    PLUSS_CHECK(
        options_.d_model % options_.nhead == 0,
        "pluss::ai::nn::TransformerEncoderLayer: d_model must be divisible by nhead");
    PLUSS_CHECK(
        options_.dim_feedforward > 0,
        "pluss::ai::nn::TransformerEncoderLayer: dim_feedforward must be > 0");
    PLUSS_CHECK(
        options_.dropout >= 0.0 && options_.dropout <= 1.0,
        "pluss::ai::nn::TransformerEncoderLayer: dropout must be in [0,1]");
    PLUSS_CHECK(
        options_.layer_norm_eps > 0.0,
        "pluss::ai::nn::TransformerEncoderLayer: layer_norm_eps must be > 0");
  }

  void sync_parameter_records_() {
    this->clear_parameter_records_();
    const Config cfg = ::pluss::detail::nn::floating_cfg_(options_.cfg);
    norm1_weight_ = Series::empty({options_.d_model}, cfg.dtype, cfg.platform);
    norm2_weight_ = Series::empty({options_.d_model}, cfg.dtype, cfg.platform);
    this->parameter_record("norm1_weight", norm1_weight_);
    this->parameter_record("norm2_weight", norm2_weight_);
    if (options_.bias) {
      norm1_bias_ = Series::empty({options_.d_model}, cfg.dtype, cfg.platform);
      norm2_bias_ = Series::empty({options_.d_model}, cfg.dtype, cfg.platform);
      this->parameter_record("norm1_bias", norm1_bias_);
      this->parameter_record("norm2_bias", norm2_bias_);
    }
  }

  void on_training_changed_(bool enabled) noexcept override { options_.training = enabled; }
  void on_platform_changed_(Platform platform) override { options_.cfg = options_.cfg.with_platform(platform); }
  void on_dtype_changed_(ScalarType dtype) override { options_.cfg = options_.cfg.with_dtype(dtype); }

  TransformerEncoderLayerOptions options_;
  MultiheadAttention self_attn_;
  Fusion linear1_;
  Fusion linear2_;
  Series norm1_weight_;
  Series norm1_bias_;
  Series norm2_weight_;
  Series norm2_bias_;
};

PLUSS_MODULE(TransformerEncoderLayer);

struct TransformerEncoderOptions {
  std::int64_t d_model = 512;
  std::int64_t nhead = 8;
  std::int64_t num_layers = 6;
  std::int64_t dim_feedforward = 2048;
  double dropout = 0.1;
  TransformerActivation activation = TransformerActivation::ReLU;
  double layer_norm_eps = 1e-5;
  bool batch_first = false;
  bool norm_first = false;
  bool bias = true;
  bool final_norm = true;
  Config cfg = ::pluss::detail::nn::default_cfg_();
  std::uint64_t seed = 0;
  bool training = true;
};

class PLUSS_API TransformerEncoderImpl final : public Module {
public:
  explicit TransformerEncoderImpl(TransformerEncoderOptions options)
      : Module(options.training), options_(std::move(options)) {
    validate_options_();
    build_layers_();
    // sync_parameter_records_() allocates real (defined) storage for this
    // class's own norm_weight_/norm_bias_ (when final_norm) and registers
    // each immediately after. reset_parameters() re-delegates to every
    // layer's own reset_parameters() (now copy_()-based, so it can never
    // untrack an already-module_record()'d layer) and fills norm_weight_/
    // norm_bias_ afterwards, in place.
    sync_parameter_records_();
    reset_parameters();
  }

  TransformerEncoderImpl(std::int64_t d_model,
                         std::int64_t nhead,
                         std::int64_t num_layers = 6,
                         std::int64_t dim_feedforward = 2048,
                         double dropout = 0.1,
                         TransformerActivation activation = TransformerActivation::ReLU,
                         bool batch_first = false,
                         bool norm_first = false,
                         bool bias = true,
                         Config cfg = ::pluss::detail::nn::default_cfg_(),
                         std::uint64_t seed = 0)
      : TransformerEncoderImpl(TransformerEncoderOptions{
            .d_model = d_model,
            .nhead = nhead,
            .num_layers = num_layers,
            .dim_feedforward = dim_feedforward,
            .dropout = dropout,
            .activation = activation,
            .layer_norm_eps = 1e-5,
            .batch_first = batch_first,
            .norm_first = norm_first,
            .bias = bias,
            .final_norm = true,
            .cfg = cfg,
            .seed = seed,
            .training = true,
        }) {}

  // Re-randomizes norm_weight_/norm_bias_ in place (when final_norm), and
  // re-delegates to every layer's own reset_parameters() (also in-place).
  // Never reassigns norm_weight_/norm_bias_, so it's safe to call any
  // number of times after construction.
  void reset_parameters() {
    for (auto& layer : layers_) {
      layer->reset_parameters();
    }
    if (!options_.final_norm) return;
    const Config cfg = ::pluss::detail::nn::floating_cfg_(options_.cfg);
    norm_weight_.copy_(pluss::ones({options_.d_model}, cfg));
    if (options_.bias) {
      norm_bias_.copy_(pluss::zeros({options_.d_model}, cfg));
    }
  }

  Series forward(const Series& src,
                 const Series& mask = Series(),
                 const Series& src_key_padding_mask = Series(),
                 bool is_causal = false) const {
    Series x = src;
    for (const auto& layer : layers_) {
      x = layer->forward(x, mask, src_key_padding_mask, is_causal);
    }
    if (options_.final_norm) {
      x = detail::transformer::layer_norm_(x, norm_weight_, norm_bias_, options_.layer_norm_eps);
    }
    return x;
  }

  Series operator()(const Series& src,
                    const Series& mask = Series(),
                    const Series& src_key_padding_mask = Series(),
                    bool is_causal = false) const {
    return forward(src, mask, src_key_padding_mask, is_causal);
  }

  TransformerEncoderImpl* operator->() noexcept { return this; }
  const TransformerEncoderImpl* operator->() const noexcept { return this; }

  TransformerEncoderOptions& options() noexcept { return options_; }
  const TransformerEncoderOptions& options() const noexcept { return options_; }

  std::size_t depth() const noexcept { return layers_.size(); }

  TransformerEncoderLayer& layer(std::size_t index) {
    PLUSS_CHECK(index < layers_.size(), "pluss::ai::nn::TransformerEncoder::layer: index out of range");
    return layers_[index];
  }

  const TransformerEncoderLayer& layer(std::size_t index) const {
    PLUSS_CHECK(index < layers_.size(), "pluss::ai::nn::TransformerEncoder::layer: index out of range");
    return layers_[index];
  }

private:
  void validate_options_() const {
    PLUSS_CHECK(options_.num_layers > 0, "pluss::ai::nn::TransformerEncoder: num_layers must be > 0");
    PLUSS_CHECK(options_.d_model > 0, "pluss::ai::nn::TransformerEncoder: d_model must be > 0");
    PLUSS_CHECK(options_.nhead > 0, "pluss::ai::nn::TransformerEncoder: nhead must be > 0");
    PLUSS_CHECK(
        options_.d_model % options_.nhead == 0,
        "pluss::ai::nn::TransformerEncoder: d_model must be divisible by nhead");
  }

  void build_layers_() {
    layers_.clear();
    layers_.reserve(static_cast<std::size_t>(options_.num_layers));
    for (std::int64_t i = 0; i < options_.num_layers; ++i) {
      layers_.emplace_back(TransformerEncoderLayerOptions{
          .d_model = options_.d_model,
          .nhead = options_.nhead,
          .dim_feedforward = options_.dim_feedforward,
          .dropout = options_.dropout,
          .activation = options_.activation,
          .layer_norm_eps = options_.layer_norm_eps,
          .batch_first = options_.batch_first,
          .norm_first = options_.norm_first,
          .bias = options_.bias,
          .average_attn_weights = true,
          .cfg = options_.cfg,
          .seed = options_.seed + static_cast<std::uint64_t>(719 * i + 1),
          .training = options_.training,
      });
    }

    this->clear_module_records_();
    for (std::size_t i = 0; i < layers_.size(); ++i) {
      this->module_record("layer" + std::to_string(i), layers_[i]);
    }
  }

  void sync_parameter_records_() {
    this->clear_parameter_records_();
    if (!options_.final_norm) return;
    const Config cfg = ::pluss::detail::nn::floating_cfg_(options_.cfg);
    norm_weight_ = Series::empty({options_.d_model}, cfg.dtype, cfg.platform);
    this->parameter_record("norm_weight", norm_weight_);
    if (options_.bias) {
      norm_bias_ = Series::empty({options_.d_model}, cfg.dtype, cfg.platform);
      this->parameter_record("norm_bias", norm_bias_);
    }
  }

  void on_training_changed_(bool enabled) noexcept override { options_.training = enabled; }
  void on_platform_changed_(Platform platform) override { options_.cfg = options_.cfg.with_platform(platform); }
  void on_dtype_changed_(ScalarType dtype) override { options_.cfg = options_.cfg.with_dtype(dtype); }

  TransformerEncoderOptions options_;
  std::vector<TransformerEncoderLayer> layers_;
  Series norm_weight_;
  Series norm_bias_;
};

PLUSS_MODULE(TransformerEncoder);

struct TransformerDecoderLayerOptions {
  std::int64_t d_model = 512;
  std::int64_t nhead = 8;
  std::int64_t dim_feedforward = 2048;
  double dropout = 0.1;
  TransformerActivation activation = TransformerActivation::ReLU;
  double layer_norm_eps = 1e-5;
  bool batch_first = false;
  bool norm_first = false;
  bool bias = true;
  bool average_attn_weights = true;
  Config cfg = ::pluss::detail::nn::default_cfg_();
  std::uint64_t seed = 0;
  bool training = true;
};

class PLUSS_API TransformerDecoderLayerImpl final : public Module {
public:
  explicit TransformerDecoderLayerImpl(TransformerDecoderLayerOptions options)
      : Module(options.training),
        options_(std::move(options)),
        self_attn_(MultiheadAttentionOptions{
            .embed_dim = options_.d_model,
            .num_heads = options_.nhead,
            .dropout = options_.dropout,
            .bias = options_.bias,
            .batch_first = options_.batch_first,
            .average_weights = options_.average_attn_weights,
            .scale = 0.0,
            .cfg = options_.cfg,
            .seed = options_.seed + 401ULL,
            .training = options_.training,
        }),
        cross_attn_(MultiheadAttentionOptions{
            .embed_dim = options_.d_model,
            .num_heads = options_.nhead,
            .dropout = options_.dropout,
            .bias = options_.bias,
            .batch_first = options_.batch_first,
            .average_weights = options_.average_attn_weights,
            .scale = 0.0,
            .cfg = options_.cfg,
            .seed = options_.seed + 409ULL,
            .training = options_.training,
        }),
        linear1_(options_.d_model, options_.dim_feedforward, options_.bias, options_.cfg, options_.seed + 419ULL),
        linear2_(options_.dim_feedforward, options_.d_model, options_.bias, options_.cfg, options_.seed + 421ULL) {
    validate_options_();
    this->module_record("self_attn", self_attn_);
    this->module_record("cross_attn", cross_attn_);
    this->module_record("linear1", linear1_);
    this->module_record("linear2", linear2_);
    // sync_parameter_records_() allocates real (defined) storage for this
    // layer's own norm*_ series and registers each immediately after -- see
    // TransformerEncoderLayerImpl's identical fix.
    sync_parameter_records_();
    reset_parameters();
  }

  TransformerDecoderLayerImpl(std::int64_t d_model,
                              std::int64_t nhead,
                              std::int64_t dim_feedforward = 2048,
                              double dropout = 0.1,
                              TransformerActivation activation = TransformerActivation::ReLU,
                              bool batch_first = false,
                              bool norm_first = false,
                              bool bias = true,
                              Config cfg = ::pluss::detail::nn::default_cfg_(),
                              std::uint64_t seed = 0)
      : TransformerDecoderLayerImpl(TransformerDecoderLayerOptions{
            .d_model = d_model,
            .nhead = nhead,
            .dim_feedforward = dim_feedforward,
            .dropout = dropout,
            .activation = activation,
            .layer_norm_eps = 1e-5,
            .batch_first = batch_first,
            .norm_first = norm_first,
            .bias = bias,
            .average_attn_weights = true,
            .cfg = cfg,
            .seed = seed,
            .training = true,
        }) {}

  // Re-randomizes this layer's own norm weights/biases in place, and
  // re-delegates to self_attn_/cross_attn_/linear1_/linear2_'s own
  // reset_parameters() (also in-place). Never reassigns norm1_weight_/etc,
  // so it's safe to call any number of times after construction.
  void reset_parameters() {
    self_attn_->reset_parameters();
    cross_attn_->reset_parameters();
    linear1_->reset_parameters();
    linear2_->reset_parameters();

    const Config cfg = ::pluss::detail::nn::floating_cfg_(options_.cfg);
    norm1_weight_.copy_(pluss::ones({options_.d_model}, cfg));
    norm2_weight_.copy_(pluss::ones({options_.d_model}, cfg));
    norm3_weight_.copy_(pluss::ones({options_.d_model}, cfg));
    if (options_.bias) {
      norm1_bias_.copy_(pluss::zeros({options_.d_model}, cfg));
      norm2_bias_.copy_(pluss::zeros({options_.d_model}, cfg));
      norm3_bias_.copy_(pluss::zeros({options_.d_model}, cfg));
    }
  }

  Series forward(const Series& tgt,
                 const Series& memory,
                 const Series& tgt_mask = Series(),
                 const Series& memory_mask = Series(),
                 const Series& tgt_key_padding_mask = Series(),
                 const Series& memory_key_padding_mask = Series(),
                 bool tgt_is_causal = false,
                 bool memory_is_causal = false) const {
    const auto self_attn_block = [&](const Series& x) {
      auto [attn_out, _] = self_attn_->forward(x, x, x, tgt_mask, tgt_key_padding_mask, tgt_is_causal);
      return pluss::ai::dropout(attn_out, options_.dropout, options_.training, options_.seed + 431ULL);
    };

    const auto cross_attn_block = [&](const Series& x) {
      auto [attn_out, _] = cross_attn_->forward(
          x,
          memory,
          memory,
          memory_mask,
          memory_key_padding_mask,
          memory_is_causal);
      return pluss::ai::dropout(attn_out, options_.dropout, options_.training, options_.seed + 433ULL);
    };

    const auto ff_block = [&](const Series& x) {
      Series y = linear1_->forward(x);
      y = detail::transformer::apply_activation_(y, options_.activation);
      y = pluss::ai::dropout(y, options_.dropout, options_.training, options_.seed + 439ULL);
      y = linear2_->forward(y);
      return pluss::ai::dropout(y, options_.dropout, options_.training, options_.seed + 443ULL);
    };

    if (options_.norm_first) {
      Series x = tgt;
      x = x.add(self_attn_block(detail::transformer::layer_norm_(x, norm1_weight_, norm1_bias_, options_.layer_norm_eps)));
      x = x.add(cross_attn_block(detail::transformer::layer_norm_(x, norm2_weight_, norm2_bias_, options_.layer_norm_eps)));
      x = x.add(ff_block(detail::transformer::layer_norm_(x, norm3_weight_, norm3_bias_, options_.layer_norm_eps)));
      return x;
    }

    Series x = tgt.add(self_attn_block(tgt));
    x = detail::transformer::layer_norm_(x, norm1_weight_, norm1_bias_, options_.layer_norm_eps);
    x = x.add(cross_attn_block(x));
    x = detail::transformer::layer_norm_(x, norm2_weight_, norm2_bias_, options_.layer_norm_eps);
    x = x.add(ff_block(x));
    x = detail::transformer::layer_norm_(x, norm3_weight_, norm3_bias_, options_.layer_norm_eps);
    return x;
  }

  Series operator()(const Series& tgt,
                    const Series& memory,
                    const Series& tgt_mask = Series(),
                    const Series& memory_mask = Series(),
                    const Series& tgt_key_padding_mask = Series(),
                    const Series& memory_key_padding_mask = Series(),
                    bool tgt_is_causal = false,
                    bool memory_is_causal = false) const {
    return forward(
        tgt,
        memory,
        tgt_mask,
        memory_mask,
        tgt_key_padding_mask,
        memory_key_padding_mask,
        tgt_is_causal,
        memory_is_causal);
  }

private:
  void validate_options_() const {
    PLUSS_CHECK(options_.d_model > 0, "pluss::ai::nn::TransformerDecoderLayer: d_model must be > 0");
    PLUSS_CHECK(options_.nhead > 0, "pluss::ai::nn::TransformerDecoderLayer: nhead must be > 0");
    PLUSS_CHECK(
        options_.d_model % options_.nhead == 0,
        "pluss::ai::nn::TransformerDecoderLayer: d_model must be divisible by nhead");
    PLUSS_CHECK(
        options_.dim_feedforward > 0,
        "pluss::ai::nn::TransformerDecoderLayer: dim_feedforward must be > 0");
    PLUSS_CHECK(
        options_.dropout >= 0.0 && options_.dropout <= 1.0,
        "pluss::ai::nn::TransformerDecoderLayer: dropout must be in [0,1]");
    PLUSS_CHECK(
        options_.layer_norm_eps > 0.0,
        "pluss::ai::nn::TransformerDecoderLayer: layer_norm_eps must be > 0");
  }

  void sync_parameter_records_() {
    this->clear_parameter_records_();
    const Config cfg = ::pluss::detail::nn::floating_cfg_(options_.cfg);
    norm1_weight_ = Series::empty({options_.d_model}, cfg.dtype, cfg.platform);
    norm2_weight_ = Series::empty({options_.d_model}, cfg.dtype, cfg.platform);
    norm3_weight_ = Series::empty({options_.d_model}, cfg.dtype, cfg.platform);
    this->parameter_record("norm1_weight", norm1_weight_);
    this->parameter_record("norm2_weight", norm2_weight_);
    this->parameter_record("norm3_weight", norm3_weight_);
    if (options_.bias) {
      norm1_bias_ = Series::empty({options_.d_model}, cfg.dtype, cfg.platform);
      norm2_bias_ = Series::empty({options_.d_model}, cfg.dtype, cfg.platform);
      norm3_bias_ = Series::empty({options_.d_model}, cfg.dtype, cfg.platform);
      this->parameter_record("norm1_bias", norm1_bias_);
      this->parameter_record("norm2_bias", norm2_bias_);
      this->parameter_record("norm3_bias", norm3_bias_);
    }
  }

  void on_training_changed_(bool enabled) noexcept override { options_.training = enabled; }
  void on_platform_changed_(Platform platform) override { options_.cfg = options_.cfg.with_platform(platform); }
  void on_dtype_changed_(ScalarType dtype) override { options_.cfg = options_.cfg.with_dtype(dtype); }

  TransformerDecoderLayerOptions options_;
  MultiheadAttention self_attn_;
  MultiheadAttention cross_attn_;
  Fusion linear1_;
  Fusion linear2_;
  Series norm1_weight_;
  Series norm1_bias_;
  Series norm2_weight_;
  Series norm2_bias_;
  Series norm3_weight_;
  Series norm3_bias_;
};

PLUSS_MODULE(TransformerDecoderLayer);

struct TransformerDecoderOptions {
  std::int64_t d_model = 512;
  std::int64_t nhead = 8;
  std::int64_t num_layers = 6;
  std::int64_t dim_feedforward = 2048;
  double dropout = 0.1;
  TransformerActivation activation = TransformerActivation::ReLU;
  double layer_norm_eps = 1e-5;
  bool batch_first = false;
  bool norm_first = false;
  bool bias = true;
  bool final_norm = true;
  Config cfg = ::pluss::detail::nn::default_cfg_();
  std::uint64_t seed = 0;
  bool training = true;
};

class PLUSS_API TransformerDecoderImpl final : public Module {
public:
  explicit TransformerDecoderImpl(TransformerDecoderOptions options)
      : Module(options.training), options_(std::move(options)) {
    validate_options_();
    build_layers_();
    // sync_parameter_records_() allocates real (defined) storage for this
    // class's own norm_weight_/norm_bias_ (when final_norm) and registers
    // each immediately after -- see TransformerEncoderImpl's identical fix.
    sync_parameter_records_();
    reset_parameters();
  }

  // Re-randomizes norm_weight_/norm_bias_ in place (when final_norm), and
  // re-delegates to every layer's own reset_parameters() (also in-place).
  // Never reassigns norm_weight_/norm_bias_, so it's safe to call any
  // number of times after construction.
  void reset_parameters() {
    for (auto& layer : layers_) {
      layer->reset_parameters();
    }
    if (!options_.final_norm) return;
    const Config cfg = ::pluss::detail::nn::floating_cfg_(options_.cfg);
    norm_weight_.copy_(pluss::ones({options_.d_model}, cfg));
    if (options_.bias) {
      norm_bias_.copy_(pluss::zeros({options_.d_model}, cfg));
    }
  }

  Series forward(const Series& tgt,
                 const Series& memory,
                 const Series& tgt_mask = Series(),
                 const Series& memory_mask = Series(),
                 const Series& tgt_key_padding_mask = Series(),
                 const Series& memory_key_padding_mask = Series(),
                 bool tgt_is_causal = false,
                 bool memory_is_causal = false) const {
    Series x = tgt;
    for (const auto& layer : layers_) {
      x = layer->forward(
          x,
          memory,
          tgt_mask,
          memory_mask,
          tgt_key_padding_mask,
          memory_key_padding_mask,
          tgt_is_causal,
          memory_is_causal);
    }
    if (options_.final_norm) {
      x = detail::transformer::layer_norm_(x, norm_weight_, norm_bias_, options_.layer_norm_eps);
    }
    return x;
  }

private:
  void validate_options_() const {
    PLUSS_CHECK(options_.num_layers > 0, "pluss::ai::nn::TransformerDecoder: num_layers must be > 0");
    PLUSS_CHECK(options_.d_model > 0, "pluss::ai::nn::TransformerDecoder: d_model must be > 0");
    PLUSS_CHECK(options_.nhead > 0, "pluss::ai::nn::TransformerDecoder: nhead must be > 0");
    PLUSS_CHECK(
        options_.d_model % options_.nhead == 0,
        "pluss::ai::nn::TransformerDecoder: d_model must be divisible by nhead");
  }

  void build_layers_() {
    layers_.clear();
    layers_.reserve(static_cast<std::size_t>(options_.num_layers));
    for (std::int64_t i = 0; i < options_.num_layers; ++i) {
      layers_.emplace_back(TransformerDecoderLayerOptions{
          .d_model = options_.d_model,
          .nhead = options_.nhead,
          .dim_feedforward = options_.dim_feedforward,
          .dropout = options_.dropout,
          .activation = options_.activation,
          .layer_norm_eps = options_.layer_norm_eps,
          .batch_first = options_.batch_first,
          .norm_first = options_.norm_first,
          .bias = options_.bias,
          .average_attn_weights = true,
          .cfg = options_.cfg,
          .seed = options_.seed + static_cast<std::uint64_t>(811 * i + 1),
          .training = options_.training,
      });
    }
    this->clear_module_records_();
    for (std::size_t i = 0; i < layers_.size(); ++i) {
      this->module_record("layer" + std::to_string(i), layers_[i]);
    }
  }

  void sync_parameter_records_() {
    this->clear_parameter_records_();
    if (!options_.final_norm) return;
    const Config cfg = ::pluss::detail::nn::floating_cfg_(options_.cfg);
    norm_weight_ = Series::empty({options_.d_model}, cfg.dtype, cfg.platform);
    this->parameter_record("norm_weight", norm_weight_);
    if (options_.bias) {
      norm_bias_ = Series::empty({options_.d_model}, cfg.dtype, cfg.platform);
      this->parameter_record("norm_bias", norm_bias_);
    }
  }

  void on_training_changed_(bool enabled) noexcept override { options_.training = enabled; }
  void on_platform_changed_(Platform platform) override { options_.cfg = options_.cfg.with_platform(platform); }
  void on_dtype_changed_(ScalarType dtype) override { options_.cfg = options_.cfg.with_dtype(dtype); }

  TransformerDecoderOptions options_;
  std::vector<TransformerDecoderLayer> layers_;
  Series norm_weight_;
  Series norm_bias_;
};

PLUSS_MODULE(TransformerDecoder);

struct TransformerOptions {
  std::int64_t d_model = 512;
  std::int64_t nhead = 8;
  std::int64_t num_encoder_layers = 6;
  std::int64_t num_decoder_layers = 6;
  std::int64_t dim_feedforward = 2048;
  double dropout = 0.1;
  TransformerActivation activation = TransformerActivation::ReLU;
  double layer_norm_eps = 1e-5;
  bool batch_first = false;
  bool norm_first = false;
  bool bias = true;
  Config cfg = ::pluss::detail::nn::default_cfg_();
  std::uint64_t seed = 0;
  bool training = true;
};

class PLUSS_API TransformerImpl final : public Module {
public:
  explicit TransformerImpl(TransformerOptions options)
      : Module(options.training),
        options_(std::move(options)),
        encoder_(TransformerEncoderOptions{
            .d_model = options_.d_model,
            .nhead = options_.nhead,
            .num_layers = options_.num_encoder_layers,
            .dim_feedforward = options_.dim_feedforward,
            .dropout = options_.dropout,
            .activation = options_.activation,
            .layer_norm_eps = options_.layer_norm_eps,
            .batch_first = options_.batch_first,
            .norm_first = options_.norm_first,
            .bias = options_.bias,
            .final_norm = true,
            .cfg = options_.cfg,
            .seed = options_.seed + 1001ULL,
            .training = options_.training,
        }),
        decoder_(TransformerDecoderOptions{
            .d_model = options_.d_model,
            .nhead = options_.nhead,
            .num_layers = options_.num_decoder_layers,
            .dim_feedforward = options_.dim_feedforward,
            .dropout = options_.dropout,
            .activation = options_.activation,
            .layer_norm_eps = options_.layer_norm_eps,
            .batch_first = options_.batch_first,
            .norm_first = options_.norm_first,
            .bias = options_.bias,
            .final_norm = true,
            .cfg = options_.cfg,
            .seed = options_.seed + 2003ULL,
            .training = options_.training,
        }) {
    validate_options_();
    this->module_record("encoder", encoder_);
    this->module_record("decoder", decoder_);
  }

  TransformerImpl(std::int64_t d_model,
                  std::int64_t nhead,
                  std::int64_t num_encoder_layers = 6,
                  std::int64_t num_decoder_layers = 6,
                  std::int64_t dim_feedforward = 2048,
                  double dropout = 0.1,
                  TransformerActivation activation = TransformerActivation::ReLU,
                  bool batch_first = false,
                  bool norm_first = false,
                  bool bias = true,
                  Config cfg = ::pluss::detail::nn::default_cfg_(),
                  std::uint64_t seed = 0)
      : TransformerImpl(TransformerOptions{
            .d_model = d_model,
            .nhead = nhead,
            .num_encoder_layers = num_encoder_layers,
            .num_decoder_layers = num_decoder_layers,
            .dim_feedforward = dim_feedforward,
            .dropout = dropout,
            .activation = activation,
            .layer_norm_eps = 1e-5,
            .batch_first = batch_first,
            .norm_first = norm_first,
            .bias = bias,
            .cfg = cfg,
            .seed = seed,
            .training = true,
        }) {}

  Series forward(const Series& src,
                 const Series& tgt,
                 const Series& src_mask = Series(),
                 const Series& tgt_mask = Series(),
                 const Series& memory_mask = Series(),
                 const Series& src_key_padding_mask = Series(),
                 const Series& tgt_key_padding_mask = Series(),
                 const Series& memory_key_padding_mask = Series(),
                 bool src_is_causal = false,
                 bool tgt_is_causal = false,
                 bool memory_is_causal = false) const {
    validate_io_(src, tgt);
    const Series memory = encoder_->forward(src, src_mask, src_key_padding_mask, src_is_causal);
    return decoder_->forward(
        tgt,
        memory,
        tgt_mask,
        memory_mask,
        tgt_key_padding_mask,
        memory_key_padding_mask,
        tgt_is_causal,
        memory_is_causal);
  }

  Series forward(const Series& src) const {
    validate_encoder_io_(src);
    return encoder_->forward(src);
  }

  Series encode(const Series& src,
                const Series& src_mask = Series(),
                const Series& src_key_padding_mask = Series(),
                bool src_is_causal = false) const {
    validate_encoder_io_(src);
    return encoder_->forward(src, src_mask, src_key_padding_mask, src_is_causal);
  }

  Series operator()(const Series& src, const Series& tgt) const { return forward(src, tgt); }
  Series operator()(const Series& src) const { return forward(src); }

private:
  void validate_options_() const {
    PLUSS_CHECK(options_.d_model > 0, "pluss::ai::nn::Transformer: d_model must be > 0");
    PLUSS_CHECK(options_.nhead > 0, "pluss::ai::nn::Transformer: nhead must be > 0");
    PLUSS_CHECK(
        options_.d_model % options_.nhead == 0,
        "pluss::ai::nn::Transformer: d_model must be divisible by nhead");
    PLUSS_CHECK(options_.num_encoder_layers > 0, "pluss::ai::nn::Transformer: num_encoder_layers must be > 0");
    PLUSS_CHECK(options_.num_decoder_layers > 0, "pluss::ai::nn::Transformer: num_decoder_layers must be > 0");
  }

  void validate_encoder_io_(const Series& src) const {
    ::pluss::detail::nn::require_rank_at_least_(src, 2, "nn::Transformer::forward");
    PLUSS_CHECK(
        src.sizes()[src.dim() - 1] == options_.d_model,
        "pluss::ai::nn::Transformer: src feature dimension must equal d_model");
  }

  void validate_io_(const Series& src, const Series& tgt) const {
    validate_encoder_io_(src);
    ::pluss::detail::nn::require_rank_at_least_(tgt, 2, "nn::Transformer::forward");
    PLUSS_CHECK(
        tgt.sizes()[tgt.dim() - 1] == options_.d_model,
        "pluss::ai::nn::Transformer: tgt feature dimension must equal d_model");
    if (src.dim() == 3 && tgt.dim() == 3) {
      const std::int64_t src_batch = options_.batch_first ? src.sizes()[0] : src.sizes()[1];
      const std::int64_t tgt_batch = options_.batch_first ? tgt.sizes()[0] : tgt.sizes()[1];
      PLUSS_CHECK(src_batch == tgt_batch, "pluss::ai::nn::Transformer: src and tgt batch sizes must match");
    }
  }

  void on_training_changed_(bool enabled) noexcept override { options_.training = enabled; }
  void on_platform_changed_(Platform platform) override { options_.cfg = options_.cfg.with_platform(platform); }
  void on_dtype_changed_(ScalarType dtype) override { options_.cfg = options_.cfg.with_dtype(dtype); }

  TransformerOptions options_;
  TransformerEncoder encoder_;
  TransformerDecoder decoder_;
};

PLUSS_MODULE(Transformer);

} // namespace pluss::ai::nn

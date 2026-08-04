#pragma once

#include <pluss/nn_detail.h>
#include "NodeK/kernels/cudnn_attention.h"

#include <cmath>
#include <cstdint>
#include <tuple>
#include <utility>
#include <vector>

namespace pluss::ai {

struct AttentionOptions {
  double dropout{0.0};
  bool training{true};
  bool causal{false};
  double scale{0.0};
  std::uint64_t seed{0};
};

struct MultiheadOptions {
  std::int64_t num_heads{8};
  bool batch_first{false};
  bool average_weights{true};
  AttentionOptions attention{};
};

struct MultiheadWeights {
  Series q_weight{};
  Series k_weight{};
  Series v_weight{};
  Series out_weight{};
  Series q_bias{};
  Series k_bias{};
  Series v_bias{};
  Series out_bias{};
};

inline Series causal_attention_mask(
    std::int64_t target_len,
    std::int64_t source_len,
    Config cfg = Config{}.with_dtype(ScalarType::Bool).with_platform(pluss::pCPU)) {
  PLUSS_CHECK(target_len > 0, "pluss::ai::causal_attention_mask: target_len must be > 0");
  PLUSS_CHECK(source_len > 0, "pluss::ai::causal_attention_mask: source_len must be > 0");
  const Config idx_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(cfg.platform);
  const Series rows = pluss::series::arange(0, target_len, 1, idx_cfg).reshape({target_len, 1});
  const Series cols = pluss::series::arange(0, source_len, 1, idx_cfg).reshape({1, source_len});
  Series mask = cols.gt(rows);
  if (cfg.dtype != ScalarType::Bool) {
    mask = mask.to(cfg.platform, cfg.dtype);
  }
  return mask;
}

inline Series square_subsequent_mask(
    std::int64_t size,
    Config cfg = Config{}.with_dtype(ScalarType::Bool).with_platform(pluss::pCPU)) {
  return causal_attention_mask(size, size, std::move(cfg));
}

inline Series causal_attention_mask(std::int64_t target_len, std::int64_t source_len, const Series& like) {
  return causal_attention_mask(
      target_len,
      source_len,
      Config{}.with_dtype(ScalarType::Bool).with_platform(like.platform()));
}

inline Series square_subsequent_mask(std::int64_t size, const Series& like) {
  return square_subsequent_mask(size, Config{}.with_dtype(ScalarType::Bool).with_platform(like.platform()));
}

namespace detail::attn {

inline Config idx_cfg_like_(const Series& like) {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(like.platform());
}

inline Config scalar_cfg_like_(const Series& like) {
  return Config{}.with_dtype(like.scalar_type()).with_platform(like.platform());
}

inline Series cast_like_(const Series& x, const Series& like) {
  return ::pluss::detail::nn::cast_like_(x, like);
}

inline void require_qkv_shapes_(const Series& query, const Series& key, const Series& value, const char* what) {
  ::pluss::detail::nn::require_rank_at_least_(query, 2, what);
  ::pluss::detail::nn::require_rank_at_least_(key, 2, what);
  ::pluss::detail::nn::require_rank_at_least_(value, 2, what);
  PLUSS_CHECK(
      query.sizes()[query.dim() - 1] == key.sizes()[key.dim() - 1],
      std::string("pluss::ai::") + what + ": query/key feature dimensions must match");
  PLUSS_CHECK(
      key.sizes()[key.dim() - 2] == value.sizes()[value.dim() - 2],
      std::string("pluss::ai::") + what + ": key/value sequence lengths must match");
}

inline Series make_blocked_mask_(std::int64_t target_len, std::int64_t source_len, const Series& like) {
  const Series rows = pluss::series::arange(0, target_len, 1, idx_cfg_like_(like)).reshape({target_len, 1});
  const Series cols = pluss::series::arange(0, source_len, 1, idx_cfg_like_(like)).reshape({1, source_len});
  return cols.gt(rows);
}

inline Series mask_penalty_like_(const Series& like) {
  return pluss::series::full({}, -1.0e30, scalar_cfg_like_(like));
}

inline Series apply_mask_(const Series& logits, const Series& mask) {
  if (!mask.defined()) return logits;
  if (mask.scalar_type() == ScalarType::Bool) {
    const Series blocked = mask.to(logits.platform(), ScalarType::Bool);
    return blocked.where(mask_penalty_like_(logits), logits);
  }
  return logits.add(mask.to(logits.platform(), logits.scalar_type()));
}

inline Series to_batch_first_(const Series& x, bool batch_first) {
  if (x.dim() == 2) {
    return x.unsqueeze(0);
  }
  PLUSS_CHECK(x.dim() == 3, "pluss::ai::attention: expected query/key/value rank 2 or 3");
  return batch_first ? x : x.transpose(0, 1);
}

inline Series from_batch_first_(const Series& x, bool batch_first, bool unbatched_input) {
  if (unbatched_input) return x.squeeze(0);
  return batch_first ? x : x.transpose(0, 1);
}

inline Series normalize_attn_mask_(
    const Series& attn_mask,
    std::int64_t batch,
    std::int64_t heads,
    std::int64_t target_len,
    std::int64_t source_len,
    const Series& like) {
  if (!attn_mask.defined()) return Series();

  Series mask = attn_mask.to(like.platform(), attn_mask.scalar_type());
  if (mask.dim() == 2) {
    PLUSS_CHECK(
        mask.sizes()[0] == target_len && mask.sizes()[1] == source_len,
        "pluss::ai::multihead_attention: 2D attn_mask shape must be [target_len, source_len]");
    return mask.reshape({1, 1, target_len, source_len});
  }

  if (mask.dim() == 3) {
    PLUSS_CHECK(
        mask.sizes()[1] == target_len && mask.sizes()[2] == source_len,
        "pluss::ai::multihead_attention: 3D attn_mask trailing shape must be [target_len, source_len]");
    const std::int64_t lead = mask.sizes()[0];
    if (lead == 1) return mask.reshape({1, 1, target_len, source_len});
    if (lead == batch) return mask.reshape({batch, 1, target_len, source_len});
    if (lead == batch * heads) return mask.reshape({batch, heads, target_len, source_len});
    PLUSS_FAIL("pluss::ai::multihead_attention: 3D attn_mask first dimension must be 1, batch, or batch*heads");
  }

  if (mask.dim() == 4) {
    PLUSS_CHECK(
        mask.sizes()[2] == target_len && mask.sizes()[3] == source_len,
        "pluss::ai::multihead_attention: 4D attn_mask trailing shape must be [target_len, source_len]");
    return mask;
  }

  PLUSS_FAIL("pluss::ai::multihead_attention: attn_mask rank must be 2, 3, or 4");
}

inline Series normalize_key_padding_mask_(
    const Series& key_padding_mask,
    std::int64_t batch,
    std::int64_t source_len,
    const Series& like) {
  if (!key_padding_mask.defined()) return Series();
  Series mask = key_padding_mask.to(like.platform(), ScalarType::Bool);
  if (mask.dim() == 1) {
    PLUSS_CHECK(
        batch == 1 && mask.sizes()[0] == source_len,
        "pluss::ai::multihead_attention: 1D key_padding_mask requires unbatched input and shape [source_len]");
    return mask.reshape({1, 1, 1, source_len});
  }
  PLUSS_CHECK(
      mask.dim() == 2 && mask.sizes()[0] == batch && mask.sizes()[1] == source_len,
      "pluss::ai::multihead_attention: key_padding_mask shape must be [batch, source_len]");
  return mask.reshape({batch, 1, 1, source_len});
}

inline Series combine_masks_(
    const Series& attn_mask,
    const Series& key_padding_mask,
    const Series& like) {
  if (!attn_mask.defined()) return key_padding_mask;
  if (!key_padding_mask.defined()) return attn_mask;

  if (attn_mask.scalar_type() == ScalarType::Bool) {
    return attn_mask.logical_or(key_padding_mask);
  }

  const Series pad_add = key_padding_mask.to(like.platform(), like.scalar_type()).mul(mask_penalty_like_(like));
  return attn_mask.to(like.platform(), like.scalar_type()).add(pad_add);
}

} // namespace detail::attn

inline std::tuple<Series, Series> scaled_attention(
    const Series& query,
    const Series& key,
    const Series& value,
    const Series& attn_mask = Series(),
    AttentionOptions options = {}) {
  detail::attn::require_qkv_shapes_(query, key, value, "scaled_attention");
  PLUSS_CHECK(options.dropout >= 0.0 && options.dropout <= 1.0, "pluss::ai::scaled_attention: dropout must be in [0, 1]");

  const Series q = query;
  const Series k = detail::attn::cast_like_(key, q);
  const Series v = detail::attn::cast_like_(value, q);
  const std::int64_t target_len = q.sizes()[q.dim() - 2];
  const std::int64_t source_len = k.sizes()[k.dim() - 2];
  const std::int64_t head_dim = q.sizes()[q.dim() - 1];
  PLUSS_CHECK(head_dim > 0, "pluss::ai::scaled_attention: head dimension must be > 0");

  const double scale = options.scale > 0.0 ? options.scale : 1.0 / std::sqrt(static_cast<double>(head_dim));
  Series logits = q.matmul(k.transpose(-2, -1)).mul(scale);

  if (options.causal) {
    const Series causal_block = detail::attn::make_blocked_mask_(target_len, source_len, logits);
    logits = detail::attn::apply_mask_(logits, causal_block);
  }

  logits = detail::attn::apply_mask_(logits, attn_mask);
  const bool require_backward =
      ::pluss::codem::series_tape::recording_enabled() &&
      (query.tracks_slope() || key.tracks_slope() || value.tracks_slope());
  Series weights;
  if (auto cudnn_weights = ::pluss::ai::kernels::cudnn::try_scaled_attention_softmax(logits, require_backward);
      cudnn_weights.has_value()) {
    weights = std::move(*cudnn_weights);
  } else {
    weights = logits.softmax(-1);
  }
  if (options.dropout > 0.0) {
    weights = pluss::ai::dropout(weights, options.dropout, options.training, options.seed);
  }
  const Series output = weights.matmul(v);
  return {output, weights};
}

inline std::tuple<Series, Series> multihead_attention(
    const Series& query,
    const Series& key,
    const Series& value,
    const MultiheadWeights& weights,
    MultiheadOptions options = {},
    const Series& attn_mask = Series(),
    const Series& key_padding_mask = Series()) {
  PLUSS_CHECK(options.num_heads > 0, "pluss::ai::multihead_attention: num_heads must be > 0");

  const bool unbatched_input = query.dim() == 2;
  const Series q_in = detail::attn::to_batch_first_(query, options.batch_first);
  const Series k_in = detail::attn::to_batch_first_(key, options.batch_first);
  const Series v_in = detail::attn::to_batch_first_(value, options.batch_first);
  detail::attn::require_qkv_shapes_(q_in, k_in, v_in, "multihead_attention");

  const std::int64_t batch = q_in.sizes()[0];
  const std::int64_t target_len = q_in.sizes()[1];
  const std::int64_t source_len = k_in.sizes()[1];
  const std::int64_t embed_dim = q_in.sizes()[2];
  PLUSS_CHECK(embed_dim % options.num_heads == 0, "pluss::ai::multihead_attention: embed_dim must be divisible by num_heads");

  const std::int64_t head_dim = embed_dim / options.num_heads;
  const Series q_proj = pluss::ai::ff(q_in, weights.q_weight, weights.q_bias);
  const Series k_proj = pluss::ai::ff(k_in, weights.k_weight, weights.k_bias);
  const Series v_proj = pluss::ai::ff(v_in, weights.v_weight, weights.v_bias);

  const Series q_heads = q_proj.reshape({batch, target_len, options.num_heads, head_dim}).transpose(1, 2);
  const Series k_heads = k_proj.reshape({batch, source_len, options.num_heads, head_dim}).transpose(1, 2);
  const Series v_heads = v_proj.reshape({batch, source_len, options.num_heads, head_dim}).transpose(1, 2);

  const Series norm_attn_mask = detail::attn::normalize_attn_mask_(
      attn_mask,
      batch,
      options.num_heads,
      target_len,
      source_len,
      q_heads);
  const Series norm_kpm = detail::attn::normalize_key_padding_mask_(key_padding_mask, batch, source_len, q_heads);
  const Series merged_mask = detail::attn::combine_masks_(norm_attn_mask, norm_kpm, q_heads);

  AttentionOptions attn_options = options.attention;
  auto [attn_out, attn_weights] =
      scaled_attention(q_heads, k_heads, v_heads, merged_mask, attn_options);

  Series output = attn_out.transpose(1, 2).reshape({batch, target_len, embed_dim});
  output = pluss::ai::ff(output, weights.out_weight, weights.out_bias);
  output = detail::attn::from_batch_first_(output, options.batch_first, unbatched_input);

  Series weights_out = attn_weights;
  if (options.average_weights) {
    weights_out = attn_weights.mean(1, false);
  }
  if (unbatched_input) {
    weights_out = weights_out.squeeze(0);
  }
  return {output, weights_out};
}

} // namespace pluss::ai

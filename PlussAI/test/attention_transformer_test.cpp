#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <cmath>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

bool approx(double a, double b, double tol = 1e-8) {
  return std::abs(a - b) <= tol;
}

bool same_shape(const Series& x, std::initializer_list<std::int64_t> expected) {
  const auto sizes = x.sizes();
  if (sizes.size() != expected.size()) return false;
  std::size_t i = 0;
  for (const auto v : expected) {
    if (sizes[i++] != v) return false;
  }
  return true;
}

bool run_attention_transformer_checks(std::ostream* err = nullptr) {
  const Config cfg = f64();
  const Series x = pluss::rand({2, 4, 8}, cfg, 71ULL);

  const auto [sa_out, sa_w] = pluss::scaled_attention(x, x, x);
  if (!same_shape(sa_out, {2, 4, 8}) || !same_shape(sa_w, {2, 4, 4})) {
    if (err) *err << "scaled_attention shape mismatch\n";
    return false;
  }
  const auto row_sum = sa_w.sum(-1).to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  for (double v : row_sum) {
    if (!approx(v, 1.0, 1e-6)) {
      if (err) *err << "scaled_attention softmax row normalization mismatch\n";
      return false;
    }
  }

  const Series functional_causal_mask = pluss::square_subsequent_mask(4, Config{}.with_dtype(ScalarType::Bool).with_platform(pluss::pCPU));
  if (functional_causal_mask.scalar_type() != ScalarType::Bool || !same_shape(functional_causal_mask, {4, 4})) {
    if (err) *err << "square_subsequent_mask should return bool [L,L]\n";
    return false;
  }
  const Series nn_causal_mask = pluss::nn::generate_square_subsequent_mask(4, cfg);
  if (!same_shape(nn_causal_mask, {4, 4})) {
    if (err) *err << "nn::generate_square_subsequent_mask shape mismatch\n";
    return false;
  }

  const auto [sa_masked_out, sa_masked_w] = pluss::scaled_attention(x, x, x, functional_causal_mask);
  const auto [sa_causal_out, sa_causal_w] = pluss::scaled_attention(
      x,
      x,
      x,
      Series(),
      pluss::AttentionOptions{
          .dropout = 0.0,
          .training = true,
          .causal = true,
          .scale = 0.0,
          .seed = 0ULL,
      });
  const double masked_vs_causal_out_max = sa_masked_out.sub(sa_causal_out).abs().max().item<double>();
  const double masked_vs_causal_w_max = sa_masked_w.sub(sa_causal_w).abs().max().item<double>();
  if (!(masked_vs_causal_out_max <= 1e-9 && masked_vs_causal_w_max <= 1e-9)) {
    if (err) *err << "bool causal mask and causal=true path should match\n";
    return false;
  }
  const double upper_leak = sa_masked_w
                                .mul(functional_causal_mask.to(sa_masked_w.platform(), sa_masked_w.scalar_type()))
                                .abs()
                                .max()
                                .item<double>();
  if (upper_leak > 1e-9) {
    if (err) *err << "causal mask should block upper-triangular attention mass\n";
    return false;
  }

  const Series wq = pluss::rand({8, 8}, cfg, 73ULL);
  const Series wk = pluss::rand({8, 8}, cfg, 79ULL);
  const Series wv = pluss::rand({8, 8}, cfg, 83ULL);
  const Series wo = pluss::rand({8, 8}, cfg, 89ULL);
  const Series bq = pluss::zeros({8}, cfg);
  const Series bk = pluss::zeros({8}, cfg);
  const Series bv = pluss::zeros({8}, cfg);
  const Series bo = pluss::zeros({8}, cfg);

  pluss::MultiheadWeights weights{
      .q_weight = wq,
      .k_weight = wk,
      .v_weight = wv,
      .out_weight = wo,
      .q_bias = bq,
      .k_bias = bk,
      .v_bias = bv,
      .out_bias = bo,
  };
  pluss::MultiheadOptions mh_opts{
      .num_heads = 2,
      .batch_first = true,
      .average_weights = true,
      .attention = pluss::AttentionOptions{
          .dropout = 0.0,
          .training = true,
          .causal = false,
          .scale = 0.0,
          .seed = 97ULL,
      },
  };
  const auto [mh_out, mh_w] = pluss::multihead_attention(x, x, x, weights, mh_opts);
  if (!same_shape(mh_out, {2, 4, 8}) || !same_shape(mh_w, {2, 4, 4})) {
    if (err) *err << "functional multihead_attention shape mismatch\n";
    return false;
  }

  pluss::nn::MultiheadAttention mha(pluss::nn::MultiheadAttentionOptions{
      .embed_dim = 8,
      .num_heads = 2,
      .dropout = 0.0,
      .bias = true,
      .batch_first = true,
      .average_weights = true,
      .scale = 0.0,
      .cfg = cfg,
      .seed = 101ULL,
      .training = true,
  });
  const auto [mha_out, mha_w] = mha->forward(x, x, x);
  if (!same_shape(mha_out, {2, 4, 8}) || !same_shape(mha_w, {2, 4, 4})) {
    if (err) *err << "nn::MultiheadAttention shape mismatch\n";
    return false;
  }
  mha->eval();
  if (mha->options().training) {
    if (err) *err << "nn::MultiheadAttention eval() should update options.training\n";
    return false;
  }

  pluss::nn::TransformerEncoderLayer encoder_layer(pluss::nn::TransformerEncoderLayerOptions{
      .d_model = 8,
      .nhead = 2,
      .dim_feedforward = 16,
      .dropout = 0.0,
      .activation = pluss::nn::TransformerActivation::GELU,
      .layer_norm_eps = 1e-5,
      .batch_first = true,
      .norm_first = true,
      .bias = true,
      .average_attn_weights = true,
      .cfg = cfg,
      .seed = 131ULL,
      .training = true,
  });
  const Series layer_out = encoder_layer->forward(x);
  if (!same_shape(layer_out, {2, 4, 8})) {
    if (err) *err << "nn::TransformerEncoderLayer shape mismatch\n";
    return false;
  }

  pluss::nn::Transformer encoder(pluss::nn::TransformerOptions{
      .d_model = 8,
      .nhead = 2,
      .num_encoder_layers = 2,
      .num_decoder_layers = 2,
      .dim_feedforward = 16,
      .dropout = 0.0,
      .activation = pluss::nn::TransformerActivation::ReLU,
      .layer_norm_eps = 1e-5,
      .batch_first = true,
      .norm_first = false,
      .bias = true,
      .cfg = cfg,
      .seed = 151ULL,
      .training = true,
  });
  const Series enc_out = encoder->forward(x);
  if (!same_shape(enc_out, {2, 4, 8})) {
    if (err) *err << "nn::Transformer shape mismatch\n";
    return false;
  }
  if (encoder->named_modules().size() <= 1) {
    if (err) *err << "nn::Transformer named_modules should include registered layer modules\n";
    return false;
  }

  pluss::nn::TransformerDecoderLayer decoder_layer(pluss::nn::TransformerDecoderLayerOptions{
      .d_model = 8,
      .nhead = 2,
      .dim_feedforward = 16,
      .dropout = 0.0,
      .activation = pluss::nn::TransformerActivation::ReLU,
      .layer_norm_eps = 1e-5,
      .batch_first = true,
      .norm_first = true,
      .bias = true,
      .average_attn_weights = true,
      .cfg = cfg,
      .seed = 181ULL,
      .training = true,
  });
  const Series dec_layer_out = decoder_layer->forward(x, x);
  if (!same_shape(dec_layer_out, {2, 4, 8})) {
    if (err) *err << "nn::TransformerDecoderLayer shape mismatch\n";
    return false;
  }

  pluss::nn::TransformerDecoder decoder(pluss::nn::TransformerDecoderOptions{
      .d_model = 8,
      .nhead = 2,
      .num_layers = 2,
      .dim_feedforward = 16,
      .dropout = 0.0,
      .activation = pluss::nn::TransformerActivation::GELU,
      .layer_norm_eps = 1e-5,
      .batch_first = true,
      .norm_first = false,
      .bias = true,
      .final_norm = true,
      .cfg = cfg,
      .seed = 191ULL,
      .training = true,
  });
  const Series dec_out = decoder->forward(x, x);
  if (!same_shape(dec_out, {2, 4, 8})) {
    if (err) *err << "nn::TransformerDecoder shape mismatch\n";
    return false;
  }

  const Series full_out = encoder->forward(x, x);
  if (!same_shape(full_out, {2, 4, 8})) {
    if (err) *err << "nn::Transformer full forward(src,tgt) shape mismatch\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("attention and transformer surfaces stay aligned") {
  REQUIRE(run_attention_transformer_checks(&std::cerr));
}
#else
int main() {
  if (!run_attention_transformer_checks(&std::cerr)) return 1;
  std::cout << "attention_transformer_test passed\n";
  return 0;
}
#endif

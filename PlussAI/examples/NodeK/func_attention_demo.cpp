#include <pluss/pluss.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const Series x = pluss::series::rand({2, 6, 8}, cfg, 41ULL);

  const pluss::MultiheadWeights weights{
      .q_weight = pluss::series::rand({8, 8}, cfg, 43ULL),
      .k_weight = pluss::series::rand({8, 8}, cfg, 47ULL),
      .v_weight = pluss::series::rand({8, 8}, cfg, 53ULL),
      .out_weight = pluss::series::rand({8, 8}, cfg, 59ULL),
      .q_bias = pluss::series::zeros({8}, cfg),
      .k_bias = pluss::series::zeros({8}, cfg),
      .v_bias = pluss::series::zeros({8}, cfg),
      .out_bias = pluss::series::zeros({8}, cfg),
  };
  const pluss::MultiheadOptions options{
      .num_heads = 2,
      .batch_first = true,
      .average_weights = true,
      .attention = pluss::AttentionOptions{
          .dropout = 0.0,
          .training = true,
          .causal = false,
          .scale = 0.0,
          .seed = 61ULL,
      },
  };

  const auto [out, attn_w] = pluss::multihead_attention(x, x, x, weights, options);
  const Series causal_mask = pluss::square_subsequent_mask(6, x);
  const auto [out_causal, attn_causal] = pluss::multihead_attention(x, x, x, weights, options, causal_mask);
  std::cout << "functional_attention_out_mean=" << out.mean().item<double>() << "\n";
  std::cout << "functional_attention_weights_mean=" << attn_w.mean().item<double>() << "\n";
  std::cout << "functional_attention_causal_out_mean=" << out_causal.mean().item<double>() << "\n";
  std::cout << "functional_attention_causal_weights_mean=" << attn_causal.mean().item<double>() << "\n";
  return 0;
}

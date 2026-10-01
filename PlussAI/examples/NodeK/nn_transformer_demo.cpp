#include <pluss/pluss.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const Series x = pluss::series::rand({3, 12, 16}, cfg, 101ULL);

  pluss::nn::Transformer model(pluss::nn::TransformerOptions{
      .d_model = 16,
      .nhead = 4,
      .num_encoder_layers = 2,
      .num_decoder_layers = 2,
      .dim_feedforward = 64,
      .dropout = 0.1,
      .activation = pluss::nn::TransformerActivation::GELU,
      .layer_norm_eps = 1e-5,
      .batch_first = true,
      .norm_first = true,
      .bias = true,
      .cfg = cfg,
      .seed = 131ULL,
      .training = true,
  });

  const Series tgt_mask = pluss::nn::generate_square_subsequent_mask(x.sizes()[1], x);
  const Series mem_mask = pluss::nn::generate_causal_mask(x.sizes()[1], x.sizes()[1], x);
  const Series y_train = model->forward(
      x,
      x,
      pluss::series::undefined(),
      tgt_mask,
      mem_mask,
      pluss::series::undefined(),
      pluss::series::undefined(),
      pluss::series::undefined(),
      false,
      true,
      false);
  model->eval();
  const Series y_eval = model->forward(
      x,
      x,
      pluss::series::undefined(),
      tgt_mask,
      mem_mask,
      pluss::series::undefined(),
      pluss::series::undefined(),
      pluss::series::undefined(),
      false,
      true,
      false);
  const Series memory = model->forward(x);

  std::cout << "transformer_train_mean=" << y_train.mean().item<double>() << "\n";
  std::cout << "transformer_eval_mean=" << y_eval.mean().item<double>() << "\n";
  std::cout << "transformer_encoder_memory_mean=" << memory.mean().item<double>() << "\n";
  std::cout << "tgt_mask_true_count=" << tgt_mask.to(pluss::pCPU, ScalarType::Int64).sum().item<std::int64_t>() << "\n";
  std::cout << "named_parameter_count=" << model->named_parameters().size() << "\n";
  return 0;
}

#include <pluss/pluss.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  std::cout << "cuda_ready=" << (pluss::cuda::is_ready() ? 1 : 0) << "\n";
  std::cout << "cudnn_ready=" << (pluss::cuda::cudnn_ready() ? 1 : 0) << "\n";
  std::cout << "cudnn_attention_ready=" << (pluss::cudnn_attention_ready() ? 1 : 0) << "\n";
  std::cout << "cudnn_transformer_ready=" << (pluss::cudnn_transformer_ready() ? 1 : 0) << "\n";

  const Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const Series src = pluss::series::rand({2, 10, 16}, cfg, 701ULL);
  const Series tgt = pluss::series::rand({2, 10, 16}, cfg, 709ULL);

  pluss::nn::Transformer model(pluss::nn::TransformerOptions{
      .d_model = 16,
      .nhead = 4,
      .num_encoder_layers = 2,
      .num_decoder_layers = 2,
      .dim_feedforward = 64,
      .dropout = 0.0,
      .activation = pluss::nn::TransformerActivation::GELU,
      .layer_norm_eps = 1e-5,
      .batch_first = true,
      .norm_first = true,
      .bias = true,
      .cfg = cfg,
      .seed = 733ULL,
      .training = false,
  });

  const Series tgt_mask = pluss::nn::generate_square_subsequent_mask(tgt.sizes()[1], tgt);
  const Series out_cpu = model->forward(src, tgt, Series(), tgt_mask, Series(), Series(), Series(), Series(), false, true, false);
  std::cout << "transformer_cpu_mean=" << out_cpu.mean().item<double>() << "\n";

  if (pluss::cuda::is_ready()) {
    const Series src_cuda = src.to(pluss::pCUDA);
    const Series tgt_cuda = tgt.to(pluss::pCUDA);
    const Series tgt_mask_cuda = pluss::nn::generate_square_subsequent_mask(tgt_cuda.sizes()[1], tgt_cuda);
    model->to(pluss::pCUDA);
    const Series out_cuda = model->forward(
        src_cuda,
        tgt_cuda,
        Series(),
        tgt_mask_cuda,
        Series(),
        Series(),
        Series(),
        Series(),
        false,
        true,
        false);
    const Series out_cuda_cpu = out_cuda.to(pluss::pCPU);
    std::cout << "transformer_cuda_mean=" << out_cuda_cpu.mean().item<double>() << "\n";
    std::cout << "cpu_cuda_max_abs_diff=" << out_cpu.sub(out_cuda_cpu).abs().max().item<double>() << "\n";
  } else {
    std::cout << "transformer_cuda_mean=skipped\n";
  }

  return 0;
}

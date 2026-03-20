#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  std::cout << "cuDNN route status\n";
  std::cout << "cuda_ready: " << (pluss::cuda::is_ready() ? "true" : "false") << '\n';
  std::cout << "cudnn_ready: " << (pluss::cuda::cudnn_ready() ? "true" : "false") << '\n';
  std::cout << "cudnn_conv_enabled: " << (pluss::cudnn_conv_enabled() ? "true" : "false") << '\n';
  std::cout << "cudnn_conv_strict: " << (pluss::cudnn_conv_strict() ? "true" : "false") << '\n';
  std::cout << "cudnn_conv_route_ready: " << (pluss::cudnn_conv_ready() ? "true" : "false") << '\n';

  const auto cpu_cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU);
  const auto x_cpu = pluss::rand({2, 4, 16, 16}, cpu_cfg, 301ULL);
  const auto w_cpu = pluss::rand({8, 2, 3, 3}, cpu_cfg, 303ULL);
  const auto b_cpu = pluss::zeros({8}, cpu_cfg);
  const auto y_cpu = pluss::conv(
      x_cpu,
      w_cpu,
      b_cpu,
      std::array<std::int64_t, 2>{1, 1},
      std::array<std::int64_t, 2>{1, 1},
      std::array<std::int64_t, 2>{1, 1},
      2);
  std::cout << "cpu conv shape: [" << y_cpu.sizes()[0] << "," << y_cpu.sizes()[1] << "," << y_cpu.sizes()[2] << "," << y_cpu.sizes()[3] << "]\n";

  if (pluss::cuda::is_ready() && pluss::cuda::cudnn_ready()) {
    const auto gpu_cfg = cpu_cfg.with_platform(pluss::pCUDA);
    const auto x_gpu = x_cpu.to(pluss::pCUDA);
    const auto w_gpu = w_cpu.to(pluss::pCUDA);
    const auto b_gpu = b_cpu.to(pluss::pCUDA);
    const auto y_gpu = pluss::conv(
        x_gpu,
        w_gpu,
        b_gpu,
        std::array<std::int64_t, 2>{1, 1},
        std::array<std::int64_t, 2>{1, 1},
        std::array<std::int64_t, 2>{1, 1},
        2);
    std::cout << "gpu conv shape: [" << y_gpu.sizes()[0] << "," << y_gpu.sizes()[1] << "," << y_gpu.sizes()[2] << "," << y_gpu.sizes()[3] << "]\n";
    (void)gpu_cfg;
  }

  return 0;
}

#include <pluss/pluss.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  std::cout << "cuda_ready=" << (pluss::cuda::is_ready() ? 1 : 0) << "\n";
  std::cout << "mps_ready=" << (pluss::mps::is_ready() ? 1 : 0) << "\n";
  std::cout << "cublas_ready=" << (pluss::cuda::cublas_available() ? 1 : 0) << "\n";
  std::cout << "fusion_cublas_ready=" << (pluss::cublas_fusion_ready() ? 1 : 0) << "\n";
  std::cout << "fusion_mps_ready=" << (pluss::mps_fusion_ready() ? 1 : 0) << "\n";
  if (!pluss::cuda::cublas_available()) {
    std::cout << "cublas_error=" << pluss::cuda::cublas_error() << "\n";
  }

  const Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const Series x = pluss::series::rand({4, 16}, cfg, 101ULL);
  const Series w = pluss::series::rand({8, 16}, cfg, 103ULL);
  const Series b = pluss::series::rand({8}, cfg, 107ULL);
  const Series y_cpu = pluss::fusion(x, w, b);
  std::cout << "cpu_fusion_mean=" << y_cpu.mean().item<double>() << "\n";

  if (pluss::cuda::is_ready()) {
    const Series y_cuda = pluss::fusion(x.to(pluss::pCUDA), w.to(pluss::pCUDA), b.to(pluss::pCUDA)).to(pluss::pCPU);
    std::cout << "cuda_fusion_mean=" << y_cuda.mean().item<double>() << "\n";
    std::cout << "cpu_cuda_max_abs_diff=" << y_cpu.sub(y_cuda).abs().max().item<double>() << "\n";
  } else {
    std::cout << "cuda_fusion_mean=skipped\n";
  }

  return 0;
}

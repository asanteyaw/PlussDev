#include <pluss/pluss.h>
#include <pluss/nn.h>

#include <cmath>
#include <iostream>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

bool run_fusion_route_checks(std::ostream* err = nullptr) {
  if (pluss::nn::cublas_fusion_ready() != pluss::cuda::cublas_available()) {
    if (err) *err << "cublas_fusion_ready must match cuda::cublas_available\n";
    return false;
  }

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::series::rand({5, 7}, cfg, 13ULL);
  const pluss::Series w = pluss::series::rand({3, 7}, cfg, 17ULL);
  const pluss::Series b = pluss::series::rand({3}, cfg, 19ULL);
  const pluss::Series y = pluss::nn::fusion(x, w, b);
  const pluss::Series y_ref = x.matmul(w.t()).add(b);
  const double max_abs = y.sub(y_ref).abs().max().item<double>();
  if (!(max_abs <= 1e-12)) {
    if (err) *err << "fusion affine path mismatch against matmul+bias reference\n";
    return false;
  }

  if (pluss::cuda::is_ready()) {
    const pluss::Series y_cuda = pluss::nn::fusion(x.to(pluss::pCUDA), w.to(pluss::pCUDA), b.to(pluss::pCUDA)).to(pluss::pCPU);
    const double max_abs_cuda = y.sub(y_cuda).abs().max().item<double>();
    if (!(max_abs_cuda <= 1e-6)) {
      if (err) *err << "CPU/CUDA fusion mismatch too large\n";
      return false;
    }
  }
  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("fusion route surface remains coherent") {
  REQUIRE(run_fusion_route_checks(&std::cerr));
}
#else
int main() {
  if (!run_fusion_route_checks(&std::cerr)) return 1;
  std::cout << "fusion_route_test passed\n";
  return 0;
}
#endif

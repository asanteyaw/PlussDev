#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

bool same_shape(const pluss::Series& x, std::initializer_list<std::int64_t> expected) {
  const auto sizes = x.sizes();
  if (sizes.size() != expected.size()) return false;
  std::size_t i = 0;
  for (const auto v : expected) {
    if (sizes[i++] != v) return false;
  }
  return true;
}

bool run_cudnn_attention_route_checks(std::ostream* err = nullptr) {
  if (pluss::nn::cudnn_attention_ready() && !pluss::cuda::cudnn_ready()) {
    if (err) *err << "cudnn_attention_ready implies cuda::cudnn_ready\n";
    return false;
  }

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::series::rand({2, 5, 8}, cfg, 401ULL);
  const auto [out_cpu, w_cpu] = pluss::nn::scaled_attention(x, x, x);
  if (!same_shape(out_cpu, {2, 5, 8}) || !same_shape(w_cpu, {2, 5, 5})) {
    if (err) *err << "scaled_attention CPU fallback shape mismatch\n";
    return false;
  }

  if (pluss::cuda::is_ready() && pluss::cuda::cpu_bridge_enabled()) {
    const pluss::Series x_cuda = x.to(pluss::pCUDA);
    const auto [out_cuda, w_cuda] = pluss::nn::scaled_attention(x_cuda, x_cuda, x_cuda);
    if (!same_shape(out_cuda, {2, 5, 8}) || !same_shape(w_cuda, {2, 5, 5})) {
      if (err) *err << "scaled_attention CUDA route shape mismatch\n";
      return false;
    }
  } else if (pluss::cuda::is_ready() && err != nullptr) {
    *err << "skipping CUDA attention route shape check: cuda::cpu_bridge_enabled() is false\n";
  }
  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("cudnn attention route stays coherent") {
  REQUIRE(run_cudnn_attention_route_checks(&std::cerr));
}
#else
int main() {
  if (!run_cudnn_attention_route_checks(&std::cerr)) return 1;
  std::cout << "cudnn_attention_route_test passed\n";
  return 0;
}
#endif

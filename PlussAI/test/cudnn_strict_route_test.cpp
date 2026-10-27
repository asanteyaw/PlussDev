#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

class ScopedEnvVar final {
public:
  ScopedEnvVar(const char* key, const char* value) : key_(key) {
    if (const char* old = std::getenv(key_)) {
      old_ = std::string(old);
    }
#if defined(_WIN32)
    _putenv_s(key_, value);
#else
    setenv(key_, value, 1);
#endif
  }

  ~ScopedEnvVar() {
    if (old_.has_value()) {
#if defined(_WIN32)
      _putenv_s(key_, old_->c_str());
#else
      setenv(key_, old_->c_str(), 1);
#endif
    } else {
#if defined(_WIN32)
      _putenv_s(key_, "");
#else
      unsetenv(key_);
#endif
    }
  }

private:
  const char* key_;
  std::optional<std::string> old_;
};

double max_abs_diff_(const std::vector<float>& a, const std::vector<float>& b) {
  const std::size_t n = std::min(a.size(), b.size());
  double m = 0.0;
  for (std::size_t i = 0; i < n; ++i) {
    m = std::max(m, std::fabs(static_cast<double>(a[i]) - static_cast<double>(b[i])));
  }
  return m;
}

} // namespace

bool run_cudnn_strict_route_checks(std::ostream* err = nullptr) {
  const ScopedEnvVar bridge_off("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "0");
  const ScopedEnvVar conv_on("PLUSS_CUDA_NATIVE_CUDNN_CONV", "1");
  const ScopedEnvVar conv_strict("PLUSS_CUDA_NATIVE_CUDNN_CONV_STRICT", "1");
  const ScopedEnvVar attn_on("PLUSS_CUDNN_ATTENTION", "1");
  const ScopedEnvVar attn_strict("PLUSS_CUDNN_ATTENTION_STRICT", "1");

  if (!pluss::cuda::is_ready() || !pluss::cuda::cudnn_ready()) {
    if (err) *err << "CUDA/cuDNN unavailable on this host\n";
    return true;
  }
  if (!pluss::nn::cudnn_conv_ready() || !pluss::nn::cudnn_attention_ready()) {
    if (err) *err << "cuDNN strict route unavailable on this host\n";
    return true;
  }

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU);

  // conv strict parity
  const auto x = pluss::series::rand({2, 4, 9, 9}, cfg, 9101ULL);
  const auto w = pluss::series::rand({8, 2, 3, 3}, cfg, 9103ULL);
  const auto b = pluss::series::rand({8}, cfg, 9107ULL);
  const auto y_cpu = pluss::nn::conv(x, w, b, {1, 1}, {1, 1}, {1, 1}, 2);
  const auto y_gpu = pluss::nn::conv(
      x.to(pluss::pCUDA),
      w.to(pluss::pCUDA),
      b.to(pluss::pCUDA),
      {1, 1},
      {1, 1},
      {1, 1},
      2);
  if (max_abs_diff_(y_cpu.to(pluss::pCPU, pluss::ScalarType::Float32).host<float>(),
                    y_gpu.to(pluss::pCPU, pluss::ScalarType::Float32).host<float>()) > 8e-3) {
    if (err) *err << "strict conv route parity exceeded tolerance\n";
    return false;
  }

  // attention strict parity
  const auto q = pluss::series::rand({2, 2, 6, 8}, cfg, 9111ULL);
  const auto k = pluss::series::rand({2, 2, 6, 8}, cfg, 9113ULL);
  const auto v = pluss::series::rand({2, 2, 6, 8}, cfg, 9117ULL);
  const auto mask = pluss::nn::square_subsequent_mask(6, q);
  const auto [out_cpu, w_cpu] = pluss::nn::scaled_attention(q, k, v, mask);
  const auto [out_gpu, w_gpu] = pluss::nn::scaled_attention(
      q.to(pluss::pCUDA),
      k.to(pluss::pCUDA),
      v.to(pluss::pCUDA),
      mask.to(pluss::pCUDA));
  if (max_abs_diff_(out_cpu.to(pluss::pCPU, pluss::ScalarType::Float32).host<float>(),
                    out_gpu.to(pluss::pCPU, pluss::ScalarType::Float32).host<float>()) > 8e-3) {
    if (err) *err << "strict attention output parity exceeded tolerance\n";
    return false;
  }
  if (max_abs_diff_(w_cpu.to(pluss::pCPU, pluss::ScalarType::Float32).host<float>(),
                    w_gpu.to(pluss::pCPU, pluss::ScalarType::Float32).host<float>()) > 8e-3) {
    if (err) *err << "strict attention weights parity exceeded tolerance\n";
    return false;
  }
  return true;
}

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("cuDNN strict conv/attention routes execute without bridge fallback", "[cudnn][strict]") {
  REQUIRE(run_cudnn_strict_route_checks(&std::cerr));
}
#else
int main() {
  if (!run_cudnn_strict_route_checks(&std::cerr)) return 1;
  std::cout << "cudnn_strict_route_test: ok\n";
  return 0;
}
#endif

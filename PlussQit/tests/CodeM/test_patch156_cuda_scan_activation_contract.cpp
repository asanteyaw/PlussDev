#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <vector>

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

void require_close_(const std::vector<float>& got, const std::vector<float>& want, float tol = 2e-4f) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(std::fabs(got[i] - want[i]) <= tol);
  }
}

void require_equal_u8_(const std::vector<std::uint8_t>& got, const std::vector<std::uint8_t>& want) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(got[i] == want[i]);
  }
}

} // namespace

TEST_CASE("CUDA scan baseline capability is runtime-dynamic", "[cuda][scan][runtime]") {
  const bool cuda_ready = pluss::cuda::is_ready();
  {
    const ScopedEnvVar baseline_off("PLUSS_CUDA_NATIVE_BASELINE", "0");
    const ScopedEnvVar scan_on("PLUSS_CUDA_NATIVE_SCAN", "1");
    REQUIRE(pluss::cuda::native_scan_enabled());
    REQUIRE_FALSE(pluss::cuda::supports_cumsum_scan_baseline(pluss::ScalarType::Float32));
    REQUIRE_FALSE(pluss::cuda::supports_cumprod_scan_baseline(pluss::ScalarType::Float32));
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    const ScopedEnvVar scan_off("PLUSS_CUDA_NATIVE_SCAN", "0");
    REQUIRE_FALSE(pluss::cuda::native_scan_enabled());
    REQUIRE_FALSE(pluss::cuda::supports_cumsum_scan_baseline(pluss::ScalarType::Float32));
    REQUIRE_FALSE(pluss::cuda::supports_cumprod_scan_baseline(pluss::ScalarType::Float32));
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    const ScopedEnvVar scan_on("PLUSS_CUDA_NATIVE_SCAN", "1");
    REQUIRE(pluss::cuda::native_scan_enabled());
    REQUIRE(pluss::cuda::supports_cumsum_scan_baseline(pluss::ScalarType::Float32) == cuda_ready);
    REQUIRE(pluss::cuda::supports_cumprod_scan_baseline(pluss::ScalarType::Int32) == cuda_ready);
  }
}

TEST_CASE("CUDA scan strict policy is runtime-dynamic", "[cuda][scan][policy][runtime]") {
  {
    const ScopedEnvVar strict_off("PLUSS_CUDA_NATIVE_SCAN_STRICT", "0");
    REQUIRE_FALSE(pluss::cuda::native_scan_strict());
  }
  {
    const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_SCAN_STRICT", "1");
    REQUIRE(pluss::cuda::native_scan_strict());
  }
}

TEST_CASE("CUDA bridge enables scan fallback when strict native is off", "[cuda][bridge][scan]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar scan_off("PLUSS_CUDA_NATIVE_SCAN", "0");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto x_cpu = pluss::make_series(std::vector<float>{1.f, 2.f, -1.f, 3.f, 2.f, -4.f}, {2, 3});
  const auto x_cuda = x_cpu.to(pluss::pCUDA);

  require_close_(
      x_cuda.cumsum(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<float>(),
      x_cpu.cumsum(/*dim=*/1, /*keepdim=*/false).to_vec<float>());
  require_close_(
      x_cuda.cumprod(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<float>(),
      x_cpu.cumprod(/*dim=*/1, /*keepdim=*/false).to_vec<float>());
}

TEST_CASE("CUDA strict native scan executes supported route", "[cuda][bridge][policy][scan]") {
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar scan_on("PLUSS_CUDA_NATIVE_SCAN", "1");
  const ScopedEnvVar scan_strict("PLUSS_CUDA_NATIVE_SCAN_STRICT", "1");
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto x_cpu = pluss::make_series(std::vector<float>{2.f, -1.f, 5.f, 3.f, -2.f, 4.f}, {2, 3});
  const auto x_cuda = x_cpu.to(pluss::pCUDA);

  require_close_(
      x_cuda.cumsum(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<float>(),
      x_cpu.cumsum(/*dim=*/1, /*keepdim=*/false).to_vec<float>());
  require_close_(
      x_cuda.cumprod(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<float>(),
      x_cpu.cumprod(/*dim=*/1, /*keepdim=*/false).to_vec<float>());
}

TEST_CASE("CUDA bridge parity covers activation and logical families", "[cuda][bridge][activation]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar elementwise_on("PLUSS_CUDA_NATIVE_ELEMENTWISE", "1");
  const ScopedEnvVar reduction_on("PLUSS_CUDA_NATIVE_REDUCTION", "1");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto x_cpu = pluss::make_series(std::vector<float>{1.f, -2.f, 3.f, 0.5f, -1.f, 4.f}, {2, 3});
  const auto x_cuda = x_cpu.to(pluss::pCUDA);
  const auto mask_cpu = pluss::make_series(std::vector<std::uint8_t>{1, 0, 1, 0, 1, 0}, {2, 3}).to(pluss::ScalarType::Bool);
  const auto mask_cuda = mask_cpu.to(pluss::pCUDA);
  const auto alt_cpu = pluss::make_series(std::vector<float>{10.f, 20.f, 30.f, 40.f, 50.f, 60.f}, {2, 3});
  const auto alt_cuda = alt_cpu.to(pluss::pCUDA);

  require_close_(x_cuda.relu().to(pluss::pCPU).to_vec<float>(), x_cpu.relu().to_vec<float>());
  require_close_(x_cuda.sigmoid().to(pluss::pCPU).to_vec<float>(), x_cpu.sigmoid().to_vec<float>());
  require_close_(x_cuda.softplus(1.0, 20.0).to(pluss::pCPU).to_vec<float>(), x_cpu.softplus(1.0, 20.0).to_vec<float>());
  require_close_(x_cuda.silu().to(pluss::pCPU).to_vec<float>(), x_cpu.silu().to_vec<float>());
  require_close_(x_cuda.gelu(true).to(pluss::pCPU).to_vec<float>(), x_cpu.gelu(true).to_vec<float>());
  require_close_(
      x_cuda.leaky_relu(0.1).to(pluss::pCPU).to_vec<float>(),
      x_cpu.leaky_relu(0.1).to_vec<float>());
  require_close_(x_cuda.softmax(1).to(pluss::pCPU).to_vec<float>(), x_cpu.softmax(1).to_vec<float>());
  require_close_(
      x_cuda.log_softmax(1).to(pluss::pCPU).to_vec<float>(),
      x_cpu.log_softmax(1).to_vec<float>());
  require_close_(
      x_cuda.logsumexp(1, false).to(pluss::pCPU).to_vec<float>(),
      x_cpu.logsumexp(1, false).to_vec<float>());
  require_close_(
      x_cuda.where(mask_cuda, alt_cuda).to(pluss::pCPU).to_vec<float>(),
      x_cpu.where(mask_cpu, alt_cpu).to_vec<float>());
  require_equal_u8_(
      mask_cuda.logical_and(mask_cuda.logical_not()).to(pluss::pCPU).to_vec<std::uint8_t>(),
      mask_cpu.logical_and(mask_cpu.logical_not()).to_vec<std::uint8_t>());
  require_equal_u8_(
      mask_cuda.logical_xor(mask_cuda.logical_not()).to(pluss::pCPU).to_vec<std::uint8_t>(),
      mask_cpu.logical_xor(mask_cpu.logical_not()).to_vec<std::uint8_t>());
}

TEST_CASE("CUDA strict policy covers activation and logical families", "[cuda][bridge][policy][activation]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar elementwise_on("PLUSS_CUDA_NATIVE_ELEMENTWISE", "1");
  const ScopedEnvVar elementwise_strict("PLUSS_CUDA_NATIVE_ELEMENTWISE_STRICT", "1");
  const ScopedEnvVar reduction_on("PLUSS_CUDA_NATIVE_REDUCTION", "1");
  const ScopedEnvVar reduction_strict("PLUSS_CUDA_NATIVE_REDUCTION_STRICT", "1");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto x_cpu = pluss::make_series(std::vector<float>{2.f, -1.f, 5.f, 3.f}, {2, 2});
  const auto x_cuda = x_cpu.to(pluss::pCUDA);
  const auto mask_cpu = pluss::make_series(std::vector<std::uint8_t>{1, 0, 0, 1}, {2, 2}).to(pluss::ScalarType::Bool);
  const auto mask_cuda = mask_cpu.to(pluss::pCUDA);
  const auto y_cpu = pluss::make_series(std::vector<float>{10.f, 20.f, 30.f, 40.f}, {2, 2});
  const auto y_cuda = y_cpu.to(pluss::pCUDA);

  require_close_(x_cuda.relu().to(pluss::pCPU).to_vec<float>(), x_cpu.relu().to_vec<float>());
  require_close_(x_cuda.sigmoid().to(pluss::pCPU).to_vec<float>(), x_cpu.sigmoid().to_vec<float>());
  require_close_(x_cuda.gelu(false).to(pluss::pCPU).to_vec<float>(), x_cpu.gelu(false).to_vec<float>());
  require_close_(x_cuda.softmax(1).to(pluss::pCPU).to_vec<float>(), x_cpu.softmax(1).to_vec<float>());
  require_close_(
      x_cuda.where(mask_cuda, y_cuda).to(pluss::pCPU).to_vec<float>(),
      x_cpu.where(mask_cpu, y_cpu).to_vec<float>());
  require_equal_u8_(
      mask_cuda.logical_or(mask_cuda.logical_not()).to(pluss::pCPU).to_vec<std::uint8_t>(),
      mask_cpu.logical_or(mask_cpu.logical_not()).to_vec<std::uint8_t>());
}


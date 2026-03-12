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

void require_close_(const std::vector<float>& got, const std::vector<float>& want, float tol = 1e-5f) {
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

TEST_CASE("CUDA elementwise baseline capability is runtime-dynamic", "[cuda][elementwise][runtime]") {
  const bool cuda_ready = pluss::cuda::is_ready();
  {
    const ScopedEnvVar baseline_off("PLUSS_CUDA_NATIVE_BASELINE", "0");
    const ScopedEnvVar elementwise_on("PLUSS_CUDA_NATIVE_ELEMENTWISE", "1");
    REQUIRE(pluss::cuda::native_elementwise_enabled());
    REQUIRE_FALSE(pluss::cuda::supports_unary_elementwise_baseline(pluss::ScalarType::Float32));
    REQUIRE_FALSE(
        pluss::cuda::supports_binary_elementwise_baseline(pluss::ScalarType::Float32, pluss::ScalarType::Float32));
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    const ScopedEnvVar elementwise_off("PLUSS_CUDA_NATIVE_ELEMENTWISE", "0");
    REQUIRE_FALSE(pluss::cuda::native_elementwise_enabled());
    REQUIRE_FALSE(pluss::cuda::supports_unary_elementwise_baseline(pluss::ScalarType::Float32));
    REQUIRE_FALSE(
        pluss::cuda::supports_binary_elementwise_baseline(pluss::ScalarType::Float32, pluss::ScalarType::Float32));
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    const ScopedEnvVar elementwise_on("PLUSS_CUDA_NATIVE_ELEMENTWISE", "1");
    REQUIRE(pluss::cuda::native_elementwise_enabled());
    REQUIRE(pluss::cuda::supports_unary_elementwise_baseline(pluss::ScalarType::Float32) == cuda_ready);
    REQUIRE(pluss::cuda::supports_unary_elementwise_baseline(pluss::ScalarType::Complex64) == cuda_ready);
    REQUIRE(pluss::cuda::supports_binary_elementwise_baseline(
                pluss::ScalarType::Float16,
                pluss::ScalarType::Float16) == cuda_ready);
    REQUIRE_FALSE(
        pluss::cuda::supports_binary_elementwise_baseline(pluss::ScalarType::Float32, pluss::ScalarType::Int64));
  }
}

TEST_CASE("CUDA elementwise strict policy is runtime-dynamic", "[cuda][elementwise][policy][runtime]") {
  {
    const ScopedEnvVar strict_off("PLUSS_CUDA_NATIVE_ELEMENTWISE_STRICT", "0");
    REQUIRE_FALSE(pluss::cuda::native_elementwise_strict());
  }
  {
    const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_ELEMENTWISE_STRICT", "1");
    REQUIRE(pluss::cuda::native_elementwise_strict());
  }
}

TEST_CASE("CUDA elementwise route contract rejects non-CUDA tensors", "[cuda][elementwise][policy]") {
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar elementwise_on("PLUSS_CUDA_NATIVE_ELEMENTWISE", "1");

  const auto x_cpu = pluss::make_series(std::vector<float>{1.f, -2.f, 3.f}, {3});
  const auto y_cpu = pluss::make_series(std::vector<float>{2.f, 4.f, -1.f}, {3});
  REQUIRE_FALSE(pluss::cuda::supports_unary_elementwise(x_cpu));
  REQUIRE_FALSE(pluss::cuda::supports_binary_elementwise(x_cpu, y_cpu));
  REQUIRE_FALSE(pluss::cuda::supports_comparison_elementwise(x_cpu, y_cpu));
}

TEST_CASE("CUDA bridge enables elementwise fallback when strict native is off", "[cuda][bridge][elementwise]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar elementwise_off("PLUSS_CUDA_NATIVE_ELEMENTWISE", "0");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto x_cpu = pluss::make_series(std::vector<float>{1.f, -2.f, 3.f, 0.5f}, {2, 2});
  const auto y_cpu = pluss::make_series(std::vector<float>{2.f, 4.f, -1.f, 2.f}, {2, 2});
  const auto x_cuda = x_cpu.to(pluss::pCUDA);
  const auto y_cuda = y_cpu.to(pluss::pCUDA);

  require_close_(x_cuda.add(y_cuda).to(pluss::pCPU).to_vec<float>(), x_cpu.add(y_cpu).to_vec<float>());
  require_close_(x_cuda.mul(y_cuda).to(pluss::pCPU).to_vec<float>(), x_cpu.mul(y_cpu).to_vec<float>());
  require_close_(x_cuda.neg().to(pluss::pCPU).to_vec<float>(), x_cpu.neg().to_vec<float>());
  require_close_(x_cuda.abs().to(pluss::pCPU).to_vec<float>(), x_cpu.abs().to_vec<float>());
  require_equal_u8_(x_cuda.gt(y_cuda).to(pluss::pCPU).to_vec<std::uint8_t>(),
                    x_cpu.gt(y_cpu).to_vec<std::uint8_t>());
}

TEST_CASE("CUDA strict native elementwise executes supported native route", "[cuda][bridge][policy][elementwise]") {
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar elementwise_on("PLUSS_CUDA_NATIVE_ELEMENTWISE", "1");
  const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_ELEMENTWISE_STRICT", "1");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto x_cpu = pluss::make_series(std::vector<float>{2.f, -1.f, 5.f, 3.f}, {2, 2});
  const auto y_cpu = pluss::make_series(std::vector<float>{1.f, 4.f, -2.f, 5.f}, {2, 2});
  const auto x_cuda = x_cpu.to(pluss::pCUDA);
  const auto y_cuda = y_cpu.to(pluss::pCUDA);

  require_close_(x_cuda.sub(y_cuda).to(pluss::pCPU).to_vec<float>(), x_cpu.sub(y_cpu).to_vec<float>());
  require_close_(x_cuda.div(y_cuda).to(pluss::pCPU).to_vec<float>(), x_cpu.div(y_cpu).to_vec<float>());
  require_close_(x_cuda.expm1().to(pluss::pCPU).to_vec<float>(), x_cpu.expm1().to_vec<float>());
  require_equal_u8_(x_cuda.eq(y_cuda).to(pluss::pCPU).to_vec<std::uint8_t>(),
                    x_cpu.eq(y_cpu).to_vec<std::uint8_t>());
}

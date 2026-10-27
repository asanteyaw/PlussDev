#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cmath>
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

} // namespace

TEST_CASE("CUDA matmul baseline capability is runtime-dynamic", "[cuda][matmul][runtime]") {
  const bool cuda_ready = pluss::cuda::is_ready();
  {
    const ScopedEnvVar baseline_off("PLUSS_CUDA_NATIVE_BASELINE", "0");
    const ScopedEnvVar matmul_on("PLUSS_CUDA_NATIVE_MATMUL", "1");
    REQUIRE(pluss::cuda::native_matmul_enabled());
    REQUIRE_FALSE(pluss::cuda::supports_matmul_native_baseline(pluss::ScalarType::Float32, pluss::ScalarType::Float32));
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    const ScopedEnvVar matmul_off("PLUSS_CUDA_NATIVE_MATMUL", "0");
    REQUIRE_FALSE(pluss::cuda::native_matmul_enabled());
    REQUIRE_FALSE(pluss::cuda::supports_matmul_native_baseline(pluss::ScalarType::Float32, pluss::ScalarType::Float32));
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    const ScopedEnvVar matmul_on("PLUSS_CUDA_NATIVE_MATMUL", "1");
    REQUIRE(pluss::cuda::native_matmul_enabled());
    REQUIRE(
        pluss::cuda::supports_matmul_native_baseline(pluss::ScalarType::Float32, pluss::ScalarType::Float32) ==
        cuda_ready);
    REQUIRE(
        pluss::cuda::supports_matmul_native_baseline(pluss::ScalarType::Float64, pluss::ScalarType::Float64) ==
        cuda_ready);
    REQUIRE_FALSE(pluss::cuda::supports_matmul_native_baseline(pluss::ScalarType::Int64, pluss::ScalarType::Int64));
  }
}

TEST_CASE("CUDA matmul strict policy is runtime-dynamic", "[cuda][matmul][policy][runtime]") {
  {
    const ScopedEnvVar strict_off("PLUSS_CUDA_NATIVE_MATMUL_STRICT", "0");
    REQUIRE_FALSE(pluss::cuda::native_matmul_strict());
  }
  {
    const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_MATMUL_STRICT", "1");
    REQUIRE(pluss::cuda::native_matmul_strict());
  }
}

TEST_CASE("CUDA matmul route contract rejects non-CUDA tensors", "[cuda][matmul][policy]") {
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar matmul_on("PLUSS_CUDA_NATIVE_MATMUL", "1");
  const auto a_cpu = pluss::make_series(std::vector<float>{1.f, 2.f, 3.f, 4.f}, {2, 2});
  const auto b_cpu = pluss::make_series(std::vector<float>{2.f, 0.f, 1.f, 3.f}, {2, 2});
  REQUIRE(a_cpu.platform() == pluss::pCPU);
  REQUIRE_FALSE(pluss::cuda::supports_matmul_native(a_cpu, b_cpu));
}

TEST_CASE("CUDA bridge enables matmul fallback when strict native is off", "[cuda][bridge][matmul]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar matmul_off("PLUSS_CUDA_NATIVE_MATMUL", "0");
  const bool cuda_ready = pluss::cuda::is_ready();
  if (!cuda_ready) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto a_cpu = pluss::make_series(std::vector<float>{1.f, 2.f, 3.f, 4.f, 5.f, 6.f}, {2, 3});
  const auto b_cpu = pluss::make_series(std::vector<float>{7.f, 8.f, 9.f, 10.f, 11.f, 12.f}, {3, 2});
  const auto a_cuda = a_cpu.to(pluss::pCUDA);
  const auto b_cuda = b_cpu.to(pluss::pCUDA);

  const auto got = a_cuda.matmul(b_cuda);
  REQUIRE(got.platform() == pluss::pCUDA);
  require_close_(got.to(pluss::pCPU).host<float>(), a_cpu.matmul(b_cpu).host<float>());
}

TEST_CASE("CUDA strict native matmul executes supported native route", "[cuda][bridge][policy][matmul]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar matmul_on("PLUSS_CUDA_NATIVE_MATMUL", "1");
  const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_MATMUL_STRICT", "1");
  const bool cuda_ready = pluss::cuda::is_ready();
  if (!cuda_ready) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto a_cpu = pluss::make_series(std::vector<float>{1.f, 2.f, 3.f, 4.f}, {2, 2});
  const auto b_cpu = pluss::make_series(std::vector<float>{2.f, 0.f, 1.f, 3.f}, {2, 2});
  const auto a_cuda = a_cpu.to(pluss::pCUDA);
  const auto b_cuda = b_cpu.to(pluss::pCUDA);
  const auto got = a_cuda.matmul(b_cuda);
  REQUIRE(got.platform() == pluss::pCUDA);
  require_close_(got.to(pluss::pCPU).host<float>(), a_cpu.matmul(b_cpu).host<float>());
}

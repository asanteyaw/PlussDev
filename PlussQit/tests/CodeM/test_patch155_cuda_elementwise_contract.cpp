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
    REQUIRE_FALSE(pluss::cuda::supports_unary_elementwise_baseline(pluss::ScalarType::Complex64));
    REQUIRE_FALSE(pluss::cuda::supports_binary_elementwise_baseline(
        pluss::ScalarType::Float16,
        pluss::ScalarType::Float16));
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

  require_close_(x_cuda.add(y_cuda).to(pluss::pCPU).host<float>(), x_cpu.add(y_cpu).host<float>());
  require_close_(x_cuda.mul(y_cuda).to(pluss::pCPU).host<float>(), x_cpu.mul(y_cpu).host<float>());
  require_close_(x_cuda.neg().to(pluss::pCPU).host<float>(), x_cpu.neg().host<float>());
  require_close_(x_cuda.abs().to(pluss::pCPU).host<float>(), x_cpu.abs().host<float>());
  require_equal_u8_(x_cuda.gt(y_cuda).to(pluss::pCPU).host<std::uint8_t>(),
                    x_cpu.gt(y_cpu).host<std::uint8_t>());
}

TEST_CASE("CUDA strict native elementwise executes supported native route", "[cuda][bridge][policy][elementwise]") {
  const ScopedEnvVar bridge_off("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "0");
  const ScopedEnvVar forbid_materialize("PLUSS_CUDA_FORBID_CPU_MATERIALIZATION", "1");
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar elementwise_on("PLUSS_CUDA_NATIVE_ELEMENTWISE", "1");
  const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_ELEMENTWISE_STRICT", "1");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto x_cpu = pluss::make_series(std::vector<float>{2.f, -1.f, 5.f, 3.f}, {2, 2});
  const auto y_cpu = pluss::make_series(std::vector<float>{1.f, 4.f, -2.f, 5.f}, {2, 2});
  const auto z_cpu = pluss::make_series(std::vector<float>{0.25f, 1.0f, 2.25f, 4.0f}, {2, 2});
  const auto frac_cpu = pluss::make_series(std::vector<float>{-1.7f, -0.2f, 1.2f, 2.8f}, {2, 2});
  const auto x_cuda = x_cpu.to(pluss::pCUDA);
  const auto y_cuda = y_cpu.to(pluss::pCUDA);
  const auto z_cuda = z_cpu.to(pluss::pCUDA);
  const auto frac_cuda = frac_cpu.to(pluss::pCUDA);

  const auto sub = x_cuda.sub(y_cuda);
  const auto div = x_cuda.div(y_cuda);
  const auto expm1 = x_cuda.expm1();
  const auto eq = x_cuda.eq(y_cuda);
  const auto sqrt = z_cuda.sqrt();
  const auto exp = z_cuda.exp();
  const auto log = z_cuda.log();
  const auto sin = z_cuda.sin();
  const auto cos = z_cuda.cos();
  const auto tanh = z_cuda.tanh();
  const auto floor = frac_cuda.floor();
  const auto ceil = frac_cuda.ceil();
  const auto round = frac_cuda.round();
  const auto trunc = frac_cuda.trunc();
  const auto reciprocal = z_cuda.reciprocal();

  REQUIRE(sub.platform() == pluss::pCUDA);
  REQUIRE(div.platform() == pluss::pCUDA);
  REQUIRE(expm1.platform() == pluss::pCUDA);
  REQUIRE(eq.platform() == pluss::pCUDA);
  REQUIRE(sqrt.platform() == pluss::pCUDA);
  REQUIRE(exp.platform() == pluss::pCUDA);
  REQUIRE(log.platform() == pluss::pCUDA);
  REQUIRE(sin.platform() == pluss::pCUDA);
  REQUIRE(cos.platform() == pluss::pCUDA);
  REQUIRE(tanh.platform() == pluss::pCUDA);
  REQUIRE(floor.platform() == pluss::pCUDA);
  REQUIRE(ceil.platform() == pluss::pCUDA);
  REQUIRE(round.platform() == pluss::pCUDA);
  REQUIRE(trunc.platform() == pluss::pCUDA);
  REQUIRE(reciprocal.platform() == pluss::pCUDA);
  REQUIRE(sub.storage().location() == pluss::codem::StorageLocation::CudaDevice);
  REQUIRE(sqrt.storage().location() == pluss::codem::StorageLocation::CudaDevice);
  REQUIRE(reciprocal.storage().location() == pluss::codem::StorageLocation::CudaDevice);

  const ScopedEnvVar readback_bridge("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar allow_materialize("PLUSS_CUDA_FORBID_CPU_MATERIALIZATION", "0");
  require_close_(sub.to(pluss::pCPU).host<float>(), x_cpu.sub(y_cpu).host<float>());
  require_close_(div.to(pluss::pCPU).host<float>(), x_cpu.div(y_cpu).host<float>());
  require_close_(expm1.to(pluss::pCPU).host<float>(), x_cpu.expm1().host<float>());
  require_close_(sqrt.to(pluss::pCPU).host<float>(), z_cpu.sqrt().host<float>());
  require_close_(exp.to(pluss::pCPU).host<float>(), z_cpu.exp().host<float>());
  require_close_(log.to(pluss::pCPU).host<float>(), z_cpu.log().host<float>());
  require_close_(sin.to(pluss::pCPU).host<float>(), z_cpu.sin().host<float>());
  require_close_(cos.to(pluss::pCPU).host<float>(), z_cpu.cos().host<float>());
  require_close_(tanh.to(pluss::pCPU).host<float>(), z_cpu.tanh().host<float>());
  require_close_(floor.to(pluss::pCPU).host<float>(), frac_cpu.floor().host<float>());
  require_close_(ceil.to(pluss::pCPU).host<float>(), frac_cpu.ceil().host<float>());
  require_close_(round.to(pluss::pCPU).host<float>(), frac_cpu.round().host<float>());
  require_close_(trunc.to(pluss::pCPU).host<float>(), frac_cpu.trunc().host<float>());
  require_close_(reciprocal.to(pluss::pCPU).host<float>(), z_cpu.reciprocal().host<float>());
  require_equal_u8_(eq.to(pluss::pCPU).host<std::uint8_t>(),
                    x_cpu.eq(y_cpu).host<std::uint8_t>());
}

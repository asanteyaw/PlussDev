#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

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

template <class Got, class Want>
void require_equal_(const Got& got, const Want& want) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(got[i] == want[i]);
  }
}

} // namespace

TEST_CASE("CUDA bitwise baseline capability is runtime-dynamic", "[cuda][bitwise][runtime]") {
  const bool cuda_ready = pluss::cuda::is_ready();
  {
    const ScopedEnvVar baseline_off("PLUSS_CUDA_NATIVE_BASELINE", "0");
    const ScopedEnvVar bitwise_on("PLUSS_CUDA_NATIVE_BITWISE", "1");
    REQUIRE(pluss::cuda::native_bitwise_enabled());
    REQUIRE_FALSE(pluss::cuda::supports_bitwise_unary_baseline(pluss::ScalarType::Int32));
    REQUIRE_FALSE(
        pluss::cuda::supports_bitwise_binary_baseline(pluss::ScalarType::Int32, pluss::ScalarType::Int32));
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    const ScopedEnvVar bitwise_off("PLUSS_CUDA_NATIVE_BITWISE", "0");
    REQUIRE_FALSE(pluss::cuda::native_bitwise_enabled());
    REQUIRE_FALSE(pluss::cuda::supports_bitwise_unary_baseline(pluss::ScalarType::UInt8));
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    const ScopedEnvVar bitwise_on("PLUSS_CUDA_NATIVE_BITWISE", "1");
    REQUIRE(pluss::cuda::native_bitwise_enabled());
    REQUIRE(pluss::cuda::supports_bitwise_unary_baseline(pluss::ScalarType::Int16) == cuda_ready);
    REQUIRE(pluss::cuda::supports_bitwise_binary_baseline(
                pluss::ScalarType::UInt8,
                pluss::ScalarType::UInt8) == cuda_ready);
    REQUIRE_FALSE(pluss::cuda::supports_bitwise_binary_baseline(
        pluss::ScalarType::UInt8,
        pluss::ScalarType::Int8));
  }
}

TEST_CASE("CUDA bitwise strict policy is runtime-dynamic", "[cuda][bitwise][policy][runtime]") {
  {
    const ScopedEnvVar strict_off("PLUSS_CUDA_NATIVE_BITWISE_STRICT", "0");
    REQUIRE_FALSE(pluss::cuda::native_bitwise_strict());
  }
  {
    const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_BITWISE_STRICT", "1");
    REQUIRE(pluss::cuda::native_bitwise_strict());
  }
}

TEST_CASE("CUDA bitwise route contract rejects non-CUDA tensors", "[cuda][bitwise][policy]") {
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar bitwise_on("PLUSS_CUDA_NATIVE_BITWISE", "1");
  const auto x_cpu = pluss::make_series(std::vector<std::int32_t>{1, 2, 3, 4}, {2, 2});
  const auto y_cpu = pluss::make_series(std::vector<std::int32_t>{5, 6, 7, 8}, {2, 2});
  REQUIRE_FALSE(pluss::cuda::supports_bitwise_unary(x_cpu));
  REQUIRE_FALSE(pluss::cuda::supports_bitwise_binary(x_cpu, y_cpu));
}

TEST_CASE("CUDA bridge enables bitwise fallback when strict native is off", "[cuda][bridge][bitwise]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar bitwise_off("PLUSS_CUDA_NATIVE_BITWISE", "0");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto x_cpu = pluss::make_series(std::vector<std::int32_t>{1, 2, 3, 4}, {2, 2});
  const auto y_cpu = pluss::make_series(std::vector<std::int32_t>{5, 6, 7, 8}, {2, 2});
  const auto x_cuda = x_cpu.to(pluss::pCUDA);
  const auto y_cuda = y_cpu.to(pluss::pCUDA);

  require_equal_(
      x_cuda.bitwise_and(y_cuda).to(pluss::pCPU).host<std::int32_t>(),
      x_cpu.bitwise_and(y_cpu).host<std::int32_t>());
  require_equal_(
      x_cuda.bitwise_or(y_cuda).to(pluss::pCPU).host<std::int32_t>(),
      x_cpu.bitwise_or(y_cpu).host<std::int32_t>());
  require_equal_(
      x_cuda.bitwise_not().to(pluss::pCPU).host<std::int32_t>(),
      x_cpu.bitwise_not().host<std::int32_t>());
}

TEST_CASE("CUDA strict native bitwise executes supported route", "[cuda][bridge][policy][bitwise]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar bitwise_on("PLUSS_CUDA_NATIVE_BITWISE", "1");
  const ScopedEnvVar bitwise_strict("PLUSS_CUDA_NATIVE_BITWISE_STRICT", "1");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto x_cpu = pluss::make_series(std::vector<std::uint8_t>{1, 2, 3, 4}, {2, 2});
  const auto y_cpu = pluss::make_series(std::vector<std::uint8_t>{9, 8, 7, 6}, {2, 2});
  const auto x_cuda = x_cpu.to(pluss::pCUDA);
  const auto y_cuda = y_cpu.to(pluss::pCUDA);

  require_equal_(
      x_cuda.bitwise_xor(y_cuda).to(pluss::pCPU).host<std::uint8_t>(),
      x_cpu.bitwise_xor(y_cpu).host<std::uint8_t>());
}

TEST_CASE("CUDA compose baseline capability is runtime-dynamic", "[cuda][compose][runtime]") {
  const bool cuda_ready = pluss::cuda::is_ready();
  {
    const ScopedEnvVar baseline_off("PLUSS_CUDA_NATIVE_BASELINE", "0");
    const ScopedEnvVar compose_on("PLUSS_CUDA_NATIVE_COMPOSE", "1");
    REQUIRE(pluss::cuda::native_compose_enabled());
    REQUIRE_FALSE(pluss::cuda::supports_compose_baseline(pluss::ScalarType::Float32));
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    const ScopedEnvVar compose_off("PLUSS_CUDA_NATIVE_COMPOSE", "0");
    REQUIRE_FALSE(pluss::cuda::native_compose_enabled());
    REQUIRE_FALSE(pluss::cuda::supports_compose_baseline(pluss::ScalarType::Float32));
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    const ScopedEnvVar compose_on("PLUSS_CUDA_NATIVE_COMPOSE", "1");
    REQUIRE(pluss::cuda::native_compose_enabled());
    REQUIRE(pluss::cuda::supports_compose_baseline(pluss::ScalarType::Float64) == cuda_ready);
  }
}

TEST_CASE("CUDA compose strict policy is runtime-dynamic", "[cuda][compose][policy][runtime]") {
  {
    const ScopedEnvVar strict_off("PLUSS_CUDA_NATIVE_COMPOSE_STRICT", "0");
    REQUIRE_FALSE(pluss::cuda::native_compose_strict());
  }
  {
    const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_COMPOSE_STRICT", "1");
    REQUIRE(pluss::cuda::native_compose_strict());
  }
}

TEST_CASE("CUDA compose route contract rejects non-CUDA tensors", "[cuda][compose][policy]") {
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar compose_on("PLUSS_CUDA_NATIVE_COMPOSE", "1");
  const auto x_cpu = pluss::make_series(std::vector<float>{1.f, 2.f, 3.f, 4.f}, {2, 2});
  const auto y_cpu = pluss::make_series(std::vector<float>{5.f, 6.f, 7.f, 8.f}, {2, 2});
  const std::vector<pluss::Series> others{y_cpu};
  REQUIRE_FALSE(pluss::cuda::supports_unite_compose(x_cpu, others, 1));
  REQUIRE_FALSE(pluss::cuda::supports_pile_compose(x_cpu, others, 0));
}

TEST_CASE("CUDA bridge enables compose fallback when strict native is off", "[cuda][bridge][compose]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar compose_off("PLUSS_CUDA_NATIVE_COMPOSE", "0");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto x_cpu = pluss::make_series(std::vector<float>{1.f, 2.f, 3.f, 4.f}, {2, 2});
  const auto y_cpu = pluss::make_series(std::vector<float>{5.f, 6.f, 7.f, 8.f}, {2, 2});
  const auto x_cuda = x_cpu.to(pluss::pCUDA);
  const auto y_cuda = y_cpu.to(pluss::pCUDA);

  require_equal_(
      x_cuda.unite({y_cuda}, 1).to(pluss::pCPU).host<float>(),
      x_cpu.unite({y_cpu}, 1).host<float>());
  require_equal_(
      x_cuda.pile({y_cuda}, 0).to(pluss::pCPU).host<float>(),
      x_cpu.pile({y_cpu}, 0).host<float>());
}

TEST_CASE("CUDA strict native compose executes supported route", "[cuda][bridge][policy][compose]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar compose_on("PLUSS_CUDA_NATIVE_COMPOSE", "1");
  const ScopedEnvVar compose_strict("PLUSS_CUDA_NATIVE_COMPOSE_STRICT", "1");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto x_cpu = pluss::make_series(std::vector<std::int32_t>{1, 2, 3, 4}, {2, 2});
  const auto y_cpu = pluss::make_series(std::vector<std::int32_t>{5, 6, 7, 8}, {2, 2});
  const auto x_cuda = x_cpu.to(pluss::pCUDA);
  const auto y_cuda = y_cpu.to(pluss::pCUDA);

  require_equal_(
      x_cuda.unite({y_cuda}, 0).to(pluss::pCPU).host<std::int32_t>(),
      x_cpu.unite({y_cpu}, 0).host<std::int32_t>());
}


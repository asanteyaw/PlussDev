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

} // namespace

TEST_CASE("CUDA masking baseline capability is runtime-dynamic", "[cuda][masking][runtime]") {
  const bool cuda_ready = pluss::cuda::is_ready();
  {
    const ScopedEnvVar baseline_off("PLUSS_CUDA_NATIVE_BASELINE", "0");
    const ScopedEnvVar masking_on("PLUSS_CUDA_NATIVE_MASKING", "1");
    REQUIRE(pluss::cuda::native_masking_enabled());
    REQUIRE_FALSE(pluss::cuda::supports_masked_select_baseline(pluss::ScalarType::Float32, pluss::ScalarType::Bool));
    REQUIRE_FALSE(pluss::cuda::supports_masked_fill_baseline(pluss::ScalarType::Float32, pluss::ScalarType::Bool));
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    const ScopedEnvVar masking_off("PLUSS_CUDA_NATIVE_MASKING", "0");
    REQUIRE_FALSE(pluss::cuda::native_masking_enabled());
    REQUIRE_FALSE(pluss::cuda::supports_masked_scatter_baseline(
        pluss::ScalarType::Float32,
        pluss::ScalarType::Bool,
        pluss::ScalarType::Float32));
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    const ScopedEnvVar masking_on("PLUSS_CUDA_NATIVE_MASKING", "1");
    REQUIRE(pluss::cuda::native_masking_enabled());
    REQUIRE(pluss::cuda::supports_masked_select_baseline(pluss::ScalarType::Float32, pluss::ScalarType::Bool) == cuda_ready);
    REQUIRE(pluss::cuda::supports_masked_fill_baseline(pluss::ScalarType::Int32, pluss::ScalarType::Bool) == cuda_ready);
    REQUIRE(pluss::cuda::supports_masked_scatter_baseline(
                pluss::ScalarType::Float64,
                pluss::ScalarType::Bool,
                pluss::ScalarType::Float64) == cuda_ready);
  }
}

TEST_CASE("CUDA masking strict policy is runtime-dynamic", "[cuda][masking][policy][runtime]") {
  {
    const ScopedEnvVar strict_off("PLUSS_CUDA_NATIVE_MASKING_STRICT", "0");
    REQUIRE_FALSE(pluss::cuda::native_masking_strict());
  }
  {
    const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_MASKING_STRICT", "1");
    REQUIRE(pluss::cuda::native_masking_strict());
  }
}

TEST_CASE("CUDA masking route contract rejects non-CUDA tensors", "[cuda][masking][policy]") {
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar masking_on("PLUSS_CUDA_NATIVE_MASKING", "1");

  const auto x_cpu = pluss::make_series(std::vector<float>{1.f, -2.f, 3.f}, {3});
  const auto m_cpu = pluss::make_series(std::vector<std::uint8_t>{1, 0, 1}, {3}).to(pluss::ScalarType::Bool);
  const auto v_cpu = pluss::make_series(std::vector<float>{0.f}, {1});
  const auto s_cpu = pluss::make_series(std::vector<float>{9.f, 7.f}, {2});

  REQUIRE_FALSE(pluss::cuda::supports_masked_select(x_cpu, m_cpu));
  REQUIRE_FALSE(pluss::cuda::supports_masked_fill(x_cpu, m_cpu, v_cpu));
  REQUIRE_FALSE(pluss::cuda::supports_masked_scatter(x_cpu, m_cpu, s_cpu));
}

TEST_CASE("CUDA bridge enables masking fallback when strict native is off", "[cuda][bridge][masking]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar masking_off("PLUSS_CUDA_NATIVE_MASKING", "0");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto x_cpu = pluss::make_series(std::vector<float>{1.f, -2.f, 3.f, 0.5f, 4.f, -1.f}, {2, 3});
  const auto m_cpu = pluss::make_series(std::vector<std::uint8_t>{1, 0, 1, 0, 1, 0}, {2, 3}).to(pluss::ScalarType::Bool);
  const auto src_cpu = pluss::make_series(std::vector<float>{9.f, 8.f, 7.f}, {3});

  const auto x_cuda = x_cpu.to(pluss::pCUDA);
  const auto m_cuda = m_cpu.to(pluss::pCUDA);
  const auto src_cuda = src_cpu.to(pluss::pCUDA);

  require_close_(
      x_cuda.masked_select(m_cuda).to(pluss::pCPU).host<float>(),
      x_cpu.masked_select(m_cpu).host<float>());
  require_close_(
      x_cuda.masked_fill(m_cuda, -5.0).to(pluss::pCPU).host<float>(),
      x_cpu.masked_fill(m_cpu, -5.0).host<float>());
  require_close_(
      x_cuda.masked_scatter(m_cuda, src_cuda).to(pluss::pCPU).host<float>(),
      x_cpu.masked_scatter(m_cpu, src_cpu).host<float>());
}

TEST_CASE("CUDA strict native masking executes supported route", "[cuda][bridge][policy][masking]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar masking_on("PLUSS_CUDA_NATIVE_MASKING", "1");
  const ScopedEnvVar masking_strict("PLUSS_CUDA_NATIVE_MASKING_STRICT", "1");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto x_cpu = pluss::make_series(std::vector<float>{2.f, -1.f, 5.f, 3.f, 4.f, -2.f}, {2, 3});
  const auto m_cpu = pluss::make_series(std::vector<std::uint8_t>{0, 1, 1, 0, 0, 1}, {2, 3}).to(pluss::ScalarType::Bool);
  const auto src_cpu = pluss::make_series(std::vector<float>{6.f, 7.f, 8.f}, {3});

  const auto x_cuda = x_cpu.to(pluss::pCUDA);
  const auto m_cuda = m_cpu.to(pluss::pCUDA);
  const auto src_cuda = src_cpu.to(pluss::pCUDA);

  require_close_(
      x_cuda.masked_select(m_cuda).to(pluss::pCPU).host<float>(),
      x_cpu.masked_select(m_cpu).host<float>());
  require_close_(
      x_cuda.masked_fill(m_cuda, 4.0).to(pluss::pCPU).host<float>(),
      x_cpu.masked_fill(m_cpu, 4.0).host<float>());
  require_close_(
      x_cuda.masked_scatter(m_cuda, src_cuda).to(pluss::pCPU).host<float>(),
      x_cpu.masked_scatter(m_cpu, src_cpu).host<float>());
}


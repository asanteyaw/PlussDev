#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <array>
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

std::vector<float> to_cpu_f32_(const pluss::Series& x) {
  return x.to(pluss::pCPU).to(pluss::ScalarType::Float32).contiguous().to_vec<float>();
}

std::vector<std::int64_t> to_cpu_i64_(const pluss::Series& x) {
  return x.to(pluss::pCPU).to(pluss::ScalarType::Int64).contiguous().to_vec<std::int64_t>();
}

void require_close_f32_(const pluss::Series& got, const pluss::Series& want, float tol = 1.0e-5f) {
  const auto gv = to_cpu_f32_(got);
  const auto wv = to_cpu_f32_(want);
  REQUIRE(gv.size() == wv.size());
  for (std::size_t i = 0; i < gv.size(); ++i) {
    REQUIRE(std::fabs(gv[i] - wv[i]) <= tol);
  }
}

void require_equal_i64_(const pluss::Series& got, const pluss::Series& want) {
  const auto gv = to_cpu_i64_(got);
  const auto wv = to_cpu_i64_(want);
  REQUIRE(gv.size() == wv.size());
  for (std::size_t i = 0; i < gv.size(); ++i) {
    REQUIRE(gv[i] == wv[i]);
  }
}

} // namespace

TEST_CASE("CUDA stochastic baseline capability is runtime-dynamic", "[cuda][stochastic][runtime]") {
  const bool cuda_ready = pluss::cuda::is_ready();
  {
    const ScopedEnvVar baseline_off("PLUSS_CUDA_NATIVE_BASELINE", "0");
    const ScopedEnvVar stochastic_on("PLUSS_CUDA_NATIVE_STOCHASTIC", "1");
    REQUIRE(pluss::cuda::native_stochastic_enabled());
    REQUIRE_FALSE(pluss::cuda::supports_bernoulli_stochastic_baseline(pluss::ScalarType::Float32));
    REQUIRE_FALSE(pluss::cuda::supports_multinomial_stochastic_baseline(pluss::ScalarType::Float32));
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    const ScopedEnvVar stochastic_off("PLUSS_CUDA_NATIVE_STOCHASTIC", "0");
    REQUIRE_FALSE(pluss::cuda::native_stochastic_enabled());
    REQUIRE_FALSE(pluss::cuda::supports_normal_stochastic_baseline(pluss::ScalarType::Float32));
    REQUIRE_FALSE(pluss::cuda::supports_random_stochastic_baseline(pluss::ScalarType::Int64));
    REQUIRE_FALSE(pluss::cuda::supports_poisson_stochastic_baseline(pluss::ScalarType::Int64));
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    const ScopedEnvVar stochastic_on("PLUSS_CUDA_NATIVE_STOCHASTIC", "1");
    REQUIRE(pluss::cuda::native_stochastic_enabled());
    REQUIRE(pluss::cuda::supports_bernoulli_stochastic_baseline(pluss::ScalarType::Float32) == cuda_ready);
    REQUIRE(pluss::cuda::supports_normal_stochastic_baseline(pluss::ScalarType::Float16) == cuda_ready);
    REQUIRE(pluss::cuda::supports_log_normal_stochastic_baseline(pluss::ScalarType::BFloat16) == cuda_ready);
    REQUIRE(pluss::cuda::supports_uniform_stochastic_baseline(pluss::ScalarType::Float64) == cuda_ready);
    REQUIRE(pluss::cuda::supports_exponential_stochastic_baseline(pluss::ScalarType::Float32) == cuda_ready);
    REQUIRE(pluss::cuda::supports_geometric_stochastic_baseline(pluss::ScalarType::Int64) == cuda_ready);
    REQUIRE(pluss::cuda::supports_cauchy_stochastic_baseline(pluss::ScalarType::Float32) == cuda_ready);
    REQUIRE(pluss::cuda::supports_random_stochastic_baseline(pluss::ScalarType::Int64) == cuda_ready);
    REQUIRE(pluss::cuda::supports_multinomial_stochastic_baseline(pluss::ScalarType::Float32) == cuda_ready);
    REQUIRE(pluss::cuda::supports_poisson_stochastic_baseline(pluss::ScalarType::Int64) == cuda_ready);
    REQUIRE_FALSE(pluss::cuda::supports_random_stochastic_baseline(pluss::ScalarType::Complex64));
    REQUIRE_FALSE(pluss::cuda::supports_poisson_stochastic_baseline(pluss::ScalarType::Complex64));
  }
}

TEST_CASE("CUDA stochastic strict policy is runtime-dynamic", "[cuda][stochastic][policy][runtime]") {
  {
    const ScopedEnvVar strict_off("PLUSS_CUDA_NATIVE_STOCHASTIC_STRICT", "0");
    REQUIRE_FALSE(pluss::cuda::native_stochastic_strict());
  }
  {
    const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_STOCHASTIC_STRICT", "1");
    REQUIRE(pluss::cuda::native_stochastic_strict());
  }
}

TEST_CASE("CUDA bridge enables stochastic fallback when strict native is off", "[cuda][bridge][stochastic]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar stochastic_off("PLUSS_CUDA_NATIVE_STOCHASTIC", "0");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto probs_cpu =
      pluss::make_series(std::vector<float>{0.2f, 0.8f, 0.3f, 0.9f, 0.6f, 0.4f}, {2, 3});
  const auto probs_gpu = probs_cpu.to(pluss::pCUDA);

  require_equal_i64_(
      probs_gpu.bernoulli().to(pluss::ScalarType::Int64),
      probs_cpu.bernoulli().to(pluss::ScalarType::Int64));

  {
    auto cpu = probs_cpu.clone();
    auto gpu = probs_gpu.clone();
    cpu.uniform_(0.1, 0.9);
    gpu.uniform_(0.1, 0.9);
    require_close_f32_(gpu, cpu);
  }
  {
    auto cpu = probs_cpu.clone();
    auto gpu = probs_gpu.clone();
    cpu.normal_(0.5, 1.2);
    gpu.normal_(0.5, 1.2);
    require_close_f32_(gpu, cpu);
  }
  {
    auto cpu = probs_cpu.clone();
    auto gpu = probs_gpu.clone();
    cpu.log_normal_(0.0, 0.7);
    gpu.log_normal_(0.0, 0.7);
    require_close_f32_(gpu, cpu);
  }
  {
    auto cpu = probs_cpu.clone();
    auto gpu = probs_gpu.clone();
    cpu.exponential_(1.5);
    gpu.exponential_(1.5);
    require_close_f32_(gpu, cpu);
  }
  {
    auto cpu = probs_cpu.clone();
    auto gpu = probs_gpu.clone();
    cpu.geometric_(0.35);
    gpu.geometric_(0.35);
    require_close_f32_(gpu, cpu);
  }
  {
    auto cpu = probs_cpu.clone();
    auto gpu = probs_gpu.clone();
    cpu.cauchy_(0.0, 1.0);
    gpu.cauchy_(0.0, 1.0);
    require_close_f32_(gpu, cpu);
  }
  {
    auto cpu = pluss::zeros({8}, pluss::ScalarType::Int64);
    auto gpu = cpu.to(pluss::pCUDA);
    cpu.random_(2.0, 11.0);
    gpu.random_(2.0, 11.0);
    require_equal_i64_(gpu, cpu);
  }
  require_equal_i64_(
      probs_gpu.flatten(0, -1).multinomial(3),
      probs_cpu.flatten(0, -1).multinomial(3));
}

TEST_CASE("CUDA strict stochastic executes supported lanes", "[cuda][bridge][policy][stochastic]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar stochastic_on("PLUSS_CUDA_NATIVE_STOCHASTIC", "1");
  const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_STOCHASTIC_STRICT", "1");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto gpu = pluss::ones({2, 3}, pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCUDA));
  REQUIRE(pluss::cuda::supports_normal_stochastic(gpu));
  REQUIRE(pluss::cuda::supports_log_normal_stochastic(gpu));
  REQUIRE(pluss::cuda::supports_uniform_stochastic(gpu));
  REQUIRE(pluss::cuda::supports_multinomial_stochastic(gpu, 2, false));
  REQUIRE(pluss::cuda::supports_poisson_stochastic(pluss::ScalarType::Int64));

  auto y = gpu.clone();
  y.normal_(0.0, 1.0);
  REQUIRE(y.platform() == pluss::pCUDA);

  auto z = gpu.clone();
  z.log_normal_(0.0, 1.0);
  REQUIRE(z.platform() == pluss::pCUDA);

  const auto m = gpu.flatten(0, -1).multinomial(2);
  REQUIRE(m.platform() == pluss::pCUDA);

  const auto p = pluss::poisson(
      {2, 3},
      2.75,
      pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(pluss::pCUDA),
      /*seed=*/29);
  REQUIRE(p.platform() == pluss::pCUDA);
}

TEST_CASE("CUDA strict stochastic rejects unsupported policy lanes", "[cuda][bridge][policy][stochastic][reject]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar baseline_off("PLUSS_CUDA_NATIVE_BASELINE", "0");
  const ScopedEnvVar stochastic_on("PLUSS_CUDA_NATIVE_STOCHASTIC", "1");
  const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_STOCHASTIC_STRICT", "1");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  auto gpu = pluss::ones({2, 3}, pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCUDA));
  REQUIRE_THROWS(gpu.normal_(0.0, 1.0));
  REQUIRE_THROWS(gpu.random_(0.0, 1.0));
  REQUIRE_THROWS(gpu.multinomial(2));
}

TEST_CASE("CUDA bridge executes stochastic factory routes", "[cuda][bridge][stochastic]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar stochastic_off("PLUSS_CUDA_NATIVE_STOCHASTIC", "0");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const pluss::Config cfg_cuda = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCUDA);
  const pluss::Config cfg_cpu = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU);
  const std::array<double, 3> probs{0.1, 0.2, 0.7};

  require_close_f32_(
      pluss::normal(0.0, 1.0, {2, 3}, cfg_cuda, /*seed=*/17),
      pluss::normal(0.0, 1.0, {2, 3}, cfg_cpu, /*seed=*/17));
  require_close_f32_(
      pluss::bernoulli({2, 3}, 0.35, cfg_cuda, /*seed=*/23),
      pluss::bernoulli({2, 3}, 0.35, cfg_cpu, /*seed=*/23));
  require_equal_i64_(
      pluss::randperm(10, pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(pluss::pCUDA), /*seed=*/5),
      pluss::randperm(10, pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(pluss::pCPU), /*seed=*/5));
  require_equal_i64_(
      pluss::poisson(
          {2, 3},
          4.5,
          pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(pluss::pCUDA),
          /*seed=*/19),
      pluss::poisson(
          {2, 3},
          4.5,
          pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(pluss::pCPU),
          /*seed=*/19));
  require_equal_i64_(
      pluss::multinomial(
          std::span<const double>(probs.data(), probs.size()),
          /*num_samples=*/3,
          /*replacement=*/false,
          pluss::Config{}.with_platform(pluss::pCUDA),
          /*seed=*/11),
      pluss::multinomial(
          std::span<const double>(probs.data(), probs.size()),
          /*num_samples=*/3,
          /*replacement=*/false,
          pluss::Config{}.with_platform(pluss::pCPU),
          /*seed=*/11));
}

TEST_CASE("CUDA strict stochastic rejects unsupported factory routes", "[cuda][bridge][policy][stochastic][reject]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar baseline_off("PLUSS_CUDA_NATIVE_BASELINE", "0");
  const ScopedEnvVar stochastic_on("PLUSS_CUDA_NATIVE_STOCHASTIC", "1");
  const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_STOCHASTIC_STRICT", "1");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const pluss::Config cfg_cuda = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCUDA);
  REQUIRE_THROWS(pluss::normal(0.0, 1.0, {2, 3}, cfg_cuda, /*seed=*/13));
  REQUIRE_THROWS(pluss::bernoulli({2, 3}, 0.5, cfg_cuda, /*seed=*/13));
  REQUIRE_THROWS(pluss::poisson({2, 3}, 4.5, cfg_cuda, /*seed=*/13));
}

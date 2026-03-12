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

TEST_CASE("CUDA scatter/gather baseline capability is runtime-dynamic", "[cuda][scatter][runtime]") {
  const bool cuda_ready = pluss::cuda::is_ready();
  {
    const ScopedEnvVar baseline_off("PLUSS_CUDA_NATIVE_BASELINE", "0");
    const ScopedEnvVar sg_on("PLUSS_CUDA_NATIVE_SCATTER_GATHER", "1");
    REQUIRE(pluss::cuda::native_scatter_gather_enabled());
    REQUIRE_FALSE(
        pluss::cuda::supports_scatter_baseline(
            pluss::ScalarType::Float32,
            pluss::ScalarType::Int64,
            pluss::ScalarType::Float32));
    REQUIRE_FALSE(pluss::cuda::supports_gather_baseline(pluss::ScalarType::Float32, pluss::ScalarType::Int64));
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    const ScopedEnvVar sg_off("PLUSS_CUDA_NATIVE_SCATTER_GATHER", "0");
    REQUIRE_FALSE(pluss::cuda::native_scatter_gather_enabled());
    REQUIRE_FALSE(
        pluss::cuda::supports_scatter_baseline(
            pluss::ScalarType::Float32,
            pluss::ScalarType::Int64,
            pluss::ScalarType::Float32));
    REQUIRE_FALSE(pluss::cuda::supports_gather_baseline(pluss::ScalarType::Float32, pluss::ScalarType::Int64));
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    const ScopedEnvVar sg_on("PLUSS_CUDA_NATIVE_SCATTER_GATHER", "1");
    REQUIRE(pluss::cuda::native_scatter_gather_enabled());
    REQUIRE(
        pluss::cuda::supports_scatter_baseline(
            pluss::ScalarType::Float32,
            pluss::ScalarType::Int64,
            pluss::ScalarType::Float32) == cuda_ready);
    REQUIRE(
        pluss::cuda::supports_scatter_baseline(
            pluss::ScalarType::Float64,
            pluss::ScalarType::Int64,
            pluss::ScalarType::Float64) == cuda_ready);
    REQUIRE_FALSE(
        pluss::cuda::supports_scatter_baseline(
            pluss::ScalarType::Float32,
            pluss::ScalarType::Int32,
            pluss::ScalarType::Float32));
    REQUIRE(
        pluss::cuda::supports_scatter_reduce_baseline(
            pluss::ScalarType::Float32,
            pluss::ScalarType::Int64,
            pluss::ScalarType::Float32,
            "sum") == cuda_ready);
    REQUIRE_FALSE(
        pluss::cuda::supports_scatter_reduce_baseline(
            pluss::ScalarType::Float32,
            pluss::ScalarType::Int64,
            pluss::ScalarType::Float32,
            "prod"));
    REQUIRE(pluss::cuda::supports_gather_baseline(pluss::ScalarType::BFloat16, pluss::ScalarType::Int64) == cuda_ready);
    REQUIRE(pluss::cuda::supports_gather_baseline(pluss::ScalarType::Int64, pluss::ScalarType::Int64) == cuda_ready);
  }
}

TEST_CASE("CUDA scatter/gather strict policy is runtime-dynamic", "[cuda][scatter][policy][runtime]") {
  {
    const ScopedEnvVar strict_off("PLUSS_CUDA_NATIVE_SCATTER_GATHER_STRICT", "0");
    REQUIRE_FALSE(pluss::cuda::native_scatter_gather_strict());
  }
  {
    const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_SCATTER_GATHER_STRICT", "1");
    REQUIRE(pluss::cuda::native_scatter_gather_strict());
  }
}

TEST_CASE("CUDA scatter/gather route contract rejects non-CUDA tensors", "[cuda][scatter][policy]") {
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar sg_on("PLUSS_CUDA_NATIVE_SCATTER_GATHER", "1");
  const auto self_cpu = pluss::Series::zeros({2, 3}, pluss::ScalarType::Float32);
  const auto idx_cpu = pluss::make_series(std::vector<std::int64_t>{0, 1, 2, 2, 1, 0}, {2, 3});
  const auto src_cpu = pluss::Series::ones({2, 3}, pluss::ScalarType::Float32);
  REQUIRE_FALSE(pluss::cuda::supports_scatter_native(self_cpu, 1, idx_cpu, src_cpu));
  REQUIRE_FALSE(pluss::cuda::supports_scatter_add_native(self_cpu, 1, idx_cpu, src_cpu));
  REQUIRE_FALSE(pluss::cuda::supports_scatter_reduce_native(self_cpu, 1, idx_cpu, src_cpu, "sum"));
  REQUIRE_FALSE(pluss::cuda::supports_gather_native(self_cpu, 1, idx_cpu));
}

TEST_CASE("CUDA bridge enables scatter/gather fallback when strict native is off", "[cuda][bridge][scatter]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar sg_off("PLUSS_CUDA_NATIVE_SCATTER_GATHER", "0");
  const bool cuda_ready = pluss::cuda::is_ready();
  if (!cuda_ready) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto self_cpu = pluss::Series::zeros({2, 3}, pluss::ScalarType::Float32);
  const auto input_cpu = pluss::make_series(std::vector<float>{1.f, 2.f, 3.f, 4.f, 5.f, 6.f}, {2, 3});
  const auto idx_cpu = pluss::make_series(std::vector<std::int64_t>{0, 1, 2, 2, 1, 0}, {2, 3});
  const auto src_cpu = pluss::make_series(std::vector<float>{9.f, 8.f, 7.f, 6.f, 5.f, 4.f}, {2, 3});

  const auto self_cuda = self_cpu.to(pluss::pCUDA);
  const auto input_cuda = input_cpu.to(pluss::pCUDA);
  const auto idx_cuda = idx_cpu.to(pluss::pCUDA);
  const auto src_cuda = src_cpu.to(pluss::pCUDA);

  const auto scatter_cpu = self_cpu.scatter(1, idx_cpu, src_cpu);
  const auto scatter_cuda = self_cuda.scatter(1, idx_cuda, src_cuda);
  REQUIRE(scatter_cuda.platform() == pluss::pCUDA);
  require_close_(scatter_cuda.to(pluss::pCPU).to_vec<float>(), scatter_cpu.to_vec<float>());

  const auto scatter_add_cpu = self_cpu.scatter_add(1, idx_cpu, src_cpu);
  const auto scatter_add_cuda = self_cuda.scatter_add(1, idx_cuda, src_cuda);
  REQUIRE(scatter_add_cuda.platform() == pluss::pCUDA);
  require_close_(scatter_add_cuda.to(pluss::pCPU).to_vec<float>(), scatter_add_cpu.to_vec<float>());

  const auto gather_cpu = input_cpu.gather(1, idx_cpu);
  const auto gather_cuda = input_cuda.gather(1, idx_cuda);
  REQUIRE(gather_cuda.platform() == pluss::pCUDA);
  require_close_(gather_cuda.to(pluss::pCPU).to_vec<float>(), gather_cpu.to_vec<float>());
}

TEST_CASE("CUDA strict native scatter/gather executes supported native route", "[cuda][bridge][policy][scatter]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar sg_on("PLUSS_CUDA_NATIVE_SCATTER_GATHER", "1");
  const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_SCATTER_GATHER_STRICT", "1");
  const bool cuda_ready = pluss::cuda::is_ready();
  if (!cuda_ready) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto self_cpu = pluss::Series::zeros({2, 3}, pluss::ScalarType::Float32);
  const auto idx_cpu = pluss::make_series(std::vector<std::int64_t>{0, 1, 2, 2, 1, 0}, {2, 3});
  const auto src_cpu = pluss::Series::ones({2, 3}, pluss::ScalarType::Float32);
  const auto self_cuda = self_cpu.to(pluss::pCUDA);
  const auto idx_cuda = idx_cpu.to(pluss::pCUDA);
  const auto src_cuda = src_cpu.to(pluss::pCUDA);
  const auto got = self_cuda.scatter(1, idx_cuda, src_cuda);
  REQUIRE(got.platform() == pluss::pCUDA);
  require_close_(got.to(pluss::pCPU).to_vec<float>(), self_cpu.scatter(1, idx_cpu, src_cpu).to_vec<float>());
}

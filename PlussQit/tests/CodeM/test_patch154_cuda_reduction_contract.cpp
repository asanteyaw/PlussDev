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

void require_equal_i64_(const std::vector<std::int64_t>& got, const std::vector<std::int64_t>& want) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(got[i] == want[i]);
  }
}

void require_equal_u8_(const std::vector<std::uint8_t>& got, const std::vector<std::uint8_t>& want) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(got[i] == want[i]);
  }
}

} // namespace

TEST_CASE("CUDA reduction baseline capability is runtime-dynamic", "[cuda][reduction][runtime]") {
  const bool cuda_ready = pluss::cuda::is_ready();
  {
    const ScopedEnvVar baseline_off("PLUSS_CUDA_NATIVE_BASELINE", "0");
    const ScopedEnvVar reduction_on("PLUSS_CUDA_NATIVE_REDUCTION", "1");
    REQUIRE(pluss::cuda::native_reduction_enabled());
    REQUIRE_FALSE(pluss::cuda::supports_sum_reduction_baseline(pluss::ScalarType::Float32));
    REQUIRE_FALSE(pluss::cuda::supports_prod_reduction_baseline(pluss::ScalarType::Float32));
    REQUIRE_FALSE(pluss::cuda::supports_mean_reduction_baseline(pluss::ScalarType::Float32));
    REQUIRE_FALSE(pluss::cuda::supports_ordered_extrema_reduction_baseline(pluss::ScalarType::Float32));
    REQUIRE_FALSE(pluss::cuda::supports_logical_reduction_baseline(pluss::ScalarType::Bool));
    REQUIRE_FALSE(pluss::cuda::supports_arg_extrema_reduction_baseline(pluss::ScalarType::Float32));
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    const ScopedEnvVar reduction_off("PLUSS_CUDA_NATIVE_REDUCTION", "0");
    REQUIRE_FALSE(pluss::cuda::native_reduction_enabled());
    REQUIRE_FALSE(pluss::cuda::supports_sum_reduction_baseline(pluss::ScalarType::Float32));
    REQUIRE_FALSE(pluss::cuda::supports_prod_reduction_baseline(pluss::ScalarType::Float32));
    REQUIRE_FALSE(pluss::cuda::supports_mean_reduction_baseline(pluss::ScalarType::Float32));
    REQUIRE_FALSE(pluss::cuda::supports_ordered_extrema_reduction_baseline(pluss::ScalarType::Float32));
    REQUIRE_FALSE(pluss::cuda::supports_logical_reduction_baseline(pluss::ScalarType::Bool));
    REQUIRE_FALSE(pluss::cuda::supports_arg_extrema_reduction_baseline(pluss::ScalarType::Float32));
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    const ScopedEnvVar reduction_on("PLUSS_CUDA_NATIVE_REDUCTION", "1");
    REQUIRE(pluss::cuda::native_reduction_enabled());
    REQUIRE(pluss::cuda::supports_sum_reduction_baseline(pluss::ScalarType::Float32) == cuda_ready);
    REQUIRE(pluss::cuda::supports_prod_reduction_baseline(pluss::ScalarType::Float16) == cuda_ready);
    REQUIRE(pluss::cuda::supports_mean_reduction_baseline(pluss::ScalarType::Bool) == cuda_ready);
    REQUIRE(pluss::cuda::supports_ordered_extrema_reduction_baseline(pluss::ScalarType::Int64) == cuda_ready);
    REQUIRE(pluss::cuda::supports_logical_reduction_baseline(pluss::ScalarType::UInt16) == cuda_ready);
    REQUIRE(pluss::cuda::supports_arg_extrema_reduction_baseline(pluss::ScalarType::Float64) == cuda_ready);
    REQUIRE_FALSE(pluss::cuda::supports_ordered_extrema_reduction_baseline(pluss::ScalarType::Complex64));
    REQUIRE_FALSE(pluss::cuda::supports_arg_extrema_reduction_baseline(pluss::ScalarType::Int32));
  }
}

TEST_CASE("CUDA reduction strict policy is runtime-dynamic", "[cuda][reduction][policy][runtime]") {
  {
    const ScopedEnvVar strict_off("PLUSS_CUDA_NATIVE_REDUCTION_STRICT", "0");
    REQUIRE_FALSE(pluss::cuda::native_reduction_strict());
  }
  {
    const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_REDUCTION_STRICT", "1");
    REQUIRE(pluss::cuda::native_reduction_strict());
  }
}

TEST_CASE("CUDA reduction route contract rejects non-CUDA tensors", "[cuda][reduction][policy]") {
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar reduction_on("PLUSS_CUDA_NATIVE_REDUCTION", "1");

  const auto x_cpu =
      pluss::make_series(std::vector<float>{1.f, -2.f, 3.f, 4.f, 0.f, -1.f}, {2, 3});
  REQUIRE(x_cpu.platform() == pluss::pCPU);
  REQUIRE_FALSE(pluss::cuda::supports_sum_reduction(x_cpu));
  REQUIRE_FALSE(pluss::cuda::supports_sum_reduction(x_cpu, /*dim=*/1));
  REQUIRE_FALSE(pluss::cuda::supports_prod_reduction(x_cpu, /*dim=*/1));
  REQUIRE_FALSE(pluss::cuda::supports_mean_reduction(x_cpu, /*dim=*/1));
  REQUIRE_FALSE(pluss::cuda::supports_ordered_extrema_reduction(x_cpu, /*dim=*/1));
  REQUIRE_FALSE(pluss::cuda::supports_logical_reduction(x_cpu, /*dim=*/1));
  REQUIRE_FALSE(pluss::cuda::supports_arg_extrema_reduction(x_cpu, /*dim=*/1));
}

TEST_CASE("CUDA bridge enables reduction fallback when strict native is off", "[cuda][bridge][reduction]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar reduction_off("PLUSS_CUDA_NATIVE_REDUCTION", "0");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto x_cpu =
      pluss::make_series(std::vector<float>{1.f, -2.f, 3.f, 4.f, 0.f, -1.f}, {2, 3});
  const auto x_cuda = x_cpu.to(pluss::pCUDA);

  require_close_(x_cuda.sum(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.sum(/*dim=*/1, /*keepdim=*/false).to_vec<float>());
  require_close_(x_cuda.prod(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.prod(/*dim=*/1, /*keepdim=*/false).to_vec<float>());
  require_close_(x_cuda.mean(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.mean(/*dim=*/1, /*keepdim=*/false).to_vec<float>());
  require_close_(x_cuda.amax(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.amax(/*dim=*/1, /*keepdim=*/false).to_vec<float>());
  require_close_(x_cuda.amin(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.amin(/*dim=*/1, /*keepdim=*/false).to_vec<float>());

  const auto mask_cpu = x_cpu.gt(0.0);
  const auto mask_cuda = mask_cpu.to(pluss::pCUDA);
  require_equal_u8_(mask_cuda.all(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<std::uint8_t>(),
                    mask_cpu.all(/*dim=*/1, /*keepdim=*/false).to_vec<std::uint8_t>());
  require_equal_u8_(mask_cuda.any(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<std::uint8_t>(),
                    mask_cpu.any(/*dim=*/1, /*keepdim=*/false).to_vec<std::uint8_t>());
  require_equal_i64_(x_cuda.argmax(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<std::int64_t>(),
                     x_cpu.argmax(/*dim=*/1, /*keepdim=*/false).to_vec<std::int64_t>());
  require_equal_i64_(x_cuda.argmin(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<std::int64_t>(),
                     x_cpu.argmin(/*dim=*/1, /*keepdim=*/false).to_vec<std::int64_t>());
}

TEST_CASE("CUDA strict native reduction executes supported native route", "[cuda][bridge][policy][reduction]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar reduction_on("PLUSS_CUDA_NATIVE_REDUCTION", "1");
  const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_REDUCTION_STRICT", "1");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto x_cpu =
      pluss::make_series(std::vector<float>{2.f, -1.f, 5.f, 3.f, 7.f, 0.f}, {2, 3});
  const auto x_cuda = x_cpu.to(pluss::pCUDA);
  require_close_(x_cuda.sum(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.sum(/*dim=*/1, /*keepdim=*/false).to_vec<float>());
  require_close_(x_cuda.mean(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.mean(/*dim=*/1, /*keepdim=*/false).to_vec<float>());
  require_close_(x_cuda.amax(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.amax(/*dim=*/1, /*keepdim=*/false).to_vec<float>());
  require_equal_i64_(x_cuda.argmax(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<std::int64_t>(),
                     x_cpu.argmax(/*dim=*/1, /*keepdim=*/false).to_vec<std::int64_t>());
}

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

void require_equal_i64_(const std::vector<std::int64_t>& got, const std::vector<std::int64_t>& want) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(got[i] == want[i]);
  }
}

} // namespace

TEST_CASE("CUDA counting baseline capability is runtime-dynamic", "[cuda][counting][runtime]") {
  const bool cuda_ready = pluss::cuda::is_ready();
  {
    const ScopedEnvVar baseline_off("PLUSS_CUDA_NATIVE_BASELINE", "0");
    const ScopedEnvVar counting_on("PLUSS_CUDA_NATIVE_COUNTING", "1");
    REQUIRE(pluss::cuda::native_counting_enabled());
    REQUIRE_FALSE(pluss::cuda::supports_unique_counting_baseline(pluss::ScalarType::Float32, true));
    REQUIRE_FALSE(pluss::cuda::supports_bincount_counting_baseline(pluss::ScalarType::Int64));
    REQUIRE_FALSE(pluss::cuda::supports_histc_counting_baseline(pluss::ScalarType::Float32));
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    const ScopedEnvVar counting_off("PLUSS_CUDA_NATIVE_COUNTING", "0");
    REQUIRE_FALSE(pluss::cuda::native_counting_enabled());
    REQUIRE_FALSE(pluss::cuda::supports_unique_counting_baseline(pluss::ScalarType::Float32, true));
    REQUIRE_FALSE(pluss::cuda::supports_bincount_counting_baseline(pluss::ScalarType::Int64));
    REQUIRE_FALSE(pluss::cuda::supports_histc_counting_baseline(pluss::ScalarType::Float32));
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    const ScopedEnvVar counting_on("PLUSS_CUDA_NATIVE_COUNTING", "1");
    REQUIRE(pluss::cuda::native_counting_enabled());
    REQUIRE(pluss::cuda::supports_unique_counting_baseline(pluss::ScalarType::Float32, true) == cuda_ready);
    REQUIRE(pluss::cuda::supports_unique_counting_baseline(pluss::ScalarType::Int64, false) == cuda_ready);
    REQUIRE(pluss::cuda::supports_bincount_counting_baseline(pluss::ScalarType::Int64) == cuda_ready);
    REQUIRE(pluss::cuda::supports_histc_counting_baseline(pluss::ScalarType::Float16) == cuda_ready);
    REQUIRE_FALSE(pluss::cuda::supports_unique_counting_baseline(pluss::ScalarType::Float16, true));
    REQUIRE_FALSE(pluss::cuda::supports_bincount_counting_baseline(pluss::ScalarType::UInt8));
    REQUIRE_FALSE(pluss::cuda::supports_histc_counting_baseline(pluss::ScalarType::Complex64));
  }
}

TEST_CASE("CUDA counting strict policy is runtime-dynamic", "[cuda][counting][policy][runtime]") {
  {
    const ScopedEnvVar strict_off("PLUSS_CUDA_NATIVE_COUNTING_STRICT", "0");
    REQUIRE_FALSE(pluss::cuda::native_counting_strict());
  }
  {
    const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_COUNTING_STRICT", "1");
    REQUIRE(pluss::cuda::native_counting_strict());
  }
}

TEST_CASE("CUDA counting route contract rejects non-CUDA tensors", "[cuda][counting][policy]") {
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar counting_on("PLUSS_CUDA_NATIVE_COUNTING", "1");

  const auto int_cpu = pluss::make_series(std::vector<std::int64_t>{3, 1, 3, 2, 2, 5}, {6});
  const auto float_cpu = pluss::make_series(std::vector<float>{0.1f, 1.4f, 2.0f}, {3});

  REQUIRE(int_cpu.platform() == pluss::pCPU);
  REQUIRE(float_cpu.platform() == pluss::pCPU);
  REQUIRE_FALSE(pluss::cuda::supports_unique_counting(int_cpu, true));
  REQUIRE_FALSE(pluss::cuda::supports_bincount_counting(int_cpu, 0));
  REQUIRE_FALSE(pluss::cuda::supports_histc_counting(float_cpu, 4));
}

TEST_CASE("CUDA bridge enables counting fallback when strict native is off", "[cuda][bridge][counting]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar counting_off("PLUSS_CUDA_NATIVE_COUNTING", "0");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto int_cpu = pluss::make_series(std::vector<std::int64_t>{3, 1, 3, 2, 2, 5}, {6});
  const auto float_cpu = pluss::make_series(std::vector<float>{0.1f, 0.9f, 1.2f, 2.8f, 3.0f, 4.0f}, {6});
  const auto int_cuda = int_cpu.to(pluss::pCUDA);
  const auto float_cuda = float_cpu.to(pluss::pCUDA);

  require_equal_i64_(
      int_cuda.unique(true).to(pluss::pCPU).to_vec<std::int64_t>(),
      int_cpu.unique(true).to_vec<std::int64_t>());
  require_equal_i64_(
      int_cuda.bincount(/*minlength=*/7).to(pluss::pCPU).to_vec<std::int64_t>(),
      int_cpu.bincount(/*minlength=*/7).to_vec<std::int64_t>());
  require_equal_i64_(
      float_cuda.histc(/*bins=*/4, /*min=*/0.0, /*max=*/4.0).to(pluss::pCPU).to_vec<std::int64_t>(),
      float_cpu.histc(/*bins=*/4, /*min=*/0.0, /*max=*/4.0).to_vec<std::int64_t>());
}

TEST_CASE("CUDA strict native counting executes supported route and rejects unsupported lanes", "[cuda][bridge][policy][counting]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar counting_on("PLUSS_CUDA_NATIVE_COUNTING", "1");
  const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_COUNTING_STRICT", "1");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto int_cpu = pluss::make_series(std::vector<std::int64_t>{4, 4, 1, 2, 3, 3, 0}, {7});
  const auto float_cpu = pluss::make_series(std::vector<float>{0.2f, 0.7f, 1.1f, 2.2f, 3.8f}, {5});
  const auto int_cuda = int_cpu.to(pluss::pCUDA);
  const auto float_cuda = float_cpu.to(pluss::pCUDA);

  REQUIRE(pluss::cuda::supports_unique_counting(int_cuda, true));
  REQUIRE(pluss::cuda::supports_bincount_counting(int_cuda, 0));
  REQUIRE(pluss::cuda::supports_histc_counting(float_cuda, 4));

  require_equal_i64_(
      int_cuda.unique(true).to(pluss::pCPU).to_vec<std::int64_t>(),
      int_cpu.unique(true).to_vec<std::int64_t>());
  require_equal_i64_(
      int_cuda.bincount(/*minlength=*/6).to(pluss::pCPU).to_vec<std::int64_t>(),
      int_cpu.bincount(/*minlength=*/6).to_vec<std::int64_t>());
  require_equal_i64_(
      float_cuda.histc(/*bins=*/4, /*min=*/0.0, /*max=*/4.0).to(pluss::pCPU).to_vec<std::int64_t>(),
      float_cpu.histc(/*bins=*/4, /*min=*/0.0, /*max=*/4.0).to_vec<std::int64_t>());

  const auto half_cuda = float_cpu.to(pluss::ScalarType::Float16).to(pluss::pCUDA);
  REQUIRE_FALSE(pluss::cuda::supports_unique_counting(half_cuda, true));
  REQUIRE_THROWS(half_cuda.unique(true));
}

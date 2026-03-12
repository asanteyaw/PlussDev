#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

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

} // namespace

TEST_CASE("CUDA indexing baseline capability is runtime-dynamic", "[cuda][indexing][runtime]") {
  const bool cuda_ready = pluss::cuda::is_ready();
  {
    const ScopedEnvVar baseline_off("PLUSS_CUDA_NATIVE_BASELINE", "0");
    const ScopedEnvVar indexing_on("PLUSS_CUDA_NATIVE_INDEXING", "1");
    REQUIRE(pluss::cuda::native_indexing_enabled());
    REQUIRE_FALSE(pluss::cuda::supports_take_put_indexing_baseline(pluss::ScalarType::Float32, pluss::ScalarType::Int64));
    REQUIRE_FALSE(pluss::cuda::supports_nonzero_indexing_baseline(pluss::ScalarType::Float32));
    REQUIRE_FALSE(pluss::cuda::supports_index_indexing_baseline(pluss::ScalarType::Float32, pluss::ScalarType::Int64));
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    const ScopedEnvVar indexing_off("PLUSS_CUDA_NATIVE_INDEXING", "0");
    REQUIRE_FALSE(pluss::cuda::native_indexing_enabled());
    REQUIRE_FALSE(pluss::cuda::supports_take_put_indexing_baseline(pluss::ScalarType::Float32, pluss::ScalarType::Int64));
    REQUIRE_FALSE(pluss::cuda::supports_nonzero_indexing_baseline(pluss::ScalarType::Float32));
    REQUIRE_FALSE(pluss::cuda::supports_index_indexing_baseline(pluss::ScalarType::Float32, pluss::ScalarType::Int64));
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    const ScopedEnvVar indexing_on("PLUSS_CUDA_NATIVE_INDEXING", "1");
    REQUIRE(pluss::cuda::native_indexing_enabled());
    REQUIRE(
        pluss::cuda::supports_take_put_indexing_baseline(pluss::ScalarType::Float32, pluss::ScalarType::Int64) ==
        cuda_ready);
    REQUIRE_FALSE(pluss::cuda::supports_take_put_indexing_baseline(pluss::ScalarType::Float32, pluss::ScalarType::Int32));
    REQUIRE(pluss::cuda::supports_nonzero_indexing_baseline(pluss::ScalarType::Float16) == cuda_ready);
    REQUIRE_FALSE(pluss::cuda::supports_nonzero_indexing_baseline(pluss::ScalarType::Complex64));
    REQUIRE(
        pluss::cuda::supports_index_indexing_baseline(pluss::ScalarType::Int64, pluss::ScalarType::Int64) ==
        cuda_ready);
  }
}

TEST_CASE("CUDA indexing strict policy is runtime-dynamic", "[cuda][indexing][policy][runtime]") {
  {
    const ScopedEnvVar strict_off("PLUSS_CUDA_NATIVE_INDEXING_STRICT", "0");
    REQUIRE_FALSE(pluss::cuda::native_indexing_strict());
  }
  {
    const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_INDEXING_STRICT", "1");
    REQUIRE(pluss::cuda::native_indexing_strict());
  }
}

TEST_CASE("CUDA indexing route contract rejects non-CUDA tensors", "[cuda][indexing][policy]") {
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar indexing_on("PLUSS_CUDA_NATIVE_INDEXING", "1");
  const auto x_cpu = pluss::Series::ones({5}, pluss::ScalarType::Float32);
  const auto idx_cpu = pluss::make_series(std::vector<std::int64_t>{0, 2, 4}, {3});
  const auto src_cpu = pluss::make_series(std::vector<float>{9.f, 8.f, 7.f}, {3});
  REQUIRE(x_cpu.platform() == pluss::pCPU);
  REQUIRE(idx_cpu.platform() == pluss::pCPU);
  REQUIRE(src_cpu.platform() == pluss::pCPU);
  REQUIRE_FALSE(pluss::cuda::supports_take_indexing(x_cpu, idx_cpu));
  REQUIRE_FALSE(pluss::cuda::supports_put_indexing(x_cpu, idx_cpu, src_cpu, /*accumulate=*/false));
  REQUIRE_FALSE(pluss::cuda::supports_nonzero_indexing(x_cpu));
  REQUIRE_FALSE(pluss::cuda::supports_index_indexing(x_cpu, std::vector<pluss::Series>{idx_cpu}));
}

#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cstdlib>
#include <optional>
#include <string>

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

TEST_CASE("CUDA selection baseline capability is runtime-dynamic", "[cuda][selection][runtime]") {
  const bool cuda_ready = pluss::cuda::is_ready();
  {
    const ScopedEnvVar baseline_off("PLUSS_CUDA_NATIVE_BASELINE", "0");
    const ScopedEnvVar selection_on("PLUSS_CUDA_NATIVE_SELECTION", "1");
    REQUIRE(pluss::cuda::native_selection_enabled());
    REQUIRE_FALSE(pluss::cuda::supports_sort_selection_baseline(pluss::ScalarType::Float32, /*stable=*/false));
    REQUIRE_FALSE(pluss::cuda::supports_topk_selection_baseline(pluss::ScalarType::Float32, /*sorted=*/true));
    REQUIRE_FALSE(pluss::cuda::supports_kthvalue_selection_baseline(pluss::ScalarType::Float32));
    REQUIRE_FALSE(pluss::cuda::supports_argsort_selection_baseline(pluss::ScalarType::Float32, /*stable=*/false));
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    const ScopedEnvVar selection_off("PLUSS_CUDA_NATIVE_SELECTION", "0");
    REQUIRE_FALSE(pluss::cuda::native_selection_enabled());
    REQUIRE_FALSE(pluss::cuda::supports_sort_selection_baseline(pluss::ScalarType::Float32, /*stable=*/false));
    REQUIRE_FALSE(pluss::cuda::supports_topk_selection_baseline(pluss::ScalarType::Float32, /*sorted=*/true));
    REQUIRE_FALSE(pluss::cuda::supports_kthvalue_selection_baseline(pluss::ScalarType::Float32));
    REQUIRE_FALSE(pluss::cuda::supports_argsort_selection_baseline(pluss::ScalarType::Float32, /*stable=*/false));
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    const ScopedEnvVar selection_on("PLUSS_CUDA_NATIVE_SELECTION", "1");
    REQUIRE(pluss::cuda::native_selection_enabled());
    REQUIRE(pluss::cuda::supports_sort_selection_baseline(pluss::ScalarType::Float32, /*stable=*/false) == cuda_ready);
    REQUIRE_FALSE(pluss::cuda::supports_sort_selection_baseline(pluss::ScalarType::Float32, /*stable=*/true));
    REQUIRE(pluss::cuda::supports_topk_selection_baseline(pluss::ScalarType::Float16, /*sorted=*/true) == cuda_ready);
    REQUIRE_FALSE(pluss::cuda::supports_topk_selection_baseline(pluss::ScalarType::Float16, /*sorted=*/false));
    REQUIRE(pluss::cuda::supports_kthvalue_selection_baseline(pluss::ScalarType::BFloat16) == cuda_ready);
    REQUIRE_FALSE(pluss::cuda::supports_kthvalue_selection_baseline(pluss::ScalarType::Int64));
    REQUIRE(pluss::cuda::supports_argsort_selection_baseline(pluss::ScalarType::Float32, /*stable=*/false) == cuda_ready);
    REQUIRE_FALSE(pluss::cuda::supports_argsort_selection_baseline(pluss::ScalarType::Float32, /*stable=*/true));
  }
}

TEST_CASE("CUDA selection strict policy is runtime-dynamic", "[cuda][selection][policy][runtime]") {
  {
    const ScopedEnvVar strict_off("PLUSS_CUDA_NATIVE_SELECTION_STRICT", "0");
    REQUIRE_FALSE(pluss::cuda::native_selection_strict());
  }
  {
    const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_SELECTION_STRICT", "1");
    REQUIRE(pluss::cuda::native_selection_strict());
  }
}

TEST_CASE("CUDA selection route contract rejects non-CUDA tensors", "[cuda][selection][policy]") {
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar selection_on("PLUSS_CUDA_NATIVE_SELECTION", "1");
  const auto x_cpu = pluss::Series::ones({4, 6}, pluss::ScalarType::Float32);
  REQUIRE(x_cpu.platform() == pluss::pCPU);
  REQUIRE_FALSE(pluss::cuda::supports_sort_selection(x_cpu, /*dim=*/-1, /*stable=*/false));
  REQUIRE_FALSE(pluss::cuda::supports_topk_selection(x_cpu, /*k=*/3, /*dim=*/-1, /*sorted=*/true));
  REQUIRE_FALSE(pluss::cuda::supports_kthvalue_selection(x_cpu, /*k=*/2, /*dim=*/-1));
  REQUIRE_FALSE(pluss::cuda::supports_argsort_selection(x_cpu, /*dim=*/-1, /*stable=*/false));
}

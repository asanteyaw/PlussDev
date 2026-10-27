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

std::vector<float> to_cpu_f32_(const pluss::Series& x) {
  return x.to(pluss::pCPU).to(pluss::ScalarType::Float32).contiguous().host<float>();
}

std::vector<std::int64_t> to_cpu_i64_(const pluss::Series& x) {
  return x.to(pluss::pCPU).to(pluss::ScalarType::Int64).contiguous().host<std::int64_t>();
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

TEST_CASE("CUDA selection bridge executes selection routes", "[cuda][bridge][selection][route]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar selection_off("PLUSS_CUDA_NATIVE_SELECTION", "0");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto cpu = pluss::make_series(std::vector<float>{3.0f, 1.0f, 2.0f, 9.0f, 7.0f, 8.0f}, {2, 3});
  const auto gpu = cpu.to(pluss::pCUDA);

  require_close_f32_(
      gpu.sort(/*dim=*/-1, /*descending=*/false, /*stable=*/false),
      cpu.sort(/*dim=*/-1, /*descending=*/false, /*stable=*/false));
  require_close_f32_(
      gpu.topk(/*k=*/2, /*dim=*/-1, /*largest=*/true, /*sorted=*/true),
      cpu.topk(/*k=*/2, /*dim=*/-1, /*largest=*/true, /*sorted=*/true));
  require_close_f32_(
      gpu.kthvalue(/*k=*/2, /*dim=*/-1, /*keepdim=*/false),
      cpu.kthvalue(/*k=*/2, /*dim=*/-1, /*keepdim=*/false));
  require_equal_i64_(
      gpu.argsort(/*dim=*/-1, /*descending=*/false, /*stable=*/false),
      cpu.argsort(/*dim=*/-1, /*descending=*/false, /*stable=*/false));
}

TEST_CASE("CUDA strict selection executes supported lanes", "[cuda][bridge][policy][selection][route]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar selection_on("PLUSS_CUDA_NATIVE_SELECTION", "1");
  const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_SELECTION_STRICT", "1");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto cpu = pluss::make_series(std::vector<float>{5.0f, 1.0f, 3.0f, 4.0f, 9.0f, 8.0f, 2.0f, 6.0f}, {2, 4})
                       .to(pluss::ScalarType::Float16);
  const auto gpu = cpu.to(pluss::pCUDA);

  REQUIRE(pluss::cuda::supports_sort_selection(gpu, /*dim=*/-1, /*stable=*/false));
  REQUIRE(pluss::cuda::supports_topk_selection(gpu, /*k=*/3, /*dim=*/-1, /*sorted=*/true));
  REQUIRE(pluss::cuda::supports_kthvalue_selection(gpu, /*k=*/2, /*dim=*/-1));
  REQUIRE(pluss::cuda::supports_argsort_selection(gpu, /*dim=*/-1, /*stable=*/false));

  require_close_f32_(
      gpu.sort(/*dim=*/-1, /*descending=*/false, /*stable=*/false),
      cpu.sort(/*dim=*/-1, /*descending=*/false, /*stable=*/false));
  require_close_f32_(
      gpu.topk(/*k=*/3, /*dim=*/-1, /*largest=*/false, /*sorted=*/true),
      cpu.topk(/*k=*/3, /*dim=*/-1, /*largest=*/false, /*sorted=*/true));
  require_close_f32_(
      gpu.kthvalue(/*k=*/2, /*dim=*/-1, /*keepdim=*/false),
      cpu.kthvalue(/*k=*/2, /*dim=*/-1, /*keepdim=*/false));
  require_equal_i64_(
      gpu.argsort(/*dim=*/-1, /*descending=*/false, /*stable=*/false),
      cpu.argsort(/*dim=*/-1, /*descending=*/false, /*stable=*/false));
}

TEST_CASE("CUDA strict selection rejects unsupported lanes", "[cuda][bridge][policy][selection][reject]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar selection_on("PLUSS_CUDA_NATIVE_SELECTION", "1");
  const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_SELECTION_STRICT", "1");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto gpu = pluss::make_series(std::vector<float>{1.0f, 2.0f, 3.0f, 4.0f}, {2, 2}).to(pluss::pCUDA);

  REQUIRE_FALSE(pluss::cuda::supports_topk_selection(gpu, /*k=*/3, /*dim=*/-1, /*sorted=*/true));
  REQUIRE_FALSE(pluss::cuda::supports_kthvalue_selection(gpu, /*k=*/3, /*dim=*/-1));

  REQUIRE_THROWS(gpu.topk(/*k=*/3, /*dim=*/-1, /*largest=*/true, /*sorted=*/true));
  REQUIRE_THROWS(gpu.kthvalue(/*k=*/3, /*dim=*/-1, /*keepdim=*/false));
}

#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>
#include <pluss/subsp/linearalg/linearalg.h>

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

void require_close_(const std::vector<float>& got, const std::vector<float>& want, float tol = 1e-4f) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(std::fabs(got[i] - want[i]) <= tol);
  }
}

} // namespace

TEST_CASE("CUDA bridge-backed Series transfer contract", "[cuda][bridge][runtime]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const bool cuda_ready = pluss::cuda::is_ready();
  if (!cuda_ready) {
    try {
      (void)pluss::Series::empty({2, 2}, pluss::ScalarType::Float32, pluss::codem::Platform::CUDA);
      FAIL("CUDA empty should fail when runtime is unavailable");
    } catch (const std::exception& e) {
      REQUIRE(std::string(e.what()).find("CUDA runtime unavailable") != std::string::npos);
    }
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto x_cpu = pluss::Series::ones({2, 3}, pluss::ScalarType::Float32);
  const auto x_cuda = x_cpu.to(pluss::pCUDA);
  REQUIRE(x_cuda.platform() == pluss::pCUDA);
  const auto roundtrip = x_cuda.to(pluss::pCPU);
  require_close_(roundtrip.to_vec<float>(), x_cpu.to_vec<float>());
}

TEST_CASE("CUDA bridge enables selection fallback when strict native is off", "[cuda][bridge][selection]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar selection_off("PLUSS_CUDA_NATIVE_SELECTION", "0");
  const bool cuda_ready = pluss::cuda::is_ready();
  if (!cuda_ready) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto x_cpu =
      pluss::make_series(std::vector<float>{3.f, -1.f, 4.f, 2.f, 0.f, 5.f}, {2, 3});
  const auto x_cuda = x_cpu.to(pluss::pCUDA);

  const auto sort_cpu = x_cpu.sort(/*dim=*/1, /*descending=*/false, /*stable=*/false);
  const auto sort_cuda = x_cuda.sort(/*dim=*/1, /*descending=*/false, /*stable=*/false);
  REQUIRE(sort_cuda.platform() == pluss::pCUDA);
  require_close_(sort_cuda.to(pluss::pCPU).to_vec<float>(), sort_cpu.to_vec<float>());

  const auto topk_cpu = x_cpu.topk(/*k=*/2, /*dim=*/1, /*largest=*/true, /*sorted=*/true);
  const auto topk_cuda = x_cuda.topk(/*k=*/2, /*dim=*/1, /*largest=*/true, /*sorted=*/true);
  REQUIRE(topk_cuda.platform() == pluss::pCUDA);
  require_close_(topk_cuda.to(pluss::pCPU).to_vec<float>(), topk_cpu.to_vec<float>());
}

TEST_CASE("CUDA bridge enables linalg fallback when strict native is off", "[cuda][bridge][linalg]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar linalg_off("PLUSS_CUDA_NATIVE_LINALG", "0");
  const bool cuda_ready = pluss::cuda::is_ready();
  if (!cuda_ready) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto a_cpu = pluss::make_series(std::vector<float>{4.f, 1.f, 1.f, 3.f}, {2, 2});
  const auto a_cuda = a_cpu.to(pluss::pCUDA);

  const auto det_cpu = pluss::subsp::linearalg::det(a_cpu);
  const auto det_cuda = pluss::subsp::linearalg::det(a_cuda);
  REQUIRE(det_cuda.platform() == pluss::pCUDA);
  require_close_(det_cuda.to(pluss::pCPU).to_vec<float>(), det_cpu.to_vec<float>());

  const auto inv_cpu = pluss::subsp::linearalg::inverse(a_cpu);
  const auto inv_cuda = pluss::subsp::linearalg::inverse(a_cuda);
  REQUIRE(inv_cuda.platform() == pluss::pCUDA);
  require_close_(inv_cuda.to(pluss::pCPU).to_vec<float>(), inv_cpu.to_vec<float>(), 2e-3f);
}

TEST_CASE("CUDA strict native linalg executes supported native route", "[cuda][bridge][policy]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar linalg_on("PLUSS_CUDA_NATIVE_LINALG", "1");
  const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_LINALG_STRICT", "1");
  const bool cuda_ready = pluss::cuda::is_ready();
  if (!cuda_ready) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto a_cpu = pluss::make_series(std::vector<float>{2.f, 0.f, 0.f, 2.f}, {2, 2});
  const auto a_cuda = a_cpu.to(pluss::pCUDA);
  const auto det_cpu = pluss::subsp::linearalg::det(a_cpu);
  const auto det_cuda = pluss::subsp::linearalg::det(a_cuda);
  REQUIRE(det_cuda.platform() == pluss::pCUDA);
  require_close_(det_cuda.to(pluss::pCPU).to_vec<float>(), det_cpu.to_vec<float>(), 2e-3f);
}

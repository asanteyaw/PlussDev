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

void require_close_(const std::vector<float>& got, const std::vector<float>& want, float tol = 5e-4f) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(std::fabs(got[i] - want[i]) <= tol);
  }
}

} // namespace

TEST_CASE("CUDA cuBLAS strict matmul route executes without CPU fallback", "[cuda][cublas][strict]") {
  const ScopedEnvVar bridge_off("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "0");
  const ScopedEnvVar cublas_on("PLUSS_CUBLAS_MATMUL", "1");
  const ScopedEnvVar cublas_strict("PLUSS_CUBLAS_MATMUL_STRICT", "1");
  const ScopedEnvVar native_matmul_off("PLUSS_CUDA_NATIVE_MATMUL", "0");
  const ScopedEnvVar native_matmul_strict_off("PLUSS_CUDA_NATIVE_MATMUL_STRICT", "0");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }
  if (!pluss::cuda::cublas_available()) {
    SUCCEED("cuBLAS provider unavailable on this host");
    return;
  }

  const auto a_cpu = pluss::make_series(
      std::vector<float>{
          1.0f, -2.0f, 3.0f,
          0.5f, 4.0f, -1.0f,
      },
      {2, 3});
  const auto b_cpu = pluss::make_series(
      std::vector<float>{
          2.0f, -1.0f,
          0.0f, 3.0f,
          -4.0f, 1.0f,
      },
      {3, 2});

  const auto ref = a_cpu.matmul(b_cpu).to_vec<float>();
  const auto a_cuda = a_cpu.to(pluss::pCUDA);
  const auto b_cuda = b_cpu.to(pluss::pCUDA);
  REQUIRE(a_cuda.storage().location() == pluss::codem::StorageLocation::CudaDevice);
  REQUIRE(b_cuda.storage().location() == pluss::codem::StorageLocation::CudaDevice);

  auto got_cuda = pluss::series::undefined();
  {
    const ScopedEnvVar no_materialize("PLUSS_CUDA_FORBID_CPU_MATERIALIZATION", "1");
    got_cuda = a_cuda.matmul(b_cuda);
    REQUIRE(got_cuda.platform() == pluss::pCUDA);
    REQUIRE(got_cuda.storage().location() == pluss::codem::StorageLocation::CudaDevice);
  }

  const ScopedEnvVar readback_bridge("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const auto got = got_cuda.to(pluss::pCPU).to_vec<float>();
  require_close_(got, ref);
}

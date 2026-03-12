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

TEST_CASE("CUDA linalg baseline capability is runtime-dynamic", "[cuda][linalg][runtime]") {
  const bool cuda_ready = pluss::cuda::is_ready();
  {
    const ScopedEnvVar baseline_off("PLUSS_CUDA_NATIVE_BASELINE", "0");
    const ScopedEnvVar linalg_on("PLUSS_CUDA_NATIVE_LINALG", "1");
    REQUIRE(pluss::cuda::native_linalg_enabled());
    REQUIRE_FALSE(pluss::cuda::supports_det_linalg_baseline(pluss::ScalarType::Float32));
    REQUIRE_FALSE(pluss::cuda::supports_inverse_linalg_baseline(pluss::ScalarType::Float32));
    REQUIRE_FALSE(pluss::cuda::supports_cholesky_linalg_baseline(pluss::ScalarType::Float32));
    REQUIRE_FALSE(pluss::cuda::supports_qr_linalg_baseline(pluss::ScalarType::Float32, /*reduced=*/true));
    REQUIRE_FALSE(pluss::cuda::supports_svd_linalg_baseline(pluss::ScalarType::Float32, /*full_matrices=*/false));
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    const ScopedEnvVar linalg_off("PLUSS_CUDA_NATIVE_LINALG", "0");
    REQUIRE_FALSE(pluss::cuda::native_linalg_enabled());
    REQUIRE_FALSE(pluss::cuda::supports_det_linalg_baseline(pluss::ScalarType::Float32));
    REQUIRE_FALSE(pluss::cuda::supports_inverse_linalg_baseline(pluss::ScalarType::Float32));
    REQUIRE_FALSE(pluss::cuda::supports_cholesky_linalg_baseline(pluss::ScalarType::Float32));
    REQUIRE_FALSE(pluss::cuda::supports_qr_linalg_baseline(pluss::ScalarType::Float32, /*reduced=*/true));
    REQUIRE_FALSE(pluss::cuda::supports_svd_linalg_baseline(pluss::ScalarType::Float32, /*full_matrices=*/false));
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    const ScopedEnvVar linalg_on("PLUSS_CUDA_NATIVE_LINALG", "1");
    REQUIRE(pluss::cuda::native_linalg_enabled());
    REQUIRE(pluss::cuda::supports_det_linalg_baseline(pluss::ScalarType::Float32) == cuda_ready);
    REQUIRE(pluss::cuda::supports_inverse_linalg_baseline(pluss::ScalarType::Float64) == cuda_ready);
    REQUIRE(pluss::cuda::supports_cholesky_linalg_baseline(pluss::ScalarType::Float32) == cuda_ready);
    REQUIRE(pluss::cuda::supports_logdet_linalg_baseline(pluss::ScalarType::Float32) == cuda_ready);
    REQUIRE(pluss::cuda::supports_slogdet_linalg_baseline(pluss::ScalarType::Float32) == cuda_ready);
    REQUIRE(pluss::cuda::supports_pinverse_linalg_baseline(pluss::ScalarType::Float64) == cuda_ready);
    REQUIRE_FALSE(pluss::cuda::supports_det_linalg_baseline(pluss::ScalarType::Int64));

    REQUIRE(
        pluss::cuda::supports_triangular_solve_linalg_baseline(pluss::ScalarType::Float32, pluss::ScalarType::Float32) ==
        cuda_ready);
    REQUIRE_FALSE(
        pluss::cuda::supports_triangular_solve_linalg_baseline(pluss::ScalarType::Float32, pluss::ScalarType::Float64));
    REQUIRE(
        pluss::cuda::supports_cholesky_solve_linalg_baseline(pluss::ScalarType::Float32, pluss::ScalarType::Float32) ==
        cuda_ready);
    REQUIRE(pluss::cuda::supports_cholesky_inverse_linalg_baseline(pluss::ScalarType::Float32) == cuda_ready);
    REQUIRE(
        pluss::cuda::supports_lu_solve_linalg_baseline(pluss::ScalarType::Float64, pluss::ScalarType::Float64) ==
        cuda_ready);

    REQUIRE(pluss::cuda::supports_qr_linalg_baseline(pluss::ScalarType::Float32, /*reduced=*/true) == cuda_ready);
    REQUIRE_FALSE(pluss::cuda::supports_qr_linalg_baseline(pluss::ScalarType::Float32, /*reduced=*/false));
    REQUIRE(
        pluss::cuda::supports_svd_linalg_baseline(pluss::ScalarType::Float32, /*full_matrices=*/false) ==
        cuda_ready);
    REQUIRE_FALSE(pluss::cuda::supports_svd_linalg_baseline(pluss::ScalarType::Float32, /*full_matrices=*/true));

    REQUIRE(pluss::cuda::supports_matrix_power_linalg_baseline(pluss::ScalarType::Float32) == cuda_ready);
    REQUIRE(pluss::cuda::supports_matrix_exp_linalg_baseline(pluss::ScalarType::Float32) == cuda_ready);
  }
}

TEST_CASE("CUDA linalg strict policy is runtime-dynamic", "[cuda][linalg][policy][runtime]") {
  {
    const ScopedEnvVar strict_off("PLUSS_CUDA_NATIVE_LINALG_STRICT", "0");
    REQUIRE_FALSE(pluss::cuda::native_linalg_strict());
  }
  {
    const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_LINALG_STRICT", "1");
    REQUIRE(pluss::cuda::native_linalg_strict());
  }
}

TEST_CASE("CUDA linalg route contract rejects non-CUDA tensors", "[cuda][linalg][policy]") {
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar linalg_on("PLUSS_CUDA_NATIVE_LINALG", "1");
  const auto a_cpu = pluss::Series::eye(4, 4, pluss::ScalarType::Float32);
  const auto b_cpu = pluss::Series::ones({4, 2}, pluss::ScalarType::Float32);
  REQUIRE(a_cpu.platform() == pluss::pCPU);
  REQUIRE_FALSE(pluss::cuda::supports_det_linalg(a_cpu));
  REQUIRE_FALSE(pluss::cuda::supports_inverse_linalg(a_cpu));
  REQUIRE_FALSE(pluss::cuda::supports_cholesky_linalg(a_cpu));
  REQUIRE_FALSE(pluss::cuda::supports_logdet_linalg(a_cpu));
  REQUIRE_FALSE(pluss::cuda::supports_slogdet_linalg(a_cpu));
  REQUIRE_FALSE(pluss::cuda::supports_pinverse_linalg(a_cpu));
  REQUIRE_FALSE(pluss::cuda::supports_triangular_solve_linalg(a_cpu, b_cpu));
  REQUIRE_FALSE(pluss::cuda::supports_cholesky_solve_linalg(a_cpu, b_cpu));
  REQUIRE_FALSE(pluss::cuda::supports_cholesky_inverse_linalg(a_cpu));
  REQUIRE_FALSE(pluss::cuda::supports_lu_solve_linalg(a_cpu, b_cpu));
  REQUIRE_FALSE(pluss::cuda::supports_qr_linalg(a_cpu, /*reduced=*/true));
  REQUIRE_FALSE(pluss::cuda::supports_svd_linalg(a_cpu, /*full_matrices=*/false));
  REQUIRE_FALSE(pluss::cuda::supports_matrix_power_linalg(a_cpu));
  REQUIRE_FALSE(pluss::cuda::supports_matrix_exp_linalg(a_cpu));
}

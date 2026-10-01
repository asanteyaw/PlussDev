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

TEST_CASE("CUDA bootstrap backend info is coherent", "[cuda][bootstrap][backend]") {
  using namespace pluss::subsp::linearalg;

  const auto cuda = linearalg_backend_info(LinearAlgTarget::CUDA);
  const bool cuda_ready = pluss::cuda::is_ready();
  const bool cublas_ready = pluss::cuda::cublas_available();
  const bool cusolver_ready = pluss::cuda::cusolver_available();
  REQUIRE(cuda.target == LinearAlgTarget::CUDA);
  REQUIRE(cuda.dense_provider == (cublas_ready ? LinearAlgDenseProvider::CuBLAS : LinearAlgDenseProvider::None));
  REQUIRE(cuda.solver_provider == (cusolver_ready ? LinearAlgSolverProvider::CuSolver : LinearAlgSolverProvider::None));
  REQUIRE(cuda.dense_available == cublas_ready);
  REQUIRE(cuda.solver_available == cusolver_ready);
  REQUIRE(cuda.supports_matmul == pluss::cuda::supports_matmul_baseline(
      pluss::ScalarType::Float32, pluss::ScalarType::Float32));
  REQUIRE(cuda.supports_det == pluss::cuda::supports_det_linalg_baseline(pluss::ScalarType::Float32));
  REQUIRE(cuda.supports_inverse == pluss::cuda::supports_inverse_linalg_baseline(pluss::ScalarType::Float32));
  REQUIRE(cuda.supports_cholesky == pluss::cuda::supports_cholesky_linalg_baseline(pluss::ScalarType::Float32));
  REQUIRE(cuda.supports_qr == pluss::cuda::supports_qr_linalg_baseline(pluss::ScalarType::Float32, /*reduced=*/true));
  REQUIRE(cuda.supports_svd == pluss::cuda::supports_svd_linalg_baseline(pluss::ScalarType::Float32, /*full_matrices=*/false));
  REQUIRE(
      cuda.supports_triangular_solve ==
      pluss::cuda::supports_triangular_solve_linalg_baseline(pluss::ScalarType::Float32, pluss::ScalarType::Float32));
  REQUIRE(
      cuda.supports_cholesky_solve ==
      pluss::cuda::supports_cholesky_solve_linalg_baseline(pluss::ScalarType::Float32, pluss::ScalarType::Float32));
  REQUIRE(
      cuda.supports_lu_solve ==
      pluss::cuda::supports_lu_solve_linalg_baseline(pluss::ScalarType::Float32, pluss::ScalarType::Float32));
  REQUIRE(cuda.supports_matrix_power == pluss::cuda::supports_matrix_power_linalg_baseline(pluss::ScalarType::Float32));
  REQUIRE(cuda.supports_matrix_exp == pluss::cuda::supports_matrix_exp_linalg_baseline(pluss::ScalarType::Float32));

  REQUIRE(pluss::platform_available(pluss::codem::Platform::CUDA) == cuda_ready);
}

TEST_CASE("CUDA is_ready and is_usable stay coherent", "[cuda][bootstrap][runtime]") {
  const bool ready = pluss::cuda::is_ready();
  REQUIRE(pluss::cuda::is_usable() == ready);
  REQUIRE(pluss::platform_available(pluss::codem::Platform::CUDA) == ready);
}

TEST_CASE("CUDA Series::empty is runtime-gated only", "[cuda][bootstrap][policy]") {
  const std::vector<std::int64_t> shape{2, 2};
  if (!pluss::cuda::is_ready()) {
    REQUIRE_THROWS((void)pluss::Series::empty(shape, pluss::ScalarType::Float32, pluss::codem::Platform::CUDA));
    return;
  }
  REQUIRE_NOTHROW((void)pluss::Series::empty(shape, pluss::ScalarType::Float32, pluss::codem::Platform::CUDA));
}

TEST_CASE("CUDA Series::to transfer is runtime-gated only", "[cuda][bootstrap][policy]") {
  const auto cpu = pluss::Series::ones({2, 2}, pluss::ScalarType::Float32);
  REQUIRE(cpu.platform() == pluss::codem::Platform::CPU);
  if (!pluss::cuda::is_ready()) {
    REQUIRE_THROWS((void)cpu.to(pluss::codem::Platform::CUDA));
    return;
  }
  REQUIRE_NOTHROW((void)cpu.to(pluss::codem::Platform::CUDA));
}

TEST_CASE("CUDA transfer policy is runtime-dynamic", "[cuda][bootstrap][policy][runtime]") {
  using pluss::cuda::TransferPolicy;
  {
    const ScopedEnvVar bridge_off("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "0");
    REQUIRE(pluss::cuda::cpu_bridge_enabled() == false);
    REQUIRE(pluss::cuda::transfer_policy() == TransferPolicy::Strict);
    REQUIRE(std::string(pluss::cuda::transfer_policy_name(pluss::cuda::transfer_policy())) == "Strict");
  }
  {
    const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
    REQUIRE(pluss::cuda::cpu_bridge_enabled() == true);
    REQUIRE(pluss::cuda::transfer_policy() == TransferPolicy::AllowCpuBridge);
    REQUIRE(
        std::string(pluss::cuda::transfer_policy_name(pluss::cuda::transfer_policy())) ==
        "AllowCpuBridge");
  }
}

TEST_CASE("CUDA native baseline capability is runtime-dynamic", "[cuda][baseline][runtime]") {
  const bool cuda_ready = pluss::cuda::is_ready();
  {
    const ScopedEnvVar baseline_off("PLUSS_CUDA_NATIVE_BASELINE", "0");
    REQUIRE(pluss::cuda::native_baseline_enabled() == false);
    REQUIRE(pluss::cuda::supports_elementwise_baseline(pluss::ScalarType::Float32) == false);
    REQUIRE(pluss::cuda::supports_reduction_baseline(pluss::ScalarType::Float32) == false);
    REQUIRE(pluss::cuda::supports_matmul_baseline(
        pluss::ScalarType::Float32, pluss::ScalarType::Float32) == false);
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    REQUIRE(pluss::cuda::native_baseline_enabled() == true);
    REQUIRE(
        pluss::cuda::supports_elementwise_baseline(pluss::ScalarType::Float32) == cuda_ready);
    REQUIRE(
        pluss::cuda::supports_reduction_baseline(pluss::ScalarType::Float64) == cuda_ready);
    REQUIRE_FALSE(pluss::cuda::supports_reduction_baseline(pluss::ScalarType::Float16));
    REQUIRE(
        pluss::cuda::supports_matmul_baseline(pluss::ScalarType::Float32, pluss::ScalarType::Float32) ==
        cuda_ready);
    REQUIRE(
        pluss::cuda::supports_matmul_baseline(pluss::ScalarType::Int64, pluss::ScalarType::Int64) ==
        false);
  }
}

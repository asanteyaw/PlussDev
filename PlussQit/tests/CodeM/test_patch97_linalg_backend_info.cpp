#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cstdlib>
#include <string>

namespace {

enum class SolverRouteMode : std::uint8_t {
  Auto = 0,
  Native = 1,
  Bridge = 2,
};

SolverRouteMode solver_route_mode_() {
  const char *raw = std::getenv("PLUSS_MPS_LINALG_SOLVER_ROUTE");
  if (raw == nullptr) {
    return SolverRouteMode::Auto;
  }
  const auto eq_lit = [raw](const char *lit) noexcept {
    const unsigned char *a = reinterpret_cast<const unsigned char *>(raw);
    const unsigned char *b = reinterpret_cast<const unsigned char *>(lit);
    while (*a != '\0' && *b != '\0') {
      unsigned char ca = *a;
      unsigned char cb = *b;
      if (ca >= 'A' && ca <= 'Z')
        ca = static_cast<unsigned char>(ca - 'A' + 'a');
      if (cb >= 'A' && cb <= 'Z')
        cb = static_cast<unsigned char>(cb - 'A' + 'a');
      if (ca != cb)
        return false;
      ++a;
      ++b;
    }
    return *a == '\0' && *b == '\0';
  };
  if (eq_lit("bridge") || eq_lit("reference") || eq_lit("prefer_bridge") ||
      eq_lit("cpu")) {
    return SolverRouteMode::Bridge;
  }
  if (eq_lit("native") || eq_lit("metal") || eq_lit("prefer_native")) {
    return SolverRouteMode::Native;
  }
  return SolverRouteMode::Auto;
}

} // namespace

TEST_CASE("Linear algebra backend info reports CPU capabilities",
          "[linearalg][backend][cpu]") {
  using namespace pluss::subsp::linearalg;

  const auto info = linearalg_backend_info();
  REQUIRE(info.target == LinearAlgTarget::CPU);
  REQUIRE(info.supports_matmul);
  REQUIRE(info.supports_det);
  REQUIRE(info.supports_inverse);
  REQUIRE(info.supports_cholesky);
  REQUIRE(info.supports_triangular_solve);
  REQUIRE(info.supports_cholesky_solve);
  REQUIRE(info.supports_lu_solve);
  REQUIRE(info.supports_matrix_power);
  REQUIRE(info.supports_matrix_exp);
  REQUIRE(info.supports_eigh ==
          (info.solver_provider == LinearAlgSolverProvider::Accelerate));

  const std::string dense_name =
      linearalg_dense_provider_name(info.dense_provider);
  const std::string solver_name =
      linearalg_solver_provider_name(info.solver_provider);
  REQUIRE(!dense_name.empty());
  REQUIRE(!solver_name.empty());

  if (info.solver_provider == LinearAlgSolverProvider::Accelerate) {
    REQUIRE(info.supports_qr);
    REQUIRE(info.supports_svd);
  } else {
    REQUIRE(info.solver_provider == LinearAlgSolverProvider::Reference);
    REQUIRE_FALSE(info.supports_qr);
    REQUIRE_FALSE(info.supports_svd);
  }
}

TEST_CASE("Linear algebra backend info reports MPS and CUDA coherently",
          "[linearalg][backend][accelerators]") {
  using namespace pluss::subsp::linearalg;

  const auto cpu = linearalg_backend_info(LinearAlgTarget::CPU);
  const auto mps = linearalg_backend_info(LinearAlgTarget::MPS);
  REQUIRE(mps.target == LinearAlgTarget::MPS);

  if (!pluss::mps::is_ready()) {
    REQUIRE(mps.dense_provider == LinearAlgDenseProvider::None);
    REQUIRE(mps.solver_provider == LinearAlgSolverProvider::None);
    REQUIRE_FALSE(mps.dense_available);
    REQUIRE_FALSE(mps.solver_available);
    REQUIRE_FALSE(mps.supports_matmul);
    REQUIRE_FALSE(mps.supports_det);
    REQUIRE_FALSE(mps.supports_inverse);
    REQUIRE_FALSE(mps.supports_cholesky);
    REQUIRE_FALSE(mps.supports_triangular_solve);
    REQUIRE_FALSE(mps.supports_cholesky_solve);
    REQUIRE_FALSE(mps.supports_lu_solve);
    REQUIRE_FALSE(mps.supports_matrix_power);
    REQUIRE_FALSE(mps.supports_matrix_exp);
    REQUIRE_FALSE(mps.supports_qr);
    REQUIRE_FALSE(mps.supports_svd);
    REQUIRE_FALSE(mps.supports_eigh);
  } else {
    const bool bridge_ready = linearalg_mps_cpu_bridge_enabled();
    const bool native_enabled = linearalg_mps_experimental_native_enabled();
    const bool prefer_bridge =
        (solver_route_mode_() == SolverRouteMode::Bridge);
    const bool native_ready =
        native_enabled && (!prefer_bridge || !bridge_ready);
    const bool solver_ready = native_ready || bridge_ready;
    const auto expected_provider =
        native_ready ? LinearAlgSolverProvider::Metal
                     : (bridge_ready ? LinearAlgSolverProvider::Reference
                                     : LinearAlgSolverProvider::None);
    REQUIRE(mps.dense_provider == LinearAlgDenseProvider::Metal);
    REQUIRE(mps.solver_provider == expected_provider);
    REQUIRE(mps.dense_available);
    REQUIRE(mps.solver_available == solver_ready);
    REQUIRE(mps.supports_matmul);
    REQUIRE(mps.supports_matrix_power);
    REQUIRE(mps.supports_matrix_exp);
    REQUIRE(mps.supports_det == solver_ready);
    REQUIRE(mps.supports_inverse == solver_ready);
    REQUIRE(mps.supports_cholesky == solver_ready);
    REQUIRE(mps.supports_triangular_solve == solver_ready);
    REQUIRE(mps.supports_cholesky_solve == solver_ready);
    REQUIRE(mps.supports_lu_solve == solver_ready);
    REQUIRE(mps.supports_qr == solver_ready);
    REQUIRE(mps.supports_svd == solver_ready);
    REQUIRE(mps.supports_eigh == bridge_ready);
  }

  const auto cuda = linearalg_backend_info(LinearAlgTarget::CUDA);
  const bool cuda_ready = pluss::cuda::is_ready();
  const bool cublas_ready = pluss::cuda::cublas_available();
  const bool cusolver_ready = pluss::cuda::cusolver_available();
  REQUIRE(cuda.target == LinearAlgTarget::CUDA);
  REQUIRE(cuda.dense_provider == (cublas_ready ? LinearAlgDenseProvider::CuBLAS
                                               : LinearAlgDenseProvider::None));
  REQUIRE(cuda.solver_provider == (cusolver_ready
                                       ? LinearAlgSolverProvider::CuSolver
                                       : LinearAlgSolverProvider::None));
  REQUIRE(cuda.dense_available == cublas_ready);
  REQUIRE(cuda.solver_available == cusolver_ready);
  REQUIRE(cuda.supports_matmul ==
          pluss::cuda::supports_matmul_baseline(pluss::ScalarType::Float32,
                                                pluss::ScalarType::Float32));
  REQUIRE(cuda.supports_det == pluss::cuda::supports_det_linalg_baseline(
                                   pluss::ScalarType::Float32));
  REQUIRE(cuda.supports_inverse ==
          pluss::cuda::supports_inverse_linalg_baseline(
              pluss::ScalarType::Float32));
  REQUIRE(cuda.supports_cholesky ==
          pluss::cuda::supports_cholesky_linalg_baseline(
              pluss::ScalarType::Float32));
  REQUIRE(cuda.supports_qr ==
          pluss::cuda::supports_qr_linalg_baseline(pluss::ScalarType::Float32,
                                                   /*reduced=*/true));
  REQUIRE(cuda.supports_svd ==
          pluss::cuda::supports_svd_linalg_baseline(pluss::ScalarType::Float32,
                                                    /*full_matrices=*/false));
  REQUIRE(cuda.supports_eigh ==
          (cuda_ready && pluss::codem::cuda_cpu_bridge_enabled()));
  REQUIRE(cuda.supports_triangular_solve ==
          pluss::cuda::supports_triangular_solve_linalg_baseline(
              pluss::ScalarType::Float32, pluss::ScalarType::Float32));
  REQUIRE(cuda.supports_cholesky_solve ==
          pluss::cuda::supports_cholesky_solve_linalg_baseline(
              pluss::ScalarType::Float32, pluss::ScalarType::Float32));
  REQUIRE(cuda.supports_lu_solve ==
          pluss::cuda::supports_lu_solve_linalg_baseline(
              pluss::ScalarType::Float32, pluss::ScalarType::Float32));
  REQUIRE(cuda.supports_matrix_power ==
          pluss::cuda::supports_matrix_power_linalg_baseline(
              pluss::ScalarType::Float32));
  REQUIRE(cuda.supports_matrix_exp ==
          pluss::cuda::supports_matrix_exp_linalg_baseline(
              pluss::ScalarType::Float32));
  REQUIRE(pluss::platform_available(pluss::codem::Platform::CUDA) ==
          cuda_ready);
}

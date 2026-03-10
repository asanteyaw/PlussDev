#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <string>

TEST_CASE("Linear algebra backend info reports CPU capabilities", "[linearalg][backend][cpu]") {
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

  const std::string dense_name = linearalg_dense_provider_name(info.dense_provider);
  const std::string solver_name = linearalg_solver_provider_name(info.solver_provider);
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

TEST_CASE("Linear algebra backend info reports MPS and CUDA coherently", "[linearalg][backend][accelerators]") {
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
  } else {
    REQUIRE(mps.dense_provider == LinearAlgDenseProvider::Metal);
    REQUIRE(mps.solver_provider == LinearAlgSolverProvider::Reference);
    REQUIRE(mps.dense_available);
    REQUIRE(mps.solver_available);
    REQUIRE(mps.supports_matmul);
    REQUIRE(mps.supports_det);
    REQUIRE(mps.supports_inverse);
    REQUIRE(mps.supports_cholesky);
    REQUIRE(mps.supports_triangular_solve);
    REQUIRE(mps.supports_cholesky_solve);
    REQUIRE(mps.supports_lu_solve);
    REQUIRE(mps.supports_matrix_power);
    REQUIRE(mps.supports_matrix_exp);
    REQUIRE(mps.supports_qr);
    REQUIRE(mps.supports_svd);
  }

  const auto cuda = linearalg_backend_info(LinearAlgTarget::CUDA);
  REQUIRE(cuda.target == LinearAlgTarget::CUDA);
  REQUIRE(cuda.dense_provider == LinearAlgDenseProvider::None);
  REQUIRE(cuda.solver_provider == LinearAlgSolverProvider::None);
  REQUIRE_FALSE(cuda.dense_available);
  REQUIRE_FALSE(cuda.solver_available);
  REQUIRE_FALSE(cuda.supports_matmul);
  REQUIRE_FALSE(cuda.supports_det);
  REQUIRE_FALSE(cuda.supports_inverse);
  REQUIRE_FALSE(cuda.supports_cholesky);
  REQUIRE_FALSE(cuda.supports_qr);
  REQUIRE_FALSE(cuda.supports_svd);
  REQUIRE_FALSE(cuda.supports_triangular_solve);
  REQUIRE_FALSE(cuda.supports_cholesky_solve);
  REQUIRE_FALSE(cuda.supports_lu_solve);
  REQUIRE_FALSE(cuda.supports_matrix_power);
  REQUIRE_FALSE(cuda.supports_matrix_exp);
}

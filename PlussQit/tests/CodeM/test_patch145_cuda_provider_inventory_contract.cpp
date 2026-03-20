#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>
#include <pluss/subsp/linearalg/linearalg.h>

#include <string>

TEST_CASE("CUDA runtime inventory reports coherent primary-device metadata", "[cuda][bootstrap][inventory]") {
  const bool cuda_ready = pluss::cuda::is_ready();
  const auto device_count = pluss::cuda::device_count();
  REQUIRE(device_count >= 0);

  if (!cuda_ready) {
    REQUIRE(device_count == 0);
    REQUIRE(pluss::cuda::runtime_version() == 0);
    REQUIRE(pluss::cuda::driver_version() == 0);
    REQUIRE(std::string(pluss::cuda::primary_device_name()).empty());
    REQUIRE(pluss::cuda::primary_compute_capability_major() == 0);
    REQUIRE(pluss::cuda::primary_compute_capability_minor() == 0);
    REQUIRE(pluss::cuda::primary_global_memory_bytes() == 0);
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  REQUIRE(device_count > 0);
  REQUIRE(pluss::cuda::runtime_version() > 0);
  REQUIRE(pluss::cuda::driver_version() > 0);
  REQUIRE_FALSE(std::string(pluss::cuda::primary_device_name()).empty());
  REQUIRE(pluss::cuda::primary_compute_capability_major() > 0);
  REQUIRE(pluss::cuda::primary_compute_capability_minor() >= 0);
  REQUIRE(pluss::cuda::primary_global_memory_bytes() > 0);
}

TEST_CASE("CUDA provider probes stay coherent with runtime availability", "[cuda][providers][runtime]") {
  const bool cuda_ready = pluss::cuda::is_ready();

  const bool cublas_ready = pluss::cuda::cublas_available();
  const bool cusolver_ready = pluss::cuda::cusolver_available();
  const bool cudnn_ready = pluss::cuda::cudnn_ready();

  if (!cuda_ready) {
    REQUIRE_FALSE(cublas_ready);
    REQUIRE_FALSE(cusolver_ready);
    REQUIRE_FALSE(cudnn_ready);
    REQUIRE_FALSE(std::string(pluss::cuda::cublas_error()).empty());
    REQUIRE_FALSE(std::string(pluss::cuda::cusolver_error()).empty());
    REQUIRE_FALSE(std::string(pluss::cuda::cudnn_error()).empty());
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  if (cublas_ready) {
    REQUIRE(std::string(pluss::cuda::cublas_error()).empty());
  } else {
    REQUIRE_FALSE(std::string(pluss::cuda::cublas_error()).empty());
  }

  if (cusolver_ready) {
    REQUIRE(std::string(pluss::cuda::cusolver_error()).empty());
  } else {
    REQUIRE_FALSE(std::string(pluss::cuda::cusolver_error()).empty());
  }

  if (cudnn_ready) {
    REQUIRE(std::string(pluss::cuda::cudnn_error()).empty());
  } else {
    REQUIRE_FALSE(std::string(pluss::cuda::cudnn_error()).empty());
  }
}

TEST_CASE("CUDA backend info mirrors provider probes", "[cuda][providers][backend]") {
  using namespace pluss::subsp::linearalg;

  const auto info = linearalg_backend_info(LinearAlgTarget::CUDA);
  const bool cublas_ready = pluss::cuda::cublas_available();
  const bool cusolver_ready = pluss::cuda::cusolver_available();

  REQUIRE(info.target == LinearAlgTarget::CUDA);
  REQUIRE(info.dense_provider == (cublas_ready ? LinearAlgDenseProvider::CuBLAS : LinearAlgDenseProvider::None));
  REQUIRE(info.solver_provider == (cusolver_ready ? LinearAlgSolverProvider::CuSolver : LinearAlgSolverProvider::None));
  REQUIRE(info.dense_available == cublas_ready);
  REQUIRE(info.solver_available == cusolver_ready);
}

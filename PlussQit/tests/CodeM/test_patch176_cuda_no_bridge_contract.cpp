#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cmath>
#include <cstdlib>
#include <cstdint>
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

TEST_CASE("CUDA no-bridge guard rejects explicit CPU materialization", "[cuda][no_bridge][materialization]") {
  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto x_cuda = pluss::make_series(std::vector<float>{1.f, 2.f, 3.f}, {3}).to(pluss::pCUDA);
  const ScopedEnvVar no_materialize("PLUSS_CUDA_FORBID_CPU_MATERIALIZATION", "1");

  REQUIRE_THROWS(x_cuda.to(pluss::pCPU));
}

TEST_CASE("CUDA no-bridge mode rejects CPU-reference native declarations", "[cuda][no_bridge][policy]") {
  const ScopedEnvVar bridge_off("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "0");
  const ScopedEnvVar no_materialize("PLUSS_CUDA_FORBID_CPU_MATERIALIZATION", "1");
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar elementwise_on("PLUSS_CUDA_NATIVE_ELEMENTWISE", "1");
  const ScopedEnvVar reduction_on("PLUSS_CUDA_NATIVE_REDUCTION", "1");
  const ScopedEnvVar scan_on("PLUSS_CUDA_NATIVE_SCAN", "1");
  const ScopedEnvVar indexing_on("PLUSS_CUDA_NATIVE_INDEXING", "1");
  const ScopedEnvVar selection_on("PLUSS_CUDA_NATIVE_SELECTION", "1");
  const ScopedEnvVar linalg_on("PLUSS_CUDA_NATIVE_LINALG", "1");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto x_cpu = pluss::make_series(std::vector<float>{2.f, -1.f, 5.f, 3.f, 7.f, 0.f}, {2, 3});
  const auto y_cpu = pluss::make_series(std::vector<float>{1.f, 4.f, -2.f, 6.f, 0.f, -3.f}, {2, 3});
  const auto x_cuda = x_cpu.to(pluss::pCUDA);
  const auto y_cuda = y_cpu.to(pluss::pCUDA);
  const auto square_cuda =
      pluss::make_series(std::vector<float>{2.f, 1.f, 1.f, 3.f}, {2, 2}).to(pluss::pCUDA);

  std::optional<pluss::Series> add_cuda;
  std::optional<pluss::Series> sum_cuda;
  REQUIRE_NOTHROW(add_cuda.emplace(x_cuda.add(y_cuda)));
  REQUIRE(add_cuda->platform() == pluss::pCUDA);
  REQUIRE(add_cuda->storage().location() == pluss::codem::StorageLocation::CudaDevice);
  REQUIRE_NOTHROW(sum_cuda.emplace(x_cuda.sum(/*dim=*/1, /*keepdim=*/false)));
  REQUIRE(sum_cuda->platform() == pluss::pCUDA);
  REQUIRE(sum_cuda->storage().location() == pluss::codem::StorageLocation::CudaDevice);
  REQUIRE_THROWS(x_cuda.cumsum(/*dim=*/1, /*keepdim=*/false));
  REQUIRE_THROWS(x_cuda.nonzero());
  REQUIRE_THROWS(x_cuda.sort(/*dim=*/-1, /*descending=*/false, /*stable=*/false));
  REQUIRE_THROWS(pluss::subsp::linearalg::qr_q(square_cuda, /*reduced=*/true));

  const ScopedEnvVar readback_bridge("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar allow_materialize("PLUSS_CUDA_FORBID_CPU_MATERIALIZATION", "0");
  require_close_(add_cuda->to(pluss::pCPU).host<float>(), x_cpu.add(y_cpu).host<float>());
  require_close_(sum_cuda->to(pluss::pCPU).host<float>(), x_cpu.sum(/*dim=*/1, /*keepdim=*/false).host<float>());
}

TEST_CASE("CUDA no-bridge provider routes do not materialize CPU internally", "[cuda][no_bridge][provider]") {
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
  const auto a_cuda = a_cpu.to(pluss::pCUDA);
  const auto b_cuda = b_cpu.to(pluss::pCUDA);
  REQUIRE(a_cuda.storage().location() == pluss::codem::StorageLocation::CudaDevice);
  REQUIRE(b_cuda.storage().location() == pluss::codem::StorageLocation::CudaDevice);

  std::optional<pluss::Series> out_cuda;
  {
    const ScopedEnvVar no_materialize("PLUSS_CUDA_FORBID_CPU_MATERIALIZATION", "1");
    out_cuda.emplace(a_cuda.matmul(b_cuda));
    REQUIRE(out_cuda->platform() == pluss::pCUDA);
    REQUIRE(out_cuda->storage().location() == pluss::codem::StorageLocation::CudaDevice);
  }

  const ScopedEnvVar readback_bridge("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar allow_result_readback("PLUSS_CUDA_FORBID_CPU_MATERIALIZATION", "0");
  require_close_(out_cuda->to(pluss::pCPU).host<float>(), a_cpu.matmul(b_cpu).host<float>());
}

TEST_CASE("CUDA device storage roundtrips through platform transfer", "[cuda][device_storage][transfer]") {
  const ScopedEnvVar device_storage("PLUSS_CUDA_DEVICE_STORAGE", "1");
  const ScopedEnvVar bridge_off("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "0");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto x_cpu = pluss::make_series(std::vector<float>{1.f, -2.f, 3.5f, 0.f}, {2, 2});
  const auto x_cuda = x_cpu.to(pluss::pCUDA);

  REQUIRE(x_cuda.platform() == pluss::pCUDA);
  REQUIRE(x_cuda.storage().location() == pluss::codem::StorageLocation::CudaDevice);
  const ScopedEnvVar readback_bridge("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  require_close_(x_cuda.to(pluss::pCPU).host<float>(), x_cpu.host<float>());
}

TEST_CASE("CUDA device zeros use device memset", "[cuda][device_storage][factory]") {
  const ScopedEnvVar device_storage("PLUSS_CUDA_DEVICE_STORAGE", "1");
  const ScopedEnvVar bridge_off("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "0");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto z = pluss::Series::zeros({5}, pluss::ScalarType::Float32).to(pluss::pCUDA);
  REQUIRE(z.storage().location() == pluss::codem::StorageLocation::CudaDevice);
  const ScopedEnvVar readback_bridge("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  require_close_(z.to(pluss::pCPU).host<float>(), std::vector<float>{0.f, 0.f, 0.f, 0.f, 0.f});
}

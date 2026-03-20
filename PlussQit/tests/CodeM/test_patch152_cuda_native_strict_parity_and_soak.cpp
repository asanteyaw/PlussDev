#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>
#include <pluss/subsp/linearalg/linearalg.h>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <random>
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

void require_close_(const std::vector<float>& got, const std::vector<float>& want, float tol = 2e-4f) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(std::fabs(got[i] - want[i]) <= tol);
  }
}

void require_equal_(const std::vector<std::int64_t>& got, const std::vector<std::int64_t>& want) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(got[i] == want[i]);
  }
}

} // namespace

TEST_CASE("CUDA strict native parity covers expanded family matrix", "[cuda][parity][native][strict]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar selection_on("PLUSS_CUDA_NATIVE_SELECTION", "1");
  const ScopedEnvVar selection_strict("PLUSS_CUDA_NATIVE_SELECTION_STRICT", "1");
  const ScopedEnvVar linalg_on("PLUSS_CUDA_NATIVE_LINALG", "1");
  const ScopedEnvVar linalg_strict("PLUSS_CUDA_NATIVE_LINALG_STRICT", "1");
  const ScopedEnvVar indexing_on("PLUSS_CUDA_NATIVE_INDEXING", "1");
  const ScopedEnvVar indexing_strict("PLUSS_CUDA_NATIVE_INDEXING_STRICT", "1");
  const ScopedEnvVar matmul_on("PLUSS_CUDA_NATIVE_MATMUL", "1");
  const ScopedEnvVar matmul_strict("PLUSS_CUDA_NATIVE_MATMUL_STRICT", "1");
  const ScopedEnvVar scatter_on("PLUSS_CUDA_NATIVE_SCATTER_GATHER", "1");
  const ScopedEnvVar scatter_strict("PLUSS_CUDA_NATIVE_SCATTER_GATHER_STRICT", "1");
  const ScopedEnvVar reduction_on("PLUSS_CUDA_NATIVE_REDUCTION", "1");
  const ScopedEnvVar reduction_strict("PLUSS_CUDA_NATIVE_REDUCTION_STRICT", "1");
  const ScopedEnvVar elementwise_on("PLUSS_CUDA_NATIVE_ELEMENTWISE", "1");
  const ScopedEnvVar elementwise_strict("PLUSS_CUDA_NATIVE_ELEMENTWISE_STRICT", "1");
  const ScopedEnvVar scan_on("PLUSS_CUDA_NATIVE_SCAN", "1");
  const ScopedEnvVar scan_strict("PLUSS_CUDA_NATIVE_SCAN_STRICT", "1");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  std::mt19937 rng(20260311u);
  std::uniform_real_distribution<float> dist(-2.5f, 2.5f);

  for (int rep = 0; rep < 20; ++rep) {
    std::vector<float> xdata(12);
    std::vector<float> ydata(8);
    std::vector<float> srcdata(4);
    for (auto& v : xdata) v = dist(rng);
    for (auto& v : ydata) v = dist(rng);
    for (auto& v : srcdata) v = dist(rng);

    const auto x_cpu = pluss::make_series(xdata, {3, 4});
    const auto y_cpu = pluss::make_series(ydata, {4, 2});
    const auto x_cuda = x_cpu.to(pluss::pCUDA);
    const auto y_cuda = y_cpu.to(pluss::pCUDA);

    require_close_(
        x_cuda.matmul(y_cuda).to(pluss::pCPU).to_vec<float>(),
        x_cpu.matmul(y_cpu).to_vec<float>());

    require_close_(
        x_cuda.sort(/*dim=*/1, /*descending=*/false, /*stable=*/false).to(pluss::pCPU).to_vec<float>(),
        x_cpu.sort(/*dim=*/1, /*descending=*/false, /*stable=*/false).to_vec<float>());

    require_close_(
        x_cuda.topk(/*k=*/2, /*dim=*/1, /*largest=*/true, /*sorted=*/true).to(pluss::pCPU).to_vec<float>(),
        x_cpu.topk(/*k=*/2, /*dim=*/1, /*largest=*/true, /*sorted=*/true).to_vec<float>());

    require_equal_(
        x_cuda.argsort(/*dim=*/1, /*descending=*/true, /*stable=*/false).to(pluss::pCPU).to_vec<std::int64_t>(),
        x_cpu.argsort(/*dim=*/1, /*descending=*/true, /*stable=*/false).to_vec<std::int64_t>());

    const auto flat_cpu = x_cpu.reshape({x_cpu.numel()});
    const auto flat_cuda = x_cuda.reshape({x_cuda.numel()});
    const auto idx_cpu = pluss::make_series(std::vector<std::int64_t>{11, 0, 5, 7}, {4});
    const auto idx_cuda = idx_cpu.to(pluss::pCUDA);

    require_close_(
        flat_cuda.take(idx_cuda).to(pluss::pCPU).to_vec<float>(),
        flat_cpu.take(idx_cpu).to_vec<float>());

    auto put_cpu = pluss::Series::zeros({flat_cpu.numel()}, pluss::ScalarType::Float32);
    auto put_cuda = put_cpu.to(pluss::pCUDA);
    const auto src_cpu = pluss::make_series(srcdata, {4});
    const auto src_cuda = src_cpu.to(pluss::pCUDA);
    auto put_fn = static_cast<pluss::Series (pluss::Series::*)(const pluss::Series&, const pluss::Series&, bool)>(&pluss::Series::put_);
    put_cpu = (put_cpu.*put_fn)(idx_cpu, src_cpu, /*accumulate=*/false);
    put_cuda = (put_cuda.*put_fn)(idx_cuda, src_cuda, /*accumulate=*/false);
    require_close_(put_cuda.to(pluss::pCPU).to_vec<float>(), put_cpu.to_vec<float>());

    const auto gather_idx_cpu =
        pluss::make_series(std::vector<std::int64_t>{0, 1, 2, 3, 3, 2, 1, 0, 1, 1, 2, 0}, {3, 4});
    const auto gather_idx_cuda = gather_idx_cpu.to(pluss::pCUDA);

    require_close_(
        x_cuda.gather(1, gather_idx_cuda).to(pluss::pCPU).to_vec<float>(),
        x_cpu.gather(1, gather_idx_cpu).to_vec<float>());

    const auto scatter_base_cpu = pluss::Series::zeros({3, 4}, pluss::ScalarType::Float32);
    const auto scatter_base_cuda = scatter_base_cpu.to(pluss::pCUDA);

    require_close_(
        scatter_base_cuda.scatter(1, gather_idx_cuda, x_cuda).to(pluss::pCPU).to_vec<float>(),
        scatter_base_cpu.scatter(1, gather_idx_cpu, x_cpu).to_vec<float>());

    require_close_(
        scatter_base_cuda.scatter_add(1, gather_idx_cuda, x_cuda).to(pluss::pCPU).to_vec<float>(),
        scatter_base_cpu.scatter_add(1, gather_idx_cpu, x_cpu).to_vec<float>());
    require_close_(
        x_cuda.cumsum(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<float>(),
        x_cpu.cumsum(/*dim=*/1, /*keepdim=*/false).to_vec<float>());
    require_close_(
        x_cuda.softmax(1).to(pluss::pCPU).to_vec<float>(),
        x_cpu.softmax(1).to_vec<float>());

    const float shift = 0.05f * static_cast<float>(rep);
    const auto a_cpu = pluss::make_series(std::vector<float>{4.f + shift, 1.f, 1.f, 3.f + shift}, {2, 2});
    const auto a_cuda = a_cpu.to(pluss::pCUDA);

    require_close_(
        pluss::subsp::linearalg::det(a_cuda).to(pluss::pCPU).to_vec<float>(),
        pluss::subsp::linearalg::det(a_cpu).to_vec<float>(),
        2e-3f);

    require_close_(
        pluss::subsp::linearalg::inverse(a_cuda).to(pluss::pCPU).to_vec<float>(),
        pluss::subsp::linearalg::inverse(a_cpu).to_vec<float>(),
        4e-3f);
  }
}

TEST_CASE("CUDA strict native soak loop remains stable", "[cuda][parity][native][soak]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar selection_on("PLUSS_CUDA_NATIVE_SELECTION", "1");
  const ScopedEnvVar selection_strict("PLUSS_CUDA_NATIVE_SELECTION_STRICT", "1");
  const ScopedEnvVar linalg_on("PLUSS_CUDA_NATIVE_LINALG", "1");
  const ScopedEnvVar linalg_strict("PLUSS_CUDA_NATIVE_LINALG_STRICT", "1");
  const ScopedEnvVar indexing_on("PLUSS_CUDA_NATIVE_INDEXING", "1");
  const ScopedEnvVar indexing_strict("PLUSS_CUDA_NATIVE_INDEXING_STRICT", "1");
  const ScopedEnvVar matmul_on("PLUSS_CUDA_NATIVE_MATMUL", "1");
  const ScopedEnvVar matmul_strict("PLUSS_CUDA_NATIVE_MATMUL_STRICT", "1");
  const ScopedEnvVar scatter_on("PLUSS_CUDA_NATIVE_SCATTER_GATHER", "1");
  const ScopedEnvVar scatter_strict("PLUSS_CUDA_NATIVE_SCATTER_GATHER_STRICT", "1");
  const ScopedEnvVar reduction_on("PLUSS_CUDA_NATIVE_REDUCTION", "1");
  const ScopedEnvVar reduction_strict("PLUSS_CUDA_NATIVE_REDUCTION_STRICT", "1");
  const ScopedEnvVar elementwise_on("PLUSS_CUDA_NATIVE_ELEMENTWISE", "1");
  const ScopedEnvVar elementwise_strict("PLUSS_CUDA_NATIVE_ELEMENTWISE_STRICT", "1");
  const ScopedEnvVar scan_on("PLUSS_CUDA_NATIVE_SCAN", "1");
  const ScopedEnvVar scan_strict("PLUSS_CUDA_NATIVE_SCAN_STRICT", "1");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  std::mt19937 rng(20260312u);
  std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

  for (int rep = 0; rep < 80; ++rep) {
    std::vector<float> xdata(6);
    std::vector<float> ydata(6);
    for (auto& v : xdata) v = dist(rng);
    for (auto& v : ydata) v = dist(rng);

    const auto x_cpu = pluss::make_series(xdata, {2, 3});
    const auto y_cpu = pluss::make_series(ydata, {3, 2});
    const auto x_cuda = x_cpu.to(pluss::pCUDA);
    const auto y_cuda = y_cpu.to(pluss::pCUDA);

    require_close_(
        x_cuda.matmul(y_cuda).to(pluss::pCPU).to_vec<float>(),
        x_cpu.matmul(y_cpu).to_vec<float>(),
        3e-4f);

    require_close_(
        x_cuda.sort(/*dim=*/1, /*descending=*/true, /*stable=*/false).to(pluss::pCPU).to_vec<float>(),
        x_cpu.sort(/*dim=*/1, /*descending=*/true, /*stable=*/false).to_vec<float>(),
        3e-4f);
    require_close_(
        x_cuda.cumsum(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<float>(),
        x_cpu.cumsum(/*dim=*/1, /*keepdim=*/false).to_vec<float>(),
        3e-4f);

    const auto idx_cpu = pluss::make_series(std::vector<std::int64_t>{2, 0, 1}, {3});
    const auto idx_cuda = idx_cpu.to(pluss::pCUDA);
    require_close_(
        x_cuda.reshape({x_cuda.numel()}).take(idx_cuda).to(pluss::pCPU).to_vec<float>(),
        x_cpu.reshape({x_cpu.numel()}).take(idx_cpu).to_vec<float>(),
        3e-4f);

    const float eps = 0.02f * static_cast<float>(rep % 10);
    const auto a_cpu = pluss::make_series(std::vector<float>{3.5f + eps, 0.5f, 0.5f, 2.25f + eps}, {2, 2});
    const auto a_cuda = a_cpu.to(pluss::pCUDA);
    require_close_(
        pluss::subsp::linearalg::det(a_cuda).to(pluss::pCPU).to_vec<float>(),
        pluss::subsp::linearalg::det(a_cpu).to_vec<float>(),
        4e-3f);
  }
}

TEST_CASE("CUDA provider inventory probes stay stable under repeated queries", "[cuda][providers][inventory][stress]") {
  const bool cuda_ready = pluss::cuda::is_ready();
  const auto device_count = pluss::cuda::device_count();
  const auto runtime_ver = pluss::cuda::runtime_version();
  const auto driver_ver = pluss::cuda::driver_version();
  const auto device_name = std::string(pluss::cuda::primary_device_name());
  const auto cc_major = pluss::cuda::primary_compute_capability_major();
  const auto cc_minor = pluss::cuda::primary_compute_capability_minor();
  const auto global_mem = pluss::cuda::primary_global_memory_bytes();

  const bool cublas_ready = pluss::cuda::cublas_available();
  const bool cusolver_ready = pluss::cuda::cusolver_available();
  const bool cudnn_ready = pluss::cuda::cudnn_ready();
  const auto cublas_err = std::string(pluss::cuda::cublas_error());
  const auto cusolver_err = std::string(pluss::cuda::cusolver_error());
  const auto cudnn_err = std::string(pluss::cuda::cudnn_error());

  for (int rep = 0; rep < 128; ++rep) {
    REQUIRE(pluss::cuda::is_ready() == cuda_ready);
    REQUIRE(pluss::cuda::device_count() == device_count);
    REQUIRE(pluss::cuda::runtime_version() == runtime_ver);
    REQUIRE(pluss::cuda::driver_version() == driver_ver);
    REQUIRE(std::string(pluss::cuda::primary_device_name()) == device_name);
    REQUIRE(pluss::cuda::primary_compute_capability_major() == cc_major);
    REQUIRE(pluss::cuda::primary_compute_capability_minor() == cc_minor);
    REQUIRE(pluss::cuda::primary_global_memory_bytes() == global_mem);

    REQUIRE(pluss::cuda::cublas_available() == cublas_ready);
    REQUIRE(pluss::cuda::cusolver_available() == cusolver_ready);
    REQUIRE(pluss::cuda::cudnn_ready() == cudnn_ready);
    REQUIRE(std::string(pluss::cuda::cublas_error()) == cublas_err);
    REQUIRE(std::string(pluss::cuda::cusolver_error()) == cusolver_err);
    REQUIRE(std::string(pluss::cuda::cudnn_error()) == cudnn_err);
  }

  if (!cuda_ready) {
    REQUIRE(device_count == 0);
    REQUIRE(runtime_ver == 0);
    REQUIRE(driver_ver == 0);
    REQUIRE(device_name.empty());
    REQUIRE(cc_major == 0);
    REQUIRE(cc_minor == 0);
    REQUIRE(global_mem == 0);
  } else {
    REQUIRE(device_count > 0);
    REQUIRE(runtime_ver > 0);
    REQUIRE(driver_ver > 0);
    REQUIRE_FALSE(device_name.empty());
    REQUIRE(cc_major > 0);
    REQUIRE(cc_minor >= 0);
    REQUIRE(global_mem > 0);
  }
}

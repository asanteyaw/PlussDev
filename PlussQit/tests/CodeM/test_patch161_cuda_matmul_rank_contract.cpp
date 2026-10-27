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

void require_close_(const std::vector<float>& got, const std::vector<float>& want, float tol = 1e-5f) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(std::fabs(got[i] - want[i]) <= tol);
  }
}

} // namespace

TEST_CASE("CUDA matmul rank-aware capability contract is runtime-dynamic", "[cuda][matmul][rank][runtime]") {
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar matmul_on("PLUSS_CUDA_NATIVE_MATMUL", "1");
  const bool cuda_ready = pluss::cuda::is_ready();

  const auto vec_cpu = pluss::make_series(std::vector<float>{1.f, 2.f, 3.f}, {3});
  const auto mat_cpu = pluss::make_series(std::vector<float>{1.f, 4.f, 2.f, 5.f, 3.f, 6.f}, {3, 2});
  const auto batch_a_cpu = pluss::make_series(
      std::vector<float>{
          1.f, 2.f, 3.f, 4.f, 5.f, 6.f,
          7.f, 8.f, 9.f, 10.f, 11.f, 12.f},
      {2, 2, 3});
  const auto batch_b_cpu = pluss::make_series(
      std::vector<float>{
          1.f, 0.f, 2.f, 1.f, 3.f, 2.f,
          4.f, 1.f, 5.f, 2.f, 6.f, 3.f},
      {2, 3, 2});

  REQUIRE_FALSE(pluss::cuda::supports_matmul_native(vec_cpu, mat_cpu));

  if (!cuda_ready) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto vec_cuda = vec_cpu.to(pluss::pCUDA);
  const auto mat_cuda = mat_cpu.to(pluss::pCUDA);
  const auto batch_a_cuda = batch_a_cpu.to(pluss::pCUDA);
  const auto batch_b_cuda = batch_b_cpu.to(pluss::pCUDA);
  const auto bad_b_cuda = pluss::make_series(std::vector<float>{1.f, 2.f, 3.f, 4.f}, {2, 2}).to(pluss::pCUDA);

  REQUIRE(pluss::cuda::supports_matmul_native(vec_cuda, mat_cuda));
  REQUIRE(pluss::cuda::supports_matmul_native(mat_cuda, vec_cuda));
  REQUIRE(pluss::cuda::supports_matmul_native(batch_a_cuda, batch_b_cuda));
  REQUIRE_FALSE(pluss::cuda::supports_matmul_native(batch_a_cuda, bad_b_cuda));
}

TEST_CASE("CUDA bridge executes vector and batched matmul routes", "[cuda][bridge][matmul][rank]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar matmul_off("PLUSS_CUDA_NATIVE_MATMUL", "0");
  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto vec_cpu = pluss::make_series(std::vector<float>{1.f, 2.f, 3.f}, {3});
  const auto mat_cpu = pluss::make_series(std::vector<float>{1.f, 4.f, 2.f, 5.f, 3.f, 6.f}, {3, 2});
  const auto mm_cpu = pluss::make_series(std::vector<float>{2.f, 1.f, 0.f, -1.f, 3.f, 2.f}, {2, 3});
  const auto vv_cpu = pluss::make_series(std::vector<float>{3.f, 1.f, 2.f}, {3});

  const auto batch_a_cpu = pluss::make_series(
      std::vector<float>{
          1.f, 2.f, 3.f, 4.f, 5.f, 6.f,
          7.f, 8.f, 9.f, 10.f, 11.f, 12.f},
      {2, 2, 3});
  const auto batch_b_cpu = pluss::make_series(
      std::vector<float>{
          1.f, 0.f, 2.f, 1.f, 3.f, 2.f,
          4.f, 1.f, 5.f, 2.f, 6.f, 3.f},
      {2, 3, 2});
  const auto broad_a_cpu = pluss::make_series(
      std::vector<float>{1.f, 2.f, 3.f, 4.f, 5.f, 6.f},
      {1, 2, 3});
  const auto broad_b_cpu = pluss::make_series(
      std::vector<float>{
          1.f, 2.f, 3.f, 4.f, 5.f, 6.f,
          7.f, 8.f, 9.f, 10.f, 11.f, 12.f},
      {2, 3, 2});

  const auto vec_cuda = vec_cpu.to(pluss::pCUDA);
  const auto mat_cuda = mat_cpu.to(pluss::pCUDA);
  const auto mm_cuda = mm_cpu.to(pluss::pCUDA);
  const auto vv_cuda = vv_cpu.to(pluss::pCUDA);
  const auto batch_a_cuda = batch_a_cpu.to(pluss::pCUDA);
  const auto batch_b_cuda = batch_b_cpu.to(pluss::pCUDA);
  const auto broad_a_cuda = broad_a_cpu.to(pluss::pCUDA);
  const auto broad_b_cuda = broad_b_cpu.to(pluss::pCUDA);

  const auto got_vm = vec_cuda.matmul(mat_cuda);
  REQUIRE(got_vm.platform() == pluss::pCUDA);
  require_close_(got_vm.to(pluss::pCPU).host<float>(), vec_cpu.matmul(mat_cpu).host<float>());

  const auto got_mv = mm_cuda.matmul(vv_cuda);
  REQUIRE(got_mv.platform() == pluss::pCUDA);
  require_close_(got_mv.to(pluss::pCPU).host<float>(), mm_cpu.matmul(vv_cpu).host<float>());

  const auto got_batched = batch_a_cuda.matmul(batch_b_cuda);
  REQUIRE(got_batched.platform() == pluss::pCUDA);
  require_close_(got_batched.to(pluss::pCPU).host<float>(), batch_a_cpu.matmul(batch_b_cpu).host<float>());

  const auto got_broadcasted = broad_a_cuda.matmul(broad_b_cuda);
  REQUIRE(got_broadcasted.platform() == pluss::pCUDA);
  require_close_(got_broadcasted.to(pluss::pCPU).host<float>(), broad_a_cpu.matmul(broad_b_cpu).host<float>());
}

TEST_CASE("CUDA strict matmul accepts rank-aware supported routes", "[cuda][bridge][policy][matmul][rank]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar matmul_on("PLUSS_CUDA_NATIVE_MATMUL", "1");
  const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_MATMUL_STRICT", "1");
  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto a_cpu = pluss::make_series(
      std::vector<float>{1.f, 2.f, 3.f, 4.f, 5.f, 6.f},
      {1, 2, 3});
  const auto b_cpu = pluss::make_series(
      std::vector<float>{
          1.f, 0.f, 2.f, 1.f, 3.f, 2.f,
          4.f, 1.f, 5.f, 2.f, 6.f, 3.f},
      {2, 3, 2});
  const auto a_cuda = a_cpu.to(pluss::pCUDA);
  const auto b_cuda = b_cpu.to(pluss::pCUDA);

  REQUIRE(pluss::cuda::supports_matmul_native(a_cuda, b_cuda));

  const auto got = a_cuda.matmul(b_cuda);
  REQUIRE(got.platform() == pluss::pCUDA);
  require_close_(got.to(pluss::pCPU).host<float>(), a_cpu.matmul(b_cpu).host<float>());
}

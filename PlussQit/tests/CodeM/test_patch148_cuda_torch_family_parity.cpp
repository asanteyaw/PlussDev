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

void require_close_(const std::vector<float>& got, const std::vector<float>& want, float tol = 1e-4f) {
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

void require_strict_fail_(const std::function<void()>& fn) {
  try {
    fn();
    FAIL("strict native CUDA route should fail until kernel is implemented");
  } catch (const std::exception& e) {
    REQUIRE(std::string(e.what()).find("strict native CUDA route requested") != std::string::npos);
  }
}

} // namespace

TEST_CASE("CUDA bridge parity covers torch-like core families", "[cuda][parity][bridge]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar selection_off("PLUSS_CUDA_NATIVE_SELECTION", "0");
  const ScopedEnvVar linalg_off("PLUSS_CUDA_NATIVE_LINALG", "0");
  const ScopedEnvVar indexing_off("PLUSS_CUDA_NATIVE_INDEXING", "0");
  const ScopedEnvVar matmul_off("PLUSS_CUDA_NATIVE_MATMUL", "0");
  const ScopedEnvVar scatter_off("PLUSS_CUDA_NATIVE_SCATTER_GATHER", "0");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto a_cpu = pluss::make_series(std::vector<float>{1.f, -2.f, 3.f, 0.5f, 4.f, -1.f}, {2, 3});
  const auto b_cpu = pluss::make_series(std::vector<float>{2.f, 0.f, -1.f, 3.f, 1.f, 4.f}, {3, 2});
  const auto a_cuda = a_cpu.to(pluss::pCUDA);
  const auto b_cuda = b_cpu.to(pluss::pCUDA);

  const auto expr_cpu = a_cpu.mul(1.25).add(0.4).square();
  const auto expr_cuda = a_cuda.mul(1.25).add(0.4).square();
  REQUIRE(expr_cuda.platform() == pluss::pCUDA);
  require_close_(expr_cuda.to(pluss::pCPU).to_vec<float>(), expr_cpu.to_vec<float>());

  const auto mm_cpu = a_cpu.matmul(b_cpu);
  const auto mm_cuda = a_cuda.matmul(b_cuda);
  REQUIRE(mm_cuda.platform() == pluss::pCUDA);
  require_close_(mm_cuda.to(pluss::pCPU).to_vec<float>(), mm_cpu.to_vec<float>());

  const auto sort_cpu = a_cpu.sort(/*dim=*/1, /*descending=*/false, /*stable=*/false);
  const auto sort_cuda = a_cuda.sort(/*dim=*/1, /*descending=*/false, /*stable=*/false);
  require_close_(sort_cuda.to(pluss::pCPU).to_vec<float>(), sort_cpu.to_vec<float>());

  const auto argsort_cpu = a_cpu.argsort(/*dim=*/1, /*descending=*/true, /*stable=*/false);
  const auto argsort_cuda = a_cuda.argsort(/*dim=*/1, /*descending=*/true, /*stable=*/false);
  require_equal_(argsort_cuda.to(pluss::pCPU).to_vec<std::int64_t>(), argsort_cpu.to_vec<std::int64_t>());

  const auto topk_cpu = a_cpu.topk(/*k=*/2, /*dim=*/1, /*largest=*/true, /*sorted=*/true);
  const auto topk_cuda = a_cuda.topk(/*k=*/2, /*dim=*/1, /*largest=*/true, /*sorted=*/true);
  require_close_(topk_cuda.to(pluss::pCPU).to_vec<float>(), topk_cpu.to_vec<float>());

  const auto flat_cpu = a_cpu.reshape({a_cpu.numel()});
  const auto flat_cuda = a_cuda.reshape({a_cuda.numel()});
  const auto idx_cpu = pluss::make_series(std::vector<std::int64_t>{5, 0, 3, 2}, {4});
  const auto idx_cuda = idx_cpu.to(pluss::pCUDA);
  require_close_(flat_cuda.take(idx_cuda).to(pluss::pCPU).to_vec<float>(), flat_cpu.take(idx_cpu).to_vec<float>());

  auto put_cpu = pluss::Series::zeros({flat_cpu.numel()}, pluss::ScalarType::Float32);
  auto put_cuda = put_cpu.to(pluss::pCUDA);
  const auto src_cpu = pluss::make_series(std::vector<float>{9.f, 8.f, 7.f, 6.f}, {4});
  const auto src_cuda = src_cpu.to(pluss::pCUDA);
  auto put_fn = static_cast<pluss::Series (pluss::Series::*)(const pluss::Series&, const pluss::Series&, bool)>(&pluss::Series::put_);
  put_cpu = (put_cpu.*put_fn)(idx_cpu, src_cpu, /*accumulate=*/false);
  put_cuda = (put_cuda.*put_fn)(idx_cuda, src_cuda, /*accumulate=*/false);
  require_close_(put_cuda.to(pluss::pCPU).to_vec<float>(), put_cpu.to_vec<float>());

  const auto nonzero_cpu = a_cpu.nonzero();
  const auto nonzero_cuda = a_cuda.nonzero();
  require_equal_(nonzero_cuda.to(pluss::pCPU).to_vec<std::int64_t>(), nonzero_cpu.to_vec<std::int64_t>());

  const auto gather_idx_cpu =
      pluss::make_series(std::vector<std::int64_t>{2, 0, 1, 1, 2, 0}, {2, 3});
  const auto gather_idx_cuda = gather_idx_cpu.to(pluss::pCUDA);
  require_close_(
      a_cuda.gather(1, gather_idx_cuda).to(pluss::pCPU).to_vec<float>(),
      a_cpu.gather(1, gather_idx_cpu).to_vec<float>());

  const auto scatter_base_cpu = pluss::Series::zeros({2, 3}, pluss::ScalarType::Float32);
  const auto scatter_base_cuda = scatter_base_cpu.to(pluss::pCUDA);
  require_close_(
      scatter_base_cuda.scatter(1, gather_idx_cuda, a_cuda).to(pluss::pCPU).to_vec<float>(),
      scatter_base_cpu.scatter(1, gather_idx_cpu, a_cpu).to_vec<float>());

  const auto linalg_cpu = pluss::make_series(std::vector<float>{4.f, 1.f, 1.f, 3.f}, {2, 2});
  const auto linalg_cuda = linalg_cpu.to(pluss::pCUDA);
  require_close_(
      pluss::subsp::linearalg::det(linalg_cuda).to(pluss::pCPU).to_vec<float>(),
      pluss::subsp::linearalg::det(linalg_cpu).to_vec<float>());
  require_close_(
      pluss::subsp::linearalg::inverse(linalg_cuda).to(pluss::pCPU).to_vec<float>(),
      pluss::subsp::linearalg::inverse(linalg_cpu).to_vec<float>(),
      2e-3f);
}

TEST_CASE("CUDA bridge parity stress loop stays stable", "[cuda][parity][bridge][stress]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar selection_off("PLUSS_CUDA_NATIVE_SELECTION", "0");
  const ScopedEnvVar matmul_off("PLUSS_CUDA_NATIVE_MATMUL", "0");
  const ScopedEnvVar indexing_off("PLUSS_CUDA_NATIVE_INDEXING", "0");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  std::mt19937 rng(1337u);
  std::uniform_real_distribution<float> dist(-2.0f, 2.0f);
  for (int rep = 0; rep < 24; ++rep) {
    std::vector<float> xdata(12);
    std::vector<float> ydata(8);
    for (auto& v : xdata) v = dist(rng);
    for (auto& v : ydata) v = dist(rng);

    const auto x_cpu = pluss::make_series(xdata, {3, 4});
    const auto y_cpu = pluss::make_series(ydata, {4, 2});
    const auto x_cuda = x_cpu.to(pluss::pCUDA);
    const auto y_cuda = y_cpu.to(pluss::pCUDA);

    require_close_(
        x_cuda.matmul(y_cuda).to(pluss::pCPU).to_vec<float>(),
        x_cpu.matmul(y_cpu).to_vec<float>(),
        2e-4f);

    require_close_(
        x_cuda.sort(/*dim=*/1, /*descending=*/false, /*stable=*/false).to(pluss::pCPU).to_vec<float>(),
        x_cpu.sort(/*dim=*/1, /*descending=*/false, /*stable=*/false).to_vec<float>(),
        2e-4f);

    const auto idx_cpu = pluss::make_series(std::vector<std::int64_t>{3, 1, 0, 2}, {4});
    const auto idx_cuda = idx_cpu.to(pluss::pCUDA);
    require_close_(
        x_cuda.reshape({x_cuda.numel()}).take(idx_cuda).to(pluss::pCPU).to_vec<float>(),
        x_cpu.reshape({x_cpu.numel()}).take(idx_cpu).to_vec<float>(),
        2e-4f);
  }
}

TEST_CASE("CUDA strict parity policy keeps fail-loud semantics", "[cuda][parity][policy]") {
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

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto x = pluss::make_series(std::vector<float>{2.f, -1.f, 3.f, 0.f}, {2, 2}).to(pluss::pCUDA);
  const auto y = pluss::make_series(std::vector<float>{1.f, 4.f, -2.f, 5.f}, {2, 2}).to(pluss::pCUDA);
  const auto idx = pluss::make_series(std::vector<std::int64_t>{1, 0}, {2}).to(pluss::pCUDA);
  const auto idx2 =
      pluss::make_series(std::vector<std::int64_t>{1, 0, 0, 1}, {2, 2}).to(pluss::pCUDA);

  require_strict_fail_([&] { (void)x.sort(/*dim=*/1, /*descending=*/false, /*stable=*/false); });
  require_strict_fail_([&] { (void)pluss::subsp::linearalg::det(x); });
  require_strict_fail_([&] { (void)x.take(idx); });
  require_strict_fail_([&] { (void)x.matmul(y); });
  require_strict_fail_([&] { (void)x.scatter(1, idx2, y); });
}

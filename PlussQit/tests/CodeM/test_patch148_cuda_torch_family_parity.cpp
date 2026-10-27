#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
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

} // namespace

TEST_CASE("CUDA bridge parity covers torch-like core families", "[cuda][parity][bridge]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar selection_off("PLUSS_CUDA_NATIVE_SELECTION", "0");
  const ScopedEnvVar linalg_off("PLUSS_CUDA_NATIVE_LINALG", "0");
  const ScopedEnvVar indexing_off("PLUSS_CUDA_NATIVE_INDEXING", "0");
  const ScopedEnvVar matmul_off("PLUSS_CUDA_NATIVE_MATMUL", "0");
  const ScopedEnvVar scatter_off("PLUSS_CUDA_NATIVE_SCATTER_GATHER", "0");
  const ScopedEnvVar reduction_off("PLUSS_CUDA_NATIVE_REDUCTION", "0");
  const ScopedEnvVar scan_off("PLUSS_CUDA_NATIVE_SCAN", "0");

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
  require_close_(expr_cuda.to(pluss::pCPU).host<float>(), expr_cpu.host<float>());

  const auto mm_cpu = a_cpu.matmul(b_cpu);
  const auto mm_cuda = a_cuda.matmul(b_cuda);
  REQUIRE(mm_cuda.platform() == pluss::pCUDA);
  require_close_(mm_cuda.to(pluss::pCPU).host<float>(), mm_cpu.host<float>());

  const auto sort_cpu = a_cpu.sort(/*dim=*/1, /*descending=*/false, /*stable=*/false);
  const auto sort_cuda = a_cuda.sort(/*dim=*/1, /*descending=*/false, /*stable=*/false);
  require_close_(sort_cuda.to(pluss::pCPU).host<float>(), sort_cpu.host<float>());

  require_close_(
      a_cuda.sum(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).host<float>(),
      a_cpu.sum(/*dim=*/1, /*keepdim=*/false).host<float>());
  require_close_(
      a_cuda.mean(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).host<float>(),
      a_cpu.mean(/*dim=*/1, /*keepdim=*/false).host<float>());
  require_close_(
      a_cuda.cumsum(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).host<float>(),
      a_cpu.cumsum(/*dim=*/1, /*keepdim=*/false).host<float>());
  require_close_(
      a_cuda.cumprod(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).host<float>(),
      a_cpu.cumprod(/*dim=*/1, /*keepdim=*/false).host<float>());
  require_close_(
      a_cuda.amax(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).host<float>(),
      a_cpu.amax(/*dim=*/1, /*keepdim=*/false).host<float>());
  require_equal_(
      a_cuda.argmax(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).host<std::int64_t>(),
      a_cpu.argmax(/*dim=*/1, /*keepdim=*/false).host<std::int64_t>());

  const auto argsort_cpu = a_cpu.argsort(/*dim=*/1, /*descending=*/true, /*stable=*/false);
  const auto argsort_cuda = a_cuda.argsort(/*dim=*/1, /*descending=*/true, /*stable=*/false);
  require_equal_(argsort_cuda.to(pluss::pCPU).host<std::int64_t>(), argsort_cpu.host<std::int64_t>());

  const auto topk_cpu = a_cpu.topk(/*k=*/2, /*dim=*/1, /*largest=*/true, /*sorted=*/true);
  const auto topk_cuda = a_cuda.topk(/*k=*/2, /*dim=*/1, /*largest=*/true, /*sorted=*/true);
  require_close_(topk_cuda.to(pluss::pCPU).host<float>(), topk_cpu.host<float>());
  require_close_(a_cuda.relu().to(pluss::pCPU).host<float>(), a_cpu.relu().host<float>());
  require_close_(a_cuda.softmax(1).to(pluss::pCPU).host<float>(), a_cpu.softmax(1).host<float>());

  const auto flat_cpu = a_cpu.reshape({a_cpu.numel()});
  const auto flat_cuda = a_cuda.reshape({a_cuda.numel()});
  const auto idx_cpu = pluss::make_series(std::vector<std::int64_t>{5, 0, 3, 2}, {4});
  const auto idx_cuda = idx_cpu.to(pluss::pCUDA);
  require_close_(flat_cuda.take(idx_cuda).to(pluss::pCPU).host<float>(), flat_cpu.take(idx_cpu).host<float>());

  auto put_cpu = pluss::Series::zeros({flat_cpu.numel()}, pluss::ScalarType::Float32);
  auto put_cuda = put_cpu.to(pluss::pCUDA);
  const auto src_cpu = pluss::make_series(std::vector<float>{9.f, 8.f, 7.f, 6.f}, {4});
  const auto src_cuda = src_cpu.to(pluss::pCUDA);
  auto put_fn = static_cast<pluss::Series (pluss::Series::*)(const pluss::Series&, const pluss::Series&, bool)>(&pluss::Series::put_raw_);
  put_cpu = (put_cpu.*put_fn)(idx_cpu, src_cpu, /*accumulate=*/false);
  put_cuda = (put_cuda.*put_fn)(idx_cuda, src_cuda, /*accumulate=*/false);
  require_close_(put_cuda.to(pluss::pCPU).host<float>(), put_cpu.host<float>());

  const auto nonzero_cpu = a_cpu.nonzero();
  const auto nonzero_cuda = a_cuda.nonzero();
  require_equal_(nonzero_cuda.to(pluss::pCPU).host<std::int64_t>(), nonzero_cpu.host<std::int64_t>());

  const auto gather_idx_cpu =
      pluss::make_series(std::vector<std::int64_t>{2, 0, 1, 1, 2, 0}, {2, 3});
  const auto gather_idx_cuda = gather_idx_cpu.to(pluss::pCUDA);
  require_close_(
      a_cuda.gather(1, gather_idx_cuda).to(pluss::pCPU).host<float>(),
      a_cpu.gather(1, gather_idx_cpu).host<float>());

  const auto scatter_base_cpu = pluss::Series::zeros({2, 3}, pluss::ScalarType::Float32);
  const auto scatter_base_cuda = scatter_base_cpu.to(pluss::pCUDA);
  require_close_(
      scatter_base_cuda.scatter(1, gather_idx_cuda, a_cuda).to(pluss::pCPU).host<float>(),
      scatter_base_cpu.scatter(1, gather_idx_cpu, a_cpu).host<float>());

  const auto linalg_cpu = pluss::make_series(std::vector<float>{4.f, 1.f, 1.f, 3.f}, {2, 2});
  const auto linalg_cuda = linalg_cpu.to(pluss::pCUDA);
  require_close_(
      pluss::subsp::linearalg::det(linalg_cuda).to(pluss::pCPU).host<float>(),
      pluss::subsp::linearalg::det(linalg_cpu).host<float>());
  require_close_(
      pluss::subsp::linearalg::inverse(linalg_cuda).to(pluss::pCPU).host<float>(),
      pluss::subsp::linearalg::inverse(linalg_cpu).host<float>(),
      2e-3f);
}

TEST_CASE("CUDA bridge parity stress loop stays stable", "[cuda][parity][bridge][stress]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar selection_off("PLUSS_CUDA_NATIVE_SELECTION", "0");
  const ScopedEnvVar matmul_off("PLUSS_CUDA_NATIVE_MATMUL", "0");
  const ScopedEnvVar indexing_off("PLUSS_CUDA_NATIVE_INDEXING", "0");
  const ScopedEnvVar reduction_off("PLUSS_CUDA_NATIVE_REDUCTION", "0");
  const ScopedEnvVar scan_off("PLUSS_CUDA_NATIVE_SCAN", "0");

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
        x_cuda.matmul(y_cuda).to(pluss::pCPU).host<float>(),
        x_cpu.matmul(y_cpu).host<float>(),
        2e-4f);

    require_close_(
        x_cuda.sort(/*dim=*/1, /*descending=*/false, /*stable=*/false).to(pluss::pCPU).host<float>(),
        x_cpu.sort(/*dim=*/1, /*descending=*/false, /*stable=*/false).host<float>(),
        2e-4f);

    require_close_(
        x_cuda.sum(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).host<float>(),
        x_cpu.sum(/*dim=*/1, /*keepdim=*/false).host<float>(),
        2e-4f);
    require_close_(
        x_cuda.cumsum(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).host<float>(),
        x_cpu.cumsum(/*dim=*/1, /*keepdim=*/false).host<float>(),
        2e-4f);

    const auto idx_cpu = pluss::make_series(std::vector<std::int64_t>{3, 1, 0, 2}, {4});
    const auto idx_cuda = idx_cpu.to(pluss::pCUDA);
    require_close_(
        x_cuda.reshape({x_cuda.numel()}).take(idx_cuda).to(pluss::pCPU).host<float>(),
        x_cpu.reshape({x_cpu.numel()}).take(idx_cpu).host<float>(),
        2e-4f);
  }
}

TEST_CASE("CUDA autodiff parity tracks CPU value and sensitivity baselines", "[cuda][parity][autodiff]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar selection_off("PLUSS_CUDA_NATIVE_SELECTION", "0");
  const ScopedEnvVar indexing_off("PLUSS_CUDA_NATIVE_INDEXING", "0");
  const ScopedEnvVar reduction_off("PLUSS_CUDA_NATIVE_REDUCTION", "0");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  pluss::Series x_cpu =
      pluss::make_series(std::vector<float>{0.5f, -1.25f, 2.0f, 0.1f, -0.4f, 1.7f}, {2, 3});
  pluss::Series x_cuda = x_cpu.to(pluss::pCUDA);
  x_cpu.tracks_slope_(true);
  x_cuda.tracks_slope_(true);

  const pluss::Series idx_cpu = pluss::make_series(std::vector<std::int64_t>{5, 0, 3, 2}, {4});
  const pluss::Series idx_cuda = idx_cpu.to(pluss::pCUDA);

  const pluss::Series flat_cpu = x_cpu.reshape({x_cpu.numel()});
  const pluss::Series flat_cuda = x_cuda.reshape({x_cuda.numel()});
  const pluss::Series loss_cpu = flat_cpu.take(idx_cpu).square().sum().add(x_cpu.sin().mean());
  const pluss::Series loss_cuda = flat_cuda.take(idx_cuda).square().sum().add(x_cuda.sin().mean());

  loss_cpu.reverse();
  loss_cuda.reverse();

  REQUIRE(std::fabs(loss_cuda.to(pluss::pCPU).item<float>() - loss_cpu.item<float>()) <= 2e-4f);
  require_close_(
      x_cuda.sensitivity().to(pluss::pCPU).host<float>(),
      x_cpu.sensitivity().host<float>(),
      3e-4f);
}

TEST_CASE("CUDA autodiff one-shot reverse fails loudly after graph release", "[cuda][parity][autodiff]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  pluss::Series x =
      pluss::make_series(std::vector<float>{1.f, -2.f, 0.5f, 3.f}, {2, 2}).to(pluss::pCUDA);
  x.tracks_slope_(true);
  const pluss::Series loss = x.square().sum();

  loss.reverse(/*keep_graph=*/false);
  x.clear();
  REQUIRE_THROWS_WITH(
      loss.reverse(),
      Catch::Matchers::ContainsSubstring("one-shot reverse pass"));
}

TEST_CASE("CUDA strict parity policy executes supported native routes", "[cuda][parity][policy]") {
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
  const ScopedEnvVar scan_on("PLUSS_CUDA_NATIVE_SCAN", "1");
  const ScopedEnvVar scan_strict("PLUSS_CUDA_NATIVE_SCAN_STRICT", "1");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto x_cpu = pluss::make_series(std::vector<float>{2.f, -1.f, 3.f, 0.f}, {2, 2});
  const auto y_cpu = pluss::make_series(std::vector<float>{1.f, 4.f, -2.f, 5.f}, {2, 2});
  const auto idx_cpu = pluss::make_series(std::vector<std::int64_t>{1, 0}, {2});
  const auto idx2_cpu =
      pluss::make_series(std::vector<std::int64_t>{1, 0, 0, 1}, {2, 2});
  const auto x = x_cpu.to(pluss::pCUDA);
  const auto y = y_cpu.to(pluss::pCUDA);
  const auto idx = idx_cpu.to(pluss::pCUDA);
  const auto idx2 = idx2_cpu.to(pluss::pCUDA);

  require_close_(
      x.sort(/*dim=*/1, /*descending=*/false, /*stable=*/false).to(pluss::pCPU).host<float>(),
      x_cpu.sort(/*dim=*/1, /*descending=*/false, /*stable=*/false).host<float>());
  require_close_(
      pluss::subsp::linearalg::det(x).to(pluss::pCPU).host<float>(),
      pluss::subsp::linearalg::det(x_cpu).host<float>(),
      2e-3f);
  require_close_(x.take(idx).to(pluss::pCPU).host<float>(), x_cpu.take(idx_cpu).host<float>());
  require_close_(x.matmul(y).to(pluss::pCPU).host<float>(), x_cpu.matmul(y_cpu).host<float>());
  require_close_(
      x.scatter(1, idx2, y).to(pluss::pCPU).host<float>(),
      x_cpu.scatter(1, idx2_cpu, y_cpu).host<float>());
  require_close_(
      x.sum(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).host<float>(),
      x_cpu.sum(/*dim=*/1, /*keepdim=*/false).host<float>());
  require_close_(
      x.mean(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).host<float>(),
      x_cpu.mean(/*dim=*/1, /*keepdim=*/false).host<float>());
  require_close_(
      x.cumsum(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).host<float>(),
      x_cpu.cumsum(/*dim=*/1, /*keepdim=*/false).host<float>());
  require_equal_(
      x.argmax(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).host<std::int64_t>(),
      x_cpu.argmax(/*dim=*/1, /*keepdim=*/false).host<std::int64_t>());
  require_close_(x.relu().to(pluss::pCPU).host<float>(), x_cpu.relu().host<float>());
  require_close_(x.softmax(1).to(pluss::pCPU).host<float>(), x_cpu.softmax(1).host<float>());
}

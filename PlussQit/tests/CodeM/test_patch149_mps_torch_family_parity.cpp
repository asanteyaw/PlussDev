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

void require_close_(
    const std::vector<float>& got,
    const std::vector<float>& want,
    const char* label,
    float tol = 1e-4f) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    INFO(label << " i=" << i << " got=" << got[i] << " want=" << want[i] << " tol=" << tol);
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

TEST_CASE("MPS parity covers torch-like core families", "[mps][parity][torch]") {
  const ScopedEnvVar fallback_on("PLUSS_MPS_ALLOW_CPU_FALLBACK", "1");
  const ScopedEnvVar native_take_put("PLUSS_MPS_NATIVE_TAKE_PUT", "1");
  const ScopedEnvVar native_sort("PLUSS_MPS_NATIVE_SORT_SELECTION", "1");

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  const auto a_cpu = pluss::make_series(std::vector<float>{1.f, -2.f, 3.f, 0.5f, 4.f, -1.f}, {2, 3});
  const auto b_cpu = pluss::make_series(std::vector<float>{2.f, 0.f, -1.f, 3.f, 1.f, 4.f}, {3, 2});
  const auto a_mps = a_cpu.to(pluss::pMPS);
  const auto b_mps = b_cpu.to(pluss::pMPS);

  const auto expr_cpu = a_cpu.mul(1.25).add(0.4).square();
  const auto expr_mps = a_mps.mul(1.25).add(0.4).square();
  REQUIRE(expr_mps.platform() == pluss::pMPS);
  require_close_(expr_mps.to(pluss::pCPU).to_vec<float>(), expr_cpu.to_vec<float>(), "expr");

  require_close_(
      a_mps.matmul(b_mps).to(pluss::pCPU).to_vec<float>(),
      a_cpu.matmul(b_cpu).to_vec<float>(),
      "matmul");

  require_close_(
      a_mps.sort(/*dim=*/1, /*descending=*/false, /*stable=*/false).to(pluss::pCPU).to_vec<float>(),
      a_cpu.sort(/*dim=*/1, /*descending=*/false, /*stable=*/false).to_vec<float>(),
      "sort");

  require_equal_(
      a_mps.argsort(/*dim=*/1, /*descending=*/true, /*stable=*/false).to(pluss::pCPU).to_vec<std::int64_t>(),
      a_cpu.argsort(/*dim=*/1, /*descending=*/true, /*stable=*/false).to_vec<std::int64_t>());

  require_close_(
      a_mps.topk(/*k=*/2, /*dim=*/1, /*largest=*/true, /*sorted=*/true).to(pluss::pCPU).to_vec<float>(),
      a_cpu.topk(/*k=*/2, /*dim=*/1, /*largest=*/true, /*sorted=*/true).to_vec<float>(),
      "topk");

  const auto flat_cpu = a_cpu.reshape({a_cpu.numel()});
  const auto flat_mps = a_mps.reshape({a_mps.numel()});
  const auto idx_cpu = pluss::make_series(std::vector<std::int64_t>{5, 0, 3, 2}, {4});
  const auto idx_mps = idx_cpu.to(pluss::pMPS);
  require_close_(
      flat_mps.take(idx_mps).to(pluss::pCPU).to_vec<float>(),
      flat_cpu.take(idx_cpu).to_vec<float>(),
      "take");

  auto put_cpu = pluss::Series::zeros({flat_cpu.numel()}, pluss::ScalarType::Float32);
  auto put_mps = put_cpu.to(pluss::pMPS);
  const auto src_cpu = pluss::make_series(std::vector<float>{9.f, 8.f, 7.f, 6.f}, {4});
  const auto src_mps = src_cpu.to(pluss::pMPS);
  auto put_fn = static_cast<pluss::Series (pluss::Series::*)(const pluss::Series&, const pluss::Series&, bool)>(&pluss::Series::put_);
  put_cpu = (put_cpu.*put_fn)(idx_cpu, src_cpu, /*accumulate=*/false);
  put_mps = (put_mps.*put_fn)(idx_mps, src_mps, /*accumulate=*/false);
  require_close_(put_mps.to(pluss::pCPU).to_vec<float>(), put_cpu.to_vec<float>(), "put");

  require_equal_(
      a_mps.nonzero().to(pluss::pCPU).to_vec<std::int64_t>(),
      a_cpu.nonzero().to_vec<std::int64_t>());

  const auto gather_idx_cpu =
      pluss::make_series(std::vector<std::int64_t>{2, 0, 1, 1, 2, 0}, {2, 3});
  const auto gather_idx_mps = gather_idx_cpu.to(pluss::pMPS);
  require_close_(
      a_mps.gather(1, gather_idx_mps).to(pluss::pCPU).to_vec<float>(),
      a_cpu.gather(1, gather_idx_cpu).to_vec<float>(),
      "gather");

  const auto scatter_base_cpu = pluss::Series::zeros({2, 3}, pluss::ScalarType::Float32);
  const auto scatter_base_mps = scatter_base_cpu.to(pluss::pMPS);
  require_close_(
      scatter_base_mps.scatter(1, gather_idx_mps, a_mps).to(pluss::pCPU).to_vec<float>(),
      scatter_base_cpu.scatter(1, gather_idx_cpu, a_cpu).to_vec<float>(),
      "scatter");

  const auto linalg_cpu = pluss::make_series(std::vector<float>{4.f, 1.f, 1.f, 3.f}, {2, 2});
  const auto linalg_mps = linalg_cpu.to(pluss::pMPS);
  require_close_(
      pluss::subsp::linearalg::det(linalg_mps).to(pluss::pCPU).to_vec<float>(),
      pluss::subsp::linearalg::det(linalg_cpu).to_vec<float>(),
      "det");
  require_close_(
      pluss::subsp::linearalg::inverse(linalg_mps).to(pluss::pCPU).to_vec<float>(),
      pluss::subsp::linearalg::inverse(linalg_cpu).to_vec<float>(),
      "inverse",
      2e-3f);
}

TEST_CASE("MPS parity stress loop stays stable", "[mps][parity][torch][stress]") {
  const ScopedEnvVar fallback_on("PLUSS_MPS_ALLOW_CPU_FALLBACK", "1");
  const ScopedEnvVar native_sort("PLUSS_MPS_NATIVE_SORT_SELECTION", "1");

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
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
    const auto x_mps = x_cpu.to(pluss::pMPS);
    const auto y_mps = y_cpu.to(pluss::pMPS);

    require_close_(
        x_mps.matmul(y_mps).to(pluss::pCPU).to_vec<float>(),
        x_cpu.matmul(y_cpu).to_vec<float>(),
        "stress-matmul",
        2e-4f);
    require_close_(
        x_mps.sort(/*dim=*/1, /*descending=*/false, /*stable=*/false).to(pluss::pCPU).to_vec<float>(),
        x_cpu.sort(/*dim=*/1, /*descending=*/false, /*stable=*/false).to_vec<float>(),
        "stress-sort",
        2e-4f);
  }
}

TEST_CASE("MPS autodiff parity tracks CPU value and sensitivity baselines", "[mps][parity][autodiff]") {
  const ScopedEnvVar fallback_on("PLUSS_MPS_ALLOW_CPU_FALLBACK", "1");
  const ScopedEnvVar native_take_put("PLUSS_MPS_NATIVE_TAKE_PUT", "1");
  const ScopedEnvVar native_sort("PLUSS_MPS_NATIVE_SORT_SELECTION", "1");

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  pluss::Series x_cpu =
      pluss::make_series(std::vector<float>{0.5f, -1.25f, 2.0f, 0.1f, -0.4f, 1.7f}, {2, 3});
  pluss::Series x_mps = x_cpu.to(pluss::pMPS);
  x_cpu.tracks_slope_(true);
  x_mps.tracks_slope_(true);

  const pluss::Series idx_cpu = pluss::make_series(std::vector<std::int64_t>{5, 0, 3, 2}, {4});
  const pluss::Series idx_mps = idx_cpu.to(pluss::pMPS);

  const pluss::Series flat_cpu = x_cpu.reshape({x_cpu.numel()});
  const pluss::Series flat_mps = x_mps.reshape({x_mps.numel()});
  const pluss::Series loss_cpu = flat_cpu.take(idx_cpu).square().sum().add(x_cpu.sin().mean());
  const pluss::Series loss_mps = flat_mps.take(idx_mps).square().sum().add(x_mps.sin().mean());

  loss_cpu.reverse();
  loss_mps.reverse();

  REQUIRE(std::fabs(loss_mps.to(pluss::pCPU).item<float>() - loss_cpu.item<float>()) <= 2e-4f);
  require_close_(
      x_mps.sensitivity().to(pluss::pCPU).to_vec<float>(),
      x_cpu.sensitivity().to_vec<float>(),
      "autodiff-sensitivity",
      3e-4f);
}

TEST_CASE("MPS autodiff one-shot reverse fails loudly after graph release", "[mps][parity][autodiff]") {
  const ScopedEnvVar fallback_on("PLUSS_MPS_ALLOW_CPU_FALLBACK", "1");
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  pluss::Series x =
      pluss::make_series(std::vector<float>{1.f, -2.f, 0.5f, 3.f}, {2, 2}).to(pluss::pMPS);
  x.tracks_slope_(true);
  const pluss::Series loss = x.square().sum();

  loss.reverse(/*keep_graph=*/false);
  x.clear_sensitivity();
  REQUIRE_THROWS_WITH(
      loss.reverse(),
      Catch::Matchers::ContainsSubstring("one-shot reverse pass"));
}

TEST_CASE("MPS strict parity lane works without CPU fallback", "[mps][parity][torch][strict]") {
  const ScopedEnvVar fallback_off("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const ScopedEnvVar native_take_put("PLUSS_MPS_NATIVE_TAKE_PUT", "1");
  const ScopedEnvVar native_sort("PLUSS_MPS_NATIVE_SORT_SELECTION", "1");
  const ScopedEnvVar native_sort_strict("PLUSS_MPS_NATIVE_SORT_SELECTION_STRICT", "1");
  const ScopedEnvVar native_topk("PLUSS_MPS_NATIVE_TOPK_SELECTION", "1");
  const ScopedEnvVar linalg_native("PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE", "1");
  const ScopedEnvVar linalg_no_bridge("PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE", "0");
  const ScopedEnvVar linalg_route("PLUSS_MPS_LINALG_SOLVER_ROUTE", "native");
  const ScopedEnvVar linalg_svd("PLUSS_MPS_LINALG_NATIVE_DEVICE_SVD", "1");

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  const auto x_cpu = pluss::make_series(std::vector<float>{3.f, 1.f, 4.f, 2.f, 5.f, -1.f}, {2, 3});
  const auto idx_cpu = pluss::make_series(std::vector<std::int64_t>{5, 2, 0, 3}, {4});
  const auto x_mps = x_cpu.to(pluss::pMPS);
  const auto idx_mps = idx_cpu.to(pluss::pMPS);

  require_close_(
      x_mps.sort(/*dim=*/1, /*descending=*/false, /*stable=*/false).to(pluss::pCPU).to_vec<float>(),
      x_cpu.sort(/*dim=*/1, /*descending=*/false, /*stable=*/false).to_vec<float>(),
      "strict-sort");
  require_equal_(
      x_mps.argsort(/*dim=*/1, /*descending=*/true, /*stable=*/false).to(pluss::pCPU).to_vec<std::int64_t>(),
      x_cpu.argsort(/*dim=*/1, /*descending=*/true, /*stable=*/false).to_vec<std::int64_t>());
  require_close_(
      x_mps.topk(/*k=*/2, /*dim=*/1, /*largest=*/true, /*sorted=*/true).to(pluss::pCPU).to_vec<float>(),
      x_cpu.topk(/*k=*/2, /*dim=*/1, /*largest=*/true, /*sorted=*/true).to_vec<float>(),
      "strict-topk");
  require_close_(
      x_mps.kthvalue(/*k=*/2, /*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<float>(),
      x_cpu.kthvalue(/*k=*/2, /*dim=*/1, /*keepdim=*/false).to_vec<float>(),
      "strict-kthvalue");
  require_close_(
      x_mps.reshape({x_mps.numel()}).take(idx_mps).to(pluss::pCPU).to_vec<float>(),
      x_cpu.reshape({x_cpu.numel()}).take(idx_cpu).to_vec<float>(),
      "strict-take");

  using namespace pluss::subsp::linearalg;
  linearalg_reset_dispatch_counters();
  const auto l_cpu = pluss::make_series(std::vector<float>{5.f, 1.f, 1.f, 4.f}, {2, 2});
  const auto l_mps = l_cpu.to(pluss::pMPS);
  require_close_(
      det(l_mps).to(pluss::pCPU).to_vec<float>(),
      det(l_cpu).to_vec<float>(),
      "strict-det");
  require_close_(
      inverse(l_mps).to(pluss::pCPU).to_vec<float>(),
      inverse(l_cpu).to_vec<float>(),
      "strict-inverse",
      2e-3f);
  const auto counters = linearalg_dispatch_counters();
  REQUIRE(counters.mps_bridge_total == 0);
}

#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

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

TEST_CASE("MPS strict native selection supports non-last dim lane", "[mps][parity][torch][strict][selection][shape]") {
  const ScopedEnvVar fallback_off("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const ScopedEnvVar native_sort("PLUSS_MPS_NATIVE_SORT_SELECTION", "1");
  const ScopedEnvVar native_sort_strict("PLUSS_MPS_NATIVE_SORT_SELECTION_STRICT", "1");
  const ScopedEnvVar native_topk("PLUSS_MPS_NATIVE_TOPK_SELECTION", "1");

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  const auto x_cpu = pluss::make_series(
      std::vector<float>{5.f, -2.f, 1.f, 4.f, 7.f, -1.f, 0.f, 3.f, 9.f, 6.f, -3.f, 2.f},
      {4, 3});
  const auto x_mps = x_cpu.to(pluss::pMPS);

  require_close_(
      x_mps.sort(/*dim=*/0, /*descending=*/false, /*stable=*/false).to(pluss::pCPU).host<float>().array().data,
      x_cpu.sort(/*dim=*/0, /*descending=*/false, /*stable=*/false).host<float>().array().data);
  require_equal_(
      x_mps.argsort(/*dim=*/0, /*descending=*/true, /*stable=*/false).to(pluss::pCPU).host<std::int64_t>().array().data,
      x_cpu.argsort(/*dim=*/0, /*descending=*/true, /*stable=*/false).host<std::int64_t>().array().data);
  require_close_(
      x_mps.topk(/*k=*/2, /*dim=*/0, /*largest=*/true, /*sorted=*/true).to(pluss::pCPU).host<float>().array().data,
      x_cpu.topk(/*k=*/2, /*dim=*/0, /*largest=*/true, /*sorted=*/true).host<float>().array().data);
  require_close_(
      x_mps.kthvalue(/*k=*/2, /*dim=*/0, /*keepdim=*/true).to(pluss::pCPU).host<float>().array().data,
      x_cpu.kthvalue(/*k=*/2, /*dim=*/0, /*keepdim=*/true).host<float>().array().data);
}

TEST_CASE("MPS strict native selection rejects cast-assisted dtype lanes", "[mps][parity][torch][strict][selection][policy]") {
  const ScopedEnvVar fallback_off("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const ScopedEnvVar native_sort("PLUSS_MPS_NATIVE_SORT_SELECTION", "1");
  const ScopedEnvVar native_sort_strict("PLUSS_MPS_NATIVE_SORT_SELECTION_STRICT", "1");

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  const auto x_i16 =
      pluss::make_series(std::vector<std::int16_t>{3, -1, 2, 7, 0, 5}, {2, 3}).to(pluss::pMPS);
  try {
    (void)x_i16.sort(/*dim=*/1, /*descending=*/false, /*stable=*/false);
    FAIL("Expected strict native selection to reject cast-assisted dtype lane");
  } catch (const std::exception& e) {
    REQUIRE(std::string(e.what()).find("cast-assisted") != std::string::npos);
  }
}

TEST_CASE("MPS strict native selection soak stays parity-stable", "[mps][parity][torch][strict][selection][soak][perf]") {
  const ScopedEnvVar fallback_off("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const ScopedEnvVar native_sort("PLUSS_MPS_NATIVE_SORT_SELECTION", "1");
  const ScopedEnvVar native_sort_strict("PLUSS_MPS_NATIVE_SORT_SELECTION_STRICT", "1");
  const ScopedEnvVar native_topk("PLUSS_MPS_NATIVE_TOPK_SELECTION", "1");

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  std::mt19937 rng(20260311u);
  std::uniform_real_distribution<float> dist(-8.0f, 8.0f);

  for (int rep = 0; rep < 80; ++rep) {
    std::vector<float> xdata(48);
    for (auto& v : xdata) {
      v = dist(rng);
    }
    const auto x_cpu = pluss::make_series(xdata, {8, 6});
    const auto x_mps = x_cpu.to(pluss::pMPS);

    const std::int64_t dim = (rep % 2 == 0) ? 0 : 1;
    const std::int64_t k = (rep % 4) + 1;
    const bool descending = ((rep % 3) == 0);
    const bool largest = ((rep % 2) == 1);
    const bool keepdim = ((rep % 5) == 0);

    require_close_(
        x_mps.sort(dim, descending, /*stable=*/false).to(pluss::pCPU).host<float>().array().data,
        x_cpu.sort(dim, descending, /*stable=*/false).host<float>().array().data,
        2e-4f);
    require_equal_(
        x_mps.argsort(dim, descending, /*stable=*/false).to(pluss::pCPU).host<std::int64_t>().array().data,
        x_cpu.argsort(dim, descending, /*stable=*/false).host<std::int64_t>().array().data);
    require_close_(
        x_mps.topk(k, dim, largest, /*sorted=*/true).to(pluss::pCPU).host<float>().array().data,
        x_cpu.topk(k, dim, largest, /*sorted=*/true).host<float>().array().data,
        2e-4f);
    require_close_(
        x_mps.kthvalue(k, dim, keepdim).to(pluss::pCPU).host<float>().array().data,
        x_cpu.kthvalue(k, dim, keepdim).host<float>().array().data,
        2e-4f);
  }
}

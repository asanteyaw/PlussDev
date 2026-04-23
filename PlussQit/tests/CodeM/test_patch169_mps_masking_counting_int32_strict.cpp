#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cstdint>
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

template <class T>
void require_equal_(const std::vector<T>& got, const std::vector<T>& want) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(got[i] == want[i]);
  }
}

} // namespace

TEST_CASE("MPS strict int32 masking ops stay native and match CPU", "[mps][strict][int32][masking]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }
  const ScopedEnvVar fallback_off("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");

  const auto x_cpu = pluss::make_series(
      std::vector<std::int32_t>{1, 2, 3, 4, 5, 6},
      {2, 3});
  const auto mask_cpu = pluss::make_series(
      std::vector<bool>{true, false, true, false, true, false},
      {2, 3});

  const auto x_mps = x_cpu.to(pluss::pMPS);
  const auto mask_mps = mask_cpu.to(pluss::pMPS);

  require_equal_(
      x_mps.masked_select(mask_mps).to(pluss::pCPU).to_vec<std::int32_t>(),
      x_cpu.masked_select(mask_cpu).to_vec<std::int32_t>());

  require_equal_(
      x_mps.masked_fill(mask_mps, -9.0).to(pluss::pCPU).to_vec<std::int32_t>(),
      x_cpu.masked_fill(mask_cpu, -9.0).to_vec<std::int32_t>());

  auto xf_cpu = x_cpu.clone();
  auto xf_mps = x_mps.clone();
  xf_cpu.masked_fill_(mask_cpu, 42.0);
  xf_mps.masked_fill_(mask_mps, 42.0);
  require_equal_(
      xf_mps.to(pluss::pCPU).to_vec<std::int32_t>(),
      xf_cpu.to_vec<std::int32_t>());

  const auto src_cpu = pluss::make_series(
      std::vector<std::int32_t>{11, 22, 33},
      {3});
  require_equal_(
      x_mps.masked_scatter(mask_mps, src_cpu.to(pluss::pMPS)).to(pluss::pCPU).to_vec<std::int32_t>(),
      x_cpu.masked_scatter(mask_cpu, src_cpu).to_vec<std::int32_t>());

  auto xs_cpu = x_cpu.clone();
  auto xs_mps = x_mps.clone();
  xs_cpu.masked_scatter_(mask_cpu, src_cpu);
  xs_mps.masked_scatter_(mask_mps, src_cpu.to(pluss::pMPS));
  require_equal_(
      xs_mps.to(pluss::pCPU).to_vec<std::int32_t>(),
      xs_cpu.to_vec<std::int32_t>());
}

TEST_CASE("MPS strict int32 bincount/unique parity matches CPU", "[mps][strict][int32][counting][parity]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }
  const ScopedEnvVar fallback_off("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");

  const auto x_cpu = pluss::make_series(
      std::vector<std::int32_t>{0, 1, 1, 3, 2, 3, 3, 5, 0},
      {9});
  const auto x_mps = x_cpu.to(pluss::pMPS);

  require_equal_(
      x_mps.bincount(/*minlength=*/7).to(pluss::pCPU).to_vec<std::int64_t>(),
      x_cpu.bincount(/*minlength=*/7).to_vec<std::int64_t>());

  require_equal_(
      x_mps.unique(/*sorted=*/true).to(pluss::pCPU).to_vec<std::int64_t>(),
      x_cpu.unique(/*sorted=*/true).to_vec<std::int64_t>());
}

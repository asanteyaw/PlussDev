#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cmath>
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

void require_close_(const std::vector<float>& got, const std::vector<float>& want, float tol = 1e-4f) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(std::fabs(got[i] - want[i]) <= tol);
  }
}

template <typename T>
void require_equal_(const std::vector<T>& got, const std::vector<T>& want) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(got[i] == want[i]);
  }
}

} // namespace

TEST_CASE("MPS native take survives strict no-fallback policy", "[mps][take][native][strict]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }
  const ScopedEnvVar no_fallback("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const ScopedEnvVar native_take_put("PLUSS_MPS_NATIVE_TAKE_PUT", "1");

  const auto x_cpu = pluss::make_series(std::vector<float>{3.f, 1.f, 4.f, 2.f, 5.f}, {5});
  const auto idx_cpu = pluss::make_series(std::vector<std::int64_t>{4, 2, 0, 3}, {2, 2});

  const auto x_mps = x_cpu.to(pluss::pMPS);
  const auto idx_mps = idx_cpu.to(pluss::pMPS);
  const auto got = x_mps.take(idx_mps);
  REQUIRE(got.platform() == pluss::pMPS);
  require_close_(got.to(pluss::pCPU).host<float>().array().data, x_cpu.take(idx_cpu).host<float>().array().data);
}

TEST_CASE("MPS native put survives strict no-fallback policy", "[mps][put][native][strict]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }
  const ScopedEnvVar no_fallback("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const ScopedEnvVar native_take_put("PLUSS_MPS_NATIVE_TAKE_PUT", "1");

  auto base_cpu = pluss::Series::zeros({6}, pluss::ScalarType::Float32);
  const auto idx_cpu = pluss::make_series(std::vector<std::int64_t>{5, 1, 4, 0}, {4});
  const auto src_cpu = pluss::make_series(std::vector<float>{9.f, 8.f, 7.f, 6.f}, {4});
  auto base_mps = base_cpu.to(pluss::pMPS);
  auto put_fn = static_cast<pluss::Series (pluss::Series::*)(const pluss::Series&, const pluss::Series&, bool)>(&pluss::Series::put_raw_);

  base_cpu = (base_cpu.*put_fn)(idx_cpu, src_cpu, /*accumulate=*/false);
  base_mps = (base_mps.*put_fn)(idx_cpu.to(pluss::pMPS), src_cpu.to(pluss::pMPS), /*accumulate=*/false);
  REQUIRE(base_mps.platform() == pluss::pMPS);
  require_close_(base_mps.to(pluss::pCPU).host<float>(), base_cpu.host<float>());
}

TEST_CASE("MPS native take supports float16 work lane under strict policy", "[mps][take][native][strict][fp16]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }
  const ScopedEnvVar no_fallback("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const ScopedEnvVar native_take_put("PLUSS_MPS_NATIVE_TAKE_PUT", "1");

  const auto x_cpu_f32 = pluss::make_series(std::vector<float>{0.5f, -1.25f, 3.75f, 2.5f}, {4});
  const auto idx_cpu = pluss::make_series(std::vector<std::int64_t>{3, 0, 2}, {3});
  const auto x_cpu = x_cpu_f32.to(pluss::ScalarType::Float16);
  const auto want = x_cpu.take(idx_cpu).to(pluss::ScalarType::Float32).host<float>();

  const auto got =
      x_cpu.to(pluss::pMPS).take(idx_cpu.to(pluss::pMPS)).to(pluss::pCPU).to(pluss::ScalarType::Float32).host<float>();
  require_close_(got, want, 2e-2f);
}

TEST_CASE("MPS strict take rejects unsupported dtype when native lane is unavailable", "[mps][take][native][strict][policy]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }
  const ScopedEnvVar no_fallback("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const ScopedEnvVar native_take_put("PLUSS_MPS_NATIVE_TAKE_PUT", "1");
  const auto x_u64 = pluss::make_series(std::vector<std::uint64_t>{1, 2, 3, 4}, {4}).to(pluss::pMPS);
  const auto idx = pluss::make_series(std::vector<std::int64_t>{1, 3}, {2}).to(pluss::pMPS);
  try {
    (void)x_u64.take(idx);
    FAIL("strict native take should fail for unsupported dtype");
  } catch (const std::exception& e) {
    REQUIRE(std::string(e.what()).find("no native MPS kernel yet") != std::string::npos);
  }
}

TEST_CASE("MPS native take supports int16 lane under strict policy", "[mps][take][native][strict][int16]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }
  const ScopedEnvVar no_fallback("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const ScopedEnvVar native_take_put("PLUSS_MPS_NATIVE_TAKE_PUT", "1");

  const auto x_cpu = pluss::make_series(std::vector<std::int16_t>{7, -2, 5, -9, 11}, {5});
  const auto idx_cpu = pluss::make_series(std::vector<std::int64_t>{4, 1, 3, 0}, {2, 2});

  const auto want = x_cpu.take(idx_cpu).host<std::int16_t>().array().data;
  const auto got = x_cpu.to(pluss::pMPS)
                       .take(idx_cpu.to(pluss::pMPS))
                       .to(pluss::pCPU)
                       .host<std::int16_t>()
                       .array()
                       .data;
  require_equal_(got, want);
}

TEST_CASE("MPS native put supports uint8 lane under strict policy", "[mps][put][native][strict][uint8]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }
  const ScopedEnvVar no_fallback("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const ScopedEnvVar native_take_put("PLUSS_MPS_NATIVE_TAKE_PUT", "1");

  auto base_cpu = pluss::Series::zeros({6}, pluss::ScalarType::UInt8);
  const auto idx_cpu = pluss::make_series(std::vector<std::int64_t>{5, 1, 4, 0}, {4});
  const auto src_cpu = pluss::make_series(std::vector<std::uint8_t>{9, 200, 7, 60}, {4});
  auto base_mps = base_cpu.to(pluss::pMPS);
  auto put_fn = static_cast<pluss::Series (pluss::Series::*)(const pluss::Series&, const pluss::Series&, bool)>(&pluss::Series::put_raw_);

  base_cpu = (base_cpu.*put_fn)(idx_cpu, src_cpu, /*accumulate=*/false);
  base_mps = (base_mps.*put_fn)(idx_cpu.to(pluss::pMPS), src_cpu.to(pluss::pMPS), /*accumulate=*/false);
  REQUIRE(base_mps.platform() == pluss::pMPS);
  require_equal_(base_mps.to(pluss::pCPU).items<std::uint8_t>(), base_cpu.items<std::uint8_t>());
}

TEST_CASE("MPS strict put rejects unsupported accumulate lane when fallback is disabled", "[mps][put][native][strict][policy]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }
  const ScopedEnvVar no_fallback("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const ScopedEnvVar native_take_put("PLUSS_MPS_NATIVE_TAKE_PUT", "1");

  auto base = pluss::Series::zeros({4}, pluss::ScalarType::Int16).to(pluss::pMPS);
  const auto idx = pluss::make_series(std::vector<std::int64_t>{1, 1, 2}, {3}).to(pluss::pMPS);
  const auto src = pluss::make_series(std::vector<std::int16_t>{2, 3, 4}, {3}).to(pluss::pMPS);
  try {
    auto put_fn = static_cast<pluss::Series (pluss::Series::*)(const pluss::Series&, const pluss::Series&, bool)>(&pluss::Series::put_raw_);
    (void)(base.*put_fn)(idx, src, /*accumulate=*/true);
    FAIL("strict native put should fail for unsupported accumulate lane");
  } catch (const std::exception& e) {
    REQUIRE(std::string(e.what()).find("no native MPS kernel yet") != std::string::npos);
  }
}

#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cstdlib>
#include <string>
#include <vector>

namespace {

class ScopedEnvVar final {
 public:
  ScopedEnvVar(const char* key, const char* value) : key_(key) {
    if (const char* existing = std::getenv(key_); existing != nullptr) {
      had_previous_ = true;
      previous_ = existing;
    }
    if (value != nullptr) {
      ::setenv(key_, value, 1);
    } else {
      ::unsetenv(key_);
    }
  }

  ~ScopedEnvVar() {
    if (had_previous_) {
      ::setenv(key_, previous_.c_str(), 1);
    } else {
      ::unsetenv(key_);
    }
  }

 private:
  const char* key_{nullptr};
  bool had_previous_{false};
  std::string previous_{};
};

} // namespace

TEST_CASE("MPS native QR device telemetry is reported", "[mps][linearalg][native][telemetry][qr]") {
  using namespace pluss::subsp::linearalg;

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }
  if (!linearalg_mps_experimental_native_enabled()) {
    SUCCEED("Telemetry test requires PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE=1");
    return;
  }

  linearalg_reset_dispatch_counters();

  const auto a_cpu = pluss::make_series(
      std::vector<float>{
          2.5f, -0.3f, 0.7f,
          0.2f, 2.8f, -0.4f,
          -0.5f, 0.3f, 2.2f,
          0.6f, -0.1f, 1.1f},
      {4, 3});
  const auto a_mps = a_cpu.to(pluss::pMPS);

  const auto q = qr_q(a_mps, true);
  const auto r = qr_r(a_mps, true);
  REQUIRE(q.platform() == pluss::pMPS);
  REQUIRE(r.platform() == pluss::pMPS);

  const auto c = linearalg_dispatch_counters();
  REQUIRE(c.mps_bridge_qr == 0);
  REQUIRE(c.mps_bridge_total == 0);
  REQUIRE((c.mps_native_qr_device_success + c.mps_native_qr_device_fallback) >= 1);
}

TEST_CASE("MPS native SVD safe gate telemetry reports fallback lane", "[mps][linearalg][native][telemetry][svd]") {
  using namespace pluss::subsp::linearalg;

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }
  if (!linearalg_mps_experimental_native_enabled()) {
    SUCCEED("Telemetry test requires PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE=1");
    return;
  }

  const ScopedEnvVar enable_svd_device("PLUSS_MPS_LINALG_NATIVE_DEVICE_SVD", "1");
  const ScopedEnvVar safe_mode("PLUSS_MPS_LINALG_NATIVE_DEVICE_SVD_UNSAFE", "0");

  linearalg_reset_dispatch_counters();

  const auto a_cpu = pluss::make_series(
      std::vector<float>{
          2.7f, -0.2f, 0.5f,
          0.2f, 3.1f, -0.4f,
          -0.3f, 0.5f, 2.4f,
          0.7f, -0.2f, 0.8f},
      {4, 3});
  const auto a_mps = a_cpu.to(pluss::pMPS);

  const auto u = svd_u(a_mps, false);
  const auto s = svd_s(a_mps, false);
  const auto vh = svd_vh(a_mps, false);
  REQUIRE(u.platform() == pluss::pMPS);
  REQUIRE(s.platform() == pluss::pMPS);
  REQUIRE(vh.platform() == pluss::pMPS);

  const auto c = linearalg_dispatch_counters();
  REQUIRE(c.mps_bridge_svd == 0);
  REQUIRE(c.mps_bridge_total == 0);
  REQUIRE(c.mps_native_svd_device_success == 0);
  REQUIRE(c.mps_native_svd_device_fallback >= 1);
}


#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

void require_close_(const std::vector<float>& got, const std::vector<float>& want, float atol) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(std::fabs(got[i] - want[i]) <= atol);
  }
}

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

TEST_CASE("MPS explicit to(dtype) covers float16/int32 chains", "[mps][cast][dtype]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  const auto base_cpu = pluss::make_series(
      std::vector<float>{-4.25f, -1.5f, -0.1f, 0.0f, 0.1f, 1.5f, 3.75f, 9.0f},
      {2, 4});
  const auto base_mps = base_cpu.to(pluss::pMPS);

  const auto f16_mps = base_mps.to(pluss::ScalarType::Float16);
  REQUIRE(f16_mps.platform() == pluss::pMPS);
  const auto f16_back = f16_mps.to(pluss::ScalarType::Float32).to(pluss::pCPU).contiguous().to_vec<float>();
  const auto f16_ref = base_cpu.to(pluss::ScalarType::Float16).to(pluss::ScalarType::Float32).contiguous().to_vec<float>();
  require_close_(f16_back, f16_ref, 2e-3f);

  const auto i32_mps = base_mps.to(pluss::ScalarType::Int32);
  REQUIRE(i32_mps.platform() == pluss::pMPS);
  const auto i32_back = i32_mps.to(pluss::ScalarType::Float32).to(pluss::pCPU).contiguous().to_vec<float>();
  const auto i32_ref = base_cpu.to(pluss::ScalarType::Int32).to(pluss::ScalarType::Float32).contiguous().to_vec<float>();
  require_close_(i32_back, i32_ref, 1e-6f);

  const auto i64_roundtrip = i32_mps.to(pluss::ScalarType::Int64).to(pluss::ScalarType::Int32);
  REQUIRE(i64_roundtrip.platform() == pluss::pMPS);
  REQUIRE(i64_roundtrip.to(pluss::pCPU).to_vec<std::int32_t>() == i32_mps.to(pluss::pCPU).to_vec<std::int32_t>());

  const auto bool_mps = i32_mps.to(pluss::ScalarType::Bool);
  REQUIRE(bool_mps.platform() == pluss::pMPS);
  const auto bool_i32_mps = bool_mps.to(pluss::ScalarType::Int32);
  REQUIRE(bool_i32_mps.platform() == pluss::pMPS);
  REQUIRE(bool_i32_mps.to(pluss::pCPU).to_vec<std::int32_t>() == bool_mps.to(pluss::pCPU).to(pluss::ScalarType::Int32).to_vec<std::int32_t>());
}

TEST_CASE("MPS to(dtype) remains available when CPU fallback policy is strict", "[mps][cast][dtype][policy]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  const ScopedEnvVar strict_policy("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");

  const auto base_cpu =
      pluss::make_series(std::vector<float>{-3.5f, -0.5f, 0.0f, 2.25f, 4.0f, 8.5f}, {2, 3});
  const auto base_mps = base_cpu.to(pluss::pMPS);

  // Float16 -> Int64 has no native MPS cast kernel today and must bridge
  // through CPU deterministically even under strict runtime policy.
  const auto f16_mps = base_mps.to(pluss::ScalarType::Float16);
  const auto i64_mps = f16_mps.to(pluss::ScalarType::Int64);
  REQUIRE(i64_mps.platform() == pluss::pMPS);
  REQUIRE(i64_mps.to(pluss::pCPU).to_vec<std::int64_t>() ==
          base_cpu.to(pluss::ScalarType::Float16).to(pluss::ScalarType::Int64).to_vec<std::int64_t>());
}

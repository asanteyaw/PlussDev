#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include "CodeM/native/mps/Sorting.h"

#include <cmath>
#include <cstdlib>
#include <cstdint>
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

void require_close_vec_(const std::vector<float>& got, const std::vector<float>& want, float atol = 1e-4f) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(std::fabs(got[i] - want[i]) <= atol);
  }
}

} // namespace

TEST_CASE("MPS native sort and argsort strict route parity", "[mps][sort][native][strict]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  const auto x_cpu = pluss::make_series(
      std::vector<float>{
          4.0f, -1.0f, 7.0f, 0.0f, 2.0f, 5.0f,
          3.0f, 9.0f, -2.0f, 6.0f, 8.0f, 1.0f,
          -3.0f, 2.0f, 11.0f, -4.0f, 10.0f, 12.0f},
      {3, 6});
  const auto x_mps = x_cpu.to(pluss::pMPS);

  if (!pluss::codem::native::mps::supports_sort_selection(x_mps, /*dim=*/1, /*stable=*/false)) {
    SUCCEED("Native MPS sort-selection kernel unavailable on this runtime");
    return;
  }

  const ScopedEnvVar native_enable("PLUSS_MPS_NATIVE_SORT_SELECTION", "1");
  const ScopedEnvVar native_strict("PLUSS_MPS_NATIVE_SORT_SELECTION_STRICT", "1");
  const ScopedEnvVar strict_fallback("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");

  const auto sort_cpu = x_cpu.sort(/*dim=*/1, /*descending=*/true, /*stable=*/false);
  const auto sort_mps = x_mps.sort(/*dim=*/1, /*descending=*/true, /*stable=*/false);
  REQUIRE(sort_mps.platform() == pluss::pMPS);
  require_close_vec_(sort_mps.to(pluss::pCPU).to_vec<float>(), sort_cpu.to_vec<float>());

  const auto argsort_cpu = x_cpu.argsort(/*dim=*/1, /*descending=*/false, /*stable=*/false);
  const auto argsort_mps = x_mps.argsort(/*dim=*/1, /*descending=*/false, /*stable=*/false);
  REQUIRE(argsort_mps.platform() == pluss::pMPS);
  REQUIRE(argsort_mps.to(pluss::pCPU).to_vec<std::int64_t>() == argsort_cpu.to_vec<std::int64_t>());
}

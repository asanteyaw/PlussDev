#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <pluss/subsp/linearalg/linearalg.h>

#include <cstdlib>
#include <string>

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

TEST_CASE("MPS linalg env policies are runtime-dynamic", "[mps][linearalg][policy][runtime]") {
  using namespace pluss::subsp::linearalg;

  {
    ScopedEnvVar allow_bridge("PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE", "0");
    ScopedEnvVar native_gate("PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE", "0");
    REQUIRE(!linearalg_mps_cpu_bridge_enabled());
    REQUIRE(!linearalg_mps_experimental_native_enabled());
  }

  {
    ScopedEnvVar allow_bridge("PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE", "1");
    ScopedEnvVar native_gate("PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE", "0");
    REQUIRE(linearalg_mps_cpu_bridge_enabled());
    REQUIRE(!linearalg_mps_experimental_native_enabled());
  }

  {
    ScopedEnvVar allow_bridge("PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE", "0");
    ScopedEnvVar native_gate("PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE", "1");
    REQUIRE(!linearalg_mps_cpu_bridge_enabled());
    REQUIRE(linearalg_mps_experimental_native_enabled());
  }
}


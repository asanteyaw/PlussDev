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

TEST_CASE("MPS solver route policy selects provider precedence coherently", "[mps][linearalg][policy][route]") {
  using namespace pluss::subsp::linearalg;

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  {
    ScopedEnvVar route("PLUSS_MPS_LINALG_SOLVER_ROUTE", "auto");
    ScopedEnvVar allow_bridge("PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE", "1");
    ScopedEnvVar native_gate("PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE", "1");
    const auto info = linearalg_backend_info(LinearAlgTarget::MPS);
    REQUIRE(info.solver_provider == LinearAlgSolverProvider::Metal);
    REQUIRE(info.solver_available);
    REQUIRE(info.supports_qr);
    REQUIRE(info.supports_svd);
  }

  {
    ScopedEnvVar route("PLUSS_MPS_LINALG_SOLVER_ROUTE", "bridge");
    ScopedEnvVar allow_bridge("PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE", "1");
    ScopedEnvVar native_gate("PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE", "1");
    const auto info = linearalg_backend_info(LinearAlgTarget::MPS);
    REQUIRE(info.solver_provider == LinearAlgSolverProvider::Reference);
    REQUIRE(info.solver_available);
  }

  {
    ScopedEnvVar route("PLUSS_MPS_LINALG_SOLVER_ROUTE", "bridge");
    ScopedEnvVar allow_bridge("PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE", "0");
    ScopedEnvVar native_gate("PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE", "1");
    const auto info = linearalg_backend_info(LinearAlgTarget::MPS);
    REQUIRE(info.solver_provider == LinearAlgSolverProvider::Metal);
    REQUIRE(info.solver_available);
  }

  {
    ScopedEnvVar route("PLUSS_MPS_LINALG_SOLVER_ROUTE", "native");
    ScopedEnvVar allow_bridge("PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE", "1");
    ScopedEnvVar native_gate("PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE", "0");
    const auto info = linearalg_backend_info(LinearAlgTarget::MPS);
    REQUIRE(info.solver_provider == LinearAlgSolverProvider::Reference);
    REQUIRE(info.solver_available);
  }

  {
    ScopedEnvVar route("PLUSS_MPS_LINALG_SOLVER_ROUTE", "auto");
    ScopedEnvVar allow_bridge("PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE", "0");
    ScopedEnvVar native_gate("PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE", "0");
    const auto info = linearalg_backend_info(LinearAlgTarget::MPS);
    REQUIRE(info.solver_provider == LinearAlgSolverProvider::None);
    REQUIRE_FALSE(info.solver_available);
    REQUIRE_FALSE(info.supports_qr);
    REQUIRE_FALSE(info.supports_svd);
  }
}


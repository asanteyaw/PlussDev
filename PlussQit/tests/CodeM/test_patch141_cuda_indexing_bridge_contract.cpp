#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cmath>
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

void require_close_(const std::vector<float>& got, const std::vector<float>& want, float tol = 1e-5f) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(std::fabs(got[i] - want[i]) <= tol);
  }
}

} // namespace

TEST_CASE("CUDA bridge enables indexing fallback when strict native is off", "[cuda][bridge][indexing]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar indexing_off("PLUSS_CUDA_NATIVE_INDEXING", "0");
  const bool cuda_ready = pluss::cuda::is_ready();
  if (!cuda_ready) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto x_cpu = pluss::make_series(std::vector<float>{3.f, 0.f, 4.f, 0.f, 5.f}, {5});
  const auto idx_cpu = pluss::make_series(std::vector<std::int64_t>{4, 2, 0}, {3});
  const auto src_cpu = pluss::make_series(std::vector<float>{9.f, 8.f, 7.f}, {3});

  const auto x_cuda = x_cpu.to(pluss::pCUDA);
  const auto idx_cuda = idx_cpu.to(pluss::pCUDA);
  const auto src_cuda = src_cpu.to(pluss::pCUDA);

  const auto take_cpu = x_cpu.take(idx_cpu);
  const auto take_cuda = x_cuda.take(idx_cuda);
  REQUIRE(take_cuda.platform() == pluss::pCUDA);
  require_close_(take_cuda.to(pluss::pCPU).to_vec<float>(), take_cpu.to_vec<float>());

  auto put_cpu = pluss::Series::zeros({5}, pluss::ScalarType::Float32);
  auto put_cuda = put_cpu.to(pluss::pCUDA);
  put_cpu.put_(idx_cpu, src_cpu, /*accumulate=*/false);
  put_cuda.put_(idx_cuda, src_cuda, /*accumulate=*/false);
  REQUIRE(put_cuda.platform() == pluss::pCUDA);
  require_close_(put_cuda.to(pluss::pCPU).to_vec<float>(), put_cpu.to_vec<float>());

  const auto nonzero_cpu = x_cpu.nonzero();
  const auto nonzero_cuda = x_cuda.nonzero();
  REQUIRE(nonzero_cuda.platform() == pluss::pCUDA);
  REQUIRE(nonzero_cuda.to(pluss::pCPU).to_vec<std::int64_t>() == nonzero_cpu.to_vec<std::int64_t>());

  const auto index_cpu = x_cpu.index(std::vector<pluss::Series>{idx_cpu});
  const auto index_cuda = x_cuda.index(std::vector<pluss::Series>{idx_cuda});
  REQUIRE(index_cuda.platform() == pluss::pCUDA);
  require_close_(index_cuda.to(pluss::pCPU).to_vec<float>(), index_cpu.to_vec<float>());
}

TEST_CASE("CUDA strict native indexing executes supported native route", "[cuda][bridge][policy][indexing]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar indexing_on("PLUSS_CUDA_NATIVE_INDEXING", "1");
  const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_INDEXING_STRICT", "1");
  const bool cuda_ready = pluss::cuda::is_ready();
  if (!cuda_ready) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto x_cpu = pluss::make_series(std::vector<float>{1.f, 2.f, 3.f, 4.f}, {4});
  const auto idx_cpu = pluss::make_series(std::vector<std::int64_t>{3, 1}, {2});
  const auto x_cuda = x_cpu.to(pluss::pCUDA);
  const auto idx_cuda = idx_cpu.to(pluss::pCUDA);
  const auto take_cpu = x_cpu.take(idx_cpu);
  const auto take_cuda = x_cuda.take(idx_cuda);
  REQUIRE(take_cuda.platform() == pluss::pCUDA);
  require_close_(take_cuda.to(pluss::pCPU).to_vec<float>(), take_cpu.to_vec<float>());
}

#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <optional>
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

void require_close_(const std::vector<double>& got, const std::vector<double>& want, double tol = 1e-8) {
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

TEST_CASE("CUDA native lane expansion covers dim/layout/dtype widening", "[cuda][parity][native][expanded]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar selection_on("PLUSS_CUDA_NATIVE_SELECTION", "1");
  const ScopedEnvVar selection_strict("PLUSS_CUDA_NATIVE_SELECTION_STRICT", "1");
  const ScopedEnvVar indexing_on("PLUSS_CUDA_NATIVE_INDEXING", "1");
  const ScopedEnvVar indexing_strict("PLUSS_CUDA_NATIVE_INDEXING_STRICT", "1");
  const ScopedEnvVar matmul_on("PLUSS_CUDA_NATIVE_MATMUL", "1");
  const ScopedEnvVar matmul_strict("PLUSS_CUDA_NATIVE_MATMUL_STRICT", "1");
  const ScopedEnvVar scatter_on("PLUSS_CUDA_NATIVE_SCATTER_GATHER", "1");
  const ScopedEnvVar scatter_strict("PLUSS_CUDA_NATIVE_SCATTER_GATHER_STRICT", "1");
  const ScopedEnvVar reduction_on("PLUSS_CUDA_NATIVE_REDUCTION", "1");
  const ScopedEnvVar reduction_strict("PLUSS_CUDA_NATIVE_REDUCTION_STRICT", "1");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  // Selection: non-last-dim + stable lane + integer dtype.
  const auto sel_cpu = pluss::make_series(std::vector<std::int64_t>{9, 1, 5, 2, 7, 0}, {2, 3});
  const auto sel_cuda = sel_cpu.to(pluss::pCUDA);
  const auto sort0_cpu = sel_cpu.sort(/*dim=*/0, /*descending=*/false, /*stable=*/true);
  const auto sort0_cuda = sel_cuda.sort(/*dim=*/0, /*descending=*/false, /*stable=*/true);
  require_equal_(sort0_cuda.to(pluss::pCPU).to_vec<std::int64_t>(), sort0_cpu.to_vec<std::int64_t>());

  require_equal_(
      sel_cuda.sum(/*dim=*/0, /*keepdim=*/false).to(pluss::pCPU).to_vec<std::int64_t>(),
      sel_cpu.sum(/*dim=*/0, /*keepdim=*/false).to_vec<std::int64_t>());

  const auto topk_cpu = sel_cpu.topk(/*k=*/2, /*dim=*/0, /*largest=*/true, /*sorted=*/false);
  const auto topk_cuda = sel_cuda.topk(/*k=*/2, /*dim=*/0, /*largest=*/true, /*sorted=*/false);
  require_equal_(topk_cuda.to(pluss::pCPU).to_vec<std::int64_t>(), topk_cpu.to_vec<std::int64_t>());

  const auto argsort_cpu = sel_cpu.argsort(/*dim=*/0, /*descending=*/false, /*stable=*/true);
  const auto argsort_cuda = sel_cuda.argsort(/*dim=*/0, /*descending=*/false, /*stable=*/true);
  require_equal_(argsort_cuda.to(pluss::pCPU).to_vec<std::int64_t>(), argsort_cpu.to_vec<std::int64_t>());

  // Matmul: float64 + non-contiguous lane.
  const auto a_base_cpu = pluss::make_series(
      std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0},
      {4, 3});
  const auto b_base_cpu = pluss::make_series(
      std::vector<double>{2.0, 0.0, 1.0, 3.0, 4.0, 1.0, -1.0, 2.0},
      {2, 4});
  const auto a_cpu = a_base_cpu.transpose(0, 1); // [3,4], non-contiguous
  const auto b_cpu = b_base_cpu.transpose(0, 1); // [4,2], non-contiguous
  const auto a_cuda = a_cpu.to(pluss::pCUDA);
  const auto b_cuda = b_cpu.to(pluss::pCUDA);
  require_close_(
      a_cuda.matmul(b_cuda).to(pluss::pCPU).to_vec<double>(),
      a_cpu.matmul(b_cpu).to_vec<double>());

  // Indexing: non-contiguous take lane.
  const auto idx_cpu = pluss::make_series(std::vector<std::int64_t>{0, 5, 8, 3}, {4});
  const auto idx_cuda = idx_cpu.to(pluss::pCUDA);
  require_close_(
      a_cuda.take(idx_cuda).to(pluss::pCPU).to_vec<double>(),
      a_cpu.take(idx_cpu).to_vec<double>());

  // Scatter/gather: float64 + broadcast src lane.
  const auto self_cpu = pluss::Series::zeros({2, 3}, pluss::ScalarType::Float64);
  const auto idx2_cpu = pluss::make_series(std::vector<std::int64_t>{2, 0, 1, 1, 2, 0}, {2, 3});
  const auto src_cpu = pluss::make_series(std::vector<double>{10.0, 20.0, 30.0}, {1, 3});
  const auto self_cuda = self_cpu.to(pluss::pCUDA);
  const auto idx2_cuda = idx2_cpu.to(pluss::pCUDA);
  const auto src_cuda = src_cpu.to(pluss::pCUDA);

  require_close_(
      self_cuda.scatter(1, idx2_cuda, src_cuda).to(pluss::pCPU).to_vec<double>(),
      self_cpu.scatter(1, idx2_cpu, src_cpu).to_vec<double>());

  const auto input_cpu = pluss::make_series(std::vector<double>{3.0, 4.0, 5.0, 6.0, 7.0, 8.0}, {2, 3});
  const auto input_cuda = input_cpu.to(pluss::pCUDA);
  require_close_(
      input_cuda.gather(1, idx2_cuda).to(pluss::pCPU).to_vec<double>(),
      input_cpu.gather(1, idx2_cpu).to_vec<double>());

  require_close_(
      a_cuda.sum(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<double>(),
      a_cpu.sum(/*dim=*/1, /*keepdim=*/false).to_vec<double>());
  require_close_(
      a_cuda.mean(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<double>(),
      a_cpu.mean(/*dim=*/1, /*keepdim=*/false).to_vec<double>());
  require_equal_(
      a_cuda.argmax(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<std::int64_t>(),
      a_cpu.argmax(/*dim=*/1, /*keepdim=*/false).to_vec<std::int64_t>());
}

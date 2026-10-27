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

std::vector<float> to_cpu_f32_(const pluss::Series& x) {
  return x.to(pluss::pCPU).to(pluss::ScalarType::Float32).contiguous().host<float>();
}

std::vector<std::int64_t> to_cpu_i64_(const pluss::Series& x) {
  return x.to(pluss::pCPU).to(pluss::ScalarType::Int64).contiguous().host<std::int64_t>();
}

void require_close_f32_(const pluss::Series& got, const pluss::Series& want, float tol = 1.0e-6f) {
  const auto gv = to_cpu_f32_(got);
  const auto wv = to_cpu_f32_(want);
  REQUIRE(gv.size() == wv.size());
  for (std::size_t i = 0; i < gv.size(); ++i) {
    REQUIRE(std::fabs(gv[i] - wv[i]) <= tol);
  }
}

void require_equal_i64_(const pluss::Series& got, const pluss::Series& want) {
  const auto gv = to_cpu_i64_(got);
  const auto wv = to_cpu_i64_(want);
  REQUIRE(gv.size() == wv.size());
  for (std::size_t i = 0; i < gv.size(); ++i) {
    REQUIRE(gv[i] == wv[i]);
  }
}

} // namespace

TEST_CASE("CUDA factory baseline capability is runtime-dynamic", "[cuda][factory][runtime]") {
  const bool cuda_ready = pluss::cuda::is_ready();
  {
    const ScopedEnvVar baseline_off("PLUSS_CUDA_NATIVE_BASELINE", "0");
    const ScopedEnvVar factory_on("PLUSS_CUDA_NATIVE_FACTORY", "1");
    REQUIRE(pluss::cuda::native_factory_enabled());
    REQUIRE_FALSE(pluss::cuda::supports_arange_factory_baseline(pluss::ScalarType::Int64));
    REQUIRE_FALSE(pluss::cuda::supports_rand_factory_baseline(pluss::ScalarType::Float32));
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    const ScopedEnvVar factory_off("PLUSS_CUDA_NATIVE_FACTORY", "0");
    REQUIRE_FALSE(pluss::cuda::native_factory_enabled());
    REQUIRE_FALSE(pluss::cuda::supports_zeros_factory_baseline(pluss::ScalarType::Float32));
    REQUIRE_FALSE(pluss::cuda::supports_randn_factory_baseline(pluss::ScalarType::Float32));
  }
  {
    const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
    const ScopedEnvVar factory_on("PLUSS_CUDA_NATIVE_FACTORY", "1");
    REQUIRE(pluss::cuda::native_factory_enabled());
    REQUIRE(pluss::cuda::supports_zeros_factory_baseline(pluss::ScalarType::Float32) == cuda_ready);
    REQUIRE(pluss::cuda::supports_ones_factory_baseline(pluss::ScalarType::UInt16) == cuda_ready);
    REQUIRE_FALSE(pluss::cuda::supports_ones_factory_baseline(pluss::ScalarType::Complex64));
    REQUIRE(pluss::cuda::supports_full_factory_baseline(pluss::ScalarType::Int64) == cuda_ready);
    REQUIRE_FALSE(pluss::cuda::supports_full_factory_baseline(pluss::ScalarType::Float8E4M3FN));
    REQUIRE(pluss::cuda::supports_arange_factory_baseline(pluss::ScalarType::Int64) == cuda_ready);
    REQUIRE(pluss::cuda::supports_linspace_factory_baseline(pluss::ScalarType::Float64) == cuda_ready);
    REQUIRE_FALSE(pluss::cuda::supports_linspace_factory_baseline(pluss::ScalarType::Float16));
    REQUIRE(pluss::cuda::supports_eye_factory_baseline(pluss::ScalarType::UInt16) == cuda_ready);
    REQUIRE(pluss::cuda::supports_rand_factory_baseline(pluss::ScalarType::Float32) == cuda_ready);
    REQUIRE(pluss::cuda::supports_randn_factory_baseline(pluss::ScalarType::Float64) == cuda_ready);
    REQUIRE_FALSE(pluss::cuda::supports_randn_factory_baseline(pluss::ScalarType::BFloat16));
    REQUIRE_FALSE(pluss::cuda::supports_rand_factory_baseline(pluss::ScalarType::Int64));
    REQUIRE_FALSE(pluss::cuda::supports_linspace_factory_baseline(pluss::ScalarType::Int64));
  }
}

TEST_CASE("CUDA factory strict policy is runtime-dynamic", "[cuda][factory][policy][runtime]") {
  {
    const ScopedEnvVar strict_off("PLUSS_CUDA_NATIVE_FACTORY_STRICT", "0");
    REQUIRE_FALSE(pluss::cuda::native_factory_strict());
  }
  {
    const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_FACTORY_STRICT", "1");
    REQUIRE(pluss::cuda::native_factory_strict());
  }
}

TEST_CASE("CUDA bridge enables factory fallback when strict native is off", "[cuda][bridge][factory]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar factory_off("PLUSS_CUDA_NATIVE_FACTORY", "0");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const pluss::Config cfg_cuda = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCUDA);

  require_close_f32_(pluss::series::zeros({2, 3}, cfg_cuda), pluss::series::zeros({2, 3}, pluss::ScalarType::Float32));
  require_close_f32_(pluss::series::ones({2, 3}, cfg_cuda), pluss::series::ones({2, 3}, pluss::ScalarType::Float32));
  require_close_f32_(pluss::series::full({2, 3}, 2.5, cfg_cuda), pluss::series::full({2, 3}, 2.5, pluss::ScalarType::Float32));
  require_equal_i64_(
      pluss::series::arange(0, 7, pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(pluss::pCUDA)),
      pluss::series::arange(0, 7, pluss::ScalarType::Int64));
  require_close_f32_(
      pluss::series::linspace(0.0, 1.0, 7, cfg_cuda),
      pluss::series::linspace(0.0, 1.0, 7, pluss::ScalarType::Float32));
  require_close_f32_(pluss::series::eye(4, cfg_cuda), pluss::series::eye(4, -1, pluss::ScalarType::Float32));

  require_close_f32_(
      pluss::series::rand({3, 3}, cfg_cuda, /*seed=*/1234),
      pluss::series::rand({3, 3}, pluss::ScalarType::Float32, /*seed=*/1234));
  require_close_f32_(
      pluss::series::randn({3, 3}, cfg_cuda, /*seed=*/4321),
      pluss::series::randn({3, 3}, pluss::ScalarType::Float32, /*seed=*/4321));
  require_equal_i64_(
      pluss::series::randint(2, 10, {8}, pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(pluss::pCUDA), /*seed=*/42),
      pluss::series::randint(2, 10, {8}, pluss::ScalarType::Int64, /*seed=*/42));
}

TEST_CASE("CUDA strict native factory executes supported lanes without CPU bridge", "[cuda][bridge][policy][factory][no_bridge]") {
  const ScopedEnvVar bridge_off("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "0");
  const ScopedEnvVar forbid_materialize("PLUSS_CUDA_FORBID_CPU_MATERIALIZATION", "1");
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar factory_on("PLUSS_CUDA_NATIVE_FACTORY", "1");
  const ScopedEnvVar elementwise_on("PLUSS_CUDA_NATIVE_ELEMENTWISE", "1");
  const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_FACTORY_STRICT", "1");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const pluss::Config cfg_cuda = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCUDA);
  const auto zeros = pluss::series::zeros({2, 2}, cfg_cuda);
  const auto ones = pluss::series::ones({2, 2}, cfg_cuda);
  const auto full = pluss::series::full({2, 2}, 2.5, cfg_cuda);
  const auto range = pluss::series::arange(0, 5, pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(pluss::pCUDA));
  const auto line = pluss::series::linspace(0.0, 1.0, 5, cfg_cuda);
  const auto eye = pluss::series::eye(3, cfg_cuda);
  const auto rand = pluss::series::rand({2, 2}, cfg_cuda, /*seed=*/7);
  const auto randn = pluss::series::randn({2, 2}, cfg_cuda, /*seed=*/11);
  const auto nan = pluss::series::nan(cfg_cuda);
  const auto inf = pluss::series::inf(cfg_cuda);
  const auto logspace = pluss::series::logspace(0.0, 2.0, 4, 10.0, cfg_cuda);
  const auto rand_btw = pluss::series::rand_btw(-2.0, 3.0, {2, 2}, cfg_cuda, /*seed=*/17);
  const auto randint = pluss::series::randint(
      2,
      10,
      {4},
      pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(pluss::pCUDA),
      /*seed=*/13);
  const auto real = pluss::series::linspace(1.0, 3.0, 3, cfg_cuda);
  const auto imag = pluss::series::full({3}, 0.5, cfg_cuda);
  const auto complex = pluss::series::complex(real, imag, pluss::ScalarType::Complex64);
  const auto polar = pluss::series::polar(real, imag, pluss::ScalarType::Complex64);

  REQUIRE(zeros.platform() == pluss::pCUDA);
  REQUIRE(ones.platform() == pluss::pCUDA);
  REQUIRE(full.platform() == pluss::pCUDA);
  REQUIRE(range.platform() == pluss::pCUDA);
  REQUIRE(line.platform() == pluss::pCUDA);
  REQUIRE(eye.platform() == pluss::pCUDA);
  REQUIRE(rand.platform() == pluss::pCUDA);
  REQUIRE(randn.platform() == pluss::pCUDA);
  REQUIRE(nan.platform() == pluss::pCUDA);
  REQUIRE(inf.platform() == pluss::pCUDA);
  REQUIRE(logspace.platform() == pluss::pCUDA);
  REQUIRE(rand_btw.platform() == pluss::pCUDA);
  REQUIRE(randint.platform() == pluss::pCUDA);
  REQUIRE(complex.platform() == pluss::pCUDA);
  REQUIRE(polar.platform() == pluss::pCUDA);
  REQUIRE(zeros.storage().location() == pluss::codem::StorageLocation::CudaDevice);
  REQUIRE(ones.storage().location() == pluss::codem::StorageLocation::CudaDevice);
  REQUIRE(full.storage().location() == pluss::codem::StorageLocation::CudaDevice);
  REQUIRE(range.storage().location() == pluss::codem::StorageLocation::CudaDevice);
  REQUIRE(line.storage().location() == pluss::codem::StorageLocation::CudaDevice);
  REQUIRE(eye.storage().location() == pluss::codem::StorageLocation::CudaDevice);
  REQUIRE(rand.storage().location() == pluss::codem::StorageLocation::CudaDevice);
  REQUIRE(randn.storage().location() == pluss::codem::StorageLocation::CudaDevice);
  REQUIRE(nan.storage().location() == pluss::codem::StorageLocation::CudaDevice);
  REQUIRE(inf.storage().location() == pluss::codem::StorageLocation::CudaDevice);
  REQUIRE(logspace.storage().location() == pluss::codem::StorageLocation::CudaDevice);
  REQUIRE(rand_btw.storage().location() == pluss::codem::StorageLocation::CudaDevice);
  REQUIRE(randint.storage().location() == pluss::codem::StorageLocation::CudaDevice);
  REQUIRE(complex.storage().location() == pluss::codem::StorageLocation::CudaDevice);
  REQUIRE(polar.storage().location() == pluss::codem::StorageLocation::CudaDevice);

  {
    const ScopedEnvVar readback_bridge("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
    const ScopedEnvVar allow_materialize("PLUSS_CUDA_FORBID_CPU_MATERIALIZATION", "0");
    require_close_f32_(ones, pluss::series::ones({2, 2}, pluss::ScalarType::Float32));
    require_close_f32_(full, pluss::series::full({2, 2}, 2.5, pluss::ScalarType::Float32));
    require_equal_i64_(range, pluss::series::arange(0, 5, pluss::ScalarType::Int64));
    require_close_f32_(line, pluss::series::linspace(0.0, 1.0, 5, pluss::ScalarType::Float32));
    require_close_f32_(eye, pluss::series::eye(3, -1, pluss::ScalarType::Float32));
    const auto nan_v = to_cpu_f32_(nan);
    const auto inf_v = to_cpu_f32_(inf);
    REQUIRE(nan_v.size() == 1);
    REQUIRE(std::isnan(nan_v[0]));
    REQUIRE(inf_v.size() == 1);
    REQUIRE(std::isinf(inf_v[0]));
    require_close_f32_(logspace, pluss::series::logspace(0.0, 2.0, 4, 10.0, pluss::ScalarType::Float32), 5.0e-5f);
    REQUIRE(to_cpu_f32_(rand).size() == 4);
    REQUIRE(to_cpu_f32_(randn).size() == 4);
    REQUIRE(to_cpu_f32_(rand_btw).size() == 4);
    REQUIRE(to_cpu_i64_(randint).size() == 4);
  }
}

TEST_CASE("CUDA strict native factory rejects unsupported policy lanes", "[cuda][bridge][policy][factory][reject]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar baseline_off("PLUSS_CUDA_NATIVE_BASELINE", "0");
  const ScopedEnvVar factory_on("PLUSS_CUDA_NATIVE_FACTORY", "1");
  const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_FACTORY_STRICT", "1");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const pluss::Config cfg_cuda = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCUDA);
  REQUIRE_THROWS(pluss::series::zeros({2, 2}, cfg_cuda));
  REQUIRE_THROWS(pluss::series::rand({2, 2}, cfg_cuda, /*seed=*/11));
}

TEST_CASE("CUDA bridge executes factory like-routes", "[cuda][bridge][factory]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar factory_off("PLUSS_CUDA_NATIVE_FACTORY", "0");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto like_cpu = pluss::make_series(std::vector<float>{1.f, 2.f, 3.f, 4.f, 5.f, 6.f}, {2, 3});
  const auto like_gpu = like_cpu.to(pluss::pCUDA);
  const pluss::Config cfg_cuda = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCUDA);
  const pluss::Config cfg_cpu = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU);

  require_close_f32_(pluss::series::zeros_like(like_gpu, cfg_cuda), pluss::series::zeros_like(like_cpu, cfg_cpu));
  require_close_f32_(pluss::series::ones_like(like_gpu, cfg_cuda), pluss::series::ones_like(like_cpu, cfg_cpu));
  require_close_f32_(pluss::series::full_like(like_gpu, 3.0, cfg_cuda), pluss::series::full_like(like_cpu, 3.0, cfg_cpu));
  require_close_f32_(pluss::series::eye_like(like_gpu, cfg_cuda), pluss::series::eye_like(like_cpu, cfg_cpu));
  require_close_f32_(pluss::series::rand_like(like_gpu, cfg_cuda, /*seed=*/97), pluss::series::rand_like(like_cpu, cfg_cpu, /*seed=*/97));
  require_close_f32_(pluss::series::randn_like(like_gpu, cfg_cuda, /*seed=*/31), pluss::series::randn_like(like_cpu, cfg_cpu, /*seed=*/31));
  require_equal_i64_(
      pluss::series::randint_like(
          like_gpu,
          2,
          11,
          pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(pluss::pCUDA),
          /*seed=*/53),
      pluss::series::randint_like(
          like_cpu,
          2,
          11,
          pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(pluss::pCPU),
          /*seed=*/53));
}

TEST_CASE("CUDA strict native factory rejects unsupported like-routes", "[cuda][bridge][policy][factory][reject]") {
  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  std::optional<pluss::Series> like_gpu;
  {
    const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
    const ScopedEnvVar factory_off("PLUSS_CUDA_NATIVE_FACTORY", "0");
    const ScopedEnvVar strict_off("PLUSS_CUDA_NATIVE_FACTORY_STRICT", "0");
    like_gpu = pluss::series::ones({2, 3}, pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCUDA));
  }

  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar baseline_off("PLUSS_CUDA_NATIVE_BASELINE", "0");
  const ScopedEnvVar factory_on("PLUSS_CUDA_NATIVE_FACTORY", "1");
  const ScopedEnvVar strict_on("PLUSS_CUDA_NATIVE_FACTORY_STRICT", "1");

  const pluss::Config cfg_cuda = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCUDA);

  REQUIRE(like_gpu.has_value());
  REQUIRE_THROWS(pluss::series::zeros_like(*like_gpu, cfg_cuda));
  REQUIRE_THROWS(pluss::series::rand_like(*like_gpu, cfg_cuda, /*seed=*/7));
}

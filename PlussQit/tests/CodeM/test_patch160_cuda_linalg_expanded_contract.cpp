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

void require_close_(const std::vector<double>& got, const std::vector<double>& want, double tol = 1e-8) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(std::fabs(got[i] - want[i]) <= tol);
  }
}

} // namespace

TEST_CASE("CUDA expanded linalg baseline supports fp16 and bf16 lanes", "[cuda][expanded][linalg][runtime]") {
  const bool cuda_ready = pluss::cuda::is_ready();
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar linalg_on("PLUSS_CUDA_NATIVE_LINALG", "1");

  REQUIRE(pluss::cuda::supports_det_linalg_baseline(pluss::ScalarType::Float16) == cuda_ready);
  REQUIRE(pluss::cuda::supports_inverse_linalg_baseline(pluss::ScalarType::BFloat16) == cuda_ready);
  REQUIRE(pluss::cuda::supports_cholesky_linalg_baseline(pluss::ScalarType::Float16) == cuda_ready);
  REQUIRE(pluss::cuda::supports_pinverse_linalg_baseline(pluss::ScalarType::BFloat16) == cuda_ready);
  REQUIRE(pluss::cuda::supports_triangular_solve_linalg_baseline(pluss::ScalarType::Float16, pluss::ScalarType::Float16) == cuda_ready);
  REQUIRE_FALSE(pluss::cuda::supports_triangular_solve_linalg_baseline(pluss::ScalarType::Float16, pluss::ScalarType::Float32));
}

TEST_CASE("CUDA bridge expanded linalg supports non-contiguous and batched layouts", "[bridge][cuda][expanded][linalg]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto a_cpu = pluss::make_series(
      std::vector<double>{4.0, 1.0, 0.5, 0.0, 3.0, 0.2, 0.0, 0.0, 2.0},
      {3, 3});
  const auto a_cpu_t = a_cpu.transpose(0, 1);
  const auto a_cuda_t = a_cpu.to(pluss::pCUDA).transpose(0, 1);

  REQUIRE_FALSE(a_cuda_t.is_contiguous());
  REQUIRE(pluss::cuda::supports_inverse_linalg(a_cuda_t));
  require_close_(
      pluss::subsp::linearalg::inverse(a_cuda_t).to(pluss::pCPU).to_vec<double>(),
      pluss::subsp::linearalg::inverse(a_cpu_t).to_vec<double>(),
      1e-8);

  std::vector<double> mats;
  mats.reserve(2 * 2 * 3 * 3);
  for (std::int64_t b0 = 0; b0 < 2; ++b0) {
    for (std::int64_t b1 = 0; b1 < 2; ++b1) {
      const double s = 1.0 + static_cast<double>(b0 + b1);
      mats.push_back(2.0 * s);
      mats.push_back(0.0);
      mats.push_back(0.0);
      mats.push_back(0.0);
      mats.push_back(3.0 * s);
      mats.push_back(0.0);
      mats.push_back(0.0);
      mats.push_back(0.0);
      mats.push_back(4.0 * s);
    }
  }
  const auto batch_cpu = pluss::make_series(mats, {2, 2, 3, 3});
  const auto batch_cpu_nt = batch_cpu.transpose(0, 1);
  const auto batch_cuda_nt = batch_cpu.to(pluss::pCUDA).transpose(0, 1);

  REQUIRE(pluss::cuda::supports_det_linalg(batch_cuda_nt));
  require_close_(
      pluss::subsp::linearalg::det(batch_cuda_nt).to(pluss::pCPU).to_vec<double>(),
      pluss::subsp::linearalg::det(batch_cpu_nt).to_vec<double>(),
      1e-8);
}

TEST_CASE("CUDA strict expanded linalg supports fp16-bf16 and non-contiguous QR", "[bridge][cuda][expanded][linalg][policy]") {
  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar linalg_on("PLUSS_CUDA_NATIVE_LINALG", "1");
  const ScopedEnvVar linalg_strict_on("PLUSS_CUDA_NATIVE_LINALG_STRICT", "1");

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const auto base = pluss::make_series(std::vector<double>{5.0, 1.0, 0.0, 2.0}, {2, 2});
  for (const auto dtype : {pluss::ScalarType::Float16, pluss::ScalarType::BFloat16}) {
    const auto cpu_typed = base.to(dtype);
    const auto cuda_typed = cpu_typed.to(pluss::pCUDA);
    REQUIRE(pluss::cuda::supports_det_linalg(cuda_typed));
    const auto det_cuda = pluss::subsp::linearalg::det(cuda_typed).to(pluss::pCPU).item<double>();
    const auto det_cpu = pluss::subsp::linearalg::det(cpu_typed).item<double>();
    REQUIRE(std::fabs(det_cuda - det_cpu) <= 1e-6);
  }

  const auto qin_cpu = pluss::make_series(
      std::vector<double>{1.0, 0.2, 0.3, 0.4, 1.5, 0.6, 0.7, 0.8, 1.9, 1.0, 1.1, 2.4},
      {4, 3});
  const auto qin_cpu_nt = qin_cpu.transpose(0, 1);
  const auto qin_cuda_nt = qin_cpu.to(pluss::pCUDA).transpose(0, 1);
  REQUIRE_FALSE(qin_cuda_nt.is_contiguous());
  REQUIRE(pluss::cuda::supports_qr_linalg(qin_cuda_nt, /*reduced=*/true));

  const auto q = pluss::subsp::linearalg::qr_q(qin_cuda_nt, /*reduced=*/true).to(pluss::pCPU);
  const auto r = pluss::subsp::linearalg::qr_r(qin_cuda_nt, /*reduced=*/true).to(pluss::pCPU);
  const auto recon = q.matmul(r);
  require_close_(recon.to_vec<double>(), qin_cpu_nt.to_vec<double>(), 5e-6);
}

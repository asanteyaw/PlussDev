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

void require_shape_(const pluss::Series& s, const std::vector<std::int64_t>& want) {
  const auto got = s.sizes();
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < want.size(); ++i) {
    REQUIRE(got[i] == want[i]);
  }
}

} // namespace

TEST_CASE("CUDA batched linalg handles multi-leading det shape", "[cuda][linalg][batched][runtime]") {
  std::vector<double> mats;
  mats.reserve(2 * 3 * 2 * 2);
  for (std::int64_t b0 = 0; b0 < 2; ++b0) {
    for (std::int64_t b1 = 0; b1 < 3; ++b1) {
      const double d0 = 1.0 + static_cast<double>(b0 + b1);
      const double d1 = 2.0 + static_cast<double>(b0 + 2 * b1);
      mats.push_back(d0);
      mats.push_back(0.0);
      mats.push_back(0.0);
      mats.push_back(d1);
    }
  }
  const auto a_cpu = pluss::make_series(mats, {2, 3, 2, 2});
  const auto det_cpu = pluss::subsp::linearalg::det(a_cpu);
  require_shape_(det_cpu, {2, 3});

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar linalg_on("PLUSS_CUDA_NATIVE_LINALG", "1");
  const ScopedEnvVar linalg_strict_off("PLUSS_CUDA_NATIVE_LINALG_STRICT", "0");

  const auto det_cuda = pluss::subsp::linearalg::det(a_cpu.to(pluss::pCUDA)).to(pluss::pCPU);
  require_shape_(det_cuda, {2, 3});
  require_close_(det_cuda.to_vec<double>(), det_cpu.to_vec<double>());
}

TEST_CASE("CUDA bridge batched triangular_solve keeps full leading batch shape", "[cuda][bridge][linalg][batched]") {
  std::vector<double> coeff;
  coeff.reserve(2 * 3 * 2 * 2);
  for (std::int64_t b0 = 0; b0 < 2; ++b0) {
    for (std::int64_t b1 = 0; b1 < 3; ++b1) {
      const double a00 = 2.0 + 0.1 * static_cast<double>(b0 + b1);
      const double a10 = 0.25 + 0.05 * static_cast<double>(b1);
      const double a11 = 3.0 + 0.2 * static_cast<double>(b0 + 2 * b1);
      coeff.push_back(a00);
      coeff.push_back(0.0);
      coeff.push_back(a10);
      coeff.push_back(a11);
    }
  }
  std::vector<double> rhs(2 * 3 * 2 * 1, 0.0);
  for (std::size_t i = 0; i < rhs.size(); ++i) {
    rhs[i] = 0.5 + static_cast<double>(i % 7);
  }

  const auto a_cpu = pluss::make_series(coeff, {2, 3, 2, 2});
  const auto b_cpu = pluss::make_series(rhs, {2, 3, 2, 1});
  const auto x_cpu = pluss::subsp::linearalg::triangular_solve(a_cpu, b_cpu, /*upper=*/false, /*unitriangular=*/false);
  require_shape_(x_cpu, {2, 3, 2, 1});

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");

  const auto x_cuda = pluss::subsp::linearalg::triangular_solve(
      a_cpu.to(pluss::pCUDA),
      b_cpu.to(pluss::pCUDA),
      /*upper=*/false,
      /*unitriangular=*/false).to(pluss::pCPU);

  require_shape_(x_cuda, {2, 3, 2, 1});
  require_close_(x_cuda.to_vec<double>(), x_cpu.to_vec<double>());
}

TEST_CASE("CUDA strict linalg batched QR keeps full leading batch shape", "[cuda][bridge][policy][linalg][batched]") {
  std::vector<double> mats;
  mats.reserve(2 * 3 * 3 * 2);
  for (std::int64_t b0 = 0; b0 < 2; ++b0) {
    for (std::int64_t b1 = 0; b1 < 3; ++b1) {
      const double s = 1.0 + static_cast<double>(b0 + b1);
      mats.push_back(1.0 * s);
      mats.push_back(0.25 * s);
      mats.push_back(0.5 * s);
      mats.push_back(1.5 * s);
      mats.push_back(2.0 * s);
      mats.push_back(0.75 * s);
    }
  }

  const auto a_cpu = pluss::make_series(mats, {2, 3, 3, 2});
  const auto q_cpu = pluss::subsp::linearalg::qr_q(a_cpu, /*reduced=*/true);
  require_shape_(q_cpu, {2, 3, 3, 2});

  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  const ScopedEnvVar bridge_on("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar baseline_on("PLUSS_CUDA_NATIVE_BASELINE", "1");
  const ScopedEnvVar linalg_on("PLUSS_CUDA_NATIVE_LINALG", "1");
  const ScopedEnvVar linalg_strict_on("PLUSS_CUDA_NATIVE_LINALG_STRICT", "1");

  const auto q_cuda = pluss::subsp::linearalg::qr_q(a_cpu.to(pluss::pCUDA), /*reduced=*/true).to(pluss::pCPU);
  require_shape_(q_cuda, {2, 3, 3, 2});

  const auto r_cuda = pluss::subsp::linearalg::qr_r(a_cpu.to(pluss::pCUDA), /*reduced=*/true).to(pluss::pCPU);
  const auto recon_cuda = q_cuda.matmul(r_cuda);
  const auto recon_cpu = a_cpu;
  require_close_(recon_cuda.to_vec<double>(), recon_cpu.to_vec<double>(), 5e-6);
}

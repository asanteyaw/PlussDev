#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>
#include <pluss/subsp/linearalg/linearalg.h>

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
    if (const char* old = std::getenv(key_)) old_ = std::string(old);
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
  std::optional<std::string> old_{};
};

void require_close_(const std::vector<float>& got, const std::vector<float>& want, float tol, const char* label) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    if (std::isnan(got[i]) && std::isnan(want[i])) continue;
    if (std::isinf(got[i]) || std::isinf(want[i])) {
      INFO(label << " i=" << i << " got=" << got[i] << " want=" << want[i] << " tol=" << tol);
      REQUIRE(got[i] == want[i]);
      continue;
    }
    INFO(label << " i=" << i << " got=" << got[i] << " want=" << want[i] << " tol=" << tol);
    REQUIRE(std::fabs(got[i] - want[i]) <= tol);
  }
}

void require_equal_i32_(const std::vector<std::int32_t>& got, const std::vector<std::int32_t>& want, const char* label) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    INFO(label << " i=" << i << " got=" << got[i] << " want=" << want[i]);
    REQUIRE(got[i] == want[i]);
  }
}

void require_equal_i64_(const std::vector<std::int64_t>& got, const std::vector<std::int64_t>& want, const char* label) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    INFO(label << " i=" << i << " got=" << got[i] << " want=" << want[i]);
    REQUIRE(got[i] == want[i]);
  }
}

} // namespace

TEST_CASE("MPS strict torchwide float32 unary/binary/reduction parity", "[mps][strict][torchwide][float32]") {
  const ScopedEnvVar fallback_off("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const ScopedEnvVar native_take_put("PLUSS_MPS_NATIVE_TAKE_PUT", "1");
  const ScopedEnvVar native_sort("PLUSS_MPS_NATIVE_SORT_SELECTION", "1");
  const ScopedEnvVar native_sort_strict("PLUSS_MPS_NATIVE_SORT_SELECTION_STRICT", "1");
  const ScopedEnvVar native_topk("PLUSS_MPS_NATIVE_TOPK_SELECTION", "1");

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  const auto x_cpu = pluss::make_series(
      std::vector<float>{-2.5f, -1.25f, -0.4f, 0.1f, 0.75f, 1.7f},
      {2, 3});
  const auto y_cpu = pluss::make_series(
      std::vector<float>{1.25f, -0.5f, 0.4f, 2.1f, -0.75f, 0.2f},
      {2, 3});
  const auto x_mps = x_cpu.to(pluss::pMPS);
  const auto y_mps = y_cpu.to(pluss::pMPS);

  require_close_(x_mps.abs().to(pluss::pCPU).to_vec<float>(), x_cpu.abs().to_vec<float>(), 2e-4f, "abs");
  require_close_(x_mps.sign().to(pluss::pCPU).to_vec<float>(), x_cpu.sign().to_vec<float>(), 2e-4f, "sign");
  require_close_(x_mps.exp().to(pluss::pCPU).to_vec<float>(), x_cpu.exp().to_vec<float>(), 4e-4f, "exp");
  require_close_(x_mps.log().to(pluss::pCPU).to_vec<float>(),
                 x_cpu.log().to_vec<float>(),
                 4e-4f,
                 "log");
  require_close_(x_mps.sqrt().to(pluss::pCPU).to_vec<float>(),
                 x_cpu.sqrt().to_vec<float>(),
                 4e-4f,
                 "sqrt");
  require_close_(x_mps.sin().to(pluss::pCPU).to_vec<float>(), x_cpu.sin().to_vec<float>(), 3e-4f, "sin");
  require_close_(x_mps.cos().to(pluss::pCPU).to_vec<float>(), x_cpu.cos().to_vec<float>(), 3e-4f, "cos");
  require_close_(x_mps.tanh().to(pluss::pCPU).to_vec<float>(), x_cpu.tanh().to_vec<float>(), 3e-4f, "tanh");
  require_close_(x_mps.log1p().to(pluss::pCPU).to_vec<float>(),
                 x_cpu.log1p().to_vec<float>(),
                 4e-4f,
                 "log1p");
  require_close_(x_mps.expm1().to(pluss::pCPU).to_vec<float>(),
                 x_cpu.expm1().to_vec<float>(),
                 4e-4f,
                 "expm1");
  require_close_(x_mps.erf().to(pluss::pCPU).to_vec<float>(), x_cpu.erf().to_vec<float>(), 8e-4f, "erf");
  require_close_(x_mps.erfc().to(pluss::pCPU).to_vec<float>(), x_cpu.erfc().to_vec<float>(), 8e-4f, "erfc");
  require_close_(x_mps.asinh().to(pluss::pCPU).to_vec<float>(),
                 x_cpu.asinh().to_vec<float>(),
                 6e-4f,
                 "asinh");
  require_close_(
      x_mps.add(2.6).acosh().to(pluss::pCPU).to_vec<float>(),
      x_cpu.add(2.6).acosh().to_vec<float>(),
      6e-4f,
      "acosh");
  require_close_(
      x_mps.div(3.0).atanh().to(pluss::pCPU).to_vec<float>(),
      x_cpu.div(3.0).atanh().to_vec<float>(),
      6e-4f,
      "atanh");
  require_close_(x_mps.nan_to_num(0.0, 100.0, -100.0).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.nan_to_num(0.0, 100.0, -100.0).to_vec<float>(),
                 2e-4f,
                 "nan_to_num");

  require_close_(x_mps.add(y_mps).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.add(y_cpu).to_vec<float>(),
                 2e-4f,
                 "add");
  require_close_(x_mps.sub(y_mps).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.sub(y_cpu).to_vec<float>(),
                 2e-4f,
                 "sub");
  require_close_(x_mps.mul(y_mps).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.mul(y_cpu).to_vec<float>(),
                 2e-4f,
                 "mul");
  require_close_(x_mps.div(y_mps.add(3.0)).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.div(y_cpu.add(3.0)).to_vec<float>(),
                 3e-4f,
                 "div");
  require_close_(x_mps.pow(2.0).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.pow(2.0).to_vec<float>(),
                 3e-4f,
                 "pow");
  require_close_(x_mps.minimum(y_mps).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.minimum(y_cpu).to_vec<float>(),
                 2e-4f,
                 "minimum");
  require_close_(x_mps.maximum(y_mps).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.maximum(y_cpu).to_vec<float>(),
                 2e-4f,
                 "maximum");

  require_close_(x_mps.sum().to(pluss::pCPU).to_vec<float>(), x_cpu.sum().to_vec<float>(), 2e-4f, "sum");
  require_close_(
      x_mps.sum(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<float>(),
      x_cpu.sum(/*dim=*/1, /*keepdim=*/false).to_vec<float>(),
      2e-4f,
      "sum-dim");
  require_close_(
      x_mps.mean(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<float>(),
      x_cpu.mean(/*dim=*/1, /*keepdim=*/false).to_vec<float>(),
      2e-4f,
      "mean-dim");
  require_close_(
      x_mps.amax(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<float>(),
      x_cpu.amax(/*dim=*/1, /*keepdim=*/false).to_vec<float>(),
      2e-4f,
      "amax-dim");
  require_close_(
      x_mps.amin(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<float>(),
      x_cpu.amin(/*dim=*/1, /*keepdim=*/false).to_vec<float>(),
      2e-4f,
      "amin-dim");

  require_equal_i64_(
      x_mps.gt(0.0).all(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<std::int64_t>(),
      x_cpu.gt(0.0).all(/*dim=*/1, /*keepdim=*/false).to_vec<std::int64_t>(),
      "all-dim");
  require_equal_i64_(
      x_mps.gt(0.0).any(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<std::int64_t>(),
      x_cpu.gt(0.0).any(/*dim=*/1, /*keepdim=*/false).to_vec<std::int64_t>(),
      "any-dim");
}

TEST_CASE("MPS strict torchwide indexing/selection/counting parity", "[mps][strict][torchwide][indexing]") {
  const ScopedEnvVar fallback_off("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const ScopedEnvVar native_take_put("PLUSS_MPS_NATIVE_TAKE_PUT", "1");
  const ScopedEnvVar native_sort("PLUSS_MPS_NATIVE_SORT_SELECTION", "1");
  const ScopedEnvVar native_sort_strict("PLUSS_MPS_NATIVE_SORT_SELECTION_STRICT", "1");
  const ScopedEnvVar native_topk("PLUSS_MPS_NATIVE_TOPK_SELECTION", "1");

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  const auto xf_cpu = pluss::make_series(
      std::vector<float>{1.0f, -2.0f, 3.0f, 0.2f, -4.0f, 5.0f},
      {2, 3});
  const auto xf_mps = xf_cpu.to(pluss::pMPS);

  const auto xi_cpu = pluss::make_series(std::vector<std::int32_t>{5, 2, 0, 3, 2, 5}, {2, 3});
  const auto xi_mps = xi_cpu.to(pluss::pMPS);

  const auto idx1_cpu = pluss::make_series(std::vector<std::int64_t>{5, 0, 3, 2}, {4});
  const auto idx1_mps = idx1_cpu.to(pluss::pMPS);
  require_close_(
      xf_mps.reshape({xf_mps.numel()}).take(idx1_mps).to(pluss::pCPU).to_vec<float>(),
      xf_cpu.reshape({xf_cpu.numel()}).take(idx1_cpu).to_vec<float>(),
      2e-4f,
      "take");

  const auto gather_idx_cpu =
      pluss::make_series(std::vector<std::int64_t>{2, 0, 1, 1, 2, 0}, {2, 3});
  const auto gather_idx_mps = gather_idx_cpu.to(pluss::pMPS);
  require_close_(
      xf_mps.gather(1, gather_idx_mps).to(pluss::pCPU).to_vec<float>(),
      xf_cpu.gather(1, gather_idx_cpu).to_vec<float>(),
      2e-4f,
      "gather");

  const auto scatter_base_cpu = pluss::Series::zeros({2, 3}, pluss::ScalarType::Float32);
  const auto scatter_base_mps = scatter_base_cpu.to(pluss::pMPS);
  require_close_(
      scatter_base_mps.scatter(1, gather_idx_mps, xf_mps).to(pluss::pCPU).to_vec<float>(),
      scatter_base_cpu.scatter(1, gather_idx_cpu, xf_cpu).to_vec<float>(),
      2e-4f,
      "scatter");
  require_close_(
      scatter_base_mps.scatter_add(1, gather_idx_mps, xf_mps).to(pluss::pCPU).to_vec<float>(),
      scatter_base_cpu.scatter_add(1, gather_idx_cpu, xf_cpu).to_vec<float>(),
      2e-4f,
      "scatter_add");

  require_equal_i64_(xf_mps.nonzero().to(pluss::pCPU).to_vec<std::int64_t>(),
                     xf_cpu.nonzero().to_vec<std::int64_t>(),
                     "nonzero");

  require_close_(
      xf_mps.sort(/*dim=*/1, /*descending=*/false, /*stable=*/false).to(pluss::pCPU).to_vec<float>(),
      xf_cpu.sort(/*dim=*/1, /*descending=*/false, /*stable=*/false).to_vec<float>(),
      2e-4f,
      "sort");
  require_equal_i64_(
      xf_mps.argsort(/*dim=*/1, /*descending=*/true, /*stable=*/false).to(pluss::pCPU).to_vec<std::int64_t>(),
      xf_cpu.argsort(/*dim=*/1, /*descending=*/true, /*stable=*/false).to_vec<std::int64_t>(),
      "argsort");
  require_close_(
      xf_mps.topk(/*k=*/2, /*dim=*/1, /*largest=*/true, /*sorted=*/true).to(pluss::pCPU).to_vec<float>(),
      xf_cpu.topk(/*k=*/2, /*dim=*/1, /*largest=*/true, /*sorted=*/true).to_vec<float>(),
      2e-4f,
      "topk");
  require_close_(
      xf_mps.kthvalue(/*k=*/2, /*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<float>(),
      xf_cpu.kthvalue(/*k=*/2, /*dim=*/1, /*keepdim=*/false).to_vec<float>(),
      2e-4f,
      "kthvalue");

  require_equal_i32_(
      xi_mps.unique(/*sorted=*/true).to(pluss::pCPU).to_vec<std::int32_t>(),
      xi_cpu.unique(/*sorted=*/true).to_vec<std::int32_t>(),
      "unique-i32");
  require_equal_i64_(
      xi_mps.bincount(/*minlength=*/7).to(pluss::pCPU).to_vec<std::int64_t>(),
      xi_cpu.bincount(/*minlength=*/7).to_vec<std::int64_t>(),
      "bincount-i64");
  require_equal_i64_(
      xi_mps.histc(/*bins=*/8, /*min=*/0.0, /*max=*/7.0).to(pluss::pCPU).to_vec<std::int64_t>(),
      xi_cpu.histc(/*bins=*/8, /*min=*/0.0, /*max=*/7.0).to_vec<std::int64_t>(),
      "histc-i64");

  require_equal_i32_(
      xi_mps.cumsum(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<std::int32_t>(),
      xi_cpu.cumsum(/*dim=*/1, /*keepdim=*/false).to_vec<std::int32_t>(),
      "cumsum-i32");
  require_equal_i32_(
      xi_mps.cumprod(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<std::int32_t>(),
      xi_cpu.cumprod(/*dim=*/1, /*keepdim=*/false).to_vec<std::int32_t>(),
      "cumprod-i32");
}

TEST_CASE("MPS strict torchwide linalg parity", "[mps][strict][torchwide][linalg]") {
  const ScopedEnvVar fallback_off("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const ScopedEnvVar linalg_native("PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE", "1");
  const ScopedEnvVar linalg_no_bridge("PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE", "0");
  const ScopedEnvVar linalg_route("PLUSS_MPS_LINALG_SOLVER_ROUTE", "native");
  const ScopedEnvVar linalg_svd("PLUSS_MPS_LINALG_NATIVE_DEVICE_SVD", "1");

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  using namespace pluss::subsp::linearalg;

  const auto a_cpu = pluss::make_series(std::vector<float>{4.f, 1.f, 1.f, 3.f}, {2, 2});
  const auto a_mps = a_cpu.to(pluss::pMPS);
  require_close_(det(a_mps).to(pluss::pCPU).to_vec<float>(), det(a_cpu).to_vec<float>(), 3e-3f, "det");
  require_close_(
      inverse(a_mps).to(pluss::pCPU).to_vec<float>(),
      inverse(a_cpu).to_vec<float>(),
      3e-3f,
      "inverse");
  require_close_(
      cholesky(a_mps, /*upper=*/false).to(pluss::pCPU).to_vec<float>(),
      cholesky(a_cpu, /*upper=*/false).to_vec<float>(),
      3e-3f,
      "cholesky");
  require_close_(logdet(a_mps).to(pluss::pCPU).to_vec<float>(), logdet(a_cpu).to_vec<float>(), 3e-3f, "logdet");
  require_close_(
      matrix_power(a_mps, 3).to(pluss::pCPU).to_vec<float>(),
      matrix_power(a_cpu, 3).to_vec<float>(),
      4e-3f,
      "matrix_power");
  require_close_(
      matrix_exp(a_mps).to(pluss::pCPU).to_vec<float>(),
      matrix_exp(a_cpu).to_vec<float>(),
      5e-3f,
      "matrix_exp");
  require_close_(
      pinverse(a_mps).to(pluss::pCPU).to_vec<float>(),
      pinverse(a_cpu).to_vec<float>(),
      5e-3f,
      "pinverse");

  const auto b_cpu = pluss::make_series(std::vector<float>{2.f, -1.f}, {2, 1});
  const auto b_mps = b_cpu.to(pluss::pMPS);
  require_close_(
      triangular_solve(a_mps, b_mps, /*upper=*/true, /*unitriangular=*/false).to(pluss::pCPU).to_vec<float>(),
      triangular_solve(a_cpu, b_cpu, /*upper=*/true, /*unitriangular=*/false).to_vec<float>(),
      6e-3f,
      "triangular_solve");

  const auto c_cpu = pluss::make_series(
      std::vector<float>{4.f, 1.f, 1.f, 3.f, 3.f, 0.5f, 0.5f, 2.f},
      {2, 2, 2});
  const auto c_mps = c_cpu.to(pluss::pMPS);
  require_close_(
      qr_q(c_mps, /*reduced=*/true).abs().to(pluss::pCPU).to_vec<float>(),
      qr_q(c_cpu, /*reduced=*/true).abs().to_vec<float>(),
      7e-3f,
      "batched-qr_q");
  require_close_(
      svd_s(c_mps, /*full_matrices=*/false).to(pluss::pCPU).to_vec<float>(),
      svd_s(c_cpu, /*full_matrices=*/false).to_vec<float>(),
      7e-3f,
      "batched-svd_s");
}

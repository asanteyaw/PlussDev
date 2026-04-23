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

TEST_CASE("MPS strict torchwide extended float32 parity", "[mps][strict][torchwide][extended][float32]") {
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
      std::vector<float>{1.0f, -2.0f, 3.0f, 0.2f, -4.0f, 5.0f},
      {2, 3});
  const auto y_cpu = pluss::make_series(
      std::vector<float>{0.5f, 2.0f, -1.5f, 4.0f, 3.0f, -2.0f},
      {2, 3});
  const auto x_mps = x_cpu.to(pluss::pMPS);
  const auto y_mps = y_cpu.to(pluss::pMPS);

  require_close_(x_mps.transpose(0, 1).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.transpose(0, 1).to_vec<float>(),
                 2e-4f,
                 "transpose");
  require_close_(x_mps.permute({1, 0}).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.permute({1, 0}).to_vec<float>(),
                 2e-4f,
                 "permute");
  require_close_(x_mps.unsqueeze(0).squeeze(0).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.unsqueeze(0).squeeze(0).to_vec<float>(),
                 2e-4f,
                 "unsqueeze-squeeze");
  require_close_(x_mps.flatten(0, 1).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.flatten(0, 1).to_vec<float>(),
                 2e-4f,
                 "flatten");
  require_close_(x_mps.unite({y_mps}, 1).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.unite({y_cpu}, 1).to_vec<float>(),
                 2e-4f,
                 "unite");
  require_close_(x_mps.pile({y_mps}, 0).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.pile({y_cpu}, 0).to_vec<float>(),
                 2e-4f,
                 "pile");

  require_close_(x_mps.where(x_mps.gt(0.0), y_mps).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.where(x_cpu.gt(0.0), y_cpu).to_vec<float>(),
                 3e-4f,
                 "where");
  require_close_(x_mps.clamp_min(-1.0).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.clamp_min(-1.0).to_vec<float>(),
                 2e-4f,
                 "clamp_min");
  require_close_(x_mps.clamp_max(2.0).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.clamp_max(2.0).to_vec<float>(),
                 2e-4f,
                 "clamp_max");

  const auto mask_cpu = x_cpu.gt(0.0);
  const auto mask_mps = mask_cpu.to(pluss::pMPS);
  const auto src_cpu = pluss::make_series(std::vector<float>{9.0f, 8.0f, 7.0f, 6.0f}, {4});
  const auto src_mps = src_cpu.to(pluss::pMPS);
  require_close_(x_mps.masked_select(mask_mps).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.masked_select(mask_cpu).to_vec<float>(),
                 2e-4f,
                 "masked_select");
  require_close_(x_mps.masked_fill(mask_mps, 1.25).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.masked_fill(mask_cpu, 1.25).to_vec<float>(),
                 2e-4f,
                 "masked_fill");
  require_close_(x_mps.masked_scatter(mask_mps, src_mps).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.masked_scatter(mask_cpu, src_cpu).to_vec<float>(),
                 2e-4f,
                 "masked_scatter");

  const auto base_cpu = pluss::Series::zeros({2, 4}, pluss::ScalarType::Float32);
  const auto base_mps = base_cpu.to(pluss::pMPS);
  const auto idx_cpu = pluss::make_series(std::vector<std::int64_t>{0, 1, 2, 3, 3, 2, 1, 0}, {2, 4});
  const auto idx_mps = idx_cpu.to(pluss::pMPS);
  const auto red_src_cpu = pluss::make_series(
      std::vector<float>{1.0f, -2.0f, 3.0f, 4.0f, -1.0f, 2.0f, -3.0f, 5.0f},
      {2, 4});
  const auto red_src_mps = red_src_cpu.to(pluss::pMPS);
  require_close_(
      base_mps.scatter_reduce_sum(1, idx_mps, red_src_mps, /*include_self=*/false).to(pluss::pCPU).to_vec<float>(),
      base_cpu.scatter_reduce_sum(1, idx_cpu, red_src_cpu, /*include_self=*/false).to_vec<float>(),
      3e-4f,
      "scatter_reduce_sum");
  require_close_(
      base_mps.scatter_reduce_amax(1, idx_mps, red_src_mps, /*include_self=*/false, /*initial=*/-100.0)
          .to(pluss::pCPU)
          .to_vec<float>(),
      base_cpu.scatter_reduce_amax(1, idx_cpu, red_src_cpu, /*include_self=*/false, /*initial=*/-100.0)
          .to_vec<float>(),
      3e-4f,
      "scatter_reduce_amax");
  require_close_(
      base_mps.scatter_reduce_amin(1, idx_mps, red_src_mps, /*include_self=*/false, /*initial=*/100.0)
          .to(pluss::pCPU)
          .to_vec<float>(),
      base_cpu.scatter_reduce_amin(1, idx_cpu, red_src_cpu, /*include_self=*/false, /*initial=*/100.0)
          .to_vec<float>(),
      3e-4f,
      "scatter_reduce_amin");

  require_close_(x_mps.var(/*unbiased=*/false).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.var(/*unbiased=*/false).to_vec<float>(),
                 4e-4f,
                 "var");
  require_close_(x_mps.std(/*unbiased=*/false).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.std(/*unbiased=*/false).to_vec<float>(),
                 4e-4f,
                 "std");
  require_close_(x_mps.norm(2.0).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.norm(2.0).to_vec<float>(),
                 4e-4f,
                 "norm");
}

TEST_CASE("MPS strict torchwide extended int32 parity", "[mps][strict][torchwide][extended][int32]") {
  const ScopedEnvVar fallback_off("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const ScopedEnvVar native_take_put("PLUSS_MPS_NATIVE_TAKE_PUT", "1");
  const ScopedEnvVar native_sort("PLUSS_MPS_NATIVE_SORT_SELECTION", "1");
  const ScopedEnvVar native_sort_strict("PLUSS_MPS_NATIVE_SORT_SELECTION_STRICT", "1");

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  const auto x_cpu = pluss::make_series(std::vector<std::int32_t>{5, -2, 3, 1, -4, 2}, {2, 3});
  const auto y_cpu = pluss::make_series(std::vector<std::int32_t>{1, 4, -3, 2, 0, -1}, {2, 3});
  const auto x_mps = x_cpu.to(pluss::pMPS);
  const auto y_mps = y_cpu.to(pluss::pMPS);

  require_equal_i32_(x_mps.bitwise_and(y_mps).to(pluss::pCPU).to_vec<std::int32_t>(),
                     x_cpu.bitwise_and(y_cpu).to_vec<std::int32_t>(),
                     "bitwise_and");
  require_equal_i32_(x_mps.bitwise_or(y_mps).to(pluss::pCPU).to_vec<std::int32_t>(),
                     x_cpu.bitwise_or(y_cpu).to_vec<std::int32_t>(),
                     "bitwise_or");
  require_equal_i32_(x_mps.bitwise_xor(y_mps).to(pluss::pCPU).to_vec<std::int32_t>(),
                     x_cpu.bitwise_xor(y_cpu).to_vec<std::int32_t>(),
                     "bitwise_xor");
  require_equal_i32_(x_mps.bitwise_not().to(pluss::pCPU).to_vec<std::int32_t>(),
                     x_cpu.bitwise_not().to_vec<std::int32_t>(),
                     "bitwise_not");

  require_equal_i32_(x_mps.sum(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<std::int32_t>(),
                     x_cpu.sum(/*dim=*/1, /*keepdim=*/false).to_vec<std::int32_t>(),
                     "sum-dim-i32");
  require_equal_i32_(x_mps.prod(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<std::int32_t>(),
                     x_cpu.prod(/*dim=*/1, /*keepdim=*/false).to_vec<std::int32_t>(),
                     "prod-dim-i32");
  require_equal_i32_(x_mps.amax(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<std::int32_t>(),
                     x_cpu.amax(/*dim=*/1, /*keepdim=*/false).to_vec<std::int32_t>(),
                     "amax-dim-i32");
  require_equal_i32_(x_mps.amin(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<std::int32_t>(),
                     x_cpu.amin(/*dim=*/1, /*keepdim=*/false).to_vec<std::int32_t>(),
                     "amin-dim-i32");
  require_equal_i64_(x_mps.argmax(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<std::int64_t>(),
                     x_cpu.argmax(/*dim=*/1, /*keepdim=*/false).to_vec<std::int64_t>(),
                     "argmax-dim-i32");
  require_equal_i64_(x_mps.argmin(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<std::int64_t>(),
                     x_cpu.argmin(/*dim=*/1, /*keepdim=*/false).to_vec<std::int64_t>(),
                     "argmin-dim-i32");

  require_close_(x_mps.mean().to(pluss::pCPU).to_vec<float>(),
                 x_cpu.mean().to_vec<float>(),
                 3e-4f,
                 "mean-i32");
  require_close_(x_mps.mean(/*dim=*/1, /*keepdim=*/false).to(pluss::pCPU).to_vec<float>(),
                 x_cpu.mean(/*dim=*/1, /*keepdim=*/false).to_vec<float>(),
                 3e-4f,
                 "mean-dim-i32");

  const auto cond_cpu = x_cpu.gt(0);
  const auto cond_mps = cond_cpu.to(pluss::pMPS);
  require_equal_i32_(x_mps.where(cond_mps, y_mps).to(pluss::pCPU).to_vec<std::int32_t>(),
                     x_cpu.where(cond_cpu, y_cpu).to_vec<std::int32_t>(),
                     "where-i32");
  require_equal_i32_(x_mps.clamp_min(-1.0).to(pluss::pCPU).to_vec<std::int32_t>(),
                     x_cpu.clamp_min(-1.0).to_vec<std::int32_t>(),
                     "clamp_min-i32");
  require_equal_i32_(x_mps.clamp_max(2.0).to(pluss::pCPU).to_vec<std::int32_t>(),
                     x_cpu.clamp_max(2.0).to_vec<std::int32_t>(),
                     "clamp_max-i32");
}

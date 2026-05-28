#include <pluss/ai.h>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

class ScopedEnvVar final {
public:
  ScopedEnvVar(const char* key, const char* value) : key_(key) {
    if (const char* old = std::getenv(key); old != nullptr) {
      had_old_ = true;
      old_ = old;
    }
    setenv(key, value, 1);
  }

  ~ScopedEnvVar() {
    if (had_old_) {
      setenv(key_.c_str(), old_.c_str(), 1);
    } else {
      unsetenv(key_.c_str());
    }
  }

private:
  std::string key_;
  std::string old_;
  bool had_old_{false};
};

std::vector<float> vec_f32(const pluss::Series& x) {
  return x.to(pluss::pCPU, pluss::ScalarType::Float32).reshape({x.numel()}).to_vec<float>();
}

bool all_close(const std::vector<float>& got, const std::vector<float>& want, float tol) {
  if (got.size() != want.size()) return false;
  for (std::size_t i = 0; i < got.size(); ++i) {
    if (std::fabs(got[i] - want[i]) > tol) return false;
  }
  return true;
}

bool all_zero(const pluss::Series& x) {
  for (const float v : vec_f32(x)) {
    if (std::fabs(v) > 1e-7f) return false;
  }
  return true;
}

bool run_dropout_parity_checks(std::ostream* err = nullptr) {
  const pluss::Config cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<float>{-1.0f, 0.5f, 2.0f, 4.0f}, {2, 2}, cfg);

  const pluss::Series dropped_all = pluss::dropout(x, 1.0, true, 7);
  if (dropped_all.platform() != x.platform() || dropped_all.scalar_type() != x.scalar_type() || !all_zero(dropped_all)) {
    if (err) *err << "dropout p=1 should return same-platform zeros\n";
    return false;
  }

  const pluss::Series eval = pluss::dropout(x, 1.0, false, 7);
  if (!all_close(vec_f32(eval), vec_f32(x), 0.0f)) {
    if (err) *err << "dropout eval path should return input values even when p=1\n";
    return false;
  }

  const auto [attn_out, attn_w] = pluss::scaled_attention(
      x,
      x,
      x,
      pluss::Series(),
      pluss::AttentionOptions{.dropout = 1.0, .training = true, .causal = false, .scale = 0.0, .seed = 11});
  if (!all_zero(attn_out) || !all_zero(attn_w)) {
    if (err) *err << "scaled_attention dropout p=1 should zero weights and output\n";
    return false;
  }

  if (pluss::mps::is_ready()) {
    const ScopedEnvVar no_fallback("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
    const pluss::Series xm = x.to(pluss::pMPS);
    const pluss::Series zm = pluss::dropout(xm, 1.0, true, 13);
    if (zm.platform() != pluss::pMPS || zm.scalar_type() != xm.scalar_type() || !all_zero(zm)) {
      if (err) *err << "MPS dropout p=1 should stay on MPS and zero values\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("NodeK dropout accepts p=1 and keeps MPS tensors on device", "[nn][dropout][mps][parity]") {
  REQUIRE(run_dropout_parity_checks(&std::cerr));
}
#else
int main() {
  if (!run_dropout_parity_checks(&std::cerr)) return 1;
  std::cout << "nn_dropout_parity_test passed\n";
  return 0;
}
#endif

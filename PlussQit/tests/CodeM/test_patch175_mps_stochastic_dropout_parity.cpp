#include <catch2/catch_test_macros.hpp>

#include <pluss/core.h>
#include "CodeM/core/Complex.h"
#include "CodeM/compat/Invoke.h"
#include "CodeM/native/mps/Stochastic.h"
#include "pluss/subsp/linearalg/linearalg.h"

#include <array>
#include <cstdlib>
#include <cmath>
#include <complex>
#include <cstdint>
#include <string>
#include <vector>

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

void require_feature_groups_constant(const pluss::Series& x) {
  REQUIRE(x.sizes() == std::vector<std::int64_t>{2, 3, 4});
  const std::vector<float> v = vec_f32(x);
  for (std::int64_t b = 0; b < 2; ++b) {
    for (std::int64_t c = 0; c < 3; ++c) {
      const float first = v[static_cast<std::size_t>((b * 3 + c) * 4)];
      for (std::int64_t r = 1; r < 4; ++r) {
        REQUIRE(v[static_cast<std::size_t>((b * 3 + c) * 4 + r)] == first);
      }
    }
  }
}

void require_close(const std::vector<float>& actual, const std::vector<float>& expected, float tol = 1.0e-5f) {
  REQUIRE(actual.size() == expected.size());
  for (std::size_t i = 0; i < actual.size(); ++i) {
    REQUIRE(std::abs(actual[i] - expected[i]) <= tol);
  }
}

void require_close_complex64(const std::vector<pluss::codem::Complex64>& actual,
                             const std::vector<pluss::codem::Complex64>& expected,
                             float tol = 1.0e-5f) {
  REQUIRE(actual.size() == expected.size());
  for (std::size_t i = 0; i < actual.size(); ++i) {
    REQUIRE(std::abs(actual[i].real() - expected[i].real()) <= tol);
    REQUIRE(std::abs(actual[i].imag() - expected[i].imag()) <= tol);
  }
}

} // namespace

TEST_CASE("MPS stochastic dropout and random-like ops stay on device", "[mps][stochastic][dropout][parity]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable");
    return;
  }

  const ScopedEnvVar no_fallback("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const pluss::Config cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pMPS);
  const pluss::Series x = pluss::make_series(std::vector<float>{-1.0f, 0.5f, 2.0f, 4.0f}, {4}, cfg);

  const pluss::Series dropped = pluss::codem::_compat::invoke("dropout", x, {0.25, true});
  REQUIRE(dropped.platform() == pluss::pMPS);
  REQUIRE(dropped.scalar_type() == pluss::ScalarType::Float32);
  REQUIRE(dropped.sizes() == x.sizes());

  pluss::Series uniform = x.clone();
  uniform.uniform_(-0.5, 0.5);
  REQUIRE(uniform.platform() == pluss::pMPS);
  REQUIRE(uniform.scalar_type() == pluss::ScalarType::Float32);
  for (const float v : vec_f32(uniform)) {
    REQUIRE(v >= -0.5f);
    REQUIRE(v <= 0.5f);
  }

  pluss::Series normal = x.clone();
  normal.normal_(1.0, 0.25);
  REQUIRE(normal.platform() == pluss::pMPS);
  REQUIRE(normal.scalar_type() == pluss::ScalarType::Float32);
  REQUIRE(normal.sizes() == x.sizes());

  pluss::Series bernoulli = x.clone();
  bernoulli.bernoulli_(0.5);
  REQUIRE(bernoulli.platform() == pluss::pMPS);
  REQUIRE(bernoulli.scalar_type() == pluss::ScalarType::Float32);
  for (const float v : vec_f32(bernoulli)) {
    REQUIRE((v == 0.0f || v == 1.0f));
  }

  pluss::Series ints = pluss::series::zeros({6}, pluss::Config{}.with_dtype(pluss::ScalarType::Int32).with_platform(pluss::pMPS));
  ints.random_(2.0, 7.0);
  REQUIRE(ints.platform() == pluss::pMPS);
  REQUIRE(ints.scalar_type() == pluss::ScalarType::Int32);
  for (const float v : vec_f32(ints)) {
    REQUIRE(v >= 2.0f);
    REQUIRE(v < 7.0f);
  }
}

TEST_CASE("MPSGraph native stochastic generators produce device tensors", "[mps][stochastic][native]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable");
    return;
  }

  REQUIRE(pluss::codem::native::mps::supports_uniform_stochastic(pluss::ScalarType::Float32));
  REQUIRE(pluss::codem::native::mps::supports_normal_stochastic(pluss::ScalarType::Float32));

  const pluss::Series uniform =
      pluss::codem::native::mps::uniform_stochastic({16}, pluss::ScalarType::Float32, -0.5, 0.5, 123);
  REQUIRE(uniform.platform() == pluss::pMPS);
  REQUIRE(uniform.scalar_type() == pluss::ScalarType::Float32);
  REQUIRE(uniform.sizes() == std::vector<std::int64_t>{16});
  for (const float v : vec_f32(uniform)) {
    REQUIRE(v >= -0.5f);
    REQUIRE(v < 0.5f);
  }

  const pluss::Series uniform_again =
      pluss::codem::native::mps::uniform_stochastic({16}, pluss::ScalarType::Float32, -0.5, 0.5, 123);
  REQUIRE(vec_f32(uniform_again) == vec_f32(uniform));

  const pluss::Series normal =
      pluss::codem::native::mps::normal_stochastic({16}, pluss::ScalarType::Float32, 1.0, 0.25, 321);
  REQUIRE(normal.platform() == pluss::pMPS);
  REQUIRE(normal.scalar_type() == pluss::ScalarType::Float32);
  REQUIRE(normal.sizes() == std::vector<std::int64_t>{16});
}

TEST_CASE("MPS public stochastic factories use native device generation", "[mps][stochastic][factory]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable");
    return;
  }

  const ScopedEnvVar no_fallback("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const pluss::Config cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pMPS);

  const pluss::Series uniform = pluss::series::rand({8}, cfg, 777);
  REQUIRE(uniform.platform() == pluss::pMPS);
  REQUIRE(uniform.scalar_type() == pluss::ScalarType::Float32);
  for (const float v : vec_f32(uniform)) {
    REQUIRE(v >= 0.0f);
    REQUIRE(v < 1.0f);
  }

  const pluss::Series uniform_again = pluss::series::rand({8}, cfg, 777);
  REQUIRE(vec_f32(uniform_again) == vec_f32(uniform));

  const pluss::Series normal = pluss::series::randn({8}, cfg, 778);
  REQUIRE(normal.platform() == pluss::pMPS);
  REQUIRE(normal.scalar_type() == pluss::ScalarType::Float32);

  const pluss::Series shifted = pluss::series::normal(2.0, 0.5, {8}, cfg, 779);
  REQUIRE(shifted.platform() == pluss::pMPS);
  REQUIRE(shifted.scalar_type() == pluss::ScalarType::Float32);

  const pluss::Series ints =
      pluss::series::randint(3, 9, {8}, pluss::Config{}.with_dtype(pluss::ScalarType::Int32).with_platform(pluss::pMPS), 780);
  REQUIRE(ints.platform() == pluss::pMPS);
  REQUIRE(ints.scalar_type() == pluss::ScalarType::Int32);
  for (const float v : vec_f32(ints)) {
    REQUIRE(v >= 3.0f);
    REQUIRE(v < 9.0f);
  }

  const pluss::Series bernoulli = pluss::series::bernoulli({8}, 0.5, cfg, 781);
  REQUIRE(bernoulli.platform() == pluss::pMPS);
  REQUIRE(bernoulli.scalar_type() == pluss::ScalarType::Bool);
}

TEST_CASE("MPS bernoulli from probability tensor returns an MPS mask", "[mps][stochastic][bernoulli]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable");
    return;
  }

  const ScopedEnvVar no_fallback("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const pluss::Config cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pMPS);
  const pluss::Series probs = pluss::make_series(std::vector<float>{0.0f, 0.25f, 0.75f, 1.0f}, {4}, cfg);
  const pluss::Series mask = pluss::codem::_compat::invoke("bernoulli", probs);
  REQUIRE(mask.platform() == pluss::pMPS);
  // torch.bernoulli(input) preserves input's own dtype (0.0/1.0 values),
  // it does not return a Bool tensor -- see test_random_sampling_family.cpp
  // for the same contract verified independently on CPU.
  REQUIRE(mask.scalar_type() == probs.scalar_type());
  REQUIRE(mask.sizes() == probs.sizes());
}

TEST_CASE("MPS dropout family matches PyTorch dropout variant contracts", "[mps][stochastic][dropout][feature][alpha]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable");
    return;
  }

  const ScopedEnvVar no_fallback("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const pluss::Config cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pMPS);
  const std::vector<std::int64_t> shape{2, 3, 4};
  const pluss::Series x = pluss::series::ones(shape, cfg);

  const pluss::Series feature = pluss::codem::_compat::invoke("feature_dropout", x, {0.5, true});
  REQUIRE(feature.platform() == pluss::pMPS);
  REQUIRE(feature.scalar_type() == pluss::ScalarType::Float32);
  require_feature_groups_constant(feature);
  for (const float v : vec_f32(feature)) {
    REQUIRE((v == 0.0f || v == 2.0f));
  }

  const pluss::Series alpha = pluss::codem::_compat::invoke("alpha_dropout", x, {0.5, true});
  REQUIRE(alpha.platform() == pluss::pMPS);
  REQUIRE(alpha.scalar_type() == pluss::ScalarType::Float32);
  REQUIRE(alpha.sizes() == x.sizes());
  bool alpha_changed = false;
  for (const float v : vec_f32(alpha)) {
    REQUIRE(std::isfinite(v));
    alpha_changed = alpha_changed || std::abs(v - 1.0f) > 1.0e-5f;
  }
  REQUIRE(alpha_changed);

  const pluss::Series feature_alpha = pluss::codem::_compat::invoke("feature_alpha_dropout", x, {0.5, true});
  REQUIRE(feature_alpha.platform() == pluss::pMPS);
  REQUIRE(feature_alpha.scalar_type() == pluss::ScalarType::Float32);
  require_feature_groups_constant(feature_alpha);

  const pluss::Series dropped_all = pluss::codem::_compat::invoke("feature_alpha_dropout", x, {1.0, true});
  REQUIRE(dropped_all.platform() == pluss::pMPS);
  for (const float v : vec_f32(dropped_all)) {
    REQUIRE(v == 0.0f);
  }

  const pluss::Series eval = pluss::codem::_compat::invoke("alpha_dropout", x, {0.75, false});
  REQUIRE(eval.platform() == pluss::pMPS);
  REQUIRE(vec_f32(eval) == vec_f32(x));

  const pluss::Series bf16 = x.to(pluss::ScalarType::BFloat16);
  REQUIRE(bf16.platform() == pluss::pMPS);
  REQUIRE(bf16.scalar_type() == pluss::ScalarType::BFloat16);
  const pluss::Series bf16_back = bf16.to(pluss::ScalarType::Float32);
  REQUIRE(bf16_back.platform() == pluss::pMPS);
  REQUIRE(vec_f32(bf16_back) == vec_f32(x));

  REQUIRE_THROWS(x.to(pluss::ScalarType::UInt32));
  {
    const ScopedEnvVar allow_fallback("PLUSS_MPS_ALLOW_CPU_FALLBACK", "1");
    const pluss::Series bridged = x.to(pluss::ScalarType::UInt32);
    REQUIRE(bridged.platform() == pluss::pMPS);
    REQUIRE(bridged.scalar_type() == pluss::ScalarType::UInt32);
  }

  const pluss::Series i16 =
      pluss::make_series(std::vector<std::int16_t>{-2, 0, 7, 9},
                         {4},
                         pluss::Config{}.with_dtype(pluss::ScalarType::Int16).with_platform(pluss::pMPS));
  REQUIRE(i16.to(pluss::ScalarType::Float32).platform() == pluss::pMPS);
  REQUIRE(vec_f32(i16.to(pluss::ScalarType::Float32)) == std::vector<float>{-2.0f, 0.0f, 7.0f, 9.0f});

  const pluss::Series u8 =
      pluss::make_series(std::vector<std::uint8_t>{0, 2, 255, 17},
                         {4},
                         pluss::Config{}.with_dtype(pluss::ScalarType::UInt8).with_platform(pluss::pMPS));
  REQUIRE(u8.to(pluss::ScalarType::Float32).platform() == pluss::pMPS);
  REQUIRE(vec_f32(u8.to(pluss::ScalarType::Float32)) == std::vector<float>{0.0f, 2.0f, 255.0f, 17.0f});

  const pluss::Series index =
      pluss::make_series(std::vector<std::int64_t>{3, 0, 2},
                         {3},
                         pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(pluss::pMPS));
  const pluss::Series taken_i16 = i16.take(index);
  REQUIRE(taken_i16.platform() == pluss::pMPS);
  REQUIRE(taken_i16.scalar_type() == pluss::ScalarType::Int16);
  REQUIRE(taken_i16.to(pluss::pCPU, pluss::ScalarType::Int16).to_vec<std::int16_t>() ==
          std::vector<std::int16_t>{9, -2, 7});

  const pluss::Series taken_u8 = u8.take(index);
  REQUIRE(taken_u8.platform() == pluss::pMPS);
  REQUIRE(taken_u8.scalar_type() == pluss::ScalarType::UInt8);
  REQUIRE(vec_f32(taken_u8.to(pluss::ScalarType::Float32)) == std::vector<float>{17.0f, 0.0f, 255.0f});

  const pluss::Series rank1 = pluss::series::ones(std::vector<std::int64_t>{4}, cfg);
  REQUIRE_THROWS(pluss::codem::_compat::invoke("feature_dropout", rank1, {0.5, true}));

  pluss::Series inplace = x.clone();
  pluss::codem::_compat::invoke_("feature_alpha_dropout_", inplace, {0.5, true});
  REQUIRE(inplace.platform() == pluss::pMPS);
  REQUIRE(inplace.scalar_type() == pluss::ScalarType::Float32);
  require_feature_groups_constant(inplace);
}

TEST_CASE("MPS reduction and scan backward paths stay device-native under strict fallback", "[mps][autodiff][reduction][scan]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable");
    return;
  }

  const ScopedEnvVar no_fallback("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const pluss::Config cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pMPS);

  pluss::Series x = pluss::make_series(std::vector<float>{1.0f, 5.0f, 5.0f, 2.0f, -1.0f, 2.0f}, {2, 3}, cfg);
  x.tracks_slope_(true);
  x.amax(1, false).sum().reverse();
  REQUIRE(x.sensitivity().platform() == pluss::pMPS);
  require_close(vec_f32(x.sensitivity()), {0.0f, 0.5f, 0.5f, 0.5f, 0.0f, 0.5f});

  x.clear();
  x.amin(0, false).sum().reverse();
  REQUIRE(x.sensitivity().platform() == pluss::pMPS);
  require_close(vec_f32(x.sensitivity()), {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f});

  pluss::Series scan = pluss::make_series(std::vector<float>{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f}, {2, 3}, cfg);
  scan.tracks_slope_(true);
  scan.cumsum(1).sum().reverse();
  REQUIRE(scan.sensitivity().platform() == pluss::pMPS);
  require_close(vec_f32(scan.sensitivity()), {3.0f, 2.0f, 1.0f, 3.0f, 2.0f, 1.0f});
}

TEST_CASE("MPS indexing and where backward paths stay device-native under strict fallback", "[mps][autodiff][indexing][where]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable");
    return;
  }

  const ScopedEnvVar no_fallback("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const pluss::Config cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pMPS);
  const pluss::Config idx_cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(pluss::pMPS);
  const pluss::Config mask_cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Bool).with_platform(pluss::pMPS);

  pluss::Series a = pluss::make_series(std::vector<float>{1.0f, 2.0f, 3.0f, 4.0f}, {4}, cfg);
  pluss::Series b = pluss::make_series(std::vector<float>{5.0f, 6.0f, 7.0f, 8.0f}, {4}, cfg);
  const pluss::Series mask = pluss::make_series(std::vector<bool>{true, false, true, false}, {4}, mask_cfg);
  a.tracks_slope_(true);
  b.tracks_slope_(true);
  mask.where(a, b).sum().reverse();
  REQUIRE(a.sensitivity().platform() == pluss::pMPS);
  REQUIRE(b.sensitivity().platform() == pluss::pMPS);
  require_close(vec_f32(a.sensitivity()), {1.0f, 0.0f, 1.0f, 0.0f});
  require_close(vec_f32(b.sensitivity()), {0.0f, 1.0f, 0.0f, 1.0f});

  pluss::Series take_src = pluss::make_series(std::vector<float>{1.0f, 2.0f, 3.0f, 4.0f}, {4}, cfg);
  const pluss::Series take_idx = pluss::make_series(std::vector<std::int64_t>{3, 0, 3, 1}, {4}, idx_cfg);
  take_src.tracks_slope_(true);
  take_src.take(take_idx).sum().reverse();
  REQUIRE(take_src.sensitivity().platform() == pluss::pMPS);
  require_close(vec_f32(take_src.sensitivity()), {1.0f, 1.0f, 0.0f, 2.0f});

  pluss::Series sliced = pluss::make_series(
      std::vector<float>{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f},
      {2, 4},
      cfg);
  sliced.tracks_slope_(true);
  sliced.slice(1, 1, 4, 2).sum().reverse();
  REQUIRE(sliced.sensitivity().platform() == pluss::pMPS);
  require_close(vec_f32(sliced.sensitivity()), {0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f});
}

TEST_CASE("MPS linalg determinant-family backward paths stay device-native under strict fallback",
          "[mps][autodiff][linalg]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable");
    return;
  }

  const ScopedEnvVar no_fallback("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const ScopedEnvVar native_linalg("PLUSS_MPS_EXPERIMENTAL_NATIVE_LINALG", "1");
  const pluss::Config cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pMPS);
  const std::vector<float> values{2.0f, 0.5f, 0.25f, 1.5f};

  pluss::Series inv_input = pluss::make_series(values, {2, 2}, cfg);
  inv_input.tracks_slope_(true);
  pluss::subsp::linearalg::inverse(inv_input).sum().reverse();
  REQUIRE(inv_input.sensitivity().platform() == pluss::pMPS);
  require_close(
      vec_f32(inv_input.sensitivity()),
      {-0.15122873f, -0.26465028f, -0.18147448f, -0.31758034f},
      5.0e-5f);

  pluss::Series det_input = pluss::make_series(values, {2, 2}, cfg);
  det_input.tracks_slope_(true);
  pluss::subsp::linearalg::det(det_input).reverse();
  REQUIRE(det_input.sensitivity().platform() == pluss::pMPS);
  require_close(vec_f32(det_input.sensitivity()), {1.5f, -0.25f, -0.5f, 2.0f}, 5.0e-5f);

  pluss::Series singular_det_input = pluss::make_series(std::vector<float>{1.0f, 2.0f, 2.0f, 4.0f}, {2, 2}, cfg);
  singular_det_input.tracks_slope_(true);
  pluss::subsp::linearalg::det(singular_det_input).reverse();
  REQUIRE(singular_det_input.sensitivity().platform() == pluss::pMPS);
  require_close(vec_f32(singular_det_input.sensitivity()), {4.0f, -2.0f, -2.0f, 1.0f}, 5.0e-5f);

  pluss::Series logdet_input = pluss::make_series(values, {2, 2}, cfg);
  logdet_input.tracks_slope_(true);
  pluss::subsp::linearalg::logdet(logdet_input).reverse();
  REQUIRE(logdet_input.sensitivity().platform() == pluss::pMPS);
  require_close(
      vec_f32(logdet_input.sensitivity()),
      {0.52173913f, -0.08695652f, -0.17391304f, 0.69565217f},
      5.0e-5f);

  pluss::Series slogdet_input = pluss::make_series(values, {2, 2}, cfg);
  slogdet_input.tracks_slope_(true);
  pluss::subsp::linearalg::slogdet(slogdet_input).select(0, 1).reverse();
  REQUIRE(slogdet_input.sensitivity().platform() == pluss::pMPS);
  require_close(
      vec_f32(slogdet_input.sensitivity()),
      {0.52173913f, -0.08695652f, -0.17391304f, 0.69565217f},
      5.0e-5f);
}

TEST_CASE("MPS solver backward paths stay device-native under strict fallback", "[mps][autodiff][linalg][solve]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable");
    return;
  }

  const ScopedEnvVar no_fallback("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const ScopedEnvVar native_linalg("PLUSS_MPS_EXPERIMENTAL_NATIVE_LINALG", "1");
  const pluss::Config cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pMPS);

  pluss::Series tri = pluss::make_series(std::vector<float>{2.0f, 0.0f, 0.5f, 1.5f}, {2, 2}, cfg);
  pluss::Series rhs = pluss::make_series(std::vector<float>{1.0f, 2.0f}, {2, 1}, cfg);
  tri.tracks_slope_(true);
  rhs.tracks_slope_(true);
  pluss::subsp::linearalg::triangular_solve(tri, rhs, /*upper=*/false, /*unitriangular=*/false).sum().reverse();
  REQUIRE(tri.sensitivity().platform() == pluss::pMPS);
  REQUIRE(rhs.sensitivity().platform() == pluss::pMPS);
  require_close(vec_f32(rhs.sensitivity()), {0.33333334f, 0.66666669f}, 5.0e-5f);
  require_close(vec_f32(tri.sensitivity()), {-0.16666667f, 0.0f, -0.33333334f, -0.77777779f}, 5.0e-5f);

  pluss::Series chol = pluss::make_series(std::vector<float>{2.0f, 0.0f, 0.5f, 1.5f}, {2, 2}, cfg);
  pluss::Series chol_rhs = pluss::make_series(std::vector<float>{1.0f, 2.0f}, {2, 1}, cfg);
  chol.tracks_slope_(true);
  chol_rhs.tracks_slope_(true);
  pluss::subsp::linearalg::cholesky_solve(chol, chol_rhs, /*upper=*/false).sum().reverse();
  REQUIRE(chol.sensitivity().platform() == pluss::pMPS);
  REQUIRE(chol_rhs.sensitivity().platform() == pluss::pMPS);
  require_close(vec_f32(chol_rhs.sensitivity()), {0.16666667f, 0.33333334f}, 5.0e-5f);
  require_close(vec_f32(chol.sensitivity()), {-0.11111111f, -0.22222222f, -0.55555558f, -0.77777779f}, 5.0e-5f);
}

TEST_CASE("MPS QR and SVD backward paths match CPU reference gradients", "[mps][autodiff][linalg][decomp]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable");
    return;
  }

  const ScopedEnvVar no_fallback("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const ScopedEnvVar native_linalg("PLUSS_MPS_EXPERIMENTAL_NATIVE_LINALG", "1");
  const ScopedEnvVar native_qr("PLUSS_MPS_LINALG_NATIVE_DEVICE_QR", "1");
  const ScopedEnvVar native_svd("PLUSS_MPS_LINALG_NATIVE_DEVICE_SVD", "1");
  const pluss::Config cpu_cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU);
  const pluss::Config mps_cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pMPS);

  const std::vector<float> qr_values{2.0f, 0.5f, 0.25f, 1.5f, 1.0f, -0.75f};
  pluss::Series qr_cpu = pluss::make_series(qr_values, {3, 2}, cpu_cfg);
  pluss::Series qr_mps = pluss::make_series(qr_values, {3, 2}, mps_cfg);
  qr_cpu.tracks_slope_(true);
  qr_mps.tracks_slope_(true);
  pluss::subsp::linearalg::qr_q(qr_cpu, /*reduced=*/true)
      .matmul(pluss::subsp::linearalg::qr_r(qr_cpu, /*reduced=*/true))
      .sum()
      .reverse();
  pluss::subsp::linearalg::qr_q(qr_mps, /*reduced=*/true)
      .matmul(pluss::subsp::linearalg::qr_r(qr_mps, /*reduced=*/true))
      .sum()
      .reverse();
  REQUIRE(qr_mps.sensitivity().platform() == pluss::pMPS);
  require_close(vec_f32(qr_mps.sensitivity()), vec_f32(qr_cpu.sensitivity()), 7.5e-3f);
  require_close(vec_f32(qr_mps.sensitivity()), std::vector<float>(qr_values.size(), 1.0f), 7.5e-3f);

  const std::vector<float> qr_wide_values{2.0f, 0.5f, -0.25f, 1.5f, 0.75f, 1.0f};
  pluss::Series qr_wide_cpu = pluss::make_series(qr_wide_values, {2, 3}, cpu_cfg);
  pluss::Series qr_wide_mps = pluss::make_series(qr_wide_values, {2, 3}, mps_cfg);
  qr_wide_cpu.tracks_slope_(true);
  qr_wide_mps.tracks_slope_(true);
  pluss::subsp::linearalg::qr_q(qr_wide_cpu, /*reduced=*/true)
      .matmul(pluss::subsp::linearalg::qr_r(qr_wide_cpu, /*reduced=*/true))
      .sum()
      .reverse();
  pluss::subsp::linearalg::qr_q(qr_wide_mps, /*reduced=*/true)
      .matmul(pluss::subsp::linearalg::qr_r(qr_wide_mps, /*reduced=*/true))
      .sum()
      .reverse();
  REQUIRE(qr_wide_mps.sensitivity().platform() == pluss::pMPS);
  require_close(vec_f32(qr_wide_mps.sensitivity()), vec_f32(qr_wide_cpu.sensitivity()), 1.0e-2f);
  require_close(vec_f32(qr_wide_mps.sensitivity()), std::vector<float>(qr_wide_values.size(), 1.0f), 1.0e-2f);

  const std::vector<float> svd_values{2.0f, 0.5f, 0.25f, 1.5f};
  pluss::Series svd_cpu_s = pluss::make_series(svd_values, {2, 2}, cpu_cfg);
  pluss::Series svd_mps_s = pluss::make_series(svd_values, {2, 2}, mps_cfg);
  svd_cpu_s.tracks_slope_(true);
  svd_mps_s.tracks_slope_(true);
  pluss::subsp::linearalg::svd_s(svd_cpu_s, /*full_matrices=*/false).sum().reverse();
  pluss::subsp::linearalg::svd_s(svd_mps_s, /*full_matrices=*/false).sum().reverse();
  REQUIRE(svd_mps_s.sensitivity().platform() == pluss::pMPS);
  require_close(vec_f32(svd_mps_s.sensitivity()), vec_f32(svd_cpu_s.sensitivity()), 5.0e-3f);

  pluss::Series svd_cpu_recon = pluss::make_series(svd_values, {2, 2}, cpu_cfg);
  pluss::Series svd_mps_recon = pluss::make_series(svd_values, {2, 2}, mps_cfg);
  svd_cpu_recon.tracks_slope_(true);
  svd_mps_recon.tracks_slope_(true);
  pluss::subsp::linearalg::svd_u(svd_cpu_recon, /*full_matrices=*/false)
      .mul(pluss::subsp::linearalg::svd_s(svd_cpu_recon, /*full_matrices=*/false).unsqueeze(0))
      .matmul(pluss::subsp::linearalg::svd_vh(svd_cpu_recon, /*full_matrices=*/false))
      .sum()
      .reverse();
  pluss::subsp::linearalg::svd_u(svd_mps_recon, /*full_matrices=*/false)
      .mul(pluss::subsp::linearalg::svd_s(svd_mps_recon, /*full_matrices=*/false).unsqueeze(0))
      .matmul(pluss::subsp::linearalg::svd_vh(svd_mps_recon, /*full_matrices=*/false))
      .sum()
      .reverse();
  REQUIRE(svd_mps_recon.sensitivity().platform() == pluss::pMPS);
  require_close(vec_f32(svd_mps_recon.sensitivity()), vec_f32(svd_cpu_recon.sensitivity()), 5.0e-3f);

  const std::vector<float> svd_wide_values{2.0f, 0.5f, -0.25f, 1.5f, 0.75f, 1.0f};
  pluss::Series svd_wide_cpu = pluss::make_series(svd_wide_values, {2, 3}, cpu_cfg);
  pluss::Series svd_wide_mps = pluss::make_series(svd_wide_values, {2, 3}, mps_cfg);
  svd_wide_cpu.tracks_slope_(true);
  svd_wide_mps.tracks_slope_(true);
  pluss::subsp::linearalg::svd_u(svd_wide_cpu, /*full_matrices=*/false)
      .mul(pluss::subsp::linearalg::svd_s(svd_wide_cpu, /*full_matrices=*/false).unsqueeze(0))
      .matmul(pluss::subsp::linearalg::svd_vh(svd_wide_cpu, /*full_matrices=*/false))
      .sum()
      .reverse();
  pluss::subsp::linearalg::svd_u(svd_wide_mps, /*full_matrices=*/false)
      .mul(pluss::subsp::linearalg::svd_s(svd_wide_mps, /*full_matrices=*/false).unsqueeze(0))
      .matmul(pluss::subsp::linearalg::svd_vh(svd_wide_mps, /*full_matrices=*/false))
      .sum()
      .reverse();
  REQUIRE(svd_wide_mps.sensitivity().platform() == pluss::pMPS);
  require_close(vec_f32(svd_wide_mps.sensitivity()), vec_f32(svd_wide_cpu.sensitivity()), 7.5e-3f);

  pluss::Series grouped_cpu = pluss::make_series(svd_wide_values, {2, 3}, cpu_cfg);
  pluss::Series grouped_mps = pluss::make_series(svd_wide_values, {2, 3}, mps_cfg);
  grouped_cpu.tracks_slope_(true);
  grouped_mps.tracks_slope_(true);
  const auto grouped_cpu_svd = pluss::subsp::linearalg::svd(grouped_cpu, /*full_matrices=*/false);
  const auto grouped_mps_svd = grouped_mps.linearalg.svd(/*full_matrices=*/false);
  REQUIRE(grouped_mps_svd.u.platform() == pluss::pMPS);
  REQUIRE(grouped_mps_svd.s.platform() == pluss::pMPS);
  REQUIRE(grouped_mps_svd.vh.platform() == pluss::pMPS);
  grouped_cpu_svd.u.mul(grouped_cpu_svd.s.unsqueeze(0)).matmul(grouped_cpu_svd.vh).sum().reverse();
  grouped_mps_svd.u.mul(grouped_mps_svd.s.unsqueeze(0)).matmul(grouped_mps_svd.vh).sum().reverse();
  REQUIRE(grouped_mps.sensitivity().platform() == pluss::pMPS);
  require_close(vec_f32(grouped_mps.sensitivity()), vec_f32(grouped_cpu.sensitivity()), 5.0e-3f);
  require_close(vec_f32(grouped_mps.sensitivity()), std::vector<float>(svd_wide_values.size(), 1.0f), 5.0e-3f);
}

TEST_CASE("MPS complex64 elementwise ops stay native under strict fallback", "[mps][complex][elementwise]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable");
    return;
  }

  using C = pluss::codem::Complex64;
  const ScopedEnvVar no_fallback("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const pluss::Config cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Complex64).with_platform(pluss::pMPS);
  const pluss::Series z = pluss::make_series(std::vector<C>{C{1.0f, 2.0f}, C{-3.0f, 0.5f}}, {2}, cfg);
  const pluss::Series w = pluss::make_series(std::vector<C>{C{0.5f, -1.0f}, C{2.0f, 3.0f}}, {2}, cfg);

  const pluss::Series conj = pluss::codem::_compat::invoke("conj", z);
  REQUIRE(conj.platform() == pluss::pMPS);
  REQUIRE(conj.scalar_type() == pluss::ScalarType::Complex64);
  require_close_complex64(
      conj.to(pluss::pCPU).to_vec<C>(),
      {C{1.0f, -2.0f}, C{-3.0f, -0.5f}});

  const pluss::Series real_part = pluss::codem::_compat::invoke("real", z);
  REQUIRE(real_part.platform() == pluss::pMPS);
  REQUIRE(real_part.scalar_type() == pluss::ScalarType::Float32);
  require_close(vec_f32(real_part), {1.0f, -3.0f});

  const pluss::Series imag_part = pluss::codem::_compat::invoke("imag", z);
  REQUIRE(imag_part.platform() == pluss::pMPS);
  REQUIRE(imag_part.scalar_type() == pluss::ScalarType::Float32);
  require_close(vec_f32(imag_part), {2.0f, 0.5f});

  const pluss::Series mag = pluss::codem::_compat::invoke("abs", z);
  REQUIRE(mag.platform() == pluss::pMPS);
  REQUIRE(mag.scalar_type() == pluss::ScalarType::Float32);
  require_close(vec_f32(mag), {std::sqrt(5.0f), std::sqrt(9.25f)}, 1.0e-5f);

  const pluss::Series phase = pluss::codem::_compat::invoke("angle", z);
  REQUIRE(phase.platform() == pluss::pMPS);
  REQUIRE(phase.scalar_type() == pluss::ScalarType::Float32);
  require_close(vec_f32(phase), {std::atan2(2.0f, 1.0f), std::atan2(0.5f, -3.0f)}, 1.0e-5f);

  pluss::Series real_leaf = z.clone();
  real_leaf.tracks_slope_(true);
  pluss::codem::_compat::invoke("real", real_leaf).sum().reverse();
  REQUIRE(real_leaf.sensitivity().platform() == pluss::pMPS);
  require_close_complex64(real_leaf.sensitivity().to(pluss::pCPU).to_vec<C>(), {C{1.0f, 0.0f}, C{1.0f, 0.0f}});

  pluss::Series imag_leaf = z.clone();
  imag_leaf.tracks_slope_(true);
  pluss::codem::_compat::invoke("imag", imag_leaf).sum().reverse();
  REQUIRE(imag_leaf.sensitivity().platform() == pluss::pMPS);
  require_close_complex64(imag_leaf.sensitivity().to(pluss::pCPU).to_vec<C>(), {C{0.0f, 1.0f}, C{0.0f, 1.0f}});

  pluss::Series abs_leaf = z.clone();
  abs_leaf.tracks_slope_(true);
  pluss::codem::_compat::invoke("abs", abs_leaf).sum().reverse();
  REQUIRE(abs_leaf.sensitivity().platform() == pluss::pMPS);
  require_close_complex64(
      abs_leaf.sensitivity().to(pluss::pCPU).to_vec<C>(),
      {C{1.0f / std::sqrt(5.0f), 2.0f / std::sqrt(5.0f)},
       C{-3.0f / std::sqrt(9.25f), 0.5f / std::sqrt(9.25f)}},
      1.0e-5f);

  pluss::Series angle_leaf = z.clone();
  angle_leaf.tracks_slope_(true);
  pluss::codem::_compat::invoke("angle", angle_leaf).sum().reverse();
  REQUIRE(angle_leaf.sensitivity().platform() == pluss::pMPS);
  require_close_complex64(
      angle_leaf.sensitivity().to(pluss::pCPU).to_vec<C>(),
      {C{-2.0f / 5.0f, 1.0f / 5.0f}, C{-0.5f / 9.25f, -3.0f / 9.25f}},
      1.0e-5f);

  pluss::Series zero_leaf =
      pluss::make_series(std::vector<C>{C{0.0f, 0.0f}, C{1.0f, -1.0f}}, {2}, cfg);
  zero_leaf.tracks_slope_(true);
  pluss::codem::_compat::invoke("abs", zero_leaf).sum().reverse();
  REQUIRE(zero_leaf.sensitivity().platform() == pluss::pMPS);
  require_close_complex64(
      zero_leaf.sensitivity().to(pluss::pCPU).to_vec<C>(),
      {C{0.0f, 0.0f}, C{1.0f / std::sqrt(2.0f), -1.0f / std::sqrt(2.0f)}},
      1.0e-5f);

  pluss::Series zero_angle_leaf =
      pluss::make_series(std::vector<C>{C{0.0f, 0.0f}, C{1.0f, -1.0f}}, {2}, cfg);
  zero_angle_leaf.tracks_slope_(true);
  pluss::codem::_compat::invoke("angle", zero_angle_leaf).sum().reverse();
  REQUIRE(zero_angle_leaf.sensitivity().platform() == pluss::pMPS);
  require_close_complex64(
      zero_angle_leaf.sensitivity().to(pluss::pCPU).to_vec<C>(),
      {C{0.0f, 0.0f}, C{0.5f, 0.5f}},
      1.0e-5f);

  const pluss::Series add = z.add(w);
  REQUIRE(add.platform() == pluss::pMPS);
  require_close_complex64(
      add.to(pluss::pCPU).to_vec<C>(),
      {C{1.5f, 1.0f}, C{-1.0f, 3.5f}});

  const pluss::Series mul = z.mul(w);
  REQUIRE(mul.platform() == pluss::pMPS);
  require_close_complex64(
      mul.to(pluss::pCPU).to_vec<C>(),
      {C{2.5f, 0.0f}, C{-7.5f, -8.0f}});

  const pluss::Series eq = z.eq(z.clone());
  REQUIRE(eq.platform() == pluss::pMPS);
  REQUIRE(eq.scalar_type() == pluss::ScalarType::Bool);
  REQUIRE(eq.to(pluss::pCPU).to_vec<bool>() == std::vector<bool>{true, true});

  const pluss::Series zm = z.reshape({1, 2});
  const pluss::Series wm = w.reshape({2, 1});
  const pluss::Series mm = zm.matmul(wm);
  REQUIRE(mm.platform() == pluss::pMPS);
  REQUIRE(mm.scalar_type() == pluss::ScalarType::Complex64);
  require_close_complex64(mm.to(pluss::pCPU).to_vec<C>(), {C{-5.0f, -8.0f}}, 1.0e-5f);

  pluss::Series zm_grad = z.reshape({1, 2}).clone();
  pluss::Series wm_grad = w.reshape({2, 1}).clone();
  zm_grad.tracks_slope_(true);
  wm_grad.tracks_slope_(true);
  zm_grad.matmul(wm_grad).reverse();
  REQUIRE(zm_grad.sensitivity().platform() == pluss::pMPS);
  REQUIRE(wm_grad.sensitivity().platform() == pluss::pMPS);
  require_close_complex64(
      zm_grad.sensitivity().to(pluss::pCPU).reshape({2}).to_vec<C>(),
      {std::conj(C{0.5f, -1.0f}), std::conj(C{2.0f, 3.0f})},
      1.0e-5f);
  require_close_complex64(
      wm_grad.sensitivity().to(pluss::pCPU).reshape({2}).to_vec<C>(),
      {std::conj(C{1.0f, 2.0f}), std::conj(C{-3.0f, 0.5f})},
      1.0e-5f);

  pluss::Series elem_mul_z = z.clone();
  pluss::Series elem_mul_w = w.clone();
  elem_mul_z.tracks_slope_(true);
  elem_mul_w.tracks_slope_(true);
  elem_mul_z.mul(elem_mul_w).reverse(pluss::make_series(std::vector<C>{C{1.0f, 0.0f}, C{1.0f, 0.0f}}, {2}, cfg));
  require_close_complex64(
      elem_mul_z.sensitivity().to(pluss::pCPU).to_vec<C>(),
      {std::conj(C{0.5f, -1.0f}), std::conj(C{2.0f, 3.0f})},
      1.0e-5f);
  require_close_complex64(
      elem_mul_w.sensitivity().to(pluss::pCPU).to_vec<C>(),
      {std::conj(C{1.0f, 2.0f}), std::conj(C{-3.0f, 0.5f})},
      1.0e-5f);

  pluss::Series elem_div_z = z.clone();
  pluss::Series elem_div_w = w.clone();
  elem_div_z.tracks_slope_(true);
  elem_div_w.tracks_slope_(true);
  elem_div_z.div(elem_div_w).reverse(pluss::make_series(std::vector<C>{C{1.0f, 0.0f}, C{1.0f, 0.0f}}, {2}, cfg));
  const C cw0 = std::conj(C{0.5f, -1.0f});
  const C cw1 = std::conj(C{2.0f, 3.0f});
  require_close_complex64(
      elem_div_z.sensitivity().to(pluss::pCPU).to_vec<C>(),
      {C{1.0f, 0.0f} / cw0, C{1.0f, 0.0f} / cw1},
      1.0e-5f);
  require_close_complex64(
      elem_div_w.sensitivity().to(pluss::pCPU).to_vec<C>(),
      {-std::conj(C{1.0f, 2.0f}) / (cw0 * cw0), -std::conj(C{-3.0f, 0.5f}) / (cw1 * cw1)},
      1.0e-5f);
}

TEST_CASE("MPS complex64 small linalg keeps complex lanes and reverse-mode", "[mps][complex][linalg][autodiff]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable");
    return;
  }

  using C = pluss::codem::Complex64;
  const ScopedEnvVar no_fallback("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const ScopedEnvVar native_linalg("PLUSS_MPS_EXPERIMENTAL_NATIVE_LINALG", "1");
  const pluss::Config cpu_cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Complex64).with_platform(pluss::pCPU);
  const pluss::Config mps_cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Complex64).with_platform(pluss::pMPS);

  const std::vector<C> values{C{2.0f, 1.0f}, C{0.5f, -0.25f}, C{-0.75f, 0.5f}, C{1.5f, -0.5f}};
  const C expected_det = values[0] * values[3] - values[1] * values[2];
  pluss::Series a_cpu = pluss::make_series(values, {2, 2}, cpu_cfg);
  pluss::Series a_mps = pluss::make_series(values, {2, 2}, mps_cfg);

  const pluss::Series det_mps = pluss::subsp::linearalg::det(a_mps);
  REQUIRE(det_mps.platform() == pluss::pMPS);
  REQUIRE(det_mps.scalar_type() == pluss::ScalarType::Complex64);
  require_close_complex64(det_mps.to(pluss::pCPU).to_vec<C>(), {expected_det}, 1.0e-5f);

  const pluss::Series inv_mps = pluss::subsp::linearalg::inverse(a_mps);
  REQUIRE(inv_mps.platform() == pluss::pMPS);
  REQUIRE(inv_mps.scalar_type() == pluss::ScalarType::Complex64);
  require_close_complex64(
      inv_mps.matmul(a_mps).to(pluss::pCPU).to_vec<C>(),
      {C{1.0f, 0.0f}, C{0.0f, 0.0f}, C{0.0f, 0.0f}, C{1.0f, 0.0f}},
      2.0e-5f);

  pluss::Series det_cpu_leaf = pluss::make_series(values, {2, 2}, cpu_cfg);
  pluss::Series det_mps_leaf = pluss::make_series(values, {2, 2}, mps_cfg);
  det_cpu_leaf.tracks_slope_(true);
  det_mps_leaf.tracks_slope_(true);
  pluss::subsp::linearalg::det(det_cpu_leaf).reverse();
  pluss::subsp::linearalg::det(det_mps_leaf).reverse();
  REQUIRE(det_mps_leaf.sensitivity().platform() == pluss::pMPS);
  require_close_complex64(
      det_mps_leaf.sensitivity().to(pluss::pCPU).to_vec<C>(),
      det_cpu_leaf.sensitivity().to_vec<C>(),
      2.0e-5f);

  pluss::Series inv_cpu_leaf = pluss::make_series(values, {2, 2}, cpu_cfg);
  pluss::Series inv_mps_leaf = pluss::make_series(values, {2, 2}, mps_cfg);
  inv_cpu_leaf.tracks_slope_(true);
  inv_mps_leaf.tracks_slope_(true);
  pluss::subsp::linearalg::inverse(inv_cpu_leaf).sum().reverse();
  pluss::subsp::linearalg::inverse(inv_mps_leaf).sum().reverse();
  REQUIRE(inv_mps_leaf.sensitivity().platform() == pluss::pMPS);
  require_close_complex64(
      inv_mps_leaf.sensitivity().to(pluss::pCPU).to_vec<C>(),
      inv_cpu_leaf.sensitivity().to_vec<C>(),
      2.0e-5f);

  const std::vector<C> lower_values{C{2.0f, 0.5f}, C{0.0f, 0.0f}, C{0.5f, -0.25f}, C{1.5f, 0.75f}};
  const std::vector<C> rhs_values{C{1.0f, -1.0f}, C{2.0f, 0.25f}};
  pluss::Series tri_cpu = pluss::make_series(lower_values, {2, 2}, cpu_cfg);
  pluss::Series tri_mps = pluss::make_series(lower_values, {2, 2}, mps_cfg);
  pluss::Series rhs_cpu = pluss::make_series(rhs_values, {2, 1}, cpu_cfg);
  pluss::Series rhs_mps = pluss::make_series(rhs_values, {2, 1}, mps_cfg);
  tri_cpu.tracks_slope_(true);
  tri_mps.tracks_slope_(true);
  rhs_cpu.tracks_slope_(true);
  rhs_mps.tracks_slope_(true);
  pluss::subsp::linearalg::triangular_solve(tri_cpu, rhs_cpu, /*upper=*/false, /*unitriangular=*/false).sum().reverse();
  pluss::subsp::linearalg::triangular_solve(tri_mps, rhs_mps, /*upper=*/false, /*unitriangular=*/false).sum().reverse();
  REQUIRE(tri_mps.sensitivity().platform() == pluss::pMPS);
  REQUIRE(rhs_mps.sensitivity().platform() == pluss::pMPS);
  require_close_complex64(tri_mps.sensitivity().to(pluss::pCPU).to_vec<C>(), tri_cpu.sensitivity().to_vec<C>(), 3.0e-5f);
  require_close_complex64(rhs_mps.sensitivity().to(pluss::pCPU).to_vec<C>(), rhs_cpu.sensitivity().to_vec<C>(), 3.0e-5f);

  pluss::Series chol_cpu = pluss::make_series(lower_values, {2, 2}, cpu_cfg);
  pluss::Series chol_mps = pluss::make_series(lower_values, {2, 2}, mps_cfg);
  pluss::Series chol_rhs_cpu = pluss::make_series(rhs_values, {2, 1}, cpu_cfg);
  pluss::Series chol_rhs_mps = pluss::make_series(rhs_values, {2, 1}, mps_cfg);
  chol_cpu.tracks_slope_(true);
  chol_mps.tracks_slope_(true);
  chol_rhs_cpu.tracks_slope_(true);
  chol_rhs_mps.tracks_slope_(true);
  pluss::subsp::linearalg::cholesky_solve(chol_cpu, chol_rhs_cpu, /*upper=*/false).sum().reverse();
  pluss::subsp::linearalg::cholesky_solve(chol_mps, chol_rhs_mps, /*upper=*/false).sum().reverse();
  REQUIRE(chol_mps.sensitivity().platform() == pluss::pMPS);
  REQUIRE(chol_rhs_mps.sensitivity().platform() == pluss::pMPS);
  require_close_complex64(chol_mps.sensitivity().to(pluss::pCPU).to_vec<C>(), chol_cpu.sensitivity().to_vec<C>(), 5.0e-5f);
  require_close_complex64(
      chol_rhs_mps.sensitivity().to(pluss::pCPU).to_vec<C>(),
      chol_rhs_cpu.sensitivity().to_vec<C>(),
      5.0e-5f);
}

TEST_CASE("MPS complex64 index put accumulate stays native under strict fallback", "[mps][complex][indexing][accumulate]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable");
    return;
  }

  using C = pluss::codem::Complex64;
  const ScopedEnvVar no_fallback("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const pluss::Config c64_mps = pluss::Config{}.with_dtype(pluss::ScalarType::Complex64).with_platform(pluss::pMPS);
  const pluss::Config i64_mps = pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(pluss::pMPS);

  pluss::Series x = pluss::make_series(std::vector<C>{C{0.0f, 0.0f}, C{0.0f, 0.0f}, C{0.0f, 0.0f}}, {3}, c64_mps);
  const pluss::Series idx = pluss::make_series(std::vector<std::int64_t>{1, 1}, {2}, i64_mps);
  const pluss::Series values = pluss::make_series(std::vector<C>{C{1.0f, 2.0f}, C{3.0f, 4.0f}}, {2}, c64_mps);
  const std::array<pluss::Series, 1> indices{{idx}};
  x.index_put_(pluss::msharp::RangeView::Of<pluss::Series>(indices), values, /*accumulate=*/true);
  REQUIRE(x.platform() == pluss::pMPS);
  require_close_complex64(x.to(pluss::pCPU).to_vec<C>(), {C{0.0f, 0.0f}, C{4.0f, 6.0f}, C{0.0f, 0.0f}});

  pluss::Series y = pluss::make_series(std::vector<C>{C{0.0f, 0.0f}, C{0.0f, 0.0f}, C{0.0f, 0.0f}}, {3}, c64_mps);
  const pluss::Series scalar = pluss::make_series(std::vector<C>{C{1.0f, -1.0f}}, {1}, c64_mps);
  const pluss::Series idx_scalar = pluss::make_series(std::vector<std::int64_t>{0, 0, 2}, {3}, i64_mps);
  const std::array<pluss::Series, 1> scalar_indices{{idx_scalar}};
  y.index_put_(pluss::msharp::RangeView::Of<pluss::Series>(scalar_indices), scalar, /*accumulate=*/true);
  REQUIRE(y.platform() == pluss::pMPS);
  require_close_complex64(y.to(pluss::pCPU).to_vec<C>(), {C{2.0f, -2.0f}, C{0.0f, 0.0f}, C{1.0f, -1.0f}});
}

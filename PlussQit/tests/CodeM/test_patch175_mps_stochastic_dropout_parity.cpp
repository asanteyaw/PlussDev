#include <catch2/catch_test_macros.hpp>

#include <pluss/core.h>
#include "CodeM/compat/Invoke.h"
#include "CodeM/native/mps/Stochastic.h"

#include <cstdlib>
#include <cmath>
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

  pluss::Series ints = pluss::zeros({6}, pluss::Config{}.with_dtype(pluss::ScalarType::Int32).with_platform(pluss::pMPS));
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

  const pluss::Series uniform = pluss::rand({8}, cfg, 777);
  REQUIRE(uniform.platform() == pluss::pMPS);
  REQUIRE(uniform.scalar_type() == pluss::ScalarType::Float32);
  for (const float v : vec_f32(uniform)) {
    REQUIRE(v >= 0.0f);
    REQUIRE(v < 1.0f);
  }

  const pluss::Series uniform_again = pluss::rand({8}, cfg, 777);
  REQUIRE(vec_f32(uniform_again) == vec_f32(uniform));

  const pluss::Series normal = pluss::randn({8}, cfg, 778);
  REQUIRE(normal.platform() == pluss::pMPS);
  REQUIRE(normal.scalar_type() == pluss::ScalarType::Float32);

  const pluss::Series shifted = pluss::normal(2.0, 0.5, {8}, cfg, 779);
  REQUIRE(shifted.platform() == pluss::pMPS);
  REQUIRE(shifted.scalar_type() == pluss::ScalarType::Float32);

  const pluss::Series ints =
      pluss::randint(3, 9, {8}, pluss::Config{}.with_dtype(pluss::ScalarType::Int32).with_platform(pluss::pMPS), 780);
  REQUIRE(ints.platform() == pluss::pMPS);
  REQUIRE(ints.scalar_type() == pluss::ScalarType::Int32);
  for (const float v : vec_f32(ints)) {
    REQUIRE(v >= 3.0f);
    REQUIRE(v < 9.0f);
  }

  const pluss::Series bernoulli = pluss::bernoulli({8}, 0.5, cfg, 781);
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
  REQUIRE(mask.scalar_type() == pluss::ScalarType::Bool);
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
  const pluss::Series x = pluss::ones(shape, cfg);

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

  const pluss::Series rank1 = pluss::ones(std::vector<std::int64_t>{4}, cfg);
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

  x.clear_sensitivity();
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

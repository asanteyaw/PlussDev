#include <catch2/catch_test_macros.hpp>

#include <pluss/core.h>
#include "CodeM/compat/Invoke.h"
#include "CodeM/native/mps/Stochastic.h"

#include <cstdlib>
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

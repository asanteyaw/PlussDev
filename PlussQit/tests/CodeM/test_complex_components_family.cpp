#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cmath>
#include <complex>
#include <cstdint>
#include <numbers>
#include <vector>

namespace {

using C64 = pluss::codem::Complex64;

void require_complex_close(const C64& got, const C64& expected, float tolerance = 1.0e-5f) {
  REQUIRE(std::abs(got.real() - expected.real()) <= tolerance);
  REQUIRE(std::abs(got.imag() - expected.imag()) <= tolerance);
}

pluss::Series sample_complex64() {
  return pluss::series::complex(
      pluss::make_series({{3.0f, -1.0f}, {0.0f, 0.0f}}),
      pluss::make_series({{4.0f, 0.0f}, {2.0f, 0.0f}}));
}

} // namespace

TEST_CASE("complex components preserve shape and paired precision", "[complex-components]") {
  const auto z = sample_complex64();
  const auto real = z.real();
  const auto imag = z.imag();

  REQUIRE(real.sizes() == std::vector<std::int64_t>{2, 2});
  REQUIRE(imag.sizes() == std::vector<std::int64_t>{2, 2});
  REQUIRE(real.scalar_type() == pluss::ScalarType::Float32);
  REQUIRE(imag.scalar_type() == pluss::ScalarType::Float32);
  REQUIRE(real.host<float>() == std::vector<float>{3.0f, -1.0f, 0.0f, 0.0f});
  REQUIRE(imag.host<float>() == std::vector<float>{4.0f, 0.0f, 2.0f, 0.0f});

  const auto rebuilt = pluss::series::complex(real, imag).host<C64>();
  const auto original = z.host<C64>();
  REQUIRE(rebuilt == original);
}

TEST_CASE("real-input component behavior has explicit dtype contracts", "[complex-components]") {
  const auto integers = pluss::make_series({-3, 0, 2});
  REQUIRE(integers.real().scalar_type() == pluss::ScalarType::Int64);
  REQUIRE(integers.imag().scalar_type() == pluss::ScalarType::Int64);
  REQUIRE(integers.imag().host<std::int64_t>() == std::vector<std::int64_t>{0, 0, 0});
  REQUIRE(integers.angle().scalar_type() == pluss::ScalarType::Float32);

  auto real_copy = integers.real();
  real_copy.add_(10);
  REQUIRE(integers.host<std::int64_t>() == std::vector<std::int64_t>{-3, 0, 2});
}

TEST_CASE("complex component reverse rules lift real cotangents", "[complex-components][autodiff]") {
  auto real_leaf = sample_complex64();
  real_leaf.tracks_slope_(true);
  real_leaf.real().sum().reverse();
  for (const auto& value : real_leaf.sensitivity().host<C64>()) {
    require_complex_close(value, C64{1.0f, 0.0f});
  }

  auto imag_leaf = sample_complex64();
  imag_leaf.tracks_slope_(true);
  imag_leaf.imag().sum().reverse();
  for (const auto& value : imag_leaf.sensitivity().host<C64>()) {
    require_complex_close(value, C64{0.0f, 1.0f});
  }
}

TEST_CASE("angle covers quadrants and has the analytic complex reverse", "[complex-components][autodiff]") {
  const auto quadrants = pluss::series::complex(
      pluss::make_series({1.0f, 0.0f, -1.0f, 0.0f, 0.0f}),
      pluss::make_series({0.0f, 1.0f, 0.0f, -1.0f, 0.0f}));
  const auto phase = quadrants.angle().host<float>();
  REQUIRE(std::abs(phase[0]) < 1.0e-6f);
  REQUIRE(std::abs(phase[1] - std::numbers::pi_v<float> / 2.0f) < 1.0e-6f);
  REQUIRE(std::abs(phase[2] - std::numbers::pi_v<float>) < 1.0e-6f);
  REQUIRE(std::abs(phase[3] + std::numbers::pi_v<float> / 2.0f) < 1.0e-6f);
  REQUIRE(phase[4] == 0.0f);

  auto z = pluss::series::complex(pluss::make_series({3.0f}),
                                  pluss::make_series({4.0f}));
  z.tracks_slope_(true);
  z.angle().sum().reverse();
  require_complex_close(z.sensitivity().host<C64>().front(), C64{-0.16f, 0.12f});
}

TEST_CASE("conj and sgn satisfy their geometric identities", "[complex-components][autodiff]") {
  const auto z = sample_complex64();
  REQUIRE(z.conj().conj().host<C64>() == z.host<C64>());

  auto conjugated = sample_complex64();
  conjugated.tracks_slope_(true);
  conjugated.conj().imag().sum().reverse();
  for (const auto& value : conjugated.sensitivity().host<C64>()) {
    require_complex_close(value, C64{0.0f, -1.0f});
  }

  const auto direction = z.sgn();
  const auto modulus = direction.abs().host<float>();
  REQUIRE(std::abs(modulus[0] - 1.0f) < 1.0e-6f);
  REQUIRE(std::abs(modulus[1] - 1.0f) < 1.0e-6f);
  REQUIRE(std::abs(modulus[2] - 1.0f) < 1.0e-6f);
  REQUIRE(modulus[3] == 0.0f);

  auto tracked = pluss::series::complex(pluss::make_series({3.0f}),
                                        pluss::make_series({4.0f}));
  tracked.tracks_slope_(true);
  tracked.sgn().real().sum().reverse();
  REQUIRE(tracked.has_sensitivity());
}

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>

#include <cmath>
#include <complex>
#include <cstdint>
#include <vector>

namespace {

void require_close(const pluss::Series& actual, const std::vector<double>& expected, double margin = 1e-9) {
  const auto values = actual.to_vec<double>();
  REQUIRE(values.size() == expected.size());
  for (std::size_t i = 0; i < values.size(); ++i) {
    REQUIRE(values[i] == Catch::Approx(expected[i]).margin(margin));
  }
}

} // namespace

TEST_CASE("linearalg matrix transformations preserve their matrix contracts", "[linearalg][transform]") {
  const auto jordan = pluss::make_series({{1.0, 2.0}, {0.0, 1.0}});
  require_close(jordan.linearalg.matrix_power(3), {1.0, 6.0, 0.0, 1.0});
  require_close(jordan.linearalg.matrix_power(0), {1.0, 0.0, 0.0, 1.0});
  require_close(jordan.linearalg.matrix_power(-1), {1.0, -2.0, 0.0, 1.0});

  const auto nilpotent = pluss::make_series({{0.0, 1.0}, {0.0, 0.0}});
  require_close(nilpotent.linearalg.matrix_exp(), {1.0, 1.0, 0.0, 1.0});
  const auto float32 = nilpotent.to(pluss::ScalarType::Float32);
  REQUIRE(float32.linearalg.matrix_exp().scalar_type() ==
          pluss::ScalarType::Float32);
  require_close(float32.linearalg.matrix_exp(), {1.0, 1.0, 0.0, 1.0}, 1e-6);
  REQUIRE_THROWS_AS(pluss::make_series({{0, 1}, {0, 0}}).linearalg.matrix_exp(),
                    pluss::Error);

  const auto large = pluss::make_series({{20.0, 0.0}, {0.0, -20.0}});
  require_close(
      large.linearalg.matrix_exp(),
      {std::exp(20.0), 0.0, 0.0, std::exp(-20.0)},
      1e-6);
}

TEST_CASE("linearalg pinverse is SVD-defined for rank loss and rcond", "[linearalg][transform]") {
  const auto rank_one = pluss::make_series({{1.0, 2.0}, {2.0, 4.0}});
  require_close(rank_one.linearalg.pinverse(), {0.04, 0.08, 0.08, 0.16});

  const auto separated = pluss::make_series({{1.0, 0.0}, {0.0, 0.001}});
  require_close(separated.linearalg.pinverse(), {1.0, 0.0, 0.0, 1000.0}, 1e-8);
  require_close(separated.linearalg.pinverse(0.01), {1.0, 0.0, 0.0, 0.0});
  REQUIRE_THROWS_AS(separated.linearalg.pinverse(-0.1), pluss::Error);

  const auto float32 = rank_one.to(pluss::ScalarType::Float32);
  REQUIRE(float32.linearalg.pinverse().scalar_type() ==
          pluss::ScalarType::Float32);
  REQUIRE_THROWS_AS(pluss::make_series({{1, 2}, {3, 4}}).linearalg.pinverse(),
                    pluss::Error);
}

TEST_CASE("linearalg matrix transformation reverse rules include matrix coupling", "[linearalg][transform]") {
  auto power = pluss::make_series({{2.0, 0.0}, {0.0, 3.0}});
  power.tracks_slope_(true);
  power.linearalg.matrix_power(3).sum().reverse();
  require_close(power.sensitivity(), {12.0, 19.0, 19.0, 27.0});

  auto exponential = pluss::make_series({{0.0, 0.0}, {0.0, std::log(2.0)}});
  exponential.tracks_slope_(true);
  exponential.linearalg.matrix_exp().sum().reverse();
  require_close(exponential.sensitivity(), {1.0, 1.0 / std::log(2.0), 1.0 / std::log(2.0), 2.0});

  auto rectangular = pluss::make_series({{1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}});
  rectangular.tracks_slope_(true);
  rectangular.linearalg.pinverse().sum().reverse();
  require_close(
      rectangular.sensitivity(),
      {-1.0 / 9.0, -1.0 / 9.0, -1.0 / 9.0, -1.0 / 9.0, -5.0 / 9.0, -5.0 / 9.0});
}

TEST_CASE("linearalg matrix transformations preserve batch tails", "[linearalg][transform]") {
  const auto square_batch = pluss::make_series({
      {{1.0, 1.0}, {0.0, 1.0}},
      {{2.0, 0.0}, {0.0, 3.0}},
  });
  REQUIRE(square_batch.linearalg.matrix_power(2).sizes() == std::vector<std::int64_t>{2, 2, 2});
  REQUIRE(square_batch.linearalg.matrix_exp().sizes() == std::vector<std::int64_t>{2, 2, 2});

  const auto rectangular_batch = pluss::make_series({
      {{1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}},
      {{2.0, 0.0}, {0.0, 1.0}, {0.0, 1.0}},
  });
  REQUIRE(rectangular_batch.linearalg.pinverse().sizes() == std::vector<std::int64_t>{2, 2, 3});
}

TEST_CASE("linearalg eigh supports batched real symmetric and complex "
          "Hermitian input",
          "[linearalg][eigh][complex]") {
  const auto real = pluss::make_series({
      {{2.0, 0.0}, {0.0, -1.0}},
      {{3.0, 1.0}, {1.0, 3.0}},
  });
  const auto real_result = real.linearalg.eigh();
  REQUIRE(real_result.values.sizes() == std::vector<std::int64_t>{2, 2});
  REQUIRE(real_result.vectors.sizes() == std::vector<std::int64_t>{2, 2, 2});
  require_close(real_result.values, {-1.0, 2.0, 2.0, 4.0});

  using Complex = std::complex<double>;
  const auto hermitian = pluss::make_series(std::vector<Complex>{{0.0, 0.0},
                                                                 {0.0, -1.0},
                                                                 {0.0, 1.0},
                                                                 {0.0, 0.0},
                                                                 {0.0, 0.0},
                                                                 {0.0, -2.0},
                                                                 {0.0, 2.0},
                                                                 {0.0, 0.0}},
                                            {2, 2, 2});
  const auto result = hermitian.linearalg.eigh();
  require_close(result.values, {-1.0, 1.0, -2.0, 2.0});
  const auto rebuilt = result.vectors.mul(result.values.unsqueeze(-2))
                           .matmul(result.vectors.adjoint());
  REQUIRE(rebuilt.allclose(hermitian, 1e-11, 1e-11));

  const auto complex64 = hermitian.to(pluss::ScalarType::Complex64);
  const auto result64 = complex64.linearalg.eigh();
  REQUIRE(result64.values.scalar_type() == pluss::ScalarType::Float32);
  REQUIRE(result64.vectors.scalar_type() == pluss::ScalarType::Complex64);
  REQUIRE_THROWS_AS(pluss::make_series({1.0, 2.0}).linearalg.eigh(),
                    pluss::Error);
}

TEST_CASE("linearalg gen_eigh solves matrix-free generalized Hermitian systems",
          "[linearalg][gen_eigh]") {
  const auto initial =
      pluss::make_series({{1.0, 1.0}, {1.0, -1.0}});
  const auto a = pluss::make_series({2.0, 8.0});
  const auto b = pluss::make_series({1.0, 2.0});
  const auto result = initial.linearalg.gen_eigh(
      [&](const pluss::Series &x) { return x.mul(a); },
      [&](const pluss::Series &x) { return x.mul(b); },
      [&](const pluss::Series &rhs) { return rhs.div(a); });

  require_close(result.values, {2.0, 4.0}, 1e-10);
  REQUIRE(result.vectors.sizes() == std::vector<std::int64_t>{2, 2});
  REQUIRE(result.residual.max().item<double>() < 1e-10);
  REQUIRE(result.converged.all().item<bool>());

  const auto lhs = result.vectors.mul(a);
  const auto rhs = result.vectors.mul(b).mul(result.values.unsqueeze(-1));
  REQUIRE(lhs.allclose(rhs, 1e-10, 1e-10));

  const auto tiny_a = a.mul(1e-20);
  const auto tiny_b = b.mul(1e-20);
  const auto scaled = initial.linearalg.gen_eigh(
      [&](const pluss::Series &x) { return x.mul(tiny_a); },
      [&](const pluss::Series &x) { return x.mul(tiny_b); },
      [&](const pluss::Series &value) { return value.div(tiny_a); });
  require_close(scaled.values, {2.0, 4.0}, 1e-10);
  REQUIRE(scaled.converged.all().item<bool>());
}

TEST_CASE("linearalg gen_eigh preserves batches dtype and reverse history",
          "[linearalg][gen_eigh][autodiff]") {
  auto stiffness =
      pluss::make_series(std::vector<double>{2.0, 8.0, 8.0, 32.0}, {2, 2});
  stiffness.tracks_slope_(true);
  const auto mass = pluss::make_series({1.0, 2.0});
  const auto initial =
      pluss::make_series({{1.0, 1.0}, {1.0, -1.0}})
          .unsqueeze(0)
          .expand({2, 2, 2}, true);
  const auto result = initial.linearalg.gen_eigh(
      [&](const pluss::Series &x) { return x.mul(stiffness.unsqueeze(-2)); },
      [&](const pluss::Series &x) { return x.mul(mass); },
      [&](const pluss::Series &rhs) {
        return rhs.div(stiffness.unsqueeze(-2));
      });

  require_close(result.values, {2.0, 4.0, 8.0, 16.0}, 2e-9);
  result.values.sum().reverse();
  require_close(stiffness.sensitivity(), {1.0, 0.5, 1.0, 0.5}, 2e-8);

  const auto initial32 = initial.to(pluss::ScalarType::Float32);
  const auto a32 = stiffness.detach().to(pluss::ScalarType::Float32);
  const auto b32 = mass.to(pluss::ScalarType::Float32);
  const auto result32 = initial32.linearalg.gen_eigh(
      [&](const pluss::Series &x) { return x.mul(a32.unsqueeze(-2)); },
      [&](const pluss::Series &x) { return x.mul(b32); },
      [&](const pluss::Series &rhs) { return rhs.div(a32.unsqueeze(-2)); });
  REQUIRE(result32.values.scalar_type() == pluss::ScalarType::Float32);
  REQUIRE(result32.vectors.scalar_type() == pluss::ScalarType::Float32);
}

TEST_CASE("linearalg gen_eigh uses Hermitian products for complex operators",
          "[linearalg][gen_eigh][complex]") {
  using Complex = std::complex<double>;
  const auto initial = pluss::make_series(
      std::vector<Complex>{{1.0, 0.0}, {1.0, 0.0},
                           {1.0, 0.0}, {0.0, 1.0}},
      {2, 2});
  const auto a = pluss::make_series(
      std::vector<Complex>{{3.0, 0.0}, {0.0, 1.0},
                           {0.0, -1.0}, {3.0, 0.0}},
      {2, 2});
  const auto b = pluss::make_series(
      std::vector<Complex>{{1.0, 0.0}, {0.0, 0.0},
                           {0.0, 0.0}, {1.0, 0.0}},
      {2, 2});
  const auto result = initial.linearalg.gen_eigh(
      [&](const pluss::Series &x) {
        return a.linearalg.mm(x.transpose(-2, -1)).transpose(-2, -1);
      },
      [&](const pluss::Series &x) {
        return b.linearalg.mm(x.transpose(-2, -1)).transpose(-2, -1);
      },
      [&](const pluss::Series &rhs) {
        return a.linearalg.lu_solve(rhs.transpose(-2, -1))
            .transpose(-2, -1);
      });
  require_close(result.values, {2.0, 4.0}, 1e-10);
  REQUIRE(result.vectors.scalar_type() == pluss::ScalarType::Complex128);
  REQUIRE(result.converged.all().item<bool>());
}

TEST_CASE("linearalg gen_eigh fails on invalid operator contracts",
          "[linearalg][gen_eigh][validation]") {
  const auto initial =
      pluss::make_series({{1.0, 1.0}, {1.0, -1.0}});
  const auto diagonal = pluss::make_series({2.0, 8.0});
  REQUIRE_THROWS_AS(
      pluss::make_series({{1.0, 0.0}, {2.0, 0.0}})
          .linearalg.gen_eigh(
              [&](const pluss::Series &x) { return x.mul(diagonal); },
              [&](const pluss::Series &x) { return x; },
              [&](const pluss::Series &x) { return x.div(diagonal); }),
      std::invalid_argument);
  REQUIRE_THROWS_AS(
      initial.linearalg.gen_eigh(
          [&](const pluss::Series &x) { return x.select(-1, 0); },
          [&](const pluss::Series &x) { return x; },
          [&](const pluss::Series &x) { return x.div(diagonal); }),
      std::invalid_argument);
  REQUIRE_THROWS_AS(
      pluss::make_series({1.0, 2.0}).linearalg.gen_eigh(
          [&](const pluss::Series &x) { return x; },
          [&](const pluss::Series &x) { return x; },
          [&](const pluss::Series &x) { return x; }),
      std::invalid_argument);
  REQUIRE_THROWS_AS(
      pluss::series::zeros({0, 2, 2}).linearalg.gen_eigh(
          [&](const pluss::Series &x) { return x; },
          [&](const pluss::Series &x) { return x; },
          [&](const pluss::Series &x) { return x; }),
      std::invalid_argument);
}
